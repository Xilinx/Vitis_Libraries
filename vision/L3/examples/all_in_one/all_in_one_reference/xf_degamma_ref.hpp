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

void compute_pxl_degamma(int pxl_val, int& out_val, ap_ufixed<32, 18> params[3][DEGAMMA_KP][3], int color_idx) {
    for (int i = 0; i < DEGAMMA_KP; i++) {
        if (pxl_val < params[color_idx][i][0]) {
            out_val = params[color_idx][i][1] * (pxl_val)-params[color_idx][i][2];
            break;
        }
    }

    return;
}

void degamma_ref(cv::Mat& gamma_img, cv::Mat& out_img) {
#if T_8U
    typedef unsigned char Pixel_t;
#else
    typedef unsigned short Pixel_t;
#endif

#if T_8U
    ap_ufixed<32, 18> params[3][DEGAMMA_KP][3] = {{{32, 0.08, 0},
                                                   {64, 0.3, 7},
                                                   {96, 0.55, 23},
                                                   {128, 0.82, 49},
                                                   {160, 1.1, 84},
                                                   {192, 1.4, 132},
                                                   {224, 1.75, 200},
                                                   {256, 2, 256}},
                                                  {{32, 0.08, 0},
                                                   {64, 0.3, 7},
                                                   {96, 0.55, 23},
                                                   {128, 0.82, 49},
                                                   {160, 1.1, 84},
                                                   {192, 1.4, 132},
                                                   {224, 1.75, 200},
                                                   {256, 2, 256}},
                                                  {{32, 0.08, 0},
                                                   {64, 0.3, 7},
                                                   {96, 0.55, 23},
                                                   {128, 0.82, 49},
                                                   {160, 1.1, 84},
                                                   {192, 1.4, 132},
                                                   {224, 1.75, 200},
                                                   {256, 2, 256}}}; // 8 knee points {upper_bound, slope, intercept}

#endif

#if T_16U
    ap_ufixed<32, 18> params[3][DEGAMMA_KP][3] = {
        {{8192, 0.082, 0},
         {16384, 0.296, 1749},
         {24576, 0.545, 5825},
         {32768, 0.816, 12476},
         {40960, 1.1, 21782},
         {49152, 1.4, 34162},
         {57344, 1.715, 49506},
         {65536, 2.0, 65536}},
        {{8192, 0.082, 0},
         {16384, 0.296, 1749},
         {24576, 0.545, 5825},
         {32768, 0.816, 12476},
         {40960, 1.1, 21782},
         {49152, 1.4, 34162},
         {57344, 1.715, 49506},
         {65536, 2.0, 65536}},
        {{8192, 0.082, 0},
         {16384, 0.296, 1749},
         {24576, 0.545, 5825},
         {32768, 0.816, 12476},
         {40960, 1.1, 21782},
         {49152, 1.4, 34162},
         {57344, 1.715, 49506},
         {65536, 2.0, 65536}}}; // 8 knee points {upper_bound, slope, intercept}
#endif
    int pxl_val, out_val, color_idx, row_idx, col_idx;

    for (int i = 0; i < gamma_img.rows; i++) {
        for (int j = 0; j < gamma_img.cols; j++) {
            pxl_val = gamma_img.at<Pixel_t>(i, j);
            row_idx = i;
            col_idx = j;

            if (XF_BAYER_PATTERN == XF_BAYER_GB) {
                col_idx += 1;
            }

            if (XF_BAYER_PATTERN == XF_BAYER_GR) {
                row_idx += 1;
            }

            if (XF_BAYER_PATTERN == XF_BAYER_RG) {
                col_idx += 1;
                row_idx += 1;
            }                             // BG
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
            compute_pxl_degamma(pxl_val, out_val, params, color_idx);

            out_img.at<Pixel_t>(i, j) = out_val;
        }
    }
}
