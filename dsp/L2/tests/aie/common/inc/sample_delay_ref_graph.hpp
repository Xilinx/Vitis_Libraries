/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_SAMPLE_DELAY_REF_GRAPH_HPP_
#define _DSPLIB_SAMPLE_DELAY_REF_GRAPH_HPP_

/*
This file captures the definition of 'L2' graph level class for
the sample_delay reference mdodel.
*/

#include <adf.h>
#include "sample_delay_ref.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace sample_delay {

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_API, unsigned int TP_MAX_DELAY>
class sample_delay_ref_graph : public graph {
   public:
    input_port in;
    input_port numSampleDelay;
    output_port out;

    kernel m_kernel;

    // constructor
    sample_delay_ref_graph() {
        m_kernel = kernel::create_object<sample_delay_ref<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY> >();
        connect<>(in, m_kernel.in[0]);
        connect<>(m_kernel.out[0], out);
        dimensions(m_kernel.in[0]) = {TP_WINDOW_VSIZE};
        dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE};
        connect<parameter>(numSampleDelay, async(m_kernel.in[1])); // RTP

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.

        // Specify source file
        source(m_kernel) = "sample_delay_ref.cpp";
    };
    // constructor arg
    sample_delay_ref_graph(unsigned int sampleDelayVal) {
        m_kernel =
            kernel::create_object<sample_delay_ref<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY> >(sampleDelayVal);
        connect<>(in, m_kernel.in[0]);
        connect<>(m_kernel.out[0], out);
        dimensions(m_kernel.in[0]) = {TP_WINDOW_VSIZE};
        dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE};
        connect<parameter>(numSampleDelay, async(m_kernel.in[1])); // RTP
        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Specify source file
        source(m_kernel) = "sample_delay_ref.cpp";
    };
};
}
}
}
} // namespace brackets

#endif // _DSPLIB_SAMPLE_DELAY_REF_GRAPH_HPP_
