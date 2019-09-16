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
#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_WRAPPER__
#define __XF_PYR_DENSE_OPTICAL_FLOW_WRAPPER__

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "video/xf_pyr_dense_optical_flow.hpp"

namespace xf{

#pragma SDS data mem_attribute("_current_img.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS, "_next_image.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_streamFlowin.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_streamFlowout.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_current_img.data":SEQUENTIAL, "_next_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_streamFlowin.data":SEQUENTIAL)
#pragma SDS data access_pattern("_streamFlowout.data":SEQUENTIAL)
#pragma SDS data copy("_current_img.data"[0:"_current_img.size"])
#pragma SDS data copy("_next_image.data"[0:"_next_image.size"])
#pragma SDS data copy("_streamFlowin.data"[0:"_streamFlowin.size"])
#pragma SDS data copy("_streamFlowout.data"[0:"_streamFlowout.size"])
#pragma SDS data data_mover("_current_img.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_next_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_streamFlowin.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_streamFlowout.data":AXIDMA_SIMPLE)
template<int NUM_PYR_LEVELS, int NUM_LINES, int WINSIZE, int FLOW_WIDTH, int FLOW_INT, int TYPE, int ROWS, int COLS, int NPC, bool USE_URAM = false>
void densePyrOpticalFlow(xf::Mat<XF_8UC1,ROWS,COLS,XF_NPPC1> & _current_img, xf::Mat<XF_8UC1,ROWS,COLS,XF_NPPC1> & _next_image, xf::Mat<XF_32UC1,ROWS,COLS,XF_NPPC1> & _streamFlowin, xf::Mat<XF_32UC1,ROWS,COLS,XF_NPPC1> & _streamFlowout, const int level, const unsigned char scale_up_flag, float scale_in, ap_uint<1> init_flag)
{
	#pragma HLS INLINE OFF
	xFLKOpticalFlowDenseKernel<ROWS, COLS, NUM_PYR_LEVELS, NUM_LINES, WINSIZE, FLOW_WIDTH, FLOW_INT, USE_URAM>((unsigned char *)_current_img.data, (unsigned char *)_next_image.data, (unsigned int *)_streamFlowin.data, (unsigned int *)_streamFlowout.data, _current_img.rows, _current_img.cols, _streamFlowin.rows, _streamFlowin.cols, level, scale_up_flag, scale_in, init_flag);
}
}	
#endif
