#ifndef __DUT_TYPE__H__
#define __DUT_TYPE__H__

#ifndef USE_X_COMPLEX_TEST
#define USE_X_COMPLEX_TEST
#endif

#include "hls_x_complex.h"

typedef hls::x_complex<float> MATRIX_IN_T;
typedef hls::x_complex<float> MATRIX_OUT_T;

#endif
