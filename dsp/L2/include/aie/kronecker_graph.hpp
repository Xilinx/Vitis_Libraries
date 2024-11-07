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
#ifndef _DSPLIB_KRONECKER_GRAPH_HPP_
#define _DSPLIB_KRONECKER_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Kronecker library element.
*/
/**
 * @file kronecker_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>
#include "kronecker.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace kronecker {
/**
 * @defgroup kronecker_graph Kronecker function
 *
 * Kronecker
**/
//--------------------------------------------------------------------------------------------------
// kronecker_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup kronecker_graph
 * @brief Kronecker calculates Kronecker Matrix Product of the two matrices.

 *
 * These are the templates to configure the function.
 * @tparam TT_DATA_A describes the data type of input A to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint16, cint32, float, cfloat.
 * @tparam TT_DATA_B describes the data type of input B to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint16, cint32, float, cfloat.
 * @tparam TP_DIM_A_ROWS describes number of rows of input Matrix A.
 * @tparam TP_DIM_A_COLS describes number of columns of input Matrix A.
 * @tparam TP_DIM_B_ROWS describes number of rows of input Matrix B.
 * @tparam TP_DIM_B_COLS describes number of columns of input Matrix B.
 * @tparam TP_NUM_FRAMES describes number of input data frames to be processed per call to the function. \n
 *         Each frame corresponds to a Kronecker matrix product.
 * @tparam TP_API describes whether to use streams (1) or windows (0) on output.
 * @tparam TP_SHIFT described the number of bits to downshift each sample prior to output.
 * @tparam TP_SSR describes the number of kernels to use in parallel.
 * @tparam TP_RND describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although, TP_RND accepts unsigned integer values descriptive macros are recommended where:
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
          unsigned int TP_DIM_A_ROWS,
          unsigned int TP_DIM_A_COLS,
          unsigned int TP_DIM_B_ROWS,
          unsigned int TP_DIM_B_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_API,
          unsigned int TP_SHIFT,
          unsigned int TP_SSR = 1,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
/**
 **/
class kronecker_graph : public graph {
   public:
    /**
     * @cond NOCOMMENTS
     */
    static constexpr int TP_DIM_A_COLS_SSR = TP_DIM_A_COLS / TP_SSR;
    using kernelClass = kronecker<TT_DATA_A,
                                  TT_DATA_B,
                                  TP_DIM_A_ROWS,
                                  TP_DIM_A_COLS_SSR,
                                  TP_DIM_B_ROWS,
                                  TP_DIM_B_COLS,
                                  TP_NUM_FRAMES,
                                  TP_API,
                                  TP_SHIFT,
                                  TP_RND,
                                  TP_SAT>;
    using out_t = typename kernelClass::out_t;
    static constexpr unsigned int vecSizeA =
        ((std::is_same<TT_DATA_A, int16>::value) && (std::is_same<TT_DATA_B, int16>::value))
            ? (128 / 8 / sizeof(TT_DATA_A))
            : (256 / 8 / sizeof(TT_DATA_A));
    static constexpr unsigned int vecSizeB =
        ((std::is_same<TT_DATA_A, int16>::value) && (std::is_same<TT_DATA_B, int16>::value))
            ? (128 / 8 / sizeof(TT_DATA_A))
            : (256 / 8 / sizeof(TT_DATA_B));
    static constexpr unsigned int vecSizeOut = 256 / 8 / sizeof(out_t);
    static constexpr unsigned int sizeMatA = TP_DIM_A_ROWS * (TP_DIM_A_COLS / TP_SSR);
    static constexpr unsigned int sizeMatB = TP_DIM_B_ROWS * TP_DIM_B_COLS;
    static constexpr unsigned int sizeMatOut = sizeMatA * sizeMatB;
    static constexpr unsigned int kWindowVsizeA = CEIL((sizeMatA * TP_NUM_FRAMES), vecSizeA);
    static constexpr unsigned int kWindowVsizeB = CEIL((sizeMatB * TP_NUM_FRAMES), vecSizeB);
    static constexpr unsigned int kWindowVsizeOut = CEIL((sizeMatOut * TP_NUM_FRAMES), vecSizeOut);

/**
  * @endcond
  */
// Configuration legality checks
#if (__SUPPORTS_CFLOAT__ == 1) || (__SUPPORTS_EMULATED_CFLOAT__ == 1)
    static_assert((std::is_same<TT_DATA_A, int16>::value) || (std::is_same<TT_DATA_A, int32>::value) ||
                      (std::is_same<TT_DATA_A, cint16>::value) || (std::is_same<TT_DATA_A, cint32>::value) ||
                      (std::is_same<TT_DATA_A, float>::value) || (std::is_same<TT_DATA_A, cfloat>::value),
                  "ERROR: TT_DATA_A is not supported");
#else
    // AIE variants that don't support cfloat should flag that.
    static_assert((std::is_same<TT_DATA_A, int16>::value) || (std::is_same<TT_DATA_A, int32>::value) ||
                      (std::is_same<TT_DATA_A, cint16>::value) || (std::is_same<TT_DATA_A, cint32>::value) ||
                      (std::is_same<TT_DATA_A, float>::value),
                  "ERROR: TT_DATA_A is not supported");
#endif // (__SUPPORTS_CFLOAT__ == 1) || (__SUPPORTS_EMULATED_CFLOAT__ == 1)

    static_assert(TP_DIM_A_ROWS % vecSizeA == 0, "ERROR: TP_DIM_A_ROWS must be an integer multiple of vecSizeA");
    static_assert(TP_DIM_B_ROWS % vecSizeB == 0, "ERROR: TP_DIM_A_ROWS must be an integer multiple of vecSizeB");
    static_assert(TP_SSR > 0, "ERROR: Invalid SSR value, must be a value greater than 0.");
    static_assert(TP_DIM_A_COLS % TP_SSR == 0, "ERROR: Invalid SSR value. TP_DIM_A_COLS must be divisible by TP_SSR");
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API is not a supported value (0 or 1)");
    static_assert(fnValidateShiftFloat<TP_SHIFT, TT_DATA_A>(), "ERROR: TP_SHIFT must be 0 for float types.");
    static_assert(fnValidateShiftRange<TP_SHIFT>(), "ERROR: TP_SHIFT is out of the supported range.");
    static_assert(fnValidateRoundMode<TP_RND>(), "ERROR: Illegal round mode.");
    static_assert(fnValidateSatMode<TP_SAT>(), "ERROR: Illegal saturation mode.");

    /**
     * The input A data to the function.
     **/
    port_array<input, TP_SSR> inA;
    /**
     * The input B data to the function.
     **/
    port_array<input, TP_SSR> inB;
    /**
     * An API of TT_DATA type for the output of the function.
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
        * @brief This is the constructor function for the Kronecker graph.
        **/
    kronecker_graph() {
        for (int i = 0; i < TP_SSR; i++) {
            m_kernels[i] = kernel::create_object<kernelClass>();
            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.8; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source file(s)
            source(m_kernels[i]) = "kronecker.cpp";
            // Make connections
            connect(inA[i], m_kernels[i].in[0]);
            connect(inB[i], m_kernels[i].in[1]);
            dimensions(m_kernels[i].in[0]) = {kWindowVsizeA};
            dimensions(m_kernels[i].in[1]) = {kWindowVsizeB};
            if (TP_API == 0) {
                connect(m_kernels[i].out[0], out[i]);
                dimensions(m_kernels[i].out[0]) = {kWindowVsizeOut};
            } else {
                connect<stream>(m_kernels[i].out[0], out[i]);
            }
        }
    }; // constructor
};     // class
} // namespace kronecker
} // namespace aie
} // namespace xf
} // namespace dsp
#endif //_DSPLIB_KRONECKER_GRAPH_HPP_