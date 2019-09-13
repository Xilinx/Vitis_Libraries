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
#include "xf_corner_tracker_config.h"

#define VIDEO_INPUT 0

#if __SDSCC__
#define MEMORYALLOC(x) sds_alloc_non_cacheable(x)
#define MEMORYFREE(x) sds_free(x)
#else
#define MEMORYALLOC(x) malloc(x)
#define MEMORYFREE(x) free(x)
#endif
/* Color Coding */
// kernel returns this type. Packed strcuts on axi ned to be powers-of-2.
typedef struct __rgba{
	unsigned char r, g, b;
	unsigned char a;    // can be unused
} rgba_t;
typedef struct __rgb{
	unsigned char r, g, b;
} rgb_t;

typedef cv::Vec<unsigned short, 3> Vec3u;
typedef cv::Vec<unsigned char, 3> Vec3ucpt;

const float powTwo15 = pow(2,15);
#define THRESHOLD 3.0
#define THRESHOLD_R 3.0
/* color coding */

// custom, hopefully, low cost colorizer.
void getPseudoColorInt (unsigned char pix, float fx, float fy, rgba_t& rgba)
{
	// TODO get the normFac from the host as cmdline arg
	const int normFac = 10;

	int y = 127 + (int) (fy * normFac);
	int x = 127 + (int) (fx * normFac);
	if (y>255) y=255;
	if (y<0) y=0;
	if (x>255) x=255;
	if (x<0) x=0;

	rgb_t rgb;
	if (x > 127) {
		if (y < 128) {
			// 1 quad
			rgb.r = x - 127 + (127-y)/2;
			rgb.g = (127 - y)/2;
			rgb.b = 0;
		} else {
			// 4 quad
			rgb.r = x - 127;
			rgb.g = 0;
			rgb.b = y - 127;
		}
	} else {
		if (y < 128) {
			// 2 quad
			rgb.r = (127 - y)/2;
			rgb.g = 127 - x + (127-y)/2;
			rgb.b = 0;
		} else {
			// 3 quad
			rgb.r = 0;
			rgb.g = 128 - x;
			rgb.b = y - 127;
		}
	}

	rgba.r = pix*1/2 + rgb.r*1/2;
	rgba.g = pix*1/2 + rgb.g*1/2;
	rgba.b = pix*1/2 + rgb.b*1/2;
	rgba.a = 0;
}



float write_result_to_image_remap_seq (cv::Mat imgNext, cv::Mat imgref, cv::Mat glx, cv::Mat gly, int level, char filename[20], char filenamediff[20]) {

	cv::Mat imgRes(imgNext.size(), CV_8U);
	char file1[20], file2[20];
	sprintf(file1,"imgNext_level%d.png", level);;
	sprintf(file2,"result_level%d.png", level);;
	// imwrite(file1, imgNext);
	for (int i=0; i<imgRes.rows; i++) {
		for (int j=0; j<imgRes.cols; j++) {
			float indx = (float)j + glx.at<float>(i,j);
			float indy = (float)i + gly.at<float>(i,j);
			//cout << "i = "<<i<<"\t\tj = "<<j<<"\t\tindx = " << indx <<"\t\tindy = "<<indy<<endl;
			//float indx = (float)j;
			//float indy = (float)i;
			int i_indx = (int)indx;
			int i_indy = (int)indy;
			float fx = indx - (float)i_indx;
			float fy = indy - (float)i_indy;
			float iRes;
			if (i_indx>imgNext.cols-1 || i_indy>imgNext.rows-1 || i_indx<0 || i_indy<0) {
				iRes = 0;
			}
			else {
				unsigned char i0 = imgNext.at<unsigned char>(i_indy,   i_indx);
				unsigned char i1 = imgNext.at<unsigned char>(i_indy,   i_indx+1);
				unsigned char i2 = imgNext.at<unsigned char>(i_indy+1, i_indx);
				unsigned char i3 = imgNext.at<unsigned char>(i_indy+1, i_indx+1);

				iRes = (1-fx)*(1-fy)*((float)i0) + (fx)*(1-fy)*((float)i1) +
						(1-fx)*(fy)*((float)i2) + (fx)*(fy)*((float)i3);
			}
			imgRes.at<unsigned char>(i,j) = (unsigned char)iRes;
		}
	}

	cv::Mat res_absdiff;
	absdiff(imgRes, imgref, res_absdiff);
	float error_percentage=0;
	for(int t1=0; t1<res_absdiff.rows; t1++)
	{
		for(int t2=0; t2<res_absdiff.cols; t2++)
		{
			if(res_absdiff.at<unsigned char>(t1,t2)>2)
				error_percentage++;
		}
	}
	error_percentage = error_percentage*100.0/((res_absdiff.rows)*(res_absdiff.cols));
	cv::imwrite(filename, imgRes);
	cv::imwrite(filenamediff, res_absdiff);
	return error_percentage;
} //end write_result_to_image

int main (int argc, char **argv) {

	if (argc!=5) {
		std::cout << "Usage incorrect! Correct usage: ./exe\n<input video or path to input images>\n<no. of frames>\n<Harris Threshold>\n<No. of frames after which Harris Corners are Reset>" << std::endl;
		return -1;
	}
	char *path = argv[1];
#if VIDEO_INPUT
	cv::VideoCapture cap;

	std::stringstream imfile;
	imfile.str("");
	imfile << argv[1];
	if( imfile.str() == "" ){
		cap.open(0);
		std::cout << "Invalid input Video" << std::endl;
		if( !cap.isOpened() )
		{
			std::cout << "Could not initialize capturing...\n";
			return 1;
		}
	}
	else
		cap.open(imfile.str());

	unsigned int imageWidth = (cap.get(CV_CAP_PROP_FRAME_WIDTH));
	unsigned int imageHeight = (cap.get(CV_CAP_PROP_FRAME_HEIGHT));
#else	
	cv::Mat frame;
	char img_name[1000],out_img_name[50];
	sprintf(img_name,"%s/im%d.png",path,0);
	fprintf(stderr,"path is %s",img_name);
	frame = cv::imread(img_name,1);
	unsigned int imageWidth = frame.cols;
	unsigned int imageHeight = frame.rows;
	
#endif
	unsigned int harrisThresh = atoi(argv[3]);

	//allocating memory spaces for all the hardware operations
	bool harris_flag = true;
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> inHarris;
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> outHarris;
	unsigned long *listfixed = (unsigned long *)MEMORYALLOC((MAXCORNERS)*8);
	unsigned int *list = (unsigned int *)MEMORYALLOC((MAXCORNERS)*sizeof( unsigned int));
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>imagepyr1[NUM_LEVELS];
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC1>imagepyr2[NUM_LEVELS];
	static xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>flow;
	static xf::Mat<XF_32UC1,HEIGHT,WIDTH,XF_NPPC1>flow_iter;
	unsigned int num_corners = 0;
	for(int i=0; i<NUM_LEVELS ; i++)
	{
		imagepyr1[i].init(HEIGHT,WIDTH);
		imagepyr2[i].init(HEIGHT,WIDTH);
	}
	for(int initlf=0; initlf<MAXCORNERS; initlf++)
	{
		list[initlf] = 0;
		listfixed[initlf] = 0;
	}
	flow.init(HEIGHT,WIDTH);
	flow_iter.init(HEIGHT,WIDTH);
	inHarris.init(HEIGHT,WIDTH);
	outHarris.init(HEIGHT,WIDTH);
	//initializing flow pointers to 0
	//initializing flow vector with 0s
	cv::Mat init_mat= cv::Mat::zeros(HEIGHT,WIDTH, CV_32SC1);
	flow_iter.copyTo((XF_PTSNAME(XF_32UC1,XF_NPPC1)*)init_mat.data);
	flow.copyTo((XF_PTSNAME(XF_32UC1,XF_NPPC1)*)init_mat.data);
	init_mat.release();	
	// done
	std::cout << "num of test cases: " << atoi(argv[2]) << "\n";
	cv::Mat im0 = cv::Mat(imageHeight, imageWidth, CV_8UC1, imagepyr1[0].data);
	cv::Mat im1 = cv::Mat(imageHeight, imageWidth, CV_8UC1, imagepyr2[0].data);
	
#if VIDEO_INPUT	
	cv::Mat readVideoImage;
	for(int readn = 0; readn<1; readn++){
		cap >> readVideoImage;
		if( readVideoImage.empty() ){
			std::cout << "im1 is empty" << std::endl;
			break;
		}
	}
	cv::cvtColor(readVideoImage, im1, cv::COLOR_BGR2GRAY);

	cv::VideoWriter video("trackedCorners.avi",CV_FOURCC('M','J','P','G'),5, cv::Size(imageWidth,imageHeight),true);
#else
cv::cvtColor(frame, im1, cv::COLOR_BGR2GRAY);

#endif	
	

	for (int i=0;i<atoi(argv[2]);i++) {

		im1.copyTo(im0);
#if VIDEO_INPUT
		cap >> readVideoImage;
		if( readVideoImage.empty() ){
			std::cout << "im1 is empty" << std::endl;
			break;
		}
		else
		{
			std::cout << "Read frame no. " << i+1 << std::endl;
		}

		cv::cvtColor(readVideoImage, im1, cv::COLOR_BGR2GRAY);
#else

	sprintf(img_name,"%s/im%d.png",path,i+1);
	frame = cv::imread(img_name,1);
	cv::cvtColor(frame, im1, cv::COLOR_BGR2GRAY);
	
#endif

		std::cout << "***************************************************" << std::endl;
		std::cout << "Test Case no: " << i+1 << std::endl;

		// Auviz Hardware implementation
		cv::Mat glx (im0.size(), CV_32F, cv::Scalar::all(0)); // flow at each level is updated in this variable
		cv::Mat gly (im0.size(), CV_32F, cv::Scalar::all(0));
		/***********************************************************************************/
		//Setting image sizes for each pyramid level
		int pyr_w[NUM_LEVELS], pyr_h[NUM_LEVELS];
		pyr_h[0] = im0.rows;
		pyr_w[0] = im0.cols;
		for(int lvls=1; lvls< NUM_LEVELS; lvls++)
		{
			pyr_h[lvls] = (pyr_h[lvls-1]+1)>>1;
			pyr_w[lvls] = (pyr_w[lvls-1]+1)>>1;
		}
		inHarris.rows = pyr_h[0];
		inHarris.cols = pyr_w[0];
		inHarris.size = pyr_h[0] * pyr_w[0];
		outHarris.rows = pyr_h[0];
		outHarris.cols = pyr_w[0];
		outHarris.size = pyr_h[0] * pyr_w[0];
		inHarris.copyTo(im0.data);
	#if __SDSCC__
		perf_counter hw_ctr;
		hw_ctr.start();
	#endif
		cornerTracker (flow, flow_iter, imagepyr1, imagepyr2, inHarris, outHarris, list, listfixed, pyr_h, pyr_w, &num_corners, harrisThresh, &harris_flag);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
		std::cout << "Total Latency: " << (hw_cycles*1.0/1200000.0) << "ms" << std::endl;
	#endif

		cv::Mat outputimage;//(im0.rows, im0.cols, CV_8UC3, im1.data);
		cv::cvtColor(im1, outputimage, cv::COLOR_GRAY2BGR);
		for (int li=0; li<num_corners; li++)
		{
			unsigned int point = list[li];
			unsigned short row_num = 0;
			unsigned short col_num = 0;
			if(listfixed[li]>>42 == 0)
			{
				row_num = (point>>16) & 0x0000FFFF;//.range(31,16);
				col_num = point & 0x0000FFFF;//.range(15,0);
				cv::Point2f rmappoint((float)col_num,(float)row_num);
				cv::circle( outputimage, rmappoint, 2, cv::Scalar(0,0,255), -1, 8);
			}
		}
		#if VIDEO_INPUT
		video.write(outputimage);
		#else
		sprintf(out_img_name,"out_img%d.png",i);
		cv::imwrite(out_img_name,outputimage);	
		#endif
		std::cout << "***************************************************"<<std::endl;
		if((i+1)%atoi(argv[4])==0)
		{
			harris_flag = true;
			num_corners = 0;
			for(int init_list=0; init_list<MAXCORNERS; init_list++)
			{
				list[init_list] = 0;
				listfixed[init_list] = 0;
			}
		}
		//releaseing mats and pointers created inside the main for loop
		glx.release();
		gly.release();
		outputimage.release();
	}
	im0.data = NULL;
	im1.data = NULL;
	#if VIDEO_INPUT
	cap.release();
	video.release();
	#endif
	im0.release();
	im1.release();
	MEMORYFREE(list);
	return 0;
}
