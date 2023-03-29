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
#ifndef _XF_BOX_FILTER_CONFIG_H_
#define _XF_BOX_FILTER_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_box_filter.hpp"

/* set the height and width */
#define HEIGHT 2160
#define WIDTH 3840

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

#define FILTER_SIZE_3 0
#define FILTER_SIZE_5 0
#define FILTER_SIZE_7 1

#if FILTER_SIZE_3
#define FILTER_WIDTH 3
#elif FILTER_SIZE_5
#define FILTER_WIDTH 5
#elif FILTER_SIZE_7
#define FILTER_WIDTH 7
#endif

#define GRAY 1
#define RGB 0

#define T_8U 0
#define T_16U 0
#define T_16S 1

#define XF_USE_URAM 1

#define NPPCX XF_NPPC1

#define IN_TYPE XF_16SC1
#define OUT_TYPE XF_16SC1

#define CV_IN_TYPE CV_16SC1
#define CV_OUT_TYPE CV_16SC1

#define INPUT_PTR_WIDTH 16
#define OUTPUT_PTR_WIDTH 16

#endif
// end of _XF_BOX_FILTER_CONFIG_H_
