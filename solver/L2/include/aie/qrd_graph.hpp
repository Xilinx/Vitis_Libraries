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
#ifndef _SOLVERLIB_QRD_GRAPH_HPP_
#define _SOLVERLIB_QRD_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the QRD function library element.
*/
/**
 * @file qrd_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "qrd.hpp"
#include "qrd_load_split.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd {


/**
* @cond NOCOMMENTS
*/

template <unsigned int cascPos,
          unsigned int ColsDist,
          typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN>
class create_casc_kernel_recur_qrd {
   public:
    static void create(kernel (&qrdKernels)[TP_CASC_LEN]) {

    constexpr unsigned int kKernelColSize = qrd_load_split <TT_DATA,TP_DIM_COLS,TP_DIM_ROWS,TP_CASC_LEN,TP_NUM_FRAMES>(ColsDist, cascPos);
    printf("Creating Kernel %d\n", cascPos);
    printf("COL DIM for Kernel %d is %d\n", cascPos, kKernelColSize);
    printf("Distributed columns before Kernel %d is %d\n", cascPos, ColsDist);

    //defensive check for buffer size
    static_assert(fnCheckBufferSize<TT_DATA, TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES>(),
                  " Assertion Failed : \n"
                  "             ERROR: Design does not fit in maximum buffer size, consider reducing one of the dimension parameters. TP_DIM_COLS/TP_DIM_ROWS/TP_NUM_FRAMES. \n");
    
    static_assert(fnCheckKernelLoadMin(kKernelColSize),
                  " Assertion Failed : \n"
                  "             ERROR: CASC_LEN is too big to fit in. Either decrease CASC_LEN or increase TP_DIM_COLS. \n");



    if constexpr (cascPos == 0 && TP_CASC_LEN == 1) {
        qrdKernels[cascPos] = kernel::create_object<qrd<TT_DATA, TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES, TP_CASC_LEN, ColsDist, TP_DIM_COLS, CASC_IN_FALSE, CASC_OUT_FALSE>>();

    } else if constexpr (cascPos == 0 && TP_CASC_LEN > 1) {
        qrdKernels[cascPos] = kernel::create_object<qrd<TT_DATA, TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES, TP_CASC_LEN, ColsDist, TP_DIM_COLS, CASC_IN_FALSE, CASC_OUT_TRUE>>();
        create_casc_kernel_recur_qrd <cascPos + 1, ColsDist + kKernelColSize, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN>::create(qrdKernels);
        
    } else if constexpr (cascPos == TP_CASC_LEN - 1 && TP_CASC_LEN > 1) {
        qrdKernels[cascPos] = kernel::create_object<qrd<TT_DATA, TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES, TP_CASC_LEN, ColsDist, TP_DIM_COLS, CASC_IN_TRUE, CASC_OUT_FALSE>>();

        static_assert(fnCheckDistributedLoad<TP_DIM_COLS>(ColsDist+kKernelColSize),
                  " Assertion Failed : \n"
                  "             ERROR: Design does not fit in cascaded stages, consider increasing TP_CASC_LEN or reducing TP_DIM_COLS. \n");

    } else if constexpr (cascPos != 0 && cascPos != TP_CASC_LEN - 1 && TP_CASC_LEN > 1) {
        qrdKernels[cascPos] = kernel::create_object<qrd<TT_DATA, TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES, TP_CASC_LEN, ColsDist, TP_DIM_COLS, CASC_IN_TRUE, CASC_OUT_TRUE>>();
        create_casc_kernel_recur_qrd <cascPos + 1, ColsDist + kKernelColSize, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN>::create(qrdKernels);
    }
        }
    };

/**
 * @endcond
 */

 
/**
 * @defgroup qrd_graph QRD function
 *
 * QR Decomposition function graph class
**/
//--------------------------------------------------------------------------------------------------
// qrd_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup qrd_graph
 * @brief QRD implements the QR Decomposition of a matrix A such that A = QR, where Q is an orthogonal matrix
 * and R is an upper triangular matrix using Modified Gram-Schmidt process.
 *
 * These are the templates to configure the function.
 * @tparam TT_DATA describes the type of individual data samples input to the function. \n
 *         This is a typename and must be one of the following: \n
 *         float, cfloat.
 * @tparam TP_DIM_ROWS describes the number of rows in the input matrix A.
 * @tparam TP_DIM_COLS describes the number of columns in the input matrix A.
 * @tparam TP_NUM_FRAMES describes the number of vectors to be processed in each
 *         call to this function.
 * @tparam TP_CASC_LEN selects the number of kernels the QRD will be split over to accomplish
 *         higher matrix sizes.
 **/


template <typename TT_DATA,
          unsigned int TP_DIM_ROWS,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_DIM_A_LEADING,
          unsigned int TP_DIM_Q_LEADING,
          unsigned int TP_DIM_R_LEADING>
class qrd_graph : public graph {
   public:
    /**
    * @cond NOCOMMENTS
    */
    // Defensive configuration legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataType<TT_DATA>(),
                  " Assertion Failed : \n"
                  "             ERROR: TT_DATA is not a supported data type. \n");


    static_assert(fnCheckChunkSize<TT_DATA, TP_DIM_COLS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_COLS x sizeof(TT_DATA) should be multiples of 32 bytes for AIE1 / 64 bytes for AIE-ML and AIE-MLv2. \n");
                  
    static_assert(fnCheckMinSize<TT_DATA, TP_DIM_COLS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_COLS should be minimum of 32 bytes / sizeof(TT_DATA) for AIE1 / 64 bytes / sizeof(TT_DATA) for AIE-ML and AIE-MLv2. \n");
                  
    //defensive checks for row size
    static_assert(fnCheckChunkSize<TT_DATA, TP_DIM_ROWS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_ROWS x sizeof(TT_DATA) should be multiples of 32 bytes for AIE1 / 64 bytes for AIE-ML and AIE-MLv2.\n");
                  
    static_assert(fnCheckMinSize<TT_DATA, TP_DIM_ROWS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_ROWS should be minimum of 32 bytes / sizeof(TT_DATA) for AIE1 / 64 bytes / sizeof(TT_DATA) for AIE-ML and AIE-MLv2. \n");

    static_assert(fnCheckTiling<TT_DATA, TP_DIM_A_LEADING, TP_DIM_Q_LEADING, TP_DIM_R_LEADING>(),
                  " Assertion Failed : \n"
                  "             ERROR: Row-major is not supported for cfloat data type on AIE1.\n");

    /**
    * @endcond
    */
   
    /**
     * The input matrix tile(s).
     **/
    port_array<input, TP_CASC_LEN> inA;
    /**
     * The output Q matrix tile(s).
     **/
    port_array<output, TP_CASC_LEN> outQ;
    /**
     * The output R matrix tile(s).
     **/
    port_array<output, TP_CASC_LEN> outR;

    /**
    * Access function to get pointer to kernel (or first kernel in a chained configuration).
    * No arguments required.
    **/
    kernel m_kernels[TP_CASC_LEN];

    /**
     * Access function to get pointer to kernel.
     **/

    kernel* getKernels() { return m_kernels; };

    /**
     * @brief This is the constructor function for the qrd graph.
     **/
    qrd_graph() {

        //Calc to set Kernel buffer Sizes
        std::pair<std::array<unsigned int, TP_CASC_LEN>, std::array<unsigned int, TP_CASC_LEN>> cols_result;
        cols_result = get_qrd_loads_of_kernels<TT_DATA, TP_DIM_COLS, TP_DIM_ROWS, TP_CASC_LEN, TP_NUM_FRAMES>();

        std::array<unsigned int, TP_CASC_LEN> col_dim_kernel_arr = cols_result.first;
        std::array<unsigned int, TP_CASC_LEN> col_dim_dist_kernel_arr = cols_result.second;
        
        // create kernels
        create_casc_kernel_recur_qrd<0, 0, TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN>::create(m_kernels);
        
        for (int i = 0; i < TP_CASC_LEN; i++) {        
            unsigned int kKernelColSize = col_dim_kernel_arr[i];
            unsigned int kKernelWindowVsizeQ = (TP_NUM_FRAMES * TP_DIM_ROWS * kKernelColSize);
            unsigned int kKernelWindowVsizeR = (TP_NUM_FRAMES * TP_DIM_COLS * kKernelColSize);
            printf("Kernel Num = %d \n", i);
            printf("COL_PER_KERNEL = %d \n", kKernelColSize);
            printf("kKernelWindowVsizeQ = %d \n", kKernelWindowVsizeQ);
            printf("kKernelWindowVsizeR = %d \n", kKernelWindowVsizeR);

            // Specify mapping constraints
            runtime<ratio>(m_kernels[i]) = 0.1; // Nominal figure. The real figure requires knowledge of the sample
                                                // rate.
            // Source files
            source(m_kernels[i]) = "qrd.cpp";

            // make connections
            connect(inA[i], m_kernels[i].in[0]);
            dimensions(m_kernels[i].in[0]) = {kKernelWindowVsizeQ};
            connect(m_kernels[i].out[0], outQ[i]);
            connect(m_kernels[i].out[1], outR[i]);
            dimensions(m_kernels[i].out[0]) = {kKernelWindowVsizeQ};
            dimensions(m_kernels[i].out[1]) = {kKernelWindowVsizeR};

            //apply single buffer constraints, QRD is a by-default single buffer implementation
            single_buffer(m_kernels[i].in[0]);
            single_buffer(m_kernels[i].out[0]);
            single_buffer(m_kernels[i].out[1]);  


            if (TP_DIM_A_LEADING == 1){
            write_access(m_kernels[i].in[0]) =
                adf::tiling(
                {       .buffer_dimension = {TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES},
                        .tiling_dimension = {1, kKernelColSize, 1},
                        .offset = {0, 0, 0},
                        .tile_traversal = {
                        {.dimension = 0, .stride = 1, .wrap = TP_DIM_ROWS},
                        {.dimension = 1, .stride = kKernelColSize, .wrap = 1},
                        {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                }
                });

            };

            if (TP_DIM_Q_LEADING == 1){
            read_access(m_kernels[i].out[0]) =
                adf::tiling(
                    {       .buffer_dimension = {TP_DIM_ROWS, kKernelColSize, TP_NUM_FRAMES},
                            .tiling_dimension = {1, kKernelColSize, 1},
                            .offset = {0, 0, 0},
                            .tile_traversal = {
                            {.dimension = 0, .stride = 1, .wrap = TP_DIM_ROWS},
                            {.dimension = 1, .stride = kKernelColSize, .wrap = 1},
                            {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                    }
                });
            }


            if (TP_DIM_R_LEADING == 1){
            read_access(m_kernels[i].out[1]) =
                adf::tiling(
                    {       .buffer_dimension = {TP_DIM_COLS, kKernelColSize, TP_NUM_FRAMES},
                            .tiling_dimension = {1, kKernelColSize, 1},
                            .offset = {0, 0, 0},
                            .tile_traversal = {
                            {.dimension = 0, .stride = 1, .wrap = TP_DIM_COLS},
                            {.dimension = 1, .stride = kKernelColSize, .wrap = 1},
                            {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                    }
                });
            }



            if (i > 0 ) {
                connect<cascade>(m_kernels[i-1].out[2], m_kernels[i].in[1]); 
            }
        }

    }; // constructor
}; //graph class

} // namespace qrd
}// namespace aie
}// namespace solver
} // namespace xf

#endif //_SOLVERLIB_QRD_GRAPH_HPP_
