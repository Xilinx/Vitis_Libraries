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

void BadPixelCorrection(cv::Mat input, cv::Mat& output) {
#if T_8U
    typedef unsigned char Pixel_t;
#else
    typedef unsigned short int Pixel_t;
#endif
    const Pixel_t MINVAL = 0;
    const Pixel_t MAXVAL = 0;
    cv::Mat mask =
        (cv::Mat_<unsigned char>(5, 5) << 1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1);
    output = input.clone(); // Not cloning saves memory
    cv::Mat min, max;
    cv::erode(input, min, mask, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, MAXVAL);  // Min Filter
    cv::dilate(input, max, mask, cv::Point(-1, -1), 1, cv::BORDER_CONSTANT, MINVAL); // Max Filter

    cv::subtract(min, input, min);                    // Difference of min and input
    cv::subtract(input, max, max);                    // Difference of input and max
    cv::threshold(min, min, 0, 0, cv::THRESH_TOZERO); // Remove all values less than zero (not required for this case
                                                      // but might be required for other data types which have signed
                                                      // values)
    cv::threshold(max, max, 0, 0, cv::THRESH_TOZERO); // Remove all values less than zero
    cv::subtract(output, max, output);
    cv::add(output, min, output);
}