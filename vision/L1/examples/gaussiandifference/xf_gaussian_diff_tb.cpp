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

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1>", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat out_img, ocv_ref, in_gray, in_gray1, in_gray2, diff;

    // Reading in the image:
    in_gray = cv::imread(argv[1], 0);

    if (!in_gray.data) {
        fprintf(stderr, "ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

    // Create memory for output image
    ocv_ref.create(in_gray.rows, in_gray.cols, in_gray.depth());
    out_img.create(in_gray.rows, in_gray.cols, in_gray.depth());

#if FILTER_WIDTH_1 == 3
    float sigma1 = 0.5f;
#endif
#if FILTER_WIDTH_1 == 7
    float sigma1 = 1.16666f;
#endif
#if FILTER_WIDTH_1 == 5
    float sigma1 = 0.8333f;
#endif

#if FILTER_WIDTH_2 == 3
    float sigma2 = 0.5f;
#endif
#if FILTER_WIDTH_2 == 7
    float sigma2 = 1.16f;
#endif
#if FILTER_WIDTH_2 == 5
    float sigma2 = 0.8333f;
#endif

    // OpenCV Reference

    cv::GaussianBlur(in_gray, in_gray1, cv::Size(FILTER_WIDTH_1, FILTER_WIDTH_1), sigma1, sigma1, cv::BORDER_CONSTANT);
    cv::GaussianBlur(in_gray1, in_gray2, cv::Size(FILTER_WIDTH_2, FILTER_WIDTH_2), sigma2, sigma2, cv::BORDER_CONSTANT);
    cv::subtract(in_gray1, in_gray2, ocv_ref);

    // OpenCL section:
    size_t image_in_size_bytes = in_gray.rows * in_gray.cols * sizeof(unsigned char);
    size_t image_out_size_bytes = image_in_size_bytes;

    int rows = in_gray.rows;
    int cols = in_gray.cols;

    //////////Top function call //////////////////////////////

    gaussian_diff_accel((ap_uint<PTR_WIDTH>*)in_gray.data, sigma1, sigma2, (ap_uint<PTR_WIDTH>*)out_img.data, rows,
                        cols);

    // Write the output of kernel:
    cv::imwrite("output_hls.png", out_img);
    cv::imwrite("ocv_ref.png", ocv_ref);

    cv::absdiff(ocv_ref, out_img, diff);
    cv::imwrite("error.png", diff); // Save the difference image for debugging purpose

    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    if (err_per > 1) {
        fprintf(stderr, "ERROR: Test Failed.\n ");
        return -1;
    } else
        std::cout << "Test Passed " << std::endl;

    return 0;
}
