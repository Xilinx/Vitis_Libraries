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
#include "xf_headers.h"
#include "xf_stereo_pipeline_config.h"
#include "cameraParameters.h"

using namespace std;

int main(int argc, char** argv)
{
	cv::setUseOptimized(false);

	if(argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage: <executable> <left image> <right image>\n");
		return -1;
	}


	cv::Mat left_img, right_img;
	left_img = cv::imread(argv[1],0);
	right_img = cv::imread(argv[2],0);

	//////////////////	HLS TOP Function Call  ////////////////////////
	xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftMat(left_img.rows,left_img.cols);
	xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightMat(right_img.rows,right_img.cols);

	int rows = left_img.rows;
	int cols = left_img.cols;

	static xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxLMat(rows,cols);
	static xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyLMat(rows,cols);
	static xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapxRMat(rows,cols);
	static xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> mapyRMat(rows,cols);

	static xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftRemappedMat(rows,cols);
	static xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightRemappedMat(rows,cols);

	static xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat(rows,cols);

	// camera parameters for rectification
#if __SDSCC__
	ap_fixed<32,12> *cameraMA_l_fix = (ap_fixed<32,12>*)sds_alloc_non_cacheable(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *cameraMA_r_fix = (ap_fixed<32,12>*)sds_alloc_non_cacheable(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *irA_l_fix = (ap_fixed<32,12>*)sds_alloc_non_cacheable(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *irA_r_fix = (ap_fixed<32,12>*)sds_alloc_non_cacheable(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *distC_l_fix = (ap_fixed<32,12>*)sds_alloc_non_cacheable(XF_DIST_COEFF_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *distC_r_fix = (ap_fixed<32,12>*)sds_alloc_non_cacheable(XF_DIST_COEFF_SIZE*sizeof(ap_fixed<32,12>));
#else
	ap_fixed<32,12> *cameraMA_l_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *cameraMA_r_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *irA_l_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *irA_r_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *distC_l_fix = (ap_fixed<32,12>*)malloc(XF_DIST_COEFF_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *distC_r_fix = (ap_fixed<32,12>*)malloc(XF_DIST_COEFF_SIZE*sizeof(ap_fixed<32,12>));
#endif

//	leftMat.copyTo(left_img.data);
//	rightMat.copyTo(right_img.data);
	leftMat = xf::imread<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>(argv[1], 0);
	rightMat = xf::imread<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>(argv[2], 0);

	xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state;
	bm_state.preFilterCap = 31;
	bm_state.uniquenessRatio = 15;
	bm_state.textureThreshold = 20;
	bm_state.minDisparity = 0;

	// copy camera params
	for(int i=0; i<XF_CAMERA_MATRIX_SIZE; i++) {
		cameraMA_l_fix[i] = (ap_fixed<32,12>)cameraMA_l[i];
		cameraMA_r_fix[i] = (ap_fixed<32,12>)cameraMA_r[i];
		irA_l_fix[i] = (ap_fixed<32,12>)irA_l[i];
		irA_r_fix[i] = (ap_fixed<32,12>)irA_r[i];
	}

	// copy distortion coefficients
	for(int i=0; i<XF_DIST_COEFF_SIZE; i++) {
		distC_l_fix[i] = (ap_fixed<32,12>)distC_l[i];
		distC_r_fix[i] = (ap_fixed<32,12>)distC_r[i];
	}

	printf("starting the kernel...\n");
#ifdef __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	stereopipeline_accel(leftMat,rightMat,dispMat,mapxLMat,mapyLMat,mapxRMat,mapyRMat,leftRemappedMat,rightRemappedMat,bm_state,cameraMA_l_fix,cameraMA_r_fix,distC_l_fix, distC_r_fix, irA_l_fix, irA_r_fix,9,5);
#ifdef __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	printf("end of kernel...\n");

	cv::Mat out_disp_16(rows,cols,CV_16UC1);
	cv::Mat out_disp_img(rows,cols,CV_8UC1);

	out_disp_16.data = dispMat.copyFrom();

	out_disp_16.convertTo(out_disp_img, CV_8U, (256.0/NO_OF_DISPARITIES)/(16.));
	imwrite("hls_output.png",out_disp_img);
	printf ("run complete !\n\n");

	return 0;
}

