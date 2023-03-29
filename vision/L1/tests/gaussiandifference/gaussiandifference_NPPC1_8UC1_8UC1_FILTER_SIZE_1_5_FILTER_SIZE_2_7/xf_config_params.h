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

#ifndef _XF_GAUSSIAN_FILTER_CONFIG_H_
#define _XF_GAUSSIAN_FILTER_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_delay.hpp"
#include "core/xf_arithm.hpp"
#include "xf_config_params.h"

#define WIDTH 128
#define HEIGHT 128

#define XF_CV_DEPTH_IN_0 2
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_IN_3 15360
#define XF_CV_DEPTH_IN_4 2
#define XF_CV_DEPTH_OUT_1 2

#define MAXDELAY 15360

// Resolve optimization type

#define FILTER_SIZE_1_3 0
#define FILTER_SIZE_1_5 1
#define FILTER_SIZE_1_7 0
#define FILTER_SIZE_2_3 0
#define FILTER_SIZE_2_5 0
#define FILTER_SIZE_2_7 1

#if FILTER_SIZE_1_3
#define FILTER_WIDTH_1 3
#define FILTER_1 3
#elif FILTER_SIZE_1_5
#define FILTER_WIDTH_1 5
#define FILTER_1 5
#elif FILTER_SIZE_1_7
#define FILTER_WIDTH_1 7
#define FILTER_1 7
#endif

#if FILTER_SIZE_2_3
#define FILTER_WIDTH_2 3
#define FILTER_2 3
#elif FILTER_SIZE_2_5
#define FILTER_WIDTH_2 5
#define FILTER_2 5
#elif FILTER_SIZE_2_7
#define FILTER_WIDTH_2 7
#define FILTER_2 7
#endif

#define GRAY 1

#define T_8U 1

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC1

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 8

void gaussian_diff_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                         float sigma1,
                         float sigma2,
                         ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                         int rows,
                         int cols);
#endif
//_XF_GAUSSIAN_FILTER_CONFIG_H_
