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

#ifndef _XF_SOLVER_SCALAR_TEST_UTILS_HPP_
#define _XF_SOLVER_SCALAR_TEST_UTILS_HPP_

// -------------------------------------------------
// Utilities for scalar *test* code.
// -------------------------------------------------
// Not for use in synthesisable code
// -------------------------------------------------

#include "ap_fixed.h"

#include "hls_x_complex.h"
#include <complex>

// ======================================================================
// Compare a single value
// ======================================================================
//
// Using a struct to allow different types of equality to be added later
// (e.g. GE, NE, etc).
//
template <typename T>
struct xil_equality {
    static bool equal(T a, T b, unsigned allowed_ulp_mismatch) {
        int diff = calcULP(a, b);
        if (diff <= allowed_ulp_mismatch)
            return true;
        else
            return false;
    }
};

// Complex data
// ----------------
// This is used by complex data of any type.  A xil_equality is called to compare the real and imaginary
// parts, so this just handles the calling of the individual parts
//
template <typename T>
struct xil_equality<hls::x_complex<T> > {
    static bool equal(hls::x_complex<T> a, hls::x_complex<T> b, unsigned allowed_ulp_mismatch) {
        // Use the basic type equals to compare the real and imaginary parts
        //
        typedef xil_equality<T> e;

        if (e::equal(a.real(), b.real(), allowed_ulp_mismatch) && e::equal(a.imag(), b.imag(), allowed_ulp_mismatch))
            return true;
        else
            return false;
    }
};
template <typename T>
struct xil_equality<std::complex<T> > {
    static bool equal(std::complex<T> a, std::complex<T> b, unsigned allowed_ulp_mismatch) {
        // Use the basic type equals to compare the real and imaginary parts
        //
        typedef xil_equality<T> e;

        if (e::equal(a.real(), b.real(), allowed_ulp_mismatch) && e::equal(a.imag(), b.imag(), allowed_ulp_mismatch))
            return true;
        else
            return false;
    }
};
// ==End: Compare a single value ========================================

#endif
