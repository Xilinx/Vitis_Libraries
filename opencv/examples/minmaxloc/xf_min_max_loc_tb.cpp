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
#include "xf_min_max_loc_config.h"


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, in_conv;

	/*  reading in the color image  */
	in_img = cv::imread(argv[1],0);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	/*  convert to gray  */
	//cvtColor(in_img,in_gray,CV_BGR2GRAY);

	/*  convert to 16S type  */
#if T_8U
	in_img.convertTo(in_conv,CV_8UC1);
#elif T_16U
	in_img.convertTo(in_conv,CV_16UC1);
#elif T_16S
	in_img.convertTo(in_conv,CV_16SC1);
#elif T_32S
	in_img.convertTo(in_conv,CV_32SC1);
#endif



	double cv_minval=0,cv_maxval=0;
	cv::Point cv_minloc,cv_maxloc;

	/////////  OpenCV reference  ///////
	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif
	cv::minMaxLoc(in_conv, &cv_minval, &cv_maxval, &cv_minloc, &cv_maxloc, cv::noArray());
	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	int32_t min_value, max_value;
	uint16_t _min_locx,_min_locy,_max_locx,_max_locy;



	static xf::Mat<PTYPE, HEIGHT, WIDTH, _NPPC> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1, HEIGHT, WIDTH, _NPPC> in_8bit(in_img.rows,in_img.cols);

	in_8bit.copyTo(in_img.data);
	
#if T_8U
	imgInput=in_8bit;
#elif T_16U
	in_8bit.convertTo(imgInput,XF_CONVERT_8U_TO_16U);
#elif T_16S
	in_8bit.convertTo(imgInput,XF_CONVERT_8U_TO_16S);
#elif T_32S
	in_8bit.convertTo(imgInput,XF_CONVERT_8U_TO_32S);
#endif
	
	#if __SDSCC__
	perf_counter hw_ctr1;
	hw_ctr1.start();
	#endif
	min_max_loc_accel(imgInput, min_value, max_value, _min_locx, _min_locy, _max_locx, _max_locy);
	#if __SDSCC__
	hw_ctr1.stop();
	uint64_t hw_cycles1 = hw_ctr1.avg_cpu_cycles();
	#endif

	/////// OpenCV output ////////
	std::cout<<"OCV-Minvalue = "<<cv_minval<<std::endl;
	std::cout<<"OCV-Maxvalue = "<<cv_maxval<<std::endl;
	std::cout<<"OCV-Min Location.x = "<<cv_minloc.x<<"  OCV-Min Location.y = "<<cv_minloc.y<<std::endl;
	std::cout<<"OCV-Max Location.x = "<<cv_maxloc.x<<"  OCV-Max Location.y = "<<cv_maxloc.y<<std::endl;

	/////// Kernel output ////////
	std::cout<<"HLS-Minvalue = "<<min_value<<std::endl;
	std::cout<<"HLS-Maxvalue = "<<max_value<<std::endl;
	std::cout<<"HLS-Min Location.x = "<< _min_locx<< "  HLS-Min Location.y = "<<_min_locy<<std::endl;
	std::cout<<"HLS-Max Location.x = "<< _max_locx<< "  HLS-Max Location.y = "<<_max_locy<<std::endl;

	/////// printing the difference in min and max, values and locations of both OpenCV and Kernel function /////
	printf("Difference in Minimum value: %d \n",(cv_minval-min_value));
	printf("Difference in Maximum value: %d \n",(cv_maxval-max_value));
	printf("Difference in Minimum value location: (%d,%d) \n",(cv_minloc.y-_min_locy),(cv_minloc.x-_min_locx));
	printf("Difference in Maximum value location: (%d,%d) \n",(cv_maxloc.y-_max_locy),(cv_maxloc.x-_max_locx));


	if(((cv_minloc.y-_min_locy) > 1) | ((cv_minloc.x-_min_locx) > 1)){
		printf("\nTestcase failed\n");
		return -1;
	}

	return 0;
}
