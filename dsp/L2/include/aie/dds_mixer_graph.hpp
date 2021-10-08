/*
 * Copyright 2021 Xilinx, Inc.
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
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the dds_mixer function. This is a typename and must be one
 *         of the following: \n
 *         cint16
 * @tparam TP_INPUT_WINDOW_VSIZE describes the number of samples in the input/output window API
 *          or number of samples to process per iteration.
 * @tparam TP_MIXER_MODE describes the mode of operation of the dds_mixer.  \n
 *         The values supported are: \n
 *         0 (dds only mode), \n 1 (dds plus single data channel mixer),  \n
 *         2 (dds plus two data channel mixer for symmetrical carriers)
 * @tparam TP_API specifies if the input/output interface should be window-based or stream-based.  \n
 *         The values supported are 0 (window API) or 1 (stream API).
 **/
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE = MIXER_MODE_2, // default is dds plus two inputs to mixers
          unsigned int TP_API = IO_API::WINDOW>
class dds_mixer_graph : public graph {
   private:
   public:
    // type alias to determine if port type is window or stream
    using inPortType = typename std::
        conditional<(TP_API == IO_API::WINDOW), window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)>, stream>::type;
    // outPortType is the same as inPortType - may not always be true - future compatible with seperate alias
    using outPortType = inPortType;

    /**
     * The input data to the function. When in TP_API=WINDOW, the port is a window of
     * samples of TT_DATA type. The number of samples in the window is
     * described by TP_INPUT_WINDOW_VSIZE.
     **/
    port<input> in1;
    port<input> in2;
    /**
     * An output port of TT_DATA type. When in TP_API=WINDOW, the port is a window of TP_INPUT_WINDOW_VSIZE samples.
     **/
    port<output> out;

    /**
     * kernel instance used to set constraints - getKernels function returns a pointer to this.
    **/
    kernel m_ddsKernel;

    /**
     * Access function for getting kernel - useful for setting runtime ratio,
     * location constraints, fifo_depth (for stream), etc.
    **/
    kernel* getKernels() { return &m_ddsKernel; };

    /**
     * @brief This is the constructor function for the dds_mixer graph.
     * @param[in] phaseInc specifies the phase increment between samples.
     *                                 Input value 2^31 corresponds to Pi (i.e. 180').
     **/
    dds_mixer_graph(const uint32_t phaseInc) {
        m_ddsKernel =
            kernel::create_object<dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API> >(phaseInc);

        connect<inPortType>(in1, m_ddsKernel.in[0]);
        connect<inPortType>(in2, m_ddsKernel.in[1]);
        connect<outPortType>(m_ddsKernel.out[0], out);
        // Specify mapping constraints
        runtime<ratio>(m_ddsKernel) = 0.8;
        // Source files
        source(m_ddsKernel) = "dds_mixer.cpp";
    }
};

/**
  * @cond NOCOMMENTS
  */
//--------------------------------------------------------------------------------------------------
// SPECIALIZATION for TP_MIXER_MODE = 1  (dds plus single input mixer configuration)
//--------------------------------------------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
/**
 * This is the class for the dds_mixer graph MIXER_MODE_1
 **/
class dds_mixer_graph<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API> : public graph {
   public:
    port<input> in1;

    port<output> out;

    using inPortType = typename std::
        conditional<(TP_API == IO_API::WINDOW), window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)>, stream>::type;
    // may not always be true - future compatible
    using outPortType = inPortType;

    kernel m_ddsKernel;

    // Access function for AIE synthesizer. (Note the use of & for a single kernal)
    kernel* getKernels() { return &m_ddsKernel; };

    dds_mixer_graph(const uint32_t phaseInc) {
        m_ddsKernel = kernel::create_object<dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API> >(
            phaseInc); // TP_MIXER_MODE = 1

        connect<inPortType>(in1, m_ddsKernel.in[0]);
        connect<outPortType>(m_ddsKernel.out[0], out);
        // Specify mapping constraints
        runtime<ratio>(m_ddsKernel) = 0.8;
        // Source files
        source(m_ddsKernel) = "dds_mixer.cpp";
    }
};

//--------------------------------------------------------------------------------------------------
// SPECIALIZATION for TP_MIXER_MODE = 0  (dds only configuration)
//--------------------------------------------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>

class dds_mixer_graph<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API> : public graph {
   public:
    port<output> out;
    using outPortType = typename std::
        conditional<(TP_API == IO_API::WINDOW), window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)>, stream>::type;
    kernel m_ddsKernel;

    // Access function for AIE synthesizer. (Note the use of & for a single kernel)
    kernel* getKernels() { return &m_ddsKernel; };

    dds_mixer_graph(const uint32_t phaseInc) {
        m_ddsKernel = kernel::create_object<dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API> >(
            phaseInc); // TP_MIXER_MODE = 1

        connect<outPortType>(m_ddsKernel.out[0], out);
        // Specify mapping constraints
        runtime<ratio>(m_ddsKernel) = 0.8;
        // Source files
        source(m_ddsKernel) = "dds_mixer.cpp";
    }
};

/**
  * @endcond
  */
}
}
}
}
} // namespace braces
#endif //_DSPLIB_DDS_MIXER_GRAPH_HPP_
