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
/*
transpose kernel code.
This file captures the body of run-time code for the kernel class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#pragma once
#include <adf.h>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"
#include "transpose.hpp"
#include "transpose_traits.hpp"
#include "transpose_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

// Base class - the actual transpose computation, shared by every entry-level specialization.
template <typename TT_DATA, unsigned int TP_DIM_ROWS_RANGE, unsigned int TP_DIM_COLS_RANGE, unsigned int TP_CONJUGATE>
INLINE_DECL void kernelTransposeClass<TT_DATA, TP_DIM_ROWS_RANGE, TP_DIM_COLS_RANGE, TP_CONJUGATE>::transposeKernel(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    // Tile edge is the largest power-of-two square that fits in one vector register (see
    // fnTposeTileDim()); it is always a divisor of the memory-access-width padding below, so
    // every tile is fully populated with real or zero-padded data - never a partial tile.
    static constexpr unsigned int kTileDim = fnTposeTileDim<TT_DATA>();
    static constexpr unsigned int kPadRows = fnTposePad<TT_DATA>(TP_DIM_ROWS_RANGE);
    static constexpr unsigned int kPadCols = fnTposePad<TT_DATA>(TP_DIM_COLS_RANGE);
    static constexpr unsigned int kRowTiles = kPadRows / kTileDim;
    static constexpr unsigned int kColTiles = kPadCols / kTileDim;

    using tileVec_t = ::aie::vector<TT_DATA, kTileDim * kTileDim>;
    using rowVec_t = ::aie::vector<TT_DATA, kTileDim>;

    TT_DATA* inPtr = (TT_DATA*)inWindow.data();
    TT_DATA* outPtr = (TT_DATA*)outWindow.data();

    for (unsigned int tr = 0; tr < kRowTiles; tr++) chess_loop_count(kRowTiles) chess_prepare_for_pipelining {
            for (unsigned int tc = 0; tc < kColTiles; tc++) chess_loop_count(kColTiles) chess_prepare_for_pipelining {
                    // Gather the kTileDim input rows that make up this tile into one vector.
                    tileVec_t tile;
                    for (unsigned int i = 0; i < kTileDim; i++) {
                        const unsigned int inRowOffset = (tr * kTileDim + i) * kPadCols + tc * kTileDim;
                        tile.insert(i, ::aie::load_v<kTileDim>(inPtr + inRowOffset));
                    }

                    if
                        constexpr(TP_CONJUGATE == 1) { tile = ::aie::conj(tile); }

                    // Square tile, so the row/col argument order to the intrinsic is symmetric.
                    tileVec_t transposedTile = ::aie::transpose(tile, kTileDim, kTileDim);

                    // Store to the transposed tile position: (tr, tc) -> (tc, tr).
                    for (unsigned int i = 0; i < kTileDim; i++) {
                        const unsigned int outRowOffset = (tc * kTileDim + i) * kPadRows + tr * kTileDim;
                        rowVec_t outRow = transposedTile.template extract<kTileDim>(i);
                        ::aie::store_v(outPtr + outRowOffset, outRow);
                    }
                }
        }
};

// Entry-level class - thin shim, forwards straight into the base class's transposeKernel().
template <typename TT_DATA, unsigned int TP_DIM_ROWS_RANGE, unsigned int TP_DIM_COLS_RANGE, unsigned int TP_CONJUGATE>
NOINLINE_DECL void transpose<TT_DATA, TP_DIM_ROWS_RANGE, TP_DIM_COLS_RANGE, TP_CONJUGATE>::transpose_main(
    input_buffer<TT_DATA>& __restrict inWindow, output_buffer<TT_DATA>& __restrict outWindow) {
    this->transposeKernel(inWindow, outWindow);
};
}
}
}
}
