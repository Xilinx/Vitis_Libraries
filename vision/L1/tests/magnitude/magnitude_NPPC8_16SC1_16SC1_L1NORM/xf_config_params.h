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
#define HEIGHT 128
#define WIDTH 128

#define XF_CV_DEPTH_IN1 2
#define XF_CV_DEPTH_IN2 2
#define XF_CV_DEPTH_OUT 2

#define NPPCX XF_NPPC8

#define IN_TYPE XF_16SC1
#define OUT_TYPE XF_16SC1

#define NORM_TYPE XF_L1NORM

#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

void magnitude_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp1,
                     ap_uint<INPUT_PTR_WIDTH>* img_inp2,
                     ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                     int rows,
                     int cols);

#endif
// end of _XF_MAGNITUDE_CONFIG_H_