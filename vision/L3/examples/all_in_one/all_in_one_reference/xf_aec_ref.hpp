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

void HistogramKernel_sin_ref(cv::Mat& src1,
                             cv::Mat& src2,
                             unsigned int hist_array1[AEC_HIST_SIZE],
                             float p,
                             float inputMin,
                             float inputMax,
                             float outputMin,
                             float outputMax,
                             int histSize) {
    int width = src1.cols;
    int height = src1.rows;
    int in_pix;          // depth of histogram tree
    int bins = histSize; // number of bins at each histogram level

    // unsigned int hist_array1[histSize];

    for (int k = 0; k < histSize; k++) {
        hist_array1[k] = 0;
    }
    int tmp_hist[histSize];
    int in_buf, in_buf1, temp_buf;
    bool flag = 0;

    for (int i = 0; i < histSize; i++) {
        tmp_hist[i] = 0;
    }
    static unsigned int old = 0;
    unsigned int acc_rd = 0;
    unsigned int acc_wr = 0;
    int readcnt = 0;
    float min_vals = inputMin - 0.5f;
    float max_vals = inputMax + 0.5f;
    float minValue = min_vals, minValue1 = min_vals;
    float maxValue = max_vals, maxValue1 = max_vals;
    float interval = (maxValue - minValue) / bins;
    float internal_inv = (1 / interval);
    int pos = 0, pos1 = 0;
    int currentBin = 0, currentBin1 = 0;

    for (int row = 0; row < (height); row++) {
        for (int col = 0; col < (width); col++) {
            in_pix = src1.at<unsigned short>(row, col);
            src2.at<unsigned short>(row, col) = in_pix;
            currentBin = int((in_pix - minValue) * internal_inv);

            if (currentBin == old) {
                acc_rd = acc_wr;
            } else {
                acc_rd = tmp_hist[currentBin];
            }

            tmp_hist[old] = acc_wr;
            acc_wr = acc_rd + 1;
            old = currentBin;
        }
    }

    tmp_hist[old] = acc_wr;

    for (int i = 0; i < histSize; i++) {
        unsigned int value = 0;
        value += tmp_hist[i];
        hist_array1[i] = value;
    }
    // for(int i=0;i<1;i++){
    //     for(int j=0;j<histSize;j++){
    //         hist_array.at<float>(i,j)=(float)hist_array1[i * (histSize) + j];
    //     }
    // }
}

void Equalize_norm_sin_ref(cv::Mat& src,
                           cv::Mat& dst,
                           float p,
                           unsigned int hist_array1[AEC_HIST_SIZE],
                           float inputMin,
                           float inputMax,
                           float outputMin,
                           float outputMax) {
    cv::Mat src2;
    src2.create(src.rows, src.cols, CV_IN_TYPE);
    int histSize = 4096;
    /// Set the ranges ( for B,G,R) )
    float range[] = {0, 4096};
    const float* histRange = {range};
    HistogramKernel_sin_ref(src, src2, hist_array1, p, inputMin, inputMax, outputMin, outputMax, histSize);
    // cv::calcHist(&src, 1, 0, cv::Mat(), hist_array, 1, &histSize, &histRange, 1, 0);
    // unsigned int hist_array1[histSize];

    FILE* fp1 = fopen("hist_array_ref.csv", "w");
    for (int i = 0; i < AEC_HIST_SIZE; i++) {
        // for (int j = 0; j < histSize; j++) {
        // unsigned int val2 = (unsigned int)hist_array.at<float>(i,j);
        // hist_array1[j]=val2;
        // fprintf(fp1, "%d ", val2);
        fprintf(fp1, "%d ", hist_array1[i]);
        fprintf(fp1, "\n");
        // }
    }
    fclose(fp1);

    // unsigned int hist_array1[histSize];
    int width = src.cols;
    int height = src.rows;

    int bins = histSize;
    int nElements = histSize;

    int total = height * width;

    float min_vals = inputMin - 0.5f;
    float max_vals = inputMax + 0.5f;
    float minValue = min_vals; //{-0.5, -0.5, -0.5};
    float maxValue = max_vals; //{12287.5, 16383.5, 12287.5};
    float s1 = p;
    float s2 = p;

    int rval = s1 * total / 100;
    int rval1 = (100 - s2) * total / 100;

    for (int j = 0; j < 1; ++j)
    // searching for s1 and s2
    {
        int p1 = 0;
        int p2 = bins - 1;
        int n1 = 0;
        int n2 = total;
        float interval = (max_vals - min_vals) / bins;

        for (int k = 0; k < 1; ++k)
        // searching for s1 and s2
        {
            int value = hist_array1[p1];
            int value1 = hist_array1[p2];

            while (n1 + hist_array1[p1] < rval && p1 < histSize) {
                p1++;
                n1 += hist_array1[p1];
                minValue += interval;
            }
            // p1 *= bins;

            while (n2 - hist_array1[p2] > rval1 && p2 != 0) {
                p2--;
                n2 -= hist_array1[p2];
                maxValue -= interval;
            }
        }
    }

    float maxmin_diff;
    float newmax = inputMax;
    float newmin = 0.0f;
    maxmin_diff = maxValue - minValue;

    float newdiff = newmax - newmin;
    int in_buf_n, in_buf_n1, out_buf_n;
    int row, col;
    float inv_val;

    if (maxmin_diff != 0) {
        inv_val = (1 / maxmin_diff);
    }

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            in_buf_n = src.at<unsigned short>(row, col);

            float value = 0.0f;
            float divval = 0.0f;
            float finalmul = 0.0f;
            int dstval;
            // clang-format on
            unsigned short val = in_buf_n;

            value = val - minValue;
            divval = value * inv_val;
            finalmul = divval * newdiff;
            dstval = (finalmul + newmin);
            if (dstval < 0) {
                dstval = 0;
            }
            out_buf_n = dstval;
            dst.at<unsigned short>(row, col) = out_buf_n;
        }
    }

    FILE* fp2 = fopen("dst_final_ocv.csv", "w");
    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width; ++j) {
            unsigned short val2 = dst.at<unsigned short>(i, j);
            fprintf(fp2, "%d ", val2);
        }
        fprintf(fp2, "\n");
    }
    fclose(fp2);
}

void aec_ref(cv::Mat& src, cv::Mat& dst, unsigned short pawb) {
    float thresh = (float)pawb / 256;
    // float thresh = 0.9f;
    float inputMin = 0.0f;
    float inputMax = 65535.0f;
    float outputMin = 0.0f;
    float outputMax = 65535.0f;
    // cv::calcHist(&src, 1, 0, cv::Mat(), hist_ocv, 1, &histSize, &histRange, 1, 0);

    unsigned int hist_array1[AEC_HIST_SIZE];
    // hist_array1.create(height, width, CV_16UC1);

    // HistogramKernel_sin_ref(src, src2, hist_array1, thresh, inputMin,inputMax, outputMin, outputMax);

    // FILE* fp1 = fopen("hist.csv", "w");
    // for (int i = 0; i < histSize; ++i) {

    //     fprintf(fp1, "%d ", hist_array1[i]);
    //     fprintf(fp1, "\n");

    // }
    // fclose(fp1);

    Equalize_norm_sin_ref(src, dst, thresh, hist_array1, inputMin, inputMax, outputMin, outputMax);
}