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
#include "xf_config_params.h"
#include "common/xf_common.hpp"
#include "common/xf_structs.hpp"

#include "imgproc/xf_rotate.hpp"

/* Set the image height and width */
#define HEIGHT 512
#define WIDTH 512

#define TILE_SIZE 32

// Resolve the optimization type:
#if NO
#define NPC1 XF_NPPC1
#if GRAY
#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 8
#else
#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32
#endif
#endif
#if RO
#define NPC1 XF_NPPC2
#if GRAY
#define INPUT_PTR_WIDTH 16
#define OUTPUT_PTR_WIDTH 16
#else
#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64
#endif
#endif

// Set the input and output pixel depth:
#if GRAY
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1
#else
#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3
#endif

void rotate_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_in, ap_uint<OUTPUT_PTR_WIDTH>* img_out, int height, int width, int direction);

#endif //_XF_ROTATE_CONFIG_H_
