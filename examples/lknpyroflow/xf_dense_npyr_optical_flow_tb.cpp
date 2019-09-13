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
#include "xf_dense_npyr_optical_flow_config.h"

int main(int argc, char *argv[]){

	cv::Mat frame0, frame1;
	cv::Mat frame_out;

	if (argc != 3) {
		std::cout << "Usage incorrect. Correct usage: ./exe <current frame> <next frame>" << std::endl;
		return -1;
	}
	frame0 = cv::imread(argv[1],0);
	frame1 = cv::imread(argv[2],0);
	
	if (frame0.empty() || frame1.empty()) {
		std::cout << "input files not found!" << std::endl;
		return -1;
	}
	
	frame_out.create(frame0.rows, frame0.cols, CV_8UC4);
	int cnt = 0;
	unsigned char p1, p2, p3, p4;
	unsigned int pix =0;

	char out_string[200];
	static xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> buf0(frame0.rows,frame0.cols);
	static xf::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> buf1(frame0.rows,frame0.cols);
	static xf::Mat<XF_32FC1,MAX_HEIGHT, MAX_WIDTH, NPPC> flowx(frame0.rows,frame0.cols);
	static xf::Mat<XF_32FC1,MAX_HEIGHT, MAX_WIDTH, NPPC> flowy(frame0.rows,frame0.cols);
	
	buf0.copyTo(frame0.data);
	buf1.copyTo(frame1.data);
	//buf0 = xf::imread<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(argv[1], 0);
	//buf1 = xf::imread<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(argv[2], 0);

		
		#if __SDSCC__
			perf_counter hw_ctr;hw_ctr.start();
		#endif
			dense_non_pyr_of_accel(buf0, buf1, flowx, flowy);
		#if __SDSCC__
			hw_ctr.stop();
			uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
		#endif
		
		/* getting the flow vectors from hardware and colorcoding the vectors on a canvas of the size of the input*/
		float *flowx_copy;
		float *flowy_copy;
		flowx_copy = (float *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(float)));
		if(flowx_copy==NULL){
			fprintf(stderr,"\nFailed to allocate memory for flowx_copy\n");
		}
		flowy_copy = (float *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(float)));
		if(flowy_copy==NULL){
			fprintf(stderr,"\nFailed to allocate memory for flowy_copy\n");
		}
		unsigned int *outputBuffer;
		outputBuffer = (unsigned int *)malloc(MAX_HEIGHT*MAX_WIDTH*(sizeof(unsigned int)));
		if(outputBuffer==NULL){
			fprintf(stderr,"\nFailed to allocate memory for outputBuffer\n");
		}
		
		flowx_copy = (float *)flowx.copyFrom();
		flowy_copy = (float *)flowy.copyFrom();
		hls::stream <rgba_t> out_pix("Color pixel");
		xf::getOutPix<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(flowx_copy,flowy_copy,frame1.data,out_pix,frame0.rows,frame0.cols,frame0.cols*frame0.rows);
		xf::writeMatRowsRGBA<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1, KMED>(out_pix, outputBuffer,frame0.rows,frame0.cols,frame0.cols*frame0.rows);
		
		rgba_t *outbuf_copy;
		for(int i=0;i<frame0.rows;i++){
			for(int j=0;j<frame0.cols;j++){
				outbuf_copy = (rgba_t *) (outputBuffer + i*(frame0.cols) + j);
				p1 = outbuf_copy->r;
				p2 = outbuf_copy->g;
				p3 = outbuf_copy->b;
				p4 = outbuf_copy->a;
				pix = ((unsigned int)p4 << 24) | ((unsigned int)p3 << 16) | ((unsigned int)p2 << 8) | (unsigned int)p1 ;
				frame_out.at<unsigned int>(i,j) = pix;
			}
		}

		sprintf(out_string,"out_%d.png", cnt);
		cv::imwrite(out_string,frame_out);
	return 0;
}

	


