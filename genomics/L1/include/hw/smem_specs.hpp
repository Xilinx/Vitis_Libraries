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
#ifndef _SMEM_SPECS_HPP_
#define _SMEM_SPECS_HPP_
#include "common.h"
constexpr auto c_occ_intv_shift = 7;
constexpr auto c_occ_interval = (1LL << c_occ_intv_shift);
constexpr auto c_occ_intv_mask = (c_occ_interval - 1);
constexpr auto c_min_seed_len = 19;
constexpr auto c_split_width = 10;
constexpr auto c_split_len = 28;
constexpr auto c_max_mem_intv = 20;
constexpr auto c_max_intv_alloc = 256;
constexpr auto c_seq_length = 256;
constexpr auto c_max_batch_size = 1024;
constexpr auto c_max_tile_size = 16;
constexpr auto c_batch_size = 16;
#endif
