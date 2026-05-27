/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
CONV_CORR_STREAM_ALIGN kernel code.
This file captures the body of run-time code for the conv_corr_stream_align kernel class.
Coding conventions
TT_      template type suffix
TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include "conv_corr_stream_align.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

#if (__HAS_ACCUM_PERMUTES__ == 1)

// conv_corr_stream_align - Output synchronization kernel to remove cascade startup transients in stream-based
// convolution/correlation
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_CASC_LEN>
NOINLINE_DECL void
conv_corr_stream_align<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_F_LEN, TP_G_LEN, TP_PHASES, TP_CASC_LEN>::
    conv_corr_output_sync(input_stream<TT_DATA_OUT>* __restrict inStream,
                          output_stream<TT_DATA_OUT>* __restrict outStream) {
    using dataVec_t = ::aie::vector<TT_DATA_OUT, kLanes>;

    if (doInit == 1) {
        // scalar discard of initial kDiscardRemainder samples to align with first full vector of valid data from
        // cascade
        for (unsigned int i = 0; i < (unsigned int)kDiscardRemainder; i++)
            chess_prepare_for_pipelining chess_loop_count(kDiscardRemainder) { readincr(inStream); }

        // full-vector discard of initial kDiscardVecCount vectors containing cascade startup transient samples
        for (unsigned int i = 0; i < (unsigned int)kDiscardVecCount; i++)
            chess_prepare_for_pipelining chess_loop_count(kDiscardVecCount) { readincr_v<kLanes>(inStream); }

        // full-vector pass of first kFirstVecs vectors containing valid samples in first batch from cascade
        constexpr unsigned int kFirstVecs =
            (unsigned int)kFirstInvocValidSamples /
            kLanes; // Number of full vectors of valid samples in first batch from cascade
        for (unsigned int i = 0; i < kFirstVecs; i++) chess_prepare_for_pipelining chess_loop_count(kFirstVecs) {
                writeincr(outStream, readincr_v<kLanes>(inStream));
            }

        // scalar pass of remaining kFirstTail samples in first batch from cascade
        constexpr unsigned int kFirstTail = (unsigned int)kFirstInvocValidSamples %
                                            kLanes; // Number of remaining valid samples in first batch from cascade
        for (unsigned int i = 0; i < kFirstTail; i++) chess_prepare_for_pipelining chess_loop_count(kFirstTail) {
                writeincr(outStream, readincr(inStream));
            }

        doInit = 0;

    } else {
        // Normal pass-through: forward all kSamplesPerInvoc samples
        constexpr unsigned int kNormalVecs = kSamplesPerInvoc / kLanes;
        for (unsigned int i = 0; i < kNormalVecs; i++) chess_prepare_for_pipelining chess_loop_count(kNormalVecs) {
                writeincr(outStream, readincr_v<kLanes>(inStream));
            }
    }
} // End of conv_corr_stream_align::conv_corr_output_sync()

#endif // (__HAS_ACCUM_PERMUTES__ == 1)

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {
