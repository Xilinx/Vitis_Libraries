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
#ifndef _DSPLIB_CUMSUM_GRAPH_HPP_
#define _DSPLIB_CUMSUM_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Cumsum function library element.
*/
/**
 * @file cumsum_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "cumsum.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace cumsum {
using namespace adf;

/**
 * @defgroup cumsum_graph Cumulative Sum functions
 *
 * Cumulative Sum
**/

//--------------------------------------------------------------------------------------------------
// cumsum_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup cumsum_graph
 * @brief cumsum is utility to apply a windowing (scaling) function such as Hamming to a
 *        frame of data samples.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat, bfloat16, cbfloat16.
 *         Type choice is restricted by aie variant.
 * @tparam TT_OUT_DATA describes the type of individual data samples output from the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, cint16, int32, cint32, float, cfloat, bfloat16, cbfloat16.
 * @tparam TP_DIM_A describes the number of samples in a vector or the first dimension in a 2D input.
 * @tparam TP_DIM_B describes the second dimension size. Set to 1 for vector operation.
 * @tparam TP_NUM_SAMPLES described the number of vectors or matrices to be operated on per call to
 *         the function (an iteration).
 * @tparam TP_MODE described the dimension to perform cumsum along. 0 accumulates along the first
 *         dimension. 1 accumulates along the second dimension. 2 is as for 0, but without shift
 *         and limited to the range of TT_OUT_DATA.
 * @tparam TP_SHIFT described the number of bits to downshift after the scaling by the window
 *         value. \n
 *         For example, for a TT_COEFF of int16 and a window value of 16384 meaning 1.000,
 *         a TP_SHIFT value of 14 is appropriate.
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
 *         Note: Rounding modes ``rnd_sym_floor`` and ``rnd_sym_ceil`` are only supported on AIE-ML and AIE-MLv2 device.
 *\n
 * @tparam TP_SAT describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *         in the range [- ( 2^(n-1) ) : +2^(n-1) - 1 ].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/
template <typename TT_DATA,
          typename TT_OUT_DATA,
          unsigned int TP_DIM_A = 16,
          unsigned int TP_DIM_B = 1,
          unsigned int TP_NUM_FRAMES = 1,
          unsigned int TP_MODE = 0,
          unsigned int TP_SHIFT = 0,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
/**
 **/
class cumsum_graph : public graph {
   public:
    static constexpr int TP_SSR = 1;

    // Defensive configuration legality checks
    //#if __SUPPORTS_CFLOAT__ == 1
    static_assert((std::is_same<TT_DATA, int16>::value) || (std::is_same<TT_DATA, cint16>::value) ||
                      (std::is_same<TT_DATA, int32>::value) || (std::is_same<TT_DATA, cint32>::value) ||
                      (std::is_same<TT_DATA, float>::value) || (std::is_same<TT_DATA, cfloat>::value) ||
                      (std::is_same<TT_DATA, bfloat16>::value) || (std::is_same<TT_DATA, cbfloat16>::value),
                  "ERROR: TT_DATA is not supported");
    //#else
    // AIE variants that don't support cfloat should flag that.
    //  static_assert((std::is_same<TT_DATA, int16>::value) ||
    //             (std::is_same<TT_DATA, cint16>::value) ||
    //             (std::is_same<TT_DATA, int32>::value) ||
    //             (std::is_same<TT_DATA, cint32>::value),
    //              "ERROR: TT_DATA is not supported");
    //#endif //__SUPPORTS_CFLOAT__ == 0
    static_assert(TP_SHIFT >= 0 && TP_SHIFT < 61, "ERROR: TP_SHIFT is out of the supported range (0 to 61)");
    static_assert(TP_SHIFT == 0 ||
                      !(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value ||
                        std::is_same<TT_DATA, bfloat16>::value || std::is_same<TT_DATA, cbfloat16>::value),
                  "ERROR: TP_SHIFT must be 0 for any float types");
    static_assert(TP_SHIFT < 20 ||
                      !(std::is_same<TT_OUT_DATA, int16>::value || std::is_same<TT_OUT_DATA, cint16>::value),
                  "ERROR: TP_SHIFT is inappropriate for TT_OUT_DATA=int16 or cint16");

    static constexpr int kmemAccWidth = TP_MODE == 2 ? __MAX_READ_WRITE__ * 2 : __MAX_READ_WRITE__; // TODO 512 for
                                                                                                    // MLv2?
    static constexpr int kdimInGran = kmemAccWidth / 8 / sizeof(TT_DATA);
    static constexpr int kdimInCeil = CEIL(TP_DIM_A, kdimInGran);
    static constexpr int kKernelWindowVsize = kdimInCeil * TP_DIM_B * TP_NUM_FRAMES; // in samples
    static_assert(kKernelWindowVsize <= __DATA_MEM_BYTES__ / sizeof(TT_OUT_DATA),
                  "ERROR: The dimensions requested of TT_OUT_DATA exceed the memory capacity of a tile."); // phrased to
                                                                                                           // allow
                                                                                                           // single
                                                                                                           // buffer
                                                                                                           // sizes

    /**
     * The input data to the function.
     **/
    port_array<input, TP_SSR> in;
    /**
     * An API of TT_DATA type.
     **/
    port_array<output, TP_SSR> out;

    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     **/
    kernel m_kernels[TP_SSR];

    /**
     * Access function to get pointer to kernel (or first kernel in a chained configuration).
     **/

    kernel* getKernels() { return m_kernels; };

    /**
     * @brief This is the constructor function for the fftWindow graph.
     **/
    cumsum_graph() {
        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<
                cumsum<TT_DATA, TT_OUT_DATA, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_MODE, TP_SHIFT, TP_RND, TP_SAT> >();
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) =
                0.9; // Nominal figure. The real figure requires knowledge of the sample rate.
            // Source files
            source(m_kernels[i]) = "cumsum.cpp";
            stack_size(m_kernels[i]) = 2048;

            // make connections
            connect(in[i], m_kernels[i].in[0]);
            dimensions(m_kernels[i].in[0]) = {kKernelWindowVsize};
            connect(m_kernels[i].out[0], out[i]);
            dimensions(m_kernels[i].out[0]) = {kKernelWindowVsize};
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_CUMSUM_GRAPH_HPP_
