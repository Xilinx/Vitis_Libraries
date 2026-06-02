/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_STITCH_CONFIG_H_
#define _XF_STITCH_CONFIG_H_

#include "hls_stream.h"
#include <ap_int.h>

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_stitch.hpp"

/* Must be >= max input image rows/cols and >= output ROI from roi_stich.
 * For stitch_remap 1312x416, 416x1056 etc. and ROI 1326x1076, use at least 1536x1920. */
#define HEIGHT 1080
#define WIDTH 1920

#define HEIGHT_1 416
#define WIDTH_1 1312
#define HEIGHT_2 1056
#define WIDTH_2 416
#define HEIGHT_3 416
#define WIDTH_3 1312
#define HEIGHT_4 1056
#define WIDTH_4 416

#define HEIGHT_DST 1056
#define WIDTH_DST 1312

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

// Resolve the optimization type:

#define NPPCX XF_NPPC1

#define GRAY 0
#define RGB 1

#if RGB == 1
#define IN_TYPE XF_8UC3
#define IN_MASK_TYPE XF_8UC1
#elif GRAY == 1
#define IN_TYPE XF_8UC1
#define IN_MASK_TYPE XF_8UC1
#endif

#define INPUT_PTR_WIDTH 32
#define MASK_INPUT_PTR_WIDTH 8

void stitch_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                  ap_uint<INPUT_PTR_WIDTH>* img_in2,
                  ap_uint<INPUT_PTR_WIDTH>* img_in3,
                  ap_uint<INPUT_PTR_WIDTH>* img_in4,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img1,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img2,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img3,
                  ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img4,
                  ap_uint<INPUT_PTR_WIDTH>* img_out,
                  int img_sizes[8],
                  int mask_corners[8],
                  int dst_height,
                  int dst_width);

#endif
//_XF_STITCH_CONFIG_H_
