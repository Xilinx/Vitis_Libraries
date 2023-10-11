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
 * @file cholesky_float_decomposition_graph.hpp
 * @brief This file captures the definition of the `L2` graph level class for the Cholesky Decomposition with float data
 *type.
 **/

#ifndef __CHOLESKY_FLOAT_GRAPHS_HPP__
#define __CHOLESKY_FLOAT_GRAPHS_HPP__

#include "aie/cholesky_float_decomposition.hpp"

namespace xf {
namespace solver {
using namespace adf;

/**
 * @class CholeskyGraph
 * @brief CholeksyDecomposition transform a Hermitian positive-definite matrix into the product of a lower triangular
 *matrix and its conjugate transpose
 *
 * These are the templates to configure the function.
 * @tparam NUM describes the number of AIE cores used.
 **/

template <int NUM>
class CholeskyGraph : public adf::graph {
   private:
    /**
     * kernel instance.
     * The chain of kernels that will be created and mapped on AIE tiles.
     * Number of kernels (`NUM`) will be connected with each other in series.
     **/
    kernel k[NUM];

   public:
    /**
     * The input data to the function.
     **/
    input_plio matA_data;
    /**
     * The output data to the function.
     **/
    output_plio matL_data;

    /**
     * @brief This is the constructor function for the CholeskyGraph.
     * @param[in] matA_data_name: specifies the attributes of port name
     * @param[in] matA_data_file_name: specifies the input data file path;
     * @param[out] matL_data_name: specifies the attributes of port name
     * @param[out] matL_data_file_name: specifies the output data file path;
     **/
    CholeskyGraph(std::string matA_data_name,
                  std::string matA_data_file_name,
                  std::string matL_data_name,
                  std::string matL_data_file_name) {
        matA_data = input_plio::create(matA_data_name, adf::plio_32_bits, matA_data_file_name);
        matL_data = output_plio::create(matL_data_name, adf::plio_32_bits, matL_data_file_name);

        for (int i = 0; i < NUM; i++) {
            k[i] = kernel::create(cholesky_float);
            // source file
            source(k[i]) = "cholesky_float_decomposition.cpp";
            runtime<ratio>(k[i]) = 1.0;
        }

        connect<stream>(matA_data.out[0], k[0].in[0]);

        for (int i = 0; i < NUM - 1; i++) {
            connect<stream>(k[i].out[0], k[i + 1].in[0]);
            stack_size(k[i]) = 3000;
        }

        connect<stream>(k[NUM - 1].out[0], matL_data.in[0]);
        stack_size(k[NUM - 1]) = 3000;
    }
};

} // namespace solver
} // namespace xf

#endif
