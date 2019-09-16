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
#include "xf_corner_tracker_config.h"

extern "C"
{
void cornerupdate_accel(unsigned long *list_fix, unsigned int *list, uint32_t nCorners, unsigned int *flow_vectors, bool harris_flag, int flow_rows, int flow_cols)
{
#pragma HLS INTERFACE m_axi     port=list_fix  offset=slave bundle=gmem7
#pragma HLS INTERFACE m_axi     port=list  offset=slave bundle=gmem8
#pragma HLS INTERFACE m_axi     port=flow_vectors  offset=slave bundle=gmem9

#pragma HLS INTERFACE s_axilite port=list_fix     bundle=control
#pragma HLS INTERFACE s_axilite port=list     bundle=control
#pragma HLS INTERFACE s_axilite port=flow_vectors     bundle=control

#pragma HLS INTERFACE s_axilite port=nCorners     bundle=control
#pragma HLS INTERFACE s_axilite port=harris_flag     bundle=control
#pragma HLS INTERFACE s_axilite port=flow_rows     bundle=control
#pragma HLS INTERFACE s_axilite port=flow_cols     bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control

xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1> flow_mat(flow_rows,flow_cols,flow_vectors);

xf::cornerUpdate<MAXCORNERS,XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>(list_fix, list, nCorners, flow_mat, (ap_uint<1>)(harris_flag));


}
}