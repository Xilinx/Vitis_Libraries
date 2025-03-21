/*
 * Copyright 2022 Xilinx, Inc.
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
#include "xf_autogaincontrol_tb_config.h"

#include <time.h>

void autogainControlref(cv::Mat& input,
                        unsigned int histogram[3][HIST_SIZE],
                        unsigned int* rgain,
                        unsigned int* bgain,
                        unsigned int* ggain) {
#if T_8U
    float inputMin = 0.0f;
    float inputMax = 255.0f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;
    unsigned int gain = 256;
#else
    float inputMin = 0.0f;
    float inputMax = 65535.0f;
    float outputMin = 0.0f;
    float outputMax = 65535.0f;
    unsigned int gain = 65536;
#endif

    std::cout << "started autogaincontrol" << std::endl;
    int bins = HIST_SIZE;                       // number of bins at each histogram level
    const int GAIN_ARRAY_SIZE = HIST_SIZE / 32; // histogram is divided into cluster of size 32bins each

    std::cout << "GAIN_ARRAY_SIZE is " << GAIN_ARRAY_SIZE << std::endl;
    unsigned int gain_array[GAIN_ARRAY_SIZE] = {
        0}; // Sample for gain_array values for 8bit {256,224,192,160,128,96,64,32}

    int temp_gain = gain;
    for (int i = 0; i < GAIN_ARRAY_SIZE; i++) {
        gain_array[i] = temp_gain;
        temp_gain = temp_gain - 32;
    }

    unsigned int hist_cluster_of_32bins[3][GAIN_ARRAY_SIZE] = {0};
    int gain_range_r = 0;
    int gain_range_g = 0;
    int gain_range_b = 0;
    unsigned int max_hist_cluster_of_32bins_r = 0;
    unsigned int max_hist_cluster_of_32bins_g = 0;
    unsigned int max_hist_cluster_of_32bins_b = 0;
    float minValue = inputMin - 0.5f;
    float maxValue = inputMax + 0.5f;
    float interval = float(maxValue - minValue) / bins;

    for (int y = 0; y < input.rows; ++y) {
        for (int x = 0; x < input.cols; ++x) {
#if T_8U
            int in_r = input.at<cv::Vec3b>(y, x)[0];
            int in_g = input.at<cv::Vec3b>(y, x)[1];
            int in_b = input.at<cv::Vec3b>(y, x)[2];
#elif T_16U
            unsigned int in_r = input.at<cv::Vec3w>(y, x)[0];
            unsigned int in_g = input.at<cv::Vec3w>(y, x)[1];
            unsigned int in_b = input.at<cv::Vec3w>(y, x)[2];
#endif
            unsigned int currentBin_r = (in_r / interval);
            unsigned int currentBin_g = (in_g / interval);
            unsigned int currentBin_b = (in_b / interval);
            histogram[0][currentBin_r] = histogram[0][currentBin_r] + 1;
            histogram[1][currentBin_g] = histogram[1][currentBin_g] + 1;
            histogram[2][currentBin_b] = histogram[2][currentBin_b] + 1;
        }
    }

    int hist_count = 0;
    for (int i = 0; i < GAIN_ARRAY_SIZE; i++) {
        for (int j = 0; j < 32; j++) {
            hist_cluster_of_32bins[0][i] += histogram[0][hist_count];
            hist_cluster_of_32bins[1][i] += histogram[1][hist_count];
            hist_cluster_of_32bins[2][i] += histogram[2][hist_count++];
        }
        if (hist_cluster_of_32bins[0][gain_range_r] <= hist_cluster_of_32bins[0][i]) {
            gain_range_r = i;
        }
        if (hist_cluster_of_32bins[1][gain_range_g] <= hist_cluster_of_32bins[1][i]) {
            gain_range_g = i;
        }
        if (hist_cluster_of_32bins[2][gain_range_b] <= hist_cluster_of_32bins[2][i]) {
            gain_range_b = i;
        }
    }

    *rgain = gain_array[gain_range_r];
    *ggain = gain_array[gain_range_g];
    *bgain = gain_array[gain_range_b];
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> \n");
        return -1;
    }

    cv::Mat in_img, ocv_ref, out_gray, diff;
#if T_8U
    in_img = cv::imread(argv[1], 1); // read image
#else
    in_img = cv::imread(argv[1], -1); // read image
#endif
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }

    int height = in_img.rows;
    int width = in_img.cols;

    unsigned int rgain_tb = 0;
    unsigned int bgain_tb = 0;
    unsigned int ggain_tb = 0;

    uint16_t gain[XF_CHANNELS(IN_TYPE, NPPCX)] = {0};

    // OpenCV Reference
    unsigned int histogram[XF_CHANNELS(IN_TYPE, NPPCX)][HIST_SIZE] = {0};

    autogainControlref(in_img, histogram, &rgain_tb, &bgain_tb, &ggain_tb);

    std::cout << "rgain is " << rgain_tb << std::endl;
    std::cout << "ggain is " << ggain_tb << std::endl;
    std::cout << "bgain is " << bgain_tb << std::endl;

    ///////////////////////////Top function call ///////////////////////////

    autogaincontrol_accel(histogram, gain);

    std::cout << "rgain_kernel is " << gain[0] << std::endl;
    std::cout << "ggain_kernel is " << gain[1] << std::endl;
    std::cout << "bgain_kernel is " << gain[2] << std::endl;

    imwrite("in_img.png", in_img);
    std::cout << "autogainControlref is done" << std::endl;

    return 0;
}
