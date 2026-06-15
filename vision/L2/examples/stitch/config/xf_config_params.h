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

#ifndef _XF_STITCH_L2_PARAMS_H_
#define _XF_STITCH_L2_PARAMS_H_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_stitch.hpp"

#define HEIGHT_1 416
#define WIDTH_1 1312
#define HEIGHT_2 1056
#define WIDTH_2 416
#define HEIGHT_3 416
#define WIDTH_3 1312
#define HEIGHT_4 1056
#define WIDTH_4 416

#define HEIGHT_DST 1076
#define WIDTH_DST 1326

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC3
#define IN_MASK_TYPE XF_8UC1

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

#define SRC_PTR_WIDTH 32
#define MASK_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 32

#endif /* _XF_STITCH_L2_PARAMS_H_ */
