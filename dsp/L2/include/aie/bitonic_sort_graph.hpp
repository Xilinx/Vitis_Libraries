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
#ifndef _DSPLIB_BITONIC_SORT_GRAPH_HPP_
#define _DSPLIB_BITONIC_SORT_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Bitonic Sort library element.
*/
/**
 * @file bitonic_sort_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "bitonic_sort.hpp"
#include "bitonic_sort_traits.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace bitonic_sort {
using namespace adf;

/**
  * @cond NOCOMMENTS
  */

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN,
          unsigned int TP_CASC_IDX>
class bitonic_sort_recur {
   public:
    using bitonic_sort_template = bitonic_sort<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, TP_CASC_IDX>;
    using bitonic_sort_recur_template =
        bitonic_sort_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, TP_CASC_IDX + 1>;

    static void create(kernel (&m_kernels)[TP_CASC_LEN]) {
        m_kernels[TP_CASC_IDX] = kernel::create_object<bitonic_sort_template>(); // Create bitonic kernel and add it to
                                                                                 // idx position TP_CASC_IDX.
        if
            constexpr(TP_CASC_IDX != TP_CASC_LEN - 1) {
                bitonic_sort_recur_template::create(m_kernels);
            } // If not the base case, recursively call with TP_CASC_IDX+1.
    }
};

/**
  * @endcond
  */

/**
 * @defgroup bitonic_sort Bitonic Sort
 *
 * Bitonic Sort
 *
 */

//--------------------------------------------------------------------------------------------------
// bitonic_sort_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup bitonic_sort
 * @brief Bitonic Sort is a parallel sorting algorithm with an asymptotic complexity of log2(n)*(log2(n)+1)/2 \n
 *        (assuming all the operations in each stage occur in one time step).
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following: \n
 *         int16, uint16 (not available on AIE1), int32, float.
 * @tparam TP_DIM describes the number of samples in the list.
 * @tparam TP_NUM_FRAMES describes the number of lists to sort per call to the kernel.
 * @tparam TP_ASCENDING describes whether to sort the list in descending (0) or ascending (1) order.
 * @tparam TP_CASC_LEN describes the number of tiles to cascade computation across to increase throughput. \n
 **/
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN>
class bitonic_sort_graph : public graph {
   public:
    static constexpr unsigned int kKernelSize = TP_DIM;

    static_assert(fnLog2<kKernelSize * sizeof(TT_DATA) * TP_NUM_FRAMES>() != -1,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be a power of 2.");
    static_assert(kKernelSize * sizeof(TT_DATA) * TP_NUM_FRAMES >= 2 * __MAX_READ_WRITE__ / 8,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be greater than or equal to 64 bytes.");
    static_assert(kKernelSize * sizeof(TT_DATA) * TP_NUM_FRAMES <= __DATA_MEM_BYTES__,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be less than or equal to ping-pong size.");
    static_assert(TP_ASCENDING == 0 || TP_ASCENDING == 1,
                  "ERROR: TP_ASCENDING must be 0 (descending) or (1) ascending.");
    static_assert(
        TP_CASC_LEN <= getNumStages<TP_DIM>(),
        "ERROR: TP_CASC_LEN must be less or equal to the number of bitonic stages (log2(TP_DIM)+1)*log2(TP_DIM)/2.");

    /**
     * The input data to be sorted.
     **/
    port_array<input, 1> in;
    /**
     * The sorted output data.
     **/
    port_array<output, 1> out;
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     **/
    kernel m_kernels[TP_CASC_LEN];
    /**
     * @brief This is the constructor function for the bitonic_sort graph.
     **/
    bitonic_sort_graph() {
        printf("Graph constructor...\n");
        printf("kKernelSize = %d\n", kKernelSize);
        printf("TP_DIM = %d\n", TP_DIM);
        printf("TP_NUM_FRAMES = %d\n", TP_NUM_FRAMES);
        printf("TP_ASCENDING = %d\n", TP_ASCENDING);
        printf("TP_CASC_LEN = %d\n", TP_CASC_LEN);
        printf("Graph constructor...\n");

        bitonic_sort_recur<TT_DATA, kKernelSize, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, 0>::create(m_kernels);

        for (int k = 0; k < TP_CASC_LEN; k++) {
            // Specify mapping constraints
            runtime<ratio>(m_kernels[k]) = 0.9; // Nominal figure. Requires knowledge of sample rate.

            // Source files
            source(m_kernels[k]) = "bitonic_sort.cpp";
            stack_size(m_kernels[k]) = 2048;

            // make connections
            if (k == 0) {
                connect(in[0], m_kernels[k].in[0]);
            } else {
                connect(m_kernels[k - 1].out[0], m_kernels[k].in[0]);
            }

            if (k == TP_CASC_LEN - 1) {
                connect(m_kernels[k].out[0], out[0]);
            }

            dimensions(m_kernels[k].in[0]) = {kKernelSize * TP_NUM_FRAMES};
            dimensions(m_kernels[k].out[0]) = {kKernelSize * TP_NUM_FRAMES};
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_BITONIC_SORT_GRAPH_HPP_
