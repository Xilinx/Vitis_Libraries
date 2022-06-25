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

#ifndef HOST_CLUSTER_HISTOGRAM_HPP
#define HOST_CLUSTER_HISTOGRAM_HPP

#include <iostream>
#include <sys/time.h>

#ifndef HLS_TEST
#include "xcl2.hpp"
#include "xf_utils_sw/logger.hpp"

const int PIXEL_W = 2048;
const int PIXEL_H = 2048;
const int FRAME_DIM = 3;
const int ALL_PIXEL = PIXEL_W * PIXEL_H * FRAME_DIM;
const int BLOCK8_W = PIXEL_W / 8;
const int BLOCK8_H = PIXEL_H / 8;
const int BLOCK8_NUM = BLOCK8_W * BLOCK8_H * FRAME_DIM;
const int TILE_W = PIXEL_W / 64;
const int TILE_H = PIXEL_H / 64;
const int MAX_ORDER = 320 * 3 + 1;
const int MAX_NUM_CONFIG = 32;

#else
#include "hls_lossy_enc_compute.hpp"
#endif

void hls_lossy_enc_compute_wrapper(std::string xclbinPath,
                                   int config[MAX_NUM_CONFIG],
                                   float config_fl[MAX_NUM_CONFIG],
                                   float* hls_opsin_1,
                                   float* hls_opsin_2,
                                   float* hls_opsin_3,
                                   float* quant_field_row,
                                   float* masking_field_row,
                                   float* aq_map_f,
                                   int8_t* cmap_axi,
                                   int* ac_coef_axiout,
                                   uint8_t* strategy_all,
                                   int* raw_quant_field_i,
                                   uint32_t* hls_order,
                                   float* hls_dc8x8,
                                   float* hls_dc16x16,
                                   float* hls_dc32x32);
#endif
