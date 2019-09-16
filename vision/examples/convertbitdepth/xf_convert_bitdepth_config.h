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

#ifndef _XF_CONVERT_BITDEPTH_CONFIG_H_
#define _XF_CONVERT_BITDEPTH_CONFIG_H_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.h"
#include "xf_config_params.h"
#include "core/xf_convert_bitdepth.hpp"

/* config width and height */
#define WIDTH 	3840
#define HEIGHT	2160

#if RO
#define _NPC XF_NPPC8
#elif NO
#define _NPC XF_NPPC1
#endif


#if XF_CONVERT8UTO16U
#define IN_TYPE   		unsigned char
#define _SRC_T XF_8UC1
#define _DST_T XF_16UC1
#define OCV_INTYPE 		CV_8UC1
#define OCV_OUTTYPE 	CV_16UC1
#define CONVERT_TYPE 	XF_CONVERT_8U_TO_16U
#endif

#if XF_CONVERT8UTO16S
#define IN_TYPE   		unsigned char
#define _SRC_T XF_8UC1
#define _DST_T XF_16SC1
#define OCV_INTYPE 		CV_8UC1
#define OCV_OUTTYPE 	CV_16SC1
#define CONVERT_TYPE 	XF_CONVERT_8U_TO_16S
#endif

#if XF_CONVERT8UTO32S
#define _SRC_T XF_8UC1
#define _DST_T XF_32SC1
#define OCV_INTYPE 		CV_8UC1
#define OCV_OUTTYPE 	CV_32SC1
#define CONVERT_TYPE 	XF_CONVERT_8U_TO_32S
#define IN_TYPE   		unsigned char
#endif

#if XF_CONVERT16UTO32S
#define IN_TYPE   		unsigned short int
#define _SRC_T XF_16UC1
#define _DST_T XF_32SC1
#define OCV_INTYPE 		CV_16UC1
#define OCV_OUTTYPE 	CV_32SC1
#define CONVERT_TYPE 	XF_CONVERT_16U_TO_32S
#endif

#if XF_CONVERT16STO32S
#define IN_TYPE		short int
#define _SRC_T XF_16SC1
#define _DST_T XF_32SC1
#define OCV_INTYPE 		CV_16SC1
#define OCV_OUTTYPE 	CV_32SC1
#define CONVERT_TYPE 	XF_CONVERT_16S_TO_32S
#endif


#if XF_CONVERT16UTO8U
#define IN_TYPE    		unsigned short int
#define _SRC_T XF_16UC1
#define _DST_T XF_8UC1
#define OCV_INTYPE 		CV_16UC1
#define OCV_OUTTYPE 	CV_8UC1
#define CONVERT_TYPE 	XF_CONVERT_16U_TO_8U
#endif

#if XF_CONVERT16STO8U
#define IN_TYPE    		short int
#define _SRC_T XF_16SC1
#define _DST_T XF_8UC1
#define CONVERT_TYPE 	XF_CONVERT_16S_TO_8U
#define OCV_INTYPE 		CV_16SC1
#define OCV_OUTTYPE 	CV_8UC1
#endif

#if XF_CONVERT32STO8U
#define _SRC_T XF_32SC1
#define _DST_T XF_8UC1
#define OCV_INTYPE 		CV_32SC1
#define OCV_OUTTYPE 	CV_8UC1
#define CONVERT_TYPE 	XF_CONVERT_32S_TO_8U
#define IN_TYPE			int
#endif

#if XF_CONVERT32STO16U
#define _SRC_T XF_32SC1
#define _DST_T XF_16UC1
#define OCV_INTYPE 		CV_32SC1
#define OCV_OUTTYPE 	CV_16UC1
#define CONVERT_TYPE 	XF_CONVERT_32S_TO_16U
#define IN_TYPE 		int
#endif

#if XF_CONVERT32STO16S
#define _SRC_T XF_32SC1
#define _DST_T XF_16SC1
#define OCV_INTYPE 		CV_32SC1
#define OCV_OUTTYPE 	CV_16SC1
#define CONVERT_TYPE 	XF_CONVERT_32S_TO_16S
#define IN_TYPE 		int
#endif

void convert_bitdepth_accel(xf::Mat<_SRC_T, HEIGHT, WIDTH, _NPC> &imgInput, xf::Mat<_DST_T, HEIGHT, WIDTH, _NPC> &imgOutput, ap_int<4> _convert_type, int shift);

#endif // _XF_CONVERT_BITDEPTH_CONFIG_H_


