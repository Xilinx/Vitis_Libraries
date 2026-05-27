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

// TT_DATA, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED
template <typename TT_DATA,
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
        * Output port to write the data out.
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
        m_ED_ref_kernel = kernel::create_object<
            euclidean_distance_ref<TT_DATA, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED> >();
#ifdef _DSPLIB_EUCLIDEAN_DISTANCE_REF_DEBUG_
        printf("==========================================\n");
        printf("== EUCLIDEAN_DISTANCE REF KERNEL Graph ===\n");
        printf("==========================================\n");
        printf("LEN               = %d \n", TP_LEN);
        printf("DIM               = %d \n", TP_DIM);
        printf("API               = %d \n", TP_API);
        printf("RND               = %d \n", TP_RND);
        printf("SAT               = %d \n", TP_SAT);
        printf("IS_OUTPUT_SQUARED = %d \n", TP_IS_OUTPUT_SQUARED);
        printf("===================================================\n");
        printf("== END of EUCLIDEAN_DISTANCE REF KERNEL Graph =====\n");
        printf("===================================================\n");
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
    };
};
} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {

#endif // _DSPLIB_EUCLIDEAN_DISTANCE_REF_GRAPH_HPP_
