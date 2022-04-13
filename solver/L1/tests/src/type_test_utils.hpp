/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _XF_SOLVER_TYPE_TEST_UTILS_HPP_
#define _XF_SOLVER_TYPE_TEST_UTILS_HPP_

// -------------------------------------------------
// Utilities for *test* code to help work with types
// -------------------------------------------------
// Not for use in synthesisable code
// -------------------------------------------------

// ======================================================================
// Type identification
// ======================================================================
//
#include <complex>

// Generic types
// -------------
template <typename T>
bool x_is_float(T x) {
    return false;
};
template <typename T>
bool x_is_double(T x) {
    return false;
};
template <typename T>
bool x_is_fixed(T x) {
    return false;
};
template <typename T>
bool x_is_complex(T x) {
    return false;
};

// Floats
// ------
template <>
bool x_is_float(float x) {
    return true;
};
template <>
bool x_is_double(float x) {
    return false;
};
template <>
bool x_is_fixed(float x) {
    return false;
};
template <>
bool x_is_complex(float x) {
    return false;
};

// Doubles
// ------
template <>
bool x_is_float(double x) {
    return false;
};
template <>
bool x_is_double(double x) {
    return true;
};
template <>
bool x_is_fixed(double x) {
    return false;
};
template <>
bool x_is_complex(double x) {
    return false;
};

// Fixed point
// ------
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_float(ap_fixed<W, I, Q, O, N> x) {
    return false;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_double(ap_fixed<W, I, Q, O, N> x) {
    return false;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_fixed(ap_fixed<W, I, Q, O, N> x) {
    return true;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_complex(ap_fixed<W, I, Q, O, N> x) {
    return false;
};

// Complex
// ---------

template <typename S>
bool x_is_complex(hls::x_complex<S> x) {
    return true;
};

template <>
bool x_is_float(hls::x_complex<float> x) {
    return true;
};
template <>
bool x_is_float(hls::x_complex<double> x) {
    return false;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_float(hls::x_complex<ap_fixed<W, I, Q, O, N> > x) {
    return false;
};

template <>
bool x_is_double(hls::x_complex<float> x) {
    return false;
};
template <>
bool x_is_double(hls::x_complex<double> x) {
    return true;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_double(hls::x_complex<ap_fixed<W, I, Q, O, N> > x) {
    return false;
};

template <>
bool x_is_fixed(hls::x_complex<float> x) {
    return false;
};
template <>
bool x_is_fixed(hls::x_complex<double> x) {
    return false;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_fixed(hls::x_complex<ap_fixed<W, I, Q, O, N> > x) {
    return true;
};

template <typename S>
bool x_is_complex(std::complex<S> x) {
    return true;
};

template <>
bool x_is_float(std::complex<float> x) {
    return true;
};
template <>
bool x_is_float(std::complex<double> x) {
    return false;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_float(std::complex<ap_fixed<W, I, Q, O, N> > x) {
    return false;
};

template <>
bool x_is_double(std::complex<float> x) {
    return false;
};
template <>
bool x_is_double(std::complex<double> x) {
    return true;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_double(std::complex<ap_fixed<W, I, Q, O, N> > x) {
    return false;
};

template <>
bool x_is_fixed(std::complex<float> x) {
    return false;
};
template <>
bool x_is_fixed(std::complex<double> x) {
    return false;
};
template <int W, int I, ap_q_mode Q, ap_o_mode O, int N>
bool x_is_fixed(std::complex<ap_fixed<W, I, Q, O, N> > x) {
    return true;
};

// ======================================================================
// End: Type identification
// ======================================================================

// Some test code for type identification:
//  float                       f;
//  double                      d;
//  ap_fixed<16, 1>             fxd;
//  x_complex<float>            comp_f;
//  x_complex<double>           comp_d;
//  x_complex<ap_fixed<16, 1> > comp_fxd;
//
//  printf("x_is_float  (f  )      = %d\n", x_is_float  (f  )      );
//  printf("x_is_float  (d  )      = %d\n", x_is_float  (d  )      );
//  printf("x_is_float  (fxd)      = %d\n", x_is_float  (fxd)      );
//  printf("x_is_float  (comp_f  ) = %d\n", x_is_float  (comp_f  ) );
//  printf("x_is_float  (comp_d  ) = %d\n", x_is_float  (comp_d  ) );
//  printf("x_is_float  (comp_fxd) = %d\n", x_is_float  (comp_fxd) );
//
//  printf("x_is_double  (f  )      = %d\n", x_is_double  (f  )      );
//  printf("x_is_double  (d  )      = %d\n", x_is_double  (d  )      );
//  printf("x_is_double  (fxd)      = %d\n", x_is_double  (fxd)      );
//  printf("x_is_double  (comp_f  ) = %d\n", x_is_double  (comp_f  ) );
//  printf("x_is_double  (comp_d  ) = %d\n", x_is_double  (comp_d  ) );
//  printf("x_is_double  (comp_fxd) = %d\n", x_is_double  (comp_fxd) );
//
//  printf("x_is_fixed  (f  )      = %d\n", x_is_fixed  (f  )      );
//  printf("x_is_fixed  (d  )      = %d\n", x_is_fixed  (d  )      );
//  printf("x_is_fixed  (fxd)      = %d\n", x_is_fixed  (fxd)      );
//  printf("x_is_fixed  (comp_f  ) = %d\n", x_is_fixed  (comp_f  ) );
//  printf("x_is_fixed  (comp_d  ) = %d\n", x_is_fixed  (comp_d  ) );
//  printf("x_is_fixed  (comp_fxd) = %d\n", x_is_fixed  (comp_fxd) );
//
//  printf("x_is_complex  (f  )      = %d\n", x_is_complex  (f  )      );
//  printf("x_is_complex  (d  )      = %d\n", x_is_complex  (d  )      );
//  printf("x_is_complex  (fxd)      = %d\n", x_is_complex  (fxd)      );
//  printf("x_is_complex  (comp_f  ) = %d\n", x_is_complex  (comp_f  ) );
//  printf("x_is_complex  (comp_d  ) = %d\n", x_is_complex  (comp_d  ) );
//  printf("x_is_complex  (comp_fxd) = %d\n", x_is_complex  (comp_fxd) );
//
//
//  exit(0);

#endif
