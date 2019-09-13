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
#include "xf_harris_config.h"
#include "xf_ocv_ref.hpp"

int main(int argc,char **argv)
{
	cv::Mat in_img, img_gray;
	cv::Mat hls_out_img, ocv_out_img;
	cv::Mat ocvpnts, hlspnts;


	if(argc != 2){
		printf("Usage : <executable> <input image> \n");
		return -1;
	}
	in_img = cv::imread(argv[1],0); // reading in the color image

	if(!in_img.data)
	{
		printf("Failed to load the image ... %s\n!", argv[1]);
		return -1;
	}

	uint16_t Thresh;										// Threshold for HLS
	float Th;
	if (FILTER_WIDTH == 3){
		Th = 30532960.00;
		Thresh = 442;
	}
	else if (FILTER_WIDTH == 5){
		Th = 902753878016.0;
		Thresh = 3109;
	}
	else if (FILTER_WIDTH == 7){
		Th = 41151168289701888.000000;
		Thresh = 566;
	}
//	cvtColor(in_img, img_gray, CV_BGR2GRAY);
	// Convert rgb into grayscale
	hls_out_img.create(in_img.rows, in_img.cols, CV_8U); 					// create memory for hls output image
	ocv_out_img.create(in_img.rows, in_img.cols, CV_8U);		 			// create memory for opencv output image

	ocv_ref(in_img, ocv_out_img, Th);
	/**************		HLS Function	  *****************/
	float K = 0.04;
	uint16_t k = K*(1<<16);														// Convert to Q0.16 format
	uint32_t nCorners = 0;
	uint16_t imgwidth  = in_img.cols;
	uint16_t imgheight = in_img.rows;




#if NO
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);
//	imgInput = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(argv[1], 0);
#ifdef __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	harris_accel(imgInput,imgOutput, Thresh, k);
#ifdef __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

#endif

#if RO

	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC8> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC8> imgOutput(in_img.rows,in_img.cols);

	//imgInput.copyTo(img_gray.data);
	imgInput = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8>(argv[1], 0);
#ifdef __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
#endif
	harris_accel(imgInput,imgOutput, Thresh, k);
#ifdef __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
#endif

#endif

	///hls_out_img.data = (unsigned char *)imgOutput.copyFrom();
	xf::imwrite("hls_out.jpg",imgOutput);

	unsigned int val;
	unsigned short int row, col;

	cv::Mat out_img;
	out_img = in_img.clone();

	std::vector<cv::Point> hls_points;
	std::vector<cv::Point> ocv_points;
	std::vector<cv::Point> common_pts;
	/*						Mark HLS points on the image 				*/

	for( int j = 0; j < imgOutput.rows ; j++ ){
		int l=0;
		for( int i = 0; i < (imgOutput.cols>>XF_BITSHIFT(NPIX)); i++ ){

			if(NPIX==XF_NPPC8){
				ap_uint<64> value = imgOutput.read(j*(imgOutput.cols>>XF_BITSHIFT(NPIX))+i);//.at<unsigned char>(j,i);
				for(int k=0; k<64;k+=8,l++){
					uchar pix = value.range(k+7,k);
					if(pix != 0)
					{
						cv::Point tmp;
						tmp.x = l;
						tmp.y = j;
						if((tmp.x < in_img.cols) && (tmp.y <in_img.rows) && (j>0)){
							hls_points.push_back(tmp);
						}
						short int y, x;
						y = j;
						x = l;
						if (j > 0)
							cv::circle(out_img, cv::Point( x, y), 5,  Scalar(0,0,255,255), 2, 8, 0 );
					}
				}
			}
			if(NPIX==XF_NPPC1){
				unsigned char pix = imgOutput.read(j*(imgOutput.cols>>XF_BITSHIFT(NPIX))+i);
				if(pix != 0)
				{
					cv::Point tmp;
					tmp.x = i;
					tmp.y = j;
					if((tmp.x < in_img.cols) && (tmp.y <in_img.rows) && (j>0)){
						hls_points.push_back(tmp);
					}
					short int y, x;
					y = j;
					x = i;
					if (j > 0)
						cv::circle(out_img, cv::Point( x, y), 5,  Scalar(0,0,255,255), 2, 8, 0 );
				}
			}

		}
	}

	/*						End of marking HLS points on the image 				*/
	/*						Write HLS and Opencv corners into a file			*/

	ocvpnts = in_img.clone();


	int nhls = hls_points.size();

	/// Drawing a circle around corners
	for( int j = 1; j < ocv_out_img.rows-1 ; j++ ){
		for( int i = 1; i < ocv_out_img.cols-1; i++ ){
			if( (int)ocv_out_img.at<unsigned char>(j,i) ){
				cv::circle( ocvpnts, cv::Point( i, j ), 5,  Scalar(0,0,255), 2, 8, 0 );
				ocv_points.push_back(cv::Point(i,j));

			}
		}
	}

	printf("ocv corner count = %d, Hls corner count = %d\n", ocv_points.size(), hls_points.size());
	int nocv = ocv_points.size();

	/*									End 								*/
	/*							Find common points in among opencv and HLS						*/
	int ocv_x, ocv_y, hls_x, hls_y;
	for(int j=0;j<nocv;j++)
	{
		for(int k=0; k<nhls; k++)
		{
			ocv_x = ocv_points[j].x;
			ocv_y = ocv_points[j].y;
			hls_x = hls_points[k].x;
			hls_y = hls_points[k].y;

			if((ocv_x==hls_x) && (ocv_y==hls_y)){
				common_pts.push_back(ocv_points[j]);
				break;
			}
		}
	}
	/*							End								*/
	imwrite("output_hls.png", out_img);									// HLS Image
	imwrite("output_ocv.png", ocvpnts);									// Opencv Image
	/*						Success, Loss and Gain Percentages					*/
	float persuccess,perloss,pergain;

	int totalocv = ocv_points.size();
	int ncommon = common_pts.size();
	int totalhls = hls_points.size();
	persuccess = (((float)ncommon/totalhls)* 100) ;
	perloss = (((float)(totalocv-ncommon)/totalocv)*100);
	pergain = (((float)(totalhls-ncommon)/totalhls)*100);

	printf("Commmon = %d\t Success = %f\t Loss = %f\t Gain = %f\n",ncommon,persuccess,perloss,pergain);

	if(persuccess < 60 || totalhls == 0)
		return 1;


	return 0;
}

