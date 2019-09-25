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
#include "xf_accumulate_squared_config.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> <input image path2> \n");
        return -1;
    }

    cv::Mat in_img, in_img1, out_img;
    cv::Mat in_gray, in_gray1, diff;
#if GRAY
    in_gray = cv::imread(argv[1], 0);  // read image
    in_gray1 = cv::imread(argv[2], 0); // read image
#else
    in_gray = cv::imread(argv[1], 1);  // read image
    in_gray1 = cv::imread(argv[2], 1); // read image

#endif
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return 0;
    }
    if (in_gray1.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[2]);
        return 0;
    }
#if GRAY
    cv::Mat inout_gray(in_gray.rows, in_gray.cols, CV_16U, 1);
    cv::Mat out_gray(in_gray.rows, in_gray.cols, CV_16U, 1);
    cv::Mat inout_gray1(in_gray.rows, in_gray.cols, CV_32FC1, 1);

    cv::Mat ocv_ref(in_gray.rows, in_gray.cols, CV_16U, 1);
    cv::Mat ocv_ref_in1(in_gray.rows, in_gray.cols, CV_32FC1, 1);
    cv::Mat ocv_ref_in2(in_gray.rows, in_gray.cols, CV_32FC1, 1);

    in_gray.convertTo(ocv_ref_in1, CV_32FC1);
    in_gray1.convertTo(ocv_ref_in2, CV_32FC1);
#else
    cv::Mat inout_gray(in_gray.rows, in_gray.cols, CV_16UC3);
    cv::Mat out_gray(in_gray.rows, in_gray.cols, CV_16UC3);
    cv::Mat inout_gray1(in_gray.rows, in_gray.cols, CV_32FC3);

    cv::Mat ocv_ref(in_gray.rows, in_gray.cols, CV_16UC3);
    cv::Mat ocv_ref_in1(in_gray.rows, in_gray.cols, CV_32FC3);
    cv::Mat ocv_ref_in2(in_gray.rows, in_gray.cols, CV_32FC3);

    in_gray.convertTo(ocv_ref_in1, CV_32FC3);
    in_gray1.convertTo(ocv_ref_in2, CV_32FC3);
#endif
    // OpenCV function

    cv::accumulateSquare(ocv_ref_in1, ocv_ref_in2, cv::noArray());

#if GRAY
    ocv_ref_in2.convertTo(ocv_ref, CV_16UC1);
    in_gray1.convertTo(inout_gray, CV_8UC1);
#else
    ocv_ref_in2.convertTo(ocv_ref, CV_16UC3);
    in_gray1.convertTo(inout_gray, CV_8UC3);
#endif
    // write OpenCV reference output
    imwrite("out_ocv.jpg", ocv_ref);

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput2(inout_gray.rows, inout_gray.cols);

    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows, in_gray1.cols);

    imgInput1.copyTo(in_gray.data);
    imgInput2.copyTo(inout_gray.data);

    accumulate_squared(imgInput1, imgInput2, imgOutput);

    out_gray.data = imgOutput.copyFrom();

    // Write the output
    imwrite("out_hls.jpg", out_gray);
#if GRAY
    out_gray.convertTo(inout_gray1, CV_32FC1);
#else
    out_gray.convertTo(inout_gray1, CV_32FC3);
#endif
    // Compute absolute difference image
    absdiff(ocv_ref_in2, inout_gray1, diff);
    // Save the difference image
    imwrite("diff.jpg", diff);

    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_gray.rows; i++) {
        for (int j = 0; j < in_gray.cols; j++) {
            unsigned short v = diff.at<unsigned short>(i, j);
            if (v > 0.0f) cnt++;
            if (minval > v) minval = v;
            if (maxval < v) maxval = v;
        }
    }
    float err_per = 100.0 * (float)cnt / (in_gray.rows * in_gray.cols);

    fprintf(stderr,
            "Minimum error in intensity = %f\n"
            "Maximum error in intensity = %f\n"
            "Percentage of pixels above error threshold = %f\n",
            minval, maxval, err_per);
    if (err_per > 0.0f) return (int)-1;

    return 0;
}
