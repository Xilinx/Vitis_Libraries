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
 * @file cholesky_complex_decomposition_flexible_graph.hpp
 * @brief This file captures the definition of the `L2` graph level class for the flexible Cholesky Decomposition with
 *cfloat
 *data type.
 **/

#ifndef __CHOLESKY_COMPLEX_FLEXIBLE_GRAPHS_HPP__
#define __CHOLESKY_COMPLEX_FLEXIBLE_GRAPHS_HPP__

#include "cholesky_complex_decomposition_flexible.hpp"

namespace xf {
namespace solver {
using namespace adf;

/**
 * @class CholeskyFlexibleGraph
 * @brief CholeksyDecomposition transform a Hermitian positive-definite matrix into the product of a lower triangular
 *matrix and its conjugate transpose
 *
 * These are the templates to configure the function.
 * @tparam Dim describes the dimensition of input matrix.
 * @tparam CoreNum describes the number of AIE cores used.
 * @tparam BlkNum describes the number of matrix columns are calculated by each core.
 **/

template <int Dim, int CoreNum, int BlkNum>
class CholeskyFlexibleGraph : public adf::graph {
   public:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     * Number of kernels (`CoreNum`) will be connected with each other in series.
     **/
    kernel k[CoreNum];

    /**
     * The input data to the function.
     **/
    input_port sig_i;
    /**
     * The output data to the function.
     **/
    output_port sig_o;

    /**
     * @brief This is the constructor function for the CholeskyFlexibleGraph.
     **/
    CholeskyFlexibleGraph() {
        for (int i = 0; i < CoreNum; i++) {
            k[i] = adf::kernel::create_object<CholeskyComplexFlexible<Dim, CoreNum, BlkNum> >(i * BlkNum);
            // source file
            headers(k[i]) = {"cholesky_complex_decomposition_flexible.hpp"};
            source(k[i]) = "cholesky_complex_decomposition_flexible.cpp";
            runtime<ratio>(k[i]) = 0.9;
        }

        // connections:
        connect<>(sig_i, k[0].in[0]);

        for (int i = 0; i < CoreNum - 1; i++) {
            connect<>(k[i].out[0], k[i + 1].in[0]);
        }
        connect<>(k[CoreNum - 1].out[0], sig_o);

        // Dimensions:
        for (int i = 0; i < CoreNum; i++) {
            adf::dimensions(k[i].in[0]) = {Dim};
            adf::dimensions(k[i].out[0]) = {Dim};
            repetition_count(k[i]) = Dim;
        }
    }
};

} // namespace solver
} // namespace xf

#endif
