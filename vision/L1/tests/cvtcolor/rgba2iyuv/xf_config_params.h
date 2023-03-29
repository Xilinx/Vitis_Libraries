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

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
//#include "imgproc/xf_rgb2hsv.hpp"
//#include "imgproc/xf_bgr2hsv.hpp"
// Has to be set when synthesizing

/*  set the optimisation type  */
#define SPC 1
// Single Pixel per Clock operation
#define MPC 0
// Multiple Pixels per Clock operation

// Check if define already passed in command line
#if !(defined(BGR2NV12) || defined(BGR2NV21) || defined(NV122BGR) || defined(NV122IYUV) || defined(NV122NV21) ||   \
      defined(NV122UYVY) || defined(NV122YUV4) || defined(NV122YUYV) || defined(NV212BGR) || defined(NV212IYUV) || \
      defined(NV212NV12) || defined(NV212UYVY) || defined(NV212YUYV) || defined(RGB2IYUV) || defined(RGB2NV12) ||  \
      defined(RGB2NV21) || defined(RGB2UYVY) || defined(RGB2YUV4) || defined(RGB2YUYV) || defined(RGBA2IYUV) ||    \
      defined(UYVY2IYUV) || defined(UYVY2NV12) || defined(UYVY2NV21) || defined(UYVY2RGB) || defined(UYVY2YUYV) || \
      defined(YUYV2IYUV) || defined(YUYV2NV12) || defined(YUYV2RGBA) || defined(YUYV2UYVY))

#ifndef RGBA2IYUV
#define RGBA2IYUV 1
#endif

#ifndef RGBA2NV12
#define RGBA2NV12 0
#endif

#ifndef RGBA2NV21
#define RGBA2NV21 0
#endif

#ifndef RGBA2YUV4
#define RGBA2YUV4 0
#endif

#ifndef RGB2IYUV
#define RGB2IYUV 0
#endif

#ifndef RGB2NV12
#define RGB2NV12 0
#endif

#ifndef RGB2NV21
#define RGB2NV21 0
#endif

#ifndef RGB2YUV4
#define RGB2YUV4 0
#endif

#ifndef RGB2UYVY
#define RGB2UYVY 0
#endif

#ifndef RGB2YUYV
#define RGB2YUYV 0
#endif

#ifndef RGB2BGR
#define RGB2BGR 0
#endif

#ifndef BGR2UYVY
#define BGR2UYVY 0
#endif

#ifndef BGR2YUYV
#define BGR2YUYV 0
#endif

#ifndef BGR2RGB
#define BGR2RGB 0
#endif

#ifndef BGR2NV12
#define BGR2NV12 0
#endif

#ifndef BGR2NV21
#define BGR2NV21 0
#endif

#ifndef IYUV2NV12
#define IYUV2NV12 0
#endif

#ifndef IYUV2RGBA
#define IYUV2RGBA 0
#endif

#ifndef IYUV2RGB
#define IYUV2RGB 0
#endif

#ifndef IYUV2YUV4
#define IYUV2YUV4 0
#endif

#ifndef NV122IYUV
#define NV122IYUV 0
#endif

#ifndef NV122RGBA
#define NV122RGBA 0
#endif

#ifndef NV122YUV4
#define NV122YUV4 0
#endif

#ifndef NV122RGB
#define NV122RGB 0
#endif

#ifndef NV122BGR
#define NV122BGR 0
#endif

#ifndef NV122UYVY
#define NV122UYVY 0
#endif

#ifndef NV122YUYV
#define NV122YUYV 0
#endif

#ifndef NV122NV21
#define NV122NV21 0
#endif

#ifndef NV212IYUV
#define NV212IYUV 0
#endif

#ifndef NV212RGBA
#define NV212RGBA 0
#endif

#ifndef NV212RGB
#define NV212RGB 0
#endif

#ifndef NV212BGR
#define NV212BGR 0
#endif

#ifndef NV212YUV4
#define NV212YUV4 0
#endif

#ifndef NV212UYVY
#define NV212UYVY 0
#endif

#ifndef NV212YUYV
#define NV212YUYV 0
#endif

#ifndef NV212NV12
#define NV212NV12 0
#endif

#ifndef UYVY2IYUV
#define UYVY2IYUV 0
#endif

#ifndef UYVY2NV12
#define UYVY2NV12 0
#endif

#ifndef UYVY2NV21
#define UYVY2NV21 0
#endif

#ifndef UYVY2RGBA
#define UYVY2RGBA 0
#endif

#ifndef UYVY2RGB
#define UYVY2RGB 0
#endif

#ifndef UYVY2BGR
#define UYVY2BGR 0
#endif

#ifndef UYVY2YUYV
#define UYVY2YUYV 0
#endif

#ifndef YUYV2IYUV
#define YUYV2IYUV 0
#endif

#ifndef YUYV2NV12
#define YUYV2NV12 0
#endif

#ifndef YUYV2NV21
#define YUYV2NV21 0
#endif

#ifndef YUYV2RGBA
#define YUYV2RGBA 0
#endif

#ifndef YUYV2RGB
#define YUYV2RGB 0
#endif

#ifndef YUYV2BGR
#define YUYV2BGR 0
#endif

#ifndef YUYV2UYVY
#define YUYV2UYVY 0
#endif

#ifndef RGB2GRAY
#define RGB2GRAY 0
#endif

#ifndef BGR2GRAY
#define BGR2GRAY 0
#endif

#ifndef GRAY2RGB
#define GRAY2RGB 0
#endif

#ifndef GRAY2BGR
#define GRAY2BGR 0
#endif

#ifndef RGB2XYZ
#define RGB2XYZ 0
#endif

#ifndef BGR2XYZ
#define BGR2XYZ 0
#endif

#ifndef XYZ2RGB
#define XYZ2RGB 0
#endif

#ifndef XYZ2BGR
#define XYZ2BGR 0
#endif

#ifndef RGB2YCrCb
#define RGB2YCrCb 0
#endif

#ifndef BGR2YCrCb
#define BGR2YCrCb 0
#endif

#ifndef YCrCb2RGB
#define YCrCb2RGB 0
#endif

#ifndef YCrCb2BGR
#define YCrCb2BGR 0
#endif

#ifndef RGB2HLS
#define RGB2HLS 0
#endif

#ifndef BGR2HLS
#define BGR2HLS 0
#endif

#ifndef HLS2RGB
#define HLS2RGB 0
#endif

#ifndef HLS2BGR
#define HLS2BGR 0
#endif

#ifndef RGB2HSV
#define RGB2HSV 0
#endif

#ifndef BGR2HSV
#define BGR2HSV 0
#endif

#ifndef HSV2RGB
#define HSV2RGB 0
#endif

#ifndef HSV2BGR
#define HSV2BGR 0
#endif

#endif

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define XF_CV_DEPTH_IN_0 2
#define XF_CV_DEPTH_OUT_0 2

#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_OUT_2 2

#define _XF_SYNTHESIS_ 1

// Image Dimensions
static constexpr int WIDTH = 1920;
static constexpr int HEIGHT = 1080;

#if (IYUV2NV12 || NV122IYUV || NV212IYUV || NV122YUV4 || NV212YUV4 || UYVY2NV12 || UYVY2NV21 || YUYV2NV12 ||           \
     YUYV2NV21 || RGBA2NV12 || RGBA2NV21 || RGB2NV12 || RGB2NV21 || NV122RGBA || NV212RGB || NV212RGBA || NV122RGB ||  \
     NV122BGR || NV212BGR || NV122YUYV || NV212YUYV || NV122UYVY || NV212UYVY || NV122NV21 || NV212NV12 || BGR2NV12 || \
     BGR2NV21)
#if SPC
static constexpr int NPC1 = XF_NPPC1;
static constexpr int NPC2 = XF_NPPC1;
#endif
#if MPC
static constexpr int NPC1 = XF_NPPC8;
static constexpr int NPC2 = XF_NPPC4;
#endif

#else
#if SPC
static constexpr int NPC1 = XF_NPPC1;
static constexpr int NPC2 = XF_NPPC1;
#else
static constexpr int NPC1 = XF_NPPC8;
static constexpr int NPC2 = XF_NPPC8;
#endif
#endif

void cvtcolor_rgba2iyuv(ap_uint<32 * NPC1>* imgInput,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_rgba2nv12(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_rgba2nv21(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_rgba2yuv4(ap_uint<32 * NPC1>* imgInput,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_rgb2iyuv(ap_uint<32 * NPC1>* imgInput,
                       ap_uint<8 * NPC1>* imgOutput0,
                       ap_uint<8 * NPC1>* imgOutput1,
                       ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_rgb2nv12(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_rgb2nv21(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_rgb2yuv4(ap_uint<32 * NPC1>* imgInput,
                       ap_uint<8 * NPC1>* imgOutput0,
                       ap_uint<8 * NPC1>* imgOutput1,
                       ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_rgb2uyvy(ap_uint<32 * NPC1>* imgInput, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_rgb2yuyv(ap_uint<32 * NPC1>* imgInput, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_rgb2bgr(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_bgr2uyvy(ap_uint<32 * NPC1>* imgInput, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_bgr2yuyv(ap_uint<32 * NPC1>* imgInput, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_bgr2rgb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_bgr2nv12(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_bgr2nv21(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_iyuv2nv12(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<8 * NPC1>* imgInput1,
                        ap_uint<8 * NPC1>* imgInput2,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_iyuv2rgba(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<8 * NPC1>* imgInput1,
                        ap_uint<8 * NPC1>* imgInput2,
                        ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_iyuv2rgb(ap_uint<8 * NPC1>* imgInput0,
                       ap_uint<8 * NPC1>* imgInput1,
                       ap_uint<8 * NPC1>* imgInput2,
                       ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_iyuv2yuv4(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<8 * NPC1>* imgInput1,
                        ap_uint<8 * NPC1>* imgInput2,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_nv122iyuv(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_nv122rgba(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_nv122yuv4(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_nv122rgb(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_nv122bgr(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_nv122uyvy(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_nv122yuyv(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_nv122nv21(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_nv212iyuv(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_nv212rgba(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_nv212rgb(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_nv212bgr(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_nv212yuv4(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_nv212uyvy(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_nv212yuyv(ap_uint<8 * NPC1>* imgInput0, ap_uint<16 * NPC2>* imgInput1, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_nv212nv12(ap_uint<8 * NPC1>* imgInput0,
                        ap_uint<16 * NPC2>* imgInput1,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_uyvy2iyuv(ap_uint<16 * NPC1>* imgInput,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_uyvy2nv12(ap_uint<16 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_uyvy2nv21(ap_uint<16 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_uyvy2rgba(ap_uint<16 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_uyvy2rgb(ap_uint<16 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_uyvy2bgr(ap_uint<16 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_uyvy2yuyv(ap_uint<16 * NPC1>* imgInput, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_yuyv2iyuv(ap_uint<16 * NPC1>* imgInput,
                        ap_uint<8 * NPC1>* imgOutput0,
                        ap_uint<8 * NPC1>* imgOutput1,
                        ap_uint<8 * NPC1>* imgOutput2);
void cvtcolor_yuyv2nv12(ap_uint<16 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_yuyv2nv21(ap_uint<16 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput0, ap_uint<16 * NPC2>* imgOutput1);
void cvtcolor_yuyv2rgba(ap_uint<16 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_yuyv2rgb(ap_uint<16 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_yuyv2bgr(ap_uint<16 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_yuyv2uyvy(ap_uint<16 * NPC1>* imgInput, ap_uint<16 * NPC1>* imgOutput);
void cvtcolor_rgb2gray(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput);
void cvtcolor_bgr2gray(ap_uint<32 * NPC1>* imgInput, ap_uint<8 * NPC1>* imgOutput);
void cvtcolor_gray2rgb(ap_uint<8 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_gray2bgr(ap_uint<8 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_rgb2xyz(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_bgr2xyz(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_xyz2rgb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_xyz2bgr(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_rgb2ycrcb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_bgr2ycrcb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_ycrcb2rgb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_ycrcb2bgr(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_rgb2hls(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_bgr2hls(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_hls2rgb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_hls2bgr(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_rgb2hsv(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_bgr2hsv(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_hsv2rgb(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);
void cvtcolor_hsv2bgr(ap_uint<32 * NPC1>* imgInput, ap_uint<32 * NPC1>* imgOutput);