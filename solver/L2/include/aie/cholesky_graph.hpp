/*
 * Copyright (C) 2025, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_CHOLESKY_GRAPH_HPP_
#define _DSPLIB_CHOLESKY_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the Cholesky library element.
*/
/**
 * @file cholesky_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "cholesky.hpp"
#include "cholesky_traits.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace cholesky {
using namespace adf;

#if (__CASCADES_PER_TILE__ == 2)
#define HORIZONTAL_PORT cascade
#define VERTICAL_PORT stream
#elif (__STREAMS_PER_TILE__ == 2)
#define HORIZONTAL_PORT stream
#define VERTICAL_PORT stream
#endif

/**
  * @cond NOCOMMENTS
  */

template <typename TT_DATA,
          unsigned int kKernelDim,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_GRID_DIM,
          unsigned int kNumKernels,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int currTileIdx>
class cholesky_recur {
   public:
    using cholesky_template = cholesky<TT_DATA, kKernelDim, TP_NUM_FRAMES, TP_X, TP_Y, TP_GRID_DIM>;
    using cholesky_recur_right_template = cholesky_recur<TT_DATA, kKernelDim, TP_NUM_FRAMES, TP_GRID_DIM, kNumKernels, TP_X+1, TP_Y, currTileIdx+1>;
    using cholesky_recur_next_row_template = cholesky_recur<TT_DATA, kKernelDim, TP_NUM_FRAMES, TP_GRID_DIM, kNumKernels, 0, TP_Y+1, currTileIdx+1>;

    static void create(kernel (&m_kernels)[kNumKernels]) {
        alignas(__ALIGN_BYTE_SIZE__) std::vector<TT_DATA> m_diagColBuffer(kKernelDim);  // Redundant if single tile.
        alignas(__ALIGN_BYTE_SIZE__) std::vector<TT_DATA> m_diagRowBuffer(kKernelDim);  

        m_kernels[currTileIdx] = kernel::create_object<cholesky_template>(m_diagColBuffer, m_diagRowBuffer);

        if constexpr(TP_X < TP_Y) {
            cholesky_recur_right_template::create(m_kernels);
        }
        else if constexpr(TP_Y < TP_GRID_DIM-1) {
            cholesky_recur_next_row_template::create(m_kernels);
        }
    }
};

/**
  * @endcond
  */

/**
 * @defgroup cholesky Cholesky Decomposition
 *
 * Cholesky Decomposition
 *
 */

//--------------------------------------------------------------------------------------------------
// cholesky_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup cholesky
 * @brief The Cholesky Decomposition is a specialization of the LU Decomposition. It decomposes a
 *        Hermitian, positive-definite matrix into the product of a lower triangular matrix and its
 *        conjugate transpose.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following: \n
 *         float, cfloat.
 * @tparam TP_DIM describes the length of one dimension of the matrix, in samples.
 * @tparam TP_NUM_FRAMES describes the number of matrices to sort per call to the kernel.
 * @tparam TP_GRID_DIM describes the length of one dimension of the tiling scheme used to divide up the input matrix. \n
 *         This allows support for larger matrices than can fit on a single kernel. \n
 *         The number of kernels is TP_GRID_DIM * (TP_GRID_DIM+1) / 2. \n
 **/
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_GRID_DIM>

class cholesky_graph : public graph {
    public:
    /**
    * @cond NOCOMMENTS
    */
        static constexpr unsigned int kTotalMatrixSize = TP_DIM * TP_DIM;
        static constexpr unsigned int kNumKernels = TP_GRID_DIM * (TP_GRID_DIM+1) / 2;
        static constexpr unsigned int kKernelDim = TP_DIM / TP_GRID_DIM;
        static constexpr unsigned int kKernelMatrixSize = kKernelDim * kKernelDim;

        static_assert(kKernelDim % fnVecSampleNum<TT_DATA>() == 0,
            "ERROR: kKernelDim must be a multiple of number of samples that can fit in a vector.");
        static_assert(kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) <= __DATA_MEM_BYTES__ / 2,
            "ERROR: kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) must be less than or equal to __DATA_MEM_BYTES__ / 2.");
        static_assert(TP_DIM % TP_GRID_DIM == 0,
            "ERROR: TP_DIM must exactly divide into TP_GRID_DIM.");
    /**
    * @endcond
    */

    /**
     * The input matrix tile(s).
     **/
    xf::dsp::aie::port_array<input, kNumKernels> in;
    /**
     * The output matrix tile(s).
     **/
    xf::dsp::aie::port_array<output, kNumKernels> out;
    /**
     * The array of kernels that will be created and mapped onto AIE tiles.
     **/
    kernel m_kernels[kNumKernels];
    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return m_kernels; };

    /**
     * @brief This is the constructor function for the cholesky graph.
     **/
    cholesky_graph() {
        
        cholesky_recur<TT_DATA, kKernelDim, TP_NUM_FRAMES, TP_GRID_DIM, kNumKernels, 0, 0, 0>::create(m_kernels);

        int currTileIdx = 0;
        for (int y = 0; y < TP_GRID_DIM; y++) {
            for (int x = 0; x < y+1; x++) {


                // Specify mapping constraints
                runtime<ratio>(m_kernels[currTileIdx]) = 0.9;     // Nominal figure. Requires knowledge of sample rate.

                // Source files
                source(m_kernels[currTileIdx]) = "cholesky.cpp";
                stack_size(m_kernels[currTileIdx]) = 4096;  // Empirically seen cases which utilise ~2560 bytes

                // Make connections
                connect(in[currTileIdx], m_kernels[currTileIdx].in[0]);
                connect(m_kernels[currTileIdx].out[0], out[currTileIdx]);
                dimensions(m_kernels[currTileIdx].in[0]) = {kKernelMatrixSize * TP_NUM_FRAMES};
                dimensions(m_kernels[currTileIdx].out[0]) = {kKernelMatrixSize * TP_NUM_FRAMES};

                int outPortIdx = 1;
                if (x < y) {
                    int rightTileIdx = currTileIdx + 1;
                    connect<HORIZONTAL_PORT>(m_kernels[currTileIdx].out[outPortIdx], m_kernels[rightTileIdx].in[outPortIdx]);
                    outPortIdx++;
                }
                if (y < TP_GRID_DIM-1) {
                    int downTileIdx = currTileIdx + y + 1;
                    if (x == 0) {
                        connect<VERTICAL_PORT>(m_kernels[currTileIdx].out[outPortIdx], m_kernels[downTileIdx].in[1]);
                    } else {
                        connect<VERTICAL_PORT>(m_kernels[currTileIdx].out[outPortIdx], m_kernels[downTileIdx].in[2]);
                    }          
                }
                currTileIdx++;
            }
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_CHOLESKY_GRAPH_HPP_
