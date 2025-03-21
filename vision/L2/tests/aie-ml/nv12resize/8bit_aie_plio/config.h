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
using DATA_TYPE = uint8_t;

static constexpr int Y_IN_TILE_WIDTH = 1920;
static constexpr int Y_IN_TILE_HEIGHT = 5;
static constexpr int Y_OUT_TILE_WIDTH = 384;
static constexpr int Y_OUT_TILE_HEIGHT = 1;

static constexpr int Y_IN_TILE_ELEMENTS = Y_IN_TILE_WIDTH * Y_IN_TILE_HEIGHT;
static constexpr int Y_OUT_TILE_ELEMENTS = Y_OUT_TILE_WIDTH * Y_OUT_TILE_HEIGHT;

static constexpr int ELEM_WITH_METADATA = Y_IN_TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int ELEM_OUT_WITH_METADATA = Y_OUT_TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));

static constexpr int UV_IN_TILE_WIDTH = 1920; // UV is 16bit
static constexpr int UV_IN_TILE_HEIGHT = 5;
static constexpr int UV_OUT_TILE_WIDTH = 384;
static constexpr int UV_OUT_TILE_HEIGHT = 1;

static constexpr int UV_IN_TILE_ELEMENTS = UV_IN_TILE_WIDTH * UV_IN_TILE_HEIGHT;
static constexpr int UV_OUT_TILE_ELEMENTS = UV_OUT_TILE_WIDTH * UV_OUT_TILE_HEIGHT;

static constexpr int UV_ELEM_WITH_METADATA = UV_IN_TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int UV_ELEM_OUT_WITH_METADATA =
    UV_OUT_TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));

/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 16;
static constexpr int __X86_DEVICE__ = 0;
#endif //__CONFIG_H_
