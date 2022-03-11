/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef COMMON_HPP_
#define COMMON_HPP_
#define MAX_READ_LEN 192
#define MAX_HAP_LEN 1024
#define MAX_RSDATA_NUM 2048 // default 2048, minimum 32, maximum 4096
#define MAX_HAPDATA_NUM 128 // default 128, minimum 8, maximum 256
#define DEP_DIST 42
#define READ_BLOCK_SIZE 2
#define HAP_BLOCK_SIZE 4
#define FPGA_PERF 16.5
#define AVX_PERF 0.3
// this is the pipeline stages of the computeEngine pipeline, modify this based on that shown in HLS report
#endif
