/***************************************************************************
 Copyright (c) 2020, Xilinx, Inc.
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

#include "common/xf_headers.hpp"
#include "xf_isp_types.h"


using namespace std;



/*********************************************************************************
 * Function:    IplImage2MultiPixelAXIvideo
 * Parameters:
 * Return:
 * Description:  Currently fixed for Dual Pixel
 **********************************************************************************/
static void IplImage2MultiBayerAXIvideo(
		IplImage* img,
		InVideoStrm_t& AXI_video_strm,
		unsigned char InColorFormat)
{
	int i, j, k, l;
	CvScalar cv_pix;
	ap_axiu<AXI_WIDTH_IN, 1, 1, 1> axi;
	int depth = 8;

	for (i = 0; i < img->height; i++) {
		for (j = 0; j < img->width; j += XF_NPPC) {
			if ((i == 0) && (j == 0)) {
				axi.user = 1;
			} else {
				axi.user = 0;
			}
			if (j == (img->width - XF_NPPC)) {
				axi.last = 1;
			} else {
				axi.last = 0;
			}
			axi.data = -1;
			for (l = 0; l < XF_NPPC; l++) {
				cv_pix = cvGet2D(img, i, j + l);
				switch (img->depth) {
				case 10:
				case 12:
				case 16:
					xf::cv::AXISetBitFields(axi, (l) * depth, depth, (unsigned short) cv_pix.val[0]);
					break;
				case IPL_DEPTH_8U:
				default:
					xf::cv::AXISetBitFields(axi, (l) * depth, depth, (unsigned short) cv_pix.val[0]);
					break;
				}
			}
			axi.keep = -1;
			AXI_video_strm << axi;
		}
	}
}


/*********************************************************************************
 * Function:    MultiPixelAXIvideo2IplImage
 * Parameters:  96bit stream with 4 pixels packed
 * Return:      None
 * Description: extract pixels from stream and write to open CV Image
 **********************************************************************************/
static void MultiPixelAXIvideo2IplImage(OutVideoStrm_t& AXI_video_strm,
		IplImage* img, unsigned char ColorFormat) {
	int i, j, k, l;
	ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> axi;
	CvScalar cv_pix;
	int depth = 8;
	bool sof = 0;

	for (i = 0; i < img->height; i++) {
		for (j = 0; j < img->width / XF_NPPC; j++) { //4 pixels read per iteration
			AXI_video_strm >> axi;
			if ((i == 0) && (j == 0)) {
				if (axi.user.to_int() == 1) {
					sof = 1;
				} else {
					j--;
				}
			}
			if (sof) {
				for (l = 0; l < XF_NPPC; l++) {
					int num_comp =
							((ColorFormat == 0) || (ColorFormat == 1)
									|| (ColorFormat == 4)) ?
											(img->nChannels) : 2;
					for (k = 0; k < num_comp; k++) {
						#if XF_AXI_GBR==1
						const int mapComp[5][3] = { { 1, 0, 2 },     //RGB  //GBR
								{ 0, 1, 2 },     //4:4:4
								{ 0, 1, 1 },     //4:2:2
								{ 0, 1, 1 },     //4:2:0
								{ 1, 0, 2 }, };  //4:2:0 HDMI
						#else
							const int mapComp[5][3] = { { 0, 1, 2},     //RGB  //GBR
								{ 0, 1, 2 },     //4:4:4
								{ 0, 1, 1 },     //4:2:2
								{ 0, 1, 1 },     //4:2:0
								{ 1, 0, 2 }, };  //4:2:0 HDMI
						#endif
								int kMap = mapComp[ColorFormat][k];
								switch (depth) {
								case 10:
								case 12:
								case 16: {
									unsigned short temp;
									xf::cv::AXIGetBitFields(axi,
											(kMap + l * num_comp) * depth, depth, temp);
									cv_pix.val[k] = temp;
								}
								break;
								case IPL_DEPTH_8U:
								default: {
									unsigned char temp;
									xf::cv::AXIGetBitFields(axi,
											(kMap + l * num_comp) * depth, depth, temp);
									cv_pix.val[k] = temp;
								}
								break;
								}
					}
					cvSet2D(img, i, (XF_NPPC * j + l), cv_pix); //write p0
				}
			} //if(sof)
		}
	}
}

int main(int argc, char **argv) {


	IplImage *raw_input, *final_output;

	InVideoStrm_t src_axi;
	OutVideoStrm_t dst_axi;
	int result = 0;
	unsigned char InColorFormat = 0;
	CvSize imgSize;
	int nrFrames = 1;
	//read input image
	raw_input = cvLoadImage(argv[1], 0);

	HW_STRUCT_REG hwReg;
	// H/W Kernel configuration
	hwReg.width =raw_input->width;
	hwReg.height = raw_input->height;
	hwReg.bayer_phase = 0;

	imgSize.height = raw_input->height;
	imgSize.width = raw_input->width;

	// Allocate memory for final image
	final_output = cvCreateImage(imgSize, (8 > 8) ? 16 : 8, NR_COMPONENTS);

	cvSaveImage("input.png", raw_input);


	// As we are processing still image H/W kernel needs to be run twice

	for (int i = 0; i < 2; i++) {

	IplImage2MultiBayerAXIvideo(raw_input, src_axi, InColorFormat);

	//Call IP Processing function
	ISPPipeline_accel(hwReg, src_axi, dst_axi);


	//Convert processed image back to CV image, then to XVID image
	MultiPixelAXIvideo2IplImage(dst_axi, final_output, InColorFormat);

	}

	cvSaveImage("output.png", final_output);



	return 0;
}
