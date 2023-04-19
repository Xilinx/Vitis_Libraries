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
#ifndef __CHOLESKY_FLOAT_GRAPHS_HPP__
#define __CHOLESKY_FLOAT_GRAPHS_HPP__

#include "aie/cholesky_float_decomposition.hpp"

namespace xf {
namespace solver {
using namespace adf;

template <int NUM>
class CholeskyGraph : public adf::graph {
   private:
    kernel k[NUM];

   public:
    input_plio matA_data;
    output_plio matL_data;

    CholeskyGraph(std::string matA_data_name,
                  std::string matA_data_file_name,
                  std::string matL_data_name,
                  std::string matL_data_file_name) {
        matA_data = input_plio::create(matA_data_name, adf::plio_32_bits, matA_data_file_name);
        matL_data = output_plio::create(matL_data_name, adf::plio_32_bits, matL_data_file_name);

        for (int i = 0; i < NUM; i++) {
            k[i] = kernel::create(cholesky_float);
            source(k[i]) = "aie/cholesky_float_decomposition.cpp";
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
