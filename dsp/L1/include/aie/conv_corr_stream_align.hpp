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
// Stream Output Alignment Kernel
//
// Discards the MAC4_ROT pipeline startup-transient samples from each
// per-phase conv_corr UUT output stream so that only valid, aligned samples
// are forwarded to the graph output port.
//
// UUT startup-transient discard formula:
//
//   streams_per_core = (TP_G_LEN > (kMuls * TP_PHASES) / 2) ? 1 : 2
//   phase_incr       = TP_PHASES / streams_per_core
//   delay_offset     = MAC4ROTDELAY * TP_PHASES * (phase_incr - 1)
//
//   casc_delay_last  = kCascDelay of the final cascade kernel
//                    = (MAC4ROTDELAY - kMacsPerCore*kPoints*kDataBuffLenFactor)
//                      * (kLastKernelPos >> (kPhaseIncr-1))
//                      + MAC4ROTDELAY * ((kLastKernelPos >> (kPhaseIncr-1)) - 1)
//                        * (kPhaseIncr-1)
//                      + MAC4ROTDELAY * (kLastKernelPos & (kPhaseIncr-1))
//   (all unsigned arithmetic; negative delays wrap as UINT_MAX+1-|delay|)
//
//   kStartSampleUUT  = (TP_G_LEN + delay_offset) / TP_PHASES + casc_delay_last
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
    static constexpr unsigned int kLanes =
        fnNumOfLanesForMac4Rot<TT_DATA_F>(); // Number of lanes in MAC4_ROT intrinsic (always 4 for AIE-1)
    static constexpr unsigned int kMuls =
        getNumofMULs<TT_DATA_F, TT_DATA_G>();               // Number of multiplications per MAC4_ROT operation
    static constexpr unsigned int kPoints = kMuls / kLanes; // Number of points per MAC4_ROT operation

    // Threshold for determining if G_LEN requires 1 or 2 streams per core
    static constexpr unsigned int kStreamPerCoreVar = (kMuls * TP_PHASES) >> 1;

    // Number of output streams per core (1 or 2)
    static constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kStreamPerCoreVar) ? 1u : kMaxNumOfStreams;

    // Phase increment between consecutive outputs from the same core
    static constexpr int kPhaseIncr = static_cast<int>(TP_PHASES) / static_cast<int>(kStreamsPerCore);

    // Phase increment minus 1 (used in delay calculations)
    static constexpr int kPhaseIncr_m1 = kPhaseIncr - 1;

    // UUT startup delay offset based on phase increment
    static constexpr int kDelayOffset = static_cast<int>(MAC4ROTDELAY) * static_cast<int>(TP_PHASES) * kPhaseIncr_m1;

    // Total multiplications across all cascade kernels and phases
    static constexpr unsigned int kMaxMuls = kMuls * TP_CASC_LEN * TP_PHASES;

    // Number of MAC operations per core per stream
    static constexpr unsigned int kMacsPerCoreStream = CEIL(TP_G_LEN, kMaxMuls) / kMaxMuls;

    // Position index of the last cascade kernel (0-based)
    static constexpr unsigned int kLastKernelPos = TP_CASC_LEN - 1;

    // Cascade delay component (uses unsigned wrap for negative delays)
    static constexpr unsigned int kCascDelayComp = (MAC4ROTDELAY - ((kMacsPerCoreStream * kPoints) << kShiftFactor2)) *
                                                   (kLastKernelPos >> static_cast<unsigned int>(kPhaseIncr_m1));

    // Per-phase delay component of the last cascade kernel
    static constexpr unsigned int kPhaseDelayComp =
        MAC4ROTDELAY * ((kLastKernelPos >> static_cast<unsigned int>(kPhaseIncr_m1)) - 1u) *
            static_cast<unsigned int>(kPhaseIncr_m1) +
        MAC4ROTDELAY * (kLastKernelPos & static_cast<unsigned int>(kPhaseIncr_m1));

    // Total delay of the last cascade kernel
    static constexpr unsigned int kCascDelayLast = (kLastKernelPos == 0) ? 0u : (kCascDelayComp + kPhaseDelayComp);

    // Initialization flag: 1 = discard startup transients, 0 = forward all samples
    int doInit = 1;

   public:
    // Number of startup-transient samples to discard from each per-phase UUT stream
    static constexpr int kStartSampleUUT =
        (static_cast<int>(TP_G_LEN) + kDelayOffset) / static_cast<int>(TP_PHASES) + static_cast<int>(kCascDelayLast);

    // Number of samples produced per kernel invocation (F samples split across phases)
    static constexpr unsigned int kSamplesPerInvoc = TP_F_LEN / TP_PHASES;

    // Number of valid samples remaining in first invocation after startup discard
    static constexpr int kFirstInvocValidSamples = static_cast<int>(kSamplesPerInvoc) - kStartSampleUUT;

    // Number of full vectors to discard from startup transient
    static constexpr int kDiscardVecCount = kStartSampleUUT / static_cast<int>(kLanes);

    // Number of remaining samples to discard after vector-aligned discard
    static constexpr int kDiscardRemainder = kStartSampleUUT % static_cast<int>(kLanes);

    conv_corr_stream_align() {
        static_assert(kStartSampleUUT >= 0,
                      "conv_corr_stream_align: kStartSampleUUT is negative – "
                      "check TP_CASC_LEN / TP_PHASES / TP_G_LEN combination.");
        static_assert(kFirstInvocValidSamples >= 0,
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
