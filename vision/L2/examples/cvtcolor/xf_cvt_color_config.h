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

#ifndef _XF_CVT_COLOR_CONFIG_H_
#define _XF_CVT_COLOR_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
//#include "imgproc/xf_rgb2hsv.hpp"
//#include "imgproc/xf_bgr2hsv.hpp"
// Has to be set when synthesizing
#define _XF_SYNTHESIS_ 1

#define OUTPUT_PTR_WIDTH 512
#define INPUT_PTR_WIDTH 512

// Image Dimensions
#define WIDTH 1920
#define HEIGHT 1080

#define NPC1
#define NPC2

#if (RGB2IYUV || RGB2NV12 || RGB2NV21 || BGR2NV12 || BGR2NV21 || RGB2YUV4)
#define INPUT_CH_TYPE XF_RGB
#endif

#if (NV122RGB || NV212RGB || IYUV2RGB || UYVY2RGB || YUYV2RGB || NV122BGR || NV212BGR)
#define OUTPUT_CH_TYPE XF_RGB
#endif
#if (RGB2GRAY || BGR2GRAY)
#define INPUT_CH_TYPE XF_RGB
#define OUTPUT_CH_TYPE XF_GRAY
#endif
#if (GRAY2RGB || GRAY2BGR)
#define INPUT_CH_TYPE XF_GRAY
#define OUTPUT_CH_TYPE XF_RGB
#endif
#if (RGB2XYZ || BGR2XYZ || XYZ2RGB || XYZ2BGR || RGB2YCrCb || BGR2YCrCb || YCrCb2RGB || YCrCb2BGR || RGB2HLS || \
     BGR2HLS || HLS2RGB || HLS2BGR || RGB2HSV || BGR2HSV || HSV2RGB || HSV2BGR || RGB2BGR || BGR2RGB)
#define INPUT_CH_TYPE XF_RGB
#define OUTPUT_CH_TYPE XF_RGB
#endif

#if (IYUV2NV12 || NV122IYUV || NV212IYUV || NV122YUV4 || NV212YUV4 || UYVY2NV12 || UYVY2NV21 || YUYV2NV12 ||           \
     YUYV2NV21 || RGBA2NV12 || RGBA2NV21 || RGB2NV12 || RGB2NV21 || NV122RGBA || NV212RGB || NV212RGBA || NV122RGB ||  \
     NV122BGR || NV212BGR || NV122YUYV || NV212YUYV || NV122UYVY || NV212UYVY || NV122NV21 || NV212NV12 || BGR2NV12 || \
     BGR2NV21)
#if NO
#define NPC1 XF_NPPC1
#define NPC2 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#define NPC2 XF_NPPC4
#endif
void cvtcolor_rgba2nv12(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);
void cvtcolor_rgba2nv21(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

void cvtcolor_nv122iyuv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2);
void cvtcolor_iyuv2nv12(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput2,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

void cvtcolor_nv122rgba(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_nv212rgba(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_nv122yuv4(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2);
void cvtcolor_nv212yuv4(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2);

void cvtcolor_nv212iyuv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2);

void cvtcolor_yuyv2nv21(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);
void cvtcolor_yuyv2nv12(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

void cvtcolor_uyvy2nv12(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);
void cvtcolor_uyvy2nv21(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

void cvtcolor_nv122bgr(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_nv212bgr(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_nv122yuyv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_nv212yuyv(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_nv122uyvy(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_nv212uyvy(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_nv122nv21(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);
void cvtcolor_nv212nv12(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

void cvtcolor_nv122rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_nv212rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgInput1,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_rgb2nv12(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);
void cvtcolor_rgb2nv21(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

void cvtcolor_bgr2nv21(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);
void cvtcolor_bgr2nv12(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC2, HEIGHT / 2, WIDTH / 2, NPC2>& imgOutput1);

#else
#if NO
#define NPC1 XF_NPPC1
#else
#define NPC1 XF_NPPC8
#endif
#endif

void cvtcolor_rgba2iyuv(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2);
void cvtcolor_rgba2yuv4(xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2);
void cvtcolor_iyuv2rgba(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput2,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_iyuv2yuv4(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgInput2,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2);
void cvtcolor_uyvy2iyuv(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2);
void cvtcolor_uyvy2rgba(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_yuyv2iyuv(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                        xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2);
void cvtcolor_yuyv2rgba(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_rgb2gray(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_bgr2gray(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_gray2rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_gray2bgr(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_rgb2xyz(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_bgr2xyz(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_xyz2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_xyz2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_rgb2ycrcb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_bgr2ycrcb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_ycrcb2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_ycrcb2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_rgb2hls(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_bgr2hls(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_hls2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_hls2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_rgb2hsv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_bgr2hsv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_hsv2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);
void cvtcolor_hsv2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput);

void cvtcolor_rgb2iyuv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput1,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1>& imgOutput2);
void cvtcolor_rgb2yuv4(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput0,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput1,
                       xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>& imgOutput2);
void cvtcolor_uyvy2rgb(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_yuyv2rgb(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_iyuv2rgb(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>& imgInput0,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, XF_NPPC1>& imgInput1,
                       xf::cv::Mat<XF_8UC1, HEIGHT / 4, WIDTH, XF_NPPC1>& imgInput2,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>& imgOutput0);

void cvtcolor_rgb2uyvy(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_rgb2yuyv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_rgb2bgr(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_bgr2uyvy(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_bgr2yuyv(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_bgr2rgb(xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgInput,
                      xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);

void cvtcolor_yuyv2bgr(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_uyvy2bgr(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                       xf::cv::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_uyvy2yuyv(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
void cvtcolor_yuyv2uyvy(xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgInput,
                        xf::cv::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1>& imgOutput0);
#endif //_XF_CVT_COLOR_CONFIG_H_
