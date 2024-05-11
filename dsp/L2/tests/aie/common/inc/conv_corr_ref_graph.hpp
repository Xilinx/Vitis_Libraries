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
#ifndef _DSPLIB_CONV_CORR_REF_GRAPH_HPP_
#define _DSPLIB_CONV_CORR_REF_GRAPH_HPP_

/*
    This file captures the definition of 'L2' graph level class for
    conv_corr reference model graph.
*/

#include <adf.h>
#include <vector>
#include <type_traits>
#include <adf/arch/aie_arch_properties.hpp>
#include "conv_corr_ref.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

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
          unsigned int TP_RND,
          unsigned int TP_SAT>
class conv_corr_ref_graph : public graph {
   public:
    // Defensive configuration legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataTypesOfInputs<TT_DATA_F, TT_DATA_G>(),
                  "ERROR: TT_DATA_F and TT_DATA_G are not a supported combination ");

    // defensive check for Output data type
    static_assert(fnCheckDataTypeOfOutput<TT_DATA_F, TT_DATA_G, TT_DATA_OUT>(),
                  " ERROR: TT_DATA_F and TT_DATA_G are not a supported combination for TT_DATA_OUT.");

    // defensive check for Function type i.e either CONV or CORR
    static_assert(TP_FUNCT_TYPE <= 1,
                  "ERROR: TP_FUNCT_TYPE must be 0-CORRELATION or 1-CONVOLUTION for AIE-1 and AIE-2");

    // defensive check for CONV/CORR compute mode i.e. full/same/valid
    static_assert(TP_COMPUTE_MODE <= 2,
                  "ERROR: TP_COMPUTE_MODE must be 0-FULL_MODE or 1-SAME_MODE or 2-VALID_MODE for both CONV. and CORR.");

    // defensive check for G sig length should be always less than or equal to F Sig Length
    static_assert(TP_G_LEN <= TP_F_LEN,
                  "ERROR: TP_G_LEN should be always less than or equal to TP_F_LEN as per con_corr requirement. ");

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
    static_assert(TP_SHIFT >= SHIFT_MIN && TP_SHIFT < SHIFT_MAX, "ERROR: TP_SHIFT is out of the supported range.");

    // defensive check for API Port is whether iobuffer or stream.
    static_assert(TP_API == 0 || TP_API == 1, "ERROR: TP_API must be either 0 for 'iobuffer' or 1 for 'Stream'. ");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX, "ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2, "ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");

    /**
        * The input data to the function.
    **/
    input_port inWindowF;
    input_port inWindowG;

    /**
        * An API of TT_DATA type.
    **/
    output_port outWindow;

    /**
        * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    kernel m_conv_corr_ref;

    // Constructor
    conv_corr_ref_graph() {
        // Create CONV_CORR class
        printf("\n CONV_CORR Ref\n");
        m_conv_corr_ref =
            kernel::create_object<conv_corr_ref<TT_DATA_F, TT_DATA_G, TT_DATA_OUT, TP_FUNCT_TYPE, TP_COMPUTE_MODE,
                                                TP_F_LEN, TP_G_LEN, TP_SHIFT, TP_API, TP_RND, TP_SAT> >();

        static constexpr unsigned int kLanes = ref_CC_NumLanes<TT_DATA_F, TT_DATA_G>();
        static constexpr unsigned int kLoopCount = getLoopCount<TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();
        static constexpr unsigned int kRefLoopCount = (CEIL(kLoopCount, kLanes) / kLanes);
        static constexpr unsigned int kRefOutLen = (kRefLoopCount * kLanes);
        static constexpr unsigned int kRefPaddedFsigLen =
            getRefPaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE, TP_F_LEN, TP_G_LEN>();

#ifdef _DSPLIB_CONV_CORR_REF_DEBUG_
        printf("=================================\n");
        printf("== CONV_CORR REF KERNEL Graph ===\n");
        printf("=================================\n");
        printf(
            " if 'TP_FUNCT_TYPE' is 1 the its CONV.\
                                     else its CORR if its 0\n");
        printf("Function Type        = %d\n", TP_FUNCT_TYPE);
        printf(
            "if TP_COMPUTE_MODE is 0 then FULL Mode\
                                    if TP_COMPUTE_MODE is 1 then SAME Mode \
                                    if TP_COMPUTE_MODE is 2 then its VALID Mode \n");
        printf("Mode                 = %d\n", TP_COMPUTE_MODE);
        printf("TP_F_LEN             = %d\n", TP_F_LEN);
        printf("TP_G_LEN             = %d\n", TP_G_LEN);
        printf("TP_SHIFT             = %d\n", TP_SHIFT);
        printf("TP_API               = %d\n", TP_API);
        printf("TP_RND               = %d\n", TP_RND);
        printf("TP_SAT               = %d\n", TP_SAT);
#endif
        // make connections
        connect<>(inWindowF, m_conv_corr_ref.in[0]);
        dimensions(m_conv_corr_ref.in[0]) = {kRefPaddedFsigLen};
        connect<>(inWindowG, m_conv_corr_ref.in[1]);
        dimensions(m_conv_corr_ref.in[1]) = {TP_G_LEN};

        connect<>(m_conv_corr_ref.out[0], outWindow);
        dimensions(m_conv_corr_ref.out[0]) = {kRefOutLen};

        runtime<ratio>(m_conv_corr_ref) = 0.8;

        // Source files
        source(m_conv_corr_ref) = "conv_corr_ref.cpp";
        headers(m_conv_corr_ref) = {"conv_corr_ref.hpp"};

        printf("==========================================\n");
        printf("== END of CONV_CORR REF KERNEL Graph =====\n");
        printf("==========================================\n");
    };
};
} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_CONV_CORR_REF_GRAPH_HPP_