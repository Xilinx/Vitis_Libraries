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
#ifndef _DSPLIB_CONV_CORR_STREAM_ALIGN_HPP_
#define _DSPLIB_CONV_CORR_STREAM_ALIGN_HPP_

/*
CONV_CORR_STREAM_ALIGN
This file captures the definition of the stream output alignment kernel class for conv_corr.
The kernel discards MAC4_ROT pipeline startup-transient samples from each per-phase conv_corr
UUT output stream so that only valid, aligned samples are forwarded to the graph output port.
The main runtime function is captured in conv_corr_stream_align.cpp.
*/

#include <adf.h>
#include "device_defs.h"
#include "fir_utils.hpp"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "conv_corr_traits.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

#if (__HAS_ACCUM_PERMUTES__ == 1)

// ---------------------------------------------------------------------------
// AIE-1 stream output alignment kernel for conv/corr.
// Discards kStartSampleUUT MAC4_ROT startup-transient samples from each per-phase
// output stream on the first invocation; forwards all subsequent samples unchanged.
//
// kStartSampleUUT = (G_LEN + delay_offset) / PHASES + int32(casc_delay_last)
//   delay_offset    = MAC4ROTDELAY * PHASES * (phase_incr - 1)
//   casc_delay_last = 0 for CASC_LEN=1 (→ kStart=G_LEN);
//                   = uint32(kCascDelayComp + kPhaseDelayComp) for CASC_LEN>1
//                     (uint32 wrap encodes the signed correction)
// ---------------------------------------------------------------------------
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_CASC_LEN>
class conv_corr_stream_align {
   private:
    static constexpr unsigned int kLanes = fnNumOfLanesForMac4Rot<TT_DATA_F>(); // MAC4_ROT lane count (= 4 on AIE-1)
    static constexpr unsigned int kMuls = getNumofMULs<TT_DATA_F, TT_DATA_G>(); // multiplications per MAC4_ROT
    static constexpr unsigned int kPoints = kMuls / kLanes;                     // G-data points per lane per MAC4_ROT

    // TP_G_LEN threshold: single stream per core when exceeded, two streams otherwise.
    static constexpr unsigned int kSingleStreamThreshold = (kMuls * TP_PHASES) >> 1;

    // Output streams per core: 1 (large G fills single-stream bandwidth) or 2 (small G).
    static constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kSingleStreamThreshold) ? 1u : kMaxNumOfStreams;

    // Phases processed per core pass (= TP_PHASES / kStreamsPerCore).
    static constexpr int kPhaseIncr = static_cast<int>(TP_PHASES) / static_cast<int>(kStreamsPerCore);

    // Phase increment minus 1; used as multiplier and bit-mask in cascade delay formulas.
    static constexpr int kPhaseIncrMinusOne = kPhaseIncr - 1;

    // Startup delay offset: MAC4ROTDELAY * PHASES * (kPhaseIncr - 1).
    // For CASC_LEN > PHASES, the extra broadcast latency shifts only the REF window (handled in Tcl).
    static constexpr int kDelayOffset =
        static_cast<int>(MAC4ROTDELAY) * static_cast<int>(TP_PHASES) * kPhaseIncrMinusOne;

    // Total MACs across all cascade kernels and phases; used to derive per-core G-tap count.
    static constexpr unsigned int kMaxMuls = kMuls * TP_CASC_LEN * TP_PHASES;

    // MAC4_ROT operations per core per stream: CEIL(G_LEN, kMaxMuls) / kMaxMuls groups of G taps.
    static constexpr unsigned int kMacsPerCoreStream = CEIL(TP_G_LEN, kMaxMuls) / kMaxMuls;

    // 0-based index of the last cascade kernel; when 0 (CASC_LEN=1), kCascDelayLast=0.
    static constexpr unsigned int kLastKernelPos = TP_CASC_LEN - 1;

    // Cascade delay component for the last kernel; uint32 wrap represents a signed correction
    // (negative values wrap as UINT_MAX+1-|val|, reinterpreted as int32 in kStartSampleUUT).
    static constexpr unsigned int kCascDelayComp = (MAC4ROTDELAY - ((kMacsPerCoreStream * kPoints) << kShiftFactor2)) *
                                                   (kLastKernelPos >> getLog2<static_cast<unsigned int>(kPhaseIncr)>());

    // Per-phase MAC4_ROT latency of the last cascade kernel: inter-phase + intra-phase terms.
    static constexpr unsigned int kPhaseDelayComp =
        MAC4ROTDELAY * ((kLastKernelPos >> getLog2<static_cast<unsigned int>(kPhaseIncr)>()) - 1u) *
            static_cast<unsigned int>(kPhaseIncrMinusOne) +
        MAC4ROTDELAY * (kLastKernelPos & static_cast<unsigned int>(kPhaseIncrMinusOne));

    // Total last-kernel pipeline delay: 0 for CASC_LEN=1 (kStartSampleUUT=G_LEN, REF trimmed to
    // F*NITER-G_LEN in Tcl). CASC_LEN>1: uint32(kCascDelayComp + kPhaseDelayComp).
    static constexpr unsigned int kCascDelayLast = (kLastKernelPos == 0) ? 0u : (kCascDelayComp + kPhaseDelayComp);

    // Flag: 1 on first invocation (discard startup transients), 0 on all subsequent invocations.
    int doInit = 1;

   public:
    // Startup-transient samples to discard per per-phase stream on first invocation.
    // = (G_LEN + kDelayOffset) / PHASES + int32(kCascDelayLast).
    static constexpr int kStartSampleUUT =
        (static_cast<int>(TP_G_LEN) + kDelayOffset) / static_cast<int>(TP_PHASES) + static_cast<int>(kCascDelayLast);

    // Output samples per per-phase UUT kernel per invocation (= TP_F_LEN / TP_PHASES).
    static constexpr unsigned int kSamplesPerInvocation = TP_F_LEN / TP_PHASES;

    // Valid samples remaining in the first invocation after discarding kStartSampleUUT transients (must be >= 0).
    static constexpr int kFirstInvocationValidSamples = static_cast<int>(kSamplesPerInvocation) - kStartSampleUUT;

    // Full kLanes-wide vectors to discard from the startup transient (maximises read throughput).
    static constexpr int kDiscardVecCount = kStartSampleUUT / static_cast<int>(kLanes);

    // Remaining samples to discard element-by-element after the vector-aligned discard (= kStartSampleUUT % kLanes).
    static constexpr int kDiscardRemainder = kStartSampleUUT % static_cast<int>(kLanes);

    conv_corr_stream_align() {
        static_assert(kStartSampleUUT >= 0,
                      "conv_corr_stream_align: kStartSampleUUT is negative – "
                      "check TP_CASC_LEN / TP_PHASES / TP_G_LEN combination.");
        static_assert(kFirstInvocationValidSamples >= 0,
                      "conv_corr_stream_align: startup transient exceeds one invocation "
                      "(kStartSampleUUT > TP_F_LEN/TP_PHASES). Increase TP_F_LEN or "
                      "reduce TP_G_LEN.");
    }

    static void registerKernelClass() { REGISTER_FUNCTION(conv_corr_stream_align::conv_corr_output_sync); }

    void conv_corr_output_sync(input_stream<TT_DATA_OUT>* __restrict inStream,
                               output_stream<TT_DATA_OUT>* __restrict outStream);
};

#endif // (__HAS_ACCUM_PERMUTES__ == 1)

} // namespace conv_corr
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_CONV_CORR_STREAM_ALIGN_HPP_
