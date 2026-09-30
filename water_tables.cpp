/*
 * Water uses the generated 2.4.x lookup data directly.  FluidSynth 2.6.x
 * computes the same tables with constexpr GCEM generators; keeping the
 * generated values here avoids making Water recurse into GCEM just to build
 * its internal static synthesizer.
 */
#include "utils/fluidsynth_priv.h"

#define fluid_ct2hz_tab water_fluid_ct2hz_data
#define fluid_cb2amp_tab water_fluid_cb2amp_data
#define fluid_concave_tab water_fluid_concave_data
#define fluid_convex_tab water_fluid_convex_data
#define fluid_pan_tab water_fluid_pan_data
#include "fluid_conv_tables.inc.h"
#undef fluid_ct2hz_tab
#undef fluid_cb2amp_tab
#undef fluid_concave_tab
#undef fluid_convex_tab
#undef fluid_pan_tab

#define interp_coeff_linear water_interp_coeff_linear_data
#define interp_coeff water_interp_coeff_data
#define sinc_table7 water_sinc_table7_data
#include "fluid_rvoice_dsp_tables.inc.h"
#undef interp_coeff_linear
#undef interp_coeff
#undef sinc_table7

extern "C" {
extern const fluid_real_t *const fluid_ct2hz_tab = water_fluid_ct2hz_data;
extern const fluid_real_t *const fluid_cb2amp_tab = water_fluid_cb2amp_data;
extern const fluid_real_t *const fluid_concave_tab = water_fluid_concave_data;
extern const fluid_real_t *const fluid_convex_tab = water_fluid_convex_data;
extern const fluid_real_t *const fluid_pan_tab = water_fluid_pan_data;

extern const fluid_real_t *const interp_coeff_linear = &water_interp_coeff_linear_data[0][0];
extern const fluid_real_t *const interp_coeff = &water_interp_coeff_data[0][0];
extern const fluid_real_t *const sinc_table7 = &water_sinc_table7_data[0][0];
}
