/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "fluid_sys.h"

#include <chrono>
#include <condition_variable>
#include <exception>
#ifndef FLUIDSYNTH_WATER
#include <map>
#endif
#include <mutex>
#include <new>
#include <thread>

#if defined(FLUIDSYNTH_WATER) && defined(_WIN32)
#include <windows.h>
#endif

static std::mutex atomic_lock;
fluid_mutex_t _atomic_lock = &atomic_lock;

#if defined(FLUIDSYNTH_WATER) && defined(_WIN32)
struct water_private_key
{
    DWORD tls_index;
};
#else
static thread_local std::map<fluid_private_t, void *> private_data;
#endif


void fluid_msleep(unsigned int msecs)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
}

double fluid_utime()
{
    auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

static void thread_wrapper(fluid_thread_func_t func, void *data)
{
    try
    {
        func(data);
    }
    catch (...)
    {
        FLUID_LOG(FLUID_ERR, "Exception thrown in thread function");
    }
}

fluid_thread_t *
new_fluid_thread(const char *name, fluid_thread_func_t func, void *data, int prio_level, int detach)
{
    if (func == nullptr)
        return nullptr;

    fluid_thread_info_t *info = nullptr;

    try
    {
        if (prio_level > 0)
        {
            /*
             * fluid_thread_high_prio() releases this with FLUID_FREE().
             * Keep the allocator pair consistent; using C++ new here corrupts
             * the heap when the high-priority thread exits.
             */
            info = FLUID_NEW(fluid_thread_info_t);
            if (info == nullptr)
            {
                FLUID_LOG(FLUID_PANIC, "Out of memory on high-priority thread allocation");
                return nullptr;
            }
            info->func = func;
            info->data = data;
            info->prio_level = prio_level;

            func = fluid_thread_high_prio;
            data = info;
        }

        std::thread *thread = new std::thread(thread_wrapper, func, data);
        if (detach)
            thread->detach();

        return thread;
    }
    catch (const std::bad_alloc &)
    {
        FLUID_LOG(FLUID_PANIC, "Out of memory on thread allocation");
    }
    catch (...)
    {
        FLUID_LOG(FLUID_ERR, "Failed to create thread");
    }

    FLUID_FREE(info);
    return nullptr;
}

void delete_fluid_thread(fluid_thread_t *_thread)
{
    std::thread *thread = static_cast<std::thread *>(_thread);

    if (thread == nullptr)
        return;

    if (thread->joinable())
    {
        try
        {
            if (thread->get_id() != std::this_thread::get_id())
                FLUID_LOG(FLUID_WARN, "deleting a joinable thread; detaching it");

            /*
             * Destroying a joinable std::thread calls std::terminate().
             * delete_fluid_thread() is documented as releasing the thread
             * object, not as joining or stopping the underlying thread.
             */
            thread->detach();
        }
        catch (const std::exception &exc)
        {
            /*
             * Do not delete a still-joinable std::thread after detach failed;
             * that would unconditionally terminate the process.
             */
            FLUID_LOG(FLUID_ERR, "Failed to detach thread before deletion: %s", exc.what());
            return;
        }
        catch (...)
        {
            FLUID_LOG(FLUID_ERR, "Failed to detach thread before deletion");
            return;
        }
    }

    delete thread;
}

int fluid_thread_join(fluid_thread_t *_thread)
{
    std::thread *thread = static_cast<std::thread *>(_thread);

    if (thread == nullptr || !thread->joinable())
    {
        FLUID_LOG(FLUID_ERR, "cannot join a null or detached thread");
        return FLUID_FAILED;
    }

    try
    {
        thread->join();
    }
    catch (const std::exception &exc)
    {
        FLUID_LOG(FLUID_ERR, "Failed to join thread: %s", exc.what());
        return FLUID_FAILED;
    }
    catch (...)
    {
        FLUID_LOG(FLUID_ERR, "Failed to join thread");
        return FLUID_FAILED;
    }

    return FLUID_OK;
}

void _fluid_mutex_init(fluid_mutex_t *mutex)
{
    *mutex = new(std::nothrow) std::mutex();
    if (*mutex == nullptr)
        FLUID_LOG(FLUID_PANIC, "Out of memory on mutex allocation");
}

void fluid_mutex_destroy(fluid_mutex_t mutex)
{
    delete static_cast<std::mutex *>(mutex);
}

template<class T>
static void ensure_lock_mutex(T *mutex)
{
    try
    {
        mutex->lock();
    }
    catch (const std::exception &exc)
    {
        /*
         * Returning without the lock violates every caller's invariant.
         * The previous do/while(false) "retry" returned unlocked after an
         * exception because continue advanced directly to the false test.
         */
        FLUID_LOG(FLUID_PANIC, "Failed to lock C++ mutex: %s", exc.what());
        std::terminate();
    }
    catch (...)
    {
        FLUID_LOG(FLUID_PANIC, "Failed to lock C++ mutex");
        std::terminate();
    }
}

void _fluid_mutex_lock(fluid_mutex_t *mutex)
{
    if (*mutex == nullptr)
    {
        // First use of a statically initialized mutex
        fluid_mutex_lock(_atomic_lock);
        if (*mutex == nullptr)
            _fluid_mutex_init(mutex);
        fluid_mutex_unlock(_atomic_lock);
    }

    ensure_lock_mutex(static_cast<std::mutex *>(*mutex));
}

void fluid_mutex_unlock(fluid_mutex_t mutex)
{
    static_cast<std::mutex *>(mutex)->unlock();
}

void _fluid_rec_mutex_init(fluid_rec_mutex_t *mutex)
{
    *mutex = new(std::nothrow) std::recursive_mutex();
    if (*mutex == nullptr)
        FLUID_LOG(FLUID_PANIC, "Out of memory on recursive mutex allocation");
}

void fluid_rec_mutex_destroy(fluid_rec_mutex_t mutex)
{
    delete static_cast<std::recursive_mutex *>(mutex);
}

void fluid_rec_mutex_lock(fluid_rec_mutex_t mutex)
{
    ensure_lock_mutex(static_cast<std::recursive_mutex *>(mutex));
}

void fluid_rec_mutex_unlock(fluid_rec_mutex_t mutex)
{
    static_cast<std::recursive_mutex *>(mutex)->unlock();
}

void fluid_cond_mutex_lock(fluid_cond_mutex_t *mutex)
{
    ensure_lock_mutex(static_cast<std::mutex *>(mutex));
}

void fluid_cond_mutex_unlock(fluid_cond_mutex_t *mutex)
{
    static_cast<std::mutex *>(mutex)->unlock();
}

fluid_cond_mutex_t *new_fluid_cond_mutex(void)
{
    std::mutex *mutex = new(std::nothrow) std::mutex();
    if (mutex == nullptr)
        FLUID_LOG(FLUID_PANIC, "Out of memory on condition mutex allocation");
    return mutex;
}

void delete_fluid_cond_mutex(fluid_cond_mutex_t *mutex)
{
    delete static_cast<std::mutex *>(mutex);
}

void fluid_cond_signal(fluid_cond_t cond)
{
    static_cast<std::condition_variable *>(cond)->notify_one();
}

void fluid_cond_broadcast(fluid_cond_t cond)
{
    static_cast<std::condition_variable *>(cond)->notify_all();
}

void fluid_cond_wait(fluid_cond_t cond, fluid_cond_mutex_t *mutex)
{
    std::unique_lock<std::mutex> lock(*static_cast<std::mutex *>(mutex), std::adopt_lock);
    static_cast<std::condition_variable *>(cond)->wait(lock);
    lock.release();
}

fluid_cond_t new_fluid_cond(void)
{
    std::condition_variable *cond = new(std::nothrow) std::condition_variable();
    if (cond == nullptr)
        FLUID_LOG(FLUID_PANIC, "Out of memory on condition variable allocation");
    return cond;
}

void delete_fluid_cond(fluid_cond_t cond)
{
    delete static_cast<std::condition_variable *>(cond);
}

void _fluid_private_init(fluid_private_t *priv)
{
#if defined(FLUIDSYNTH_WATER) && defined(_WIN32)
    water_private_key *key = static_cast<water_private_key *>(FLUID_MALLOC(sizeof(water_private_key)));

    if (key == nullptr)
    {
        FLUID_LOG(FLUID_PANIC, "Out of memory on thread-private key allocation");
        *priv = nullptr;
        return;
    }

    key->tls_index = TlsAlloc();
    if (key->tls_index == TLS_OUT_OF_INDEXES)
    {
        FLUID_LOG(FLUID_ERR, "Failed to allocate Windows TLS index");
        FLUID_FREE(key);
        *priv = nullptr;
        return;
    }

    *priv = key;
#else
    *priv = priv;
#endif
}

void fluid_private_free(fluid_private_t priv)
{
#if defined(FLUIDSYNTH_WATER) && defined(_WIN32)
    water_private_key *key = static_cast<water_private_key *>(priv);

    if (key == nullptr)
        return;

    if (!TlsFree(key->tls_index))
        FLUID_LOG(FLUID_WARN, "Failed to release Windows TLS index");
    FLUID_FREE(key);
#else
    private_data.erase(priv);
#endif
}

void *fluid_private_get(fluid_private_t priv)
{
#if defined(FLUIDSYNTH_WATER) && defined(_WIN32)
    water_private_key *key = static_cast<water_private_key *>(priv);
    return key == nullptr ? nullptr : TlsGetValue(key->tls_index);
#else
    const auto entry = private_data.find(priv);
    return entry == private_data.end() ? nullptr : entry->second;
#endif
}

void fluid_private_set(fluid_private_t priv, void *value)
{
#if defined(FLUIDSYNTH_WATER) && defined(_WIN32)
    water_private_key *key = static_cast<water_private_key *>(priv);

    if (key == nullptr || !TlsSetValue(key->tls_index, value))
        FLUID_LOG(FLUID_ERR, "Failed to set Windows thread-private value");
#else
    try
    {
        private_data[priv] = value;
    }
    catch (const std::exception &exc)
    {
        FLUID_LOG(FLUID_ERR, "Failed to set C++ thread-private value: %s", exc.what());
    }
    catch (...)
    {
        FLUID_LOG(FLUID_ERR, "Failed to set C++ thread-private value");
    }
#endif
}

#if HAVE_CXX_FILESYSTEM

#include <filesystem>

int fluid_stat(const char *_path, fluid_stat_buf_t *buffer)
{
    try
    {
        std::filesystem::path path = std::filesystem::u8path(_path);
        auto mtime = std::filesystem::last_write_time(path).time_since_epoch();
        buffer->st_mtime = std::chrono::duration_cast<std::chrono::seconds>(mtime).count();
        return FLUID_OK;
    }
    catch (...)
    {
    }

#else

int fluid_stat(const char *_path, fluid_stat_buf_t *buffer)
{
    FLUID_LOG(FLUID_ERR, "fluid_stat is unavailable, returning -1");
#endif
    buffer->st_mtime = 0;
    return FLUID_FAILED;
}
