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
#include "xf_otsuthreshold_config.h"

/************* Otsu reference implementation *********************************/

double GetOtsuThresholdFloat(cv::Mat _src) {
    cv::Size size = _src.size();
    if (_src.isContinuous()) {
        size.width *= size.height;
        size.height = 1;
    }
    const int N = 256;
    int i, j, h[N] = {0};

    for (i = 0; i < size.height; i++) {
        const unsigned char* src = _src.data + _src.step * i;
        j = 0;
        for (; j < size.width; j++) {
            h[src[j]]++;
        }
    }
    double mu = 0.f;
    double scale;

    scale = 1. / (size.width * size.height);

    for (i = 0; i < N; i += 2) {
        double a = (double)h[i];
        double b = (double)h[i + 1];
        mu += i * (a + b) + b;
    }

    mu = mu * scale;

    double mu1 = 0, q1 = 0;
    double max_sigma = 0, max_val = 0;

    for (i = 0; i < N; i++) {
        double p_i, q2, mu2, sigma;

        p_i = h[i] * scale;
        mu1 *= q1;
        q1 += p_i;
        q2 = 1. - q1;

        if (std::min(q1, q2) < FLT_EPSILON || std::max(q1, q2) > 1. - FLT_EPSILON) continue;

        mu1 = (mu1 + i * p_i) / q1;

        mu2 = (mu - q1 * mu1) / q2;

        sigma = q1 * q2 * (mu1 - mu2) * (mu1 - mu2);
        if (sigma > max_sigma) {
            max_sigma = sigma;
            max_val = i;
        }
    }

    return max_val;
}

/***************************************************************************/

int main(int argc, char** argv) {
    double Otsuval_ref;
    uint8_t Otsuval;

    float minerr = 9999999.f;
    float maxerr = 0.f;
    int maxdiff = 0;
    int maxerr_imgnum = 0;
    int x1, y1, x2, y2, nw, nh;

    cv::Mat img = cv::imread(argv[1], 0);
    cv::Mat in_gray, res_img;
    //	cvtColor(img, in_gray, CV_BGR2GRAY);

    if (img.empty()) {
        std::cout << "Image not opened correctly, check path " << std::endl;
        return 1;
    }

    res_img = img.clone();
    x1 = 0;
    y1 = 0;
    nw = res_img.cols;
    nh = res_img.rows;

    Otsuval_ref = GetOtsuThresholdFloat(res_img);

    uint16_t img_height = img.rows;
    uint16_t img_width = img.cols;

    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPPC> imgInput(img.rows, img.cols);

    imgInput.copyTo(img.data);

    otsuthreshold_accel(imgInput, Otsuval);

    if (abs(Otsuval_ref - Otsuval) > maxdiff) maxdiff = abs(Otsuval_ref - Otsuval);

    printf("Floating Point: %d	HLS Threshold : %d	Difference :%d", (int)Otsuval_ref, (int)Otsuval, (int)maxdiff);

    if (maxdiff > 1) {
        printf("\n Test Failed");
        return -1;
    } else {
        printf("\n Test Passed\n");
        return 0;
    }
}
