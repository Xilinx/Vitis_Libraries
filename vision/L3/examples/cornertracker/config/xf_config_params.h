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
#ifndef __XF_CORNER_TRACKER_CONFIG__
#define __XF_CORNER_TRACKER_CONFIG__

#include "ap_int.h"
#include "hls_stream.h"
#include "assert.h"
#include "common/xf_common.hpp"
#include "xf_config_params.h"
#include "video/xf_pyr_dense_optical_flow_wrapper.hpp"
#include "imgproc/xf_pyr_down.hpp"
#include "features/xf_harris.hpp"
#include "imgproc/xf_corner_update.hpp"
#include "imgproc/xf_corner_img_to_list.hpp"

#define HEIGHT 1080
#define WIDTH 1920

#define XF_CV_DEPTH_IN_HARRIS 2
#define XF_CV_DEPTH_OUT_HARRIS 2

#define XF_CV_DEPTH_CORNER_UPDATE -1

#define XF_CV_DEPTH_PDOF_1 2
#define XF_CV_DEPTH_PDOF_2 2
#define XF_CV_DEPTH_PDOF_3 2
#define XF_CV_DEPTH_PDOF_4 2

#define XF_CV_DEPTH_PD_1 2
#define XF_CV_DEPTH_PD_2 2
#define XF_CV_DEPTH_PD_3 2
#define XF_CV_DEPTH_PD_4 2

#define TYPE_FLOW_WIDTH 16
#define TYPE_FLOW_INT 10
#define TYPE_FLOW_TYPE ap_fixed<TYPE_FLOW_WIDTH, TYPE_FLOW_INT>

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

#define WINSIZE_OFLOW 11

#define NUM_LEVELS 5
#define NUM_ITERATIONS 5

#define NUM_LINES_FINDIT 50

// harris parameters
#define FILTER_WIDTH 3
#define BLOCK_WIDTH 3
#define NMS_RADIUS 1
#define MAXCORNERS 10000

#define XF_USE_URAM false

#define CH_TYPE XF_GRAY

/*void cornerTracker(xf::cv::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> & flow, xf::cv::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> &
 * flow_iter, xf::cv::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> mat_imagepyr1[NUM_LEVELS] ,
 * xf::cv::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1> mat_imagepyr2[NUM_LEVELS] , xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>
 * &inHarris, xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &outHarris, unsigned int *list, unsigned long *listfixed,
 * int pyr_h[NUM_LEVELS], int pyr_w[NUM_LEVELS], unsigned int *params);*/
#endif
