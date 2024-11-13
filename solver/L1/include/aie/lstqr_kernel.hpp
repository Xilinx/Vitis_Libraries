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
#ifndef __LSTQR_HPP__
#define __LSTQR_HPP__

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "adf/stream/streams.h"
#include <adf.h>

namespace xf {
namespace solver {

template <int M, int N, int K>
void lstqr(input_stream<cfloat>* __restrict matAB_0,
           input_stream<cfloat>* __restrict matAB_1,
           output_stream<cfloat>* __restrict matRC_0,
           output_stream<cfloat>* __restrict matRC_1,
           const int column_id);
template <int M, int N, int K>
void lstqr_last(input_stream<cfloat>* __restrict matAB_0,
                input_stream<cfloat>* __restrict matAB_1,
                output_stream<cfloat>* __restrict matRC_0,
                output_stream<cfloat>* __restrict matRC_1,
                const int column_id);

template <int M, int N, int K>
void transform(input_stream<cfloat>* __restrict in0,
               input_stream<cfloat>* __restrict in1,
               output_stream<float>* __restrict out0,
               output_stream<float>* __restrict out1);

template <int M, int N, int K>
void backSubstitution(input_stream<float>* __restrict matRC_0,
                      input_stream<float>* __restrict matRC_1,
                      output_stream<float>* __restrict matXC_0,
                      output_stream<float>* __restrict matXC_1,
                      const int column_id);

} // namespace solver
} // namespace xf
#endif
