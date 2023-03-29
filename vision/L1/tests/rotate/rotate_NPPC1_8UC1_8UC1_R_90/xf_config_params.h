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

#ifndef _XF_ROTATE_CONFIG_H_
#define _XF_ROTATE_CONFIG_H_

#include "hls_stream.h"
#include <ap_int.h>

#include "common/xf_common.hpp"
#include "common/xf_structs.hpp"

#include "imgproc/xf_rotate.hpp"

/* Set the image height and width */
#define HEIGHT 512
#define WIDTH 512

#define TILE_SIZE 32

/* Image type */
#define RGB 0
#define GRAY 1

#define R_90 1
#define R_180 0
#define R_270 0
#define NPPCX XF_NPPC1
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC1

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 8

void rotate_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_in, ap_uint<OUTPUT_PTR_WIDTH>* img_out, int height, int width, int direction);

#endif
//_XF_ROTATE_CONFIG_H_
