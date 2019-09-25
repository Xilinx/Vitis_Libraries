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

#include "common/xf_headers.hpp"
#include "xf_gaussian_diff_config.h"

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: <executable> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, in_img_gau;
    cv::Mat in_gray, in_gray1, diff;

    in_img = cv::imread(argv[1], 1); // reading in the color image
    if (!in_img.data) {
        printf("Failed to load the image ... !!!");
        return -1;
    }

    extractChannel(in_img, in_gray, 1);

    ocv_ref.create(in_gray.rows, in_gray.cols, in_gray.depth()); // create memory for output image
#if FILTER_WIDTH == 3
    float sigma = 0.5f;
#endif
#if FILTER_WIDTH == 7
    float sigma = 1.16666f;
#endif
#if FILTER_WIDTH == 5
    float sigma = 0.8333f;
#endif

    out_img.create(in_gray.rows, in_gray.cols, in_gray.depth()); // create memory for output image

    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows, in_gray.cols);

    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat1(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat2(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat3(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat4(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgmat5(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows, in_gray.cols);

    imgInput.copyTo(in_gray.data);

    gaussian_diff_accel(imgInput, imgmat1, imgmat2, imgmat3, imgmat4, imgmat5, imgOutput, sigma);

    FILE* fp = fopen("gaussdiff.txt", "w");
    out_img.data = imgOutput.copyFrom();

    for (int i = 0; i < out_img.rows; i++) {
        for (int j = 0; j < out_img.cols; j++) {
            uchar v = out_img.at<uchar>(i, j);
            fprintf(fp, "%d ", v);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);

    imwrite("output_hls.png", out_img);
}
