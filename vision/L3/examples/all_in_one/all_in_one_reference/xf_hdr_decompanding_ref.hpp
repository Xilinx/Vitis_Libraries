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

void compute_pxl_ref(int pxl_val, int& out_val, int params[3][4][3], int color_idx) {
    if (pxl_val < params[color_idx][0][0]) {
        out_val = params[color_idx][0][1] * (pxl_val - params[color_idx][0][2]);
    } else if (pxl_val < params[color_idx][1][0]) {
        out_val = params[color_idx][1][1] * (pxl_val - params[color_idx][1][2]);
    } else if (pxl_val < params[color_idx][2][0]) {
        out_val = params[color_idx][2][1] * (pxl_val - params[color_idx][2][2]);
    } else {
        out_val = params[color_idx][3][1] * (pxl_val - params[color_idx][3][2]);
    }

    return;
}

void compute_pxl_12bit(int pxl_val, int& out_val, float params[3][4][3], int color_idx) {
    if (pxl_val < params[color_idx][0][0]) {
        out_val = params[color_idx][0][1] * pxl_val + params[color_idx][0][2];
    } else if (pxl_val < params[color_idx][1][0]) {
        out_val = params[color_idx][1][1] * pxl_val + params[color_idx][1][2];
    } else if (pxl_val < params[color_idx][2][0]) {
        out_val = params[color_idx][2][1] * pxl_val + params[color_idx][2][2];
    } else {
        out_val = params[color_idx][3][1] * pxl_val + params[color_idx][3][2];
    }

    return;
}

void hdr_decompanding_ref(
    cv::Mat& input_image, cv::Mat& output_image, int params[3][4][3], int bformat, int img_height, int img_width) {
    // // conversion from   16 bit to 12 bit
    // float params_16_to_12[3][4][3] = {
    //     {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}},
    //     {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}},
    //     {{4096, 0.25, 0}, {8192, 0.125, 512}, {32768, 0.0625, 1024}, {65536, 0.03125, 2048}}};
    // cv::Mat out_img_12bit;
    // out_img_12bit.create(input_image.rows, input_image.cols, CV_16UC1);

    // int pxl_val_12bit;
    // int out_val_12bit;
    // int color_idx_12bit, row_idx_12bit, col_idx_12bit;

    // // Convertion of 16bit image to 12bit image
    // for (int i = 0; i < img_height; i++) {
    //     for (int j = 0; j < img_width; j++) {
    //         pxl_val_12bit = input_image.at<CVTYPE>(i, j);
    //         row_idx_12bit = i;
    //         col_idx_12bit = j;

    //         if (bformat == XF_BAYER_GB) {
    //             col_idx_12bit += 1;
    //         }

    //         if (bformat == XF_BAYER_GR) {
    //             row_idx_12bit += 1;
    //         }

    //         if (bformat == XF_BAYER_RG) { // RGRG
    //                                         // GBGB
    //             col_idx_12bit += 1;
    //             row_idx_12bit += 1;
    //         }
    //         if ((row_idx_12bit & 1) == 0) {     // even row
    //             if ((col_idx_12bit & 1) == 0) { // even col
    //                 color_idx_12bit = 0;        // R location
    //             } else {                  // odd col
    //                 color_idx_12bit = 1;        // G location
    //             }
    //         } else {                      // odd row
    //             if ((col_idx_12bit & 1) == 0) { // even col
    //                 color_idx_12bit = 1;        // G location
    //             } else {                  // odd col
    //                 color_idx_12bit = 2;        // B location
    //             }
    //         }
    //         compute_pxl_12bit(pxl_val_12bit, out_val_12bit, params_16_to_12, color_idx_12bit);
    //         out_img_12bit.at<CVTYPE>(i, j) = out_val_12bit;
    //     }
    // }

    int color_idx, row_idx, col_idx;
    int pxl_val;
    int out_val;
    for (int i = 0; i < img_height; i++) {
        for (int j = 0; j < img_width; j++) {
            pxl_val = input_image.at<ushort>(i, j);
            row_idx = i;
            col_idx = j;

            if (bformat == XF_BAYER_GB) {
                col_idx += 1;
            }

            if (bformat == XF_BAYER_GR) {
                row_idx += 1;
            }

            if (bformat == XF_BAYER_RG) {
                col_idx += 1;
                row_idx += 1;
            }
            if ((row_idx & 1) == 0) {     // even row
                if ((col_idx & 1) == 0) { // even col
                    color_idx = 0;        // R location
                } else {                  // odd col
                    color_idx = 1;        // G location
                }
            } else {                      // odd row
                if ((col_idx & 1) == 0) { // even col
                    color_idx = 1;        // G location
                } else {                  // odd col
                    color_idx = 2;        // B location
                }
            }
            compute_pxl_ref(pxl_val, out_val, params, color_idx);
            output_image.at<ushort>(i, j) = out_val;
        }
    }
}
