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
#include "xf_box_filter_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, in_gray, in_conv_img, out_img, ocv_ref, diff;

    in_img = cv::imread(argv[1], 0); // reading in the color image

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

/*  convert to specific types  */
#if T_8U
    in_img.convertTo(in_conv_img, CV_8U); // Size conversion
#elif T_16U
    in_img.convertTo(in_conv_img, CV_16U); // Size conversion
#elif T_16S
    in_img.convertTo(in_conv_img, CV_16S); // Size conversion
#endif

    ocv_ref.create(in_img.rows, in_img.cols, in_conv_img.depth()); // create memory for output image
    diff.create(in_img.rows, in_img.cols, in_conv_img.depth());    // create memory for output image

/////////////////    OpenCV reference  /////////////////
#if FILTER_SIZE_3
    cv::boxFilter(in_conv_img, ocv_ref, -1, cv::Size(3, 3), cv::Point(-1, -1), true, cv::BORDER_CONSTANT);
#elif FILTER_SIZE_5
    cv::boxFilter(in_conv_img, ocv_ref, -1, cv::Size(5, 5), cv::Point(-1, -1), true, cv::BORDER_CONSTANT);
#elif FILTER_SIZE_7
    cv::boxFilter(in_conv_img, ocv_ref, -1, cv::Size(7, 7), cv::Point(-1, -1), true, cv::BORDER_CONSTANT);
#endif

    unsigned short height = in_img.rows;
    unsigned short width = in_img.cols;

    static xf::cv::Mat<IN_T, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows, in_img.cols);
    static xf::cv::Mat<IN_T, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows, in_img.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> in_8bit(in_img.rows, in_img.cols);

    in_8bit.copyTo(in_img.data);
#if T_8U
    imgInput = in_8bit;
#elif T_16U
    in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_16U);
#else
    in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_16S);
#endif

    boxfilter_accel(imgInput, imgOutput);

    xf::cv::imwrite("hls_out.jpg", imgOutput);
    imwrite("ref_img.jpg", ocv_ref);           // reference image
    xf::cv::absDiff(ocv_ref, imgOutput, diff); // Compute absolute difference image
    imwrite("diff_img.jpg", diff);             // Save the difference image for debugging purpose

    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
            uchar v = diff.at<uchar>(i, j);
            if (v > 1) cnt++;
            if (minval > v) minval = v;
            if (maxval < v) maxval = v;
        }
    }
    float err_per = 100.0 * (float)cnt / (in_img.rows * in_img.cols);
    fprintf(stderr,
            "Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error "
            "threshold = %f\n",
            minval, maxval, err_per);

    if (err_per > 0.0f) {
        return 1;
    }

    return 0;
}
