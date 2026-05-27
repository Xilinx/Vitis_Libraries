/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
#include <ap_int.h>
#include <stdint.h>
#include <vector>
#include <opencv2/core.hpp>
#include "common/xf_headers.hpp"
#include "xf_findcontours_tb_config.h"
#include <time.h>

std::chrono::time_point<std::chrono::high_resolution_clock> start;
std::chrono::time_point<std::chrono::high_resolution_clock> stop;
std::chrono::microseconds tdiff;
#define START_TIMER start = std::chrono::high_resolution_clock::now();
#define STOP_TIMER(name)                                                                                       \
    stop = std::chrono::high_resolution_clock::now();                                                          \
    tdiff = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);                               \
    std::cout << "RUNTIME of " << name << ": " << ((float)tdiff.count() / (float)1000) << " ms " << std::endl; \
    std::cout << "FPS of " << name << ": " << (1000000 / tdiff.count()) << " fps " << std::endl;

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img, out_img;
    cv::Mat in_gray, diff;

    // Reading in the images:
    in_gray = cv::imread(argv[1], 0);

    if (in_gray.data == NULL) {
        fprintf(stderr, "ERROR: Cannot open image %s\n ", argv[1]);
        return EXIT_FAILURE;
    }

    cv::Mat refThreshold1, refThreshold2;

    threshold(in_gray, refThreshold1, 130, 255, cv::THRESH_BINARY);

    ap_uint<32>* points = (ap_uint<32>*)malloc(MAX_TOTAL_POINTS * sizeof(int));
    ap_uint<32>* offsets = (ap_uint<32>*)malloc((MAX_CONTOURS + 1) * sizeof(int));
    ap_uint<32>* numc = (ap_uint<32>*)malloc(sizeof(int));
START_TIMER
    findcontours_accel((ap_uint<8>*)refThreshold1.data, in_gray.rows, in_gray.cols, points, offsets, numc);
STOP_TIMER("Total time to process frame")

    std::vector<std::vector<cv::Point> > hlscontours;
    std::cout << "Contours: " << (unsigned)numc[0] << "\n";
    for (unsigned c = 0; c < (unsigned)numc[0]; ++c) {
        unsigned s = offsets[c];
        unsigned e = offsets[c + 1];
        std::cout << "Contour " << c << " size=" << (e - s) << "\n  ";
        std::vector<cv::Point> contour_pts;
        contour_pts.reserve(e - s);
        for (unsigned i = s; i < e; i++) {
            ap_uint<32> p = points[i];
            unsigned x = p & 0xFFFF;
            unsigned y = (p >> 16) & 0xFFFF;
            std::cout << "(" << x << "," << y << ") ";
            contour_pts.emplace_back((int)x, (int)y);
        }
        hlscontours.push_back(std::move(contour_pts));
        std::cout << "\n";
    }

    cv::Mat hlsContourOutput(in_gray.rows, in_gray.cols, CV_8UC1);
    hlsContourOutput.setTo(cv::Scalar(255));
    drawContours(hlsContourOutput, hlscontours, -1, cv::Scalar(0, 255, 0), 0); // Green contours
    imwrite("hls_contours.png", hlsContourOutput);

    // Opencv Reference
    std::vector<std::vector<cv::Point> > refcontours;
START_TIMER
    cv::findContours(refThreshold1, refcontours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
STOP_TIMER("Total time to process ref frame")
    cv::Mat refContourOutput(in_gray.rows, in_gray.cols, CV_8UC1);
    refContourOutput.setTo(cv::Scalar(255));
    drawContours(refContourOutput, refcontours, -1, cv::Scalar(0, 255, 0), 0); // Green contours
    imwrite("ocv_contours.png", refContourOutput);

    for (size_t i = 0; i < refcontours.size(); ++i) {
        std::cout << "External Contour " << i << ": ";
        for (auto& p : refcontours[i]) {
            std::cout << "(" << p.x << "," << p.y << ") ";
        }
        std::cout << "\n";
    }

    cv::absdiff(hlsContourOutput, refContourOutput, diff);
    imwrite("diff.png", diff);

    return 0;
}
