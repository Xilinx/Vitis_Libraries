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

#ifndef _XF_SOLVER_SVD_KERNEL_HPP_
#define _XF_SOLVER_SVD_KERNEL_HPP_
#include <aie_api/aie.hpp>
#include <adf.h>

namespace xf {
namespace solver {

template <int ROW, int COL, int KN>
class OneSidedJacobiComplexFloat {
   public:
    OneSidedJacobiComplexFloat(int total_column, int total_row, int current_column) {
        column_num = total_column;
        row_num = total_row;
        column_id = current_column;
    }

    static void registerKernelClass() { REGISTER_FUNCTION(OneSidedJacobiComplexFloat::process); }

    void process(input_stream_cfloat* in_0,
                 input_stream_cfloat* in_1,
                 output_stream_cfloat* out_0,
                 output_stream_cfloat* out_1);

   public:
    int column_num;
    int row_num;
    int column_id;
};
} // namespace solver
} // namespace xf
#endif
