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

#ifndef _XF_SOLVER_SVD_GRAPH_HPP_
#define _XF_SOLVER_SVD_GRAPH_HPP_

#include "svd_kernel.hpp"

namespace xf {
namespace solver {

using namespace adf;

template <int column_num, int row_num, int k_rep>
class SVDComplexFloat : public adf::graph {
   public:
    kernel m_k[column_num];
    input_port in_0;
    input_port in_1;
    output_port out_0;
    output_port out_1;

    SVDComplexFloat() {
        for (int i = 0; i < column_num; i++) {
            m_k[i] =
                kernel::create_object<OneSidedJacobiComplexFloat<row_num, column_num, k_rep> >(column_num, row_num, i);
            headers(m_k[i]) = {"svd_kernel.hpp"};
            source(m_k[i]) = "svd_kernel.cpp";
            runtime<ratio>(m_k[i]) = 1.0;
            stack_size(m_k[i]) = 22000;

            if (i == 0) {
                connect<stream> net0(in_0, m_k[i].in[0]);
                connect<stream> net1(in_1, m_k[i].in[1]);
            } else {
                connect<stream> net0(m_k[i - 1].out[0], m_k[i].in[0]);
                connect<stream> net1(m_k[i - 1].out[1], m_k[i].in[1]);
            }
            if (i == column_num - 1) {
                connect<stream> net2(m_k[i].out[0], out_0);
                connect<stream> net3(m_k[i].out[1], out_1);
            }
        }
    }
};

} // namespace solver
} // namespace xf

#endif
