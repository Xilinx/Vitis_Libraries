/*
 * Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
 * Copyright 2023-2024 Advanced Micro Devices, Inc. All Rights Reserved.
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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <common/xf_aie_const.hpp>
using DATA_TYPE = uint8_t;

// To be set at compile time
static constexpr int NO_CORES = 11;
static constexpr int IMAGE_WIDTH = 1920;
static constexpr int IMAGE_HEIGHT = 1080;
static constexpr int TILE_WINSIZE = 5;
static constexpr int NO_DISPARITIES = 112;
static constexpr int TILE_IN_HEIGHT = IMAGE_HEIGHT;
static constexpr int UNIQUENESS_RATIO = 15;
static constexpr int TEXTURE_THRESHOLD = 20;
static constexpr int FILTERED = 0;
static constexpr int COLS_COMBINE = 64 - (TILE_WINSIZE - 1);
static constexpr int TILE_IN_HEIGHT_WITH_WINDOW = TILE_IN_HEIGHT + TILE_WINSIZE;
static constexpr int LEFT_TILE_IN_WIDTH = 64;
static constexpr int V_SHIFT_SOBEL = TILE_IN_HEIGHT - 2;
static constexpr int RIGHT_TILE_IN_WIDTH = 64 + NO_DISPARITIES;
static constexpr int TILE_OUT_HEIGHT = TILE_IN_HEIGHT - TILE_WINSIZE + 1;
static constexpr int TILE_OUT_WIDTH = COLS_COMBINE;
static constexpr int V_SHIFT = TILE_OUT_HEIGHT;
static constexpr int VECTORIZATION_FACTOR = 64;
static constexpr int SIZEOF_INT16 = 2;
static constexpr int LOVERLAP = (LEFT_TILE_IN_WIDTH - COLS_COMBINE);
static constexpr int ROVERLAP = (RIGHT_TILE_IN_WIDTH - COLS_COMBINE);

static constexpr int NO_CORES_SOBEL = 2;
static constexpr int TILE_WINSIZE_SOBEL = 3;
static constexpr int TILE_IN_HEIGHT_SOBEL = 542;
static constexpr int TILE_IN_WIDTH_SOBEL = 32;
static constexpr int TILE_OUT_WIDTH_SOBEL = 32 + 240 + 32;
static constexpr int TILE_OUT_HEIGHT_SOBEL = 540 + TILE_WINSIZE + 1;
static constexpr int TILE_WIDTH = TILE_IN_WIDTH_SOBEL;
static constexpr int TILE_HEIGHT = TILE_IN_HEIGHT_SOBEL;

static constexpr int __X86_DEVICE__ = 0;

#endif //__CONFIG_H_
