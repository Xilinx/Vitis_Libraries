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
#include "cholesky_cascade_table.hpp"

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
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_GRID_DIM,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_CASC_IDX,
          unsigned int kDiagStart>
class cholesky_cascade_recur {
   public:
    static constexpr unsigned int kKernelDim = TP_DIM / TP_GRID_DIM;
    static constexpr unsigned int kNumDiagsSeenByXY = (TP_X+1) * kKernelDim;    // I think TP_X instead of TP_Y?
    static constexpr unsigned int rowIndex = TP_DIM - kDiagStart - 1;
    static constexpr unsigned int colIndex = TP_CASC_LEN - TP_CASC_IDX - 1;
    static constexpr unsigned int kNumStages = cholesky_cascade_stages_table[rowIndex][colIndex];
    static constexpr unsigned int kDiagEnd = kDiagStart + kNumStages;
    
    using cholesky_template = cholesky<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_DIAG_INV, TP_X, TP_Y, kDiagStart, kDiagEnd>;
    using cholesky_cascade_recur_template = cholesky_cascade_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, TP_DIAG_INV, TP_X, TP_Y, TP_CASC_IDX+1, kDiagEnd>;

    static void create( kernel  (&m_kernels)[TP_GRID_DIM][TP_GRID_DIM][TP_CASC_LEN], 
                        int     (&depthMap)[TP_GRID_DIM][TP_GRID_DIM],
                        int     (&activeGridOffsets)[TP_CASC_LEN]) {


        alignas(__ALIGN_BYTE_SIZE__) std::vector<TT_DATA> m_diagColBuffer(kKernelDim);  // Redundant if single tile.
        alignas(__ALIGN_BYTE_SIZE__) std::vector<TT_DATA> m_diagRowBuffer(kKernelDim);     

        activeGridOffsets[TP_CASC_IDX] = kDiagStart / kKernelDim;
        m_kernels[TP_X][TP_Y][TP_CASC_IDX] = kernel::create_object<cholesky_template>(m_diagColBuffer, m_diagRowBuffer);

        if constexpr(kDiagEnd < kNumDiagsSeenByXY) {
            cholesky_cascade_recur_template::create(m_kernels, depthMap, activeGridOffsets);
        } else {
            depthMap[TP_X][TP_Y] = TP_CASC_IDX + 1;  // The depth of the cascade at this XY position.
        }
    }
};

template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_GRID_DIM,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV,
          unsigned int TP_X,
          unsigned int TP_Y>
class cholesky_grid_recur {
   public:
    using cholesky_cascade_recur_entry = cholesky_cascade_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, TP_DIAG_INV, TP_X, TP_Y, 0, 0>;
    using cholesky_recur_right_template = cholesky_grid_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, TP_DIAG_INV, TP_X+1, TP_Y>;
    using cholesky_recur_next_row_template = cholesky_grid_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, TP_DIAG_INV, 0, TP_Y+1>;

    static void create( kernel  (&m_kernels)[TP_GRID_DIM][TP_GRID_DIM][TP_CASC_LEN], 
                        int     (&depthMap)[TP_GRID_DIM][TP_GRID_DIM],
                        int     (&activeGridOffsets)[TP_CASC_LEN]) {

        cholesky_cascade_recur_entry::create(m_kernels, depthMap, activeGridOffsets);

        if constexpr(TP_X < TP_Y) {
            cholesky_recur_right_template::create(m_kernels, depthMap, activeGridOffsets);
        }
        else if constexpr(TP_Y < TP_GRID_DIM-1) {
            cholesky_recur_next_row_template::create(m_kernels, depthMap, activeGridOffsets);
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
 * @tparam TP_CASC_LEN describes the depth of the cascade chain to increase throughput. 
 **/
template <typename TT_DATA,
          unsigned int TP_DIM,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_GRID_DIM,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIAG_INV = 0>

class cholesky_graph : public graph {
    public:
    /**
    * @cond NOCOMMENTS
    */
        static constexpr unsigned int kTotalMatrixSize = TP_DIM * TP_DIM;
        static constexpr unsigned int kKernelDim = TP_DIM / TP_GRID_DIM;
        static constexpr unsigned int kNumPortKernels = TP_GRID_DIM * (TP_GRID_DIM+1) / 2;
        static constexpr unsigned int kKernelMatrixSize = kKernelDim * kKernelDim;

        static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
            "ERROR: TP_DATA must be either float or cfloat.");
        static_assert(kKernelDim % fnVecSampleNum<TT_DATA>() == 0,
            "ERROR: kKernelDim must be a multiple of number of samples that can fit in a vector.");
        static_assert(kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) <= __DATA_MEM_BYTES__,
            "ERROR: kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) must be less than or equal to __DATA_MEM_BYTES__.");
        static_assert(TP_DIM % TP_GRID_DIM == 0,
            "ERROR: TP_DIM must exactly divide into TP_GRID_DIM.");
        static_assert(TP_NUM_FRAMES > 0,
            "ERROR: TP_NUM_FRAMES must be greater than 0.");
        static_assert(TP_CASC_LEN > 0,
            "ERROR: TP_CASC_LEN must be greater than 0.");
        static_assert(TP_CASC_LEN <= TP_DIM,
            "ERROR: TP_CASC_LEN must be less than or equal to TP_DIM.");
        static_assert(TP_DIAG_INV == 0 || TP_DIAG_INV == 1,
            "ERROR: TP_DIAG_INV must be either 0 or 1.");
    /**
    * @endcond
    */

    /**
     * The input matrix tile(s).
     **/
    xf::dsp::aie::port_array<input, kNumPortKernels> in;
    /**
     * The output matrix tile(s).
     **/
    xf::dsp::aie::port_array<output, kNumPortKernels> out;
    /**
     * The cascade depth map and 3D array of kernels that will be created and mapped onto AIE tiles.
     * Although we are initializing the full voxel grid of tiles, this is for ease of indexing.
     * The depth map indicates the depth of the cascade chain at a given X, Y coordinate.
     * 
     * Kernels are classified as one of 8 types, with associated XY interconnections:-
     *  DTL = Diagonal Top Left     (Incoming:          Outgoing: Down      )
     *  DM  = Diagonal Middle       (Incoming: Left,    Outgoing: Down      )
     *  DBR = Diagonal Bottom Right (Incoming: Left,    Outgoing:           )
     *  LLE = Lower Left Edge       (Incoming: Up,      Outgoing: Right,Down)
     *  LBL = Lower Bottom Left     (Incoming: Up,      Outgoing: Right     )
     *  LBE = Lower Bottom Edge     (Incoming: Left,Up  Outgoing: Right     )
     *  LNE = Lower None Edge       (Incoming: Left,Up  Outgoing: Right,Down)
     *  IT  = Isolated Tile         (Incoming:          Outgoing:           )
     *  
     *          + ____  ____  ----------+    ↑ 
     *        / /____ /____ /|        / |    | 
     *      /  | DTL | DTL | |      /   |    | 
     *     +-- |_____|_____|/|___ -+    |    | 
     *     |  /____ /____ /____ /| |    |    | TP_GRID_DIM (Y-axis)   
     *     | | DM  | DM  | DTL | | |    |    | 
     *     | |_____|_____|_____|/|_|_   |    | 
     *     |/____ /____ /____ /____|/|--+  ↑ ↓       
     *     ‖ DBR | DBR | DBR | IT  ‖ | /  /         
     *     ‖_____|_____|_____|_____‖//  /  TP_GRID_DIM (X-axis)   
     *     +-----------------------+  ↓  
     *     ←-----------------------→ 
     *        TP_CASC_LEN (Z-axis)  
     **/
    kernel m_kernels[TP_GRID_DIM][TP_GRID_DIM][TP_CASC_LEN];
    int depthMap[TP_GRID_DIM][TP_GRID_DIM];
    int activeGridOffsets[TP_CASC_LEN];
    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return m_kernels; };

    /**
     * @brief This is the constructor function for the cholesky graph.
     **/
    cholesky_graph() {
        
        cholesky_grid_recur<TT_DATA, TP_DIM, TP_NUM_FRAMES, TP_GRID_DIM, TP_CASC_LEN, TP_DIAG_INV, 0, 0>::create(m_kernels, depthMap, activeGridOffsets);

        int portIdx = 0;
        for (int y = 0; y < TP_GRID_DIM; y++) {
            for (int x = 0; x < y+1; x++) {

                connect(in[portIdx], m_kernels[x][y][0].in[0]); // cascade entry

                int cascadeDepth = depthMap[x][y];
                for (int z = 0; z < cascadeDepth; z++) {

                    // Specify mapping constraints
                    runtime<ratio>(m_kernels[x][y][z]) = 0.9;     // Nominal figure. Requires knowledge of sample rate.

                    // Source files
                    source(m_kernels[x][y][z]) = "cholesky.cpp";
                    stack_size(m_kernels[x][y][z]) = 4096;  // Empirically seen cases which utilise ~2560 bytes

                    dimensions(m_kernels[x][y][z].in[0]) = {kKernelMatrixSize * TP_NUM_FRAMES};
                    dimensions(m_kernels[x][y][z].out[0]) = {kKernelMatrixSize * TP_NUM_FRAMES};
                    
                    if (kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) > __DATA_MEM_BYTES__ / 2) {
                        single_buffer(m_kernels[x][y][z].in[0]);
                        single_buffer(m_kernels[x][y][z].out[0]);
                    }

                    if (z + 1 < cascadeDepth) {
                        connect(m_kernels[x][y][z].out[0], m_kernels[x][y][z+1].in[0]);
                    }

                    if (x < y) {
                        connect<HORIZONTAL_PORT>(m_kernels[x][y][z].out[1], m_kernels[x+1][y][z].in[1]);

                        if (x == activeGridOffsets[z]) {
                            connect<VERTICAL_PORT>(m_kernels[x][x][z].out[1], m_kernels[x][y][z].in[1]);
                        } else {
                            connect<VERTICAL_PORT>(m_kernels[x][x][z].out[1], m_kernels[x][y][z].in[2]);
                        }     
                    }
                }
                connect(m_kernels[x][y][cascadeDepth-1].out[0], out[portIdx]);  // cascade exit
                portIdx++;
            }
        }
    }; // constructor
};
}
}
}
} // namespace braces

#endif //_DSPLIB_CHOLESKY_GRAPH_HPP_
