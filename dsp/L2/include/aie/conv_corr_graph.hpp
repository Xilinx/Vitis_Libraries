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

#ifndef _DSPLIB_CONV_CORR_GRAPH_HPP_
#define _DSPLIB_CONV_CORR_GRAPH_HPP_
/*
    The file captures the definition of the 'L2' graph level class for
    the Convolution / Correlation  function library element.
*/

/**
    * @file conv_corr_graph.hpp
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
#include "conv_corr.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {
/**
 * @defgroup conv_corr_graph Convolution / Correlation functions
 *
 * Convolution / Correlation
**/
//--------------------------------------------------------------------------------------------------
// conv_corr_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup conv_corr_graph
 * @brief Convolution / Correlation of input vectors F and G
 *
 * These are the templates to configure the convolution and correlation:
 * @tparam TT_DATA_F describes the type of individual data samples of signal F to input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int8, int16, int32, cint16, cint32.
 * @tparam TT_DATA_G describes the type of individual data samples of signal G to input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         int8, int16, int32, cint16, cint32.
 * @tparam TT_DATA_OUT describes the type individual data samples at the function output. \n
 *         This is a typename and must be one of the following: \n
 *         int16, int32, cint32, cint32.
 *         Note: TT_DATA_OUT type should be complex if any input is complex.
 * @tparam TP_FUNCT_TYPE describes the selection of function i.e convolution or correlation.
 * @tparam TP_COMPUTE_MODE describes the computation mode of conv/corr. we have 3 modes which are
 *         - FULL  (0) -- Length of F Signal will be updated by padding with zeros of G len (G-1) i.e. Len_F = [ (G-1)
 *         + F + (G-1) ]
 *         - SAME  (1) -- Length of F Signal will be updated by padding with zeros of G len (G/2-1)  i.e. Len_F = [
 *         ((G/2)-1) + F + ((G/2)-1) ]
 *         - VALID (2) -- Length of F Signal will be updated by padding with zeros of F_load i.e. Len_F = [ F +
 *         data_load]
 * @tparam TP_F_LEN describes the number of samples in vector F.
 * @tparam TP_G_LEN describes the number of samples in vector G.
 * @tparam TP_SHIFT describes the number of bits to downshift.
 * @tparam TP_API describes whether to use streams (1) or windows (0).
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
 *         - 3: symmetric      = Controls symmetric saturation. Symmetric saturation rounds
 *         an n-bit signed value in the range [- ( 2^(n-1) -1 ) : +2^(n-1) - 1 ]. \n
 **/
// TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND,
// TP_SAT
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>

class conv_corr_graph : public graph {
   public:
    /**
     * @cond NOCOMMENTS
     */
    // Parameter value defensive and legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataTypesOfInputs<TT_DATA_F, TT_DATA_G>(),
                  "ERROR: TT_DATA_F and TT_DATA_G are not a supported combination.");

    // defensive check for Output data type
    static_assert(fnCheckDataTypeOfOutput<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>(),
                  "ERROR: TT_DATA_F and TT_DATA_G are not a supported combination for TT_DATA_OUT.");

    // defensive check for Function type i.e either CONV or CORR
    static_assert(TP_FUNCT_TYPE <= 1,
                  "ERROR: TP_FUNCT_TYPE must be 0-CORRELATION or 1-CONVOLUTION for AIE-1 and AIE-2.");

    // defensive check for CONV/CORR compute mode i.e. full/same/valid
    static_assert(TP_COMPUTE_MODE <= 2,
                  "ERROR: TP_COMPUTE_MODE must be 0-FULL_MODE or 1-SAME_MODE or 2-VALID_MODE for both CONV. and CORR.");

    // defensive check for G sig length should be always less than or equal to F Sig Length
    static_assert(TP_G_LEN <= TP_F_LEN,
                  "ERROR: TP_G_LEN should be always less than or equal to TP_F_LEN as per con_corr requirement.");

    // defensive check for Lengths of F and G should be in the given range of Min and Max
    static_assert(fnCheckLenOfData<TT_DATA_F, TP_F_LEN>(),
                  " Assertion Failed : \n            ERROR: TP_F_LEN should be granuality of Min data_load on AIE i.e. "
                  "[(256/samplesize<TT_DATA_F>())] \n                   [int8   - (32*N) ] \n                   [int16 "
                  " - (16*N) ] \n                   [int32  - (8*N)  ] \n                   [cint16 - (8*N)  ] \n      "
                  "             [cint32 - (4*N)  ] \n            where N is Integer > 1] and \n            TP_F_LEN "
                  "should be greater than or equal to minimum length [((256/samplesize<TT_DATA_F>())*2)] based on "
                  "given data type i.e.\n                 '[Data Type-    MIN    MAX]' \n                 "
                  "'--------------------------' \n                 '[int8     -    64    8192]' \n                 "
                  "'[int16    -    32    4096]' \n                 '[int32    -    16    2048]' \n                 "
                  "'[cint16   -    16    2048]' \n                 '[cint32   -    8     1024]' ");
    static_assert(fnCheckLenOfData<TT_DATA_G, TP_G_LEN>(),
                  " Assertion Failed : \n            ERROR: TP_G_LEN should be granuality of Min data_load on AIE i.e. "
                  "[(256/samplesize<TT_DATA_G>())] \n                   [int8   - (32*N) ] \n                   [int16 "
                  " - (16*N) ] \n                   [int32  - (8*N)  ] \n                   [cint16 - (8*N)  ] \n      "
                  "             [cint32 - (4*N)  ] \n            where N is Integer > 1] and \n            TP_G_LEN "
                  "should be greater than or equal to minimum length [((256/samplesize<TT_DATA_G>())*2)] based on "
                  "given data type i.e.\n                 '[Data Type-    MIN    MAX]' \n                 "
                  "'--------------------------' \n                 '[int8     -    64    8192]' \n                 "
                  "'[int16    -    32    4096]' \n                 '[int32    -    16    2048]' \n                 "
                  "'[cint16   -    16    2048]' \n                 '[cint32   -    8     1024]' ");

    // defensive check for scaling factor should be in the range i.e. 0 < SHIFT < 61
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT <= SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");

    // defensive check for API Port is whether iobuffer or stream.
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API must be either 0 for 'iobuffer' or 1 for 'Stream'. ");

    // defensive check for ROUND Mode which should be in the range i.e. ROUND_MIN <TP_RND< ROUND_MAX
    static_assert(TP_RND >= ROUND_MIN && TP_RND <= ROUND_MAX, "ERROR: TP_RND is out of the supported range.");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");
    /**
    * @endcond
    */
    /**
        * The input F data to the function.
    **/
    input_port inWindowF;
    /**
        * The input G data to the function.
    **/
    input_port inWindowG;
    /**
        * An API of TT_DATA type.
    **/
    output_port outWindow;

    /**
        * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    kernel m_conv_corr;

    /**
        * @brief This is the constructor function for the conv_corr graph.
    **/
    conv_corr_graph() {
        m_conv_corr = kernel::create_object<conv_corr<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE,
                                                      TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT> >();

        // Loop Count
        static constexpr int kLanes = getNumofLanes<TT_DATA_F, TT_DATA_G>();
        static constexpr int kLoopCount = getLoopCount<TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();
        static constexpr int kAieLoopCount = (CEIL(kLoopCount, kLanes) / kLanes);
        static constexpr int kOutLen = (kAieLoopCount * kLanes);
        static constexpr int kpaddedFsigLen =
            getPaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

        // make connections
        if
            constexpr(TP_API == 0) {
                connect(inWindowF, m_conv_corr.in[0]);
                connect(inWindowG, m_conv_corr.in[1]);
                dimensions(m_conv_corr.in[0]) = {kpaddedFsigLen};
                dimensions(m_conv_corr.in[1]) = {TP_G_LEN};

                // connect final kernel output to output of the graph
                connect(m_conv_corr.out[0], outWindow);
                dimensions(m_conv_corr.out[0]) = {kOutLen};
            }
        else {
            // To DO: Then below connection should be change in future when we have stream
            //       related kernel function till then iobuffer is using to process when user pass stream data.
            connect<>(inWindowF, m_conv_corr.in[0]);
            connect<>(inWindowG, m_conv_corr.in[1]);
            dimensions(m_conv_corr.in[0]) = {kpaddedFsigLen};
            dimensions(m_conv_corr.in[1]) = {TP_G_LEN};

            connect<>(m_conv_corr.out[0], outWindow);
            dimensions(m_conv_corr.out[0]) = {kOutLen};
        }
        // Specify mapping constraints
        runtime<ratio>(m_conv_corr) = 0.1; // Nominal figure. The real figure requires knowledge of the sample rate.
        // Source files
        source(m_conv_corr) = "conv_corr.cpp";
        headers(m_conv_corr) = {"conv_corr.hpp"};
    }; // constructor
};

} // namespace conv_corr
} // namespace aie
} // namespace dsp
} // namespace xf

#endif // _DSPLIB_CONV_CORR_GRAPH_HPP_
