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
#ifndef _DSPLIB_CONV_CORR_REF_GRAPH_HPP_
#define _DSPLIB_CONV_CORR_REF_GRAPH_HPP_

/*
    This file captures the definition of 'L2' graph level class for
    conv_corr reference model graph.
*/

#include <adf.h>
#include <vector>
#include <array>
#include <type_traits>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "conv_corr_ref.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

// TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND,
// TP_SAT, TP_CASC_LEN, TP_PHASES
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_USE_RTP_VECTOR_LENGTHS>
class conv_corr_ref_graph : public graph {
   public:
    using rtp_port_array = port_conditional_array<input, (TP_USE_RTP_VECTOR_LENGTHS == 1), 1>;

    // Defensive configuration legality checks

    // defensive check for Input data types
    static_assert(fnCheckInputDataTypes<TT_DATA_F, TT_DATA_G, TP_API>(),
                  " Assertion Failed : \n"
                  "             ERROR: TT_DATA_F and TT_DATA_G are not a supported combination ");

    // defensive check for Output data type
    static_assert(fnCheckDataTypeOfOutput<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>(),
                  " Assertion Failed : \n"
                  "              ERROR:TT_DATA_OUT should be complex if any input data type is complex.");

    // defensive check for Function type i.e either CONV or CORR
    static_assert(TP_FUNCT_TYPE <= 1,
                  " Assertion Failed : \n"
                  "              ERROR: 'TP_FUNCT_TYPE' must be \n "
                  "                     0-CORRELATION or \n "
                  "                     1-CONVOLUTION for both AIE-1 and AIE-2");

    // defensive check for CONV/CORR compute mode i.e. full/same/valid
    static_assert(fnCheckIsComputeModeValid<TP_COMPUTE_MODE, TP_API>(),
                  " Assertion Failed : \n"
                  "              ERROR: [TP_API = 1 'STREAM']  'TP_COMPUTE_MODE' always 2-VALID_MODE only \n"
                  "                     [TP_API = 0 'IOBuffer'] \n "
                  "                    'TP_COMPUTE_MODE' must be 0-FULL_MODE  or 1-SAME_MODE or 2-VALID_MODE \n ");

    // defensive check for G sig length should be always less than or equal to F Sig Length
    // defensive check for TP_G_LEN which should be multiples of (phases*4)) when Stream only Processing happening
    static_assert(fnCheckGLen<TT_DATA_F, TT_DATA_G, TP_F_LEN, TP_G_LEN, TP_PHASES, TP_API>(),
                  " Assertion Failed : \n            ERROR: 'TP_G_LEN' should be always less than or equal to TP_F_LEN "
                  "as per con_corr requirement \n"
                  "                   For API-1[STREAM] [COND-1] : TP_G_LEN must be in the range "
                  "<(256/8/sizeof(TT_DATA_G)), 256> when stream "
                  "only processing AND \n  "
                  "                                    [COND-2] : TP_G_LEN must be a multiple of (TP_PHASES * kLanes * "
                  "(kPoints / kStreamsPerCore)).  \n");

    // defensive check for num of frames i.e., TP_NUM_FRAMES restricted to 1 to maintain the better performance for time
    // being.
    static_assert(TP_NUM_FRAMES == 1,
                  " Assertion Failed : \n"
                  "              ERROR: 'TP_NUM_FRAMES' should be always equal to 1 (ONE) for current version to "
                  "maintain the same performance of conv/corr.");

    // defensive check for Lengths of F and G should be in the given range of Min and Max
    static_assert(fnCheckInBuffofFLen<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_F_LEN, TP_API, TP_COMPUTE_MODE>(),
                  " Assertion Failed : \n "
                  "             ERROR: TP_F_LEN must be a multiple of data_load = 256/samplesize(TT_DATA_F) : \n"
                  "                   [int8 - (32*N)] [int16/bfloat16 - (16*N)] [int32/cint16/float - (8*N)] \n"
                  "                   [cint32/cfloat - (4*N)]  where N >= 2 \n"
                  "             TP_F_LEN in [2*data_load, MAX_F] where \n"
                  "             MAX_F = min(__DATA_MEM_BYTES__/sizeof(TT_DATA_F), \n"
                  "                         FULL:  __DATA_MEM_BYTES__/sizeof(TT_DATA_OUT) - G_min + 1, \n"
                  "                         SAME:  __DATA_MEM_BYTES__/sizeof(TT_DATA_OUT), \n"
                  "                         VALID: __DATA_MEM_BYTES__/sizeof(TT_DATA_OUT) + G_min - 1).");

    static_assert(fnCheckInBuffofGLen<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_F_LEN, TP_G_LEN, TP_API, TP_COMPUTE_MODE>(),
                  " Assertion Failed : \n "
                  "             ERROR: TP_G_LEN must be a multiple of data_load = 256/samplesize(TT_DATA_G) : \n"
                  "                   [int8 - (32*N)] [int16/bfloat16 - (16*N)] [int32/cint16/float - (8*N)] \n"
                  "                   [cint32/cfloat - (4*N)]  where N >= 2 \n"
                  "             TP_G_LEN in [2*data_load, MAX_G] where \n"
                  "             MAX_G = min(TP_F_LEN, __DATA_MEM_BYTES__/sizeof(TT_DATA_G), \n"
                  "                         FULL:  __DATA_MEM_BYTES__/sizeof(TT_DATA_OUT) - TP_F_LEN + 1, \n"
                  "                         SAME/VALID: __DATA_MEM_BYTES__/sizeof(TT_DATA_G)).");
    // defensive check for scaling factor should be in the range i.e. 0 < SHIFT < 61
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX,
                  " Assertion Failed : \n "
                  "              ERROR: TP_SHIFT is out of the supported range.");

    // defensive check for API Port is whether iobuffer or stream (stream not supported in current version).
    static_assert(fnCheckIsStreamSupportedbyArch<TP_API>(),
                  " Assertion Failed : \n"
                  "            ERROR: TP_API must be \n "
                  "                           -0 for 'iobuffer' \n "
                  "                           -1 for 'Stream' \n");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX,
                  " Assertion Failed : \n"
                  "             ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2,
                  " Assertion Failed : \n"
                  "              ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");

    // STREAM BASED PROCESSING: DEFENSIVE CHECKS
    // defensive check for (TP_G_LEN/TP_CASC_LEN)should be multiples of (Lanes*Points) when Stream only Processing
    // happening
    static_assert(fnCheckCascLen<TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_CASC_LEN, TP_PHASES, TP_API>(),
                  "Assertion Failed : \n"
                  "            ERROR: TP_CASC_LEN should be equal to (TP_G_LEN/8) or \n"
                  "            TP_CASC_LEN should be (TP_G_LEN/16) or (TP_G_LEN/32) for less throughput requirement\n");

    // defensive check for PHASES which should be power of 2
    static_assert(
        fnCheckPhases<TT_DATA_F, TT_DATA_G, TP_G_LEN, TP_CASC_LEN, TP_PHASES, TP_API>(),
        "Assertion Failed : \n"
        "            ERROR: TP_PHASES can be greater than 1 only when TP_CASC_LEN should be equal to (TP_G_LEN/8) \n "
        "                  TP_PHASES is always 1 for both TP_API=0 and TP_API==1 \n ");

    /**
        * The input data to the function.
    **/
    port_array<input, TP_PHASES> inF;
    input_port inG;

    /**
        * An API of TT_DATA type.
    **/
    port_array<output, TP_PHASES> out;

    /**
     * Conditional RTP port array for runtime F and G vector lengths.
     * Active (1 port) when TP_USE_RTP_VECTOR_LENGTHS = 1, empty otherwise.
     */
    rtp_port_array rtpVecLen;

    /**
        * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    kernel m_conv_corr_ref[TP_PHASES];

   public:
    /**
     * @brief Updates the RTP port with runtime F and G vector lengths.
     *
     * When TP_USE_RTP_VECTOR_LENGTHS = 1, call this method before each kernel invocation
     * to pass the runtime F and G lengths down to the kernel via the RTP port.
     *
     * @tparam TopGraph    The type of the top-level graph containing this conv_corr_ref_graph instance.
     * @tparam T_RtpArray  The type of the RTP port array (deduced from the argument).
     * @param top      Reference to the top-level graph used to call update().
     * @param rtpPort  Reference to the top-level graph's RTP port array to update.
     * @param rtpFLen     Runtime F vector length.
     * @param rtpGLen     Runtime G vector length.
     **/
    template <typename TopGraph, typename T_RtpArray>
    void update_rtp(TopGraph& top, T_RtpArray& rtpPort, unsigned int rtpFLen, unsigned int rtpGLen) {
        if
            constexpr(TP_USE_RTP_VECTOR_LENGTHS == 1) {
                const std::array<int32_t, 2> vecLens = {static_cast<int32_t>(rtpFLen), static_cast<int32_t>(rtpGLen)};
                top.update(rtpPort[0], vecLens.data(), 2);
            }
    }

    // Constructor
    conv_corr_ref_graph() {
        // Create CONV_CORR class
        static constexpr unsigned int kLanes = fnRefNumLanes<TT_DATA_F, TT_DATA_G>();
        static constexpr int kMinGLenWin =
            ((TP_USE_RTP_VECTOR_LENGTHS == 1) && (TP_COMPUTE_MODE == VALID_MODE)) ? getMinLen<TT_DATA_G>() : TP_G_LEN;
        static constexpr int kLoopCount = getRefLoopCount<TP_COMPUTE_MODE, TP_F_LEN, kMinGLenWin>();
        static constexpr unsigned int kRefLoopCount = (CEIL(kLoopCount, kLanes) / kLanes);
        static constexpr unsigned int kRefOutLen = (kRefLoopCount * kLanes);

        // make connections
        m_conv_corr_ref[0] =
            kernel::create_object<conv_corr_ref<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE,
                                                TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES,
                                                TP_CASC_LEN, TP_PHASES, TP_USE_RTP_VECTOR_LENGTHS> >();
        connect<>(inF[0], m_conv_corr_ref[0].in[0]);
        if (TP_API == USE_WINDOW_API) {
            dimensions(m_conv_corr_ref[0].in[0]) = {TP_F_LEN * TP_NUM_FRAMES};
        } else {
            dimensions(m_conv_corr_ref[0].in[0]) = {TP_F_LEN};
        }

        connect<>(inG, m_conv_corr_ref[0].in[1]);
        dimensions(m_conv_corr_ref[0].in[1]) = {TP_G_LEN * TP_NUM_FRAMES};
        if
            constexpr(TP_USE_RTP_VECTOR_LENGTHS == 1) {
                connect<parameter>(rtpVecLen[0], async(m_conv_corr_ref[0].in[2]));
            }

        connect<>(m_conv_corr_ref[0].out[0], out[0]);
        if (TP_API == USE_WINDOW_API) {
            dimensions(m_conv_corr_ref[0].out[0]) = {kRefOutLen * TP_NUM_FRAMES};
        } else {
            dimensions(m_conv_corr_ref[0].out[0]) = {TP_F_LEN};
        }

        if
            constexpr(TP_API == USE_WINDOW_API) {
                static constexpr bool useSingleBufferF =
                    (TP_F_LEN * sizeof(TT_DATA_F) * TP_NUM_FRAMES) > (__DATA_MEM_BYTES__ / 2) ? true : false;
                static constexpr bool useSingleBufferG =
                    (TP_G_LEN * sizeof(TT_DATA_G) * TP_NUM_FRAMES) > (__DATA_MEM_BYTES__ / 2) ? true : false;
                static constexpr bool useSingleBufferOut =
                    (kRefOutLen * sizeof(TT_DATA_OUT) * TP_NUM_FRAMES) > (__DATA_MEM_BYTES__ / 2) ? true : false;
                if
                    constexpr(useSingleBufferF) { single_buffer(m_conv_corr_ref[0].in[0]); }
                if
                    constexpr(useSingleBufferG) { single_buffer(m_conv_corr_ref[0].in[1]); }
                if
                    constexpr(useSingleBufferOut) { single_buffer(m_conv_corr_ref[0].out[0]); }
            }

        runtime<ratio>(m_conv_corr_ref[0]) = 0.8;

        // Source files
        source(m_conv_corr_ref[0]) = "conv_corr_ref.cpp";
        headers(m_conv_corr_ref[0]) = {"conv_corr_ref.hpp"};

        printf("==========================================\n");
        printf("== END of CONV_CORR REF KERNEL Graph =====\n");
        printf("==========================================\n");
    }; // constructor
};
} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_CONV_CORR_REF_GRAPH_HPP_
