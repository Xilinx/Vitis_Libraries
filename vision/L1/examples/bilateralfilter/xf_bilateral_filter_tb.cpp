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
#include "xf_bilateral_filter_config.h"

int main(int argc, char** argv) {
    cv::Mat in_img, out_img, ocv_ref, in_img_gau;
    cv::Mat in_gray, in_gray1, diff;

    cv::RNG rng;

    if (argc != 2) {
        printf("Usage: <executable> <input image path> \n");
        return -1;
    }
#if GRAY
    in_img = cv::imread(argv[1], 0); // reading in the color image
#else
    in_img = cv::imread(argv[1], 1); // reading in the color image
#endif

    if (!in_img.data) {
        printf("Failed to load the image ... !!!");
        return -1;
    }

// cv::extractChannel(in_img, in_img,1);
// create memory for output image
#if GRAY
    ocv_ref.create(in_img.rows, in_img.cols, CV_8UC1);
    out_img.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for output image
    diff.create(in_img.rows, in_img.cols, CV_8UC1);
#else
    ocv_ref.create(in_img.rows, in_img.cols, CV_8UC3);
    out_img.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for output image
    diff.create(in_img.rows, in_img.cols, CV_8UC3);
#endif

    float sigma_color = rng.uniform(0.0, 1.0) * 255;
    float sigma_space = rng.uniform(0.0, 1.0);

    std::cout << " sigma_color: " << sigma_color << " sigma_space: " << sigma_space << std::endl;

    // OpenCV bilateral filter function
    cv::bilateralFilter(in_img, ocv_ref, FILTER_WIDTH, sigma_color, sigma_space, cv::BORDER_REPLICATE);

    cv::imwrite("output_ocv.png", ocv_ref);

    uint16_t width = in_img.cols;
    uint16_t height = in_img.rows;

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> _src(height, width);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> _dst(height, width);

    _src.copyTo(in_img.data);

    bilateral_filter_accel(_src, _dst, sigma_color, sigma_space);

    // Write output image
    // xf::cv::imwrite("hls_out.jpg",_dst);
    out_img.data = _dst.copyFrom();

    // Compute absolute difference image
    cv::absdiff(ocv_ref, out_img, diff);

    cv::imwrite("diff.jpg", diff);

    // 	Find minimum and maximum differences.

    double minval = 256, maxval = 0;
    int cnt = 0;
    float err = 0;

    xf::cv::analyzeDiff(diff, 0, err);

    return 0;
}
