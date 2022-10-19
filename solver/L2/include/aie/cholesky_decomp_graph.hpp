/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef __CHOLESKY_DECOMP_GRAPHS_HPP_
#define __CHOLESKY_DECOMP_GRAPHS_HPP_

#include "aie/cholesky_decomp_kernels.hpp"

namespace xf {
namespace solver {
using namespace adf;
class GivensRotationQRD : public adf::graph {
   private:
    kernel k1;

   public:
    input_plio in_lower_row;
    input_plio in_higher_row;
    output_plio out_update_row;

    input_port row_num;

    GivensRotationQRD(std::string in_lower_row_name,
                      std::string in_lower_file_name,
                      std::string in_higher_row_name,
                      std::string in_higher_file_name,
                      std::string out_update_row_name,
                      std::string out_update_file_name) {
        in_lower_row = input_plio::create(in_lower_row_name, adf::plio_32_bits, in_lower_file_name);
        in_higher_row = input_plio::create(in_higher_row_name, adf::plio_32_bits, in_higher_file_name);
        out_update_row = output_plio::create(out_update_row_name, adf::plio_32_bits, out_update_file_name);

        k1 = kernel::create(cholesky_decomp);
        source(k1) = "aie/cholesky_decomp_kernels.cpp";
        runtime<ratio>(k1) = 1.0;

        connect<stream>(in_lower_row.out[0], k1.in[0]);
        connect<stream>(in_higher_row.out[0], k1.in[1]);
        connect<stream>(k1.out[0], out_update_row.in[0]);
        connect<parameter>(row_num, k1.in[2]);
    }
};

} // namespace solver
} // namespace xf

#endif
