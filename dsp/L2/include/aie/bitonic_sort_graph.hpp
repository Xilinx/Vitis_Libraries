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
#include "merge_sort.hpp"
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
        m_kernels[TP_CASC_IDX] =
            kernel::create_object<bitonic_sort_template>(); // Create bitonic kernel and add it to idx position
                                                            // TP_CASC_IDX.
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
 * @tparam TP_SSR describes the number of bitonic_sort tiles that will be placed in parallel to split the list. \n
 *         The resulting kernels will independently split TP_SSR sub-lists, which will then be passed through a tree of
 *         merge sort kernels.
 *         This allows support for greater list sizes than a single kernel (by a factor of TP_SSR) and performance
 *         improvements can be observed on larger sizes of TP_DIM. \n
 *         The design will contain TP_SSR bitonic_kernels plus (TP_SSR - 1) merge_sort kernels.
 *         Connections between the bitonic_sort and merge_sort kernels are done internally.
 *         The bitonic_sort graph will have TP_SSR inputs (via iobuffer) and a single streaming output.
 * @tparam TP_INDEX
 *         This parameter is for internal use regarding the recursion of the parallel power feature. \n
 *         It is recommended
 *         to miss this parameter from the configuration and rely instead on default values. If this parameter is set
 *         by the user, the behaviour of the library unit is undefined.
 **/
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN,
          unsigned int TP_SSR = 1,
          unsigned int TP_INDEX = 0>
class bitonic_sort_graph : public graph {
   public:
    static constexpr unsigned int kKernelSize = TP_DIM;
    static constexpr int kStreamsPerTile = get_input_streams_core_module(); // a device trait
    // Use dual streams in if available (AIE1). Else use iobuffers for merge kernels connected directly to bitonic_sort
    // kernels outputs. The next "level" of merge_sort kernels will use one stream and one cascade in
    static constexpr unsigned int TP_IN_API =
        (kStreamsPerTile == 2) ? kStreamAPI : (TP_SSR == 2) ? kWindowAPI : kStreamCascAPI;
    // If two streams available, each merge_sort out can be stream. Else alternate between stream and cascade. TP_INDEX
    // / TP_SSR % 2 == 0 for all desired stream inputs. False for cascades
    static constexpr unsigned int TP_OUT_API =
        (kStreamsPerTile == 2) ? kStreamAPI : (TP_INDEX / TP_SSR % 2 == 0) ? kStreamAPI : kCascAPI;

    static_assert(fnLog2<kKernelSize * sizeof(TT_DATA) * TP_NUM_FRAMES>() != -1,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be a power of 2.");
    // TODO: Please fix static_assert. Checks against __MAX_READ_WRITE__, but underlying vector is fixed at 256-bits).
    // Reverting to using const.
    static_assert(kKernelSize * sizeof(TT_DATA) * TP_NUM_FRAMES >= 2 * 256 / 8,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be greater than or equal to 64 bytes.");
    static_assert(kKernelSize * sizeof(TT_DATA) * TP_NUM_FRAMES <= __DATA_MEM_BYTES__,
                  "ERROR: TP_DIM * sizeof(TT_DATA) * TP_NUM_FRAMES must be less than or equal to I/O buffer bytes.");
    static_assert(TP_ASCENDING == 0 || TP_ASCENDING == 1,
                  "ERROR: TP_ASCENDING must be 0 (descending) or (1) ascending.");
    static_assert(
        TP_CASC_LEN <= getNumStages<TP_DIM>(),
        "ERROR: TP_CASC_LEN must be less or equal to the number of bitonic stages (log2(TP_DIM)+1)*log2(TP_DIM)/2.");
    static_assert(TP_NUM_FRAMES == 1 || TP_SSR == 1, "ERROR: TP_NUM_FRAMES > 1 is only supported for TP_SSR = 1");
    /**
     * The input data to be sorted.
     **/
    port_array<input, TP_SSR> in;
    /**
     * The sorted output data.
     **/
    port_array<output, 1> out;
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     **/
    kernel m_kernels[TP_CASC_LEN];
    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return m_kernels; };

    kernel merge_sorter;
    /**
     * @brief This is the constructor function for the bitonic_sort graph.
     **/

    bitonic_sort_graph<TT_DATA, TP_DIM / 2, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, TP_SSR / 2, TP_INDEX>
        bitonic_merge_subframe0;
    bitonic_sort_graph<TT_DATA,
                       TP_DIM / 2,
                       TP_NUM_FRAMES,
                       TP_ASCENDING,
                       TP_CASC_LEN,
                       TP_SSR / 2,
                       TP_INDEX + (TP_SSR / 2)>
        bitonic_merge_subframe1;

    using mergerClass = merge_sort<TT_DATA, TP_IN_API, TP_OUT_API, TP_DIM, TP_ASCENDING>;
    bitonic_sort_graph() {
        merge_sorter = kernel::create_object<mergerClass>();
        // connect inputs - first half to subframe0, second half to subframe1
        for (int i = 0; i < TP_SSR / 2; i++) {
            connect(in[2 * i], bitonic_merge_subframe0.in[i]);
            connect(in[2 * i + 1], bitonic_merge_subframe1.in[i]);
        }
        if (TP_IN_API == kStreamAPI) {
            connect<stream>(bitonic_merge_subframe0.out[0], merge_sorter.in[0]);
            connect<stream>(bitonic_merge_subframe1.out[0], merge_sorter.in[1]);
        } else if (TP_IN_API == kWindowAPI) {
            // This is the first layer of merge sorts. Bitonic out connects to merge in, thus merge sort input can be
            // iobuffer
            connect(bitonic_merge_subframe0.out[0], merge_sorter.in[0]);
            dimensions(merge_sorter.in[0]) = {TP_DIM / 2};
            connect(bitonic_merge_subframe1.out[0], merge_sorter.in[1]);
            dimensions(merge_sorter.in[1]) = {TP_DIM / 2};
        } else { // kStreamCascAPI - streams to in[0] and casc to in[1]
            connect<stream>(bitonic_merge_subframe0.out[0], merge_sorter.in[0]);
            connect<cascade>(bitonic_merge_subframe1.out[0], merge_sorter.in[1]);
        }
        if
            constexpr(TP_OUT_API == kStreamAPI) {
                // output connections - connect merge out to out
                connect<stream>(merge_sorter.out[0], out[0]);
            }
        else {
            connect<cascade>(merge_sorter.out[0], out[0]);
        }

        runtime<ratio>(merge_sorter) = 0.9;
        source(merge_sorter) = "merge_sort.cpp";
    }; // constructor
};
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_ASCENDING,
          unsigned int TP_CASC_LEN,
          //   unsigned int TP_SSR,
          unsigned int TP_INDEX>
class bitonic_sort_graph<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, 1, TP_INDEX> : public graph {
   public:
    static constexpr unsigned int TP_SSR = 1;
    /**
    * Input to bitonic kernel
    **/
    port_array<input, 1> in;
    /**
    * Output to bitonic kernel
    **/
    port_array<output, 1> out;
    /**
    * The array of kernels that will be created and mapped onto AIE tiles.
    **/
    kernel m_kernels[TP_CASC_LEN];
    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return m_kernels; };

    bitonic_sort_graph() {
        bitonic_sort_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_ASCENDING, TP_CASC_LEN, 0>::create(m_kernels);

        for (int k = 0; k < TP_CASC_LEN; k++) {
            int kernelIdx = k;
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

            dimensions(m_kernels[k].in[0]) = {TP_DIM * TP_NUM_FRAMES};
            dimensions(m_kernels[k].out[0]) = {TP_DIM * TP_NUM_FRAMES};
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_BITONIC_SORT_GRAPH_HPP_
