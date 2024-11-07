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
#ifndef _DSPLIB_HADAMARD_GRAPH_HPP_
#define _DSPLIB_HADAMARD_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Hadamard function library element.
*/
/**
 * @file hadamard_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "hadamard.hpp"
#include "hadamard_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace hadamard {
using namespace adf;
/**
 * @defgroup hadamard_graph Hadamard function
 *
 * hadamard
**/
//--------------------------------------------------------------------------------------------------
// hadamard_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup hadamard_graph
 * @brief Hadamard product is element-wise multiplication of two vectors of same size.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA_A describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint16, cint32, float, cfloat.
 * @tparam TT_DATA_B describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint16, cint32, float, cfloat.
 * @tparam TP_DIM describes the number of samples in the vectors A and B.
 * @tparam TP_NUM_FRAMES describes the number of vectors to be processed in each
 *         call to this function.
 * @tparam TP_SHIFT describes power of 2 shift down applied to the accumulation of
 *         product terms before each output. ``TP_SHIFT`` must be in the range 0 to 59 (61 for AIE1).
 * @tparam TP_API described whether to use streams (1) or windows (0).
 * @tparam TP_SSR describes the number of kernels to use in parallel.
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
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_SSR,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
/**
 **/
class hadamard_graph : public graph {
   public:
    /**
    * @cond NOCOMMENTS
    */
    static constexpr int kMaxSSR = 16;

    // Defensive configuration legality checks

    static_assert(fnValidateShiftFloat<TP_SHIFT, TT_DATA_A>(), "ERROR: TP_SHIFT must be 0 for float types.");
    static_assert(fnValidateShiftRange<TP_SHIFT>(), "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(fnValidateRoundMode<TP_RND>(), "ERROR: Illegal round mode.");
    static_assert(fnValidateSatMode<TP_SAT>(), "ERROR: Illegal saturation mode.");

#if __STREAMS_PER_TILE__ == 2
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API is not a supported value i.e.; (0 or 1)");
#elif __STREAMS_PER_TILE__ == 1
    static_assert(TP_API == 0, "ERROR: TP_API is not a supported value i.e.; (0)");
#endif

#ifdef __SUPPORTS_ACC64__
    static_assert(!((std::is_same<TT_DATA_A, int32>::value) && (std::is_same<TT_DATA_B, cint16>::value)) &&
                      !((std::is_same<TT_DATA_A, cint16>::value) && (std::is_same<TT_DATA_B, int32>::value)),
                  "ERROR: Data type combination is not supported by AIE-ML.");
#endif

    static_assert(TP_SSR >= 0 && TP_SSR <= kMaxSSR, "ERROR: TP_SSR is not in the supported range of 1 to 16");
    static_assert(TP_DIM % TP_SSR == 0, "ERROR: TP_DIM is not a multiple of TP_SSR");

    static constexpr unsigned int kSamplesInVectOutData =
        TP_API == 0 ? 256 / 8 / (vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffWin)
                    : (128 / 8 / (vectByte<TT_DATA_A, TT_DATA_B>().val_byteBuffStream));

    static constexpr int DataSizePerSSR = TP_DIM / TP_SSR;
    static constexpr int paddedDataSize = CEIL(DataSizePerSSR, kSamplesInVectOutData);
    static constexpr int kKernelPtSize = paddedDataSize;
    static constexpr int kKernelWindowVsize = (TP_NUM_FRAMES * kKernelPtSize);

    static_assert(kKernelPtSize >= 16, "ERROR: TP_DIM/TP_SSR must be at least 16");
    static_assert(kKernelWindowVsize <= (__DATA_MEM_BYTES__ / 2),
                  "ERROR: TP_NUM_FRAMES*(TP_DIM/TP_SSR) must be at no more than data memory size.");
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
     * Access function to get pointer to kernel.
     **/

    kernel* getKernels() { return m_kernels; };

    /**
     * @brief This is the constructor function for the hadamard graph.
     **/
    hadamard_graph() {
        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<hadamard<TT_DATA_A, TT_DATA_B, kKernelPtSize, TP_NUM_FRAMES, TP_SHIFT,
                                                          TP_API, TP_SSR, TP_RND, TP_SAT> >();
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "hadamard.cpp";

            // make connections
            if (TP_API == 0) {
                connect(inA[i], m_kernels[i].in[0]);
                connect(inB[i], m_kernels[i].in[1]);
                dimensions(m_kernels[i].in[0]) = {kKernelWindowVsize};
                dimensions(m_kernels[i].in[1]) = {kKernelWindowVsize};
                connect(m_kernels[i].out[0], out[i]);
                dimensions(m_kernels[i].out[0]) = {kKernelWindowVsize};

            } else {
                connect<stream>(inA[i], m_kernels[i].in[0]);
                connect<stream>(inB[i], m_kernels[i].in[1]);
                connect<stream>(m_kernels[i].out[0], out[i]);
            }
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_HADAMARD_GRAPH_HPP_
