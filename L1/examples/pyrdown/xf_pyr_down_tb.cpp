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
#include "xf_pyr_down_config.h"
int main(int argc, char* argv[]) {
    cv::Mat input_image, output_image, output_xf, output_diff_xf_cv;

#if GRAY
    input_image = cv::imread(argv[1], 0);
#else
    input_image = cv::imread(argv[1], 1);
#endif

    unsigned short input_height = input_image.rows;
    unsigned short input_width = input_image.cols;

    unsigned short output_height = (input_image.rows + 1) >> 1;
    unsigned short output_width = (input_image.cols + 1) >> 1;
#if GRAY
    output_xf.create(output_height, output_width, CV_8UC1);
    output_diff_xf_cv.create(output_height, output_width, CV_8UC1);

#else
    output_xf.create(output_height, output_width, CV_8UC3);
    output_diff_xf_cv.create(output_height, output_width, CV_8UC3);
#endif

    std::cout << "Input Height " << input_height << " Input_Width " << input_width << std::endl;
    std::cout << "Output Height " << output_height << " Output_Width " << output_width << std::endl;

    cv::pyrDown(input_image, output_image, cv::Size(output_width, output_height), cv::BORDER_REPLICATE);

    cv::imwrite("opencv_image.png", output_image);

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgInput(input_image.rows, input_image.cols);
    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgOutput(output_height, output_width);

    imgInput.copyTo(input_image.data);

    pyr_down_accel(imgInput, imgOutput);

    xf::cv::imwrite("hls_out.jpg", imgOutput);

    xf::cv::absDiff(output_image, imgOutput, output_diff_xf_cv);

    imwrite("error.png", output_diff_xf_cv); // Save the difference image for debugging purpose

    float err_per;
    xf::cv::analyzeDiff(output_diff_xf_cv, 0, err_per);

    if (err_per > 1) {
        printf("\nTest failed\n");
        return -1;
    } else {
        printf("\nTest Pass\n");
        return 0;
    }
}
