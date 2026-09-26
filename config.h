#ifndef CONFIG_H
#define CONFIG_H

/* Static configuration used only by Water's bundled FluidSynth build. */
#define ENABLE_MIXER_THREADS 1

#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_IO_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MATH_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDIO_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_WINDOWS_H 1

#define NO_GUI 1

/*
 * Water drives this bundled build with Clang's Windows target rather than
 * MinGW mode.  Do not advertise MINGW32 or POSIX <strings.h>: doing so makes
 * FluidSynth select strcasecmp()/strncasecmp() and include a header that is
 * absent from the Windows cross-compiler's CRT surface.  _WIN32 selects the
 * native _stricmp()/_strnicmp() compatibility path instead.
 */

#define PACKAGE "fluidsynth"
#define PACKAGE_NAME "FluidSynth"
#define PACKAGE_STRING "FluidSynth 2.6.1"
#define PACKAGE_TARNAME "fluidsynth"
#define PACKAGE_VERSION "2.6.1"

#define STDC_HEADERS 1
#define SUPPORTS_VLA 1

#define HAVE_SINF 1
#define HAVE_COSF 1
#define HAVE_FABSF 1
#define HAVE_POWF 1
#define HAVE_SQRTF 1
#define HAVE_LOGF 1

/* Water uses the C++11 OS abstraction but keeps file probing on Win32 APIs. */
#define OSAL_cpp11 1
#define HAVE_CXX_FILESYSTEM 0

#endif /* CONFIG_H */
