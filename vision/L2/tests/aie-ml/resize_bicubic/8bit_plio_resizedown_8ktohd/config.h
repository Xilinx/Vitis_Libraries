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
// 7680x4320 -> 1920x1080
// resize 1st pass image_resolution
static constexpr int IMAGE_WIDTH_IN = 7680;
static constexpr int IMAGE_HEIGHT_IN = 4320;
static constexpr int IMAGE_WIDTH_OUT = 7680;
static constexpr int IMAGE_HEIGHT_OUT = 1080;

static constexpr int TILE_WIDTH_IN = 256;
static constexpr int TILE_HEIGHT_IN = (IMAGE_HEIGHT_IN / ((IMAGE_HEIGHT_OUT / 4)) + 4);
static constexpr int TILE_WIDTH_OUT = 256;
static constexpr int TILE_HEIGHT_OUT = 4;

static constexpr int CHANNELS = 4;
static constexpr int TILE_ELEMENTS_IN = (TILE_WIDTH_IN * TILE_HEIGHT_IN * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_IN = (TILE_ELEMENTS_IN * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;
static constexpr int TILE_ELEMENTS_OUT = (TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_OUT = (TILE_ELEMENTS_OUT * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;
;
// resize 2nd pass image_resolution
static constexpr int IMAGE_WIDTH_IN2 = 1080;
static constexpr int IMAGE_HEIGHT_IN2 = 7680;
static constexpr int IMAGE_WIDTH_OUT2 = 1080;
static constexpr int IMAGE_HEIGHT_OUT2 = 1920;

static constexpr int TILE_WIDTH_IN2 = 256;
static constexpr int TILE_HEIGHT_IN2 = (IMAGE_HEIGHT_IN2 / ((IMAGE_HEIGHT_OUT2 / 4)) + 4);
static constexpr int TILE_WIDTH_OUT2 = 256;
static constexpr int TILE_HEIGHT_OUT2 = 4;

static constexpr int TILE_ELEMENTS_IN2 = (TILE_WIDTH_IN2 * TILE_HEIGHT_IN2 * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_IN2 = (TILE_ELEMENTS_IN2 * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;
static constexpr int TILE_ELEMENTS_OUT2 = (TILE_WIDTH_OUT2 * TILE_HEIGHT_OUT2 * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_OUT2 = (TILE_ELEMENTS_OUT2 * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;
;

static constexpr int METADATA_SIZE = xf::cv::aie::METADATA_SIZE / sizeof(DATA_TYPE);

static constexpr int __X86_DEVICE__ = 0;

#define LUT_DEPTH 256

#endif //__CONFIG_H_
