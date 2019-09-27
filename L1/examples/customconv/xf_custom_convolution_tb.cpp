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
#include "xf_custom_convolution_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, diff, filter;

#if GRAY
    in_img = cv::imread(argv[1], 0); // reading in the gray image
#else
    in_img = cv::imread(argv[1], 1); // reading in the gray image
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    unsigned char shift = SHIFT;

    //////////////////  Creating the kernel ////////////////
    filter.create(FILTER_HEIGHT, FILTER_WIDTH, CV_32F);

    /////////	Filling the Filter coefficients  /////////
    for (int i = 0; i < FILTER_HEIGHT; i++) {
        for (int j = 0; j < FILTER_WIDTH; j++) {
            filter.at<float>(i, j) = (float)0.1111;
        }
    }

/////////////////    OpenCV reference   /////////////////
#if GRAY
#if OUT_8U
    out_img.create(in_img.rows, in_img.cols, CV_8UC1); // create memory for output image
    diff.create(in_img.rows, in_img.cols, CV_8UC1);    // create memory for difference image
#elif OUT_16S
    out_img.create(in_img.rows, in_img.cols, CV_16SC1); // create memory for output image
    diff.create(in_img.rows, in_img.cols, CV_16SC1);    // create memory for difference image
#endif
#else
#if OUT_8U
    out_img.create(in_img.rows, in_img.cols, CV_8UC3); // create memory for output image
    diff.create(in_img.rows, in_img.cols, CV_8UC3);    // create memory for difference image
#elif OUT_16S
    out_img.create(in_img.rows, in_img.cols, CV_16SC3); // create memory for output image
    diff.create(in_img.rows, in_img.cols, CV_16SC3);    // create memory for difference image
#endif
#endif

    cv::Point anchor = cv::Point(-1, -1);

#if OUT_8U
    cv::filter2D(in_img, ocv_ref, CV_8U, filter, anchor, 0, cv::BORDER_CONSTANT);
#elif OUT_16S
    cv::filter2D(in_img, ocv_ref, CV_16S, filter, anchor, 0, cv::BORDER_CONSTANT);
#endif

    imwrite("ref_img.jpg", ocv_ref); // reference image

    short int* filter_ptr = (short int*)malloc(FILTER_WIDTH * FILTER_HEIGHT * sizeof(short int));

    for (int i = 0; i < FILTER_HEIGHT; i++) {
        for (int j = 0; j < FILTER_WIDTH; j++) {
            filter_ptr[i * FILTER_WIDTH + j] = 3640;
        }
    }

    static xf::cv::Mat<INTYPE, HEIGHT, WIDTH, NPC_T> imgInput(in_img.rows, in_img.cols);
    static xf::cv::Mat<OUTTYPE, HEIGHT, WIDTH, NPC_T> imgOutput(in_img.rows, in_img.cols);

    imgInput.copyTo(in_img.data);

    Filter2d_accel(imgInput, imgOutput, filter_ptr, shift);

    xf::cv::imwrite("out_img.jpg", imgOutput);
    xf::cv::absDiff(ocv_ref, imgOutput, diff); // Compute absolute difference image

    float err_per;
    xf::cv::analyzeDiff(diff, 2, err_per);
    cv::imwrite("diff_img.jpg", diff);

    ocv_ref.~Mat();
    diff.~Mat();
    in_img.~Mat();
    out_img.~Mat();

    if (err_per > 0.0f) return 1;

    return 0;
}
