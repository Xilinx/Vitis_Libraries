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
#include <fstream>
#include <math.h>
#include "common/xf_types.hpp"
#include "common/xf_infra.hpp"

static unsigned char saturate_cast_ref(int Value, int offset) {
    // Right shifting Value 15 times to get the integer part
    unsigned char Value_uchar = 0;
    int value_int = Value + offset;
    if (value_int > 255)
        Value_uchar = 255;
    else if (value_int < 0)
        Value_uchar = 0;
    else
        Value_uchar = (unsigned char)value_int;

    return Value_uchar;
}

void bgr2yuyv_ref(cv::Mat& bgrImage, cv::Mat& yuyv_out_ref) {
    // Step 3: Convert the image to YUV color space and pack into YUYV format
    for (int y = 0; y < bgrImage.rows; y++) {
        for (int x = 0; x < bgrImage.cols; x += 2) {
            // Calculate Y values
            cv::Vec3b bgrPixel1 = bgrImage.at<cv::Vec3b>(y, x);
            double y1_double = ((0.257 * bgrPixel1[2]) + (0.504 * bgrPixel1[1]) + (0.098 * bgrPixel1[0]));
            int y1 = (y1_double - std::floor(y1_double) > 0.5) ? std::ceil(y1_double) : std::floor(y1_double);
            // int y1 = std::floor(y1_double);
            unsigned char y1_value = cv::saturate_cast<unsigned char>(y1 + 16);

            cv::Vec3b bgrPixel2 = bgrImage.at<cv::Vec3b>(y, x + 1);
            double y2_double = ((0.257 * bgrPixel2[2]) + (0.504 * bgrPixel2[1]) + (0.098 * bgrPixel2[0]));
            int y2 = (y2_double - std::floor(y2_double) > 0.5) ? std::ceil(y2_double) : std::floor(y2_double);
            // int y2 = std::floor(y2_double);
            unsigned char y2_value = cv::saturate_cast<unsigned char>(y2 + 16);

            // Calculate U and V values
            double u_double = ((-0.148 * bgrPixel1[2]) - (0.291 * bgrPixel1[1]) + (0.439 * bgrPixel1[0]));
            int u = (u_double - std::floor(u_double) >= 0.5) ? std::ceil(u_double) : std::floor(u_double);
            // int u =  std::ceil(u_double);
            unsigned char u_value = cv::saturate_cast<unsigned char>(u + 128);

            double v_double = ((0.4389 * bgrPixel1[2]) - (0.368 * bgrPixel1[1]) - (0.071 * bgrPixel1[0]));
            int v = (v_double - std::floor(v_double) >= 0.5) ? std::ceil(v_double) : std::floor(v_double);
            // int v =  std::ceil(v_double);
            unsigned char v_value = cv::saturate_cast<unsigned char>(v + 128);

            // Pack Y, U, Y, V values
            yuyv_out_ref.at<unsigned short>(y, x) = (unsigned short)((u_value << 8) | y1_value);
            yuyv_out_ref.at<unsigned short>(y, x + 1) = (unsigned short)((v_value << 8) | y2_value);
        }
    }

    // cv::imwrite("yuyv_out_ref.png",yuyv_out_ref);

    // FILE* fp9 = fopen("yuyvImage_ref.csv", "w");
    // for (int i = 0; i < bgrImage.rows; i++) {
    //     for (int j = 0; j < bgrImage.cols; j++) {
    //         fprintf(fp9, "%d ",yuyv_out_ref.at<ushort>(i,j));
    //     }
    //     fprintf(fp9, "\n");
    // }
    // fclose(fp9);
}
