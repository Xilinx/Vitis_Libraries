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
#ifndef _SOLVERLIB_QRD_HH_GRAPH_HPP_
#define _SOLVERLIB_QRD_HH_GRAPH_HPP_
/*
The file captures the definition of the 'L2' graph level class for
the QRD function library element.
*/
/**
 * @file qrd_hh_graph.hpp
 *
 **/

#include <adf.h>
#include <vector>
#include <array>
#include <adf/arch/aie_arch_properties.hpp>
#include "graph_utils.hpp"
#include <tuple>

#include "qrd_hh.hpp"

namespace xf {
namespace solver {
namespace aie {
namespace qrd_hh {


/**
* @cond NOCOMMENTS
*/

template <typename TT_DATA,
          unsigned int kernelRowSize,
          unsigned int TP_DIM_COLS,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          bool streamEn,
          unsigned int cascPos = 0>
class create_casc_kernel_recur_qrd {
   public:
    static void create(kernel (&m_kernels)[TP_CASC_LEN]) {
    constexpr unsigned int kRrowsDistributed = cascPos*kernelRowSize;

    if constexpr (TP_CASC_LEN == 1) {
        m_kernels[cascPos] = kernel::create_object<qrd_hh<TT_DATA, kernelRowSize, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, cascPos, true, streamEn> >();
    }
    else{
        if constexpr (kRrowsDistributed < TP_DIM_COLS) {
            m_kernels[cascPos] = kernel::create_object<qrd_hh<TT_DATA, kernelRowSize, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, cascPos, true, streamEn> >();
        }
        else {
            m_kernels[cascPos] = kernel::create_object<qrd_hh<TT_DATA, kernelRowSize, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, cascPos, false, streamEn> >();
        }

        if constexpr (cascPos < TP_CASC_LEN - 1) {
        create_casc_kernel_recur_qrd <TT_DATA, kernelRowSize, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, streamEn, cascPos+1>::create(m_kernels);
        }
    }

    }// constructor
}; //   create_casc_kernel_recur_qrd

/**
 * @endcond
 */

 
/**
 * @defgroup qrd_hh_graph QRD function
 *
 * QR Decomposition function graph class
**/
//--------------------------------------------------------------------------------------------------
// qrd_hh_graph template
//--------------------------------------------------------------------------------------------------
/**
 * @ingroup qrd_hh_graph
 * @brief QRD Housholder implements the QR Decomposition of a matrix A such that A = QR, where Q is an orthogonal matrix
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
class qrd_hh_graph : public graph {
   public:
    /**
    * @cond NOCOMMENTS
    */
    // Defensive configuration legality checks

    // defensive check for Input data types
    static_assert(fnCheckDataType<TT_DATA>(),
                  " Assertion Failed : \n"
                  "             ERROR: TT_DATA is not a supported data type. \n");

    //defensive checks for row size
    static_assert(fnCheckChunkSize<TT_DATA, TP_DIM_COLS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_COLS x sizeof(TT_DATA) should be multiples of 32 bytes for AIE1 / 64 bytes for AIE-ML and AIE-MLv2. \n");
                  
    static_assert(fnCheckMinSize<TT_DATA, TP_DIM_COLS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_COLS should be minimum of 32 bytes / sizeof(TT_DATA) for AIE1 / 64 bytes / sizeof(TT_DATA) for AIE-ML and AIE-MLv2. \n");

    static_assert(TP_DIM_COLS <= TP_DIM_ROWS, 
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_COLS should be less than or equal to TP_DIM_ROWS for QRD. \n");   

    //defensive checks for row size
    static_assert(fnCheckChunkSize<TT_DATA, TP_DIM_ROWS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_ROWS x sizeof(TT_DATA) should be multiples of 32 bytes for AIE1 / 64 bytes for AIE-ML and AIE-MLv2.\n");

    static_assert(fnCheckMinSize<TT_DATA, TP_DIM_ROWS>(),
                  " Assertion Failed : \n"
                  "             ERROR: TP_DIM_ROWS should be minimum of 32 bytes / sizeof(TT_DATA) for AIE1 / 64 bytes / sizeof(TT_DATA) for AIE-ML and AIE-MLv2. \n");
    static_assert(fnCheckKernelLoadShare<TT_DATA, TP_DIM_ROWS, TP_CASC_LEN>(),
                  " Assertion Failed : \n"
                  "             ERROR: The number of row chunks per kernel (TP_DIM_ROWS/(TP_CASC_LEN))  should be multiples of 32 bytes for AIE1 / 64 bytes for AIE-ML and AIE-MLv2.\n");


    //defensive checks for buffer size per kernel
    static_assert(fnCheckMaxSize<TT_DATA, TP_DIM_ROWS, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN>(),
                    " Assertion Failed : \n"
                    "             ERROR: The total input data size (TP_DIM_ROWS x TP_DIM_COLS x TP_NUM_FRAMES x sizeof(TT_DATA)) exceeds the maximum supported for the given cascade length (TP_CASC_LEN). Please reduce the matrix dimensions, number of frames or increase the cascade length. \n");


    // static_assert(fnCheckTiling<TT_DATA, TP_DIM_A_LEADING, TP_DIM_Q_LEADING, TP_DIM_R_LEADING>(),
    //               " Assertion Failed : \n"
    //               "             ERROR: Row-major is not supported for cfloat data type on AIE1.\n");

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
    // kernel m_qkernels[TP_CASC_LEN];

    /**
     * Access function to get pointer to kernel.
     **/

    kernel* getKernelsR() { return m_kernels; };
    // kernel* getKernelsQ() { return m_qkernels; };

    /**
     * @brief This is the constructor function for the qrd_hh graph.
     **/
    qrd_hh_graph() {        
        constexpr unsigned int kKernelRowSize = (TP_DIM_ROWS/TP_CASC_LEN);  
        unsigned int kKernelWindowVsizeQ = (TP_NUM_FRAMES * kKernelRowSize * TP_DIM_COLS);
        printf("kKernelRowSize: %d \n", kKernelRowSize);
        printf("kKernelWindowVsizeQ: %d \n", kKernelWindowVsizeQ);

        constexpr bool streamEn = (TP_CASC_LEN > 1);
        create_casc_kernel_recur_qrd <TT_DATA, kKernelRowSize, TP_DIM_COLS, TP_NUM_FRAMES, TP_CASC_LEN, streamEn, 0>::create(m_kernels);

        for (int i = 0; i < TP_CASC_LEN; i++) {  
            unsigned int kRrowsDistributed = i*kKernelRowSize;
            bool routEn = (kRrowsDistributed < TP_DIM_COLS); 
 
            // Specify mapping constraints
            source(m_kernels[i]) = "qrd_hh.cpp";
            headers(m_kernels[i]) = {"qrd_hh.hpp"};
            runtime<ratio>(m_kernels[i]) = 0.9;
            stack_size(m_kernels[i]) = 2048;

            // make connections
            connect(inA[i], m_kernels[i].in[0]);
            dimensions(m_kernels[i].in[0]) = {kKernelWindowVsizeQ};
            connect(m_kernels[i].out[0], outQ[i]);
            dimensions(m_kernels[i].out[0]) = {kKernelWindowVsizeQ};
            single_buffer(m_kernels[i].in[0]);
            single_buffer(m_kernels[i].out[0]);

            if (routEn){
                unsigned int kKernelRrowSize =  (kKernelRowSize <= (TP_DIM_COLS-kRrowsDistributed)) ? kKernelRowSize : (TP_DIM_COLS-kRrowsDistributed); 
                unsigned int kKernelWindowVsizeR = (TP_NUM_FRAMES * kKernelRrowSize * TP_DIM_COLS); 
                printf("Kernel : %d \n", i);
                printf("kKernelWindowVsizeR: %d \n", kKernelWindowVsizeR);

                connect(m_kernels[i].out[1], outR[i]);
                dimensions(m_kernels[i].out[1]) = {kKernelWindowVsizeR};
                single_buffer(m_kernels[i].out[1]);

                if constexpr (TP_DIM_R_LEADING == 1){
                read_access(m_kernels[i].out[1]) =
                    adf::tiling(
                        {       .buffer_dimension = {kKernelRrowSize, TP_DIM_COLS, TP_NUM_FRAMES},
                                .tiling_dimension = {1, TP_DIM_COLS, 1},
                                .offset = {0, 0, 0},
                                .tile_traversal = {
                                {.dimension = 0, .stride = 1, .wrap = kKernelRrowSize},
                                {.dimension = 1, .stride = TP_DIM_COLS, .wrap = 1},
                                {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                        }
                    });
                }
            }

            if constexpr (TP_CASC_LEN > 1) {
                if (routEn){
                    if (i > 0) {connect<stream>(m_kernels[i].out[2], m_kernels[i-1].in[1]);}
                    if (i == TP_CASC_LEN - 1) {connect<stream>(m_kernels[0].out[2], m_kernels[i].in[1]);}
                }else{//in this scenario stream is out1 for the kernel instead of out2
                    if (i > 0) {connect<stream>(m_kernels[i].out[1], m_kernels[i-1].in[1]);}
                    if (i == TP_CASC_LEN - 1) {connect<stream>(m_kernels[0].out[2], m_kernels[i].in[1]);}
                }

            }       

            if constexpr (TP_DIM_A_LEADING == 1){
            write_access(m_kernels[i].in[0]) =
                adf::tiling(
                {       .buffer_dimension = {(TP_DIM_ROWS/TP_CASC_LEN), TP_DIM_COLS, TP_NUM_FRAMES},
                        .tiling_dimension = {1, TP_DIM_COLS, 1},
                        .offset = {0, 0, 0},
                        .tile_traversal = {
                        {.dimension = 0, .stride = 1, .wrap = (TP_DIM_ROWS/TP_CASC_LEN)},
                        {.dimension = 1, .stride = TP_DIM_COLS, .wrap = 1},
                        {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                }
                });

            };

            if constexpr (TP_DIM_Q_LEADING == 1){
            read_access(m_kernels[i].out[0]) =
                adf::tiling(
                    {       .buffer_dimension = {(TP_DIM_ROWS/TP_CASC_LEN), TP_DIM_COLS, TP_NUM_FRAMES},
                            .tiling_dimension = {1, TP_DIM_COLS, 1},
                            .offset = {0, 0, 0},
                            .tile_traversal = {
                            {.dimension = 0, .stride = 1, .wrap = (TP_DIM_ROWS/TP_CASC_LEN)},
                            {.dimension = 1, .stride = TP_DIM_COLS, .wrap = 1},
                            {.dimension = 2, .stride = 1, .wrap = TP_NUM_FRAMES}
                    }
                });
            }


        }

    }; // constructor
}; //graph class

} // namespace qrd_hh
}// namespace aie
}// namespace solver
} // namespace xf

#endif //_SOLVERLIB_QRD_HH_GRAPH_HPP_