/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef HOST_TOKINIT_HISTOGRAM_HPP
#define HOST_TOKINIT_HISTOGRAM_HPP

#include <iostream>
#include <sys/time.h>
#ifndef HLS_TEST
#include "xcl2.hpp"
#include "xf_utils_sw/logger.hpp"
const int PIXEL_W = 2048;
const int PIXEL_H = 2048;
const int FRAME_DIM = 3;
const int ALL_PIXEL = PIXEL_W * PIXEL_H * FRAME_DIM;
const int MAX_NUM_BLK88_W = PIXEL_W / 8;
const int MAX_NUM_BLK88_H = PIXEL_H / 8;
const int MAX_NUM_BLK88 = MAX_NUM_BLK88_W * MAX_NUM_BLK88_H;
const int MAX_ORDERS_SIZE = (3 * 64 + 3 * 64 + 3 * 256 + 3 * 1024);
const int MAX_QF_THRESH_SIZE = 256;
const int MAX_CTX_MAP_SIZE = 256;
const int MAX_AC_TOKEN_SIZE = ALL_PIXEL;
#else
#include "hls_init_histogram.hpp"
#endif

void hls_ANSinitHistogram_wrapper(std::string xclbinPath,
                                  int config[32],
                                  //====================
                                  int32_t* ac_coeff_ordered_ddr,
                                  int32_t* strategy_ddr,
                                  int32_t* qf_ddr,
                                  uint8_t* qdc_ddr,
                                  uint8_t* ctx_map,
                                  uint32_t* qf_thresholds,
                                  uint64_t* ac_tokens_ddr,
                                  //====================
                                  uint64_t* tokens0_ptr,
                                  uint64_t* tokens1_ptr,
                                  uint64_t* tokens2_ptr,
                                  uint64_t* tokens3_ptr,
                                  //====================
                                  int32_t* histograms0_ptr,
                                  uint32_t* histograms_size0_ptr,
                                  uint32_t* total_count0_ptr,
                                  uint32_t* nonempty0_ptr,
                                  //======================
                                  int32_t* histograms1_ptr,
                                  uint32_t* histograms_size1_ptr,
                                  uint32_t* total_count1_ptr,
                                  uint32_t* nonempty1_ptr,
                                  //======================
                                  int32_t* histograms2_ptr,
                                  uint32_t* histograms_size2_ptr,
                                  uint32_t* total_count2_ptr,
                                  uint32_t* nonempty2_ptr,
                                  //======================
                                  int32_t* histograms3_ptr,
                                  uint32_t* histograms_size3_ptr,
                                  uint32_t* total_count3_ptr,
                                  uint32_t* nonempty3_ptr,
                                  //======================
                                  int32_t* histograms4_ptr,
                                  uint32_t* histograms_size4_ptr,
                                  uint32_t* total_count4_ptr,
                                  uint32_t* nonempty4_ptr);

#endif
