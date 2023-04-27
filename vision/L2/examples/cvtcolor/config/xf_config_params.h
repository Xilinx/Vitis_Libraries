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

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
//#include "imgproc/xf_rgb2hsv.hpp"
#include "imgproc/xf_bgr2hsv.hpp"
// Has to be set when synthesizing

/*  set the optimisation type  */
#define SPC 1
// Single Pixel per Clock operation
#define MPC 0
// Multiple Pixels per Clock operation

#define XF_CV_DEPTH_IN_0 2
#define XF_CV_DEPTH_OUT_0 2
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_OUT_2 2
// Set Conversion type
#define IYUV2NV12 0
#define IYUV2RGBA 0
#define IYUV2RGB 0
#define IYUV2YUV4 0

#define NV122IYUV 0
#define NV122RGBA 0
#define NV122RGB 0
#define NV122BGR 0
#define NV122YUV4 0

#define NV212IYUV 0
#define NV212RGBA 0
#define NV212RGB 0
#define NV212BGR 0
#define NV212YUV4 0

#define RGBA2IYUV 0
#define RGBA2NV12 0
#define RGBA2NV21 0
#define RGBA2YUV4 0

#define RGB2IYUV 0
#define RGB2NV12 0
#define RGB2NV21 0
#define RGB2YUV4 0

#define UYVY2IYUV 0
#define UYVY2NV12 0
#define UYVY2RGBA 0
#define UYVY2RGB 0

#define YUYV2IYUV 0
#define YUYV2NV12 0
#define YUYV2RGBA 0
#define YUYV2RGB 0

#define RGB2GRAY 0
#define BGR2GRAY 1
#define GRAY2RGB 0
#define GRAY2BGR 0

#define RGB2XYZ 0
#define BGR2XYZ 0
#define XYZ2RGB 0
#define XYZ2BGR 0

#define RGB2YCrCb 0
#define BGR2YCrCb 0
#define YCrCb2RGB 0
#define YCrCb2BGR 0

#define RGB2HSV 0
#define BGR2HSV 0
#define HSV2RGB 0
#define HSV2BGR 0

#define RGB2HLS 0
#define BGR2HLS 0
#define HLS2RGB 0
#define HLS2BGR 0

#define _XF_SYNTHESIS_ 1

#define OUTPUT_PTR_WIDTH 512
#define INPUT_PTR_WIDTH 512

// Image Dimensions
#define WIDTH 1920
#define HEIGHT 1080

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
#if SPC
#define NPC1 XF_NPPC1
#define NPC2 XF_NPPC1
#endif
#if MPC
#define NPC1 XF_NPPC8
#define NPC2 XF_NPPC4
#endif
#else
#if SPC
#define NPC1 XF_NPPC1
#else
#define NPC1 XF_NPPC8
#endif
#endif