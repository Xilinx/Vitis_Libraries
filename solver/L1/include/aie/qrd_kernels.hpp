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

#ifndef _QRD_KERNELS_HPP_
#define _QRD_KERNELS_HPP_

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "adf/stream/streams.h"
#include <adf.h>

/*
void qrd_givens_rotation(input_stream<float>* lower_row,
                         input_stream<float>* higher_row,
                         output_stream<float>* update_lower_row,
                         output_stream<float>* update_higher_row,
                         unsigned int row_num,
                         unsigned int column_num);
*/
namespace xf {
namespace solver {
void vec_cl_add_sh(input_stream<float>* cl_row,
                   input_stream<float>* sh_row,
                   output_stream<float>* ul_row,
                   unsigned int row_num,
                   unsigned int column_num);

void vec_ch_sub_sl(input_stream<float>* ch_row,
                   input_stream<float>* sl_row,
                   output_stream<float>* uh_row,
                   unsigned int row_num,
                   unsigned int column_num);

void vec_scalar_mul_csl(input_stream<float>* row,
                        output_stream<float>* c_row,
                        output_stream<float>* s_row,
                        unsigned int row_num,
                        unsigned int column_num);

void vec_scalar_mul_csh(input_stream<float>* row,
                        output_stream<float>* c_row,
                        output_stream<float>* s_row,
                        unsigned int row_num,
                        unsigned int column_num);
} // namespace solver
} // namespace xf
#endif
