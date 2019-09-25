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
#include "xf_pyr_up_config.h"
int main(int argc, char* argv[]) {
    cv::Mat input_image, output_image, output_xf, output_diff_xf_cv;
#if GRAY
    input_image = cv::imread(argv[1], 0);
#else
    input_image = cv::imread(argv[1], 1);
#endif

    int channels = input_image.channels();
    unsigned short input_height = input_image.rows;
    unsigned short input_width = input_image.cols;

    unsigned short output_height = input_image.rows << 1;
    unsigned short output_width = input_image.cols << 1;
    std::cout << "Input Height " << input_height << " Input_Width " << input_width << std::endl;
    std::cout << "Output Height " << output_height << " Output_Width " << output_width << std::endl;

    cv::pyrUp(input_image, output_image, cv::Size(output_width, output_height));

    cv::imwrite("opencv_image.png", output_image);
#if GRAY
    output_xf.create(output_height, output_width, CV_8UC1);
    output_diff_xf_cv.create(output_height, output_width, CV_8UC1);
#else
    output_xf.create(output_height, output_width, CV_8UC3);
    output_diff_xf_cv.create(output_height, output_width, CV_8UC3);
#endif

    static xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgInput(input_image.rows, input_image.cols);
    static xf::cv::Mat<TYPE, 2 * HEIGHT, 2 * WIDTH, XF_NPPC1> imgOutput(output_height, output_width);

    // imgInput = xf::cv::imread<TYPE, HEIGHT, WIDTH, XF_NPPC1>(argv[1], 0);
    imgInput.copyTo(input_image.data);

    pyr_up_accel(imgInput, imgOutput);

    int num_errors_xf = 0;
    unsigned char max_error = 0;
    unsigned char min_error = 255;
    FILE* fp = fopen("hls.txt", "w");
    FILE* fp1 = fopen("cv.txt", "w");

    output_xf.data = (unsigned char*)imgOutput.copyFrom();
    for (int i = (2 * channels); i < output_height - 2; i++) {
        for (int j = (2 * channels); j < output_width - 2; j++) {
            if (output_xf.at<unsigned char>(i, j) == output_image.at<unsigned char>(i, j)) {
                output_diff_xf_cv.at<unsigned char>(i, j) = 0;
                fprintf(fp, "%d \n", output_xf.at<unsigned char>(i, j));
                fprintf(fp1, "%d \n", output_image.at<unsigned char>(i, j));

            } else {
                output_diff_xf_cv.at<unsigned char>(i, j) = 255;

                unsigned char temp1 = output_xf.at<unsigned char>(i, j);
                unsigned char temp2 = output_image.at<unsigned char>(i, j);
                unsigned char temp = std::abs(temp1 - temp2);
                fprintf(fp, "%d \n", temp1);
                fprintf(fp1, "%d \n", temp2);
                if (temp > max_error) {
                    max_error = temp;

                } else if (temp < min_error) {
                    min_error = temp;
                }
                if (temp > 0) {
                    num_errors_xf++;
                }
            }
        }
    }
    fclose(fp);
    fclose(fp1);
    std::cout << "number of differences between opencv and xf: " << num_errors_xf << std::endl;
    std::cout << "Max Error between opencv and xf: " << (unsigned int)max_error << std::endl;
    std::cout << "Min Error between opencv and xf: " << (unsigned int)min_error << std::endl;
    cv::imwrite("xf_cv_diff_image.png", output_diff_xf_cv);
    cv::imwrite("xf_image.png", output_xf);

    if (max_error > 0) {
        return 1;
    } else {
        return 0;
    }

    return 0;
}
