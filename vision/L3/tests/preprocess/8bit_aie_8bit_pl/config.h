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

static constexpr int IMAGE_WIDTH_IN = 1920;
static constexpr int IMAGE_HEIGHT_IN = 1080;
static constexpr int IMAGE_WIDTH_OUT = 300;
static constexpr int IMAGE_HEIGHT_OUT = 300;

static constexpr int CROP_X = 1;
static constexpr int CROP_Y = 1;
static constexpr int CROP_WT = 1920;
static constexpr int CROP_HT = 1080;

static constexpr int TILE_WIDTH = CROP_WT;
static constexpr int TILE_HEIGHT = 2;
static constexpr int TILE_ELEMENTS = TILE_WIDTH * TILE_HEIGHT;
static constexpr int TILE_WINDOW_SIZE = TILE_ELEMENTS * sizeof(DATA_TYPE);
static constexpr int TILE_WINDOW_SIZE_UV = TILE_WINDOW_SIZE;
static constexpr int TILE_WINDOW_SIZE_RGBA = TILE_WINDOW_SIZE * 4;

static constexpr int ELEM_WITH_METADATA_Y = TILE_ELEMENTS + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_UV = (TILE_ELEMENTS) + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_OUT = (4 * TILE_ELEMENTS) + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));

static constexpr int TILE_WIDTH_IN = CROP_WT;
static constexpr int TILE_HEIGHT_IN = 2;

static constexpr int TILE_WIDTH_OUT = IMAGE_WIDTH_OUT;
static constexpr int TILE_HEIGHT_OUT = 1;

static constexpr int CHANNELS = 4;
static constexpr int TILE_ELEMENTS_IN = (TILE_WIDTH_IN * TILE_HEIGHT_IN * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_IN = (TILE_ELEMENTS_IN * sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_IN = TILE_ELEMENTS_IN + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));

static constexpr int TILE_ELEMENTS_OUT = (TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_OUT = (TILE_ELEMENTS_OUT * sizeof(DATA_TYPE));
static constexpr int ELEM_WITH_METADATA_RESIZED_OUT =
    (TILE_ELEMENTS_OUT) + (xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE));

/* Graph specific configuration */
static constexpr int VECTORIZATION_FACTOR = 32;

static constexpr int __X86_DEVICE__ = 0;

#endif //__CONFIG_H_
