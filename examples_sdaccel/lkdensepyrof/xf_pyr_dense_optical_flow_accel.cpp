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
#include "xf_pyr_dense_optical_flow_config.h"


extern "C" {
void pyr_dense_optical_flow_accel(ap_uint<INPUT_PTR_WIDTH> *_current_img, ap_uint<INPUT_PTR_WIDTH> *_next_image, ap_uint<OUTPUT_PTR_WIDTH> *_streamFlowin, ap_uint<OUTPUT_PTR_WIDTH> *_streamFlowout, int level, bool scale_up_flag, float scale_in, bool init_flag, int cur_img_rows, int cur_img_cols, int next_img_rows, int next_img_cols, int flow_rows, int flow_cols, int flow_iter_rows, int flow_iter_cols)
{	


#pragma HLS INTERFACE m_axi     port=_current_img  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=_next_image  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=_streamFlowin  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=_streamFlowout  offset=slave bundle=gmem4

#pragma HLS INTERFACE s_axilite port=level   bundle=control
#pragma HLS INTERFACE s_axilite port=scale_up_flag   bundle=control
#pragma HLS INTERFACE s_axilite port=scale_in   bundle=control
#pragma HLS INTERFACE s_axilite port=init_flag   bundle=control
#pragma HLS INTERFACE s_axilite port=cur_img_rows   bundle=control
#pragma HLS INTERFACE s_axilite port=cur_img_cols   bundle=control
#pragma HLS INTERFACE s_axilite port=next_img_rows   bundle=control
#pragma HLS INTERFACE s_axilite port=next_img_cols   bundle=control
#pragma HLS INTERFACE s_axilite port=flow_rows   bundle=control
#pragma HLS INTERFACE s_axilite port=flow_cols   bundle=control
#pragma HLS INTERFACE s_axilite port=flow_iter_rows   bundle=control
#pragma HLS INTERFACE s_axilite port=flow_iter_cols   bundle=control
#pragma HLS INTERFACE s_axilite port=return   bundle=control


  
  	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> current_img_mat;
#pragma HLS stream variable=current_img_mat.data depth=2
	current_img_mat.rows = cur_img_rows;
	current_img_mat.cols = cur_img_cols;

    xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> next_img_mat;
#pragma HLS stream variable=next_img_mat.data depth=2
	next_img_mat.rows = next_img_rows;
	next_img_mat.cols = next_img_cols;
	
	xf::Mat<XF_32UC1, HEIGHT, WIDTH, XF_NPPC1> streamFlowin_mat;
#pragma HLS stream variable=streamFlowin_mat.data depth=2
  streamFlowin_mat.rows = flow_rows;
  streamFlowin_mat.cols = flow_cols;
  
  	xf::Mat<XF_32UC1, HEIGHT, WIDTH, XF_NPPC1> streamFlowout_mat;
#pragma HLS stream variable=streamFlowout_mat.data depth=2
  streamFlowout_mat.rows = flow_iter_rows;
  streamFlowout_mat.cols = flow_iter_cols;
  
  
  	#pragma HLS DATAFLOW
	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(_current_img,current_img_mat);
	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>(_next_image,next_img_mat);
	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>(_streamFlowin,streamFlowin_mat);
	

	xf::densePyrOpticalFlow<NUM_LEVELS, NUM_LINES_FINDIT, WINSIZE_OFLOW, TYPE_FLOW_WIDTH, TYPE_FLOW_INT, XF_8UC1, HEIGHT, WIDTH, XF_NPPC1, XF_USE_URAM>(current_img_mat, next_img_mat, streamFlowin_mat, streamFlowout_mat, level, scale_up_flag, scale_in, init_flag);

	xf::xfMat2Array<OUTPUT_PTR_WIDTH,XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>(streamFlowout_mat,_streamFlowout);
	
	
	
	}
}
