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

void gainControlOCV(
    cv::Mat input, cv::Mat& output, int code, unsigned short rgain, unsigned short bgain, unsigned short ggain) {
    cv::Mat mat = input.clone();
    int height = mat.size().height;
    int width = mat.size().width;
#if T_8U
    typedef uint8_t realSize;
#else
    typedef uint16_t realSize;
#endif
    typedef unsigned int maxSize;
    maxSize pixel;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // std::cout<<"("<<i<<","<<j<<")\t";
            pixel = (maxSize)mat.at<realSize>(i, j); // extracting each pixel
            // std::cout<<"Initial: "<<pixel<<"\t";
            bool cond1, cond2;
            cond1 = (j % 2 == 0);
            cond2 = (j % 2 != 0);
            if (code == XF_BAYER_RG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else if (i % 2 != 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);
            } else if (code == XF_BAYER_GR) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);
            } else if (code == XF_BAYER_BG) {
                if (i % 2 == 0 && cond1)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);
            } else if (code == XF_BAYER_GB) {
                if (i % 2 == 0 && cond2)
                    pixel = (maxSize)((pixel * bgain) >> 7);
                else if (i % 2 != 0 && cond1)
                    pixel = (maxSize)((pixel * rgain) >> 7);
                else
                    pixel = (maxSize)((pixel * ggain) >> 7);
            }
            // std::cout<<"Final: "<<pixel<<std::endl;
            mat.at<realSize>(i, j) = cv::saturate_cast<realSize>(pixel); // writing each pixel
        }
    }
    output = mat;
}
