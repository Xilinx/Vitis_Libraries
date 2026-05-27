/*
* Copyright 2019 Xilinx, Inc.
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
#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <math.h>
#include "common/xf_types.hpp"
#include "common/xf_infra.hpp"

void extractExposureFrames_ref(cv::Mat& interleaved_image,
                               cv::Mat& lef_image,
                               cv::Mat& sef_image,
                               int rows,
                               int cols,
                               int N_ROWS_ref,
                               int N_COLS_ref) {
#if T_8U
    int height, width;
    height = lef_image.rows;
    width = lef_image.cols;
    int count = 0, count1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width + N_COLS_ref; c++) {
            if (r < N_ROWS_ref) {
                if (c >= N_COLS_ref)
                    lef_image.at<unsigned char>(r, c - N_COLS_ref) = interleaved_image.at<unsigned char>(r, c);
            }
            if (r >= N_ROWS_ref && r <= ((2 * height) - N_ROWS_ref)) {
                if (r % 2 == 0) {
                    if (c >= N_COLS_ref)
                        sef_image.at<unsigned char>(count, c - N_COLS_ref) = interleaved_image.at<unsigned char>(r, c);
                } else {
                    if (c >= N_COLS_ref)
                        lef_image.at<unsigned char>(count1 + N_ROWS_ref - 1, c - N_COLS_ref) =
                            interleaved_image.at<unsigned char>(r, c);
                }
            }
            if (r >= ((2 * height) - N_ROWS_ref)) {
                if (c >= N_COLS_ref) {
                    sef_image.at<unsigned char>(count, c - N_COLS_ref) = interleaved_image.at<unsigned char>(r, c);
                }
            }
        }

        if (r % 2 == 0 && r >= N_ROWS_ref && r <= ((2 * height) - N_ROWS_ref)) {
            count++;
        }
        if (r > ((2 * height) - N_ROWS_ref)) {
            count++;
        }
        if (r % 2 != 0 && r >= N_ROWS_ref && r <= ((2 * height) - N_ROWS_ref)) {
            count1++;
        }
    }
#else
    int height, width;
    height = lef_image.rows;
    width = lef_image.cols;
    int count = 0, count1 = 0;
    for (int r = 0; r < height * 2; r++) {
        for (int c = 0; c < width + N_COLS_ref; c++) {
            if (r <= N_ROWS_ref) {
                if (c >= N_COLS_ref)
                    lef_image.at<unsigned short>(r, c - N_COLS_ref) = interleaved_image.at<unsigned short>(r, c);
            }
            if (r >= N_ROWS_ref && r <= ((2 * height) - N_ROWS_ref)) {
                if (r % 2 == 0) {
                    if (c >= N_COLS_ref)
                        sef_image.at<unsigned short>(count, c - N_COLS_ref) =
                            interleaved_image.at<unsigned short>(r, c);
                } else {
                    if (c >= N_COLS_ref)
                        lef_image.at<unsigned short>(count1 + N_ROWS_ref, c - N_COLS_ref) =
                            interleaved_image.at<unsigned short>(r, c);
                }
            }
            if (r >= ((2 * height) - N_ROWS_ref)) {
                if (c >= N_COLS_ref)
                    sef_image.at<unsigned short>(count, c - N_COLS_ref) = interleaved_image.at<unsigned short>(r, c);
            }
        }
        if (r % 2 == 0 && r >= N_ROWS_ref && r <= ((2 * height) - N_ROWS_ref)) {
            count++;
        }
        if (r > ((2 * height) - N_ROWS_ref)) {
            count++;
        }
        if (r % 2 != 0 && r >= N_ROWS_ref && r <= ((2 * height) - N_ROWS_ref)) {
            count1++;
        }
    }
#endif

    // imwrite("lef_img_ref.png", lef_image);
    // imwrite("sef_img_ref.png", sef_image);
}

// double compute_datareliabilityweight(float& C, float& mu_h, float& mu_l, float& r) {
//     double wr;

//     if (r < mu_l)
//         wr = exp(-C * (std::pow((r - mu_l), 2)));
//     else if (r < mu_h)
//         wr = 1;
//     else
//         wr = exp(-C * (std::pow((r - mu_h), 2)));

//     return wr;
// }

// void HDR_merge_ref(cv::Mat& _src1,
//                    cv::Mat& _src2,
//                    double wr[NO_EXPS][W_B_SIZE],
//                    cv::Mat& final_img) {

// cv::Mat final_w1, final_w2;

// #if T_8U
//     final_img.create(_src1.rows, _src1.cols, CV_8UC1);
// #else
//     final_img.create(_src1.rows, _src1.cols, CV_16UC1);
// #endif
//     final_w1.create(_src1.rows, _src1.cols, CV_32FC1);
//     final_w2.create(_src1.rows, _src1.cols, CV_32FC1);

//     FILE* fp1 = fopen("imagevals_ocv.txt", "w");

//     for (int i = 0; i < _src1.rows; i++) {
//         for (int j = 0; j < _src1.cols; j++) {
// #if T_8U
//             int val1 = _src1.at<unsigned char>(i, j);
//             int val2 = _src2.at<unsigned char>(i, j);
// #else
//             int val1 = _src1.at<unsigned short>(i, j);
//             int val2 = _src2.at<unsigned short>(i, j);
// #endif
//             final_w1.at<float>(i, j) = (float)(wr[0][val1]);
//             final_w2.at<float>(i, j) = (float)(wr[1][val2]);

//             float val_1 = final_w1.at<float>(i, j) *
//                         val1; // (g_value_com(_src1.at<unsigned short>(i,j),alpha,optical_black_value)/t[0]);
//             float val_2 = final_w2.at<float>(i, j) *
//                         val2; //(g_value_com(_src2.at<unsigned short>(i,j),alpha,optical_black_value)/t[1]);

//             float sum_wei = final_w1.at<float>(i, j) + final_w2.at<float>(i, j);

//             int final_val = (int)((float)(val_1 + val_2) / (float)sum_wei);

//             if (final_val > (W_B_SIZE - 1)) {
//                 final_val = (W_B_SIZE - 1);
//             }
//             fprintf(fp1, "%d,", final_val);
// #if T_8U
//             final_img.at<unsigned char>(i, j) = (unsigned char)final_val;
// #else
//             final_img.at<unsigned short>(i, j) = (unsigned short)final_val;
// #endif
//         }
//         fprintf(fp1, "\n");
//     }
//     fclose(fp1);
// }