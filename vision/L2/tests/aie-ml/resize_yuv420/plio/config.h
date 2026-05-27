/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2025 Advanced Micro Devices, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __CONFIG_RESIZE_YUV420_H_
#define __CONFIG_RESIZE_YUV420_H_
#include <common/xf_aie_const.hpp>
#include <cmath>

// tile dimensions are normally computed by tiler but we need to
// hardcode these values to set the graph window sizes
static constexpr int NUM_TILES = 1;
using DATA_TYPE = uint8_t;

static constexpr int NO_CORES_PER_COL = 2;
static constexpr int NO_CORES_PER_COL_Y = 4;

static constexpr int NO_COLS = 1;
/********  UV  *********** */
static constexpr int IMAGE_WIDTH_IN_UV = 1920;  
static constexpr int IMAGE_HEIGHT_IN_UV = 1080; 
static constexpr int IMAGE_WIDTH_OUT_UV = 1728; 
static constexpr int IMAGE_HEIGHT_OUT_UV = 972; 
static constexpr int METADATA_SIZE = xf::cv::aie::METADATA_SIZE;

static constexpr int TILE_WIDTH_IN_UV = IMAGE_WIDTH_IN_UV; 
static constexpr int TILE_HEIGHT_IN_UV = 2;
static constexpr int TILE_WIDTH_OUT_UV = IMAGE_WIDTH_OUT_UV; 
static constexpr int TILE_HEIGHT_OUT_UV = 1;

static constexpr int CHANNELS_UV = 2;
static constexpr int TILE_ELEMENTS_IN_UV = (TILE_WIDTH_IN_UV * TILE_HEIGHT_IN_UV * CHANNELS_UV);
static constexpr int TILE_WINDOW_SIZE_IN_UV = (TILE_ELEMENTS_IN_UV * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;

static constexpr int TILE_ELEMENTS_OUT_UV = (TILE_WIDTH_OUT_UV * TILE_HEIGHT_OUT_UV * CHANNELS_UV);
static constexpr int TILE_WINDOW_SIZE_OUT_UV = (TILE_ELEMENTS_OUT_UV * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;

/********* Y       ****************/
static constexpr int IMAGE_HEIGHT_IN = 2160;
static constexpr int IMAGE_WIDTH_IN = 3840;

static constexpr int IMAGE_HEIGHT_OUT = 1944;
static constexpr int IMAGE_WIDTH_OUT = 3456;

static constexpr int TILE_WIDTH_IN = IMAGE_WIDTH_IN;
static constexpr int TILE_HEIGHT_IN = 2;
static constexpr int TILE_WIDTH_OUT = IMAGE_WIDTH_OUT;
static constexpr int TILE_HEIGHT_OUT = 1;

static constexpr int CHANNELS_Y = 1;

static constexpr int TILE_ELEMENTS_IN_Y = (TILE_WIDTH_IN * TILE_HEIGHT_IN * CHANNELS_Y);
static constexpr int TILE_WINDOW_SIZE_IN_Y = (TILE_ELEMENTS_IN_Y * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;

static constexpr int TILE_ELEMENTS_OUT_Y = (TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS_Y);
static constexpr int TILE_WINDOW_SIZE_OUT_Y = (TILE_ELEMENTS_OUT_Y * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;

static constexpr int __X86_DEVICE__ = 0;
/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 32;

template <int FBITS>
uint32_t compute_scalefactor(int M, int N) {
    float x_scale = (float)M / (float)N;
    float scale = x_scale * (1 << FBITS);
    return (uint32_t)(std::roundf(scale));
}

#endif //__CONFIG_H_
