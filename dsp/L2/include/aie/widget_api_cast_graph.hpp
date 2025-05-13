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
#ifndef _DSPLIB_WIDGET_API_CAST_GRAPH_HPP_
#define _DSPLIB_WIDGET_API_CAST_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the API Cast Widget library element.
*/
/**
 * @file widget_api_cast_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include "graph_utils.hpp"
#include <tuple>

#include "widget_api_cast.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace widget {
namespace api_cast {
using namespace adf;

/**
 * @defgroup widget_graph Widgets
 *
 * Contains elements that provide flexibility when connecting other kernels.
 * Widgets may change the interface type between Window buffers and Streams, as well as change the data pattern by
 * reordering or converting data samples.
 *
 */

//--------------------------------------------------------------------------------------------------
// widget_api_cast_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @brief widget_api_cast is a design to change the interface between connected components.
 *        This component is able to change the stream interface to window interface and vice-versa.
 *        In addition, multiple input stream ports may be defined, as well as multiple copies of the window output.
 *
 * @ingroup widget_graph
 *
 * These are the templates to configure the Widget API Cast class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TP_IN_API defines the input interface type. \n
 *         0 = Window, 1 = Stream
 * @tparam TP_OUT_API defines the output interface type. \n
 *         0 = Window, 1 = Stream
 * @tparam TP_NUM_INPUTS describes the number of input stream interfaces to be processed. \n
 *         When 2 inputs are configured, whe data will be read sequentially from each.
 * @tparam TP_WINDOW_VSIZE describes the number of samples in the window API
 *         used if either input or output is a window. \n
 *         \n
 *         Note: Margin size should not be included in TP_INPUT_WINDOW_VSIZE.
 * @tparam TP_NUM_OUTPUT_CLONES sets the number of output ports to write the input
 *         data to. Note that while input data from multiple ports is independent,
 *         data out is not.
 * @tparam TP_PATTERN sets the interleave or deinterleave pattern for configurations using dual
 *         streams, since streams are not considered clones for input nor for output. \n
 *         The patterns supported are:
 *         0 (default) : 128bits are taken from each input, concatenated to 256b and output to window.
 *                       or one 256b window read is split into upper and lower 128b chunks for output.
 *         1           : kSampleIntlv. One TT_DATA sample is taken from each stream and written to window or vice versa.
 *         2           : kSplit. The window is split into 2 halves with each half going to a stream.
 * @tparam TP_HEADER_BYTES sets the number of bytes at the beginning of a window which are not subject to interlace. \n
 *         These bytes are not included in TP_WINDOW_VSIZE as that refers to payload data whereas a header is intended
 *         for control information. Where this widget is configured for 2 streams in, the header is read from the first
 *         stream and copied to output. The header on the second stream is read and discarded. The header is written
 *         to all output windows or streams.
 **/
template <typename TT_DATA,
          unsigned int TP_IN_API, // 0 = Window, 1 = Stream
          unsigned int TP_OUT_API,
          unsigned int TP_NUM_INPUTS,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_NUM_OUTPUT_CLONES = 1,
          unsigned int TP_PATTERN = 0,
          unsigned int TP_HEADER_BYTES = 0>
class widget_api_cast_graph : public graph {
   public:
    static_assert(TP_IN_API == kStreamAPI || TP_IN_API == kWindowAPI,
                  "Error: TP_IN_API is not a supported value of 0 (window) or 1 (stream)");
    static_assert(TP_OUT_API == kStreamAPI || TP_OUT_API == kWindowAPI,
                  "Error: TP_OUT_API is not a supported value of 0 (window) or 1 (stream)");
    // static_assert(!(TP_IN_API == kWindowAPI && TP_NUM_INPUTS >1), "Error: Only a single input is supported for an
    // input API of Window");
    // static_assert(TP_NUM_INPUTS >=1 && TP_NUM_INPUTS<=2, "Error: TP_NUM_INPUTS is out of the supported range");
    static_assert(TP_NUM_OUTPUT_CLONES >= 1 && TP_NUM_OUTPUT_CLONES <= 4,
                  "Error: TP_NUM_OUTPUT_CLONES is out of the supported range");
    static_assert(TP_IN_API == 1 || TP_NUM_OUTPUT_CLONES < 4,
                  "Error: TP_NUM_OUTPUT_CLONES may only be 4 when stream input is configured.");
    static_assert(TP_IN_API != 1 || TP_OUT_API != 1, "Error: stream to stream connection is not supported");
    // static_assert(TP_IN_API != 0 || TP_OUT_API != 1 || TP_NUM_INPUTS==1, "Error: Window to stream supports only a
    // single connection in.");
    static_assert(TP_PATTERN >= 0 && TP_PATTERN < 3, "Error: TP_PATTERN is out of range.");
    static_assert(TP_PATTERN == 0 || (TP_IN_API == kStreamAPI && TP_NUM_INPUTS == 2) ||
                      (TP_OUT_API == kStreamAPI && TP_NUM_OUTPUT_CLONES == 2),
                  "Error: non-zero TP_PATTERN features require dual streams on input or output");
    /**
     * The input data to the function. This input may be stream or window.
     * Data is read from here and written directly to output. When there are
     * multiple input streams, a read from each will occur of the maximum size
     * supported (32 bits) with these 2 being
     * concatenated, before being written to the output(s). Multiple input
     * windows is not supported
     **/
    port<input> in[TP_NUM_INPUTS];
    /**
     * An API of TT_DATA type.
     **/
    port<output> out[TP_NUM_OUTPUT_CLONES];

    /**
     * The kernel that that will be created and mapped onto AIE tile.
     **/
    kernel m_kernel;

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/
    kernel* getKernels() { return &m_kernel; };

    /**
     * @brief This is the constructor function for the Widget API Cast graph.
     **/
    widget_api_cast_graph() {
        m_kernel = kernel::create_object<widget_api_cast<TT_DATA, TP_IN_API, TP_OUT_API, TP_NUM_INPUTS, TP_WINDOW_VSIZE,
                                                         TP_NUM_OUTPUT_CLONES, TP_PATTERN, TP_HEADER_BYTES> >();

        // Specify mapping constraints
        runtime<ratio>(m_kernel) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.

        // Source files
        source(m_kernel) = "widget_api_cast.cpp";

        // make connections
        if (TP_IN_API == kWindowAPI) {
            for (int i = 0; i < TP_NUM_INPUTS; i++) {
                //                connect<window<TP_WINDOW_VSIZE*sizeof(TT_DATA)+TP_HEADER_BYTES>>(in[i],
                //                m_kernel.in[i]);
                connect(in[i], m_kernel.in[i]);
                dimensions(m_kernel.in[i]) = {TP_WINDOW_VSIZE + TP_HEADER_BYTES / sizeof(TT_DATA)};
            }
        } else if (TP_IN_API == kStreamAPI) {
            for (int i = 0; i < TP_NUM_INPUTS; i++) {
                connect<stream>(in[i], m_kernel.in[i]);
            }
        }
        if (TP_OUT_API == kWindowAPI) {
            for (int i = 0; i < TP_NUM_OUTPUT_CLONES; i++) {
                //                connect<window<TP_WINDOW_VSIZE*sizeof(TT_DATA)+TP_HEADER_BYTES>>(m_kernel.out[i],
                //                out[i]);
                connect(m_kernel.out[i], out[i]);
                constexpr unsigned int outWindowInterleaved =
                    (TP_IN_API == kWindowAPI) ? TP_WINDOW_VSIZE * TP_NUM_INPUTS : TP_WINDOW_VSIZE;
                dimensions(m_kernel.out[i]) = {outWindowInterleaved + TP_HEADER_BYTES / sizeof(TT_DATA)};
            }
        } else if (TP_OUT_API == kStreamAPI) {
            for (int i = 0; i < TP_NUM_OUTPUT_CLONES; i++) {
                connect<stream>(m_kernel.out[i], out[i]);
            }
        }
    }; // constructor
};
}
}
}
}
} // namespace braces

#endif //_DSPLIB_WIDGET_API_CAST_GRAPH_HPP_
