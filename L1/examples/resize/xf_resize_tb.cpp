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
#include "xf_resize_config.h"

int main(int argc, char** argv) {
    cv::Mat img, result_ocv, error;

    if (argc != 2) {
        printf("Usage : <executable> <input image> \n");
        return -1;
    }

#if GRAY
    img.create(cv::Size(WIDTH, HEIGHT), CV_8UC1);
    result_ocv.create(cv::Size(NEWWIDTH, NEWHEIGHT), CV_8UC1);
    error.create(cv::Size(NEWWIDTH, NEWHEIGHT), CV_8UC1);
#else
    img.create(cv::Size(WIDTH, HEIGHT), CV_8UC3);
    result_ocv.create(cv::Size(NEWWIDTH, NEWHEIGHT), CV_8UC3);
    error.create(cv::Size(NEWWIDTH, NEWHEIGHT), CV_8UC3);
#endif

#if GRAY
    // reading in the color image
    img = cv::imread(argv[1], 0);
#else
    img = cv::imread(argv[1], 1);
#endif

    if (!img.data) {
        return -1;
    }

    cv::imwrite("input.png", img);

    unsigned short in_width, in_height;
    unsigned short out_width, out_height;

    in_width = img.cols;
    in_height = img.rows;
    out_height = NEWHEIGHT;
    out_width = NEWWIDTH;

/*OpenCV resize function*/

#if INTERPOLATION == 0
    cv::resize(img, result_ocv, cv::Size(out_width, out_height), 0, 0, CV_INTER_NN);
#endif
#if INTERPOLATION == 1
    cv::resize(img, result_ocv, cv::Size(out_width, out_height), 0, 0, CV_INTER_LINEAR);
#endif
#if INTERPOLATION == 2
    cv::resize(img, result_ocv, cv::Size(out_width, out_height), 0, 0, CV_INTER_AREA);
#endif

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC_T> imgInput(in_height, in_width);
    static xf::cv::Mat<TYPE, NEWHEIGHT, NEWWIDTH, NPC_T> imgOutput(out_height, out_width);

    imgInput.copyTo(img.data);

    resize_accel(imgInput, imgOutput);

    float err_per;
    xf::cv::absDiff(result_ocv, imgOutput, error);
    xf::cv::analyzeDiff(error, 5, err_per);
    xf::cv::imwrite("hls_out.png", imgOutput);
    cv::imwrite("resize_ocv.png", result_ocv);
    cv::imwrite("error.png", error);

    if (err_per > 0.0f) {
        printf("\nTest Failed\n");
        return -1;
    }

    return 0;
}
