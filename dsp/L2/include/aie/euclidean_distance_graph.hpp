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

#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_GRAPH_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_GRAPH_HPP_
/*
    The file captures the definition of the 'L2' graph level class for
    the Euclidean Distance function library element.
*/

/**
    * @file euclidean_distance_graph.hpp
    *
**/
#include <stdio.h>
#include <adf.h>
#include <vector>
#include <array>
#include <type_traits>
#include <assert.h>
#include <cstdint>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "euclidean_distance.hpp"

using namespace adf;

alignas(__ALIGN_BYTE_SIZE__) extern const int8 sqrtLUT0[1024];
alignas(__ALIGN_BYTE_SIZE__) extern const int8 sqrtLUT1[1024];
#define FIXED_DIM 4

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {
/**
 * @defgroup euclidean_distance_graph euclidean distance functions
 *
 * Euclidean Distance
**/
//--------------------------------------------------------------------------------------------------
// euclidean_distance_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup euclidean_distance_graph
 * @brief euclidean_distance of input vectors P and Q
 *
 * These are the templates to configure the Euclidean Distance:
 * @tparam TT_DATA_P describes the type of individual data samples of point P with N Dimensions to input to the
 *function. \n
 *         This is a typename and must be one of the following: \n
 *         float and bfloat16.
 * @tparam TT_DATA_Q describes the type of individual data samples of point Q with N Dimensions to input to the
 *function. \n
 *         This is a typename and must be one of the following: \n
 *         float and bfloat16.
 * @tparam TT_DATA_OUT describes the type individual data samples at the function output. \n
 *         This is a typename and must be one of the following: \n
 *         float and bfloat16.TP_LEN_P
 * @tparam TP_LEN_P describes the number of samples in vector P. \n
 * @tparam TP_LEN_Q describes the number of samples in vector Q. \n
 * @tparam TP_DIM_P describes the Dimension of plane for the point P. \n
 *         This is a constant and must be nearest power of 2. the following are: \n
 *         4, 8, 16, 32, 64.....2^N;
 * @tparam TP_DIM_Q describes the number of samples in vector Q. \n
 *         This is a constant and must be nearest power of 2. the following are: \n
 *         4, 8, 16, 32, 64.....2^N;
 * @tparam TP_API describes whether to use streams (1) or windows (0). \n
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
 *
 * @tparam TP_NUM_FRAMES describes the number of frames
 *
 **/
// TT_DATA_P, TT_DATA_Q, TT_DATA_OUT, TP_LEN_P, TP_LEN_Q, TP_DIM_P, TP_DIM_Q, TP_API, TP_RND,
// TP_SAT, TP_NUM_FRAMES, TP_IS_OUTPUT_SQUARED
template <typename TT_DATA_P,
          typename TT_DATA_Q,
          typename TT_DATA_OUT,
          unsigned int TP_LEN_P,
          unsigned int TP_LEN_Q,
          unsigned int TP_DIM_P,
          unsigned int TP_DIM_Q,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_IS_OUTPUT_SQUARED>

class euclidean_distance_graph : public graph {
   public:
#if (__HAS_ACCUM_PERMUTES__ == 0)
    parameter edSqrtLut0;                            // LUT-0 for SQRT
    parameter edSqrtLut1;                            // LUT-1 for SQRT
    static constexpr unsigned int kNumOfKernels = 2; // Num of kernels becomes 2 when Arch. is AIE-ML
#else
    static constexpr unsigned int kNumOfKernels = 1; // Num of kernels becomes 1 when Arch. is AIE-1
#endif
    /**
     * @cond NOCOMMENTS
     */
    // Parameter value defensive and legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataTypesOfInputs<TT_DATA_P, TT_DATA_Q>(),
                  " Assertion Failed : \n"
                  "            ERROR: TT_DATA_P and TT_DATA_Q are not a supported Input Data type combination.");

    // defensive check for Output data type
    static_assert(
        fnCheckDataTypeOfOutput<TT_DATA_P, TT_DATA_Q, TT_DATA_OUT>(),
        " Assertion Failed : \n"
        "            ERROR: output data type 'TT_DATA_OUT' is not a supported one as per input data type combination");

    // defensive check for TP_LEN_P and TP_LEN_Q should be always same
    static_assert(TP_LEN_P == TP_LEN_Q,
                  " Assertion Failed : \n"
                  "             ERROR: Lengths of points P and Q should be same to compute ED.");

    // defensive check for Q sig length should be always less than or equal to P Sig Length
    static_assert(TP_DIM_P == TP_DIM_Q,
                  " Assertion Failed : \n"
                  "           ERROR: Dimensions of P and Q should be always same/equal to compute ED.");

    // defensive check for Dimension of point P should not be greater than 4
    static_assert(fnCheckforDimension<TP_DIM_P>(),
                  " Assertion Failed : \n"
                  "               ERROR: Dimension of point P should be less than or equal to 4 as per kernel design.");

    // defensive check for Dimension of point Q should not be greater than 4
    static_assert(fnCheckforDimension<TP_DIM_Q>(),
                  " Assertion Failed : \n"
                  "              ERROR: Dimension of point Q should be less than or equal to 4 as per kernel design.");

    // defensive check for API Port is whether iobuffer or stream.
    static_assert(TP_API == 0,
                  " Assertion Failed : \n"
                  "            ERROR: TP_API must be 0 for 'iobuffer'. ");

    // defensive check for ROUND Mode which should be in the range i.e. ROUND_MIN <TP_RND< ROUND_MAX
    static_assert(fnValidateRoundMode<TP_RND>(), "ERROR: Illegal round mode.");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(fnValidateSatMode<TP_SAT>(), "ERROR: Illegal saturation mode.");

    // defensive check for num of frames i.e., TP_NUM_FRAMES restricted to 1 to maintain the better performance for time
    // being.
    static_assert(TP_NUM_FRAMES == 1,
                  " Assertion Failed : \n"
                  "           ERROR: 'TP_NUM_FRAMES' should be always equal to 1 (ONE) for current version of ED.");

    // defensive check for output of ED i.e either SQRT Res or SQUARED Res
    static_assert(TP_IS_OUTPUT_SQUARED <= 1,
                  " Assertion Failed : \n"
                  "              ERROR: 'TP_IS_OUTPUT_SQUARED' must be \n "
                  "                               0- SQUARE_ROOT or \n "
                  "                     1- SQUARED  for both AIE-1 and AIE-2");

    /**
    * @endcond
    */
    /**
        * The input P data to the function.
    **/
    input_port inWindowP;
    /**
        * The input Q data to the function.
    **/
    input_port inWindowQ;
    /**
        * Output port to write the data out
    **/
    output_port outWindow;

    /**
        * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    // kernel m_ED_Squared;
    kernel m_EDKernels[kNumOfKernels];

    /**
 * Access function to get pointer to kernel (or first kernel in a chained and/or PHASE configurations).
 * No arguments required.
 **/
    kernel* getKernels() { return m_EDKernels; };

    /**
        * @brief This is the constructor function for the euclidean_distance graph.
    **/
    euclidean_distance_graph() {
// create the kernels
#if (__HAS_ACCUM_PERMUTES__ == 0)
        if
            constexpr(TP_IS_OUTPUT_SQUARED == 0) {
                typedef typename std::conditional<((std::is_same<TT_DATA_OUT, float>::value) &&
                                                   (TP_IS_OUTPUT_SQUARED == 0)),
                                                  bfloat16, TT_DATA_OUT>::type TT_INTERNAL_DATA_TYPE;
                edSqrtLut0 = parameter::array(sqrtLUT0); // LUT0 of SQRT
                edSqrtLut1 = parameter::array(sqrtLUT1); // LUT1 of SQRT
                m_EDKernels[0] = kernel::create_object<euclidean_distance_squared<
                    TT_DATA_P, TT_DATA_Q, TT_INTERNAL_DATA_TYPE, TP_LEN_P, TP_LEN_Q, TP_DIM_P, TP_DIM_Q, TP_API, TP_RND,
                    TP_SAT, TP_NUM_FRAMES, TP_IS_OUTPUT_SQUARED> >(); // SQUARED
                m_EDKernels[1] =
                    kernel::create_object<euclidean_distance_sqrt<TT_INTERNAL_DATA_TYPE, TT_DATA_OUT, TP_LEN_P,
                                                                  TP_NUM_FRAMES, TP_IS_OUTPUT_SQUARED> >(); // SQRT
            }
        else {
            m_EDKernels[0] = kernel::create_object<
                euclidean_distance_squared<TT_DATA_P, TT_DATA_Q, TT_DATA_OUT, TP_LEN_P, TP_LEN_Q, TP_DIM_P, TP_DIM_Q,
                                           TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_IS_OUTPUT_SQUARED> >(); // SQUARED
        }
#else
        m_EDKernels[0] = kernel::create_object<
            euclidean_distance_squared<TT_DATA_P, TT_DATA_Q, TT_DATA_OUT, TP_LEN_P, TP_LEN_Q, TP_DIM_P, TP_DIM_Q,
                                       TP_API, TP_RND, TP_SAT, TP_NUM_FRAMES, TP_IS_OUTPUT_SQUARED> >(); // SQUARED
#endif

        if
            constexpr(TP_API == USE_WINDOW_API) {
                // make connections
                connect<>(inWindowP, m_EDKernels[0].in[0]);
                connect<>(inWindowQ, m_EDKernels[0].in[1]);
                dimensions(m_EDKernels[0].in[0]) = {(TP_LEN_P * FIXED_DIM) * TP_NUM_FRAMES};
                dimensions(m_EDKernels[0].in[1]) = {(TP_LEN_Q * FIXED_DIM) * TP_NUM_FRAMES};

#if (__HAS_ACCUM_PERMUTES__ == 0)
                if
                    constexpr(TP_IS_OUTPUT_SQUARED == 0) {
                        // connect LUTs of Sqrt() to the kernel
                        connect<>(edSqrtLut0, m_EDKernels[1]);
                        connect<>(edSqrtLut1, m_EDKernels[1]);
                    }
#endif

                if
                    constexpr(TP_IS_OUTPUT_SQUARED == 0) {
#if (__HAS_ACCUM_PERMUTES__ == 0)
                        connect<>(m_EDKernels[0].out[0], m_EDKernels[1].in[0]);
                        dimensions(m_EDKernels[0].out[0]) = {(TP_LEN_P * TP_NUM_FRAMES)};
                        dimensions(m_EDKernels[1].in[0]) = {(TP_LEN_P * TP_NUM_FRAMES)};

                        // connect final kernel output to output of the graph
                        connect<>(m_EDKernels[1].out[0], outWindow);
                        dimensions(m_EDKernels[1].out[0]) = {(TP_LEN_P * TP_NUM_FRAMES)};
#else
                        // connect final kernel output to output of the graph
                        connect<>(m_EDKernels[0].out[0], outWindow);
                        dimensions(m_EDKernels[0].out[0]) = {(TP_LEN_P * TP_NUM_FRAMES)};
#endif
                    }
                else {
                    // connect final kernel output to output of the graph
                    connect<>(m_EDKernels[0].out[0], outWindow);
                    dimensions(m_EDKernels[0].out[0]) = {(TP_LEN_P * TP_NUM_FRAMES)};
                }
            }
        // Specify mapping constraints
        runtime<ratio>(m_EDKernels[0]) = 0.5; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_EDKernels[0]) = "euclidean_distance.cpp";
        headers(m_EDKernels[0]) = {"euclidean_distance.hpp"};

#if (__HAS_ACCUM_PERMUTES__ == 0)
        if
            constexpr(TP_IS_OUTPUT_SQUARED == 0) {
                // Specify mapping constraints
                runtime<ratio>(m_EDKernels[1]) =
                    0.4; // Nominal figure. The real figure requires knowledge of the sample rate.
                // Source files
                source(m_EDKernels[1]) = "euclidean_distance.cpp";
                headers(m_EDKernels[1]) = {"euclidean_distance.hpp"};
            }
#endif

    }; // constructor
};

} // namespace euclidean_distance
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_GRAPH_HPP_
