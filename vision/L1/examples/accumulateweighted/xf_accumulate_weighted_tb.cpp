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
#include "xf_accumulate_weighted_config.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> <input image path2> \n");
        return -1;
    }

    cv::Mat in_img, in_img1, out_img;
    cv::Mat in_gray, in_gray1;
#if GRAY
    in_gray = cv::imread(argv[1], 0);  // read image
    in_gray1 = cv::imread(argv[2], 0); // read image
#else
    in_gray = cv::imread(argv[1], 1);  // read image
    in_gray1 = cv::imread(argv[2], 1); // read image
#endif
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }
    if (in_gray1.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[2]);
        return -1;
    }

#if GRAY

    cv::Mat ocv_in(in_gray.rows, in_gray.cols, CV_32FC1, 1);
    cv::Mat ocv_inout(in_gray.rows, in_gray.cols, CV_32FC1, 1);
    cv::Mat ocv_out_16bit(in_gray.rows, in_gray.cols, CV_16UC1, 1);
    cv::Mat diff(in_gray.rows, in_gray.cols, CV_16UC1, 1);

    in_gray.convertTo(ocv_in, CV_32FC1);
    in_gray1.convertTo(ocv_inout, CV_32FC1);
#else

    cv::Mat ocv_in(in_gray.rows, in_gray.cols, CV_32FC3);
    cv::Mat ocv_inout(in_gray.rows, in_gray.cols, CV_32FC3);
    cv::Mat ocv_out_16bit(in_gray.rows, in_gray.cols, CV_16UC3);
    cv::Mat diff(in_gray.rows, in_gray.cols, CV_16UC3, 1);
    in_gray.convertTo(ocv_in, CV_32FC3);
    in_gray1.convertTo(ocv_inout, CV_32FC3);
#endif
    // Weight ( 0 to 1 )
    float alpha = 0.76;

    // OpenCV function
    cv::accumulateWeighted(ocv_in, ocv_inout, alpha, cv::noArray());
#if GRAY
    ocv_inout.convertTo(ocv_out_16bit, CV_16UC1);
#else
    ocv_inout.convertTo(ocv_out_16bit, CV_16UC3);
#endif
    // Write OpenCV reference image
    cv::imwrite("out_ocv.jpg", ocv_out_16bit);

    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows, in_gray1.cols);
    xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput2(in_gray.rows, in_gray.cols);
    xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray.rows, in_gray.cols);

    imgInput1.copyTo(in_gray.data);
    imgInput2.copyTo(in_gray1.data);

    accumulate_weighted_accel(imgInput1, imgInput2, imgOutput, alpha);

    xf::cv::imwrite("out_hls.jpg", imgOutput);

    xf::cv::absDiff(ocv_out_16bit, imgOutput, diff);
    // Save the difference image
    cv::imwrite("diff.jpg", diff);
    int err_thresh;
    float err_per;
    xf::cv::analyzeDiff(diff, err_thresh, err_per);

    if (err_per > 0.0f) return -1;
    return 0;
}
