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
#include "xf_channel_combine_config.h"

int main(int argc, char** argv) {
#if FOUR_INPUT
    if (argc != 5) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr,
                "<Executable Name> <input image1 path> <input image2 path> <input image3 path> <input image4 path>\n");
        return -1;
    }
#endif
#if THREE_INPUT
    if (argc != 4) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image1 path> <input image2 path> <input image3 path> \n");
        return -1;
    }
#endif
#if TWO_INPUT
    if (argc != 3) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image1 path> <input image2 path> \n");
        return -1;
    }
#endif
    cv::Mat in_gray1, in_gray2;
    cv::Mat in_gray3, in_gray4;
    cv::Mat out_img, ref;
    cv::Mat diff;

    // Read images
    in_gray1 = cv::imread(argv[1], 0);
    in_gray2 = cv::imread(argv[2], 0);

    if ((in_gray1.data == NULL) | (in_gray2.data == NULL)) {
        fprintf(stderr, "Cannot open input images \n");
        return 0;
    }

#if !TWO_INPUT
    in_gray3 = cv::imread(argv[3], 0);
    if ((in_gray3.data == NULL)) {
        fprintf(stderr, "Cannot open input images \n");
        return 0;
    }

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput3(in_gray3.rows, in_gray3.cols);

    // creating memory for diff image
    diff.create(in_gray1.rows, in_gray1.cols, CV_TYPE);
#endif

#if FOUR_INPUT

    in_gray4 = cv::imread(argv[4], 0);

    if ((in_gray4.data == NULL)) {
        fprintf(stderr, "Cannot open image 4\n");
        return 0;
    }
    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput4(in_gray4.rows, in_gray4.cols);

#endif

    // image height and width
    uint16_t height = in_gray1.rows;
    uint16_t width = in_gray1.cols;

    // OpenCV Reference
    std::vector<cv::Mat> bgr_planes;
    cv::Mat merged;

#if (!TWO_INPUT)
    bgr_planes.push_back(in_gray3);
#endif
    bgr_planes.push_back(in_gray2);
    bgr_planes.push_back(in_gray1);

#if FOUR_INPUT
    bgr_planes.push_back(in_gray4);
#endif

    cv::merge(bgr_planes, merged);

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput1(in_gray1.rows, in_gray1.cols);
    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC> imgInput2(in_gray2.rows, in_gray2.cols);
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC> imgOutput(in_gray1.rows, in_gray1.cols);

    imgInput1.copyTo(in_gray1.data);
    imgInput2.copyTo(in_gray2.data);

#if (!TWO_INPUT)
    imgInput3.copyTo(in_gray3.data);
#endif
#if FOUR_INPUT
    imgInput4.copyTo(in_gray4.data);
#endif

#if FOUR_INPUT
    channel_combine_accel(imgInput1, imgInput2, imgInput3, imgInput4, imgOutput);
#elif THREE_INPUT
    channel_combine_accel(imgInput1, imgInput2, imgInput3, imgOutput);
#else
    channel_combine_accel(imgInput1, imgInput2, imgOutput);
#endif

// Write output image
#if !TWO_INPUT
    xf::cv::imwrite("hls_out.jpg", imgOutput);
    cv::imwrite("out_ocv.jpg", merged);

    // compute the absolute difference
    xf::cv::absDiff(merged, imgOutput, diff);

    // write the difference image
    cv::imwrite("diff.jpg", diff);

    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    float err_per = 0;
    xf::cv::analyzeDiff(diff, 0, err_per);

    if (err_per > 0) return 1;
#endif

    return 0;
}
