/* FluidSynth - A Software Synthesizer
 *
 * Generated for Water's bundled static build from FluidSynth 2.6.1.
 */
#ifndef _FLUIDSYNTH_VERSION_H
#define _FLUIDSYNTH_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define FLUIDSYNTH_VERSION       "2.6.1"
#define FLUIDSYNTH_VERSION_MAJOR 2
#define FLUIDSYNTH_VERSION_MINOR 6
#define FLUIDSYNTH_VERSION_MICRO 1

FLUIDSYNTH_API void fluid_version(int *major, int *minor, int *micro);
FLUIDSYNTH_API const char* fluid_version_str(void);

#ifdef __cplusplus
}
#endif

#endif /* _FLUIDSYNTH_VERSION_H */
