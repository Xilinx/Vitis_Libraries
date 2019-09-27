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
#include "xf_median_blur_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, diff;

    unsigned short in_width, in_height;

/*  reading the gray/color image  */
#if GRAY
    in_img = cv::imread(argv[1], 0);
    ocv_ref.create(in_img.rows, in_img.cols, CV_8UC1);
    out_img.create(in_img.rows, in_img.cols, CV_8UC1);
    diff.create(in_img.rows, in_img.cols, CV_8UC1);
#else
    in_img = cv::imread(argv[1], 1);
    ocv_ref.create(in_img.rows, in_img.cols, CV_8UC3);
    out_img.create(in_img.rows, in_img.cols, CV_8UC3);
    diff.create(in_img.rows, in_img.cols, CV_8UC3);
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    /////////////////    OpenCV reference  /////////////////

    cv::medianBlur(in_img, ocv_ref, WINDOW_SIZE);

    in_width = in_img.cols;
    in_height = in_img.rows;

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPxPC> imgInput(in_height, in_width);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPxPC> imgOutput(in_height, in_width);

    imgInput.copyTo(in_img.data);

    median_blur_accel(imgInput, imgOutput);

    // Write output image
    xf::cv::imwrite("hls_out.jpg", imgOutput);
    cv::imwrite("ref_img.jpg", ocv_ref);
    xf::cv::absDiff(ocv_ref, imgOutput, diff);

    float err_per;
    xf::cv::analyzeDiff(diff, 0, err_per);
    cv::imwrite("diff_img.jpg", diff);

    in_img.~Mat();
    out_img.~Mat();
    ocv_ref.~Mat();
    in_img.~Mat();
    diff.~Mat();

    if (err_per > 0.0f) {
        return 1;
    }

    return 0;
}
