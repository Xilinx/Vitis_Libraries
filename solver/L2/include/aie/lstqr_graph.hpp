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
 * @file lstqr_graph.hpp
 * @brief This file captures the definition of the `L2` graph level class for the QR Decomposition with cfloat data
 *type.
 **/

#ifndef _XF_SOLVER_LSTQR_GRAPH_HPP_
#define _XF_SOLVER_LSTQR_GRAPH_HPP_

#include "lstqr_kernel.hpp"

namespace xf {
namespace solver {

using namespace adf;

/**
 * @class LSTQR_Graph
 * @brief To solve least squares problems. QR decomposition with householder is used.
 *
 * These are the templates to configure the function.
 * @tparam rowA_num describes the matrixA number of rows.
 * @tparam colA_num describes the matrixA number of columns.
 * @tparam rowB_num describes the vector dim.
 * @tparam colB_num describes the number vectorB, more than one vetorB is supported.
 **/

template <int rowA_num, int colA_num, int rowB_num, int colB_num>
class LSTQR_Graph : public adf::graph {
   public:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     **/
    kernel k0[colA_num - 1];
    kernel k1;
    /**
     * The input data to the function.
     **/
    input_port din0;
    input_port din1;
    /**
     * The output data to the function.
     **/
    output_port dout0;
    output_port dout1;

    /**
     * The column_id
     **/
    input_port column_id[colA_num];

    /**
     * @brief This is the constructor function for the LSTQR_Graph.
     **/
    LSTQR_Graph() {
        for (int i = 0; i < colA_num - 1; i++) {
            k0[i] = kernel::create(lstqr<rowA_num, colA_num, colB_num>);
            headers(k0[i]) = {"lstqr_kernel.hpp"};
            // source file
            source(k0[i]) = "lstqr_kernel.cpp";
            runtime<ratio>(k0[i]) = 1.0;
            stack_size(k0[i]) = 30000;
        }
        k1 = kernel::create(lstqr_last<rowA_num, colA_num, colB_num>);
        headers(k1) = {"lstqr_kernel.hpp"};
        // source file
        source(k1) = "lstqr_kernel.cpp";
        runtime<ratio>(k1) = 1.0;
        stack_size(k1) = 30000;

        connect<stream> net00(din0, k0[0].in[0]);
        connect<stream> net01(din1, k0[0].in[1]);
        connect<parameter>(column_id[0], k0[0].in[2]);
        fifo_depth(net00) = 16;
        fifo_depth(net01) = 16;
        for (int i = 0; i < colA_num - 2; i++) {
            connect<stream> net00(k0[i].out[0], k0[i + 1].in[0]);
            connect<stream> net01(k0[i].out[1], k0[i + 1].in[1]);
            connect<parameter>(column_id[i + 1], k0[i + 1].in[2]);
            fifo_depth(net00) = 16;
            fifo_depth(net01) = 16;
        }
        connect<stream> net02(k0[colA_num - 2].out[0], k1.in[0]);
        connect<stream> net03(k0[colA_num - 2].out[1], k1.in[1]);
        connect<parameter>(column_id[colA_num - 1], k1.in[2]);
        fifo_depth(net02) = 16;
        fifo_depth(net03) = 16;

        connect<stream> net10(k1.out[0], dout0);
        connect<stream> net11(k1.out[1], dout1);
        fifo_depth(net10) = 16;
        fifo_depth(net11) = 16;
    }
};

} // namespace solver
} // namespace xf

#endif
