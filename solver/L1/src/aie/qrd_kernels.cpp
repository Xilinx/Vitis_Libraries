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

#include "aie/qrd_kernels.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/adf/stream.hpp"

namespace xf {
namespace solver {

void givens_params(float& a11, float& a21, float& c, float& s) {
    if (a21 == 0.0) {
        c = 1;
        s = 0;
    } else {
        float cot = a11 / a21;
        float cot_1 = 1.0 + cot * cot;
        s = aie::invsqrt(cot_1);
        c = s * cot;
    }
}

void vec_cl_add_sh(input_stream<float>* cl_row,
                   input_stream<float>* sh_row,
                   output_stream<float>* ul_row,
                   unsigned int row_num,
                   unsigned int column_num) {
    unsigned int col_batch = column_num / 4;
    for (unsigned int j = 0; j < column_num; j++) {
        for (unsigned int i = row_num - 1; i > j; i--) {
            for (unsigned int b = 0; b < col_batch; b++) {
                aie::vector<float, 4> cl_v = readincr_v<4>(cl_row);
                aie::vector<float, 4> sh_v = readincr_v<4>(sh_row);
                aie::vector<float, 4> ul_v = aie::add(cl_v, sh_v);
                writeincr(ul_row, ul_v);
            }
        }
    }
}

void vec_ch_sub_sl(input_stream<float>* ch_row,
                   input_stream<float>* sl_row,
                   output_stream<float>* uh_row,
                   unsigned int row_num,
                   unsigned int column_num) {
    unsigned int col_batch = column_num / 4;
    for (unsigned int j = 0; j < column_num; j++) {
        for (unsigned int i = row_num - 1; i > j; i--) {
            for (unsigned int b = 0; b < col_batch; b++) {
                aie::vector<float, 4> ch_v = readincr_v<4>(ch_row);
                aie::vector<float, 4> sl_v = readincr_v<4>(sl_row);
                aie::vector<float, 4> uh_v = aie::sub(ch_v, sl_v);
                writeincr(uh_row, uh_v);
            }
        }
    }
}

void vec_scalar_mul_csl(input_stream<float>* row,
                        output_stream<float>* c_row,
                        output_stream<float>* s_row,
                        unsigned int row_num,
                        unsigned int column_num) {
    unsigned int col_batch = column_num / 4;
    for (unsigned int j = 0; j < column_num; j++) {
        for (unsigned int i = row_num - 1; i > j; i--) {
            float a11, a21, c, s;
            aie::vector<float, 4> tmp = readincr_v<4>(row);
            a11 = tmp[0];
            a21 = tmp[1];
            givens_params(a11, a21, c, s);
            for (unsigned int b = 0; b < col_batch; b++) {
                aie::vector<float, 4> r_v = readincr_v<4>(row);
                aie::vector<float, 4> cr_v = aie::mul(r_v, c);
                aie::vector<float, 4> sr_v = aie::mul(r_v, s);
                writeincr(c_row, cr_v);
                writeincr(s_row, sr_v);
            }
        }
    }
}

void vec_scalar_mul_csh(input_stream<float>* row,
                        output_stream<float>* c_row,
                        output_stream<float>* s_row,
                        unsigned int row_num,
                        unsigned int column_num) {
    unsigned int col_batch = column_num / 4;
    for (unsigned int j = 0; j < column_num; j++) {
        for (unsigned int i = row_num - 1; i > j; i--) {
            float a11, a21, c, s;
            aie::vector<float, 4> tmp = readincr_v<4>(row);
            a11 = tmp[0];
            a21 = tmp[1];
            givens_params(a11, a21, c, s);
            {
                /*
                    tmp[2] = c;
                    tmp[3] = s;
                    aie::print(tmp, true, "first batch");
                    */
            }
            for (unsigned int b = 0; b < col_batch; b++) {
                aie::vector<float, 4> r_v = readincr_v<4>(row);
                aie::vector<float, 4> cr_v = aie::mul(r_v, c);
                aie::vector<float, 4> sr_v = aie::mul(r_v, s);
                writeincr(c_row, cr_v);
                writeincr(s_row, sr_v);
            }
        }
    }
}

/*
void qrd_givens_rotation(input_stream<float>*  lower_row,
                         input_stream<float>*  higher_row,
                         output_stream<float>*  update_lower_row,
                         output_stream<float>*  update_higher_row,
                         unsigned int row_num,
                         unsigned int column_num) {
    unsigned int col_batch = column_num / 4;
    for (unsigned int j = 0; j < column_num; j++) {
        for (unsigned int i = row_num - 1; i > j; i--) {
            float a11, a21;
            a11 = readincr(lower_row);
            a21 = readincr(lower_row);
            // dummy value in lower and higher stream, to pack to v4float
            readincr(lower_row);
            readincr(lower_row);
            readincr(higher_row);
            readincr(higher_row);
            readincr(higher_row);
            readincr(higher_row);
            float c, s;
            givens_params(a11, a21, c, s);
            for (unsigned int b = 0; b < col_batch; b++) {
                aie::vector<float, 4> l_v = readincr_v<4>(lower_row);
                aie::vector<float, 4> h_v = readincr_v<4>(higher_row);
                aie::vector<float, 4> cl_v = aie::mul(l_v, c);
                aie::vector<float, 4> sl_v = aie::mul(l_v, s);
                aie::vector<float, 4> ch_v = aie::mul(h_v, c);
                aie::vector<float, 4> sh_v = aie::mul(h_v, s);
                aie::vector<float, 4> ul_v = aie::add(cl_v, sh_v);
                aie::vector<float, 4> uh_v = aie::sub(ch_v, sl_v);
                writeincr(update_lower_row, ul_v);
                writeincr(update_higher_row, uh_v);
            }
        }
    }
}
*/
}
}
