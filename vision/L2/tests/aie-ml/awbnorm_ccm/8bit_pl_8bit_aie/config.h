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
using T = uint8_t;

#define XF_CCM_TYPE 0

static constexpr int TILE_WIDTH = 480;
static constexpr int TILE_HEIGHT = 16;

static constexpr int TILE_ELEMENTS_IN = (TILE_WIDTH * TILE_HEIGHT * 4);
static constexpr int TILE_ELEMENTS_OUT = (TILE_WIDTH * TILE_HEIGHT * 4);

static constexpr int TILE_WINDOW_SIZE_IN = ((TILE_ELEMENTS_IN * sizeof(T)) + xf::cv::aie::METADATA_SIZE);
static constexpr int ELEM_WITH_METADATA_IN = TILE_ELEMENTS_IN + (xf::cv::aie::METADATA_SIZE / sizeof(T));

static constexpr int TILE_WINDOW_SIZE_OUT = ((TILE_ELEMENTS_OUT * sizeof(T)) + xf::cv::aie::METADATA_SIZE);
static constexpr int ELEM_WITH_METADATA_OUT = TILE_ELEMENTS_OUT + (xf::cv::aie::METADATA_SIZE / sizeof(T));

/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 32;

#endif //__CONFIG_H_
