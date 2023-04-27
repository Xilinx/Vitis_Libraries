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

#include <adf.h>
#include <adf/stream/types.h>
#include <adf/io_buffer/io_buffer.h>

#define PARALLEL_FACTOR_32b 8 // Parallelization factor for 32b operations (8x mults)
#define SRS_SHIFT 10          // SRS shift used can be increased if input data likewise adjusted)
#define IMAGE_SIZE 4096       // 256x16
#define MAX_KERNEL_SIZE 128

const int kernel_width = 3;
const int kernel_height = 3;

#ifdef INLINE
#define INLINE_DECL inline
#else
#define INLINE_DECL
#endif

void filter2D(adf::input_buffer<int32>& input, adf::output_buffer<int32>& output);
