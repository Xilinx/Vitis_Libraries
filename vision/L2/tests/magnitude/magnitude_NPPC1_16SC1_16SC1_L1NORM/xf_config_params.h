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

#ifndef _XF_MAGNITUDE_CONFIG_H_
#define _XF_MAGNITUDE_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "core/xf_magnitude.hpp"

typedef unsigned short int uint16_t;

/*  set the height and weight  */
#define HEIGHT 2160
#define WIDTH 3840

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC1

#define IN_TYPE XF_16SC1
#define OUT_TYPE XF_16SC1

#define NORM_TYPE XF_L1NORM

#define INPUT_PTR_WIDTH 16
#define OUTPUT_PTR_WIDTH 16

void magnitude_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX>& _src1,
                     xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPPCX>& _src2,
                     xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPPCX>& _dst);

#endif
// end of _XF_MAGNITUDE_CONFIG_H_