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
#ifndef _DSPLIB_FFT_IFFT_DIT_2D_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_2D_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "fft_ifft_dit_1ch_graph.hpp"
#include "fft_dit_2ch_real_graph.hpp"
#include "fft_window_graph.hpp"
#include "fft_window_fns.hpp"

using namespace adf;
using namespace xf::dsp::aie::fft::dit_1ch;
using namespace xf::dsp::aie::fft_dit_2ch_real;

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace two_d {

/**
 * @defgroup fft_ifft_2dgraphs 2D FFT Graph
 *
 */

//--------------------------------------------------------------------------------------------------
// fft_2d template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_ifft_2dgraphs
 *
 *
 * These are the templates to configure the 2D FFT class.
 * @tparam TT_DATA_D1 describes the type of individual data samples input to and
 *         output from the first transform function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_DATA_D2 describes the type of individual data samples input to and
 *         output from the second transform function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
 * @tparam TT_TWIDDLE describes the type of twiddle factors of the transform. \n
 *         It must be one of the following: cint16, cint32, cfloat
 * @tparam TP_POINT_SIZE_D1 is an unsigned integer which describes the number of samples in
 *         the first transform. \n This must be 2^N where N is an integer in the range
 *         4 to 16 inclusive.
 * @tparam TP_POINT_SIZE_D2 is an unsigned integer which describes the number of samples in
 *         the second transform. \n This must be 2^N where N is an integer in the range
 *         4 to 16 inclusive.
 * @tparam TP_FFT_NIFFT selects whether the transform to perform is an FFT (1) or IFFT (0).
 * @tparam TP_SHIFT selects the power of 2 to scale the result by prior to output.
 * @tparam TP_CASC_LEN selects the number of kernels the FFT will be divided over in series
 *         to improve throughput
 * @tparam TP_WINDOW_VSIZE_D1 is an unsigned integer which describes the number of samples to be processed in each call
 *to the\n
 *         to the function. By default, TP_WINDOW_SIZE_D1 is set to match TP_POINT_SIZE_D1. \n
 *         TP_WINDOW_SIZE_D1 may be set to be an integer multiple of the TP_POINT_SIZE_D1, in which case
 *         multiple FFT iterations will be performed on a given input window, resulting in multiple
 *         iterations of output samples, reducing the numer of times the kernel needs to be triggered to
 *         process a given number of input data samples. \n
 *         As a result, the overheads inferred during kernel triggering are reduced and overall performance
 *         is increased.
 * @tparam TP_WINDOW_VSIZE_D2 is an unsigned integer which describes the number of samples to be processed in each call
 *to the\n
 *         to the function. By default, TP_WINDOW_SIZE_D2 is set to match TP_POINT_SIZE_D2. \n
 *         TP_WINDOW_SIZE_D2 may be set to be an integer multiple of the TP_POINT_SIZE_D2
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces.
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
template <typename TT_DATA_D1,
          typename TT_DATA_D2,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE_D1,
          unsigned int TP_POINT_SIZE_D2,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_WINDOW_VSIZE_D1 = TP_POINT_SIZE_D1,
          unsigned int TP_WINDOW_VSIZE_D2 = TP_POINT_SIZE_D2,
          unsigned int TP_API = 0,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class fft_ifft_2d_graph : public graph {
   private:
    static constexpr unsigned int kIntParPow = 0;
    static constexpr unsigned int kIntDynPtSize = 0;
    static constexpr unsigned int kIntUseWidgets = 0;
    static constexpr unsigned int kIsRealDataD1 =
        (std::is_same<TT_DATA_D1, int16>::value || std::is_same<TT_DATA_D1, bfloat16>::value) ? 1 : 0;
    static constexpr unsigned int kPtSizeRealSymOut = TP_POINT_SIZE_D1 / 2;
    static constexpr unsigned int kD1SizeMemTile = kIsRealDataD1 ? kPtSizeRealSymOut : TP_POINT_SIZE_D1;

   public:
    /**
     * The input data to the function. This input is a window API of
     * samples of TT_DATA_D1 type. If the TT_DATA_D1 is a real type then the samples from two channels
     * are expected to be interleaved before giving as input to the 2d fft graph.
     * The number of samples in the window is described by TP_WINDOW_VSIZE_D1.
     * A total of TP_POINT_SIZE_D1 * TP_POINT_SIZE_D2 samples are needed for one complete 2D FFT operation.
     **/
    port_array<input, 1> in;

    /**
     * The output data from the function. This output is a window API of
     * samples of TT_DATA_D2 type. The number of samples in the window is
     * described by TP_WINDOW_VSIZE_D2. A total of TP_POINT_SIZE_D1 * TP_POINT_SIZE_D2 samples are produced when
     *TT_DATA_D1 is complex.
     * When TT_DATA_D1 is of real type, a total of (TP_POINT_SIZE_D1 * TP_POINT_SIZE_D2)/2 samples are produced at the
     *output.
     * This is because only half the output samples of the first FFT are given as input to the second FFT. Since the
     *output samples of a
     * real-only FFT are symmetric, this optimisation reduces the memory footprint of the design.
     **/
    port_array<output, 1> out;

    /**
     * Front FFT graph that computes the first set of FFTs.
     **/
    typedef typename std::conditional_t<kIsRealDataD1,
                                        fft_dit_2ch_real_graph<TT_DATA_D2,
                                                               TT_TWIDDLE,
                                                               TP_POINT_SIZE_D1,
                                                               TP_FFT_NIFFT,
                                                               TP_SHIFT,
                                                               TP_CASC_LEN,
                                                               TP_WINDOW_VSIZE_D1,
                                                               TP_API,
                                                               TP_RND,
                                                               TP_SAT,
                                                               TP_TWIDDLE_MODE>,
                                        fft_ifft_dit_1ch_graph<TT_DATA_D2,
                                                               TT_TWIDDLE,
                                                               TP_POINT_SIZE_D1,
                                                               TP_FFT_NIFFT,
                                                               TP_SHIFT,
                                                               TP_CASC_LEN,
                                                               kIntDynPtSize,
                                                               TP_WINDOW_VSIZE_D1,
                                                               TP_API,
                                                               kIntParPow,
                                                               kIntUseWidgets,
                                                               TP_RND,
                                                               TP_SAT,
                                                               TP_TWIDDLE_MODE> >
        frontGraphType;

    frontGraphType frontFFTGraph[1];

    /**
     * Back FFT graph that computes the second set of FFTs.
     **/

    fft_ifft_dit_1ch_graph<TT_DATA_D2,
                           TT_TWIDDLE,
                           TP_POINT_SIZE_D2,
                           TP_FFT_NIFFT,
                           TP_SHIFT,
                           TP_CASC_LEN,
                           kIntDynPtSize,
                           TP_WINDOW_VSIZE_D2,
                           TP_API,
                           kIntParPow,
                           kIntUseWidgets,
                           TP_RND,
                           TP_SAT,
                           TP_TWIDDLE_MODE>
        backFFTGraph[1];
    /**
    * @endcond
    */
    /**
     * Memtile object that performs the transpose between the first set of FFTs and the second set of FFTs.
     **/
    adf::shared_buffer<TT_DATA_D2> memTile1;

    /**
     * @brief This is the constructor function for the 2D FFT graph.
     * No arguments required
     **/
    fft_ifft_2d_graph() {
        memTile1 = adf::shared_buffer<TT_DATA_D2>::create({kD1SizeMemTile, TP_POINT_SIZE_D2}, 1, 1);
        num_buffers(memTile1) = 2;
        write_access(memTile1.in[0]) = tiling({.buffer_dimension = {kD1SizeMemTile, TP_POINT_SIZE_D2},
                                               .tiling_dimension = {kD1SizeMemTile, TP_POINT_SIZE_D2},
                                               .offset = {0, 0}});

        read_access(memTile1.out[0]) =
            adf::tiling({.buffer_dimension = {kD1SizeMemTile, TP_POINT_SIZE_D2},
                         .tiling_dimension = {1, 1},
                         .offset = {0, 0},
                         .tile_traversal = {{.dimension = 1, .stride = 1, .wrap = TP_POINT_SIZE_D2},
                                            {.dimension = 0, .stride = 1, .wrap = kD1SizeMemTile}}});
        for (int i = 0; i < 1; i++) {
            connect<>(in[i], frontFFTGraph[i].in[0]);
            connect<>(frontFFTGraph[i].out[0], memTile1.in[0]);

            connect<>(memTile1.out[0], backFFTGraph[i].in[0]);
            connect<>(backFFTGraph[i].out[0], out[i]);
        }
    };
};

} // namespace 2d
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_FFT_IFFT_DIT_2D_GRAPH_HPP_
