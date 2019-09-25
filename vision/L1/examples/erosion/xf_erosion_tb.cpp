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
#include "xf_erosion_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, in_img1, out_img, ocv_ref;
    cv::Mat in_gray, in_gray1, diff;
#if GRAY
    // reading in the color image
    in_gray = cv::imread(argv[1], 0);
#else
    // reading in the color image
    in_gray = cv::imread(argv[1], 1);
#endif

    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

#if GRAY
    // create memory for output images
    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC1);
    out_img.create(in_gray.rows, in_gray.cols, CV_8UC1);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC1);
#else
    // create memory for output images
    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC3);
    out_img.create(in_gray.rows, in_gray.cols, CV_8UC3);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC3);
#endif

    /////////////////////	End of OpenCV reference	 ////////////////
    cv::Mat element = cv::getStructuringElement(KERNEL_SHAPE, cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));
    cv::erode(in_gray, ocv_ref, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);

    cv::imwrite("out_ocv.jpg", ocv_ref);
    ////////////////////	HLS TOP function call	/////////////////

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows, in_gray.cols);

    unsigned char structure_element[FILTER_SIZE * FILTER_SIZE];

    for (int i = 0; i < (FILTER_SIZE * FILTER_SIZE); i++) {
        structure_element[i] = element.data[i];
    }

    imgInput.copyTo(in_gray.data);

    erosion_accel(imgInput, imgOutput, structure_element);

    // Write output image
    xf::cv::imwrite("hls_out.jpg", imgOutput);

    //////////////////  Compute Absolute Difference ////////////////////

    xf::cv::absDiff(ocv_ref, imgOutput, diff);
    // absdiff(ocv_ref, out_img, diff);
    cv::imwrite("out_error.jpg", diff);

    // Find minimum and maximum differences.

    float err_per;
    xf::cv::analyzeDiff(diff, 0, err_per);

    if (err_per > 0.0f) return 1;

    return 0;
}
