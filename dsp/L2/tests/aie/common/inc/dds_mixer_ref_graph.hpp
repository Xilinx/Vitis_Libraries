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
#ifndef _DSPLIB_dds_mixer_REF_GRAPH_HPP_
#define _DSPLIB_dds_mixer_REF_GRAPH_HPP_

/*
This file holds the definition of the dds_mixer
Reference model graph.
*/

#include <adf.h>
#include <vector>
#include "dds_mixer_ref.hpp"
#include "fir_ref_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {
using namespace adf;

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 1 // ignored
          >

class dds_mixer_ref_graph : public graph {
   public:
    std::array<port<input>, 1> in1;
    std::array<port<input>, 1> in2;
    std::array<port<output>, 1> out;

    // DDS Kernel
    kernel m_ddsKernel;

    // Constructor
    dds_mixer_ref_graph(uint32_t phaseInc, uint32_t initialPhaseOffset = 0) {
        printf("========================\n");
        printf("== DDS_MIXER_REF Graph  \n");
        printf("========================\n");

        // Create DDS_MIXER_REF kernel
        // IO_API is ignored because it's basically just a implementation detail
        m_ddsKernel = kernel::create_object<dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE> >(
            phaseInc, initialPhaseOffset);

        // Make connections
        // Size of window in Bytes.
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(in1[0], m_ddsKernel.in[0]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(in2[0], m_ddsKernel.in[1]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_ddsKernel.out[0], out[0]);
        // Specify mapping constraints
        runtime<ratio>(m_ddsKernel) = 0.4;

        // Source files
        source(m_ddsKernel) = "dds_mixer_ref.cpp";
    };
};

//--------------------------------------------------------------------------------------------------
// SPECIALIZATION for TP_MIXER_MODE = 1  (dds plus single input mixer configuration)
//--------------------------------------------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API, unsigned int TP_SSR>

class dds_mixer_ref_graph<TT_DATA, TP_INPUT_WINDOW_VSIZE, 1, TP_API, TP_SSR> : public graph {
   public:
    std::array<port<input>, 1> in1;
    std::array<port<output>, 1> out;

    // DDS Kernel
    kernel m_ddsKernel;

    // Constructor
    dds_mixer_ref_graph(uint32_t phaseInc, uint32_t initialPhaseOffset = 0) {
        printf("======================================\n");
        printf("== DDS_MIXER_REF Graph MIXER MODE = 1 \n");
        printf("======================================\n");

        // Create DDS_MIXER_REF kernel
        // IO_API is ignored because it's basically just a implementation detail
        m_ddsKernel =
            kernel::create_object<dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, 1> >(phaseInc, initialPhaseOffset);

        // Make connections
        // Size of window in Bytes.
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(in1[0], m_ddsKernel.in[0]);
        //     connect<window<TP_INPUT_WINDOW_VSIZE*sizeof(TT_DATA)> >(in2, m_ddsKernel.in[1]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_ddsKernel.out[0], out[0]);
        // Specify mapping constraints
        runtime<ratio>(m_ddsKernel) = 0.4;

        // Source files
        source(m_ddsKernel) = "dds_mixer_ref.cpp";
    };
};

//--------------------------------------------------------------------------------------------------
// SPECIALIZATION for TP_MIXER_MODE = 0  (dds only configuration)
//--------------------------------------------------------------------------------------------------

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API, unsigned int TP_SSR>

class dds_mixer_ref_graph<TT_DATA, TP_INPUT_WINDOW_VSIZE, 0, TP_API, TP_SSR> : public graph {
   public:
    std::array<port<output>, 1> out;

    // DDS Kernel
    kernel m_ddsKernel;

    // Constructor
    dds_mixer_ref_graph(uint32_t phaseInc, uint32_t initialPhaseOffset = 0) {
        printf("======================================\n");
        printf("== DDS_MIXER_REF Graph MIXER MODE = 0 \n");
        printf("======================================\n");

        // Create DDS_MIXER_REF kernel
        // IO_API is ignored because it's basically just a implementation detail
        m_ddsKernel =
            kernel::create_object<dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, 0> >(phaseInc, initialPhaseOffset);

        // Make connections
        // Size of window in Bytes.
        //    connect<window<TP_INPUT_WINDOW_VSIZE*sizeof(TT_DATA)> >(in1, m_ddsKernel.in[0]);
        //     connect<window<TP_INPUT_WINDOW_VSIZE*sizeof(TT_DATA)> >(in2, m_ddsKernel.in[1]);
        connect<window<TP_INPUT_WINDOW_VSIZE * sizeof(TT_DATA)> >(m_ddsKernel.out[0], out[0]);
        // Specify mapping constraints
        runtime<ratio>(m_ddsKernel) = 0.4;

        // Source files
        source(m_ddsKernel) = "dds_mixer_ref.cpp";
    };
};
}
}
}
}
}
#endif // _DSPLIB_dds_mixer_REF_GRAPH_HPP_
