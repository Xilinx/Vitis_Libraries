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
#ifndef _DSPLIB_SAMPLE_DELAY_GRAPH_HPP_
#define _DSPLIB_SAMPLE_DELAY_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the sample_delay library element.
*/
/**
 * @file sample_delay_graph.hpp
 *
 **/

#include <adf.h>
#include "sample_delay.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace sample_delay {

//--------------------------------------------------------------------------------------------------
// sample_delay_graph template
//--------------------------------------------------------------------------------------------------

/**
 * @defgroup sample_delay_graph Sample Delay
 *
 * Vectorized Sample Delay
**/

/**
 * @brief sample_delay introduces delay to the input data. The numSampledelay is a Run Time Parameter (RTP) for setting
 *the value of delay.
 *
 * @ingroup sample_delay_graph
 *
 * These are the templates parameters:
 * @tparam TT_DATA describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         unit8, int8, int16, int32, float, cint16, cint32, cfloat.
 * @tparam TP_WINDOW_VSIZE describes the number of samples in the window API. \n It is only applicable when TP_API = 0.
 * @tparam TP_API describes if the interface is IOBuf/Window (0) or stream (1)
 * @tparam TP_MAX_DELAY sets the max threshold on the delay.
 *
 **/

template <typename TT_DATA, unsigned int TP_WINDOW_VSIZE, unsigned int TP_API, unsigned int TP_MAX_DELAY>
class sample_delay_graph : public graph {
   public:
    static constexpr int VECTOR_SIZE = 256 / 8 / sizeof(TT_DATA);
    static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                      (std::is_same<TT_DATA, cfloat>::value) || (std::is_same<TT_DATA, int8>::value) ||
                      (std::is_same<TT_DATA, int16>::value) || (std::is_same<TT_DATA, int32>::value) ||
                      (std::is_same<TT_DATA, uint8>::value) || (std::is_same<TT_DATA, float>::value),
                  "ERROR: TT_DATA is not a supported data type");

    static_assert(TP_MAX_DELAY % VECTOR_SIZE == 0, "TP_MAX_DELAY should be an integer multiple of VECTOR_SIZE");
    static_assert(TP_WINDOW_VSIZE % VECTOR_SIZE == 0, "TP_WINDOW_VSIZE should be an integer multiple of VECTOR_SIZE");
    static_assert((TP_API == 0 || TP_MAX_DELAY >= 128),
                  "ERROR: Minimum value of TP_MAX_DELAY for stream interface is 128");

    /**
     * Input data
     **/
    input_port in;

    /**
      * This is a Run Time Parameter (RTP) and sets the requested delay in terms of number of samples. The allowed range
      *is [0, TP_MAX_DELAY -1].
     **/
    input_port numSampleDelay;

    /**
      * Output data
     **/
    output_port out;

    /**
      * The kernel that will be created and mapped onto the AIE tile.
      **/
    kernel m_kernel;

    /**
      * @brief Constructor function for the sample_delay graph. \n The constructor has two forms.
      * If no constructor argument is supplied the initial sample delay will be set to 0, otherwise the
      * constructor argument sets the initial sample delay.
      **/
    // constructor arg
    sample_delay_graph(unsigned int sampleDelayVal) {
        m_kernel = kernel::create_object<sample_delay<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY> >(sampleDelayVal);
        create_port_connections(TP_API);
    };

    /**
 * @cond NOCOMMENTS
 */
    // constructor
    sample_delay_graph() {
        m_kernel = kernel::create_object<sample_delay<TT_DATA, TP_WINDOW_VSIZE, TP_API, TP_MAX_DELAY> >();
        create_port_connections(TP_API);
    };

    /**
      * @endcond
      */

    void create_port_connections(unsigned int api) {
        if (api == 1) { // axi stream
            connect<stream>(in, m_kernel.in[0]);
            connect<stream>(m_kernel.out[0], out);
            connect<parameter>(numSampleDelay, async(m_kernel.in[1])); // RTP
        } else {                                                       // io buffer
            connect<>(in, m_kernel.in[0]);
            connect<>(m_kernel.out[0], out);
            dimensions(m_kernel.in[0]) = {TP_WINDOW_VSIZE};
            dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE};
            connect<parameter>(numSampleDelay, async(m_kernel.in[1])); // RTP
        }

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.

        // Specify source file
        source(m_kernel) = "sample_delay.cpp";
    };
};
}
}
}
} // namespace brackets

#endif //_DSPLIB_SAMPLE_DELAY_GRAPH_HPP_
