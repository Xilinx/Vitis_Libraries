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

#ifndef _XF_HOG_DESCRIPTOR_CONFIG_H_
#define _XF_HOG_DESCRIPTOR_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_hog_descriptor.hpp"
#include "xf_config_params.h"

/* set the various hog parameters */
#define		XF_WIN_STRIDE		8
#define 	XF_BLOCK_HEIGHT	    	16
#define 	XF_BLOCK_WIDTH   	16
#define 	XF_CELL_HEIGHT		8
#define 	XF_CELL_WIDTH		8
#define 	XF_NO_OF_BINS   	9

#define 	OUT_T1		ap_uint16_t

// various parameters used for testing purpose and template usages, not to be edited
#define XF_NOVCPB   (XF_BLOCK_HEIGHT/XF_CELL_HEIGHT)  // number of vertical cells per block
#define XF_NOHCPB   (XF_BLOCK_WIDTH/XF_CELL_WIDTH)    // number of horizontal cells per block
#define XF_NOBPB    (XF_NO_OF_BINS*XF_NOHCPB*XF_NOVCPB)     // number of bins per block
#define XF_NOVBPW   ((XF_WIN_HEIGHT/XF_CELL_HEIGHT)-1)    // number of vertical blocks per window
#define XF_NOHBPW   ((XF_WIN_WIDTH/XF_CELL_WIDTH)-1)      // number of horizontal blocks per window
#define XF_NODPW    (XF_NOBPB*XF_NOVBPW*XF_NOHBPW)          // number of descriptors per window
#define XF_NOVW     (((XF_HEIGHT-XF_WIN_HEIGHT)/XF_WIN_STRIDE)+1)  // number of vertical windows in the image
#define XF_NOHW     (((XF_WIDTH-XF_WIN_WIDTH)/XF_WIN_STRIDE)+1)    // number of horizontal windows in the image
#define XF_NOVB     ((XF_HEIGHT/XF_CELL_HEIGHT)-1)   // number of vertical blocks in the image
#define XF_NOHB	    ((XF_WIDTH/XF_CELL_WIDTH)-1)     // number of horizontal blocks in the image
#if REPETITIVE_BLOCKS
#define XF_DESC_SIZE 	((XF_NOVW*XF_NOHW*XF_NODPW)>>1)
#define XF_OUTPUT_MODE  XF_HOG_RB
#elif NON_REPETITIVE_BLOCKS
#define XF_DESC_SIZE  	((XF_NOVB*XF_NOHB*XF_NOBPB)>>1)
#define XF_OUTPUT_MODE  XF_HOG_NRB
#endif
#if GRAY_T
#define XF_INPUT_TYPE XF_8UC1
#define XF_INPUT_COLOR XF_GRAY
#elif RGB_T
#define XF_INPUT_TYPE XF_8UC3
#define XF_INPUT_COLOR XF_RGB
#endif

void hog_descriptor_accel(xf::Mat<XF_INPUT_TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &inMat, xf::Mat<XF_32UC1, 1, XF_DESC_SIZE, XF_NPPC1> &outMat);

#endif  // end of _AU_HOG_CONFIG_H_
