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

#ifndef __CONFIG_MEAN_400_H_
#define __CONFIG_MEAN_400_H_

#include <common/xf_aie_const.hpp>
#include <cmath>

using DATA_TYPE = uint8_t;
static constexpr int NO_CORES_PER_COL = 1;
static constexpr int NO_COLS = 1;

static constexpr int IMAGE_WIDTH_IN = 3840;
static constexpr int IMAGE_HEIGHT_IN = 2160;
static constexpr int IMAGE_WIDTH_OUT = 4;
static constexpr int IMAGE_HEIGHT_OUT = IMAGE_HEIGHT_IN / 2;
static constexpr int METADATA_SIZE = xf::cv::aie::METADATA_SIZE;

static constexpr int TILE_WIDTH_IN = IMAGE_WIDTH_IN;
static constexpr int TILE_HEIGHT_IN = 2;
static constexpr int TILE_WIDTH_OUT = 4; // 4 channels each float (8 int8)
static constexpr int TILE_HEIGHT_OUT = 1;

static constexpr int CHANNELS = 1;
static constexpr int TILE_ELEMENTS_IN = (TILE_WIDTH_IN * TILE_HEIGHT_IN * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_IN = (TILE_ELEMENTS_IN * sizeof(DATA_TYPE)) + xf::cv::aie::METADATA_SIZE;

static constexpr int TILE_ELEMENTS_OUT = (TILE_WIDTH_OUT * TILE_HEIGHT_OUT * CHANNELS);
static constexpr int TILE_WINDOW_SIZE_OUT = (TILE_ELEMENTS_OUT) + xf::cv::aie::METADATA_SIZE;

static constexpr int __X86_DEVICE__ = 0;

#endif //__CONFIG_H_
