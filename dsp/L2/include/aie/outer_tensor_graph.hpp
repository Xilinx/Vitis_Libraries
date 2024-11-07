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
#ifndef _DSPLIB_OUTER_TENSOR_GRAPH_HPP_
#define _DSPLIB_OUTER_TENSOR_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Outer Tensor function library element.
*/
/**
 * @file outer_tensor_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "outer_tensor.hpp"
#include "outer_tensor_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace outer_tensor {
using namespace adf;
/**
 * @defgroup outer_tensor_graph Outer Tensor function
 *
 * Outer Tensor
**/
//--------------------------------------------------------------------------------------------------
// outer_tensor_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup outer_tensor_graph
 * @brief outer_tensor is every element-wise combination of two vectors.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA_A describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint16, cint32, float, cfloat.
 * @tparam TT_DATA_B describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint16, cint32, float, cfloat.
 * @tparam TP_DIM_A describes the number of samples in the vector A.
 * @tparam TP_DIM_B describes the number of samples in the vector B.
 * @tparam TP_NUM_FRAMES describes the number of outer tensor product operations to perform per call to the kernel.
 * @tparam TP_SHIFT describes the number of bits to downshift.
 * @tparam TP_API describes whether to use streams (1) or windows (0).
 * @tparam TP_SSR describes the number of kernels to use in parallel to perform the windowing function.
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
template <typename TT_DATA_A,
          typename TT_DATA_B,
          unsigned int TP_DIM_A,
          unsigned int TP_DIM_B,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class outer_tensor_graph : public graph {
   public:
    /**
     * @cond NOCOMMENTS
     */
    static constexpr int bufferSizeBytes = 32;
    static constexpr int pingPongSize = 32768 / 2;
    static constexpr int kMaxSSR = 16;
    static constexpr int dimSizeMinA = bufferSizeBytes / sizeof(TT_DATA_A);
    static constexpr int dimSizeMinB = bufferSizeBytes / sizeof(TT_DATA_B);

    // Defensive configuration legality checks
    static_assert(TP_NUM_FRAMES* TP_DIM_A* TP_DIM_B* vectByte<TT_DATA_A, TT_DATA_B>().val_byteOut <= pingPongSize ||
                      TP_API == 1,
                  "ERROR: Output cannot exceed 16384 bytes with windowed interface.");
    static_assert(TP_DIM_A / TP_SSR >= dimSizeMinA,
                  "ERROR: TP_DIM_A * sizeof(TT_DATA_A) / TP_SSR must be >= 32 bytes.");
    static_assert(TP_DIM_B >= dimSizeMinB, "ERROR: TP_DIM_B * sizeof(TT_DATA_B) must be >= 32 bytes.");
    static_assert(TP_DIM_A * sizeof(TT_DATA_A) * TP_NUM_FRAMES <= 16384,
                  "ERROR: Input at port A must be less than 16384 bytes");
    static_assert(TP_DIM_B * sizeof(TT_DATA_B) * TP_NUM_FRAMES <= 16384,
                  "ERROR: Input at port B must be less than 16384 bytes");
    static_assert(TP_NUM_FRAMES > 0, "ERROR: TP_NUM_FRAMES must be > 0.");
    static_assert(fnValidateShiftFloat<TP_SHIFT, TT_DATA_A>(), "ERROR: TP_SHIFT must be 0 for float types.");
    static_assert(fnValidateShiftRange<TP_SHIFT>(), "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API is not a supported value (0 or 1)");
    static_assert(TP_SSR > 0 && TP_SSR <= kMaxSSR, "ERROR: TP_SSR is not in the supported range of 1 to 16");
    static_assert((TP_DIM_A & (TP_DIM_A - 1)) == 0, "ERROR: TP_DIM_A is not a power of 2.");
    static_assert((TP_DIM_B & (TP_DIM_B - 1)) == 0, "ERROR: TP_DIM_B is not a power of 2.");
    static_assert((TP_NUM_FRAMES & (TP_NUM_FRAMES - 1)) == 0, "ERROR: TP_NUM_FRAMES is not a power of 2.");
    static_assert((TP_SSR & (TP_SSR - 1)) == 0, "ERROR: TP_SSR is not a power of 2.");
    static_assert(fnValidateRoundMode<TP_RND>(), "ERROR: Illegal round mode.");
    static_assert(fnValidateSatMode<TP_SAT>(), "ERROR: Illegal saturation mode.");

    using outer_tensor_template =
        outer_tensor<TT_DATA_A, TT_DATA_B, TP_DIM_A, TP_DIM_B, TP_NUM_FRAMES, TP_SHIFT, TP_API, TP_SSR, TP_RND, TP_SAT>;
    static constexpr unsigned int kKernelASize = CEIL(TP_DIM_A, outer_tensor_template::vecSampleNumA) / TP_SSR;
    static constexpr unsigned int kKernelBSize = CEIL(TP_DIM_B, outer_tensor_template::vecSampleNumB);
    static constexpr unsigned int kKernelOutSize =
        TP_DIM_A * CEIL(TP_DIM_B, outer_tensor_template::vecSampleNumOut) / TP_SSR;
    /**
    * @endcond
    */
    /**
        * The input A data to the function.
    **/
    port_array<input, TP_SSR> inA;
    /**
        * The input B data to the function.
    **/
    port_array<input, TP_SSR> inB;
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
     * @brief This is the constructor function for the outer_tensor graph.
     **/
    outer_tensor_graph() {
        printf("Graph constructor...\n");
        printf("kKernelOutSize = %d\n", kKernelOutSize);
        printf("TP_DIM_A = %d, TP_DIM_B= %d\n", TP_DIM_A, TP_DIM_B);
        printf("TP_NUM_FRAMES = %d\n", TP_NUM_FRAMES);
        printf("TP_SHIFT = %d\n", TP_SHIFT);
        printf("TP_API = %d\n", TP_API);
        printf("TP_SSR = %d\n", TP_SSR);
        printf("TP_SAT = %d,\nTP_RND= %d\n", TP_SAT, TP_RND);
        printf("Graph constructor...\n");
        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] =
                kernel::create_object<outer_tensor<TT_DATA_A, TT_DATA_B, kKernelASize, kKernelBSize, TP_NUM_FRAMES,
                                                   TP_SHIFT, TP_API, TP_SSR, TP_RND, TP_SAT> >();
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.9; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "outer_tensor.cpp";
            if (TP_API == 0) {
                stack_size(m_kernels[i]) =
                    outer_tensor_template::vecSampleNumA * outer_tensor_template::vecNumB * 64 / TP_SSR + 1024;
            }

            // make connections
            connect(inA[i], m_kernels[i].in[0]);
            connect(inB[i], m_kernels[i].in[1]);
            dimensions(m_kernels[i].in[0]) = {kKernelASize * TP_NUM_FRAMES};
            dimensions(m_kernels[i].in[1]) = {kKernelBSize * TP_NUM_FRAMES};
            if (TP_API == 0) {
                connect(m_kernels[i].out[0], out[i]);
                dimensions(m_kernels[i].out[0]) = {kKernelOutSize * TP_NUM_FRAMES};
            } else {
                connect<stream>(m_kernels[i].out[0], out[i]);
            }
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_OUTER_TENSOR_GRAPH_HPP_
