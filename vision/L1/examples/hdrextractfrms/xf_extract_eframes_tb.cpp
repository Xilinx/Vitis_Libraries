#ifndef __XF_EXTRACT_EFRAMES_TB_CPP__
#define __XF_EXTRACT_EFRAMES_TB_CPP__

#include <iostream>
#include <string>
#include <map>
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
#include "xf_extract_eframes_config.h"
using namespace std;

static void Mat2MultiBayerAXIvideo(cv::Mat& img, InVideoStrm_t_e_s& AXI_video_strm, unsigned char InColorFormat) {
    int i, j, k, l;

#if T_8U || T_10U || T_12U
    unsigned char cv_pix;
#else
    unsigned short cv_pix;
#endif
    ap_axiu<AXI_WIDTH_IN, 1, 1, 1> axi;
    int depth = XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC);

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols; j += XF_NPPC) {
            if ((i == 0) && (j == 0)) {
                axi.user = 1;
            } else {
                axi.user = 0;
            }
            if (j == (img.cols - XF_NPPC)) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }
            axi.data = -1;
            for (l = 0; l < XF_NPPC; l++) {
                if (img.depth() == CV_16U)
                    cv_pix = img.at<unsigned short>(i, j + l);
                else
                    cv_pix = img.at<unsigned char>(i, j + l);
                switch (depth) {
                    case 10:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                    case 12:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                    case 16:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned short)cv_pix);
                        break;
                    case CV_8U:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                    default:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned char)cv_pix);
                        break;
                }
            }
            axi.keep = -1;
            AXI_video_strm << axi;
        }
    }
}

/*********************************************************************************
 * Function:    MultiPixelAXIvideo2Mat
 * Parameters:  96bit stream with 4 pixels packed
 * Return:      None
 * Description: extract pixels from stream and write to open CV Image
 **********************************************************************************/
static void MultiPixelAXIvideo2Mat_gray(OutVideoStrm_t_e_s& AXI_video_strm, cv::Mat& img, unsigned char ColorFormat) {
    int i, j, k, l;
    ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> axi;

#if T_8U || T_10U || T_12U
    unsigned char cv_pix;
#else
    unsigned short cv_pix;
#endif
    int depth = XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC);
    bool sof = 0;

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols / XF_NPPC; j++) { // 4 pixels read per iteration
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
                    cv_pix = axi.data(l * depth + depth - 1, l * depth);

#if T_8U || T_10U || T_12U
                    img.at<unsigned char>(i, (XF_NPPC * j + l)) = cv_pix;
#else
                    img.at<unsigned short>(i, (XF_NPPC * j + l)) = cv_pix;
#endif
                }
            } // if(sof)
        }
    }
}

int main(int argc, char** argv) {
    int height, width;

    cv::Mat img1, img2, interleaved_img, lef_img, sef_img;

    img1 = cv::imread(argv[1], 0);
    img2 = cv::imread(argv[2], 0);

    height = img1.rows;
    width = img1.cols;

    interleaved_img.create(cv::Size(img1.cols, img1.rows * 2), img1.type());
    lef_img.create(img1.rows, img1.cols, CV_8U);
    sef_img.create(img1.rows, img1.cols, CV_8U);

    FILE* fp1 = fopen("img1.txt", "w");
    FILE* fp2 = fopen("img2.txt", "w");
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            img1.at<unsigned char>(r, c) = 0;
            img2.at<unsigned char>(r, c) = 255;
            fprintf(fp1, "%d ", img1.at<unsigned char>(r, c));
            fprintf(fp2, "%d ", img2.at<unsigned char>(r, c));
        }
        fprintf(fp1, "\n");
        fprintf(fp2, "\n");
    }
    fclose(fp1);
    fclose(fp2);

    cv::imwrite("img1.png", img1);
    cv::imwrite("img2.png", img2);

    int sc = 1;
    int cnt = 0, cnt1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width; c++) {
            if (r < 8) {
                interleaved_img.at<unsigned char>(r, c) = img1.at<unsigned char>(r, c);
            }

            if (r >= 8 && r <= ((2 * height) - 8)) {
                if (r % 2 == 0) {
                    interleaved_img.at<unsigned char>(r, c) = img2.at<unsigned char>(cnt, c);
                } else {
                    interleaved_img.at<unsigned char>(r, c) = img1.at<unsigned char>(cnt1 + 7, c);
                }
            }
            if (r >= ((2 * height) - 8)) {
                interleaved_img.at<unsigned char>(r, c) = img2.at<unsigned char>(cnt, c);
            }
        }
        if (r % 2 == 0 && r >= 8) {
            cnt++;
        }
        if (r % 2 != 0 && r >= 8) {
            cnt1++;
        }
    }

    unsigned char InColorFormat = 0;
    unsigned char OutColorFormat = 0;

    cv::imwrite("interleaved_img.png", interleaved_img);

    InVideoStrm_t_e_s src_axi;
    OutVideoStrm_t_e_s dst_axi1;
    OutVideoStrm_t_e_s dst_axi2;

    Mat2MultiBayerAXIvideo(interleaved_img, src_axi, InColorFormat);

    extractEFrames_accel(src_axi, dst_axi1, dst_axi2, height, width);

    MultiPixelAXIvideo2Mat_gray(dst_axi1, lef_img, OutColorFormat);
    MultiPixelAXIvideo2Mat_gray(dst_axi2, sef_img, OutColorFormat);

    cv::imwrite("lef_img.png", lef_img);
    cv::imwrite("sef_img.png", sef_img);

    return 0;
}

#endif //__XF_EXTRACT_EFRAMES_TB_CPP__
