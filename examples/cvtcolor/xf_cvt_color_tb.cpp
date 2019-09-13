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
#include "xf_cvt_color_config.h"

#define ERROR_THRESHOLD 2

int main(int argc, char** argv) {

	uint16_t img_width;
	uint16_t img_height;

	cv::Mat inputimg0, inputimg1, inputimg2, inputimg;
	cv::Mat outputimg0, outputimg1, outputimg2;
	cv::Mat error_img0, error_img1, error_img2;
	cv::Mat refimage, refimg0, refimg1, refimg2;
	cv::Mat refimage0, refimage1, refimage2;

	cv::Mat img;

	
#if __SDSCC__
	perf_counter hw_ctr;
#endif
#if IYUV2NV12

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_uv = inputimg1.cols/2;
	int newheight_uv = inputimg1.rows+inputimg2.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0, CV_8UC1);
	error_img0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1, CV_16UC1);
	error_img1.create(S1, CV_16UC1);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgInput1(inputimg1.rows,inputimg1.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgInput2(inputimg2.rows,inputimg2.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);
	imgInput2.copyTo(inputimg2.data);

#if __SDSCC__
	hw_ctr.start();
#endif

	cvtcolor_iyuv2nv12(imgInput0,imgInput1,imgInput2,imgOutput0,imgOutput1);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();


	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_UV.png",imgOutput1);

	refimage0 = cv::imread(argv[4],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[5],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);
	cv::imwrite("y_error.png",error_img0);
	cv::imwrite("UV_error.png",error_img1);
#endif
#if IYUV2RGBA

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	cv::Size S0(inputimg0.cols,inputimg0.rows);
	outputimg0.create(S0, CV_8UC4);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows + inputimg2.rows;

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S(newwidth,newheight);
	outputimg1.create(S,CV_8UC3);

	//outputimg_ocv.create(S,CV_8UC4);
	error_img0.create(S,CV_8UC3);

//#if NO
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgInput1(inputimg1.rows,inputimg1.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgInput2(inputimg2.rows,inputimg2.cols);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgOutput0(inputimg0.rows,inputimg0.cols);
//#endif
//#if RO
	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);
	imgInput2.copyTo(inputimg2.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_iyuv2rgba(imgInput0,imgInput1,imgInput2,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	outputimg0.data = imgOutput0.copyFrom();

	cvtColor(outputimg0,outputimg1,CV_RGBA2BGR);
	cv::imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[4],1);

	absdiff(outputimg1,refimage,error_img0);


#endif
#if IYUV2RGB

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	cv::Size S0(inputimg0.cols,inputimg0.rows);
	outputimg0.create(S0, CV_8UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows + inputimg2.rows;

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S(newwidth,newheight);
	outputimg0.create(S,CV_8UC3);

	//outputimg_ocv.create(S,CV_8UC4);
	error_img0.create(S,CV_8UC3);

#if NO
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> imgInput1(inputimg1.rows,inputimg1.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> imgInput2(inputimg2.rows,inputimg2.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, XF_NPPC1> imgOutput0(inputimg0.rows,inputimg0.cols);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);
	imgInput2.copyTo(inputimg2.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_iyuv2rgb(imgInput0,imgInput1,imgInput2,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();

	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[4],1);

	absdiff(outputimg0,refimage,error_img0);
	cv::imwrite("diff.jpg",error_img0);
#endif
#endif
#if IYUV2YUV4

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg1 = cv::imread(argv[2], 0);
	if(!inputimg1.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}
	inputimg2 = cv::imread(argv[3], 0);
	if(!inputimg2.data)
	{
		std::cout << "Can't open image !!" << std::endl;
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_u = inputimg1.cols;
	int newheight_u = inputimg1.rows<<2;
	int newwidth_v = inputimg2.cols;
	int newheight_v = inputimg2.rows<<2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u,newheight_u);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	cv::Size S2(newwidth_v,newheight_v);
	outputimg2.create(S2,CV_8UC1);
	error_img2.create(S2,CV_8UC1);


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgInput1(inputimg1.rows,inputimg1.cols);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgInput2(inputimg2.rows,inputimg2.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput1(newheight_u,newwidth_u);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput2(newheight_v,newwidth_v);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);
	imgInput2.copyTo(inputimg2.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_iyuv2yuv4(imgInput0,imgInput1,imgInput2,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[4],0);
	if(!refimage0.data)
	{
		printf("unable to open image \n");
		return(1);
	}
	refimage1 = cv::imread(argv[5],0);
	if(!refimage1.data)
	{
		printf("unable to open image \n");
		return(1);
	}
	refimage2 = cv::imread(argv[6],0);
	if(!refimage2.data)
	{
		printf("unable to open image \n");
		return(1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

	imwrite("error_u.png",error_img1);
	imwrite("error_V.png",error_img2);

#endif
#if NV122IYUV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv122iyuv(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	
	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimg0 = cv::imread(argv[3],0);
	if(!refimg0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimg1 = cv::imread(argv[4],0);
	if(!refimg1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimg2 = cv::imread(argv[5],0);
	if(!refimg2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimg0, imgOutput0, error_img0);
	xf::absDiff(refimg1, imgOutput1, error_img1);
	xf::absDiff(refimg2, imgOutput2, error_img2);


	imwrite("error_Y.png", error_img0);
	imwrite("error_U.png", error_img1);
	imwrite("error_V.png", error_img2);
#endif

#if NV122RGBA

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);
	error_img0.create(S0,CV_8UC4);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv122rgba(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	outputimg0.data = imgOutput0.copyFrom();


	cvtColor(outputimg0,outputimg0,CV_RGBA2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);

#endif
#if (NV122RGB || NV122BGR)

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);
	error_img0.create(S0,CV_8UC3);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows;

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1	> imgOutput0(newheight,newwidth);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	if(NV122RGB)
	cvtcolor_nv122rgb(imgInput0,imgInput1,imgOutput0);
	if(NV122BGR)
	cvtcolor_nv122bgr(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();

	if(NV122RGB)
	{
	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	}

	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);

#endif

#if NV122YUV4

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows<<1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);


#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv122yuv4(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[5],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}


	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

#endif

#if NV122UYVY

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_uyvy = inputimg0.cols;
	int newheight_uyvy = inputimg0.rows;

	cv::Size S0(newwidth_uyvy,newheight_uyvy);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);


	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_uyvy,newwidth_uyvy);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);


#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv122uyvy(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();
	cv::imwrite("out_uyvy.png",outputimg0);
	refimage0 = cv::imread(argv[3],-1);


	if(!refimage0.data) {
		printf("Failed to open uyvy reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::imwrite("error_uyvy.png",error_img0);
#endif
#if NV122YUYV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_uyvy = inputimg0.cols;
	int newheight_uyvy = inputimg0.rows;

	cv::Size S0(newwidth_uyvy,newheight_uyvy);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);


	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_uyvy,newwidth_uyvy);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);


#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv122yuyv(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();

	cv::imwrite("out_yuyv.png",outputimg0);

	refimage0 = cv::imread(argv[3],-1);
	if(!refimage0.data) {
		printf("Failed to open yuyv reference image\n");
		return (1);
	}

	cv::absdiff(refimage0, outputimg0, error_img0);
	cv::imwrite("error_yuyv.png",error_img0);
#endif
#if (NV122NV21 ||NV212NV12)

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg1.cols;
	int newheight_uv = inputimg1.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);


#if __SDSCC__
	hw_ctr.start();
#endif
	if(NV122NV21)
	cvtcolor_nv122nv21(imgInput0,imgInput1,imgOutput0,imgOutput1);
	if(NV212NV12)
	cvtcolor_nv212nv12(imgInput0,imgInput1,imgOutput0,imgOutput1);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_y.png",imgOutput0);
	xf::imwrite("out_uv.png",imgOutput1);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],-1);
	if(!refimage0.data) {
		printf("Failed to open vu reference image\n");
		return (1);
	}
	absdiff(refimage1, outputimg1, error_img1);
	absdiff(refimage0, outputimg0, error_img0);
	cv::imwrite("error_img0.png",error_img0);
	cv::imwrite("error_img1.png",error_img1);

	FILE *fp=fopen("cv.txt","w");
	for(int i=0;i<(2073600);i++)
	{
		fprintf(fp,"%d\n",refimage0.data[i]);
	}
	fclose(fp);


	FILE *fp1=fopen("hls_out.txt","w");
	for(int i=0;i<(1080);i++)
	{
		for(int j=0;j<(1920);j++)
		{
			 fprintf(fp1, "%d\n",(unsigned char) outputimg0.data[i*(1920)+j]);//.chnl[0][0]);
		}
	}
	fclose(fp1);
#endif
#if NV212UYVY

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_uyvy = inputimg0.cols;
	int newheight_uyvy = inputimg0.rows;

	cv::Size S0(newwidth_uyvy,newheight_uyvy);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);


	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_uyvy,newwidth_uyvy);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);


#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv212uyvy(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	xf::imwrite("out_uyvy.png",imgOutput0);

	refimage0 = cv::imread(argv[3],-1);
	if(!refimage0.data) {
		printf("Failed to open uyvy reference image\n");
		return (1);
	}


	xf::absDiff(refimage0, imgOutput0, error_img0);
	cv::imwrite("error_uyvy.png",error_img0);
#endif
#if NV212YUYV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_uyvy = inputimg0.cols;
	int newheight_uyvy = inputimg0.rows;

	cv::Size S0(newwidth_uyvy,newheight_uyvy);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);


	img_width = inputimg0.cols;
	img_height = inputimg0.rows; // + inputimg1.rows;


	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_uyvy,newwidth_uyvy);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);


#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv212yuyv(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	xf::imwrite("out_yuyv.png",imgOutput0);

	refimage0 = cv::imread(argv[3],-1);
	if(!refimage0.data) {
		printf("Failed to open yuyv reference image\n");
		return (1);
	}
	xf::absDiff(refimage0, imgOutput0, error_img0);
	cv::imwrite("error_yuyv.png",error_img0);

#endif
#if NV212IYUV

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);

	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv212iyuv(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[5],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

#endif

#if NV212RGBA

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);
	outputimg1.create(S0,CV_8UC3);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv212rgba(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();


	cvtColor(outputimg0,outputimg1,CV_RGBA2BGR);
	imwrite("out.png", outputimg1);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg1,refimage,error_img0);

#endif
#if (NV212RGB || NV212BGR)

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth = inputimg0.cols;
	int newheight = inputimg0.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo(inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	if(NV212RGB)
	cvtcolor_nv212rgb(imgInput0,imgInput1,imgOutput0);
	if(NV212BGR)
	cvtcolor_nv212bgr(imgInput0,imgInput1,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();

	if(NV212RGB){
	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);}
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[3],1);

	absdiff(outputimg0,refimage,error_img0);

#endif
#if NV212YUV4

	inputimg0 = cv::imread(argv[1], 0);
	if(!inputimg0.data)
	{
		return -1;
	}
	inputimg1 = cv::imread(argv[2], -1);
	if(!inputimg1.data)
	{
		return -1;
	}

	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;

	int newwidth_u_v = inputimg1.cols<<1;
	int newheight_u_v = inputimg1.rows<<1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput0(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgInput1(inputimg1.rows,inputimg1.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput0.copyTo(inputimg0.data);
	imgInput1.copyTo((unsigned short int*)inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_nv212yuv4(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[3],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[4],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[5],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

#endif

#if RGBA2YUV4

	
	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGBA);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);
	imgInput.copyTo(inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif

	cvtcolor_rgba2yuv4(imgInput,imgOutput0,imgOutput1,imgOutput2);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);



	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

#endif

#if RGBA2IYUV

	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows>>2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGBA);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput.copyTo(inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgba2iyuv(imgInput,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

	imwrite("out_Y_error.png", error_img0);
	imwrite("out_U_error.png", error_img1);
	imwrite("out_V_error.png", error_img2);

#endif

#if RGBA2NV12

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGBA);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgba2nv12(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_UV.png",imgOutput1);
	xf::imwrite("out_Y.png",imgOutput0);


	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if RGBA2NV21

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGBA);


	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgba2nv21(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_VU.png",imgOutput1);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if RGB2IYUV

	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows>>2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGB);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput.copyTo(inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgb2iyuv(imgInput,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

	imwrite("out_Y_error.png", error_img0);
	imwrite("out_U_error.png", error_img1);
	imwrite("out_V_error.png", error_img2);

#endif
#if RGB2NV12

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGB);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgb2nv12(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = imgOutput1.copyFrom();

	xf::imwrite("out_UV.png",imgOutput1);
	xf::imwrite("out_Y.png",imgOutput0);
	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif
#if RGB2NV21

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGB);


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgb2nv21(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_VU.png",imgOutput1);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if RGB2YUV4


	inputimg = cv::imread(argv[1], 1);

	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	cvtColor(inputimg,inputimg,CV_BGR2RGB);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);
	imgInput.copyTo(inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif

	cvtcolor_rgb2yuv4(imgInput,imgOutput0,imgOutput1,imgOutput2);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);



	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);

#endif

#if RGB2UYVY

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int width_uyvy = inputimg0.cols;
	int height_uyvy = inputimg0.rows;

	cv::Size S0(width_uyvy,height_uyvy);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGB);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(height_uyvy,width_uyvy);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgb2uyvy(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = (unsigned char*)imgOutput0.copyFrom();

	xf::imwrite("out_uyvy.png",imgOutput0);


	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data)
	{
		std::cout << "Can't open UYVY ref image !!" << std::endl;
		return -1;
	}

	cv::absdiff(refimage0,outputimg0,error_img0);

	cv::imwrite("error_img0.png",error_img0);

	FILE *fp=fopen("cv.txt","w");
	for(int i=0;i<(2073600);i++)
	{
		fprintf(fp,"%d\n",refimage0.data[i]);
	}
	fclose(fp);


	FILE *fp1=fopen("hls_out.txt","w");
	for(int i=0;i<(1080);i++)
	{
		for(int j=0;j<(1920);j++)
		{
			 fprintf(fp1, "%d\n",(unsigned char) outputimg0.data[i*(1920)+j]);//.chnl[0][0]);
		}
	}
	fclose(fp1);
#endif

#if RGB2YUYV

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int width_yuyv = inputimg0.cols;
	int height_yuyv = inputimg0.rows;

	cv::Size S0(width_yuyv,height_yuyv);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	cvtColor(inputimg0,inputimg0,CV_BGR2RGB);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(height_yuyv,width_yuyv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgb2yuyv(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = (unsigned char*)imgOutput0.copyFrom();

	xf::imwrite("out_yuyv.png",imgOutput0);


	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data)
	{
		std::cout << "Can't open YUYV ref image !!" << std::endl;
		return -1;
	}

	cv::absdiff(refimage0,outputimg0,error_img0);

	cv::imwrite("error_img0.png",error_img0);

	FILE *fp=fopen("cv.txt","w");
	for(int i=0;i<(2073600);i++)
	{
		fprintf(fp,"%d\n",refimage0.data[i]);
	}
	fclose(fp);


	FILE *fp1=fopen("hls_out.txt","w");
	for(int i=0;i<(1080);i++)
	{
		for(int j=0;j<(1920);j++)
		{
			 fprintf(fp1, "%d\n",(unsigned char) outputimg0.data[i*(1920)+j]);//.chnl[0][0]);
		}
	}
	fclose(fp1);
#endif
#if RGB2BGR

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int width = inputimg0.cols;
	int height = inputimg0.rows;

	cv::Size S0(width,height);
	outputimg0.create(S0,CV_8UC3);
	error_img0.create(S0,CV_8UC3);

	cvtColor(inputimg0,inputimg1,CV_BGR2RGB);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(height,width);

	imgInput.copyTo(inputimg1.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_rgb2bgr(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = (unsigned char*)imgOutput0.copyFrom();

	xf::imwrite("out_bgr.png",imgOutput0);

	cv::absdiff(inputimg0,outputimg0,error_img0);

	cv::imwrite("error_img0.png",error_img0);

//	FILE *fp=fopen("cv.txt","w");
//	for(int i=0;i<(2073600);i++)
//	{
//		fprintf(fp,"%d\n",refimage0.data[i]);
//	}
//	fclose(fp);
//
//
//	FILE *fp1=fopen("hls_out.txt","w");
//	for(int i=0;i<(1080);i++)
//	{
//		for(int j=0;j<(1920);j++)
//		{
//			 fprintf(fp1, "%d\n",(unsigned char) outputimg0.data[i*(1920)+j]);//.chnl[0][0]);
//		}
//	}
//	fclose(fp1);
#endif

#if BGR2UYVY

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int width_uyvy = inputimg0.cols;
	int height_uyvy = inputimg0.rows;

	cv::Size S0(width_uyvy,height_uyvy);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(height_uyvy,width_uyvy);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_bgr2uyvy(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = (unsigned char*)imgOutput0.copyFrom();

	xf::imwrite("out_yuyv.png",imgOutput0);


	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data)
	{
		std::cout << "Can't open UYVY ref image !!" << std::endl;
		return -1;
	}

	cv::absdiff(refimage0,outputimg0,error_img0);

	cv::imwrite("error_img0.png",error_img0);

	FILE *fp=fopen("cv.txt","w");
	for(int i=0;i<(2073600);i++)
	{
		fprintf(fp,"%d\n",refimage0.data[i]);
	}
	fclose(fp);


	FILE *fp1=fopen("hls_out.txt","w");
	for(int i=0;i<(1080);i++)
	{
		for(int j=0;j<(1920);j++)
		{
			 fprintf(fp1, "%d\n",(unsigned char) outputimg0.data[i*(1920)+j]);//.chnl[0][0]);
		}
	}
	fclose(fp1);
#endif
#if BGR2YUYV

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int width_yuyv = inputimg0.cols;
	int height_yuyv = inputimg0.rows;

	cv::Size S0(width_yuyv,height_yuyv);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);

	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(height_yuyv,width_yuyv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_bgr2yuyv(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = (unsigned char*)imgOutput0.copyFrom();

	xf::imwrite("out_yuyv.png",imgOutput0);


	refimage0 = cv::imread(argv[2],-1);
	if(!refimage0.data)
	{
		std::cout << "Can't open YUYV ref image !!" << std::endl;
		return -1;
	}

	cv::absdiff(refimage0,outputimg0,error_img0);

	cv::imwrite("error_img0.png",error_img0);

	FILE *fp=fopen("cv.txt","w");
	for(int i=0;i<(2073600);i++)
	{
		fprintf(fp,"%d\n",refimage0.data[i]);
	}
	fclose(fp);


	FILE *fp1=fopen("hls_out.txt","w");
	for(int i=0;i<(1080);i++)
	{
		for(int j=0;j<(1920);j++)
		{
			 fprintf(fp1, "%d\n",(unsigned char) outputimg0.data[i*(1920)+j]);//.chnl[0][0]);
		}
	}
	fclose(fp1);
#endif
#if BGR2RGB

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int width = inputimg0.cols;
	int height = inputimg0.rows;

	cv::Size S0(width,height);
	outputimg0.create(S0,CV_8UC3);
	error_img0.create(S0,CV_8UC3);

	cvtColor(inputimg0,inputimg1,CV_BGR2RGB);
	img_height = inputimg0.rows;
	img_width = inputimg0.cols;

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(height,width);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_bgr2rgb(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = (unsigned char*)imgOutput0.copyFrom();

	xf::imwrite("out_bgr.png",imgOutput0);

	cv::absdiff(inputimg1,outputimg0,error_img0);

	cv::imwrite("error_img0.png",error_img0);
#endif
#if BGR2NV12

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	img_height = inputimg0.rows;
	img_width = inputimg0.cols;


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_bgr2nv12(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = imgOutput1.copyFrom();

	xf::imwrite("out_UV.png",imgOutput1);
	xf::imwrite("out_Y.png",imgOutput0);


	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif

#if BGR2NV21

	inputimg0 = cv::imread(argv[1], 1);
	if(!inputimg0.data)
	{
		return -1;
	}
	int newwidth_y = inputimg0.cols;
	int newheight_y = inputimg0.rows;
	int newwidth_uv = inputimg0.cols>>1;
	int newheight_uv = inputimg0.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_uv,newheight_uv);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);


	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg0.rows,inputimg0.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_uv,newwidth_uv);

	imgInput.copyTo(inputimg0.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_bgr2nv21(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_VU.png",imgOutput1);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);

#endif
#if UYVY2IYUV
	inputimg =cv::imread(argv[1],-1);
	if (!inputimg.data) {
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows >> 2;

	cv::Size S0(newwidth_y, newheight_y);
	outputimg0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_u_v, newheight_u_v);
	outputimg1.create(S1, CV_8UC1);
	outputimg2.create(S1, CV_8UC1);
	error_img0.create(S0, CV_8UC1);
	error_img1.create(S1, CV_8UC1);
	error_img2.create(S1, CV_8UC1);


	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput.copyTo((unsigned short int*)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_uyvy2iyuv(imgInput,imgOutput0, imgOutput1, imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = imgOutput1.copyFrom();
	outputimg2.data = imgOutput2.copyFrom();

	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 =cv::imread(argv[2],0);
	if (!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 =cv::imread(argv[3],0);
	if (!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 =cv::imread(argv[4],0);
	if (!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0, imgOutput0, error_img0);
	imwrite("out_Y_error.png", error_img0);
	xf::absDiff(refimage1, imgOutput1, error_img1);
	imwrite("out_U_error.png", error_img1);
	xf::absDiff(refimage2, imgOutput2, error_img2);
	imwrite("out_V_error.png", error_img2);
#endif
#if UYVY2NV12

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols>>1;
	int newheight_u_v = inputimg.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_u_v,newwidth_u_v);

	imgInput.copyTo((unsigned short int*)inputimg.data);
	
#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_uyvy2nv12(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_UV.png",imgOutput1);
	xf::imwrite("out_Y.png",imgOutput0);


	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);
#endif
#if UYVY2NV21

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols>>1;
	int newheight_u_v = inputimg.rows>>1;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);
	error_img0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_16UC1);
	error_img1.create(S1,CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_u_v,newwidth_u_v);

	imgInput.copyTo((unsigned short int*)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_uyvy2nv21(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();

	xf::imwrite("out_UV.png",imgOutput1);
	xf::imwrite("out_Y.png",imgOutput0);


	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data)
	{
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 = cv::imread(argv[3],-1);
	if(!refimage1.data)
	{
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0,refimage0,error_img0);
	absdiff(outputimg1,refimage1,error_img1);
#endif
#if UYVY2RGBA

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}

	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;
	cv::Mat outputimgrgba;
	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);
	error_img0.create(S0,CV_8UC4);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo((unsigned short int *)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_uyvy2rgba(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	outputimg0.data = imgOutput0.copyFrom();


	cvtColor(outputimg0,outputimg0,CV_RGBA2BGR);

	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to open reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
#endif


#if UYVY2RGB

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}

	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;
	cv::Mat outputimgrgba;
	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo((unsigned short int *)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_uyvy2rgb(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();

	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);

	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to open reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
#endif
#if UYVY2BGR

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}

	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;
	cv::Mat outputimgrgba;
	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo((unsigned short int *)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif

	cvtcolor_uyvy2bgr(imgInput,imgOutput0);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();

	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to open reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
#endif
#if (UYVY2YUYV ||YUYV2UYVY)

	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}

	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;
	cv::Mat outputimgrgba;
	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_16UC1);
	error_img0.create(S0,CV_16UC1);


	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo((unsigned short int *)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	if(UYVY2YUYV)
	cvtcolor_uyvy2yuyv(imgInput,imgOutput0);
	if(YUYV2UYVY)
	cvtcolor_yuyv2uyvy(imgInput,imgOutput0);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();

	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],-1);
	if(!refimage.data)
	{
		printf("\nFailed to open reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);
	imwrite("error_img0.png", error_img0);

#endif
#if YUYV2IYUV
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth_y = inputimg.cols;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols;
	int newheight_u_v = inputimg.rows>>2;

	cv::Size S0(newwidth_y,newheight_y);
	outputimg0.create(S0,CV_8UC1);

	cv::Size S1(newwidth_u_v,newheight_u_v);
	outputimg1.create(S1,CV_8UC1);
	outputimg2.create(S1,CV_8UC1);

	error_img0.create(S0,CV_8UC1);
	error_img1.create(S1,CV_8UC1);
	error_img2.create(S1,CV_8UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput1(newheight_u_v,newwidth_u_v);
	static xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> imgOutput2(newheight_u_v,newwidth_u_v);

	imgInput.copyTo((unsigned short int*)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_yuyv2iyuv(imgInput,imgOutput0,imgOutput1,imgOutput2);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	
	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_U.png",imgOutput1);
	xf::imwrite("out_V.png",imgOutput2);

	refimage0 = cv::imread(argv[2],0);
	if(!refimage0.data) {
		printf("Failed to open Y reference image\n");
		return (1);
	}
	refimage1 = cv::imread(argv[3],0);
	if(!refimage1.data) {
		printf("Failed to open U reference image\n");
		return (1);
	}
	refimage2 = cv::imread(argv[4],0);
	if(!refimage2.data) {
		printf("Failed to open V reference image\n");
		return (1);
	}

	xf::absDiff(refimage0,imgOutput0, error_img0);
	xf::absDiff(refimage1,imgOutput1, error_img1);
	xf::absDiff(refimage2,imgOutput2, error_img2);

#endif

#if YUYV2NV12

	inputimg =cv::imread(argv[1],-1);
	if (!inputimg.data) {
		return -1;
	}
	int newwidth_y = inputimg.cols; //>>1;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols >> 1;
	int newheight_u_v = inputimg.rows >> 1;

	cv::Size S0(newwidth_y, newheight_y);
	outputimg0.create(S0, CV_8UC1);
	error_img0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_u_v, newheight_u_v);
	outputimg1.create(S1, CV_16UC1);
	error_img1.create(S1, CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_u_v,newwidth_u_v);

	imgInput.copyTo((unsigned short int*)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_yuyv2nv12(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();


	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_UV.png",imgOutput1);

	printf("\n Written output images\n");
	refimage0 =cv::imread(argv[2],0);
	if (!refimage0.data) {
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 =cv::imread(argv[3],-1);
	if (!refimage1.data) {
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0, refimage0, error_img0);
	absdiff(outputimg1, refimage1, error_img1);

	imwrite("error_Y.png", error_img0);
	imwrite("error_UV.png", error_img1);
#endif
#if YUYV2NV21

	inputimg =cv::imread(argv[1],-1);
	if (!inputimg.data) {
		return -1;
	}
	int newwidth_y = inputimg.cols; //>>1;
	int newheight_y = inputimg.rows;
	int newwidth_u_v = inputimg.cols >> 1;
	int newheight_u_v = inputimg.rows >> 1;

	cv::Size S0(newwidth_y, newheight_y);
	outputimg0.create(S0, CV_8UC1);
	error_img0.create(S0, CV_8UC1);

	cv::Size S1(newwidth_u_v, newheight_u_v);
	outputimg1.create(S1, CV_16UC1);
	error_img1.create(S1, CV_16UC1);

	img_width = inputimg.cols;
	img_height = inputimg.rows;


	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput0(newheight_y,newwidth_y);
	static xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> imgOutput1(newheight_u_v,newwidth_u_v);

	imgInput.copyTo((unsigned short int*)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_yuyv2nv21(imgInput,imgOutput0,imgOutput1);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


	outputimg0.data = imgOutput0.copyFrom();
	outputimg1.data = (unsigned char*)imgOutput1.copyFrom();


	xf::imwrite("out_Y.png",imgOutput0);
	xf::imwrite("out_VU.png",imgOutput1);

	printf("\n Written output images\n");
	refimage0 =cv::imread(argv[2],0);
	if (!refimage0.data) {
		std::cout << "Can't open Y ref image !!" << std::endl;
		return -1;
	}

	refimage1 =cv::imread(argv[3],-1);
	if (!refimage1.data) {
		std::cout << "Can't open UV ref image !!" << std::endl;
		return -1;
	}

	absdiff(outputimg0, refimage0, error_img0);
	absdiff(outputimg1, refimage1, error_img1);

	imwrite("error_Y.png", error_img0);
	imwrite("error_UV.png", error_img1);
#endif


#if YUYV2RGBA
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC4);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC4, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo((unsigned short int*)inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_yuyv2rgba(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	outputimg0.data = imgOutput0.copyFrom();
	cvtColor(outputimg0,outputimg0,CV_RGBA2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to read reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);

#endif
#if YUYV2RGB
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo(inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_yuyv2rgb(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


		outputimg0.data = imgOutput0.copyFrom();


	cvtColor(outputimg0,outputimg0,CV_RGB2BGR);
	imwrite("out.png", outputimg0);

	refimage = cv::imread(argv[2],1);
	if(!refimage.data)
	{
		printf("\nFailed to read reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);

#endif
#if YUYV2BGR
	inputimg = cv::imread(argv[1], -1);
	if(!inputimg.data)
	{
		return -1;
	}
	int newwidth = inputimg.cols;
	int newheight = inputimg.rows;

	cv::Size S0(newwidth,newheight);
	outputimg0.create(S0,CV_8UC3);

	static xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput0(newheight,newwidth);

	imgInput.copyTo(inputimg.data);

#if __SDSCC__
	hw_ctr.start();
#endif
	cvtcolor_yuyv2bgr(imgInput,imgOutput0);
#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif


		outputimg0.data = imgOutput0.copyFrom();

	imwrite("out.png", outputimg0);


	refimage = cv::imread(argv[2],1);

	if(!refimage.data)
	{
		printf("\nFailed to read reference image\n");
		return -1;
	}
	absdiff(outputimg0,refimage,error_img0);

#endif
#if RGB2GRAY
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo((unsigned short int*)inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_rgb2gray(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2GRAY,1);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2GRAY
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC1);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_bgr2gray(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2GRAY,1);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if GRAY2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 0);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_gray2rgb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_GRAY2RGB,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if GRAY2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 0);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_gray2bgr(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_GRAY2BGR,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if RGB2XYZ
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_rgb2xyz(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2XYZ);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2XYZ
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_bgr2xyz(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2XYZ);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if XYZ2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);
	cv::cvtColor(inputimg,inputimg,CV_RGB2XYZ);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_xyz2rgb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_XYZ2RGB);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if XYZ2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2XYZ);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_xyz2bgr(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_XYZ2BGR);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if RGB2YCrCb
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_rgb2ycrcb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2YCrCb);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2YCrCb
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_bgr2ycrcb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2YCrCb);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if YCrCb2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2YCrCb);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_ycrcb2rgb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_YCrCb2RGB);

	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if YCrCb2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2YCrCb);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_ycrcb2bgr(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();


		//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_YCrCb2BGR);

	FILE *fpp=fopen("diff.txt","w");
	FILE *fpp1=fopen("hls_out.txt","w");
	FILE *fpp2=fopen("cv_out.txt","w");
	FILE *fpp3=fopen("diff.txt","w");
	for(int c=1;c<(inputimg.rows*inputimg.cols*3);c+=3)
	{
			short int diffpix=(unsigned char)outputimg.data[c]-(unsigned char)ocv_outputimg.data[c];
			fprintf(fpp1,"%d\n",(unsigned char)outputimg.data[c]);
			fprintf(fpp2,"%d\n",(unsigned char)ocv_outputimg.data[c]);

			if((diffpix > 1)||(diffpix< -1))
			{
				fprintf(fpp3,"%d	loc:%d\n",(unsigned char)diffpix,c);
//				fprintf(fpp,"%d (dut:%d, cv:%d),%d,(r-%d g-%d b-%d)\n",(short int)diffpix,(unsigned char)outputimg.data[c],(unsigned char)ocv_outputimg.data[c],c,(unsigned char)imgInput.data[c].chnl[1][0],(unsigned char)imgInput.data[c].chnl[1][2],(unsigned char)imgInput.data[c].chnl[1][3]);
			}

	}

	fclose(fpp);
	fclose(fpp1);
	fclose(fpp2);
	fclose(fpp3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);
//	FILE *fpp1=fopen("diff.txt","w");
//	for(int c=0;c<(2073600*3);c++)
//	{
//		if(error_img0.data[c]>2)
//		{
//			fprintf(fpp1,"%d\n",(unsigned char)error_img0.data[c]);
//		}
//	}

//	fclose(fpp1);
	cv::imwrite("error_img0.jpg",error_img0);
#endif
#if RGB2HLS
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	printf("before MAT call\n");
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		printf("before top function call\n");
		cvtcolor_rgb2hls(imgInput,imgOutput);
		printf("after top function call\n");
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();
	printf("assigning xfdata to opencv call\n");
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2HLS);
	//c_Ref((float*)inputimg.data,(float*)ocv_outputimg.data,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2HLS
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_bgr2hls(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2HLS);
	//c_Ref((float*)inputimg.data,(float*)ocv_outputimg.data,3);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if HLS2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HLS);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_hls2rgb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HLS2RGB);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);


#endif
#if HLS2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HLS);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_hls2bgr(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HLS2BGR);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);


#endif
#if RGB2HSV
	cv::Mat outputimg,ocv_outputimg;
	inputimg1 = cv::imread(argv[1], 1);
	if(!inputimg1.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg1,inputimg,CV_BGR2RGB);

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_rgb2hsv(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_RGB2HSV);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if BGR2HSV
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}

	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);

	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_bgr2hsv(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();

	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_BGR2HSV);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if HSV2RGB
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HSV);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_hsv2rgb(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HSV2RGB);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
#if HSV2BGR
	cv::Mat outputimg,ocv_outputimg;
	inputimg = cv::imread(argv[1], 1);
	if(!inputimg.data)
	{
		return -1;
	}
	cv::cvtColor(inputimg,inputimg,CV_BGR2HSV);
	outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	ocv_outputimg.create(inputimg.rows, inputimg.cols, CV_8UC3);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgInput(inputimg.rows,inputimg.cols);
	static xf::Mat<XF_8UC3, HEIGHT, WIDTH, NPC1> imgOutput(outputimg.rows,outputimg.cols);

	imgInput.copyTo(inputimg.data);

	#if __SDSCC__
		hw_ctr.start();
	#endif
		cvtcolor_hsv2bgr(imgInput,imgOutput);
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	outputimg.data = imgOutput.copyFrom();
	//OpenCV reference
	cv::cvtColor(inputimg,ocv_outputimg,CV_HSV2BGR);
	cv::imwrite("ocv_out.jpg",ocv_outputimg);
	cv::imwrite("hls_out.jpg",outputimg);
	absdiff(outputimg,ocv_outputimg,error_img0);

#endif
	double minval, maxval;
	float err_per;
	int cnt;

	minval = 255;
	maxval = 0;
	int max_i=0, max_j=0, min_i=0, min_j=0;
	cnt = 0;
	for (int i = 0; i < error_img0.rows; i++) {
		for (int j = 0; j < error_img0.cols; j++) {
			uchar v = error_img0.at<uchar>(i, j);

			if (v > ERROR_THRESHOLD)
				cnt++;
			if (minval > v)
			{
				minval = v;
				min_i = i;
				min_j = j;
			}
			if (maxval < v)
			{
				maxval = v;
				max_i = i;
								max_j = j;
			}
		}
	}
	err_per = 100.0 * (float) cnt / (error_img0.rows * error_img0.cols);
	fprintf(stderr,
			"Minimum error in intensity = %f (%d, %d)\n\
									Maximum error in intensity = %f (%d, %d)\n\
									Percentage of pixels above error threshold = %f\n",
			minval, min_i, min_j, maxval, max_i, max_j, err_per);

	if (err_per > 3.0f)
	{
		printf("\n1st Image Test Failed\n");
	//	return 1;
	}
//xf::analyzeDiff(error_img0,ERROR_THRESHOLD,err_per);
#if (IYUV2NV12 || RGBA2NV12 || RGBA2NV21 || UYVY2NV12 || YUYV2NV12 || NV122IYUV || NV212IYUV || IYUV2YUV4 || NV122YUV4 || NV212YUV4 || RGBA2IYUV || RGBA2YUV4 || UYVY2IYUV || YUYV2IYUV ||NV122NV21 ||NV212NV12)
	minval = 255;
	maxval = 0;
	cnt = 0;
	for (int i = 0; i < error_img1.rows; i++) {
		for (int j = 0; j < error_img1.cols; j++) {
			uchar v = error_img1.at<uchar>(i, j);

			if (v > ERROR_THRESHOLD)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	err_per = 100.0 * (float) cnt / (error_img1.rows * error_img1.cols);
	fprintf(stderr,
			"Minimum error in intensity = %f\n\
							Maximum error in intensity = %f\n\
							Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);
	if (err_per > 3.0f) 
	{
		printf("\n2nd Image Test Failed\n");
		return 1;
	}

#endif
#if (IYUV2YUV4 || NV122IYUV || NV122YUV4 || NV212IYUV || NV212YUV4 || RGBA2IYUV || RGBA2YUV4 || UYVY2IYUV || YUYV2IYUV)
	minval = 255;
	maxval = 0;
	cnt = 0;
	for (int i = 0; i < error_img2.rows; i++) {
		for (int j = 0; j < error_img2.cols; j++) {
			uchar v = error_img2.at<uchar>(i, j);

			if (v > ERROR_THRESHOLD)
				cnt++;
			if (minval > v)
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	err_per = 100.0 * (float) cnt / (error_img2.rows * error_img2.cols);
	fprintf(stderr,
			"Minimum error in intensity = %f\n\
							Maximum error in intensity = %f\n\
							Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);
	if (err_per > 3.0f) 
	{
		printf("\n3rd Image Test Failed\n");
		return 1;
	}
#endif
	/* ## *************************************************************** ##*/
return 0;

}

