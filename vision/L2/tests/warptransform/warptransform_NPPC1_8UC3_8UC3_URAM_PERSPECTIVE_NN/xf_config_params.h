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

#ifndef __XF_TRANSFORM_CONFIG__
#define __XF_TRANSFORM_CONFIG__
#include <ap_int.h>
#include <cmath>
#include <iostream>
#include <math.h>
#include <iostream>
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_warp_transform.hpp"

// Number of rows in the input image
#define HEIGHT 2160
// Number of columns in  in the input image
#define WIDTH 3840
#define XF_CV_DEPTH_OUT 2
#define XF_CV_DEPTH_IN 2
// Number of rows of input image to be stored
#define NUM_STORE_ROWS 100
// Number of rows of input image after which output image processing must start
#define START_PROC 50

#define RGB 1
#define GRAY 0
// transform type 0-NN 1-BILINEAR
#define INTERPOLATION 0

// transform type 0-AFFINE 1-PERSPECTIVE
#define TRANSFORM_TYPE 1
#define XF_USE_URAM 1

#define BUILD_TRANSFORM_MATRIX 1
// Set the pixel depth:

#define RGB 1
#define GRAY 0

// Set the optimization type:
#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

#endif
