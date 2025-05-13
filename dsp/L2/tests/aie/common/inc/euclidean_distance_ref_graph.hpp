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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_REF_GRAPH_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_REF_GRAPH_HPP_

/*
    This file captures the definition of 'L2' graph level class for
    Euclidean Distance reference model graph.
*/

#include <adf.h>
#include <vector>
#include <type_traits>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include "euclidean_distance_ref.hpp"

using namespace adf;
namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

// TT_DATA, TT_DATA_OUT, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED
template <typename TT_DATA,
          typename TT_DATA_OUT,
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_IS_OUTPUT_SQUARED>
class euclidean_distance_ref_graph : public graph {
   public:
    /**
     * @cond NOCOMMENTS
     */
    // Defensive configuration legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataTypesOfInputs<TT_DATA>(),
                  " Assertion Failed : \n"
                  "            ERROR: TT_DATA is not a supported Input Data type.");

    // defensive check for Lengths of F and G should be in the given range of Min and Max
    static_assert(fnCheckLenOfData<TT_DATA, TP_LEN>(),
                  " Assertion Failed : \n "
                  "             ERROR: TP_LEN should be granuality of Min data_load on AIE i.e. "
                  "[(256/samplesize<TT_DATA>())] \n                   [float   - (8*N) ] \n   [bfloat16  - (16*N)  ]] "
                  "\n  where N is Integer > 1] and \n            TP_LEN "
                  "should be greater than or equal to minimum length [((256/samplesize<TT_DATA>())*2)] based on "
                  "given data type i.e.\n                 '[Data Type-    MIN    MAX]' \n                 "
                  "'--------------------------' \n                 '[float    -    16    2048]' \n                 "
                  "'[bfloat16 -    32    4096]'  ");

    // defensive check for Dimension of point P should not be greater than 4
    static_assert(fnCheckforDimension<TP_DIM>(),
                  " Assertion Failed : \n"
                  "               ERROR: Dimension of point P should be less than or equal to 4 as per kernel design.");

    // defensive check for API Port is whether iobuffer or stream.
    static_assert(TP_API == 0,
                  " Assertion Failed : \n"
                  "            ERROR: TP_API must be 0 for 'iobuffer'. ");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(TP_SAT >= kMinSaturationMode && TP_SAT <= kMaxSaturationMode,
                  " Assertion Failed : \n"
                  "             ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != kUnSuppportedSaturationMode,
                  " Assertion Failed : \n"
                  "              ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");

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
    kernel m_ED_ref_kernel;

    // DIM is fixed to '4' for compute ED.
    static constexpr int kDim = kFixedDimOfED;

    /**
        * @brief This is the constructor function for the euclidean_distance_ref graph.
    **/
    euclidean_distance_ref_graph() {
        // Create EUCLIDEAN_DISTANCE class
        m_ED_ref_kernel = kernel::create_object<euclidean_distance_ref<TT_DATA, TT_DATA_OUT, TP_LEN, TP_DIM, TP_API,
                                                                       TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED> >();
#ifdef _DSPLIB_EUCLIDEAN_DISTANCE_REF_DEBUG_
        printf("==========================================\n");
        printf("== EUCLIDEAN_DISTANCE REF KERNEL Graph ===\n");
        printf("==========================================\n");
        printf("LEN                 = %d \n", LEN);
        printf("DIM                 = %d \n", DIM);
        printf("API                   = %d \n", API_IO);
        printf("RND                   = %d \n", RND);
        printf("SAT                   = %d \n", SAT);
        printf("IS_OUTPUT_SQUARED     = %d \n", IS_OUTPUT_SQUARED);

#endif
        // make connections
        connect<>(inWindowP, m_ED_ref_kernel.in[0]);
        dimensions(m_ED_ref_kernel.in[0]) = {(TP_LEN * kDim)};
        connect<>(inWindowQ, m_ED_ref_kernel.in[1]);
        dimensions(m_ED_ref_kernel.in[1]) = {(TP_LEN * kDim)};

        connect<>(m_ED_ref_kernel.out[0], outWindow);
        dimensions(m_ED_ref_kernel.out[0]) = {TP_LEN};

        runtime<ratio>(m_ED_ref_kernel) = 0.8;

        // Source files
        source(m_ED_ref_kernel) = "euclidean_distance_ref.cpp";
        headers(m_ED_ref_kernel) = {"euclidean_distance_ref.hpp"};

        printf("===================================================\n");
        printf("== END of EUCLIDEAN_DISTANCE REF KERNEL Graph =====\n");
        printf("===================================================\n");
    };
};
} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_REF_GRAPH_HPP_