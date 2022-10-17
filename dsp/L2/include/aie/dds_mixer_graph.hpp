/*
 * Copyright 2022 Xilinx, Inc.
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
 *         output from the dds_mixer function. This is a typename and must be one
 *         of the following: \n
 *         cint16, cint32, cfloat. Note that for cint32, the internal DDS still works to int16 precision,
 *         so Mixer Mode 0 will be cast, though for Modes 1 and 2, data to be mixed will be cint32.
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples in the input/output window API
 *          or number of samples to process per iteration.
 * @tparam TP_MIXER_MODE describes the mode of operation of the dds_mixer.  \n
 *         The values supported are: \n
 *         0 (dds only mode), \n 1 (dds plus single data channel mixer),  \n
 *         2 (dds plus two data channel mixer for symmetrical carriers)
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 * @tparam TP_SSR specifies the super sample rate, ie how much data input/output in parallel for a single channel.  \n
 *         There will be a TP_SSR number of kernels, with a TP_SSR number of each port used on the interface. \n
 *         A default value of 1 corresponds to the typical single kernel case.
 **/
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_API = IO_API::WINDOW,
          unsigned int TP_SSR = 1>
class dds_mixer_graph : public graph {
   private:
   public:
    static_assert(TP_SSR > 0, "ERROR: Invalid SSR value, must be a value greater than 0.\n");
    // type alias to determine if port type is window or stream
    using inPortType = typename std::
        conditional<(TP_API == IO_API::WINDOW), window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)>, stream>::type;
    // outPortType is the same as inPortType - may not always be true - future compatible with seperate alias
    using outPortType = inPortType;

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

    using kernelClass = dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>;

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
            if (TP_MIXER_MODE == 1 || TP_MIXER_MODE == 2) connect<inPortType>(in1[ssrIdx], m_ddsKernel[ssrIdx].in[0]);
            if (TP_MIXER_MODE == 2) connect<inPortType>(in2[ssrIdx], m_ddsKernel[ssrIdx].in[1]);

            connect<outPortType>(m_ddsKernel[ssrIdx].out[0], out[ssrIdx]);
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
