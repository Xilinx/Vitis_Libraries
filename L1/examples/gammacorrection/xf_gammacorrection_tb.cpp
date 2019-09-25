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
#include "xf_gammacorrection_config.h"

//#include <CL/cl.h>
//#include "xcl2.hpp"

float mean_pixel(cv::Mat img) {
    if (img.channels() > 2) {
        cvtColor(img.clone(), img, CV_BGR2GRAY);
        return cv::mean(img)[0];
    } else {
        return cv::mean(img)[0];
    }
}
float auto_gamma_value(cv::Mat img) {
    float max_pixel = 255;
    float middle_pixel = 128;
    float pixel_range = 256;
    float mean_l = mean_pixel(img);

    float gamma = log(middle_pixel / pixel_range) / log(mean_l / pixel_range); // Formula from ImageJ

    return gamma;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> \n");
        return -1;
    }

    cv::Mat in_gray, in_gray1, ocv_ref, out_gray, diff, ocv_ref_in1, ocv_ref_in2, inout_gray1;
    in_gray = cv::imread(argv[1], 1); // read image
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }

    ocv_ref.create(in_gray.rows, in_gray.cols, CV_8UC3);
    out_gray.create(in_gray.rows, in_gray.cols, CV_8UC3);
    diff.create(in_gray.rows, in_gray.cols, CV_8UC3);

    float gamma_ = auto_gamma_value(in_gray);

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray.rows, in_gray.cols);
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC1> imgOutput(out_gray.rows, out_gray.cols);

    imgInput1.copyTo(in_gray.data);

    xf::cv::imwrite("in_hls.jpg", imgInput1);

    gammacorrection_accel(imgInput1, imgOutput, gamma_);

    xf::cv::imwrite("out_hls.jpg", imgOutput);

    return 0;
}
