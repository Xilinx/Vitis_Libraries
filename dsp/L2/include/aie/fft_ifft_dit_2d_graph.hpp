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
#ifndef _DSPLIB_FFT_IFFT_DIT_1CH_2D_GRAPH_HPP_
#define _DSPLIB_FFT_IFFT_DIT_1CH_2D_GRAPH_HPP_

#include <adf.h>
#include <vector>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "fft_ifft_dit_1ch_graph.hpp"
#include "fft_window_graph.hpp"
//#include "fft_ifft_dit_1ch.hpp"
#include "twiddle_rotator.hpp"
#include "fft_window_fns.hpp"

using namespace adf;
using namespace xf::dsp::aie::fft::dit_1ch;
using namespace xf::dsp::aie::fft::windowfn;

// Note on heap. The FFT twiddles and internal scratch memories can be handled by the graph scope mechanism which allows
// the use of adjacent tile's memory, hence alleviating the 32kB memory limit.
// Without the graph scope mechanism, heap can be set explicitly, or set automatically using aiecompiler switch
// --xlopt=1
// The following #defines allow control over which mechanisms is used, though this must be done in conjunction with the
// Makefile
#ifndef __X86SIM__
#define USE_GRAPH_SCOPE
#endif
//#define USE_EXPLICIT_HEAP

#ifdef USE_GRAPH_SCOPE
#include "fft_bufs.h" //Extern declarations of all twiddle factor stores and rank-temporary sample stores.
#endif                // USE_GRAPH_SCOPE

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace ifft_2d_aie_pl {

template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int dim,
          unsigned int TP_SSR,
          unsigned int TP_POINT_SIZE_D1,
          unsigned int TP_POINT_SIZE_D2,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_DYN_PT_SIZE,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class create_par_kernel_aie_pl {
   public:
    static void create(kernel (&m_fftTwRotKernels)[TP_SSR]) {
        printf("dim = %d\n", dim);
        m_fftTwRotKernels[dim] = kernel::create_object<
            twiddleRotator<TT_DATA, TT_TWIDDLE, TP_POINT_SIZE_D1, TP_POINT_SIZE_D2, TP_SSR, TP_FFT_NIFFT, dim> >();
        runtime<ratio>(m_fftTwRotKernels[dim]) = 0.2;
        // Source files
        source(m_fftTwRotKernels[dim]) = "twiddle_rotator.cpp";
        headers(m_fftTwRotKernels[dim]) = {"twiddle_rotator.hpp"};
        if
            constexpr(dim != 0) {
                create_par_kernel_aie_pl<TT_DATA, TT_TWIDDLE, dim - 1, TP_SSR, TP_POINT_SIZE_D1, TP_POINT_SIZE_D2,
                                         TP_FFT_NIFFT, TP_DYN_PT_SIZE, TP_RND, TP_SAT>::create(m_fftTwRotKernels);
            }
    }
};

/**
 * @defgroup fft_graphs FFT Graph
 *
 * The FFT graph is offered as a template class that is available with 2 template specializations,
 * that offer varied features and interfaces:
 * - window interface (TP_API == 0) or
 * - stream interface (TP_API == 1).
 *
 */

/**
 *
 * @brief Defines Window/IO Buffer API
 *
 * @ingroup fft_graphs
 */
static constexpr unsigned int kWindowAPI = 0;

/**
 *
 * @brief Defines Stream Buffer API
 *
 * @ingroup fft_graphs
 */
static constexpr unsigned int kStreamAPI = 1;

/**
  * @endcond
  */

//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch is a single-channel, decimation-in-time, fixed point size FFT.
 *
 * This class definition is only used with stream interfaces (TP_API == 1).
 * Stream interface FFT graph is offered with a dual input stream configuration,
 * which interleaves data samples between the streams.
 * Stream interface FFT implementation is capable of supporting parallel computation (TP_SSR > 0).
 * Dynamic point size, with a header embedded in the data stream.
 *
 * These are the templates to configure the single-channel decimation-in-time class.
 * @tparam TT_DATA describes the type of individual data samples input to and
 *         output from the transform function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat.
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
 * @tparam TP_DYN_PT_SIZE selects whether (1) or not (0) to use run-time point size determination. \n
 *         When set, each window of data must be preceeded, in the window, by a 256 bit header. \n
 *         This header is 8 samples when TT_DATA is cint16 and 4 samples otherwise.\n
 *         The real part of the first sample indicates the forward (1) or inverse (0) transform. \n
 *         The real part of the second sample indicates the Radix2 power of the point size. \n
 *         e.g. for a 512 point size, this field would hold 9, as 2^9 = 512.
 *         The second least significant byte  8 bits of this field describe the Radix 2 power of the following \n
 *         frame. e.g. for a 512 point size, this field would hold 9, as 2^9 = 512. \n
 *         Any value below 4 or greater than log2(TP_POINT_SIZE) is considered illegal. \n
 *         The output window will also be preceeded by a 256 bit vector which is a copy of the input \n
 *         vector, but for the real part of the top sample, which is 0 to indicate a legal frame or 1 to \n
 *         indicate an illegal frame. \n
 *         When TP_SSR is greater than 0, the header must be applied before each window of data \n
 *         for every port of the design and will appears before each window of data on the output ports. \n
 *         Note that the minimum point size of 16 applies to each lane when in parallel mode, so a configuration \n
 *         of point size 256 with TP_SSR = 2 will have 4 lanes each with a minimum of 16 so the minimum \n
 *         legal point size here is 64.
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
 * @tparam TP_API is an unsigned integer to select window (0) or stream (1) interfaces.
 *         When stream I/O is selected, one sample is taken from, or output to, a stream and the next sample
 *         from or two the next stream. Two streams minimum are used. In this example, even samples are
 *         read from input stream[0] and odd samples from input stream[1].
 * @tparam TP_SSR is an unsigned integer to describe N where 2^N is the numbers of subframe processors
 *         to use, so as to achieve higher throughput. \n
 *         The default is 0. With TP_SSR set to 2, 4 subframe processors will be used, each of which
 *         takes 2 streams in for a total of 8 streams input and output. Sample[p] must be written to stream[p modulus
 *q]
 *         where q is the number of streams.
 * @tparam TP_USE_WIDGETS is an unsigned integer to control the use of widgets for configurations which either use
 *TP_API=1 (streaming IO)
 *         or TP_SSR>0 which uses streams internally, even if not externally. The default is not to use widgets
 *         but to have the stream to window conversion performed as part of the FFT kernel or R2combiner kernel.
 *         Using widget kernels allows this conversion to be placed in a separate tile and so boost performance
 *         at the expense of more tiles being used.
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
//--------------------------------------------------------------------------------------------------
// fft_dit_1ch template specialization
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_graphs
 *
 * @brief fft_dit_1ch template specialization for single (monolithic) FFT, window API
 *
 * Window interface (kWindowAPI = 0) FFT graph is offered with a single input windowed, ping-pong buffer.
 * Window interface FFT implementation does not support parallel computation (TP_SSR = 0 only).
 * However, dynamic point size is available (TP_DYN_PT_SIZE = 1), which allows a window buffer size to be an integer
 * multiple of the FFT's point size ( TP_WINDOW_VSIZE = N * TP_POINT_SIZE).
 * Feature offers performance improvements, particularly with small FFT graphs.
**/
template <typename TT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT = 1,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_CASC_LEN = 1,
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_WINDOW_VSIZE = TP_POINT_SIZE,
          unsigned int TP_API = 0,
          unsigned int TP_SSR = 0,
          unsigned int TP_USE_WIDGETS = 0,
          unsigned int TP_RND = 4,
          unsigned int TP_SAT = 1,
          unsigned int TP_TWIDDLE_MODE = 0>
class fft_ifft_dit_2d_graph : public graph {
   public:
    // static_assert(TP_RND != rnd_sym_floor && TP_RND != rnd_sym_ceil && TP_RND != rnd_floor && TP_RND != rnd_ceil,
    // "Error: FFT does not support TP_RND set to floor, ceil, symmetric floor, and symmetric ceil. Please set TP_RND to
    // any of the other rounding modes. The mapping of integers to rounding modes is device dependent. Please refer to
    // documentation for more information.");
    /**
     * The input data to the function. This input is a window API of
     * samples of TT_DATA type. The number of samples in the window is
     * described by TP_POINT_SIZE.
     **/
    port_array<input, TP_SSR> front_i;
    port_array<input, TP_SSR> back_i;

    /**
     * A window API of TP_POINT_SIZE samples of TT_DATA type.
     **/
    port_array<output, TP_SSR> front_o;
    port_array<output, TP_SSR> back_o;

    // fft window kernel
    kernel m_fftTwRotKernels[TP_SSR];
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;

    /**
     * Monolithic FFT block.
     **/
    static constexpr unsigned int POINT_SIZE_D2 = fnPtSizeD1<TP_POINT_SIZE>(); // 32
    static constexpr unsigned int POINT_SIZE_D1 = POINT_SIZE / POINT_SIZE_D2;  // 16
    static constexpr unsigned int firstFFTShift = TP_SHIFT / 2;
    static constexpr unsigned int secondFFTShift = TP_SHIFT - TP_SHIFT / 2;
    static constexpr unsigned int PARALLEL_POWER = 0;
    fft_ifft_dit_1ch_graph<TT_DATA,
                           TT_TWIDDLE,
                           POINT_SIZE_D1,
                           TP_FFT_NIFFT,
                           firstFFTShift,
                           TP_CASC_LEN,
                           TP_DYN_PT_SIZE,
                           POINT_SIZE_D1,
                           TP_API,
                           PARALLEL_POWER,
                           TP_USE_WIDGETS,
                           TP_RND,
                           TP_SAT,
                           TP_TWIDDLE_MODE>
        frontFFTGraph[TP_SSR];

    fft_ifft_dit_1ch_graph<TT_DATA,
                           TT_TWIDDLE,
                           POINT_SIZE_D2,
                           TP_FFT_NIFFT,
                           secondFFTShift,
                           TP_CASC_LEN,
                           TP_DYN_PT_SIZE,
                           POINT_SIZE_D2,
                           TP_API,
                           PARALLEL_POWER,
                           TP_USE_WIDGETS,
                           TP_RND,
                           TP_SAT,
                           TP_TWIDDLE_MODE>
        backFFTGraph[TP_SSR];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/

    /**
     * @brief This is the constructor function for the Single channel DIT FFT graph.
     * No arguments required
     **/
    fft_ifft_dit_2d_graph() {
        create_par_kernel_aie_pl<TT_DATA, TT_TWIDDLE, TP_SSR - 1, TP_SSR, POINT_SIZE_D1, POINT_SIZE_D2, TP_FFT_NIFFT,
                                 TP_DYN_PT_SIZE, TP_RND, TP_SAT>::create(m_fftTwRotKernels);
        for (int i = 0; i < TP_SSR; i++) {
            connect<>(front_i[i], frontFFTGraph[i].in[0]);
            connect<>(frontFFTGraph[i].out[0], m_fftTwRotKernels[i].in[0]);
            dimensions(m_fftTwRotKernels[i].in[0]) = {POINT_SIZE_D1 + kHeaderBytes / sizeof(TT_DATA)};

            connect<>(m_fftTwRotKernels[i].out[0], front_o[i]);
            dimensions(m_fftTwRotKernels[i].out[0]) = {POINT_SIZE_D1 + kHeaderBytes / sizeof(TT_DATA)};

            connect<>(back_i[i], backFFTGraph[i].in[0]);
            connect<>(backFFTGraph[i].out[0], back_o[i]);
        }
    };
};

} // namespace dit_1ch
} // namespace fft
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_FFT_IFFT_DIT_1CH_2D_GRAPH_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
