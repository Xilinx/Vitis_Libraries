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
#ifndef _DSPLIB_EUCLIDEAN_DISTANCE_REF_GRAPH_HPP_
#define _DSPLIB_EUCLIDEAN_DISTANCE_REF_GRAPH_HPP_

/*
    This file captures the definition of 'L2' graph level class for
    euclidean_distance reference model graph.
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
class euclidean_distance_ref_graph : public graph {
   public:
    // Defensive configuration legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataTypesOfInputs<TT_DATA_P, TT_DATA_Q>(),
                  " Assertion Failed : \n"
                  "              ERROR: TT_DATA_P and TT_DATA_Q are not a supported combination ");

    // defensive check for Output data type
    static_assert(fnCheckDataTypeOfOutput<TT_DATA_P, TT_DATA_Q, TT_DATA_OUT>(),
                  " Assertion Failed : \n"
                  "            ERROR: TT_DATA_P and TT_DATA_Q are not a supported combination for TT_DATA_OUT.");

    // defensive check for API Port is whether iobuffer or stream.
    static_assert(TP_API == 0,
                  " Assertion Failed : \n"
                  "ERROR: TP_API must be 0 for 'iobuffer' and no 'Stream' support for ED. ");

    // defensive check for SATURATION Mode which should be in the range i.e. SAT_MODE_MIN <TP_SAT< SAT_MODE_MAX
    static_assert(TP_SAT >= SAT_MODE_MIN && TP_SAT <= SAT_MODE_MAX,
                  " Assertion Failed : \n"
                  "             ERROR: TP_SAT is out of supported range");
    static_assert(TP_SAT != 2,
                  " Assertion Failed : \n"
                  "              ERROR: TP_SAT is invalid. Valid values of TP_SAT are 0, 1, and 3");

    // defensive check for num of frames i.e., TP_NUM_FRAMES restricted to 1 to maintain the better performance for time
    // being.
    static_assert(TP_NUM_FRAMES == 1,
                  " Assertion Failed : \n"
                  "            ERROR: 'TP_NUM_FRAMES' should be always equal to 1 (ONE) for ED.");

    /**
       * The input P data to the function.
   **/
    input_port inWindowP;

    /**
        * The input Q data to the function.
    **/
    input_port inWindowQ;

    /**
        * An API of TT_DATA type.
    **/
    output_port outWindow;

    /**
        * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    kernel m_euclidean_distance_ref;

    // DIM is fixed to '4' for compute ED.
    static constexpr int kDim = FIXED_DIM_ED;

    // Constructor
    euclidean_distance_ref_graph() {
        // Create EUCLIDEAN_DISTANCE class
        printf("\n EUCLIDEAN_DISTANCE Ref\n");
        m_euclidean_distance_ref = kernel::create_object<
            euclidean_distance_ref<TT_DATA_P, TT_DATA_Q, TT_DATA_OUT, TP_LEN_P, TP_LEN_Q, TP_DIM_P, TP_DIM_Q, TP_API,
                                   TP_RND, TP_SAT, TP_NUM_FRAMES, TP_IS_OUTPUT_SQUARED> >();
#ifdef _DSPLIB_EUCLIDEAN_DISTANCE_REF_DEBUG_
        printf("==========================================\n");
        printf("== EUCLIDEAN_DISTANCE REF KERNEL Graph ===\n");
        printf("==========================================\n");
        printf("TP_LEN_P                = %d\n", TP_LEN_P);
        printf("TP_LEN_Q                = %d\n", TP_LEN_Q);
        printf("TP_DIM_P                = %d\n", TP_DIM_P);
        printf("TP_DIM_Q                = %d\n", TP_DIM_Q);
        printf("TP_API                  = %d\n", TP_API);
        printf("TP_RND                  = %d\n", TP_RND);
        printf("TP_SAT                  = %d\n", TP_SAT);
        printf("TP_NUM_FRAMES           = %d\n", TP_NUM_FRAMES);
        printf("TP_IS_OUTPUT_SQUARED    = %d\n", TP_IS_OUTPUT_SQUARED);

#endif
        // make connections
        connect<>(inWindowP, m_euclidean_distance_ref.in[0]);
        dimensions(m_euclidean_distance_ref.in[0]) = {(TP_LEN_P * kDim) * TP_NUM_FRAMES};
        connect<>(inWindowQ, m_euclidean_distance_ref.in[1]);
        dimensions(m_euclidean_distance_ref.in[1]) = {(TP_LEN_Q * kDim) * TP_NUM_FRAMES};

        connect<>(m_euclidean_distance_ref.out[0], outWindow);
        dimensions(m_euclidean_distance_ref.out[0]) = {TP_LEN_P * TP_NUM_FRAMES};

        runtime<ratio>(m_euclidean_distance_ref) = 0.8;

        // Source files
        source(m_euclidean_distance_ref) = "euclidean_distance_ref.cpp";
        headers(m_euclidean_distance_ref) = {"euclidean_distance_ref.hpp"};

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