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

void global_tone_mapping_ref(cv::Mat ccm_out_ref, cv::Mat& tm_out_ref) {
    int height = ccm_out_ref.rows;
    int width = ccm_out_ref.cols;

    cv::Mat xyzchannel[3], _xyzchannel[3];

    // xyzchannel[0].create(ccm_out_ref.rows, ccm_out_ref.cols, CV_16UC1);
    // xyzchannel[1].create(ccm_out_ref.rows, ccm_out_ref.cols, CV_16UC1);
    // xyzchannel[2].create(ccm_out_ref.rows, ccm_out_ref.cols, CV_16UC1);

    cv::Mat in_xyz;
    in_xyz.create(ccm_out_ref.rows, ccm_out_ref.cols, CV_OUT_TYPE);

    cv::cvtColor(ccm_out_ref, in_xyz, cv::COLOR_BGR2XYZ);
    cv::split(in_xyz, xyzchannel);

    _xyzchannel[0].create(ccm_out_ref.rows, ccm_out_ref.cols, CV_8UC1);
    _xyzchannel[1].create(ccm_out_ref.rows, ccm_out_ref.cols, CV_8UC1);
    _xyzchannel[2].create(ccm_out_ref.rows, ccm_out_ref.cols, CV_8UC1);

    float c1 = 3.0;
    float c2 = 1.5;

    double maxL = 0, minL = 100;
    double mean = 0;
    float pxl_val;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (xyzchannel[1].at<unsigned short>(i, j) == 0) {
                pxl_val = 0;
            } else
                pxl_val = log10(xyzchannel[1].at<unsigned short>(i, j));

            mean = mean + pxl_val;
            maxL = (maxL > pxl_val) ? maxL : pxl_val;
            minL = (minL < pxl_val) ? minL : pxl_val;
        }
    }
    std::cout << "meanbefore = " << mean << std::endl;
    std::cout << "log =" << log10(xyzchannel[1].at<unsigned short>(0, 0)) << std::endl;

    mean = (float)(mean / (float)(height * width));
    printf("\n");
    std::cout << mean << " " << maxL << " " << minL << std::endl;

    float maxLd, minLd;
    maxLd = 2.4;
    minLd = 0;
    std::cout << "maxL = " << maxL << std::endl;
    std::cout << "minL = " << minL << std::endl;

    float K1 = (maxLd - minLd) / (maxL - minL);

    float d0 = maxL - minL;
    float sigma_sq = (c1 * c1) / (2 * d0 * d0);
    float val, out_val;
    FILE* fp8 = fopen("exp_img.csv", "w");

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (xyzchannel[1].at<ushort>(i, j) == 0) {
                val = 0;
            } else
                val = log10(xyzchannel[1].at<ushort>(i, j));

            val = val - mean;

            float exp_val = exp(-(val * val * sigma_sq));
            float K2 = (1 - K1) * exp_val + K1;
            out_val = exp(c2 * K2 * val + mean);

            int x_val = xyzchannel[0].at<unsigned short>(i, j);
            int y_val = xyzchannel[1].at<unsigned short>(i, j);
            int z_val = xyzchannel[2].at<unsigned short>(i, j);

            fprintf(fp8, "(%d %d %f %f %f %f %f %f)", i, j, c1, c1, (d0 * d0), 1 / (2 * d0 * d0), (c1 * c1), sigma_sq);

            _xyzchannel[0].at<unsigned char>(i, j) = (unsigned char)((out_val / y_val) * x_val);
            _xyzchannel[2].at<unsigned char>(i, j) = (unsigned char)((out_val / y_val) * z_val);
            _xyzchannel[1].at<unsigned char>(i, j) = (unsigned char)out_val;
        }
        fprintf(fp8, "\n");
    }
    fclose(fp8);

    cv::Mat out_xyz;
    cv::merge(_xyzchannel, 3, out_xyz);
    cv::cvtColor(out_xyz, tm_out_ref, cv::COLOR_XYZ2BGR);
}
