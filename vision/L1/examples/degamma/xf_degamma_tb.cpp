/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <iostream>
#include <math.h>

#include "xf_degamma_tb_config.h"

void bayerizeImage(cv::Mat in_img, cv::Mat& bayer_image, cv::Mat& cfa_output, int code) {
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
#if T_8U
            cv::Vec3b in = in_img.at<cv::Vec3b>(i, j);
            cv::Vec3b b;
#else
            cv::Vec3w in = in_img.at<cv::Vec3w>(i, j);
            cv::Vec3w b;
#endif
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                }
            }
#if T_8U
            bayer_image.at<cv::Vec3b>(i, j) = b;
#else
            bayer_image.at<cv::Vec3w>(i, j) = b;
#endif
        }
    }
}

void compute_pxl(int pxl_val, int& out_val, ap_ufixed<32, 18> params[3][NUM][3], int color_idx) {
    for (int i = 0; i < NUM; i++) {
        if (pxl_val < params[color_idx][i][0]) {
            out_val = params[color_idx][i][1] * (pxl_val)-params[color_idx][i][2];
            break;
        }
    }

    return;
}
int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <INPUT IMAGE PATH > \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img, out_img, out_hls, diff, gamma_img, cfa_bayer_output;

// Reading in input image:
#if T_8U
    in_img = cv::imread(argv[1], 1); // read image
#else
    in_img = cv::imread(argv[1], -1); // read image
#endif

    if (in_img.empty()) {
        fprintf(stderr, "ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

#if T_8U
    out_img.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    out_hls.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    diff.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    gamma_img.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
#else
    out_img.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    out_hls.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    diff.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    gamma_img.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_OUT_TYPE);
#endif

    int height = in_img.rows;
    int width = in_img.cols;

    int out_val, out_val1;

    int pxl_val, pxl_val1;

    cv::Mat color_cfa_bayer_output(in_img.rows, in_img.cols, in_img.type()); // Bayer pattern CFA output in color
    unsigned short bformat = BPATTERN;                                       // Bayer format BG-0; GB-1; GR-2; RG-3

    bayerizeImage(in_img, color_cfa_bayer_output, cfa_bayer_output, bformat);
    cv::imwrite("bayer_image.png", color_cfa_bayer_output);
    cv::imwrite("cfa_output.png", cfa_bayer_output);

#if T_8U
    /*float params[3][N][3] = {{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}},
		{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}},
		{{64, 0.19, 0}, {128, 0.68, 31}, {192, 1.25, 104}, {256, 1.88, 225}}}; */ //

    ap_ufixed<32, 18> params[3][NUM][3] = {{{32, 0.08, 0},
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

    ap_ufixed<32, 18> params[3][NUM][3] = {{{8192, 0.082, 0},
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
    // degamma config

    /* center colomn is in Q18_14 format */

    unsigned int degamma_config[3][NUM][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < NUM; j++) {
            for (int k = 0; k < 3; k++) {
                if (k == 1) {
                    degamma_config[i][j][k] = (unsigned int)((params[i][j][k]) * (16384));
                } else {
                    degamma_config[i][j][k] = (unsigned int)((params[i][j][k]));
                }
            }
        }
    }

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
#if T_8U
            pxl_val1 = cfa_bayer_output.at<unsigned char>(i, j);
            float val = (float)pxl_val1 / 255;
            float val1 = (float)std::pow(val, 0.4545);
            float out_val1 = val1 * 255.0;

            gamma_img.at<unsigned char>(i, j) = (int)out_val1;
#else
            pxl_val1 = cfa_bayer_output.at<unsigned short>(i, j);
            float val = (float)pxl_val1 / 65535;
            float val1 = (float)std::pow(val, 0.4545);
            float out_val1 = val1 * 65535.0;

            gamma_img.at<unsigned short>(i, j) = (int)out_val1;
#endif
        }
    }
    imwrite("gamma.png", gamma_img);

    int color_idx, row_idx, col_idx;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pxl_val = gamma_img.at<pxltype>(i, j);
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
            compute_pxl(pxl_val, out_val, params, color_idx);

            out_img.at<pxltype>(i, j) = out_val;
        }
    }

    ////////////Top function call //////////////////

    // Call the top function
    degamma_accel((ap_uint<INPUT_PTR_WIDTH>*)gamma_img.data, (ap_uint<OUTPUT_PTR_WIDTH>*)out_hls.data, degamma_config,
                  bformat, height, width);

    imwrite("out_img.jpg", out_img);
    imwrite("out_hls.jpg", out_hls);

    // Compute absolute difference image
    cv::absdiff(out_img, out_hls, diff);

    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    float err_per;
    xf::cv::analyzeDiff(diff, 4, err_per);

    if (err_per > 0.0f) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return 1;
    }
    std::cout << "Test Passed " << std::endl;
    return 0;
}
