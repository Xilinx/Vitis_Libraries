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
#include <iostream>
#include <vector>
#include "opencv2/opencv.hpp"
#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <math.h>
#include "common/xf_types.hpp"
#include "common/xf_infra.hpp"

void LSC_ref(cv::Mat& _src, cv::Mat& _dst) {
    int center_pixel_pos_x = (_src.cols / 2);
    int center_pixel_pos_y = (_src.rows / 2);
    float max_distance = std::sqrt((_src.rows - center_pixel_pos_y) * (_src.rows - center_pixel_pos_y) +
                                   (_src.cols - center_pixel_pos_x) * (_src.cols - center_pixel_pos_x));

    for (int i = 0; i < _src.rows; i++) {
        for (int j = 0; j < _src.cols; j++) {
            float distance = std::sqrt((center_pixel_pos_y - i) * (center_pixel_pos_y - i) +
                                       (center_pixel_pos_x - j) * (center_pixel_pos_x - j)) /
                             max_distance;

            float gain = (0.01759 * ((distance + 28.37) * (distance + 28.37))) - 13.36;
#if T_8U
            int value = (_src.at<unsigned char>(i, j) * gain);
            if (value > 255) {
                value = 255;
            }
            _dst.at<unsigned char>(i, j) = (unsigned char)value;
#else
            int value = (_src.at<unsigned short>(i, j) * gain);
            if (value > 65535) {
                value = 65535;
            }
            _dst.at<unsigned short>(i, j) = (unsigned short)value;

#endif
        }
    }
}