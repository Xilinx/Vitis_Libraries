/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
#ifndef _XF_AVFIRS_4STREAM_CONFIG_H_
#define _XF_AVFIRS_4STREAM_CONFIG_H_

#include "ap_int.h"
#include "hls_stream.h"

// #include "common/xf_axi_io.hpp"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "common/xf_infra.hpp"
#include "imgproc/xf_remap.hpp"
#include "imgproc/xf_resize.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_sobel.hpp"
#include "core/xf_magnitude.hpp"
#include "core/xf_convert_bitdepth.hpp"
#include "imgproc/xf_threshold.hpp"
#include "imgproc/xf_findcontours.hpp"
#include "imgproc/xf_stitch.hpp"
#include "imgproc/xf_waitMat.hpp"
#include "imgproc/xf_duplicateimage.hpp"

/* Set the image height and width */
#define HEIGHT 800
#define WIDTH 1280

#define HEIGHT_1 416
#define WIDTH_1 1312
#define HEIGHT_2 1056
#define WIDTH_2 416
#define HEIGHT_3 416
#define WIDTH_3 1312
#define HEIGHT_4 1056
#define WIDTH_4 416

#define HEIGHT_DST 1076
#define WIDTH_DST 1326

#define MAX_CONTOURS 4096
#define MAX_TOTAL_POINTS 10000

#define XF_COMB_DEPTH 2 //(HEIGHT * WIDTH * 4)
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2

// Remap parameters
#define XF_REMAP_INTERPOLATION_TYPE \
    1 // 0 - Nearest Neighbor Interpolation, 1 - Bilinear Interpolation, 2 - Bicubic Interpolation

// Mat types
#define MAPXY_TYPE XF_32SC1
// Configure this based on the number of rows needed for the remap purpose
// e.g., If its a right to left flip two rows are enough
#define XF_WIN_ROWS 800
#define BARREL 0

#define XF_USE_URAM_REMAP_1 1
#define XF_USE_URAM_REMAP_2 1
#define XF_USE_URAM_REMAP_3 1
#define XF_USE_URAM_REMAP_4 1
#define XF_USE_URAM_RESIZE 1
#define XF_USE_URAM_SOBEL 1

// Resize parameter
#define MAXDOWNSCALE 2
#define INTERPOLATION 1
// 0 - Nearest Neighbor Interpolation
// 1 - Bilinear Interpolation
// 2 - AREA Interpolation

// Gaussian filter parameters
#define GAUSSIAN_FILTER_WIDTH 5

// Sobel filter parameters
#define SOBEL_FILTER_WIDTH 3

// Magnitude parameters
#define NORM_TYPE XF_L2NORM

// Convert parameter
#define CONVERT_TYPE XF_CONVERT_16S_TO_8U

// Threshold parameters
#define THRESH_TYPE XF_THRESHOLD_TYPE_BINARY

// Set the optimization type:
#define XF_NPPCX XF_NPPC1

// Set the input and output pixel depth:
#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC1
#define OUT_TYPE_1 XF_16SC1
#define IN_MASK_TYPE XF_8UC1

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC3
#define CV_OUT_TYPE_1 CV_16SC1

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 8
#define OUTPUT1_PTR_WIDTH 16
#define MAPXY_TYPE_PTR_WIDTH 32
#define MASK_INPUT_PTR_WIDTH 8

#define ERROR_THRESHOLD 10
void AVFIRS_4stream_accel(ap_uint<INPUT_PTR_WIDTH>* img_in1,
                          ap_uint<INPUT_PTR_WIDTH>* img_in2,
                          ap_uint<INPUT_PTR_WIDTH>* img_in3,
                          ap_uint<INPUT_PTR_WIDTH>* img_in4,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x1,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y1,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x2,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y2,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x3,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y3,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_x4,
                          ap_uint<MAPXY_TYPE_PTR_WIDTH>* map_y4,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img1,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img2,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img3,
                          ap_uint<MASK_INPUT_PTR_WIDTH>* mask_img4,
                          ap_uint<INPUT_PTR_WIDTH>* points_packed,
                          ap_uint<INPUT_PTR_WIDTH>* contour_offsets,
                          ap_uint<INPUT_PTR_WIDTH>* num_contours,
                          float sigma,
                          int shift,
                          unsigned char thresh,
                          unsigned char maxval,
                          int rows,
                          int cols,
                          int img_sizes[8],
                          int mask_corners[8],
                          int dst_height,
                          int dst_width,
                          ap_uint<OUTPUT_PTR_WIDTH>* resizedOutput_duplicate_2_stream);
#endif
