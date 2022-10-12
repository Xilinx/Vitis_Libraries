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

#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <iostream>
#include <math.h>

#include "xf_hdrdecompand_config.h"

void bayerizeImage(cv::Mat img, cv::Mat& bayer_image, cv::Mat& cfa_output, int code) {
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            cv::Vec3w in = img.at<cv::Vec3w>(i, j);
            cv::Vec3w b;
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<ushort>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<ushort>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<ushort>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<ushort>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<ushort>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<ushort>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<ushort>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<ushort>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<ushort>(i, j) = in[0];
                    }
                }
            }
            bayer_image.at<cv::Vec3w>(i, j) = b;
        }
    }
}

void compute_pxl(int pxl_val, int& out_val, int params[3][4][3], int color_idx) {
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
int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <INPUT IMAGE PATH > \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img, out_img, out_hls, diff;

    // Reading in the images:
    in_img = cv::imread(argv[1], -1);

    if (in_img.data == NULL) {
        printf("ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

    diff.create(in_img.rows, in_img.cols, CV_32SC1);

    out_img.create(in_img.rows, in_img.cols, CV_32SC1);
    out_hls.create(in_img.rows, in_img.cols, CV_32SC1);

    int height = in_img.rows;
    int width = in_img.cols;

    int out_val;

    int pxl_val;

    // Create the Bayer pattern CFA output
    cv::Mat cfa_bayer_output(in_img.rows, in_img.cols, CV_16UC1); // simulate the Bayer pattern CFA outputi

    cv::Mat color_cfa_bayer_output(in_img.rows, in_img.cols, in_img.type()); // Bayer pattern CFA output in color
    int bformat = BPATTERN;                                                  // Bayer format BG-0; GB-1; GR-2; RG-3

    bayerizeImage(in_img, color_cfa_bayer_output, cfa_bayer_output, bformat);
    cv::imwrite("bayer_image.png", color_cfa_bayer_output);
    cv::imwrite("cfa_output.png", cfa_bayer_output);

#if T_12U
    int params[3][4][3] = {{{512, 4, 0}, {1408, 16, 384}, {2176, 64, 1152}, {4096, 512, 2048}},
                           {{512, 4, 0}, {1408, 16, 384}, {2176, 64, 1152}, {4096, 512, 2048}},
                           {{512, 4, 0}, {1408, 16, 384}, {2176, 64, 1152}, {4096, 512, 2048}}}; // 12 bit to 20 bit
#else
    int params[3][4][3] = {
        {{8192, 4, 0}, {22528, 16, 6144}, {34816, 64, 18432}, {65536, 512, 32768}},
        {{8192, 4, 0}, {22528, 16, 6144}, {34816, 64, 18432}, {65536, 512, 32768}},
        {{8192, 4, 0}, {22528, 16, 6144}, {34816, 64, 18432}, {65536, 512, 32768}}}; // 16 bit to 24 bit
#endif

    int color_idx, row_idx, col_idx;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            pxl_val = cfa_bayer_output.at<ushort>(i, j);
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
            compute_pxl(pxl_val, out_val, params, color_idx);
            out_img.at<int>(i, j) = out_val;
        }
    }

    ////////////Top function call //////////////////

    // Call the top function
    hdrdecompand_accel((ap_uint<INPUT_PTR_WIDTH>*)cfa_bayer_output.data, (ap_uint<OUTPUT_PTR_WIDTH>*)out_hls.data,
                       params, bformat, height, width);

    imwrite("out_img.jpg", out_img);
    imwrite("out_hls.jpg", out_hls);

    // Compute absolute difference image
    cv::absdiff(out_img, out_hls, diff);

    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    if (err_per > 0.0f) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return 1;
    }
    std::cout << "Test Passed " << std::endl;
    return 0;
}
