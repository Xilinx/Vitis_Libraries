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
#ifndef _XF_CLAHE_CONFIG_HPP_
#define _XF_CLAHE_CONFIG_HPP_
#include "imgproc/xf_clahe.hpp"

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

/*  User configurable parameters */
static constexpr int IN_TYPE = XF_8UC1;
static constexpr int OUT_TYPE = XF_8UC1;
static constexpr int HEIGHT = 2160;
static constexpr int WIDTH = 3840;
static constexpr int NPPCX = XF_NPPC1;
static constexpr int CLIPLIMIT = 32;
static constexpr int TILES_Y_MIN = 2;
static constexpr int TILES_X_MIN = 2;
static constexpr int TILES_Y_MAX = 8;
static constexpr int TILES_X_MAX = 8;

static constexpr int INPUT_PTR_WIDTH = 512;
static constexpr int OUTPUT_PTR_WIDTH = 512;
extern "C" {
void clahe_accel(ap_uint<INPUT_PTR_WIDTH>* in_ptr,
                 ap_uint<OUTPUT_PTR_WIDTH>* out_ptr,
                 int height,
                 int width,
                 int clip = 40,
                 int tilesY = 8,
                 int tilesX = 8);
}

#endif