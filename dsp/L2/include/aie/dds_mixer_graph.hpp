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
#ifndef _DSPLIB_DDS_MIXER_GRAPH_HPP_
#define _DSPLIB_DDS_MIXER_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the DDS_MIXER library element.
*/

#include <adf.h>
#include <vector>
#include <tuple>

#include "graph_utils.hpp"
#include "dds_mixer.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {
using namespace adf;

/**
 * @defgroup dds_graph DDS / Mixer
 *
 * DDS contains a DDS and Mixer solution.
 *
 */

//--------------------------------------------------------------------------------------------------
// dds_mixer_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief dds_mixer operates in 3 modes: \n
 *      **Mixer Mode 0:** \n
 *                       This is dds mode only. The library element has a single output window,
 *                       which is written to with the sin/cos components corresponding to the
 *                       programmed phase increment. \n
 *      **Mixer Mode 1:** \n
 *                       This is dds plus mixer for a single data input port. \n Each data input
 *                       sample is complex multiplied with the corresponding dds sample, to
 *                       create a modulated signal that is written to the output window. \n
 *      **Mixer Mode 2:** \n
 *                       This is a special configuration for symmetrical carriers and two data
 *                       input ports. \n Each data sample of the first input is complex multiplied
 *                       with the corresponding dds sample to create a modulated signal. \n
 * These are the templates to configure the dds_mixer class. \n
 *
 * @ingroup dds_graph
 *
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the dds_mixer function. \n
 *         This is a typename and must be one of the following: \n
 *         cint16, cint32, cfloat. Note that for cint32, the internal DDS still works to int16 precision,
 *         so Mixer Mode 0 will be cast, though for Modes 1 and 2, data to be mixed will be cint32.
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples in the input/output buffer API
 *          or number of samples to process per iteration.
 * @tparam TP_MIXER_MODE describes the mode of operation of the dds_mixer.  \n
 *         The values supported are: \n
 *         0 (dds only mode), \n 1 (dds plus single data channel mixer),  \n
 *         2 (dds plus two data channel mixer for symmetrical carriers)
 * @tparam TP_USE_PHASE_RELOAD allows the user to select if runtime phase offset
 *         should be used. \n When defining the parameter:
 *         - 0 = static phase initialization, defined in dds constructor,
 *         - 1 = reloadable initial phase, passed as argument to runtime function. \n
 *         \n
 *         The form of the port is defined by TP_PHASE_RELOAD_METHOD.\n
 * @tparam TP_PHASE_RELOAD_API defines the form of the phase_offset port.\n
 *         The values supported are:\n
 *         - 0 = RTP. \n      i.e. non-blocking \n
 *         - 1 = IOBUFFER \n  i.e. blocking \n
 * @tparam TP_API specifies if the input/output interface should be buffer-based or stream-based.  \n
 *         The values supported are 0 (buffer API) or 1 (stream API).
 * @tparam TP_SSR specifies the super sample rate, ie how much data input/output in parallel for a single channel.  \n
 *         There will be a TP_SSR number of kernels, with a TP_SSR number of each port used on the interface. \n
 *         A default value of 1 corresponds to the typical single kernel case.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards nearest even number.
 *         - rnd_conv_odd   = Round halfway towards nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 *         \n
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML and AIE-MLv2 device.
 *\n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_USE_PHASE_INC_RELOAD allows the user to select if phase increment
 *         can be changed at runtimed. \n When defining the parameter:
 *         - 0 = static phase increment initialization, defined in dds constructor,
 *         - 1 = reloadable initial phase increment, passed as argument to runtime function. \n
 *         Currently on RTP update is supported.
 *         \n
 **/
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_API = IO_API::WINDOW,
          unsigned int TP_SSR = 1,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_USE_PHASE_RELOAD = 0,
          unsigned int TP_PHASE_RELOAD_API = 0,
          unsigned int TP_USE_PHASE_INC_RELOAD = 0>
class dds_mixer_graph : public graph {
   private:
   public:
    static_assert(TP_SSR > 0, "ERROR: Invalid SSR value, must be a value greater than 0.\n");
    // type alias to determine if port type is window or stream
    static_assert(TP_INPUT_WINDOW_VSIZE % TP_SSR == 0,
                  "ERROR: Unsupported frame size. TP_INPUT_WINDOW_VSIZE must be divisible by TP_SSR");
    static_assert(__SINCOS_IN_HW__ == 1,
                  "ERROR: This device does not support this implementation of the DDS. Please use the dds_mixer_lut "
                  "library element for this application.");
    static_assert(!(TP_USE_PHASE_RELOAD == 1 && TP_SSR > 1),
                  "ERROR: Phase Offset Update cannot be used for TP_SSR > 1!");
    static_assert(!(TP_USE_PHASE_INC_RELOAD == 1 && TP_SSR > 1),
                  "ERROR: Phase Increment Update cannot be used for TP_SSR > 1!");
    static_assert(!(TP_USE_PHASE_RELOAD == 0 && TP_PHASE_RELOAD_API == 1),
                  "ERROR: TP_PHASE_RELOAD_API is void when TP_USE_PHASE_RELOAD = 0");
    static_assert(!(TP_MIXER_MODE == 2 && TP_SAT == 0), "ERROR: DDS/Mixer mode 2 is not supported with TP_SAT = 0.");

    template <typename direction>
    using portArray = std::array<port<direction>, TP_SSR>;

    /**
     * The input data to the function. When in TP_API=WINDOW, the port is a window of
     * samples of TT_DATA type. The number of samples in the window is
     * described by TP_INPUT_WINDOW_VSIZE.
     **/
    portArray<input> in1;
    portArray<input> in2;

    /**
     * Conditional port. Present when phase offset is a run-time input
     */
    port_conditional_array<input, (TP_USE_PHASE_RELOAD == 1), TP_SSR> PhaseRTP;

    /**
     * Conditional port. Present when phase increment is a run-time input
     */
    port_conditional_array<input, (TP_USE_PHASE_INC_RELOAD == 1), TP_SSR> PhaseIncRTP;

    /**
     * An output port of TT_DATA type. When in TP_API=WINDOW, the port is a window of TP_INPUT_WINDOW_VSIZE samples.
     **/
    portArray<output> out;

    /**
     * kernel instance used to set constraints - getKernels function returns a pointer to this.
    **/
    kernel m_ddsKernel[TP_SSR];

    /**
     * Access function for getting kernel - useful for setting runtime ratio,
     * location constraints, fifo_depth (for stream), etc.
    **/
    kernel* getKernels() { return m_ddsKernel; };

    static constexpr unsigned int KINPUT_WINDOW_VSIZE = TP_INPUT_WINDOW_VSIZE / TP_SSR;
    static constexpr unsigned int TP_NUM_LUTS = 1;
    using kernelClass = dds_mixer<TT_DATA,
                                  KINPUT_WINDOW_VSIZE,
                                  TP_MIXER_MODE,
                                  TP_USE_PHASE_RELOAD,
                                  TP_API,
                                  USE_INBUILT_SINCOS,
                                  TP_NUM_LUTS,
                                  TP_RND,
                                  TP_SAT,
                                  TP_PHASE_RELOAD_API,
                                  TP_USE_PHASE_INC_RELOAD>;

    /**
     * @brief This is the constructor function for the dds_mixer graph.
     * @param[in] phaseInc specifies the phase increment between samples.
     *            Input value 2^31 corresponds to Pi (i.e. 180').
     * @param[in] initialPhaseOffset specifies the initial value of the phase accumulator, creating a phase offset.
     *                                 Input value 2^31 corresponds to Pi (i.e. 180').
     **/
    dds_mixer_graph(const uint32_t phaseInc, const uint32_t initialPhaseOffset = 0) {
        for (unsigned int ssrIdx = 0; ssrIdx < TP_SSR; ssrIdx++) {
            m_ddsKernel[ssrIdx] = kernel::create_object<kernelClass>(uint32_t(phaseInc * TP_SSR),
                                                                     uint32_t(initialPhaseOffset + phaseInc * ssrIdx));

            // optionally connect PHASE OFFSET port
            if
                constexpr(TP_USE_PHASE_RELOAD == 1) {
                    if
                        constexpr(TP_PHASE_RELOAD_API == USE_PHASE_RELOAD_API_RTP) {
                            if
                                constexpr(TP_MIXER_MODE == 0) {
                                    connect<parameter>(PhaseRTP[ssrIdx], async(m_ddsKernel[ssrIdx].in[0]));
                                }

                            if
                                constexpr(TP_MIXER_MODE == 1) {
                                    connect<parameter>(PhaseRTP[ssrIdx], async(m_ddsKernel[ssrIdx].in[1]));
                                }

                            if
                                constexpr(TP_MIXER_MODE == 2) {
                                    connect<parameter>(PhaseRTP[ssrIdx], async(m_ddsKernel[ssrIdx].in[2]));
                                }
                        }
                    else { // i.e. assume TP_PHASE_RELOAD_API == USE_PHASE_RELOAD_API_IOBUFF
                        // iobuffer must be 32 bytes min. This carries only one uint32.
                        unsigned int kSizeofPhaseOffsetIObuff = 32 / sizeof(uint32);
                        if
                            constexpr(TP_MIXER_MODE == 0) {
                                connect<>(PhaseRTP[ssrIdx], m_ddsKernel[ssrIdx].in[0]);
                                dimensions(m_ddsKernel[ssrIdx].in[0]) = {kSizeofPhaseOffsetIObuff};
                            }

                        if
                            constexpr(TP_MIXER_MODE == 1) {
                                connect<>(PhaseRTP[ssrIdx], m_ddsKernel[ssrIdx].in[1]);
                                dimensions(m_ddsKernel[ssrIdx].in[1]) = {kSizeofPhaseOffsetIObuff};
                            }

                        if
                            constexpr(TP_MIXER_MODE == 2) {
                                connect<>(PhaseRTP[ssrIdx], m_ddsKernel[ssrIdx].in[2]);
                                dimensions(m_ddsKernel[ssrIdx].in[2]) = {kSizeofPhaseOffsetIObuff};
                            }
                    }
                }

            // Connect in1 if it exists
            if
                constexpr(TP_MIXER_MODE == 1 || TP_MIXER_MODE == 2) {
                    if
                        constexpr(TP_API == 0) {
                            connect<>(in1[ssrIdx], m_ddsKernel[ssrIdx].in[0]);
                            dimensions(m_ddsKernel[ssrIdx].in[0]) = {KINPUT_WINDOW_VSIZE};
                        }
                    else {
                        connect<stream>(in1[ssrIdx], m_ddsKernel[ssrIdx].in[0]);
                    }
                }
            // Connect in2 if it exists
            if
                constexpr(TP_MIXER_MODE == 2) {
                    if
                        constexpr(TP_API == 0) {
                            connect<>(in2[ssrIdx], m_ddsKernel[ssrIdx].in[1]);
                            dimensions(m_ddsKernel[ssrIdx].in[1]) = {KINPUT_WINDOW_VSIZE};
                        }
                    else {
                        connect<stream>(in2[ssrIdx], m_ddsKernel[ssrIdx].in[1]);
                    }
                }

            // optionally connect phaseInc port
            if
                constexpr(TP_USE_PHASE_INC_RELOAD == 1) {
                    unsigned int portIndex = TP_MIXER_MODE + TP_USE_PHASE_RELOAD;
                    connect<parameter>(PhaseIncRTP[ssrIdx], async(m_ddsKernel[ssrIdx].in[portIndex]));
                }

            // Connect output
            if
                constexpr(TP_API == 0) {
                    connect<>(m_ddsKernel[ssrIdx].out[0], out[ssrIdx]);
                    dimensions(m_ddsKernel[ssrIdx].out[0]) = {KINPUT_WINDOW_VSIZE};
                }
            else {
                connect<stream>(m_ddsKernel[ssrIdx].out[0], out[ssrIdx]);
            }
            // Specify mapping constraints
            runtime<ratio>(m_ddsKernel[ssrIdx]) = 0.8;
            // Source files
            source(m_ddsKernel[ssrIdx]) = "dds_mixer.cpp";
        }
    }
};
}
}
}
}
} // namespace braces
#endif //_DSPLIB_DDS_MIXER_GRAPH_HPP_
