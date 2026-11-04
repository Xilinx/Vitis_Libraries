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
#pragma once
#include <adf.h>
#include "transpose_kernel_ref.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

template <typename TT_DATA, unsigned int TP_DIM_ROWS, unsigned int TP_DIM_COLS, unsigned int TP_CONJUGATE>
void transpose_kernel_ref<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_CONJUGATE>::transpose_ref_main(
    input_buffer<TT_DATA>& inWindow, output_buffer<TT_DATA>& outWindow) {
    // Input buffer is (kPadRows x kPadCols), row-major (row-stride kPadCols). Output buffer is
    // (kPadCols x kPadRows), row-major (row-stride kPadRows) - the same padded-row-stride
    // convention the UUT kernel uses (see transpose.cpp), just scalar instead of tiled/vectorized.
    static constexpr unsigned int kPadRows = fnTposePad<TT_DATA>(TP_DIM_ROWS);
    static constexpr unsigned int kPadCols = fnTposePad<TT_DATA>(TP_DIM_COLS);

    TT_DATA* inPtr = inWindow.data();
    TT_DATA* outPtr = outWindow.data();

    for (unsigned int r = 0; r < TP_DIM_ROWS; r++) {
        for (unsigned int c = 0; c < TP_DIM_COLS; c++) {
            TT_DATA sample = inPtr[r * kPadCols + c];
            if
                constexpr(TP_CONJUGATE == 1) { sample = conjElm(sample); }
            outPtr[c * kPadRows + r] = sample;
        }
    }
};
}
}
}
}
