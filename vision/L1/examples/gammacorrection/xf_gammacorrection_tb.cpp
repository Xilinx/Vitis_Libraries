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

    cv::imwrite("in_hls.jpg", in_gray);

    int height = in_gray.rows;
    int width = in_gray.cols;

    ///////////////////////Top function call //////////////////////////////
    gammacorrection_accel((ap_uint<INPUT_PTR_WIDTH>*)in_gray.data, (ap_uint<OUTPUT_PTR_WIDTH>*)out_gray.data, gamma_,
                          height, width);

    cv::imwrite("out_hls.jpg", out_gray);

    return 0;
}
