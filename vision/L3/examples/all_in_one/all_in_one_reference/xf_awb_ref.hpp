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
using namespace std;

template <typename T>
void balanceWhiteSimple(std::vector<cv::Mat_<T> >& src,
                        cv::Mat& dst,
                        const float inputMin,
                        const float inputMax,
                        const float outputMin,
                        const float outputMax,
                        const float p) {
    /********************* Simple white balance *********************/
    const float s1 = p; // low quantile
    const float s2 = p; // high quantile

    int depth = 2; // depth of histogram tree
    if (src[0].depth() != CV_8U) ++depth;
    int bins = HIST_SIZE; // number of bins at each histogram level

    int nElements = HIST_SIZE; // int(pow((float)bins, (float)depth));

    int countval = 0;

    for (size_t i = 0; i < src.size(); ++i) {
        std::vector<int> hist(nElements, 0);

        typename cv::Mat_<T>::iterator beginIt = src[i].begin();
        typename cv::Mat_<T>::iterator endIt = src[i].end();

        for (typename cv::Mat_<T>::iterator it = beginIt; it != endIt; ++it) // histogram filling
        {
            int pos = 0;
            float minValue = inputMin - 0.5f;
            float maxValue = inputMax + 0.5f;
            T val = *it;

            float interval = float(maxValue - minValue) / bins;

            for (int j = 0; j < 1; ++j) {
                int currentBin = int((val - minValue + 1e-4f) / interval);
                ++hist[pos + currentBin];

                pos = (pos + currentBin) * bins;

                minValue = minValue + currentBin * interval;
                maxValue = minValue + interval;

                interval /= bins;
            }

            countval++;
        }
        /*FILE* fp = fopen("hist.txt", "a");

        for (int j = 0; j < 256; j++) {
            fprintf(fp, "%d\n", hist[j]);
        }

        fclose(fp);*/

        int total = int(src[i].total());

        int p1 = 0, p2 = bins - 1;
        int n1 = 0, n2 = total;

        float minValue = inputMin - 0.5f;
        float maxValue = inputMax + 0.5f;

        float interval = (maxValue - minValue) / float(bins);

        for (int j = 0; j < 1; ++j)
        // searching for s1 and s2
        {
            while (n1 + hist[p1] < s1 * total / 100.0f) {
                n1 += hist[p1++];
                minValue += interval;

                // printf("ocv min vlaues:%f %d\n",minValue,n1);
            }
            p1 *= bins;

            while (n2 - hist[p2] > (100.0f - s2) * total / 100.0f) {
                n2 -= hist[p2--];
                maxValue -= interval;
            }
            p2 = (p2 + 1) * bins - 1;

            interval /= bins;
        }

        //        printf("%f %f\n",maxValue,minValue);

        src[i] = (outputMax - outputMin) * (src[i] - minValue) / (maxValue - minValue) + outputMin;
    }

    /****************************************************************/

    dst.create(/**/ src[0].size(), CV_MAKETYPE(src[0].depth(), int(src.size())) /**/);
    cv::merge(src, dst);

    // printf("\ncountvalue:%d\n", countval);
}

void calculateChannelSums(uint& sumB, uint& sumG, uint& sumR, uchar* src_data, int src_len, float thresh) {
    sumB = sumG = sumR = 0;
    ushort thresh255 = (ushort)cvRound(thresh * 255);
    int i = 0;
    unsigned int minRGB, maxRGB;
    for (; i < src_len; i += 3) {
        minRGB = std::min(src_data[i], std::min(src_data[i + 1], src_data[i + 2]));
        maxRGB = std::max(src_data[i], std::max(src_data[i + 1], src_data[i + 2]));
        if ((maxRGB - minRGB) * 255 > thresh255 * maxRGB) continue;
        sumB += src_data[i];
        sumG += src_data[i + 1];
        sumR += src_data[i + 2];
    }
}

void calculateChannelSums(uint64& sumB, uint64& sumG, uint64& sumR, ushort* src_data, int src_len, float thresh) {
    sumB = sumG = sumR = 0;
    uint thresh65535 = cvRound(thresh * 65535);
    int i = 0;
    unsigned int minRGB, maxRGB;
    for (; i < src_len; i += 3) {
        minRGB = std::min(src_data[i], std::min(src_data[i + 1], src_data[i + 2]));
        maxRGB = std::max(src_data[i], std::max(src_data[i + 1], src_data[i + 2]));
        if ((maxRGB - minRGB) * 65535 > thresh65535 * maxRGB) continue;
        sumB += src_data[i];
        sumG += src_data[i + 1];
        sumR += src_data[i + 2];
    }
}

void applyChannelGains(cv::Mat& src, cv::Mat& dst, float gainB, float gainG, float gainR) {
    int N3 = 3 * src.cols * src.rows;
    int i = 0;

    // Scale gains by their maximum (fixed point approximation works only when all
    // gains are <=1)
    float gain_max = std::max(gainB, std::max(gainG, gainR));
    if (gain_max > 0) {
        gainB /= gain_max;
        gainG /= gain_max;
        gainR /= gain_max;
    }

    if (src.type() == CV_8UC3) {
        // Fixed point arithmetic, mul by 2^8 then shift back 8 bits
        int i_gainB = cvRound(gainB * (1 << 8)), i_gainG = cvRound(gainG * (1 << 8)),
            i_gainR = cvRound(gainR * (1 << 8));
        const uchar* src_data = src.ptr<uchar>();
        uchar* dst_data = dst.ptr<uchar>();
        for (; i < N3; i += 3) {
            dst_data[i] = (uchar)((src_data[i] * i_gainB) >> 8);
            dst_data[i + 1] = (uchar)((src_data[i + 1] * i_gainG) >> 8);
            dst_data[i + 2] = (uchar)((src_data[i + 2] * i_gainR) >> 8);
        }
    } else if (src.type() == CV_16UC3) {
        // Fixed point arithmetic, mul by 2^16 then shift back 16 bits
        int i_gainB = cvRound(gainB * (1 << 16)), i_gainG = cvRound(gainG * (1 << 16)),
            i_gainR = cvRound(gainR * (1 << 16));

        // printf("%f %f %f\n",gainB,gainG,gainR);
        // printf("%d %d %d\n",i_gainB,i_gainG,i_gainR);
        const ushort* src_data = src.ptr<ushort>();
        ushort* dst_data = dst.ptr<ushort>();

        for (; i < N3; i += 3) {
            dst_data[i] = (ushort)((src_data[i] * i_gainB) >> 16);
            dst_data[i + 1] = (ushort)((src_data[i + 1] * i_gainG) >> 16);
            dst_data[i + 2] = (ushort)((src_data[i + 2] * i_gainR) >> 16);
        }
    }
}

void balanceWhiteGW(cv::Mat& src, cv::Mat& dst) {
    int N = src.cols * src.rows, N3 = N * 3;

    double dsumB = 0.0, dsumG = 0.0, dsumR = 0.0;
    // unsigned short pawb = 128;
    // float thresh = (float)pawb / 256;
    float thresh = 0.9;

#if T_8U
    uint sumB = 0, sumG = 0, sumR = 0;
    calculateChannelSums(sumB, sumG, sumR, src.ptr<uchar>(), N3, thresh);
    dsumB = (double)sumB;
    dsumG = (double)sumG;
    dsumR = (double)sumR;

    // Find inverse of averages
    double max_sum = std::max(dsumB, std::max(dsumR, dsumG));
    const double eps = 0.1;
    float dinvB = dsumB < eps ? 0.f : (float)(max_sum / dsumB), dinvG = dsumG < eps ? 0.f : (float)(max_sum / dsumG),
          dinvR = dsumR < eps ? 0.f : (float)(max_sum / dsumR);

    // Use the inverse of averages as channel gains:
    applyChannelGains(src, dst, dinvB, dinvG, dinvR);
#else
    uint64 sumB = 0, sumG = 0, sumR = 0;
    calculateChannelSums(sumB, sumG, sumR, src.ptr<ushort>(), N3, thresh);
    dsumB = (double)sumB;
    dsumG = (double)sumG;
    dsumR = (double)sumR;

    // printf("%ld %ld %ld\n",sumB,sumG,sumR);

    // Find inverse of averages
    double max_sum = std::max(dsumB, std::max(dsumR, dsumG));
    const double eps = 0.1;
    float dinvB = dsumB < eps ? 0.f : (float)(max_sum / dsumB), dinvG = dsumG < eps ? 0.f : (float)(max_sum / dsumG),
          dinvR = dsumR < eps ? 0.f : (float)(max_sum / dsumR);

    // Use the inverse of averages as channel gains:
    applyChannelGains(src, dst, dinvB, dinvG, dinvR);

#endif
}