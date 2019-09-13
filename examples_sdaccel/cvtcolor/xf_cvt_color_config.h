/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/
#ifndef _XF_CVT_COLOR_CONFIG_H_
#define _XF_CVT_COLOR_CONFIG_H_

#include"hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
//#include "imgproc/xf_rgb2hsv.hpp"
//#include "imgproc/xf_bgr2hsv.hpp"
// Has to be set when synthesizing
#define _XF_SYNTHESIS_ 1

#define OUTPUT_PTR_WIDTH 512
#define INPUT_PTR_WIDTH  512

// Image Dimensions
#define WIDTH 	1920
#define HEIGHT 	1080

#define NPC1
#define NPC2

#if (RGB2IYUV || RGB2NV12|| RGB2NV21 ||BGR2NV12|| BGR2NV21|| RGB2YUV4)
#define INPUT_CH_TYPE XF_RGB
#endif

#if (NV122RGB|| NV212RGB ||IYUV2RGB || UYVY2RGB|| YUYV2RGB|| NV122BGR|| NV212BGR)
#define OUTPUT_CH_TYPE XF_RGB
#endif
#if (RGB2GRAY||BGR2GRAY)
#define INPUT_CH_TYPE XF_RGB
#define OUTPUT_CH_TYPE XF_GRAY
#endif
#if (GRAY2RGB||GRAY2BGR)
#define INPUT_CH_TYPE XF_GRAY
#define OUTPUT_CH_TYPE XF_RGB
#endif
#if (RGB2XYZ||BGR2XYZ||XYZ2RGB||XYZ2BGR||RGB2YCrCb||BGR2YCrCb||YCrCb2RGB||YCrCb2BGR||RGB2HLS||BGR2HLS||HLS2RGB||HLS2BGR||RGB2HSV||BGR2HSV||HSV2RGB||HSV2BGR ||RGB2BGR||BGR2RGB)
#define INPUT_CH_TYPE XF_RGB
#define OUTPUT_CH_TYPE XF_RGB
#endif

#if ( IYUV2NV12 || NV122IYUV || NV212IYUV || NV122YUV4 || NV212YUV4 || UYVY2NV12 || UYVY2NV21 || YUYV2NV12 || YUYV2NV21  || RGBA2NV12 || RGBA2NV21 || RGB2NV12 ||RGB2NV21 || NV122RGBA || NV212RGB || NV212RGBA || NV122RGB ||NV122BGR  || NV212BGR || NV122YUYV || NV212YUYV || NV122UYVY || NV212UYVY || NV122NV21 || NV212NV12 || BGR2NV12 ||BGR2NV21 )
	#if NO
	#define NPC1 XF_NPPC1
	#define NPC2 XF_NPPC1
	#endif
	#if RO
	#define NPC1 XF_NPPC8
	#define NPC2 XF_NPPC4
	#endif
	void cvtcolor_rgba2nv12(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
	void cvtcolor_rgba2nv21(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

	void cvtcolor_nv122iyuv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 				&imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2);
	void cvtcolor_iyuv2nv12(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

	void cvtcolor_nv122rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0);
	void cvtcolor_nv212rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0);

	void cvtcolor_nv122yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2);
	void cvtcolor_nv212yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2);

	void cvtcolor_nv212iyuv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2);

	void cvtcolor_yuyv2nv21(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
	void cvtcolor_yuyv2nv12(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

	void cvtcolor_uyvy2nv12(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
	void cvtcolor_uyvy2nv21(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

	void cvtcolor_nv122bgr(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
	void cvtcolor_nv212bgr(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);

	void cvtcolor_nv122yuyv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
	void cvtcolor_nv212yuyv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);

	void cvtcolor_nv122uyvy(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
	void cvtcolor_nv212uyvy(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);

	void cvtcolor_nv122nv21(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
	void cvtcolor_nv212nv12(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

	void cvtcolor_nv122rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
	void cvtcolor_nv212rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);

	void cvtcolor_rgb2nv12(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
	void cvtcolor_rgb2nv21(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

	void cvtcolor_bgr2nv21(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
	void cvtcolor_bgr2nv12(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);

#else
	#if NO
	#define NPC1 XF_NPPC1
	#else
	#define NPC1 XF_NPPC8
	#endif
#endif

void cvtcolor_rgba2iyuv(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1,HEIGHT/4,WIDTH,NPC1> &imgOutput2);
void cvtcolor_rgba2yuv4(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>&imgOutput2);
void cvtcolor_iyuv2rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_iyuv2yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC1, HEIGHT, WIDTH, 					NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2);
void cvtcolor_uyvy2iyuv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, 					NPC1> &imgOutput2);
void cvtcolor_uyvy2rgba(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_yuyv2iyuv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, 					NPC1> &imgOutput2);
void cvtcolor_yuyv2rgba(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0);

void cvtcolor_rgb2gray(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_bgr2gray(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_gray2rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_gray2bgr(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_rgb2xyz(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_bgr2xyz(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_xyz2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_xyz2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_rgb2ycrcb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_bgr2ycrcb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_ycrcb2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_ycrcb2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_rgb2hls(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_bgr2hls(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_hls2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_hls2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_rgb2hsv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_bgr2hsv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_hsv2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);
void cvtcolor_hsv2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput);

void cvtcolor_rgb2iyuv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4,WIDTH,NPC1> &imgOutput2);
void cvtcolor_rgb2yuv4(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>&imgOutput2);
void cvtcolor_uyvy2rgb(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_yuyv2rgb(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_iyuv2rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput2,xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0);

void cvtcolor_rgb2uyvy(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_rgb2yuyv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_rgb2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_bgr2uyvy(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_bgr2yuyv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_bgr2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);

void cvtcolor_yuyv2bgr(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_uyvy2bgr(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_uyvy2yuyv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
void cvtcolor_yuyv2uyvy(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0);
#endif //_XF_CVT_COLOR_CONFIG_H_
