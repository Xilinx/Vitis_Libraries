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

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define LINES_SIZE 41
#define TRANSDUCERS_SIZE 128
#define SPACE_DIMENSIONS_SIZE 4
#define SIMD_DEPTH 4
#define UPSAMPLES_SIZE SIMD_DEPTH
#define RFDATA_SIZE 2048
#define SAMPLES_PROCESSED_AIE 32
#define BURSTS_DATA_FOR_AIE_ITERATIONS (RFDATA_SIZE / SAMPLES_PROCESSED_AIE)
