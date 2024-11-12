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
#ifndef _DSPLIB_FUNC_APPROX_GRAPH_HPP_
#define _DSPLIB_FUNC_APPROX_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Function Approximation library element.
*/
/**
 * @file func_approx_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <tuple>

#include "graph_utils.hpp"
#include "func_approx.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace func_approx {
using namespace adf;

/**
 * @defgroup func_approx Function Approximation
 *
 * Vectorized Function Approximation solution.
 *
 */

//--------------------------------------------------------------------------------------------------
// func_approx template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup func_approx
 * @brief `func_approx` is a utility that provides an approximation of f(x) for a given input data, x.
 *
 * These are the templates to configure the function:
 *
 * @tparam TT_DATA Describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         - int16, int32, and float for AIE
 *         - int16, int32, float, and bfloat16 for AIE-ML \n
 *
 * @tparam TP_COARSE_BITS Describes the number of bits in a sample of input data that will
 *         be used to address the provided lookup table. It determines the total number of locations in the lookup
 *table.
 *
 * @tparam TP_FINE_BITS Describes the number of bits in an input data sample used for fine interpolation.
 *
 * @tparam TP_DOMAIN_MODE Specifies a mode for the input domain of the chosen function for approximation. \n
 *         There are three modes available: \n
 *         - TP_DOMAIN_MODE = 0: Domain of input, x, is from 0 to 1 \n
 *         - TP_DOMAIN_MODE = 1: Domain of input, x, is from 1 to 2.
 *           (The most significant TP_COARSE_BITS bit must be set to zero and will be ignored when addressing the lookup
 *table.
 *           Refer to the Function Approximation User Guide for more information) \n
 *         - TP_DOMAIN_MODE = 2: Domain of input, x, is from 1 to 4 \n
 *
 * @tparam TP_WINDOW_VSIZE Describes the number of samples to be processed in each call to this function. \n
 *         Configurations that are not supported by the lookup table API functions
 *         (all data types on AIE1, and int32 and float on AIE-ML)
 *         will require memory for two additional internal buffers of size TP_WINDOW_VSIZE. \n
 *
 * @tparam TP_SHIFT Describes the number of bits to downshift the final output approximation. \n
 *
 * @tparam TP_RND Describes the selection of rounding to be applied during the
 *         shift down stage of processing. \n
 *         Although TP_RND accepts unsigned integer values, descriptive macros are recommended: \n
 *         - rnd_floor      = Truncate LSB, always round down (towards negative infinity).
 *         - rnd_ceil       = Always round up (towards positive infinity).
 *         - rnd_sym_floor  = Truncate LSB, always round towards 0.
 *         - rnd_sym_ceil   = Always round up towards infinity.
 *         - rnd_pos_inf    = Round halfway towards positive infinity.
 *         - rnd_neg_inf    = Round halfway towards negative infinity.
 *         - rnd_sym_inf    = Round halfway towards infinity (away from zero).
 *         - rnd_sym_zero   = Round halfway towards zero (away from infinity).
 *         - rnd_conv_even  = Round halfway towards the nearest even number.
 *         - rnd_conv_odd   = Round halfway towards the nearest odd number. \n
 *         No rounding is performed on ceil or floor mode variants. \n
 *         Other modes round to the nearest integer. They differ only in how
 *         they round for values of 0.5. \n
 *         \n
 *         Note: Rounding modes `rnd_sym_floor` and `rnd_sym_ceil` are only supported on the AIE-ML device. \n
 *
 * @tparam TP_SAT Describes the selection of saturation to be applied during the shift down stage of processing. \n
 *         TP_SAT accepts unsigned integer values, where:
 *         - 0: none           = No saturation is performed, and the value is truncated on the MSB side.
 *         - 1: saturate       = Default. Saturation rounds an n-bit signed value
 *           in the range [- (2^(n-1)) : +2^(n-1) - 1].
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *           an n-bit signed value in the range [- (2^(n-1) - 1) : +2^(n-1) - 1]. \n
 **/

template <typename TT_DATA,
          unsigned int TP_COARSE_BITS,
          unsigned int TP_FINE_BITS,
          unsigned int TP_DOMAIN_MODE,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class func_approx_graph : public graph {
   public:
    // When TT_DATA is bfloat16, values stored in the LUT should be float
    typedef typename std::conditional<std::is_same<TT_DATA, bfloat16>::value, float, TT_DATA>::type TT_LUT;

    /**
     * @cond NOCOMMENTS
     */

    // For TP_DOMAIN_MODE = 1, the MSB of TP_COARSE_BITS is assumed to have been set to 0. It is not used for addressing
    // the LUT.
    static constexpr int ignoreTopDomainBit = (TP_DOMAIN_MODE == 1) ? 1 : 0;
    static constexpr int kLutValues = (2 << (TP_COARSE_BITS - ignoreTopDomainBit));
// linear_approx API is only supported for AIE-ML with bfloat16 or int16 datatypes.
#ifdef _SUPPORTS_BFLOAT16_
    static constexpr int useLutAPI =
        std::is_same<TT_DATA, bfloat16>::value ? 1 : (std::is_same<TT_DATA, int16>::value ? 1 : 0);
#else
    static constexpr int useLutAPI = 0;
#endif //_SUPPORTS_BFLOAT16_
    // there are two internal buffers used when useLutAPI=0 (slopeBuff and offsetBuff). Each of size TP_WINDOW_VSIZE.
    // No internal buffers used when useLutAPI=1, but set to 16 as required for constructor argument
    static constexpr int internalBuffSize = (useLutAPI == 0) ? TP_WINDOW_VSIZE : 16;
    // When using linear_approx API with parallel access = 4, each 128 bits of lut values are to be duplicated.
    static constexpr int dupe128b = useLutAPI + 1;
    static constexpr int lutValsIn128b = 128 / 8 / sizeof(TT_LUT);

// static asserts
// TT_DATA - AIE1 - int16, int32, float
#ifdef _SUPPORTS_BFLOAT16_
    static_assert((std::is_same<TT_DATA, int16>::value) || (std::is_same<TT_DATA, int32>::value) ||
                      (std::is_same<TT_DATA, float>::value) || (std::is_same<TT_DATA, bfloat16>::value),
                  "ERROR: TT_DATA must int16, int32, float, or bfloat16 for AIE-ML");
#else
    // TT_DATA - AIE2 - int16, int32, float, bfloat16
    static_assert((std::is_same<TT_DATA, int16>::value) || (std::is_same<TT_DATA, int32>::value) ||
                      (std::is_same<TT_DATA, float>::value),
                  "ERROR: TT_DATA must int16, int32, or float for AIE");
#endif //_SUPPORTS_BFLOAT16_ == 0
       // TP_COARSE_BITS - lut size < DATA_MEMORY
    static_assert((kLutValues * sizeof(TT_LUT) * (dupe128b)) <= __DATA_MEM_BYTES__,
                  "ERROR: The required size of the lookup tables (as specified by TP_COARSE_BITS) exceeds memory.");
    // TP_FINE_BITS + TP_COARSE_BITS < 8*sizeof(TT_DATA)
    static_assert(
        (TP_FINE_BITS + TP_COARSE_BITS) <= (8 * sizeof(TT_DATA)),
        "ERROR: The sum of values for TP_FINE_BITS and TP_COARSE_BITS exceeds the number of bits in TT_DATA.");
    // TP_DOMAIN_MODE = 0, 1, 2
    static_assert(TP_DOMAIN_MODE >= 0 && TP_DOMAIN_MODE <= 3, "ERROR: TP_DOMAIN_MODE must be set to 0, 1 or 2.");
    // TP_WINDOW_VSIZE *sizeof(TT_DATA) < DATA_MEMORY
    static_assert(sizeof(TT_DATA) * (TP_WINDOW_VSIZE) <= __DATA_MEM_BYTES__,
                  "ERROR: TP_WINDOW_VSIZE * sizeof(TT_DATA) must be less or equal to data memory size.");
    // TP_SHIFT !=0 for floats - common?
    static_assert(TP_SHIFT == 0 || !(std::is_same<TT_DATA, bfloat16>::value || std::is_same<TT_DATA, bfloat16>::value),
                  "ERROR: TP_SHIFT must be 0 for float operation");
    static_assert(TP_SHIFT >= 0 && TP_SHIFT < 61, "ERROR: TP_SHIFT is out of the supported range (0 to 61)");

    /**
    * @endcond
    */

    /**
     * The input data (x) to the function.
     **/
    port_array<input, 1> in;
    /**
     * The output data  of the function.
     **/
    port_array<output, 1> out;
    /**
     * The kernel that will be created and mapped onto an AIE tile.
     **/
    kernel m_kernel[1];

    /**
     * Access function to get pointer to kernel.
     **/
    kernel* getKernels() { return m_kernel; };

    std::vector<TT_LUT> lut_ab, lut_cd;

    /**
     * @brief This is the constructor function for the func_approx graph.
     **/
    func_approx_graph(const std::array<TT_LUT, kLutValues>& lut) {
        // 128 bit duplication is carried out (if necessary), and two new identical copies of the lut are created.
        for (int i = 0; i < (kLutValues / lutValsIn128b); i++) {
            for (int j = 0; j < dupe128b; j++) {
                for (int k = 0; k < lutValsIn128b; k++) {
                    lut_ab.push_back(lut[i * lutValsIn128b + k]);
                }
            }
        }
        lut_cd = lut_ab;
        // Internal buffers that will be loaded with slope and offset values in the kernel
        std::array<TT_LUT, internalBuffSize> slopeBuff = {};
        std::array<TT_LUT, internalBuffSize> offsetBuff = {};

        m_kernel[0] =
            kernel::create_object<func_approx<TT_DATA, TP_COARSE_BITS, TP_FINE_BITS, TP_DOMAIN_MODE, TP_WINDOW_VSIZE,
                                              TP_SHIFT, TP_RND, TP_SAT> >(lut_ab, lut_cd, slopeBuff, offsetBuff);
        // Specify mapping constraints
        runtime<ratio>(m_kernel[0]) = 0.9; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_kernel[0]) = "func_approx.cpp";

        connect(in[0], m_kernel[0].in[0]);
        dimensions(m_kernel[0].in[0]) = {TP_WINDOW_VSIZE};
        connect(m_kernel[0].out[0], out[0]);
        dimensions(m_kernel[0].out[0]) = {TP_WINDOW_VSIZE};
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_FUNC_APPROX_GRAPH_HPP_
