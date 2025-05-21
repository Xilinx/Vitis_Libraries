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

#ifndef __CONFIG_H_
#define __CONFIG_H_
#include <common/xf_aie_const.hpp>

// tile dimensions are normally computed by tiler but we need to
// hardcode these values to set the graph window sizes
static constexpr int NUM_TILES = 1;
using DATA_TYPE = uint8_t;
static constexpr int TILE_WIDTH = 1920;
static constexpr int TILE_HEIGHT = 2;
static constexpr int TILE_ELEMENTS = TILE_WIDTH * TILE_HEIGHT;
static constexpr int TILE_WINDOW_SIZE = TILE_ELEMENTS * sizeof(DATA_TYPE);
static constexpr int TILE_WINDOW_SIZE_UV = TILE_WINDOW_SIZE / 2;
static constexpr int TILE_WINDOW_SIZE_RGBA = TILE_WINDOW_SIZE * 4;

static constexpr int ELEM_WITH_METADATA_Y = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_UV = (TILE_ELEMENTS / 2) + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_OUT = (4 * TILE_ELEMENTS) + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int __X86_DEVICE__ = 0;
/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 32;

#endif //__CONFIG_H_
