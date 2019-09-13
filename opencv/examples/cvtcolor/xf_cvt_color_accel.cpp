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

#include "xf_cvt_color_config.h"

#if RGBA2IYUV
	void cvtcolor_rgba2iyuv(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4,WIDTH,NPC1> &imgOutput2)
	{

		xf::rgba2iyuv<XF_8UC4,XF_8UC1, HEIGHT, WIDTH,NPC1 >(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if RGBA2NV12
	void cvtcolor_rgba2nv12(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::rgba2nv12<XF_8UC4,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if RGBA2NV21
	void cvtcolor_rgba2nv21(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::rgba2nv21<XF_8UC4,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if RGBA2YUV4
	void cvtcolor_rgba2yuv4(xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1>&imgOutput2)
	{
		xf::rgba2yuv4<XF_8UC4,XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if RGB2IYUV
	void cvtcolor_rgb2iyuv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4,WIDTH,NPC1> &imgOutput2)
	{

		xf::rgb2iyuv<XF_8UC3,XF_8UC1, HEIGHT, WIDTH,NPC1 >(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if RGB2NV12
	void cvtcolor_rgb2nv12(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::rgb2nv12<XF_8UC3,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if RGB2NV21
	void cvtcolor_rgb2nv21(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::rgb2nv21<XF_8UC3,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if RGB2YUV4
	void cvtcolor_rgb2yuv4(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH,	NPC1>&imgOutput2)
	{
		xf::rgb2yuv4<XF_8UC3,XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if RGB2UYVY
	void cvtcolor_rgb2uyvy(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::rgb2uyvy<XF_8UC3,XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if RGB2YUYV
	void cvtcolor_rgb2yuyv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::rgb2yuyv<XF_8UC3,XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if RGB2BGR
	void cvtcolor_rgb2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::rgb2bgr<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif

#if BGR2UYVY
	void cvtcolor_bgr2uyvy(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		  xf::bgr2uyvy<XF_8UC3,XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if BGR2YUYV
	void cvtcolor_bgr2yuyv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::bgr2yuyv<XF_8UC3,XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if BGR2RGB
	void cvtcolor_bgr2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput, xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::bgr2rgb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if BGR2NV12
	void cvtcolor_bgr2nv12(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::bgr2nv12<XF_8UC3,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if BGR2NV21
	void cvtcolor_bgr2nv21(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::bgr2nv21<XF_8UC3,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif


#if IYUV2NV12
	void cvtcolor_iyuv2nv12(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{	
		xf::iyuv2nv12<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1,NPC2>(imgInput0,imgInput1,imgInput2,imgOutput0,imgOutput1);
	}
#endif
#if IYUV2RGBA
	void cvtcolor_iyuv2rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::iyuv2rgba<XF_8UC1,XF_8UC4,HEIGHT,WIDTH,NPC1>(imgInput0,imgInput1,imgInput2,imgOutput0);
	}
#endif
#if IYUV2RGB
	void cvtcolor_iyuv2rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput2,xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0)
	{
		xf::iyuv2rgb<XF_8UC1,XF_8UC3,HEIGHT,WIDTH,XF_NPPC1>(imgInput0,imgInput1,imgInput2,imgOutput0);
	}
#endif

#if IYUV2YUV4
	void cvtcolor_iyuv2yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC1, HEIGHT, WIDTH, 					NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2)
	{
		xf::iyuv2yuv4<XF_8UC1,HEIGHT,WIDTH,NPC1>(imgInput0,imgInput1,imgInput2,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if NV122IYUV
	void cvtcolor_nv122iyuv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 				&imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2)
	{
		xf::nv122iyuv<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if NV122RGBA
	void cvtcolor_nv122rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv122rgba<XF_8UC1,XF_8UC2,XF_8UC4,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV122YUV4
	void cvtcolor_nv122yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2)
	{
		xf::nv122yuv4<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if NV122RGB
	void cvtcolor_nv122rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv122rgb<XF_8UC1,XF_8UC2,XF_8UC3,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV122BGR
	void cvtcolor_nv122bgr(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv122bgr<XF_8UC1,XF_8UC2,XF_8UC3,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV122UYVY
	void cvtcolor_nv122uyvy(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv122uyvy<XF_8UC1,XF_8UC2,XF_16UC1,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV122YUYV
	void cvtcolor_nv122yuyv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv122yuyv<XF_8UC1,XF_8UC2,XF_16UC1,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV122NV21
	void cvtcolor_nv122nv21(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::nv122nv21<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1);
	}
#endif


#if NV212IYUV
	void cvtcolor_nv212iyuv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2)
	{
		xf::nv212iyuv<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if NV212RGBA
	void cvtcolor_nv212rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv212rgba<XF_8UC1,XF_8UC2,XF_8UC4,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV212RGB
	void cvtcolor_nv212rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv212rgb<XF_8UC1,XF_8UC2,XF_8UC3,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV212BGR
	void cvtcolor_nv212bgr(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv212bgr<XF_8UC1,XF_8UC2,XF_8UC3,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV212YUV4
	void cvtcolor_nv212yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2)
	{
		xf::nv212yuv4<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if NV212UYVY
	void cvtcolor_nv212uyvy(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv212uyvy<XF_8UC1,XF_8UC2,XF_16UC1,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV212YUYV
	void cvtcolor_nv212yuyv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::nv212yuyv<XF_8UC1,XF_8UC2,XF_16UC1,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV212NV12
	void cvtcolor_nv212nv12(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::nv212nv12<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1);
	}
#endif



#if UYVY2IYUV
	void cvtcolor_uyvy2iyuv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, 					NPC1> &imgOutput2)
	{
		xf::uyvy2iyuv<XF_16UC1, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0, imgOutput1, imgOutput2);
	}
#endif
#if UYVY2NV12
	void cvtcolor_uyvy2nv12(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::uyvy2nv12<XF_16UC1,XF_8UC1,XF_8UC2,HEIGHT, WIDTH, NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if UYVY2NV21
	void cvtcolor_uyvy2nv21(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::uyvy2nv21<XF_16UC1,XF_8UC1,XF_8UC2,HEIGHT, WIDTH, NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if UYVY2RGBA
	void cvtcolor_uyvy2rgba(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::uyvy2rgba<XF_16UC1,XF_8UC4, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if UYVY2RGB
	void cvtcolor_uyvy2rgb(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::uyvy2rgb<XF_16UC1,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if UYVY2BGR
	void cvtcolor_uyvy2bgr(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::uyvy2bgr<XF_16UC1,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if UYVY2YUYV
	void cvtcolor_uyvy2yuyv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::uyvy2yuyv<XF_16UC1,XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif


#if YUYV2IYUV
	void cvtcolor_yuyv2iyuv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, 					NPC1> &imgOutput2)
	{
		xf::yuyv2iyuv<XF_16UC1,XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if YUYV2NV12
	void cvtcolor_yuyv2nv12(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::yuyv2nv12<XF_16UC1,XF_8UC1,XF_8UC2, HEIGHT, WIDTH, NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if YUYV2NV21
	void cvtcolor_yuyv2nv21(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xf::yuyv2nv21<XF_16UC1,XF_8UC1,XF_8UC2, HEIGHT, WIDTH, NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if YUYV2RGBA
	void cvtcolor_yuyv2rgba(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::yuyv2rgba<XF_16UC1,XF_8UC4, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if YUYV2RGB
	void cvtcolor_yuyv2rgb(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::yuyv2rgb<XF_16UC1,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if YUYV2BGR
	void cvtcolor_yuyv2bgr(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::yuyv2bgr<XF_16UC1,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif
#if YUYV2UYVY
	void cvtcolor_yuyv2uyvy(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgOutput0)
	{
		xf::yuyv2uyvy<XF_16UC1,XF_16UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0);
	}
#endif


#if RGB2GRAY
	void cvtcolor_rgb2gray(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::rgb2gray<XF_8UC3,XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if BGR2GRAY
	void cvtcolor_bgr2gray(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::bgr2gray<XF_8UC3,XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if GRAY2RGB
	void cvtcolor_gray2rgb(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::gray2rgb<XF_8UC1,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if GRAY2BGR
	void cvtcolor_gray2bgr(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::gray2bgr<XF_8UC1,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if RGB2XYZ
	void cvtcolor_rgb2xyz(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::rgb2xyz<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if BGR2XYZ
	void cvtcolor_bgr2xyz(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::bgr2xyz<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if XYZ2RGB
	void cvtcolor_xyz2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::xyz2rgb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if XYZ2BGR
	void cvtcolor_xyz2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::xyz2bgr<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if RGB2YCrCb
	void cvtcolor_rgb2ycrcb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::rgb2ycrcb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif

#if BGR2YCrCb
	void cvtcolor_bgr2ycrcb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::bgr2ycrcb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif

#if YCrCb2RGB
	void cvtcolor_ycrcb2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::ycrcb2rgb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if YCrCb2BGR
	void cvtcolor_ycrcb2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::ycrcb2bgr<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if RGB2HLS
	void cvtcolor_rgb2hls(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::rgb2hls<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if BGR2HLS
	void cvtcolor_bgr2hls(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::bgr2hls<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if HLS2RGB
	void cvtcolor_hls2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::hls2rgb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if HLS2BGR
	void cvtcolor_hls2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::hls2bgr<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if RGB2HSV
	void cvtcolor_rgb2hsv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::rgb2hsv<XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if BGR2HSV
	void cvtcolor_bgr2hsv(xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> &imgOutput )
	{
		xf::bgr2hsv<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1>(imgInput, imgOutput);
	}
#endif
#if HSV2RGB
	void cvtcolor_hsv2rgb(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::hsv2rgb<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
#if HSV2BGR
	void cvtcolor_hsv2bgr(xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> &imgOutput )
	{
		xf::hsv2bgr<XF_8UC3,XF_8UC3, HEIGHT, WIDTH, NPC1>(imgInput, imgOutput);
	}
#endif
