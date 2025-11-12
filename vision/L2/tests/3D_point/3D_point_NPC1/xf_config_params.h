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

#ifndef _XF_REPROJECTIMAGETO3D_CONFIG_H_
#define _XF_REPROJECTIMAGETO3D_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_reproject3D.hpp"

/*typedef float param_T;
param_T cameraMA_l[9] = {933.1730000000, 0.0000000000, 663.4510000000, 0.0000000000, 933.1730000000,
                         377.0150000000, 0.0000000000, 0.0000000000,   1.0000000000};

param_T cameraMA_r[9] = {933.4670000000, 0.0000000000, 678.2970000000, 0.0000000000, 933.4670000000,
                         359.6230000000, 0.0000000000, 0.0000000000,   1.0000000000};

param_T distC_l[5] = {-0.1693980000, 0.0227329000, 0.0000000000, 0.0000000000, 0.0000000000};

param_T distC_r[5] = {-0.1705810000, 0.0249444000, 0.0000000000, 0.0000000000, 0.0000000000};

param_T irA_l[9] = {0.0011976323,  -0.0000000019, -0.8153011732, 0.0000000007, 0.0011976994,
                    -0.4422348617, 0.0000126839,  0.0000001064,  0.9913820905};

param_T irA_r[9] = {0.0011976994,  0.0000000000,  -0.8047567905, -0.0000000000, 0.0011976994,
                    -0.4420566166, -0.0000000000, -0.0000001064, 1.0000392898};
*/

/* config width and height */
#define WIDTH 1280
#define HEIGHT 720

#define XF_CV_DEPTH_IN_L 2
#define XF_CV_DEPTH_IN_R 2
#define XF_CV_DEPTH_OUT 2

#define SAD_WINDOW_SIZE 11

/* NO_OF_DISPARITIES must be greater than '0' and less than the image width */
#define NO_OF_DISPARITIES 32

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 32

#define XF_USE_URAM 0

// Set the input and output pixel depth:
#define IN_TYPE XF_16SC1
#define OUT_TYPE XF_16SC3

// Set the optimization type:
#define NPPCX XF_NPPC1

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

#endif
// _XF_STEREOBM_CONFIG_H_
