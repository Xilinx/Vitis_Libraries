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

#ifndef _XF_THRESHOLD_CONFIG_H_
#define _XF_THRESHOLD_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_threshold.hpp"

#include "imgproc/xf_median_blur.hpp"

#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_otsuthreshold.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_cca_custom_imp.hpp"
#include "imgproc/xf_custom_bgr2y8.hpp"
/*#include "imgproc/xf_cvt_color.hpp"*/

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

typedef ap_uint<8> ap_uint8_t;
typedef ap_uint<64> ap_uint64_t;

/*  set the height and weight */
#define HEIGHT 1080
#define WIDTH 1920
#define STRIDE 2048

/*  set the type of thresholding*/
#define THRESH_TYPE XF_THRESHOLD_TYPE_BINARY_INV

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

/* Median kernel smoothening parameter */
#define WINDOW_SIZE 3

/* Filter width for canny */
#define FILTER_WIDTH 3

#define L1NORM 1
#define L2NORM 0

#define XF_USE_URAM 0

#define _MONO 0
#define _COLOR 1

// Set the pixel depth:
#if _MONO
#define OTSU_PIXEL_TYPE XF_8UC1
#elif _COLOR
#define OTSU_PIXEL_TYPE XF_8UC3
#endif

/* Gaussian filter params */
#define FILTER_SIZE_3 1
#define FILTER_SIZE_5 0
#define FILTER_SIZE_7 0

#if _MONO
#define GAUSSIAN_INPUT_PTR_WIDTH 8
#elif _COLOR
#define GAUSSIAN_INPUT_PTR_WIDTH 32
#endif

#define GAUSSIAN_OUTPUT_PTR_WIDTH 8
/* Gaussian filter Param ends  */

#define XF_NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#if L1NORM
#define NORM_TYPE XF_L1NORM
#elif L2NORM
#define NORM_TYPE XF_L2NORM
#endif

#endif // end of _XF_THRESHOLD_CONFIG_H_
