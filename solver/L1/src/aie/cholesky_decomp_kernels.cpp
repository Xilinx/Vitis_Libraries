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

#include "aie/cholesky_decomp_kernels.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/adf/stream.hpp"

namespace xf {
namespace solver {

void cholesky_decomp(input_stream<float>* __restrict l_row,
                     input_stream<float>* __restrict h_row,
                     output_stream<float>* __restrict update_row,
                     unsigned int row_num) {
    float diag;

    for (int j = 0; j < row_num; j++) {
        for (int i = 0; i < j; i++) {
            float res = 0;
            writeincr(update_row, res);
        }
        for (int i = j; i < j + 1; i++) chess_prepare_for_pipelining {
                float tmp = 0;
                for (int b = 0; b < j; b++) {
                    float l_v = readincr(l_row);
                    float h_v = readincr(h_row);
                    tmp += l_v * h_v;
                }

                float res = readincr(h_row);
                res = aie::sqrt(res - tmp);
                diag = aie::inv(res);
                writeincr(update_row, res);
            }
        for (int i = j + 1; i < row_num; i++) {
            float tmp = 0;
            for (int b = 0; b < j; b++) {
                float l_v = readincr(l_row);
                float h_v = readincr(h_row);
                tmp += l_v * h_v;
            }

            float res = readincr(h_row);
            res = (res - tmp) * diag;
            writeincr(update_row, res);
        }
    }
}
} // namespace solver
} // namespace xf
