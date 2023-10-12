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
#ifndef __CHOLESKY_COMPLEX_HPP__
#define __CHOLESKY_COMPLEX_HPP__

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "adf/stream/streams.h"
#include <adf.h>

void cholesky_complex(input_stream<float>* __restrict matA_real,
                      input_stream<float>* __restrict matA_imag,
                      output_stream<float>* __restrict matL_real,
                      output_stream<float>* __restrict matL_imag);
#endif
