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
#ifndef _DSPLIB_TRANSPOSE_UTILS_HPP_
#define _DSPLIB_TRANSPOSE_UTILS_HPP_

/*
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class, the graph and the metadata. These functions are separate from the
traits file because they encode the TP_CASC_LEN column-splitting policy (TP_SPLIT_MODE), not
per-TT_DATA intrinsic properties.
*/

#include "device_defs.h"
#include "fir_utils.hpp" // for fnTrunc
#include "transpose_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

enum eTposeSplitMode { kSplitModeEqual = 0, kSplitModeAligned = 1 };

INLINE_DECL constexpr unsigned int fnCeil(unsigned int x, unsigned int y) {
    return fir::fnTrunc(x + y - 1, y);
}

// Split TP_DIM_COLS as equally as possible over TP_CASC_LEN kernels. Any remainder is
// distributed one column at a time to the first (TP_DIM_COLS % TP_CASC_LEN) kernels.
// Mirrors the shape of fnFirRange() in fir_utils.hpp.
template <unsigned int TP_DIM_COLS, unsigned int TP_CASC_LEN, unsigned int TP_KP>
INLINE_DECL constexpr unsigned int fnTposeRangeEqual() {
    return (fir::fnTrunc(TP_DIM_COLS, TP_CASC_LEN) / TP_CASC_LEN) +
           ((TP_DIM_COLS - fir::fnTrunc(TP_DIM_COLS, TP_CASC_LEN)) >= (TP_KP + 1) ? 1 : 0);
}

// Every kernel but the last gets ceil(TP_DIM_COLS/TP_CASC_LEN) columns, rounded up to the
// device's memory access width (so every non-last kernel's buffer is DMA-granularity aligned
// with no padding waste). The last kernel absorbs whatever remains - the "runt".
template <unsigned int TP_DIM_COLS, unsigned int TP_CASC_LEN, unsigned int TP_KP, typename TT_DATA>
INLINE_DECL constexpr unsigned int fnTposeRangeAligned() {
    constexpr unsigned int kMemWidth = fnVecSampleNumMax<TT_DATA>();
    constexpr unsigned int kAvgCols = (TP_DIM_COLS + TP_CASC_LEN - 1) / TP_CASC_LEN;
    constexpr unsigned int kColsPerKernel = fnCeil(kAvgCols, kMemWidth);
    return (TP_KP < (TP_CASC_LEN - 1)) ? kColsPerKernel : (TP_DIM_COLS - kColsPerKernel * (TP_CASC_LEN - 1));
}

// Dispatches on TP_SPLIT_MODE. This is the single source of truth used by both the kernel
// (TP_DIM_COLS_RANGE template parameter) and the graph (getColsPerKernel()).
template <unsigned int TP_DIM_COLS,
          unsigned int TP_CASC_LEN,
          unsigned int TP_KP,
          typename TT_DATA,
          unsigned int TP_SPLIT_MODE>
INLINE_DECL constexpr unsigned int fnTposeColsPerKernel() {
    return (TP_SPLIT_MODE == kSplitModeEqual) ? fnTposeRangeEqual<TP_DIM_COLS, TP_CASC_LEN, TP_KP>()
                                              : fnTposeRangeAligned<TP_DIM_COLS, TP_CASC_LEN, TP_KP, TT_DATA>();
}

// Rounds a logical row/column count up to the device's memory access width - the padded
// iobuffer dimension actually used for the kernel's ports.
template <typename TT_DATA>
INLINE_DECL constexpr unsigned int fnTposePad(unsigned int dim) {
    return fnCeil(dim, fnVecSampleNumMax<TT_DATA>());
}
}
}
}
}

#endif // _DSPLIB_TRANSPOSE_UTILS_HPP_
