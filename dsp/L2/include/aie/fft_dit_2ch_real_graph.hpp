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
#ifndef _DSPLIB_FFT_DIT_2CH_REAL_GRAPH_HPP_
#define _DSPLIB_FFT_DIT_2CH_REAL_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Widget 2-Channel Real FFT function library element.
*/
/**
 * @file fft_dit_2ch_real_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "fft_ifft_dit_1ch_graph.hpp"
#include "widget_2ch_real_fft_graph.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft_dit_2ch_real {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// fft_dit_2ch_real_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_dit_2ch_real
 * @brief fft_dit_2ch_real takes advantage of the real signal symmetry property by passing two real \n
 * signals through a single complex FFT.
 *
 * This class definition is only used with stream interfaces (TP_API == 1).
 * Stream interface FFT graph is offered with a dual input stream configuration,
 * which interleaves data samples between the streams.
 * Stream interface FFT implementation is capable of supporting parallel computation (TP_PARALLEL_POWER > 0).
 * Dynamic point size, with a header embedded in the data stream.
 *
 * These are the templates to configure the single-channel decimation-in-time class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the transform function. \n
 *         This is a typename and must be one of the following: \n
 *         cint16, cint32, cfloat.
 *         For real-only operation, consider use of the widget_real2complex library element.
 * @tparam TT_TWIDDLE describes the type of twiddle factors of the transform. \n
 *         It must be one of the following: cint16, cint32, cfloat
 *         and must also satisfy the following rules:
 *         - 32 bit types are only supported when TT_DATA is also a 32 bit type,
 *         - TT_TWIDDLE must be an integer type if TT_DATA is an integer type
 *         - TT_TWIDDLE must be cfloat type if TT_DATA is a float type.
 * @tparam TP_POINT_SIZE is an unsigned integer which describes the number of samples in
 *         the transform. \n This must be 2^N where N is an integer in the range
 *         4 to 16 inclusive. \n When TP_DYN_PT_SIZE is set, TP_POINT_SIZE describes the maximum
 *         point size possible.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an FFT (1) or IFFT (0).
 * @tparam TP_SHIFT selects the power of 2 to scale the result by prior to output.
 * @tparam TP_CASC_LEN selects the number of kernels the FFT will be divided over in series
 *         to improve throughput
 * @tparam TP_WINDOW_VSIZE is an unsigned integer which describes the number of samples to be processed in each call \n
 *         to the function. When TP_DYN_PT_SIZE is set to 1 the actual window size will be larger than TP_WINDOW_VSIZE
 *\n
 *         because the header is not included in TP_WINDOW_VSIZE. \n
 *         By default, TP_WINDOW_SIZE is set to match TP_POINT_SIZE. \n
 *         TP_WINDOW_SIZE may be set to be an integer multiple of the TP_POINT_SIZE, in which case
 *         multiple FFT iterations will be performed on a given input window, resulting in multiple
 *         iterations of output samples, reducing the numer of times the kernel needs to be triggered to
 *         process a given number of input data samples. \n
 *         As a result, the overheads inferred during kernel triggering are reduced and overall performance
 *         is increased.
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces. \n
 *         When stream I/O is selected, one sample is taken from, or output to, a stream and the next sample
 *         from or two the next stream. Two streams minimum are used. In this example, even samples are
 *         read from input stream[0] and odd samples from input stream[1].
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
 *
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds an n-bit signed value in the
 *range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 * @tparam TP_TWIDDLE_MODE describes the magnitude of integer twiddles. It has no effect for cfloat. \n
 *         - 0: Max amplitude. Values at 2^15 (for TT_TWIDDLE=cint16) and 2^31 (TT_TWIDDLE=cint32) will saturate and so
 *introduce errors
 *         - 1: 0.5 amplitude. Twiddle values are 1/2 that of mode 0 so as to avoid twiddle saturation. However,
 *twiddles are one bit less precise versus mode 0.
 *
  **/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = 0,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0,
          typename TT_OUT_DATA = TT_DATA>
class fft_dit_2ch_real_graph : public graph {
   public:
    // ! Add in static asserts which enforce TP_FFT_NIFFT=1 and SSR=1.

    /**
     * The input data to the function.
     **/
    port_array<input, 1> in;
    /**
     * The output data.
     **/
    port_array<output, 1> out;

    fft::dit_1ch::fft_ifft_dit_1ch_graph<TT_DATA,
                                         TT_TWIDDLE,
                                         TP_POINT_SIZE,
                                         TP_FFT_NIFFT,
                                         TP_SHIFT,
                                         TP_CASC_LEN,
                                         0,
                                         TP_WINDOW_VSIZE,
                                         TP_API,
                                         0,
                                         0,
                                         TP_RND,
                                         TP_SAT,
                                         TP_TWIDDLE_MODE,
                                         TT_OUT_DATA>
        fftGraph;
    widget_2ch_real_fft::widget_2ch_real_fft_graph<TT_OUT_DATA, TP_POINT_SIZE, TP_WINDOW_VSIZE> widgetDisentangler;

    /**
     * @brief This is the constructor function for the fft_dit_2ch_real graph.
     **/

    fft_dit_2ch_real_graph() {
        // make connections
        connect(in[0], fftGraph.in[0]);
        dimensions(fftGraph.in[0]) = {TP_WINDOW_VSIZE};

        connect(fftGraph.out[0], widgetDisentangler.in[0]);
        dimensions(fftGraph.out[0]) = {TP_WINDOW_VSIZE};
        dimensions(widgetDisentangler.in[0]) = {TP_WINDOW_VSIZE};

        connect(widgetDisentangler.out[0], out[0]);
        dimensions(widgetDisentangler.out[0]) = {TP_WINDOW_VSIZE};
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_FFT_DIT_2CH_REAL_GRAPH_HPP_
