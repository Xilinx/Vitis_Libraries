/*
 * Copyright (C) 2026, Advanced Micro Devices, Inc.
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
#ifndef _SOLVERLIB_SUBSTITUTION_GRAPH_HPP_
#define _SOLVERLIB_SUBSTITUTION_GRAPH_HPP_

/*
The file captures the definition of the 'L2' graph level class for
the Forward/Backward Substitution library element.
*/
/**
 * @file substitution_graph.hpp
 *
 **/

#include <adf.h>
#include "graph_utils.hpp"
#include "substitution.hpp"
#include "castKernel.hpp"
using namespace ::xf::dsp::aie; //for graph_utils port_array

namespace xf::solver::aie::substitution {

using namespace adf;
using namespace xf::solver::aie::kernel_cast;

  /**
  * @cond NOCOMMENTS
  */

template <typename TT_DATA,
          unsigned int T_KERNEL_DIM,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING,
          unsigned int T_NUM_KERNELS,
          unsigned int TP_X,
          unsigned int TP_Y,
          unsigned int TP_GRID_DIM,
          unsigned int T_TILE_IDX,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_DIAG_INV>
class substitution_recur {
  public:

  //a buffer descriptor (DMA) is used to perform a transpose before data arrives at the IP.
  //However, on AIE1, the buffer descriptor can only handle 2 dimensions, but one of those dimensions is used for cfloat
  //(real and imag are considered a dimension). Therefore, for cfloat on AIE1, the data type in the kernel iobuffer, and therefore
  //inferred by the buffer descriptor, is set to float. This results in the transform being incorrect, but in a known incorrect pattern
  //which the kernel can fix before proceeding to process the data.
  static constexpr bool kUsingSpoofTranspose  = (__MAX_BD_DSIZE_TPOSE__ == 4) &&   // i.e. AIE1
    (std::is_same<TT_DATA,cfloat>::value) &&  // and cfloat
    ((TP_L_LEADING == 0 && TP_SUBST_TYPE == 0) || (TP_L_LEADING == 1 && TP_SUBST_TYPE == 1))  ;                      // and transpose required
  typedef typename std::conditional<kUsingSpoofTranspose, float, TT_DATA>::type TT_IO_DATA;
  //using TT_IO_DATA = kUsingSpoofTranspose ? float: TT_DATA;
  using substitution_template = substitution<TT_DATA, T_KERNEL_DIM, TP_SUBST_TYPE, TP_L_LEADING, TP_X, TP_Y, TP_GRID_DIM, TP_NUM_FRAMES, TP_DIAG_INV, TT_IO_DATA>;
  using substitution_recur_right_template    = substitution_recur<TT_DATA, T_KERNEL_DIM, TP_SUBST_TYPE, TP_L_LEADING, T_NUM_KERNELS, TP_X+1, TP_Y,   TP_GRID_DIM, T_TILE_IDX+1, TP_NUM_FRAMES, TP_DIAG_INV>;
  using substitution_recur_next_row_template = substitution_recur<TT_DATA, T_KERNEL_DIM, TP_SUBST_TYPE, TP_L_LEADING, T_NUM_KERNELS, 0,      TP_Y+1, TP_GRID_DIM, T_TILE_IDX+1, TP_NUM_FRAMES, TP_DIAG_INV>;

  static void create(kernel (&m_kernels)[T_NUM_KERNELS], kernel (&m_castKernel_kernels)[T_NUM_KERNELS]) {

      m_kernels[T_TILE_IDX] = kernel::create_object<substitution_template>();
      if (kUsingSpoofTranspose) {
        m_castKernel_kernels[T_TILE_IDX] = kernel::create_object<castKernel<T_KERNEL_DIM, TP_NUM_FRAMES>>();
      }

      if constexpr(TP_X < TP_Y) {
          substitution_recur_right_template::create(m_kernels,m_castKernel_kernels);
      }
      else if constexpr(TP_Y < TP_GRID_DIM-1) {
          substitution_recur_next_row_template::create(m_kernels,m_castKernel_kernels);
      }
  }
};

/**
  * @endcond
  */

/**
 * @defgroup substitution Forward Backward Substitution
 *
 * Forward Backward Substitution
 *
 */

//--------------------------------------------------------------------------------------------------
// substitution_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup substitution
 * @brief This class encapsulates a Forward or Backward Substitution function.
 *        When provided an input lower or upper triangular matrix L and a vector of the same length B
 *        It will solve Lx = b for x.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function.
 *         This is a typename and must be one of the following: \n
 *         float, cfloat.
 * @tparam TP_DIM describes the length of one dimension of the matrix, in samples.
 * @tparam TP_SUBST_TYPE describes the direction of substitution: forward or backward. For forward,
 *         matrix L must be a lower triangular matrix, whereas for backward substitution, L must be
 *         an upper triangular matrix.
 * @tparam TP_L_LEADING describes the memory arrangement of elements of L. When set to 0 the input
 *         must be row-major. When set to 1 the input must be column-major.
 * @tparam TP_GRID_DIM describes the length of one dimension of the tiling scheme used to divide up the input matrix. \n
 *         This allows support for larger matrices than can fit on a single kernel. \n
 *         The number of kernels is TP_GRID_DIM * (TP_GRID_DIM+1) / 2. \n
 * @tparam TP_NUM_FRAMES describes the number of matrices to perform substitution on per call to this function. \n
 *         This means kernel overheads apply only once per TP_NUM_FRAMES and so can lead to higher performance. \n
 * @tparam TP_DIAG_INV instructs this function to operate on a matrix conventionally, or on a matrix with \n
 *         pre-inverted (reciprocal) values on the diagonal elements. This is an optimization in conjunction with \n
 *         the Cholesky function which gives higher performance for the solution of a set of linear equations\n
 **/
template <typename TT_DATA,
          unsigned int TP_DIM_SIZE,
          unsigned int TP_SUBST_TYPE,
          unsigned int TP_L_LEADING = 0,
          unsigned int TP_GRID_DIM = 1,
          unsigned int TP_NUM_FRAMES = 1,
          unsigned int TP_DIAG_INV = 0>
class substitution_graph : public adf::graph {
   public:
    /**
    * @cond NOCOMMENTS
    */
    static constexpr unsigned int kNumKernels = TP_GRID_DIM*(TP_GRID_DIM+1)/2;
    static constexpr unsigned int kKernelDim = TP_DIM_SIZE / TP_GRID_DIM;
    static constexpr unsigned int kKernelMatrixSize = kKernelDim * kKernelDim;
    static constexpr bool kUsingSpoofTranspose =
        (__MAX_BD_DSIZE_TPOSE__ == 4) &&
        (std::is_same<TT_DATA, cfloat>::value) &&
        ((TP_L_LEADING == 0 && TP_SUBST_TYPE == 0) || (TP_L_LEADING == 1 && TP_SUBST_TYPE == 1));

    static_assert(std::is_same<TT_DATA, float>::value || std::is_same<TT_DATA, cfloat>::value,
      "ERROR: TP_DATA must be either float or cfloat.");
    static_assert(kKernelDim % fnVecSampleNumMax<TT_DATA>() == 0,
        "ERROR: kKernelDim must be a multiple of number of samples that can fit in a vector.");
    static_assert(kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) <= __DATA_MEM_BYTES__,
        "ERROR: kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA) must be less than or equal to __DATA_MEM_BYTES__.");
    static_assert(TP_DIM_SIZE % TP_GRID_DIM == 0,
        "ERROR: TP_DIM_SIZE must exactly divide into TP_GRID_DIM.");
    static_assert(TP_NUM_FRAMES > 0,
        "ERROR: TP_NUM_FRAMES must be greater than 0.");
    static_assert(TP_NUM_FRAMES <= 4,
        "ERROR: TP_NUM_FRAMES must be less than 4."); // max num frames due to transpose hardware constraints.
    static_assert(TP_DIAG_INV == 0 || TP_DIAG_INV == 1,
        "ERROR: TP_DIAG_INV must be either 0 or 1.");
    static_assert(TP_SUBST_TYPE == 0 || TP_SUBST_TYPE == 1,
      "ERROR: TP_SUBST_TYPE must be either 0 (forward) or 1 (backward).");
    static_assert(TP_L_LEADING == 0 || TP_L_LEADING == 1,
        "ERROR: TP_L_LEADING must be either 0 (row-major) or 1 (column-major).");

    /**
    * @endcond
    */

    // Input and output ports
    /**
     * The input matrix L in Lx = y.
     **/
    port_array<adf::input, kNumKernels> L_in;
    /**
     * The input vector y in Lx = y.
     **/
    port_array<adf::input, kNumKernels> y_in;
    /**
     * The output vector x in Lx = y.
     **/
    port_array<adf::output, TP_GRID_DIM> x_out;

    // Kernel instance
    kernel m_kernels[kNumKernels];
    kernel m_castKernel_kernels[kNumKernels];

    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel* getKernels() { return m_kernels; };

    /**
     * @brief This is the constructor function for the substitution graph.
     **/
    substitution_graph() {

        //create the kernels
        substitution_recur<TT_DATA, kKernelDim, TP_SUBST_TYPE, TP_L_LEADING, kNumKernels, 0 /*T_X*/, 0/*T_Y*/, TP_GRID_DIM, 0/*tileIdx*/, TP_NUM_FRAMES, TP_DIAG_INV>::create(m_kernels, m_castKernel_kernels);


        //-----------------------------
        //Connections
        //The connections of the substitution core are best understood by seeing a diagram.
        //Verbally: a lower triangle of kernels is created
        //L (or a chunk of L) and a vector y (or a splice of y) are connected to the input of each kernel.
        //x output is an iobuffer output from each diagonal kernel.
        //x output as a stream is also broadcast from each diagonal kernel to all kernels in the column below.
        //pp (partial product) of each non-diagonal kernel is cascaded to the right.

        unsigned L_port_map[kNumKernels];
        if constexpr (TP_SUBST_TYPE == 0) { //forward
            for (int i=0; i<kNumKernels; i++) {
                L_port_map[i] = i;
            }
        } else { //backward
            int fwd_idx = 0;
            for (int y = 0; y < TP_GRID_DIM; y++) {
                for (int x = 0; x < y + 1; x++) {
                    int bwd_y = TP_GRID_DIM - 1 - x;
                    int bwd_x = bwd_y - (y-x);
                    int bwd_idx = bwd_y * (bwd_y + 1) / 2 + bwd_x;
                    L_port_map[fwd_idx] = bwd_idx;
                    fwd_idx++;
                }
            }
        }

        int currTileIdx = 0;
        for (int y = 0; y < TP_GRID_DIM; y++) {
          for (int x = 0; x < y+1; x++) {

            //Kernel inputs are : L (index 0), y(1) and for internal connections, pp(2)
            //Kernel outputs are : x (index 0 - but only for diagonal kernels) and, for internal connections pp (0 or 1)

            // Specify source file

            // L connections
            if ((TP_L_LEADING == 0 && TP_SUBST_TYPE == 0) || (TP_L_LEADING == 1 && TP_SUBST_TYPE == 1) ) {
              //TP_L_LEADING == 0 means row-major, but the forward kernel algorithm is column-major, so a transpose is required here, but not when TP_L_LEADING == 1
              //insert buffer descriptor here for transpose.
              //For backward substitution, the matrix expected is L, but the algo works on L^H so transpose is required before the kernel see it.

              //This next line takes some explanation
              //For AIE1 cfloat transpose is not supported, so the iobuffer is defined as float.
              //This makes each frame in the iobuffer an TP_DIM_SIZE*2,TP_DIM_SIZE array rather than TP_DIM_SIZE*TP_DIM_SIZE,
              //So the spoofFactor is used to modify the buffer descriptor.
              constexpr int kSpoofFactor = kUsingSpoofTranspose ? 2 : 1;
              write_access(m_kernels[currTileIdx].in[0]) =
              /*
              adf::tiling({
                           .buffer_dimension = {kKernelDim, kKernelDim*kSpoofFactor, TP_NUM_FRAMES},
                           .tiling_dimension = {1, kKernelDim*kSpoofFactor, 1},
                            .offset = {0, 0, 0},
                            .tile_traversal = {
                                               {.dimension = 0, .stride = 1, .wrap = kKernelDim},
                                               {.dimension = 1, .stride = kKernelDim, .wrap = 1},
                                               {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                                              }
                         });
                         */
              adf::tiling({
                            .buffer_dimension = {kKernelDim* kSpoofFactor , kKernelDim, TP_NUM_FRAMES},
                            .tiling_dimension = {1, 1, 1},
                            .offset = {0, 0, 0},
                            .tile_traversal = {
                                               {.dimension = 1, .stride = 1, .wrap = kKernelDim},
                                               {.dimension = 0, .stride = 1, .wrap = kKernelDim* kSpoofFactor},
                                               {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                                              }
                         });

              if constexpr (kUsingSpoofTranspose) {
                connect(L_in[L_port_map[currTileIdx]], m_castKernel_kernels[currTileIdx].in[0]);
                dimensions(m_castKernel_kernels[currTileIdx].in[0]) = {TP_NUM_FRAMES * kKernelDim * kKernelDim};
                connect(m_castKernel_kernels[currTileIdx].out[0], m_kernels[currTileIdx].in[0]);
                dimensions(m_castKernel_kernels[currTileIdx].out[0]) = {TP_NUM_FRAMES * kKernelDim * kKernelDim * kSpoofFactor};
              } else {
                connect(L_in[L_port_map[currTileIdx]], m_kernels[currTileIdx].in[0]);
              }
              dimensions(m_kernels[currTileIdx].in[0]) = {TP_NUM_FRAMES * kKernelDim * kKernelDim * kSpoofFactor}; // Matrix L_in

            } else {
              connect(L_in[L_port_map[currTileIdx]], m_kernels[currTileIdx].in[0]);
              dimensions(m_kernels[currTileIdx].in[0]) = {TP_NUM_FRAMES * kKernelDim * kKernelDim}; // Matrix L_in
            }

            // Y connections
            connect(y_in[L_port_map[currTileIdx]], m_kernels[currTileIdx].in[1]);
            dimensions(m_kernels[currTileIdx].in[1]) = {TP_NUM_FRAMES * kKernelDim};         // Vector y_in

            // X (output) connections
            if  (x == y) {
              connect(m_kernels[L_port_map[currTileIdx]].out[0], x_out[y]);
              dimensions(m_kernels[currTileIdx].out[0]) = {TP_NUM_FRAMES * kKernelDim};         // Vector y_in
            }

            // pp connections (cascade)
            if (x<y) {
              if (x+1==y) { //connecting to last
                connect<cascade>(m_kernels[currTileIdx].out[0], m_kernels[currTileIdx+1].in[2]);
              } else {
                connect<cascade>(m_kernels[currTileIdx].out[0], m_kernels[currTileIdx+1].in[3]);
              }
            }

            // Specify runtime ratio, associate source code and stack size
            runtime<ratio>(m_kernels[currTileIdx]) = 0.9;
            source(m_kernels[currTileIdx]) = "substitution.cpp";
            stack_size(m_kernels[currTileIdx]) = 900; //no specific justification, simply enough to accommodate minimal stack operations
            if (kUsingSpoofTranspose) {
              runtime<ratio>(m_castKernel_kernels[currTileIdx]) = 0.9;
              source(m_castKernel_kernels[currTileIdx]) = "castKernel.cpp";
              stack_size(m_castKernel_kernels[currTileIdx]) = 300;
            }

            if ( (kKernelMatrixSize * TP_NUM_FRAMES * sizeof(TT_DATA)) > (__DATA_MEM_BYTES__ / 2) ) {
              single_buffer(m_kernels[currTileIdx].in[0]);
              if (x == y) {
                single_buffer(m_kernels[currTileIdx].out[0]);
              }

              if (kUsingSpoofTranspose) {
                single_buffer(m_castKernel_kernels[currTileIdx].in[0]);
                single_buffer(m_castKernel_kernels[currTileIdx].out[0]);
              }
            }

            currTileIdx++;
          }
        }
        //connect output (stream) down columns of kernels
        int sourceKernel = 0;
        int destKernel = 0;
        for (int y = 0; y < TP_GRID_DIM-1; y++) {
          destKernel = sourceKernel;
          for (int x = y+1; x < TP_GRID_DIM; x++) {
            destKernel += x;
            // X connections
            connect<stream>(m_kernels[sourceKernel].out[1], m_kernels[destKernel].in[2]);

          }
          //----
          // 0  |          y = 0
          //--------
          // 1  | 2 |      y = 1
          //------------
          // 3  | 4 | 5 |  y = 2
          //-------------
          sourceKernel += y+2; //next diagonal kernel (see diag above. Diagonal kernels are 0, 2, 5, 9, ...
        }

    }
};

} // namespace xf::solver::aie::substitution

#endif // _SOLVERLIB_SUBSTITUTION_GRAPH_HPP_
