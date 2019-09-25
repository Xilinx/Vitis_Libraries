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
#include "xf_dilation_config.h"

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

// create memory for output images
#if GRAY
    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC1);
    out_img.create(in_gray.rows, in_gray.cols, CV_8UC1);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC1);
#else
    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC3);
    out_img.create(in_gray.rows, in_gray.cols, CV_8UC3);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC3);

#endif

    cv::Mat element = cv::getStructuringElement(KERNEL_SHAPE, cv::Size(FILTER_SIZE, FILTER_SIZE), cv::Point(-1, -1));
    cv::dilate(in_gray, ocv_ref, element, cv::Point(-1, -1), ITERATIONS, cv::BORDER_CONSTANT);
    cv::imwrite("out_ocv.jpg", ocv_ref);
    /////////////////////	End of OpenCV reference	 ////////////////

    ////////////////////	HLS TOP function call	/////////////////

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows, in_gray.cols);

    unsigned char structure_element[FILTER_SIZE * FILTER_SIZE];

    for (int i = 0; i < (FILTER_SIZE * FILTER_SIZE); i++) {
        structure_element[i] = element.data[i];
    }

    //	unsigned char iterations=2;

    imgInput.copyTo(in_gray.data);

    dilation_accel(imgInput, imgOutput, structure_element);

    //////////////////  Compute Absolute Difference ////////////////////

    xf::cv::absDiff(ocv_ref, imgOutput, diff);

    cv::imwrite("out_error.jpg", diff);

    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_gray.rows; i++) {
        for (int j = 0; j < in_gray.cols; j++) {
            uchar v = diff.at<uchar>(i, j);
            if (v > 0) cnt++;
            if (minval > v) minval = v;
            if (maxval < v) maxval = v;
        }
    }
    float err_per = 100.0 * (float)cnt / (in_gray.rows * in_gray.cols);
    fprintf(stderr,
            "Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error "
            "threshold = %f\n",
            minval, maxval, err_per);

    if (err_per > 0.0f) return 1;

    return 0;
}
