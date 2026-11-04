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
#ifndef _DSPLIB_TRANSPOSE_GRAPH_HPP_
#define _DSPLIB_TRANSPOSE_GRAPH_HPP_

/*
This file captures the definition of the 'graph' level class for the transpose library element.
*/

#include <adf.h>
#include <array>
#include <utility>
#include "device_defs.h"
#include "graph_utils.hpp"
#include "transpose.hpp"
#include "transpose_utils.hpp"

using namespace adf;

/**
 * @defgroup transpose_graph Transpose function
 **/

namespace xf {
namespace dsp {
namespace aie {
namespace transpose {

/**
 * @ingroup transpose_graph
 * @brief transpose_graph transposes a TP_DIM_ROWS x TP_DIM_COLS matrix of TT_DATA held in an
 *        input iobuffer, producing a TP_DIM_COLS x TP_DIM_ROWS output iobuffer.
 *
 * @tparam TT_DATA describes the type of individual data samples input to and output from the
 *         transpose. This is a typename and must be one of the following: int16, cint16, int32,
 *         cint32, float, cfloat.
 * @tparam TP_DIM_ROWS is an unsigned integer which describes the number of rows in the input
 *         matrix. It can be any positive integer. When TP_SSR > 1, this dimension is the one
 *         split across SSR lanes.
 * @tparam TP_DIM_COLS is an unsigned integer which describes the number of columns in the input
 *         matrix (equivalently, the number of rows in the output matrix). It can be any positive
 *         integer. When TP_CASC_LEN > 1, this dimension is the one split across cascaded
 *         kernels.
 * @tparam TP_SSR is an unsigned integer which describes the number of parallel lanes that
 *         TP_DIM_ROWS is split across. Lane `s` owns a full-width, contiguous row-range of the
 *         input and produces the corresponding contiguous column-range of the output.
 * @tparam TP_CASC_LEN is an unsigned integer which describes the number of kernels that
 *         TP_DIM_COLS is split across, within each TP_SSR lane. Kernel `c` owns a full-height,
 *         contiguous column-range of the input and produces the corresponding contiguous
 *         row-range of the output.
 *
 *         Together, TP_SSR and TP_CASC_LEN form a TP_SSR x TP_CASC_LEN grid of independent
 *         kernels (indexed `ssrIdx*TP_CASC_LEN + cascIdx`), each transposing one
 *         (row-range x column-range) tile of the input. Because transposing swaps rows and
 *         columns, lane/kernel `(s, c)`'s output tile lands at block position `(c, s)` (columns
 *         become rows and vice versa) in the overall output matrix - the same axis swap that
 *         gives a block matrix `M = [[A,B],[C,D]]` the transpose `M^T = [[A^T,C^T],[B^T,D^T]]`.
 *         Routing each kernel's output port to the correct block position of the final output is
 *         the responsibility of the parent graph/testbench, exactly as for TP_SSR in other
 *         DSPLib cores (e.g. matrix_mult) - this graph does no on-chip combination across ports.
 * @tparam TP_SPLIT_MODE selects how TP_DIM_ROWS is divided across TP_SSR lanes, and how
 *         TP_DIM_COLS is divided across TP_CASC_LEN kernels, when they do not divide evenly:
 *         - 0 (kSplitModeEqual): as equal as possible, with any remainder distributed one at a
 *           time to the first lanes/kernels.
 *         - 1 (kSplitModeAligned): every lane/kernel but the last is sized to a multiple of the
 *           device's memory access width; the last one takes whatever remains (the "runt").
 * @tparam TP_CONJUGATE describes the selection of whether or not to conjugate the elements of
 *         the input during the transpose, with no reduction in throughput. TP_CONJUGATE requires
 *         a complex TT_DATA type (cint16, cint32 or cfloat).
 **/
template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_SSR = 1,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_SPLIT_MODE = kSplitModeEqual,
          unsigned int TP_CONJUGATE = 0>
class transpose_graph : public graph {
   private:
    static constexpr unsigned int kNumKernels = TP_SSR * TP_CASC_LEN;

    // Builds, at compile time, the row count owned by every SSR lane 0..TP_SSR-1 and the column
    // count owned by every cascade position 0..TP_CASC_LEN-1. Reuses the same split-math as the
    // column split (it is dimension-agnostic - it just takes a dim/splitLen/pos/mode).
    template <std::size_t... TP_SP>
    static constexpr std::array<unsigned int, TP_SSR> makeRowsPerLane(std::index_sequence<TP_SP...>) {
        return {fnTposeColsPerKernel<TP_DIM_ROWS, TP_SSR, TP_SP, TT_DATA, TP_SPLIT_MODE>()...};
    }
    static constexpr std::array<unsigned int, TP_SSR> kRowsPerLane =
        makeRowsPerLane(std::make_index_sequence<TP_SSR>{});

    template <std::size_t... TP_KP>
    static constexpr std::array<unsigned int, TP_CASC_LEN> makeColsPerKernel(std::index_sequence<TP_KP...>) {
        return {fnTposeColsPerKernel<TP_DIM_COLS, TP_CASC_LEN, TP_KP, TT_DATA, TP_SPLIT_MODE>()...};
    }
    static constexpr std::array<unsigned int, TP_CASC_LEN> kColsPerKernel =
        makeColsPerKernel(std::make_index_sequence<TP_CASC_LEN>{});

    // Instantiates kernel TP_KP (flat index ssrIdx*TP_CASC_LEN + cascIdx), then recurses to
    // TP_KP+1 until the full TP_SSR x TP_CASC_LEN grid is reached. Each kernel gets its own,
    // distinct, compile-time (row-range, column-range) template argument pair.
    template <unsigned int TP_KP>
    void createKernels() {
        if
            constexpr(TP_KP < kNumKernels) {
                constexpr unsigned int kSsrIdx = TP_KP / TP_CASC_LEN;
                constexpr unsigned int kCascIdx = TP_KP % TP_CASC_LEN;
                constexpr unsigned int kRowsThisLane = kRowsPerLane[kSsrIdx];
                constexpr unsigned int kColsThisKernel = kColsPerKernel[kCascIdx];
                m_kernels[TP_KP] =
                    kernel::create_object<transpose<TT_DATA, kRowsThisLane, kColsThisKernel, TP_CONJUGATE> >();
                source(m_kernels[TP_KP]) = "transpose.cpp";
                runtime<ratio>(m_kernels[TP_KP]) = 0.9;
                connect<>(in[TP_KP], m_kernels[TP_KP].in[0]);
                connect<>(m_kernels[TP_KP].out[0], out[TP_KP]);
                dimensions(m_kernels[TP_KP].in[0]) = {fnTposePad<TT_DATA>(kRowsThisLane) *
                                                      fnTposePad<TT_DATA>(kColsThisKernel)};
                dimensions(m_kernels[TP_KP].out[0]) = {fnTposePad<TT_DATA>(kColsThisKernel) *
                                                       fnTposePad<TT_DATA>(kRowsThisLane)};
                conditional_single_buffer<TT_DATA, fnTposePad<TT_DATA>(kRowsThisLane) *
                                                       fnTposePad<TT_DATA>(kColsThisKernel)>(m_kernels[TP_KP].in[0]);
                conditional_single_buffer<TT_DATA, fnTposePad<TT_DATA>(kColsThisKernel) *
                                                       fnTposePad<TT_DATA>(kRowsThisLane)>(m_kernels[TP_KP].out[0]);
                createKernels<TP_KP + 1>();
            }
    }

    // The largest per-lane row count and largest per-kernel column count bound the largest
    // per-kernel buffer - index 0 under both split modes (equal mode gives remainder
    // rows/columns to the first lanes/kernels; aligned mode sizes every non-last lane/kernel
    // identically and no larger than index 0).
    static constexpr unsigned int kPadRowsMax = fnTposePad<TT_DATA>(kRowsPerLane[0]);
    static constexpr unsigned int kPadColsMax = fnTposePad<TT_DATA>(kColsPerKernel[0]);
    static constexpr unsigned int kBufBytes = (kPadRowsMax * kPadColsMax + kPadColsMax * kPadRowsMax) * sizeof(TT_DATA);

    // Every lane/kernel must own at least one row/column, and all lanes/kernels together must
    // cover exactly TP_DIM_ROWS/TP_DIM_COLS. Under TP_SPLIT_MODE=1 (aligned), rounding each
    // non-last lane/kernel up to the memory access width can consume the whole dimension before
    // the last one is reached; with unsigned arithmetic that shows up as a huge wrapped-around
    // count rather than a small or negative one, so this checks exact coverage rather than just
    // "count >= 1".
    template <std::size_t... TP_SP>
    static constexpr unsigned int sumRowsPerLane(std::index_sequence<TP_SP...>) {
        return (kRowsPerLane[TP_SP] + ...);
    }
    static constexpr unsigned int kRowsPerLaneSum = sumRowsPerLane(std::make_index_sequence<TP_SSR>{});
    template <std::size_t... TP_KP>
    static constexpr unsigned int sumColsPerKernel(std::index_sequence<TP_KP...>) {
        return (kColsPerKernel[TP_KP] + ...);
    }
    static constexpr unsigned int kColsPerKernelSum = sumColsPerKernel(std::make_index_sequence<TP_CASC_LEN>{});

   public:
    static_assert(TP_SSR >= 1, "ERROR: TP_SSR must be at least 1.");
    static_assert(TP_CASC_LEN >= 1, "ERROR: TP_CASC_LEN must be at least 1.");
    static_assert(TP_DIM_ROWS >= TP_SSR,
                  "ERROR: TP_DIM_ROWS must be at least TP_SSR so every lane gets at least one row.");
    static_assert(TP_DIM_COLS >= TP_CASC_LEN,
                  "ERROR: TP_DIM_COLS must be at least TP_CASC_LEN so every kernel gets at least one column.");
    static_assert(TP_CONJUGATE == 0 || fnIsComplex<TT_DATA>(),
                  "ERROR: TP_CONJUGATE requires a complex TT_DATA type (cint16, cint32 or cfloat).");
    static_assert(kRowsPerLaneSum == TP_DIM_ROWS,
                  "ERROR: TP_SPLIT_MODE cannot divide TP_DIM_ROWS across TP_SSR lanes while giving every lane at "
                  "least one row. Reduce TP_SSR or use TP_SPLIT_MODE=0 (kSplitModeEqual).");
    static_assert(kColsPerKernelSum == TP_DIM_COLS,
                  "ERROR: TP_SPLIT_MODE cannot divide TP_DIM_COLS across TP_CASC_LEN kernels while giving every "
                  "kernel at least one column. Reduce TP_CASC_LEN or use TP_SPLIT_MODE=0 (kSplitModeEqual).");
    static_assert(kBufBytes <= __DATA_MEM_BYTES__,
                  "ERROR: The requested TP_DIM_ROWS/TP_DIM_COLS/TP_SSR/TP_CASC_LEN combination requires a "
                  "per-kernel buffer larger than this AIE variant's tile data memory. Increase TP_SSR and/or "
                  "TP_CASC_LEN.");

    port_array<input, kNumKernels> in;
    port_array<output, kNumKernels> out;

    kernel m_kernels[kNumKernels];

    /**
     * @brief Returns the number of rows (equivalently, output columns) processed by SSR lane
     *        `ssrPos`, so a user wiring up a multi-lane graph knows exactly how the work was
     *        divided.
     **/
    static unsigned int getRowsPerKernel(unsigned int ssrPos) { return kRowsPerLane[ssrPos]; }

    /**
     * @brief Returns the number of columns (equivalently, output rows) processed by cascade
     *        position `cascPos` (within any SSR lane - the split is identical in every lane), so
     *        a user wiring up a multi-kernel graph knows exactly how the work was divided.
     **/
    static unsigned int getColsPerKernel(unsigned int cascPos) { return kColsPerKernel[cascPos]; }

    kernel* getKernels() { return m_kernels; };

    // Constructor
    transpose_graph() { createKernels<0>(); };
};
}
}
}
}

#endif // _DSPLIB_TRANSPOSE_GRAPH_HPP_
