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
#ifndef _DSPLIB_FFT_WINDOW_GRAPH_HPP_
#define _DSPLIB_FFT_WINDOW_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the FFT window function library element.
*/
/**
 * @file fft_window_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include "graph_utils.hpp"
#include <tuple>

#include "fft_window.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace windowfn {
using namespace adf;

//--------------------------------------------------------------------------------------------------
// fft_window_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup fft_window
 * @brief fft_window is utility to apply a windowing (scaling) function such as Hamming to a
 *        frame of data samples.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following: \n
 *         cint16, cint32, cfloat.
 * @tparam TT_COEFF describes the type of weights in the FFT window.
 *         This is a typename and must be one of the following: \n
 *         int16, int32, float.
 *         TT_DATA and TT_COEFF must be both integer types or both float types.
 * @tparam TP_POINT_SIZE describes the number of samples in the frame to be windowed.
 * @tparam TP_WINDOW_VSIZE describes the number of samples to be processed in each
 *         call to this function. It must be an integer multiple of TP_POINT_SIZE.
 * @tparam TP_SHIFT described the number of bits to downshift after the scaling by the window
 *         value. e.g. for a TT_COEFF of int16 and a window value of 16384 meaning 1.000,
 *         a TP_SHIFT value of 14 is appropriate.
 * @tparam TP_API described whether to use streams (1) or windows (0).
 * @tparam TP_SSR describes the number of kernels to use in parallel to perform the windowing
 *         function.
 * @tparam TP_DYN_PT_SIZE describes whether to support run-time selectable point size for
 *         the frames of data within the AIE window to be processed.
 **/
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE = 0>
/**
 **/
class fft_window_graph : public graph {
   public:
    static constexpr int kMaxSSR = 16;
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;

    // Defensive configuration legality checks
    static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                      (std::is_same<TT_DATA, cfloat>::value),
                  "ERROR: TT_DATA is not supported");
    static_assert((std::is_same<TT_COEFF, int16>::value) || (std::is_same<TT_COEFF, int32>::value) ||
                      (std::is_same<TT_COEFF, float>::value),
                  "ERROR: TT_COEFF is not supported");
    static_assert((std::is_same<TT_DATA, cfloat>::value && std::is_same<TT_COEFF, float>::value) ||
                      (!std::is_same<TT_DATA, cfloat>::value && !std::is_same<TT_COEFF, float>::value),
                  "ERROR: TT_DATA and TT_COEFF are not a supported combination");
    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0,
                  "ERROR: TP_WINDOW_VSIZE must be an integer multiple of TP_POINT_SIZE");
    static_assert(TP_POINT_SIZE >= kPointSizeMin, "ERROR: TP_POINT_SIZE must be at least 16");
    static_assert(TP_POINT_SIZE <= kPointSizeMax, "ERROR: TP_POINT_SIZE must be at no more than 65536");
    // static_assert(TP_POINT_SIZE == 65536 ||
    //              TP_POINT_SIZE == 32768 ||
    //              TP_POINT_SIZE == 16384 ||
    //              TP_POINT_SIZE == 8192 ||
    //              TP_POINT_SIZE == 4096 ||
    //              TP_POINT_SIZE == 2048 ||
    //              TP_POINT_SIZE == 1024 ||
    //              TP_POINT_SIZE == 512 ||
    //              TP_POINT_SIZE == 256 ||
    //              TP_POINT_SIZE == 128 ||
    //              TP_POINT_SIZE == 64 ||
    //              TP_POINT_SIZE == 32 ||
    //              TP_POINT_SIZE == 16 , "ERROR: TP_POINT_SIZE is not a supported value (16, 32, 64,... 65536.");
    static_assert(TP_SHIFT >= 0 && TP_SHIFT < 61, "ERROR: TP_SHIFT is out of the supported range (0 to 61)");
    static_assert(TP_SHIFT == 0 || !std::is_same<TT_COEFF, float>::value,
                  "ERROR: TP_SHIFT must be 0 for float operation");
    static_assert(TP_SHIFT < 20 || !std::is_same<TT_COEFF, int16>::value,
                  "ERROR: TP_SHIFT is inappropriate for TT_COEFF=int16");
    static_assert(TP_DYN_PT_SIZE == 0 || TP_DYN_PT_SIZE == 1,
                  "ERROR: TP_DYN_PT_SIZE is not a supported value (0 or 1)");
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API is not a supported value (0 or 1)");
    static_assert(TP_SSR >= 0 && TP_SSR <= kMaxSSR, "ERROR: TP_SSR is not in the supported range of 1 to 16");

    static constexpr int kAPIFactor = TP_API == 0 ? 1 : 2; // 2 ports for stream, 1 for window.
    static constexpr int kKernelPtSize = TP_POINT_SIZE / TP_SSR;
    static constexpr int kKernelWindowVsize = TP_WINDOW_VSIZE / TP_SSR;
    static_assert(kKernelPtSize <= 4096, "ERROR: TP_POINT_SIZE/TP_SSR must be at no more than 4096");
    static_assert(kKernelPtSize >= 16, "ERROR: TP_POINT_SIZE/TP_SSR must be at least 16");
    static_assert(TP_DYN_PT_SIZE == 0 || kKernelPtSize >= 32, "ERROR: TP_DYN_PT_SIZE is not valid for this point size");

    /**
     * The input data to the function.
     **/
    port_array<input, kAPIFactor * TP_SSR> in;
    /**
     * An API of TT_DATA type.
     **/
    port_array<output, kAPIFactor * TP_SSR> out;

    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     **/
    kernel m_kernels[TP_SSR];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/

    kernel* getKernels() { return m_kernels; };
    std::array<std::array<TT_COEFF, kKernelPtSize*(1 + TP_DYN_PT_SIZE)>, TP_SSR> kernel_weights;

    /**
     * @brief This is the constructor function for the fftWindow graph.
     **/
    fft_window_graph(const std::array<TT_COEFF, TP_POINT_SIZE*(1 + TP_DYN_PT_SIZE)>& weights) {
        for (int i = 0; i < kKernelPtSize * (1 + TP_DYN_PT_SIZE); i++) {
            for (int k = 0; k < TP_SSR; k++) {
                kernel_weights[k][i] = weights[i * TP_SSR + k];
            }
        }
        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] =
                kernel::create_object<fft_window<TT_DATA, TT_COEFF, kKernelPtSize, kKernelWindowVsize, TP_SHIFT, TP_API,
                                                 TP_SSR, TP_DYN_PT_SIZE> >(kernel_weights[i]);
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "fft_window.cpp";

            // make connections
            if (TP_API == 0) {
                connect<window<kKernelWindowVsize * sizeof(TT_DATA) + kHeaderBytes> >(in[i], m_kernels[i].in[0]);
                connect<window<kKernelWindowVsize * sizeof(TT_DATA) + kHeaderBytes> >(m_kernels[i].out[0], out[i]);
            } else {
                connect<stream>(in[i * 2], m_kernels[i].in[0]);
                connect<stream>(in[i * 2 + 1], m_kernels[i].in[1]);
                connect<stream>(m_kernels[i].out[0], out[i * 2]);
                connect<stream>(m_kernels[i].out[1], out[i * 2 + 1]);
            }
        }
    }; // constructor
};
}
}
}
}
} // namespace braces

#endif //_DSPLIB_FFT_WINDOW_GRAPH_HPP_
