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
#ifndef _DSPLIB_TRANSPOSE_KERNEL_REF_HPP_
#define _DSPLIB_TRANSPOSE_KERNEL_REF_HPP_

/*
transpose reference model.

Note: this is a distinct component from the pre-existing L1/tests/aie/inc/transpose_ref.hpp -
that file is an internal helper (namespace tpose) used by fft_ifft_2d and does not support
TP_CASC_LEN/TP_CONJUGATE, so it is left untouched. This file is the reference model for the
transpose IP core (namespace xf::dsp::aie::transpose).
*/

#include <adf.h>
#include "device_defs.h"
#include "transpose_utils.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

// Helper: conjugate an element. For real types this is a no-op; for complex types it negates
// the imaginary part. Mirrors matrix_mult_ref.hpp's conjElm() helper.
template <typename T>
inline T conjElm(T val) {
    return val;
}
template <>
inline cint16 conjElm<cint16>(cint16 val) {
    cint16 r;
    r.real = val.real;
    r.imag = -val.imag;
    return r;
}
template <>
inline cint32 conjElm<cint32>(cint32 val) {
    cint32 r;
    r.real = val.real;
    r.imag = -val.imag;
    return r;
}
template <>
inline cfloat conjElm<cfloat>(cfloat val) {
    cfloat r;
    r.real = val.real;
    r.imag = -val.imag;
    return r;
}

// transpose reference model class. Operates on the full, logical (unsplit) TP_DIM_ROWS x
// TP_DIM_COLS matrix, regardless of the UUT's TP_SSR/TP_CASC_LEN - but, like the UUT kernel,
// its buffer is padded up to the device's memory access width on both axes (see fnTposePad() in
// transpose_utils.hpp): every AIE iobuffer's byte size must be a multiple of 4 bytes, which an
// arbitrary, un-padded TP_DIM_ROWS x TP_DIM_COLS buffer cannot generally guarantee (e.g. 17x23
// int16 samples is 782 bytes, not a multiple of 4). Padding both dimensions up to the memory
// access width guarantees this automatically, since the memory access width in bytes is itself
// always a multiple of 4. Only the logical TP_DIM_ROWS x TP_DIM_COLS sub-region is computed;
// the test bench pads the reference model's input and strips its output the same way it does
// for the UUT (see transpose_partition_pad.pl's --pad-single/--strip-single modes).
template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS, unsigned int TP_CONJUGATE = 0>
class transpose_kernel_ref {
   public:
    // Constructor
    transpose_kernel_ref() {}
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(transpose_kernel_ref::transpose_ref_main); }
    // Declaration of main function
    void transpose_ref_main(input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow);
};
}
}
}
}

#endif // _DSPLIB_TRANSPOSE_KERNEL_REF_HPP_
