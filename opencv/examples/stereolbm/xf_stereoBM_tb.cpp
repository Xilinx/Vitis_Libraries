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
#include "xf_stereoBM_config.h"

using namespace std;

int main(int argc, char** argv)
{
	cv::setUseOptimized(false);

	if(argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		return -1;
	}

	cv::Mat left_img, right_img;

	left_img = cv::imread(argv[1],0);
	right_img = cv::imread(argv[2],0);

	cv::Mat disp;

	//////////////////      OCV reference Function ////////////////////////
#if __SDSCC__
	cv::Ptr<cv::StereoBM> stereobm = cv::StereoBM::create(NO_OF_DISPARITIES,SAD_WINDOW_SIZE);
	stereobm-> setPreFilterCap(31);
	stereobm-> setUniquenessRatio(15);
	stereobm-> setTextureThreshold(20);
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	stereobm-> compute(left_img,right_img,disp);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

#else
	cv::StereoBM bm;
	bm.state->preFilterCap = 31;
	bm.state->preFilterType = CV_STEREO_BM_XSOBEL;
	bm.state->SADWindowSize = SAD_WINDOW_SIZE;
	bm.state->minDisparity = 0;
	bm.state->numberOfDisparities = NO_OF_DISPARITIES;
	bm.state->textureThreshold = 20;
	bm.state->uniquenessRatio = 15;
	bm(left_img,right_img,disp);
#endif

	cv::Mat disp8;
	disp.convertTo(disp8, CV_8U, (256.0/NO_OF_DISPARITIES)/(16.));
	imwrite("ocv_output.png",disp8);

	//////////////////	HLS TOP Function Call  ////////////////////////
	static xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftMat(left_img.rows,left_img.cols);
	static xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightMat(left_img.rows,left_img.cols);
	static xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat(left_img.rows,left_img.cols);
	static xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat_out(left_img.rows,left_img.cols);


	leftMat.copyTo(left_img.data);
	rightMat.copyTo(right_img.data);

	xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state;
	bm_state.preFilterCap = 31;
	bm_state.uniquenessRatio = 15;
	bm_state.textureThreshold = 20;
	bm_state.minDisparity = 0;

	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
	#endif
	stereolbm_accel(leftMat, rightMat, dispMat, bm_state);
	#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif
	
	
	dispMat.convertTo(dispMat_out, XF_CONVERT_16U_TO_8U, (256.0/NO_OF_DISPARITIES)/(16.));
	xf::imwrite("hls_out.jpg", dispMat_out);

	int cnt=0, total = 0;

	// changing the invalid value from negative to zero for validating the difference
	for (int i=0; i<disp.rows; i++) {
		for(int j=0; j<disp.cols; j++) {
			if (disp.at<short> (i,j) < 0) {
				disp.at<short> (i,j) = 0;
			}
		}
	}

	
	cv::Mat diff;
	diff.create(left_img.rows,left_img.cols,CV_16UC1);
	xf::absDiff(disp, dispMat, diff);
	cv::imwrite("diff_img.jpg",diff);

	// removing border before diff analysis
	cv::Mat diff_c;
	diff_c.create((diff.rows-SAD_WINDOW_SIZE<<1),diff.cols-(SAD_WINDOW_SIZE<<1),CV_16UC1);
	cv::Rect roi;
	roi.x = SAD_WINDOW_SIZE;
	roi.y = SAD_WINDOW_SIZE;
	roi.width = diff.cols-(SAD_WINDOW_SIZE<<1);
	roi.height = diff.rows-(SAD_WINDOW_SIZE<<1);
	diff_c = diff(roi);

	float err_per;
	xf::analyzeDiff(diff_c,0,err_per);

	return 0;
}


