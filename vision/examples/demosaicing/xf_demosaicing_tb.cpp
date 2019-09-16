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
#include "xf_demosaicing_config.h"

void bayerizeImage(cv::Mat img, cv::Mat &bayer_image, cv::Mat &cfa_output, int code) {

	//FILE *fp = fopen("output.txt","w");
	for (int i=0; i<img.rows; i++) {
		for (int j=0; j<img.cols; j++) {
			cv::Vec3b in = img.at<cv::Vec3b>(i,j);
			cv::Vec3b b;
			b[0] = 0; b[1] = 0; b[2] = 0;

			if(code == 0){//BG
				if ((i&1)==0) { // even row
					if ((j&1) ==0) { //even col
						b[0] = in[0];
						cfa_output.at<unsigned char>(i,j) = in[0];
					}
					else {			//odd col
						b[1] = in[1];
						cfa_output.at<unsigned char>(i,j) = in[1];
					}
				}
				else {			// odd row
					if ((j&1) ==0) { //even col
						b[1] = in[1];
						cfa_output.at<unsigned char>(i,j) = in[1];
					}
					else {			//odd col
						b[2] = in[2];
						cfa_output.at<unsigned char>(i,j) = in[2];
					}
				}
			}
			if(code == 1){//GB
				if ((i&1)==0) { // even row
					if ((j&1) ==0) { //even col
						b[1] = in[1];
						cfa_output.at<unsigned char>(i,j) = in[1];
					}
					else {			//odd col
						b[0] = in[0];
						cfa_output.at<unsigned char>(i,j) = in[0];
					}
				}
				else {			// odd row
					if ((j&1) ==0) { //even col
						b[2] = in[2];
						cfa_output.at<unsigned char>(i,j) = in[2];
					}
					else {			//odd col
						b[1] = in[1];
						cfa_output.at<unsigned char>(i,j) = in[1];
					}
				}
			}
			if(code == 2){//GR
				if ((i&1)==0) { // even row
					if ((j&1) ==0) { //even col
						b[1] = in[1];
						cfa_output.at<unsigned char>(i,j) = in[1];
					}
					else {			//odd col
						b[2] = in[2];
						cfa_output.at<unsigned char>(i,j) = in[2];
					}
				}
				else {			// odd row
					if ((j&1) ==0) { //even col
						b[0] = in[0];
						cfa_output.at<unsigned char>(i,j) = in[0];
					}
					else {			//odd col
						b[1] = in[1];
						cfa_output.at<unsigned char>(i,j) = in[1];
					}
				}
						}
			if(code == 3){//RG
						if ((i&1)==0) { // even row
							if ((j&1) ==0) { //even col
								b[2] = in[2];
								cfa_output.at<unsigned char>(i,j) = in[2];
							}
							else {			//odd col
								b[1] = in[1];
								cfa_output.at<unsigned char>(i,j) = in[1];
							}
						}
						else {			// odd row
							if ((j&1) ==0) { //even col
								b[1] = in[1];
								cfa_output.at<unsigned char>(i,j) = in[1];
							}
							else {			//odd col
								b[0] = in[0];
								cfa_output.at<unsigned char>(i,j) = in[0];
							}
						}
						}
			bayer_image.at<cv::Vec3b>(i,j) = b;
		}
	}
}


int main(int argc, char **argv) {
	
	//Check number of args
	if (argc != 2) {
		std::cout << "Usage incorrect! Correct usage: ./exe <image file>" << std::endl;
		return -1;
	}
	
	cv::Mat img = cv::imread(argv[1],1);
	if (img.empty()) {
		std::cout << "Image file not found, exiting! "<< std::endl;
		return -1;
	}
	
	// Create the Bayer pattern CFA output
	cv::Mat cfa_bayer_output (img.rows, img.cols, CV_8UC1); // simulate the Bayer pattern CFA output

#if (T_16U || T_10U || T_12U)
	cv::Mat cfa_bayer_16bit (img.rows, img.cols, CV_16UC1);
#endif
	cv::Mat color_cfa_bayer_output(img.rows, img.cols, img.type()); // Bayer pattern CFA output in color
	int code = BPATTERN;//Bayer format BG-0; GB-1; GR-2; RG-3

	bayerizeImage(img, color_cfa_bayer_output, cfa_bayer_output, code);
	imwrite("bayer_image.png",color_cfa_bayer_output);
	imwrite("cfa_output.png",cfa_bayer_output);
#if (T_16U || T_10U || T_12U)
	cfa_bayer_output.convertTo(cfa_bayer_16bit,CV_INTYPE);
#endif
	// Demosaic the CFA output using reference code

	cv::Mat ref_output_image(img.rows, img.cols, CV_OUTTYPE);
#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
#if (T_16U || T_10U || T_12U)
	demosaicImage (cfa_bayer_16bit, ref_output_image, code);
#else

	demosaicImage (cfa_bayer_output, ref_output_image, code);
#endif
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	imwrite("reference_output_image.png",ref_output_image);

	static xf::Mat<SRC_T, HEIGHT, WIDTH, NPPC> in_img(img.rows, img.cols);
	static xf::Mat<DST_T, HEIGHT, WIDTH, NPPC> out_img(img.rows, img.cols);
	cv::Mat output_image_hls(img.rows, img.cols, CV_OUTTYPE);

	int step = XF_PIXELDEPTH(XF_DEPTH(SRC_T,NPPC));
	for(int i=0; i<img.rows;i++){
		for(int j=0; j<((img.cols) >> (XF_BITSHIFT(NPPC)));j++){
			XF_TNAME(SRC_T,NPPC) pix=0;
			for(int k=0; k<NPPC;k++){
#if (T_16U || T_10U || T_12U)
				pix.range(step+step*k-1,step*k) = cfa_bayer_16bit.at<unsigned short>(i,j*NPPC+k);
#else
				pix.range(step+step*k-1,step*k) = cfa_bayer_output.data[NPPC*(i*((img.cols) >> (XF_BITSHIFT(NPPC)))+j)+k];
#endif
			}
			in_img.write(i*((img.cols) >> (XF_BITSHIFT(NPPC)))+j, pix);
		}
	}
#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
	/**********Calling the HLS function**********/
	demosaicing_accel(in_img,out_img);
#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif 

	xf::imwrite("output_image_hls.jpg",out_img);

	cv::Mat diff(out_img.rows, out_img.cols, CV_OUTTYPE);

	xf::absDiff(ref_output_image, out_img, diff);

	cv::imwrite("diff_img.jpg",diff);

	int err_thresh = 1;
	float err_per = 0;

	xf::analyzeDiff(diff, err_thresh, err_per);


	if(err_per>0)
		return 1;
	
	return 0;
}
