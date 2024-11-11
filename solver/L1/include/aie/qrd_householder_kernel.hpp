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
#ifndef __QRD_HOUSEHOLDER_HPP__
#define __QRD_HOUSEHOLDER_HPP__

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "adf/stream/streams.h"
#include <adf.h>

namespace xf {
namespace solver {

template <int M, int N>
void qrd_householder(input_stream<cfloat>* __restrict matAU_0,
                     input_stream<cfloat>* __restrict matAU_1,
                     output_stream<cfloat>* __restrict matRQ_0,
                     output_stream<cfloat>* __restrict matRQ_1,
                     const int column_id);
template <int M, int N>
void qrd_householder_last(input_stream<cfloat>* __restrict matAU_0,
                          input_stream<cfloat>* __restrict matAU_1,
                          output_stream<cfloat>* __restrict matRQ_0,
                          output_stream<cfloat>* __restrict matRQ_1,
                          const int column_id);

} // namespace solver
} // namespace xf
#endif
