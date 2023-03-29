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

#ifndef _XF_SGBM_CONFIG_H_
#define _XF_SGBM_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_sgbm.hpp"

#define HEIGHT 1080
#define WIDTH 1920

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_OUT 2
/* set penalties for SGM */
#define SMALL_PENALTY 20
#define LARGE_PENALTY 40

/* Census transform window size */
#define WINDOW_SIZE 5

/* NO_OF_DISPARITIES must be greater than '0' and less than the image width */
#define TOTAL_DISPARITY 64

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 32

/* Number of directions */
#define NUM_DIR 4

// Input and output depths
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

// Set the optimization type to NPPC1
// fixed config
#define NPPCX XF_NPPC1

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

#endif
// end of _XF_SGBM_CONFIG_H_
