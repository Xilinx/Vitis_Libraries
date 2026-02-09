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

// linear interpolation
float interp1(float val1, float val2, float val, int dim) {
    float ret = val1 + val * (val2 - val1); /// dim;
    return ret;
}

// Trilinear interpolation
float interp3(float p000,
              float p100,
              float p001,
              float p101,
              float p010,
              float p110,
              float p011,
              float p111,
              float dist_r,
              float dist_g,
              float dist_b,
              int len) {
    float a = interp1(p000, p100, dist_r, len);
    float b = interp1(p001, p101, dist_r, len);
    float c = interp1(p010, p110, dist_r, len);
    float d = interp1(p011, p111, dist_r, len);

    float e = interp1(a, b, dist_g, len);
    float f = interp1(c, d, dist_g, len);

    float g = interp1(e, f, dist_b, len);

    return g;
}

// Reference function
void lut3d_ref(int height, int width, int lutdim, cv::Mat& in_img, float* lut, cv::Mat& out_img) {
    struct cInfo {
        float r;
        float g;
        float b;
    };

    cInfo lutGrid[lutdim][lutdim][lutdim];

    for (int k = 0; k < lutdim; k++) {
        for (int l = 0; l < lutdim; l++) {
            for (int m = 0; m < lutdim; m++) {
                lutGrid[k][l][m].r = lut[k * lutdim * lutdim * 3 + l * lutdim * 3 + m * 3 + 0];
                lutGrid[k][l][m].g = lut[k * lutdim * lutdim * 3 + l * lutdim * 3 + m * 3 + 1];
                lutGrid[k][l][m].b = lut[k * lutdim * lutdim * 3 + l * lutdim * 3 + m * 3 + 2];
            }
        }
    }

    cv::Vec3b outPix = 0;
    float outR = 0;
    float outG = 0;
    float outB = 0;

    int count_new = 0, count_new2 = 0;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            ap_uint<8> inPixR = in_img.at<cv::Vec3b>(i, j)[0];
            ap_uint<8> inPixG = in_img.at<cv::Vec3b>(i, j)[1];
            ap_uint<8> inPixB = in_img.at<cv::Vec3b>(i, j)[2];

            float scale_r = (int)inPixR / (float)255;
            float scale_g = (int)inPixG / (float)255;
            float scale_b = (int)inPixB / (float)255;

            float index_r = scale_r * (lutdim - 1);
            float index_g = scale_g * (lutdim - 1);
            float index_b = scale_b * (lutdim - 1);

            int index_r_int = index_r;
            int index_g_int = index_g;
            int index_b_int = index_b;

            float dist_r = index_r - index_r_int;
            float dist_g = index_g - index_g_int;
            float dist_b = index_b - index_b_int;

            if ((index_r_int == lutdim - 1) || (index_g_int == lutdim - 1) || (index_b_int == lutdim - 1)) {
                count_new++;

                outR = lutGrid[index_b_int][index_g_int][index_r_int].r;
                outG = lutGrid[index_b_int][index_g_int][index_r_int].g;
                outB = lutGrid[index_b_int][index_g_int][index_r_int].b;
            } else {
                count_new2++;

                float interR000 = lutGrid[index_b_int][index_g_int][index_r_int].r;
                float interG000 = lutGrid[index_b_int][index_g_int][index_r_int].g;
                float interB000 = lutGrid[index_b_int][index_g_int][index_r_int].b;

                float interR100 = lutGrid[index_b_int + 1][index_g_int][index_r_int].r;
                float interG100 = lutGrid[index_b_int + 1][index_g_int][index_r_int].g;
                float interB100 = lutGrid[index_b_int + 1][index_g_int][index_r_int].b;

                float interR001 = lutGrid[index_b_int][index_g_int][index_r_int + 1].r;
                float interG001 = lutGrid[index_b_int][index_g_int][index_r_int + 1].g;
                float interB001 = lutGrid[index_b_int][index_g_int][index_r_int + 1].b;

                float interR101 = lutGrid[index_b_int + 1][index_g_int][index_r_int + 1].r;
                float interG101 = lutGrid[index_b_int + 1][index_g_int][index_r_int + 1].g;
                float interB101 = lutGrid[index_b_int + 1][index_g_int][index_r_int + 1].b;

                float interR010 = lutGrid[index_b_int][index_g_int + 1][index_r_int].r;
                float interG010 = lutGrid[index_b_int][index_g_int + 1][index_r_int].g;
                float interB010 = lutGrid[index_b_int][index_g_int + 1][index_r_int].b;

                float interR110 = lutGrid[index_b_int + 1][index_g_int + 1][index_r_int].r;
                float interG110 = lutGrid[index_b_int + 1][index_g_int + 1][index_r_int].g;
                float interB110 = lutGrid[index_b_int + 1][index_g_int + 1][index_r_int].b;

                float interR011 = lutGrid[index_b_int][index_g_int + 1][index_r_int + 1].r;
                float interG011 = lutGrid[index_b_int][index_g_int + 1][index_r_int + 1].g;
                float interB011 = lutGrid[index_b_int][index_g_int + 1][index_r_int + 1].b;

                float interR111 = lutGrid[index_b_int + 1][index_g_int + 1][index_r_int + 1].r;
                float interG111 = lutGrid[index_b_int + 1][index_g_int + 1][index_r_int + 1].g;
                float interB111 = lutGrid[index_b_int + 1][index_g_int + 1][index_r_int + 1].b;

                outR = interp3(interR000, interR100, interR001, interR101, interR010, interR110, interR011, interR111,
                               dist_r, dist_g, dist_b, lutdim - 1);
                outG = interp3(interG000, interG100, interG001, interG101, interG010, interG110, interG011, interG111,
                               dist_r, dist_g, dist_b, lutdim - 1);
                outB = interp3(interB000, interB100, interB001, interB101, interB010, interB110, interB011, interB111,
                               dist_r, dist_g, dist_b, lutdim - 1);
            }

            int R = outR * 255;
            int G = outG * 255;
            int B = outB * 255;

            outPix[0] = outR * 255;
            outPix[1] = outG * 255;
            outPix[2] = outB * 255;

            out_img.at<cv::Vec3b>(i, j)[0] = (unsigned char)outPix[0];
            out_img.at<cv::Vec3b>(i, j)[1] = (unsigned char)outPix[1];
            out_img.at<cv::Vec3b>(i, j)[2] = (unsigned char)outPix[2];
        }
    }
}