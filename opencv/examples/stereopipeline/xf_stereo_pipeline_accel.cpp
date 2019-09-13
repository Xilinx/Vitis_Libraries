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

void stereopipeline_accel(xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftMat, xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightMat, xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &dispMat,
	xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxLMat, xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyLMat, xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxRMat, 
	xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyRMat, xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftRemappedMat, xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightRemappedMat,
	xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> &bm_state, ap_fixed<32,12> *cameraMA_l_fix, ap_fixed<32,12> *cameraMA_r_fix, ap_fixed<32,12> *distC_l_fix, ap_fixed<32,12> *distC_r_fix, 
	ap_fixed<32,12> *irA_l_fix, ap_fixed<32,12> *irA_r_fix, int _cm_size, int _dc_size)
{
#pragma HLS INTERFACE m_axi depth=9 port=cameraMA_l_fix offset=direct bundle=cameraMA
#pragma HLS INTERFACE m_axi depth=9 port=cameraMA_r_fix offset=direct bundle=cameraMA
#pragma HLS INTERFACE m_axi depth=9 port=distC_l_fix offset=direct bundle=distC
#pragma HLS INTERFACE m_axi depth=9 port=distC_r_fix offset=direct bundle=distC
#pragma HLS INTERFACE m_axi depth=9 port=irA_l_fix offset=direct bundle=irA
#pragma HLS INTERFACE m_axi depth=9 port=irA_r_fix offset=direct bundle=irA

	xf::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE,XF_DIST_COEFF_SIZE,XF_32FC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(cameraMA_l_fix,distC_l_fix,irA_l_fix,mapxLMat,mapyLMat,_cm_size,_dc_size);
	xf::remap<XF_REMAP_BUFSIZE,XF_INTERPOLATION_BILINEAR,XF_8UC1,XF_32FC1,XF_8UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1,XF_USE_URAM>(leftMat,leftRemappedMat,mapxLMat,mapyLMat);

	xf::InitUndistortRectifyMapInverse<XF_CAMERA_MATRIX_SIZE,XF_DIST_COEFF_SIZE,XF_32FC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1>(cameraMA_r_fix,distC_r_fix,irA_r_fix,mapxRMat,mapyRMat,_cm_size,_dc_size);
	xf::remap<XF_REMAP_BUFSIZE,XF_INTERPOLATION_BILINEAR,XF_8UC1,XF_32FC1,XF_8UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1,XF_USE_URAM>(rightMat,rightRemappedMat,mapxRMat,mapyRMat);

	xf::StereoBM<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS,XF_8UC1,XF_16UC1,XF_HEIGHT,XF_WIDTH,XF_NPPC1,XF_USE_URAM>(leftRemappedMat, rightRemappedMat, dispMat, bm_state);
}
