/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#include "kernel_cholesky.hpp"
#include "xf_solver_L1.hpp"
#include <type_traits>

namespace {
// True for real/complex element types backed by IEEE floating point (float/double), false for
// fixed-point (ap_fixed) element types.
template <typename T>
struct is_float_elem : std::is_floating_point<T> {};
template <typename T>
struct is_float_elem<hls::x_complex<T> > : std::is_floating_point<T> {};
template <typename T>
struct is_float_elem<std::complex<T> > : std::is_floating_point<T> {};
} // namespace

struct my_cholesky_traits : xf::solver::choleskyTraits<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T> {
    static const int ARCH = SEL_ARCH;
    // sum_loop's accumulation recurrence caps the fadd core's latency at whatever INNER_II
    // allows (II=1, the base default, forces the shallowest fadd HLS can bind). For floating
    // point that shallow adder's per-stage logic can't meet a 3.333ns (300MHz) target, so relax
    // II to let HLS bind a more deeply pipelined, timing-clean fadd core (validated: closes
    // timing with ~0.5ns slack post-route, at the cost of extra latency cycles). Fixed-point
    // (ap_fixed) accumulation is comb-shallow enough to keep II=1 and still meet 3.333ns
    // (validated: Target II=1, Final II=1, Estimated Fmax 412MHz on the complex_fixed_arch1
    // variant of this same test), so don't pay that latency cost there.
    static const int INNER_II = is_float_elem<MATRIX_IN_T>::value ? 8 : 1;
};

extern int kernel_cholesky_0(hls::stream<MATRIX_IN_T>& matrixAStrm, hls::stream<MATRIX_OUT_T>& matrixLStrm) {
    int ret;
    ret = xf::solver::cholesky<LOWER_TRIANGULAR, DIM, MATRIX_IN_T, MATRIX_OUT_T, my_cholesky_traits>(matrixAStrm,
                                                                                                     matrixLStrm);
    return ret;
}
