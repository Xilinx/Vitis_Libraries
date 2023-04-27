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
#include "xf_config_params.h"

/* config width and height */
#define XF_HEIGHT 1080
#define XF_WIDTH 1920

#define XF_CV_DEPTH_MAT_L 2
#define XF_CV_DEPTH_MAT_R 2
#define XF_CV_DEPTH_MAT_DISP 2
#define XF_CV_DEPTH_MAP_XL 2
#define XF_CV_DEPTH_MAP_YL 2
#define XF_CV_DEPTH_MAP_XR 2
#define XF_CV_DEPTH_MAP_YR 2
#define XF_CV_DEPTH_LEFT_REMAPPED 2
#define XF_CV_DEPTH_RIGHT_REMAPPED 2

#define NO_OF_DISPARITIES 48

/* NO_OF_DISPARITIES must not be lesser than PARALLEL_UNITS and NO_OF_DISPARITIES/PARALLEL_UNITS must be a
 * non-fractional number */
#define PARALLEL_UNITS 16

/* SAD window size must be an odd number and it must be less than minimum of image height and width and less than the
 * tested size '21' */
#define SAD_WINDOW_SIZE 15

// Configure this based on the number of rows needed for Remap function
#define XF_REMAP_BUFSIZE 128

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

#define XF_USE_URAM false

#define XF_CAMERA_MATRIX_SIZE 9
#define XF_DIST_COEFF_SIZE 5
#define XF_NPPCX XF_NPPC1
#define IN_TYPE ap_uint<8>
#define OUT_TYPE ap_uint<16>

#endif // _XF_STEREO_PIPELINE_CONFIG_H_
