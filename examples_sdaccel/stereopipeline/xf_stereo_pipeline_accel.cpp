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
#include "xf_stereo_pipeline_config.h"

extern "C" {
void stereopipeline_accel(ap_uint<INPUT_PTR_WIDTH> *img_L, ap_uint<INPUT_PTR_WIDTH> *img_R, ap_uint<OUTPUT_PTR_WIDTH> *img_disp,
	ap_fixed<32,12> *cameraMA_l, ap_fixed<32,12> *cameraMA_r, ap_fixed<32,12> *distC_l, ap_fixed<32,12> *distC_r, ap_fixed<32,12> *irA_l, 
	ap_fixed<32,12> *irA_r, int *bm_state_arr, int rows, int cols)
{
#pragma HLS INTERFACE m_axi     port=img_L  offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi     port=img_R  offset=slave bundle=gmem5
#pragma HLS INTERFACE m_axi     port=img_disp  offset=slave bundle=gmem6
#pragma HLS INTERFACE m_axi     port=cameraMA_l  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=cameraMA_r  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=distC_l  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=distC_r  offset=slave bundle=gmem3
#pragma HLS INTERFACE m_axi     port=irA_l  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=irA_r  offset=slave bundle=gmem2
#pragma HLS INTERFACE m_axi     port=bm_state_arr  offset=slave bundle=gmem4
#pragma HLS INTERFACE s_axilite port=rows               bundle=control
#pragma HLS INTERFACE s_axilite port=cols               bundle=control
#pragma HLS INTERFACE s_axilite port=return                bundle=control

	ap_fixed<32,12> cameraMA_l_fix[XF_CAMERA_MATRIX_SIZE], cameraMA_r_fix[XF_CAMERA_MATRIX_SIZE], distC_l_fix[XF_DIST_COEFF_SIZE], distC_r_fix[XF_DIST_COEFF_SIZE], irA_l_fix[XF_CAMERA_MATRIX_SIZE], irA_r_fix[XF_CAMERA_MATRIX_SIZE];

	for (int i=0; i<XF_CAMERA_MATRIX_SIZE; i++)
	{
#pragma HLS PIPELINE II=1
		cameraMA_l_fix[i] = cameraMA_l[i];
		cameraMA_r_fix[i] = cameraMA_r[i];
		irA_l_fix[i] = irA_l[i];
		irA_r_fix[i] = irA_r[i];
	}
	for (int i=0; i<XF_DIST_COEFF_SIZE; i++)
	{
#pragma HLS PIPELINE II=1
		distC_l_fix[i] = distC_l[i];
		distC_r_fix[i] = distC_r[i];
	}

	xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state;
	bm_state.preFilterType = bm_state_arr[0];
	bm_state.preFilterSize = bm_state_arr[1];
	bm_state.preFilterCap = bm_state_arr[2];
	bm_state.SADWindowSize = bm_state_arr[3];
	bm_state.minDisparity = bm_state_arr[4];
	bm_state.numberOfDisparities = bm_state_arr[5];
	bm_state.textureThreshold = bm_state_arr[6];
	bm_state.uniquenessRatio = bm_state_arr[7];
	bm_state.ndisp_unit = bm_state_arr[8];
	bm_state.sweepFactor = bm_state_arr[9];
	bm_state.remainder = bm_state_arr[10];

	int _cm_size = 9, _dc_size = 5;

	xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mat_L(rows,cols);
#pragma HLS stream variable=mat_L.data depth=2
	xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mat_R(rows,cols);
#pragma HLS stream variable=mat_R.data depth=2
	xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mat_disp(rows,cols);
#pragma HLS stream variable=mat_disp.data depth=2
	xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxLMat(rows,cols);
#pragma HLS stream variable=mapxLMat.data depth=2
	xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyLMat(rows,cols);
#pragma HLS stream variable=mapyLMat.data depth=2
	xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxRMat(rows,cols);
#pragma HLS stream variable=mapxRMat.data depth=2
	xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyRMat(rows,cols); 
#pragma HLS stream variable=mapyRMat.data depth=2
	xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftRemappedMat(rows,cols); 
#pragma HLS stream variable=leftRemappedMat.data depth=2
	xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightRemappedMat(rows,cols);
#pragma HLS stream variable=rightRemappedMat.data depth=2

#pragma HLS DATAFLOW

	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(img_L,mat_L);
	xf::Array2xfMat<INPUT_PTR_WIDTH,XF_8UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(img_R,mat_R);

	xf::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE,XF_DIST_COEFF_SIZE,XF_32FC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(cameraMA_l_fix,distC_l_fix,irA_l_fix,mapxLMat,mapyLMat,_cm_size,_dc_size);
	xf::remap<XF_REMAP_BUFSIZE,XF_INTERPOLATION_BILINEAR,XF_8UC1,XF_32FC1,XF_8UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1,XF_USE_URAM>(mat_L,leftRemappedMat,mapxLMat,mapyLMat);

	xf::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE,XF_DIST_COEFF_SIZE,XF_32FC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(cameraMA_r_fix,distC_r_fix,irA_r_fix,mapxRMat,mapyRMat,_cm_size,_dc_size);
	xf::remap<XF_REMAP_BUFSIZE,XF_INTERPOLATION_BILINEAR,XF_8UC1,XF_32FC1,XF_8UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1,XF_USE_URAM>(mat_R,rightRemappedMat,mapxRMat,mapyRMat);

	xf::StereoBM<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS,XF_8UC1,XF_16UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1,XF_USE_URAM>(leftRemappedMat, rightRemappedMat, mat_disp, bm_state);

	xf::xfMat2Array<OUTPUT_PTR_WIDTH,XF_16UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(mat_disp,img_disp);
}
}
