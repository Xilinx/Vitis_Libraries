/*
 * Copyright 2022 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __XF_EXTRACT_EFRAMES_TB_CPP__
#define __XF_EXTRACT_EFRAMES_TB_CPP__

#include <iostream>
#include <string>
#include <map>
#include "common/xf_headers.hpp"
#include "xf_extract_eframes_tb_config.h"
#include "xf_extract_eframes_ref.hpp"
using namespace std;

static void Mat2MultiBayerAXIvideo(cv::Mat& img, InVideoStrm_t_e_s& AXI_video_strm, unsigned char InColorFormat) {
    int i, j, k, l;

#if T_8U
    unsigned char cv_pix;
#else
    unsigned short cv_pix;
#endif
    ap_axiu<AXI_WIDTH_IN, 1, 1, 1> axi;
    int depth = XF_DTPIXELDEPTH(IN_TYPE, NPPCX);

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols; j += NPPCX) {
            if ((i == 0) && (j == 0)) {
                axi.user = 1;
            } else {
                axi.user = 0;
            }
            if (j == (img.cols - NPPCX)) {
                axi.last = 1;
            } else {
                axi.last = 0;
            }
            axi.data = -1;
            for (l = 0; l < NPPCX; l++) {
                if (img.depth() == CV_16U)
                    cv_pix = img.at<unsigned short>(i, j + l);
                else
                    cv_pix = img.at<unsigned char>(i, j + l);
                switch (depth) {
                    case 10:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned short)cv_pix);
                        break;
                    case 12:
                        xf::cv::AXISetBitFields(axi, (l)*depth, depth, (unsigned short)cv_pix);
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

#if T_8U
    unsigned char cv_pix;
#else
    unsigned short cv_pix;
#endif
    int depth = XF_DTPIXELDEPTH(IN_TYPE, NPPCX);
    bool sof = 0;

    for (i = 0; i < img.rows; i++) {
        for (j = 0; j < img.cols / NPPCX; j++) { // 4 pixels read per iteration
            AXI_video_strm >> axi;
            if ((i == 0) && (j == 0)) {
                if (axi.user.to_int() == 1) {
                    sof = 1;
                } else {
                    j--;
                }
            }
            if (sof) {
                for (l = 0; l < NPPCX; l++) {
                    cv_pix = axi.data(l * depth + depth - 1, l * depth);

#if T_8U
                    img.at<unsigned char>(i, (NPPCX * j + l)) = cv_pix;
#else
                    img.at<unsigned short>(i, (NPPCX * j + l)) = cv_pix;
#endif
                }
            } // if(sof)
        }
    }
}

int main(int argc, char** argv) {
    int height, width;

    cv::Mat img1, img2, interleaved_img, lef_img, sef_img;
#if T_8U
    img1 = cv::imread(argv[1], 0);
    img2 = cv::imread(argv[2], 0);
#else
    img1 = cv::imread(argv[1], -1);
    img2 = cv::imread(argv[2], -1);
#endif

    height = img1.rows;
    width = img1.cols;
#if T_8U
    interleaved_img.create(cv::Size(img1.cols + NUM_H_BLANK, img1.rows * 2), CV_8UC1);
    lef_img.create(img1.rows, img1.cols, CV_8U);
    sef_img.create(img1.rows, img1.cols, CV_8U);
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            img1.at<unsigned char>(r, c) = 0;
            img2.at<unsigned char>(r, c) = 255;
        }
    }
#else
    interleaved_img.create(cv::Size(img1.cols + NUM_H_BLANK, img1.rows * 2), CV_16UC1);
    lef_img.create(img1.rows, img1.cols, CV_16U);
    sef_img.create(img1.rows, img1.cols, CV_16U);
    for (int r = 0; r < height; r++) {
        for (int c = 0; c < width; c++) {
            img1.at<unsigned short>(r, c) = 0;
            img2.at<unsigned short>(r, c) = 255;
        }
    }
#endif

    cv::imwrite("img1.png", img1);
    cv::imwrite("img2.png", img2);

#if T_8U
    int sc = 1;
    int cnt = 0, cnt1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width + NUM_H_BLANK; c++) {
            if (r < NUM_V_BLANK_LINES) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned char>(r, c) = img1.at<unsigned char>(r, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned char>(r, c) = 0;
            }

            if (r >= NUM_V_BLANK_LINES && r <= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (r % 2 == 0) {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned char>(r, c) = img2.at<unsigned char>(cnt, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned char>(r, c) = 0;
                } else {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned char>(r, c) =
                            img1.at<unsigned char>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned char>(r, c) = 0;
                }
            }
            if (r >= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned char>(r, c) = img2.at<unsigned char>(cnt, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned char>(r, c) = 0;
            }
        }
        if (r % 2 == 0 && r >= NUM_V_BLANK_LINES) {
            cnt++;
        }
        if (r % 2 != 0 && r >= NUM_V_BLANK_LINES) {
            cnt1++;
        }
    }
#else
    int sc = 1;
    int cnt = 0, cnt1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width + NUM_H_BLANK; c++) {
            if (r < NUM_V_BLANK_LINES) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned short>(r, c) = img1.at<unsigned short>(r, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned short>(r, c) = 0;
            }

            if (r >= NUM_V_BLANK_LINES && r <= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (r % 2 == 0) {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned short>(r, c) = img2.at<unsigned short>(cnt, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned short>(r, c) = 0;
                } else {
                    if (c >= NUM_H_BLANK)
                        interleaved_img.at<unsigned short>(r, c) =
                            img1.at<unsigned short>(cnt1 + NUM_V_BLANK_LINES - 1, c - NUM_H_BLANK);
                    else
                        interleaved_img.at<unsigned short>(r, c) = 0;
                }
            }
            if (r >= ((2 * height) - NUM_V_BLANK_LINES)) {
                if (c >= NUM_H_BLANK)
                    interleaved_img.at<unsigned short>(r, c) = img2.at<unsigned short>(cnt, c - NUM_H_BLANK);
                else
                    interleaved_img.at<unsigned short>(r, c) = 0;
            }
        }
        if (r % 2 == 0 && r >= NUM_V_BLANK_LINES) {
            cnt++;
        }
        if (r % 2 != 0 && r >= NUM_V_BLANK_LINES) {
            cnt1++;
        }
    }
#endif

    cv::Mat lef_img_ref, sef_img_ref;
#if T_8U
    lef_img_ref.create(img1.rows, img1.cols, CV_8U);
    sef_img_ref.create(img2.rows, img2.cols, CV_8U);
#else
    lef_img_ref.create(img1.rows, img1.cols, CV_16U);
    sef_img_ref.create(img2.rows, img2.cols, CV_16U);
#endif

    extractExposureFrames_ref(interleaved_img, lef_img_ref, sef_img_ref, img1.rows, img1.cols, NUM_V_BLANK_LINES,
                              NUM_H_BLANK);

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

    // FILE* fp7 = fopen("lef_img.csv", "w");
    // FILE* fp8 = fopen("lef_img_ref.csv", "w");
    // for (int i = 0; i < img1.rows; i++) {
    //     for (int j = 0; j < img1.cols; j++) {
    //         fprintf(fp7, "(%d %d %d) ",i,j,lef_img.at<unsigned short>(i,j));
    //         fprintf(fp8, "(%d %d %d) ",i,j,lef_img_ref.at<unsigned short>(i,j));

    //     }
    //     fprintf(fp7, "\n");
    //     fprintf(fp8, "\n");
    // }
    // fclose(fp7);
    // fclose(fp8);

    cv::imwrite("lef_img.png", lef_img);
    cv::imwrite("sef_img.png", sef_img);

    cv::imwrite("lef_img_ref.png", lef_img_ref);
    cv::imwrite("sef_img_ref.png", sef_img_ref);

    cv::Mat diff, diff1;
    cv::absdiff(lef_img, lef_img_ref, diff);
    cv::absdiff(sef_img, sef_img_ref, diff1);

    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    cv::imwrite("error1.png", diff1);

    float err_per, err_per1;
    xf::cv::analyzeDiff(diff, 1, err_per);
    xf::cv::analyzeDiff(diff1, 1, err_per1);

    if ((err_per > 1) || (err_per1 > 1)) {
        std::cerr << "ERROR: Test Failed." << std::endl;
        return 1;
    } else
        std::cout << "Test Passed " << std::endl;

    return 0;
}

#endif //__XF_EXTRACT_EFRAMES_TB_CPP__
