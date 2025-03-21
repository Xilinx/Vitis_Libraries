/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef _XF_STEREO_PIPELINE_CONFIG_H_
#define _XF_STEREO_PIPELINE_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_stereo_pipeline.hpp"
#include "imgproc/xf_remap.hpp"
#include "imgproc/xf_stereolbm.hpp"

/* config width and height */
#define XF_HEIGHT 720
#define XF_WIDTH 1280

#define XF_CV_DEPTH_L 2
#define XF_CV_DEPTH_R 2
#define XF_CV_DEPTH_disp 2
#define XF_CV_DEPTH_mapxL 2
#define XF_CV_DEPTH_mapyL 2
#define XF_CV_DEPTH_mapxR 2
#define XF_CV_DEPTH_mapyR 2
#define XF_CV_DEPTH_leftRemapped 2
#define XF_CV_DEPTH_rightRemapped 2

#define NPPCX XF_NPPC1

#define XF_CAMERA_MATRIX_SIZE 9
#define XF_DIST_COEFF_SIZE 5

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_16UC1
#define MAP_TYPE XF_32SC1
#define REMAP_TYPE XF_8UC1

#define XF_USE_URAM 0

#define NO_OF_DISPARITIES 48

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 16

/* SAD window size must be an odd number and it must be less than minimum of image height and width and less than the
 * tested size '21' */
#define SAD_WINDOW_SIZE 15

// Configure this based on the number of rows needed for Remap function
#define XF_REMAP_BUFSIZE 128

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 16

void stereopipeline_accel(ap_uint<INPUT_PTR_WIDTH>* img_L,
                          ap_uint<INPUT_PTR_WIDTH>* img_R,
                          ap_uint<OUTPUT_PTR_WIDTH>* img_disp,
                          float* cameraMA_l,
                          float* cameraMA_r,
                          float* distC_l,
                          float* distC_r,
                          float* irA_l,
                          float* irA_r,
                          int* bm_state_arr,
                          int rows,
                          int cols);

#endif
// _XF_STEREO_PIPELINE_CONFIG_H_