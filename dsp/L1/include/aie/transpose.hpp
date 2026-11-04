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
#ifndef _DSPLIB_TRANSPOSE_HPP_
#define _DSPLIB_TRANSPOSE_HPP_

/*
Transpose Kernel.
This file captures the definition of the transpose kernel class.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes

Each kernel instance transposes one, independent (TP_DIM_ROWS_RANGE x TP_DIM_COLS_RANGE) matrix
held in an input iobuffer, producing a (TP_DIM_COLS_RANGE x TP_DIM_ROWS_RANGE) output iobuffer.
Multi-kernel operation (TP_SSR > 1 and/or TP_CASC_LEN > 1) is realised entirely at the graph
level: the graph splits TP_DIM_ROWS across TP_SSR lanes and TP_DIM_COLS across TP_CASC_LEN
cascades (see transpose_utils.hpp) and instantiates one of these kernels per (row-range,
column-range) pair, so this class itself only ever needs to know its own, already-resolved,
TP_DIM_ROWS_RANGE/TP_DIM_COLS_RANGE.

TP_DIM_ROWS_RANGE and TP_DIM_COLS_RANGE are the *logical* (unpadded) dimensions. Internally, the
buffers are treated as padded up to the device's memory access width (see fnTposePad() in
transpose_utils.hpp) so that ::aie::transpose() can be applied on whole, aligned tiles.

Class structure: this kernel follows the base-class + entry-level-specialization convention
described in dsp-reference/docs/ip-structure.md ("Kernel Class Structure: Base Class +
Entry-Level Specializations", modeled on fir_sr_asym/matrix_mult) - kernelTransposeClass holds
the actual compute, and transpose is the thin, REGISTER_FUNCTION'd entry point. Transpose
currently has only one legal port combination (a single input iobuffer and a single output
iobuffer, always), so there is only one entry-level specialization today; the split is
established now so that a future optional port (e.g. a cascade connection) only needs a new
entry-level specialization, not a duplicated implementation.
*/

#include <adf.h>
#include "device_defs.h"
#include "transpose_traits.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

//-----------------------------------------------------------------------------------------------------
// Base class - holds the actual transpose computation, shared by every entry-level specialization.
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS_RANGE,
          unsigned int TP_DIM_COLS_RANGE,
          unsigned int TP_CONJUGATE = 0>
class kernelTransposeClass {
   public:
    // Constructor
    kernelTransposeClass() {}

    // Shared compute function - called by every entry-level specialization's thin shim.
    void transposeKernel(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};

//-----------------------------------------------------------------------------------------------------
// Entry-level class - windows (io buffer) i/f. Only one port combination exists today, so only one
// specialization; see the Design Notes above.
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS_RANGE,
          unsigned int TP_DIM_COLS_RANGE,
          unsigned int TP_CONJUGATE = 0>
class transpose : public kernelTransposeClass<TT_DATA, TP_DIM_ROWS_RANGE, TP_DIM_COLS_RANGE, TP_CONJUGATE> {
   public:
    // Constructor
    transpose() : kernelTransposeClass<TT_DATA, TP_DIM_ROWS_RANGE, TP_DIM_COLS_RANGE, TP_CONJUGATE>() {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(transpose::transpose_main); }

    // Main function - thin shim, forwards straight into the base class's transposeKernel().
    void transpose_main(input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow);
};
}
}
}
}

#endif // _DSPLIB_TRANSPOSE_HPP_
