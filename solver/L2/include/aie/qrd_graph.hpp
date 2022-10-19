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

#ifndef _QRD_GRAPH_HPP_
#define _QRD_GRAPH_HPP_

#include "aie/qrd_kernels.hpp"

namespace xf {
namespace solver {
using namespace adf;
class GivensRotationQRD : public adf::graph {
   private:
    kernel k_csl;
    kernel k_csh;
    kernel k_ul;
    kernel k_uh;

   public:
    input_plio in_lower_row;
    input_plio in_higher_row;
    output_plio out_lower_row;
    output_plio out_higher_row;

    input_port row_num0;
    input_port row_num1;
    input_port row_num2;
    input_port row_num3;
    input_port column_num0;
    input_port column_num1;
    input_port column_num2;
    input_port column_num3;

    GivensRotationQRD(std::string in_lower_row_name,
                      std::string in_lower_file_name,
                      std::string in_higher_row_name,
                      std::string in_higher_file_name,
                      std::string out_lower_row_name,
                      std::string out_lower_file_name,
                      std::string out_higher_row_name,
                      std::string out_higher_file_name) {
        in_lower_row = input_plio::create(in_lower_row_name, adf::plio_32_bits, in_lower_file_name);
        in_higher_row = input_plio::create(in_higher_row_name, adf::plio_32_bits, in_higher_file_name);
        out_lower_row = output_plio::create(out_lower_row_name, adf::plio_32_bits, out_lower_file_name);
        out_higher_row = output_plio::create(out_higher_row_name, adf::plio_32_bits, out_higher_file_name);

        k_ul = kernel::create(vec_cl_add_sh);
        k_uh = kernel::create(vec_ch_sub_sl);
        k_csl = kernel::create(vec_scalar_mul_csl);
        k_csh = kernel::create(vec_scalar_mul_csh);
        source(k_csl) = "aie/qrd_kernels.cpp";
        source(k_csh) = "aie/qrd_kernels.cpp";
        source(k_ul) = "aie/qrd_kernels.cpp";
        source(k_uh) = "aie/qrd_kernels.cpp";
        runtime<ratio>(k_csl) = 1.0;
        runtime<ratio>(k_csh) = 1.0;
        runtime<ratio>(k_ul) = 1.0;
        runtime<ratio>(k_uh) = 1.0;

        connect<stream>(in_lower_row.out[0], k_csl.in[0]);
        connect<stream>(in_higher_row.out[0], k_csh.in[0]);
        connect<stream>(k_csl.out[0], k_ul.in[0]);
        connect<stream>(k_csh.out[1], k_ul.in[1]);
        connect<stream>(k_csh.out[0], k_uh.in[0]);
        connect<stream>(k_csl.out[1], k_uh.in[1]);
        connect<stream>(k_ul.out[0], out_lower_row.in[0]);
        connect<stream>(k_uh.out[0], out_higher_row.in[0]);
        connect<parameter>(row_num0, k_csl.in[1]);
        connect<parameter>(row_num1, k_csh.in[1]);
        connect<parameter>(row_num2, k_ul.in[2]);
        connect<parameter>(row_num3, k_uh.in[2]);
        connect<parameter>(column_num0, k_csl.in[2]);
        connect<parameter>(column_num1, k_csh.in[2]);
        connect<parameter>(column_num2, k_ul.in[3]);
        connect<parameter>(column_num3, k_uh.in[3]);
    }
};
} // namespace solver
} // namespace xf

#endif
