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
#ifndef _DSPLIB_WIDGET_2CH_REAL_FFT_GRAPH_HPP_
#define _DSPLIB_WIDGET_2CH_REAL_FFT_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Widget 2-Channel Real FFT function library element.
*/
/**
 * @file widget_2ch_real_fft_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "widget_2ch_real_fft.hpp"
// #include "widget_2ch_real_fft_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget_2ch_real_fft {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// widget_2ch_real_fft_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup widget_2ch_real_fft
 * @brief This widget takes in the FFT of a complex signal and disentangles it into it's respective real FFTs.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following: \n
 *         cint16, cbfloat16
 * @tparam TP_POINT_SIZE describes the number of samples in the list.
 * @tparam TP_WINDOW_VSIZE describes the number of lists to sort per call to the kernel. \n
 **/
template < // typename TT_DATA_SPOOF, // Hardwiring for testing cbfloat16s..
    typename TT_DATA,
    unsigned int TP_POINT_SIZE,
    unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE>
class widget_2ch_real_fft_graph : public graph {
   public:
    // using TT_DATA = cbfloat16;
    static constexpr unsigned int kBufferBytes = 32;

    static_assert(TP_WINDOW_VSIZE * sizeof(TT_DATA) * 2 >= kBufferBytes,
                  "ERROR: TP_WINDOW_VSIZE * sizeof(TT_DATA) * 2 must be greater than or equal to 32 bytes.");
    static_assert(TP_WINDOW_VSIZE * sizeof(TT_DATA) * 2 <= __DATA_MEM_BYTES__,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be less than or equal to buffer size, 32kB for "
                  "AIE1 and 64kB for AIE-ML.");

    /**
     * The input data to the function.
     **/
    port_array<input, 1> in;
    /**
     * The output data.
     **/
    port_array<output, 1> out;
    /**
     * The kernel which will be created and mapped onto AIE tiles.
     **/
    kernel m_kernel;
    /**
     * @brief This is the constructor function for the widget_2ch_real_fft graph.
     **/
    widget_2ch_real_fft_graph() {
        m_kernel = kernel::create_object<widget_2ch_real_fft<TT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE> >();

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.9; // Nominal figure. The real figure requires knowledge of the sample rate.

        // Source files
        source(m_kernel) = "widget_2ch_real_fft.cpp";

        // stack_size(m_kernel) = 2048;

        // make connections
        connect(in[0], m_kernel.in[0]);
        dimensions(m_kernel.in[0]) = {TP_WINDOW_VSIZE};

        connect(m_kernel.out[0], out[0]);
        dimensions(m_kernel.out[0]) = {TP_WINDOW_VSIZE};
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_WIDGET_2CH_REAL_FFT_GRAPH_HPP_
