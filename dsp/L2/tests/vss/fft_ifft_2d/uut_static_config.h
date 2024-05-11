#define INPUT_SAMPLES WINDOW_VSIZE* NITER
#define OUTPUT_SAMPLES WINDOW_VSIZE* NITER

#ifndef INPUT_WINDOW_VSIZE
#if DATA_TYPE == cint16
#define INPUT_WINDOW_VSIZE ((8 * DYN_PT_SIZE) + POINT_SIZE)
#else
#define INPUT_WINDOW_VSIZE ((4 * DYN_PT_SIZE) + POINT_SIZE)
#endif
#endif

#ifdef USING_UUT
#define UUT_SSR SSR
#else
#define UUT_SSR 1 // this is because the reference model takes in the parameter as parallel factor.
#endif