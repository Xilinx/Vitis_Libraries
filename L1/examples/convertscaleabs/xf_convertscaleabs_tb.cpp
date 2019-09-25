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
#include "xf_convertscaleabs_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> \n");
        return -1;
    }

    cv::Mat in_gray, in_gray1, ocv_ref, out_gray, diff, ocv_ref_in1, ocv_ref_in2, inout_gray1;
    in_gray = cv::imread(argv[1], 0); // read image
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }

    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC1);
    out_gray.create(in_gray.rows, in_gray.cols, CV_8UC1);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC1);

    float scale = 1.5;
    float shift = 0;

    // OpenCV function

    cv::convertScaleAbs(in_gray, ocv_ref, scale, shift);

    // Write OpenCV reference image
    imwrite("out_ocv.jpg", ocv_ref);

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(out_gray.rows, out_gray.cols);

    imgInput1.copyTo(in_gray.data);

    convertScaleAbs_accel(imgInput1, imgOutput, scale, shift);

    out_gray.data = imgOutput.copyFrom();

    imwrite("out_hls.jpg", out_gray);

    // Compute absolute difference image
    absdiff(out_gray, ocv_ref, diff);

    // Save the difference image
    imwrite("diff.png", diff);

    // Find minimum and maximum differences
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_gray.rows; i++) {
        for (int j = 0; j < in_gray.cols; j++) {
            char v = diff.at<unsigned char>(i, j);
            if (v > 1) cnt++;
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
