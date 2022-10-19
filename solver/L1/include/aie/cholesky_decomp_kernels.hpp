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
#ifndef _CHOLESKY_DECOMP_KERNELS_HPP_
#define _CHOLESKY_DECOMP_KERNELS_HPP_

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "adf/stream/streams.h"
#include <adf.h>

namespace xf {
namespace solver {
void cholesky_decomp(input_stream<float>* __restrict l_row,
                     input_stream<float>* __restrict h_row,
                     output_stream<float>* __restrict update_row,
                     unsigned int row_num);
} // namespace solver
} // namespace xf
#endif
