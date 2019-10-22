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

#include "xf_colordetect_config.h"

void colordetect_accel(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPIX>& _src,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPIX>& _bgr2hsv,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter1,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter2,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter3,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _inter4,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX>& _dst,
                       unsigned char* low_thresh,
                       unsigned char* high_thresh,
                       unsigned char kernel[FILTER_SIZE * FILTER_SIZE]) {
#pragma HLS INTERFACE m_axi port = low_thresh bundle = gmem0 depth = 9
#pragma HLS INTERFACE m_axi port = high_thresh bundle = gmem1 depth = 9

    xf::cv::bgr2hsv<XF_8UC3, HEIGHT, WIDTH, NPIX>(_src, _bgr2hsv);
    xf::cv::colorthresholding<XF_8UC3, XF_8UC1, MAXCOLORS, HEIGHT, WIDTH, NPIX>(_bgr2hsv, _inter1, low_thresh,
                                                                                high_thresh);
    xf::cv::erode<XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, KERNEL_SHAPE, FILTER_SIZE, FILTER_SIZE, ITERATIONS, NPIX>(
        _inter1, _inter2, kernel);
    xf::cv::dilate<XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, KERNEL_SHAPE, FILTER_SIZE, FILTER_SIZE, ITERATIONS,
                   NPIX>(_inter2, _inter3, kernel);
    xf::cv::dilate<XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, KERNEL_SHAPE, FILTER_SIZE, FILTER_SIZE, ITERATIONS,
                   NPIX>(_inter3, _inter4, kernel);
    xf::cv::erode<XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, KERNEL_SHAPE, FILTER_SIZE, FILTER_SIZE, ITERATIONS, NPIX>(
        _inter4, _dst, kernel);
}
