/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef _XF_COLORDETECT_CONFIG_H_
#define _XF_COLORDETECT_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_colorthresholding.hpp"
#include "imgproc/xf_inrange.hpp"
#include "imgproc/xf_bgr2hsv.hpp"
#include "imgproc/xf_erosion.hpp"
#include "imgproc/xf_dilation.hpp"

#define HEIGHT 128
#define WIDTH 128

#define MAXCOLORS 3
#define NPIX XF_NPPC1

#define FILTER_SIZE 3

#define KERNEL_SHAPE XF_SHAPE_RECT

#define ITERATIONS 1

void colordetect_accel(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPIX>& _src,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPIX>& _rgb2hsv,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter1,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter2,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter3,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter4,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _dst,
                       unsigned char* low_thresh,
                       unsigned char* high_thresh,
                       unsigned char kernel[FILTER_SIZE * FILTER_SIZE]);
#endif
