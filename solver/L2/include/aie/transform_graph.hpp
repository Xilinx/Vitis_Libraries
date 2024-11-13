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

#ifndef _XF_SOLVER_Transform_GRAPH_HPP_
#define _XF_SOLVER_Transform_GRAPH_HPP_

#include "lstqr_kernel.hpp"

namespace xf {
namespace solver {

using namespace adf;

/**
 * @class Transform_Graph
 * @brief QR decomposition is a decomposition of a matrix A into a product A = QR of an orthonormal matrix Q and an
 *upper triangular matrix R.
 *
 * These are the templates to configure the function.
 * @tparam colA_num describes the number of columns.
 * @tparam rowA_num describes the number of rows.
 **/
template <int rowA_num, int colA_num, int rowB_num, int colB_num>
class Transform_Graph : public adf::graph {
   private:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     **/
    kernel k0;

   public:
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
     * @brief This is the constructor function for the Transform_Graph.
     **/
    Transform_Graph() {

        k0 = kernel::create(transform<rowA_num, colA_num, colB_num>);
        headers(k0) = {"lstqr_kernel.hpp"};
        // source file
        source(k0) = "lstqr_kernel.cpp";
        runtime<ratio>(k0) = 1.0;
        stack_size(k0) = 30000;

        connect<stream> net00(din0, k0.in[0]);
        connect<stream> net01(din1, k0.in[1]);
        fifo_depth(net00) = 16;
        fifo_depth(net01) = 16;
        connect<stream> net10(k0.out[0], dout0);
        connect<stream> net11(k0.out[1], dout1);
        fifo_depth(net10) = 16;
        fifo_depth(net11) = 16;
    }
};

} // namespace solver
} // namespace xf

#endif
