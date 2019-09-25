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
#include "xf_crop_config.h"
#include <sys/time.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
using namespace std;

int main(int argc, char** argv) {
    struct timespec start_time;
    struct timespec end_time;
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img[NUM_ROI], ocv_ref[NUM_ROI], in_gray[NUM_ROI], diff[NUM_ROI], out_img1, in_gray1, diff1,
        ocv_ref1;

#if GRAY
    /*  reading in the color image  */
    in_img = cv::imread(argv[1], 0);
#else
    in_img = cv::imread(argv[1], 1);
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    unsigned int x_loc[NUM_ROI];
    unsigned int y_loc[NUM_ROI];
    unsigned int ROI_height[NUM_ROI];
    unsigned int ROI_width[NUM_ROI];

    //	1ST ROI
    x_loc[0] = 0;
    y_loc[0] = 0;
    ROI_height[0] = 125;
    ROI_width[0] = 125;
    //	2nd ROI
    x_loc[1] = 50;
    y_loc[1] = 50;
    ROI_height[1] = 50;
    ROI_width[1] = 50;

    x_loc[2] = 48;
    y_loc[2] = 48;
    ROI_height[2] = 20;
    ROI_width[2] = 20;

    for (int i = 0; i < NUM_ROI; i++) {
#if GRAY
        out_img[i].create(ROI_height[i], ROI_width[i], in_img.depth());
        ocv_ref[i].create(ROI_height[i], ROI_width[i], in_img.depth());
        diff[i].create(ROI_height[i], ROI_width[i], in_img.depth());
#else
        out_img[i].create(ROI_height[i], ROI_width[i], CV_8UC3);
        ocv_ref[i].create(ROI_height[i], ROI_width[i], CV_8UC3);
        diff[i].create(ROI_height[i], ROI_width[i], CV_8UC3);
#endif
    }

    ////////////////  reference code  ////////////////
    //	clock_gettime(CLOCK_MONOTONIC, &start_time);
    //   struct timeval startud1, endud1;
    //   gettimeofday(&startud1,NULL);
    cv::Rect ROI(x_loc[0], y_loc[0], ROI_width[0], ROI_height[0]);
    ocv_ref[0] = in_img(ROI);
    cv::Rect ROI1(x_loc[1], y_loc[1], ROI_width[1], ROI_height[1]);
    ocv_ref[1] = in_img(ROI1);
    cv::Rect ROI2(x_loc[2], y_loc[2], ROI_width[2], ROI_height[2]);
    ocv_ref[2] = in_img(ROI2);
    //    gettimeofday(&endud1,NULL);
    //    double diff_time2_ud = (endud1.tv_usec -  startud1.tv_usec);
    //   printf (" TimeUpdate usec = %lf\n", diff_time2_ud);

    //	clock_gettime(CLOCK_MONOTONIC, &end_time);
    //	float diff_latency = (end_time.tv_nsec - start_time.tv_nsec)/1e9 + end_time.tv_sec - start_time.tv_sec;
    //	printf("\latency: %f ", diff_latency);

    //////////////////  end opencv reference code//////////

    ////////////////////// HLS TOP function call ////////////////////////////

    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> imgInput(in_img.rows, in_img.cols);
    xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC> imgOutput[NUM_ROI]; //(ROI_height,ROI_width);

    xf::cv::Rect_<unsigned int> roi[NUM_ROI];

    for (int i = 0; i < NUM_ROI; i++) {
        roi[i].height = ROI_height[i];
        roi[i].width = ROI_width[i];
        roi[i].x = x_loc[i];
        roi[i].y = y_loc[i];
    }

    for (int i = 0; i < NUM_ROI; i++) {
        imgOutput[i].rows = roi[i].height;
        imgOutput[i].cols = roi[i].width;
    }

    imgInput.copyTo(in_img.data);

    crop_accel(imgInput, imgOutput, roi);

    for (int i = 0; i < NUM_ROI; i++) {
        out_img[i].data = imgOutput[i].copyFrom();
    }
    FILE* fp = fopen("data.txt", "w");
    for (int i = 0; i < (ROI_height[2]); i++) {
        for (int j = 0; j < ROI_width[2]; j++) {
            uchar ocv_val = ocv_ref[2].at<unsigned char>(i, j);
            uchar hls_val = out_img[2].at<unsigned char>(i, j);
            fprintf(fp, "%d %d\n", ocv_val, hls_val);
        }
    }
    fclose(fp);

    char hls_strg[30];
    char ocv_strg[30];
    char diff_strg[30];

    // Write output image
    for (int i = 0; i < NUM_ROI; i++) {
        sprintf(hls_strg, "out_img[%d].jpg", i);
        sprintf(ocv_strg, "ocv_ref[%d].jpg", i);
        sprintf(diff_strg, "diff_img[%d].jpg", i);
        cv::imwrite(hls_strg, out_img[i]); // hls image
        cv::imwrite(ocv_strg, ocv_ref[i]); // reference image
        cv::absdiff(ocv_ref[i], out_img[i], diff[i]);
        cv::imwrite(diff_strg, diff[i]); // Save the difference image for debugging purpose
    }

    //	 Find minimum and maximum differences.
    for (int roi = 0; roi < NUM_ROI; roi++) {
        double minval = 256, maxval1 = 0;
        int cnt = 0;
        for (int i = 0; i < ocv_ref[roi].rows; i++) {
            for (int j = 0; j < ocv_ref[roi].cols; j++) {
                uchar v = diff[roi].at<uchar>(i, j);
                if (v > 1) cnt++;
                if (minval > v) minval = v;
                if (maxval1 < v) maxval1 = v;
            }
        }
        float err_per = 100.0 * (float)cnt / (ocv_ref[roi].rows * ocv_ref[roi].cols);
        fprintf(stderr,
                "Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error "
                "threshold = %f\n",
                minval, maxval1, err_per);

        if (err_per > 0.0f) {
            return 1;
        }
    }
    return 0;
}
