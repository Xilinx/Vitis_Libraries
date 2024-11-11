/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

/**
 * @file qrd_householder_graph.hpp
 * @brief This file captures the definition of the `L2` graph level class for the QR Decomposition with cfloat data
 *type.
 **/

#ifndef _XF_SOLVER_QRD_HOUSEHOLDER_GRAPH_HPP_
#define _XF_SOLVER_QRD_HOUSEHOLDER_GRAPH_HPP_

#include "qrd_householder_kernel.hpp"

namespace xf {
namespace solver {

using namespace adf;

/**
 * @class QRD_Householder_Graph
 * @brief QR decomposition is a decomposition of a matrix A into a product A = QR of an orthonormal matrix Q and an
 *upper triangular matrix R.
 *
 * These are the templates to configure the function.
 * @tparam column_num describes the number of columns.
 * @tparam row_num describes the number of rows.
 **/
template <int row_num, int column_num>
class QRD_Householder_Graph : public adf::graph {
   public:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     **/
    kernel k0[column_num - 1];
    kernel k1;
    /**
     * The input data to the function.
     **/
    input_plio matAU_0;
    input_plio matAU_1;
    /**
     * The output data to the function.
     **/
    output_plio matRQ_0;
    output_plio matRQ_1;

    /**
     * The column_id
     **/
    input_port column_id[column_num];

    /**
     * @brief This is the constructor function for the QRD_Householder_Graph.
     **/
    QRD_Householder_Graph(std::string matAU_0_filename,
                          std::string matAU_1_filename,
                          std::string matRQ_0_filename,
                          std::string matRQ_1_filename) {
        matAU_0 = input_plio::create("matAU_0", plio_128_bits, matAU_0_filename, 1000);
        matAU_1 = input_plio::create("matAU_1", plio_128_bits, matAU_1_filename, 1000);
        matRQ_0 = output_plio::create("matRQ_0", plio_128_bits, matRQ_0_filename, 1000);
        matRQ_1 = output_plio::create("matRQ_1", plio_128_bits, matRQ_1_filename, 1000);

        for (int i = 0; i < column_num - 1; i++) {
            k0[i] = kernel::create(qrd_householder<row_num, column_num>);
            headers(k0[i]) = {"qrd_householder_kernel.hpp"};
            // source file
            source(k0[i]) = "qrd_householder_kernel.cpp";
            runtime<ratio>(k0[i]) = 1.0;
            stack_size(k0[i]) = 30000;
        }
        k1 = kernel::create(qrd_householder_last<row_num, column_num>);
        headers(k1) = {"qrd_householder_kernel.hpp"};
        // source file
        source(k1) = "qrd_householder_kernel.cpp";
        runtime<ratio>(k1) = 1.0;
        stack_size(k1) = 30000;

        connect<stream> net00(matAU_0.out[0], k0[0].in[0]);
        connect<stream> net01(matAU_1.out[0], k0[0].in[1]);
        connect<parameter>(column_id[0], k0[0].in[2]);
        fifo_depth(net00) = 16;
        fifo_depth(net01) = 16;
        for (int i = 0; i < column_num - 2; i++) {
            connect<stream> net00(k0[i].out[0], k0[i + 1].in[0]);
            connect<stream> net01(k0[i].out[1], k0[i + 1].in[1]);
            connect<parameter>(column_id[i + 1], k0[i + 1].in[2]);
            fifo_depth(net00) = 16;
            fifo_depth(net01) = 16;
        }
        connect<stream> net02(k0[column_num - 2].out[0], k1.in[0]);
        connect<stream> net03(k0[column_num - 2].out[1], k1.in[1]);
        connect<parameter>(column_id[column_num - 1], k1.in[2]);
        fifo_depth(net02) = 16;
        fifo_depth(net03) = 16;

        connect<stream> net10(k1.out[0], matRQ_0.in[0]);
        connect<stream> net11(k1.out[1], matRQ_1.in[0]);
        fifo_depth(net10) = 16;
        fifo_depth(net11) = 16;
    }
};

} // namespace solver
} // namespace xf

#endif
