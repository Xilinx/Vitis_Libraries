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

/*
Description:
Reference code for Malvar-He-Cutler algorithm.

 */

#if __SDSCC__
#undef __ARM_NEON__
#undef __ARM_NEON
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#define __ARM_NEON__
#define __ARM_NEON
#else
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/imgproc/imgproc.hpp"
#endif

void demosaicImage (cv::Mat cfa_output, cv::Mat &output_image, int code) {
	int block[5][5];

	for (int i=0; i<output_image.rows; i++) {
		for (int j=0; j<output_image.cols; j++) {
			for (int k=-2, ki=0; k<3; k++, ki++) {
				for (int l=-2, li=0; l<3;l++, li++) {	
					if (i+k >=0 && i+k < output_image.rows && j+l >=0 && j+l < output_image.cols) {
						if (cfa_output.type() == CV_8UC1)
							block[ki][li] = (int)cfa_output.at<unsigned char>(i+k,j+l);
						else
							block[ki][li] = (int)cfa_output.at<unsigned short>(i+k,j+l);
					}
					else {
						block[ki][li] = 0;
					}
				}
			}
			cv::Vec3f out_pix;

			if(code==0){//BG
				if ((i&0x00000001)==0) { // B row
					if ((j&0x00000001) ==0) { // B location
						out_pix[0] = 8 * (float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
											+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
											+ 4.0 * (float)block[2][2] ;
						out_pix[2] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
											+ 2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
											+ 6.0 * (float)block[2][2];
					}
					else {			// G location
						out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2])
											+ (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] + block[3][1] + block[3][3] )
											+ 4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[4][2])
											+ 4.0 * (float)(block[1][2] + block[3][2]) + 0.5 * (float)(block[2][0] + block[2][4])
											+ 5.0 * (float)block[2][2];
					}
				}
				else {			// R row
					if ((j&0x00000001) ==0) { //G location
						out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4])
											+ (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] + block[3][1] + block[3][3] )
											+ 4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[2][4])
																	+ 4.0 * (float)(block[2][1] + block[2][3]) + 0.5 * (float)(block[0][2] + block[4][2])
																	+ 5.0 * (float)block[2][2];
					}
					else{	//R location
						out_pix[0] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
											+ 2.0*(float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
											+ 6.0*(float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
											+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
											+ 4.0 * (float)block[2][2] ;
						out_pix[2] = 8.0 * (float)block[2][2] ;
					}
				}
			}
			else if(code==1){//GB
				if ((i&0x00000001)==0) { // B row
					if ((j&0x00000001) ==0) { // G location
						out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2])
														+ (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] + block[3][1] + block[3][3] )
														+ 4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[4][2])
														+ 4.0 * (float)(block[1][2] + block[3][2]) + 0.5 * (float)(block[2][0] + block[2][4])
														+ 5.0 * (float)block[2][2];
					}
					else {			// B location
						out_pix[0] = 8 * (float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
														+ 4.0 * (float)block[2][2] ;
						out_pix[2] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
														+ 6.0 * (float)block[2][2];
					}
				}
				else {			// R row
					if ((j&0x00000001) ==0) { //R location
						out_pix[0] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0*(float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
														+ 6.0*(float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
														+ 4.0 * (float)block[2][2] ;
						out_pix[2] = 8.0 * (float)block[2][2] ;
					}
					else{	//G location
						out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4])
														+ (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] + block[3][1] + block[3][3] )
														+ 4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[2][4])
																				+ 4.0 * (float)(block[2][1] + block[2][3]) + 0.5 * (float)(block[0][2] + block[4][2])
																				+ 5.0 * (float)block[2][2];
					}
				}
			}
			else if(code==2){//GR
				if ((i&0x00000001)==0) { // R row
					if ((j&0x00000001) ==0) { // G location
						out_pix[0] = 0.5 * (float)(block[2][0] + block[2][4])+ (-1.0) * (float)(block[1][1] + block[1][3] + block[0][2] + block[4][2] + block[3][1] + block[3][3] )
									+ 4.0 * (float)(block[1][2] + block[3][2]) + 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[2][0] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[2][4])
									+ 4.0 * (float)(block[2][1] + block[2][3]) + 0.5 * (float)(block[0][2] + block[4][2])
									+ 5.0 * (float)block[2][2];
					}
					else {			// R location
						out_pix[0] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0*(float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
														+ 6.0*(float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
														+ 4.0 * (float)block[2][2] ;
						out_pix[2] = 8.0 * (float)block[2][2] ;
					}
				}
				else {			// B row
					if ((j&0x00000001) ==0) { //B location
						out_pix[0] = 8 * (float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
														+ 4.0 * (float)block[2][2] ;
						out_pix[2] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
														+ 2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
														+ 6.0 * (float)block[2][2];
					}
					else{	//G location
						out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2])
														+ (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] + block[3][1] + block[3][3] )
														+ 4.0 * (float)(block[2][1] + block[2][3]) + 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[4][2])
														+ 4.0 * (float)(block[1][2] + block[3][2]) + 0.5 * (float)(block[2][0] + block[2][4])
														+ 5.0 * (float)block[2][2];
					}
				}
			}
			else if(code==3){//RG
				if ((i&0x00000001)==0) { // R row
					if ((j&0x00000001) ==0) { // R location
						out_pix[0] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
										+ 2.0*(float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
										+ 6.0*(float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
										+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
										+ 4.0 * (float)block[2][2] ;
						out_pix[2] = 8.0 * (float)block[2][2] ;
					}
					else {			//G location
						out_pix[0] = -1.0 * (float)(block[0][2] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[4][2])
										+ 4.0 * (float)(block[1][2] + block[3][2])
										+ 0.5 * (float)(block[2][0] + block[2][4])
										+ 5.0 * (float)block[2][2];
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = 0.5 * (float)(block[0][2] + block[4][2])
										+ (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] + block[3][1] + block[3][3] )
										+ 4.0 * (float)(block[2][1] + block[2][3])
										+ 5.0 * (float)(block[2][2]);
					}
				}
				else {			// B row
					if ((j&0x00000001) ==0) { //G location
						out_pix[0] = 0.5 * (float)(block[0][2] + block[4][2])
										+ (-1.0) * (float)(block[1][1] + block[1][3] + block[2][0] + block[2][4] + block[3][1] + block[3][3] )
										+ 4.0 * (float)(block[2][1] + block[2][3])
										+ 5.0 * (float)(block[2][2]);
						out_pix[1] = 8.0 * (float)block[2][2];
						out_pix[2] = -1.0 * (float)(block[0][2] + block[1][1] + block [1][3] + block[3][1] + block[3][3] + block[4][2])
										+ 4.0 * (float)(block[1][2] + block[3][2])
										+ 0.5 * (float)(block[2][0] + block[2][4])
										+ 5.0 * (float)block[2][2];
					}
					else {			//B location
						out_pix[0] = 8.0 * (float)block[2][2];
						out_pix[1] = -1.0 * (float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
										+ 2.0 * (float)(block[1][2] + block[2][1] + block[2][3] + block[3][2])
										+ 4.0 * (float)block[2][2] ;
						out_pix[2] = -1.5 *(float)(block[0][2] + block[2][0] + block[2][4] + block[4][2])
										+ 2.0 * (float)(block[1][1] + block[1][3] + block[3][1] + block[3][3])
										+ 6.0 * (float)block[2][2];
					}
				}
			}
			out_pix /= 8.0;
			if (output_image.type() == CV_8UC3){
				output_image.at<cv::Vec3b>(i,j) =  (cv::Vec3b) (out_pix);
			}
			else
				output_image.at<cv::Vec3w>(i,j) = (cv::Vec3w) (out_pix);
		}
	}
}
