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
#include "xf_hist_equalize_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, diff;

    // reading in the color image

    in_img = cv::imread(argv[1], 0);

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image\n");
        return 0;
    }

    // cvtColor(in_img, in_img, CV_BGR2GRAY);

    // create memory for output images
    out_img.create(in_img.rows, in_img.cols, in_img.depth());
    ocv_ref.create(in_img.rows, in_img.cols, in_img.depth());
    diff.create(in_img.rows, in_img.cols, in_img.depth());

    ///////////////// 	Opencv  Reference  ////////////////////////
    cv::equalizeHist(in_img, ocv_ref);

    imwrite("out_ocv.jpg", ocv_ref);

    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows, in_img.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgInput1(in_img.rows, in_img.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows, in_img.cols);

    imgInput.copyTo(in_img.data);
    imgInput1.copyTo(in_img.data);

    equalizeHist_accel(imgInput, imgInput1, imgOutput);

    xf::cv::imwrite("out_hls.jpg", imgOutput);

    //////////////////  Compute Absolute Difference ////////////////////
    xf::cv::absDiff(ocv_ref, imgOutput, diff);
    imwrite("out_error.jpg", diff);

    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    if (err_per > 0.0f) {
        printf("\nTest Failed\n");
        return -1;
    }
    return 0;
}
