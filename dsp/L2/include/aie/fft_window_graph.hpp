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
#include <adf/arch/aie_arch_properties.hpp>
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
 * @tparam TT_DATA describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         cint16, cint32, cfloat.
 * @tparam TT_COEFF describes the type of weights in the FFT window. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, float.
 *         TT_DATA and TT_COEFF must be both integer types or both float types.
 * @tparam TP_POINT_SIZE describes the number of samples in the frame to be windowed.
 * @tparam TP_WINDOW_VSIZE describes the number of samples to be processed in each
 *         call to this function. It must be an integer multiple of TP_POINT_SIZE.
 * @tparam TP_SHIFT described the number of bits to downshift after the scaling by the window
 *         value. \n
 *         For example, for a TT_COEFF of int16 and a window value of 16384 meaning 1.000,
 *         a TP_SHIFT value of 14 is appropriate.
 * @tparam TP_API described whether to use streams (1) or windows (0).
 * @tparam TP_SSR describes the number of kernels to use in parallel to perform the windowing
 *         function.
 * @tparam TP_DYN_PT_SIZE describes whether to support run-time selectable point size for
 *         the frames of data within the AIE window to be processed.
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
 *         \n
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML device. \n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_DYN_PT_SIZE = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
/**
 **/
class fft_window_graph : public graph {
   public:
    static constexpr int kMaxSSR = 16;
    static constexpr int kHeaderBytes = TP_DYN_PT_SIZE > 0 ? 32 : 0;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait

// Defensive configuration legality checks
#if __SUPPORTS_CFLOAT__ == 1
    static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                      (std::is_same<TT_DATA, cfloat>::value),
                  "ERROR: TT_DATA is not supported");
#else
    // AIE variants that don't support cfloat should flag that.
    static_assert((std::is_same<TT_DATA, cint16>::value) || (std::is_same<TT_DATA, cint32>::value),
                  "ERROR: TT_DATA is not supported");
#endif //__SUPPORTS_CFLOAT__ == 0
    static_assert((std::is_same<TT_DATA, cfloat>::value && std::is_same<TT_COEFF, float>::value) ||
                      (!std::is_same<TT_DATA, cfloat>::value && !std::is_same<TT_COEFF, float>::value),
                  "ERROR: TT_DATA and TT_COEFF are not a supported combination");
    static_assert(TP_WINDOW_VSIZE % TP_POINT_SIZE == 0,
                  "ERROR: TP_WINDOW_VSIZE must be an integer multiple of TP_POINT_SIZE");
    static_assert(TP_POINT_SIZE >= kPointSizeMin, "ERROR: TP_POINT_SIZE must be at least 16");
    static_assert(TP_POINT_SIZE <= kPointSizeMax, "ERROR: TP_POINT_SIZE must be at no more than 65536");
    static_assert(TP_SHIFT >= 0 && TP_SHIFT < 61, "ERROR: TP_SHIFT is out of the supported range (0 to 61)");
    static_assert(TP_SHIFT == 0 || !std::is_same<TT_COEFF, float>::value,
                  "ERROR: TP_SHIFT must be 0 for float operation");
    static_assert(TP_SHIFT < 20 || !std::is_same<TT_COEFF, int16>::value,
                  "ERROR: TP_SHIFT is inappropriate for TT_COEFF=int16");
    static_assert(TP_DYN_PT_SIZE == 0 || TP_DYN_PT_SIZE == 1,
                  "ERROR: TP_DYN_PT_SIZE is not a supported value (0 or 1)");
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API is not a supported value (0 or 1)");
    static_assert(TP_SSR >= 0 && TP_SSR <= kMaxSSR, "ERROR: TP_SSR is not in the supported range of 1 to 16");

    static constexpr int kAPIFactor =
        TP_API == 0 ? 1 : kStreamsPerTile; // 2 ports for stream on AIE1, 1 for window or streams on AIE2.
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
                                                 TP_SSR, TP_DYN_PT_SIZE, TP_RND, TP_SAT> >(kernel_weights[i]);
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "fft_window.cpp";
            stack_size(m_kernels[i]) = sizeof(TT_COEFF) * kKernelPtSize * (1 + TP_DYN_PT_SIZE) + 2048;

            // make connections
            if (TP_API == 0) {
                connect(in[i], m_kernels[i].in[0]);
                dimensions(m_kernels[i].in[0]) = {kKernelWindowVsize + kHeaderBytes / sizeof(TT_DATA)};
                connect(m_kernels[i].out[0], out[i]);
                dimensions(m_kernels[i].out[0]) = {kKernelWindowVsize + kHeaderBytes / sizeof(TT_DATA)};
            } else {
                for (int k = 0; k < kStreamsPerTile; k++) {
                    connect<stream>(in[i * kStreamsPerTile + k], m_kernels[i].in[k]);
                    connect<stream>(m_kernels[i].out[k], out[i * kStreamsPerTile + k]);
                }
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
