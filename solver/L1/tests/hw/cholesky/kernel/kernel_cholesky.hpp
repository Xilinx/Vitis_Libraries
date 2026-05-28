/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef _KERNEL_CHOLESKY_HPP_
#define _KERNEL_CHOLESKY_HPP_

#include "dut_type.hpp"
#include "hls_stream.h"

const unsigned DIM = MATRIX_DIM;
const bool LOWER_TRIANGULAR = MATRIX_LOWER_TRIANGULAR;

extern "C" int kernel_cholesky_0(hls::stream<MATRIX_IN_T>& matrixAStrm, hls::stream<MATRIX_OUT_T>& matrixLStrm);

#endif
