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
#include "xf_accumulate_config.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> <input image path2> \n");
        return -1;
    }

    char in[100], in1[100], out_hls[100], out_ocv[100], out_error[100];

    cv::Mat in_img, in_img1, out_img;
    cv::Mat in_gray, in_gray1;
#if GRAY
    in_gray = cv::imread(argv[1], 0);  // reading image
    in_gray1 = cv::imread(argv[2], 0); // reading image
#else
    in_gray = cv::imread(argv[1], 1);  // reading image
    in_gray1 = cv::imread(argv[2], 1); // reading image

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
    cv::Mat out_gray(in_gray.rows, in_gray.cols, CV_16UC1, 1);
    cv::Mat diff(in_gray.rows, in_gray.cols, CV_16UC1, 1);
    cv::Mat ocv_ref(in_gray.rows, in_gray.cols, CV_16UC1, 1);
    cv::Mat ocv_ref_in1(in_gray.rows, in_gray.cols, CV_32FC1, 1);
    cv::Mat ocv_ref_in2(in_gray.rows, in_gray.cols, CV_32FC1, 1);
    in_gray.convertTo(ocv_ref_in1, CV_32FC1);
    in_gray1.convertTo(ocv_ref_in2, CV_32FC1);
#else
    cv::Mat out_gray(in_gray.rows, in_gray.cols, CV_16UC3, 1);
    cv::Mat diff(in_gray.rows, in_gray.cols, CV_16UC3, 1);
    cv::Mat ocv_ref(in_gray.rows, in_gray.cols, CV_16UC3, 1);
    cv::Mat ocv_ref_in1(in_gray.rows, in_gray.cols, CV_32FC3, 1);
    cv::Mat ocv_ref_in2(in_gray.rows, in_gray.cols, CV_32FC3, 1);
    in_gray.convertTo(ocv_ref_in1, CV_32FC3);
    in_gray1.convertTo(ocv_ref_in2, CV_32FC3);
#endif
    // OpenCV functions
    cv::accumulate(ocv_ref_in1, ocv_ref_in2, cv::noArray());

#if GRAY
    ocv_ref_in2.convertTo(ocv_ref, CV_16UC1);
#else
    ocv_ref_in2.convertTo(ocv_ref, CV_16UC3);
#endif
    // Write OpenCV reference image
    cv::imwrite("out_ocv.jpg", ocv_ref);

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows, in_gray1.cols);
    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput2(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows, in_gray1.cols);

    imgInput1.copyTo(in_gray.data);
    imgInput2.copyTo(in_gray1.data);

    accumulate_accel(imgInput1, imgInput2, imgOutput);

    // Write output image
    xf::cv::imwrite("out_hls.jpg", imgOutput);

    // Compute absolute difference image
    xf::cv::absDiff(ocv_ref, imgOutput, diff);
    // Save the difference image
    imwrite("diff.jpg", diff);

    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_gray.rows; i++) {
        for (int j = 0; j < in_gray.cols; j++) {
            unsigned short v = diff.at<unsigned short>(i, j);
            if (v > 0) cnt++;
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
