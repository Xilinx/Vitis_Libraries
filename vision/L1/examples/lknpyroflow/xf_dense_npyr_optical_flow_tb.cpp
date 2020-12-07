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

#include "ap_int.h"
#include "hls_stream.h"
#include "xf_dense_npyr_optical_flow_config.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <opencv2/core/core.hpp>
#include <opencv2/opencv.hpp>
//#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
//#include <iostream>
#include <string>

int main(int argc, char** argv) {
    cv::Mat frame0, frame1;
    cv::Mat flowx, flowy;
    cv::Mat frame_out;

    if (argc != 3) {
        fprintf(stderr, "Usage incorrect. Correct usage: ./exe <current frame> <next frame>\n");
        return -1;
    }

    frame0 = cv::imread(argv[1], 0);
    frame1 = cv::imread(argv[2], 0);

    if (frame0.empty() || frame1.empty()) {
        fprintf(stderr, "input files not found!\n");
        return -1;
    }

    frame_out.create(frame0.rows, frame0.cols, CV_8UC4);
    flowx.create(frame0.rows, frame0.cols, CV_32FC1);
    flowy.create(frame0.rows, frame0.cols, CV_32FC1);

    int cnt = 0;
    unsigned char p1, p2, p3, p4;
    unsigned int pix = 0;

    char out_string[200];

    int height = frame0.rows;
    int width = frame0.cols;
    size_t image_in_size_bytes = frame0.rows * frame0.cols * 1 * sizeof(unsigned char);
    size_t image_out_size_bytes = frame0.rows * frame0.cols * 4 * sizeof(unsigned char);

    dense_non_pyr_of_accel((ap_uint<INPUT_PTR_WIDTH>*)frame0.data, (ap_uint<INPUT_PTR_WIDTH>*)frame1.data,
                           (ap_uint<OUTPUT_PTR_WIDTH>*)flowx.data, (ap_uint<OUTPUT_PTR_WIDTH>*)flowy.data, height,
                           width);

    float* flowx_copy;
    float* flowy_copy;

    flowx_copy = (float*)malloc(MAX_HEIGHT * MAX_WIDTH * (sizeof(float)));
    if (flowx_copy == NULL) {
        fprintf(stderr, "\nFailed to allocate memory for flowx_copy\n");
    }
    flowy_copy = (float*)malloc(MAX_HEIGHT * MAX_WIDTH * (sizeof(float)));
    if (flowy_copy == NULL) {
        fprintf(stderr, "\nFailed to allocate memory for flowy_copy\n");
    }

    int size = height * width;
    for (int f = 0; f < height; f++) {
        for (int i = 0; i < width; i++) {
            flowx_copy[f * width + i] = flowx.at<float>(f, i);
            flowy_copy[f * width + i] = flowy.at<float>(f, i);
        }
    }

    unsigned int* outputBuffer;
    outputBuffer = (unsigned int*)malloc(MAX_HEIGHT * MAX_WIDTH * (sizeof(unsigned int)));
    if (outputBuffer == NULL) {
        fprintf(stderr, "\nFailed to allocate memory for outputBuffer\n");
    }

    hls::stream<rgba_t> out_pix("Color pixel");

    xf::cv::getOutPix<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1>(flowx_copy, flowy_copy, frame1.data, out_pix, frame0.rows,
                                                       frame0.cols, frame0.cols * frame0.rows);

    xf::cv::writeMatRowsRGBA<MAX_HEIGHT, MAX_WIDTH, XF_NPPC1, KMED>(out_pix, outputBuffer, frame0.rows, frame0.cols,
                                                                    frame0.cols * frame0.rows);

    rgba_t* outbuf_copy;
    for (int i = 0; i < frame0.rows; i++) {
        for (int j = 0; j < frame0.cols; j++) {
            outbuf_copy = (rgba_t*)(outputBuffer + i * (frame0.cols) + j);
            p1 = outbuf_copy->r;
            p2 = outbuf_copy->g;
            p3 = outbuf_copy->b;
            p4 = outbuf_copy->a;
            pix = ((unsigned int)p4 << 24) | ((unsigned int)p3 << 16) | ((unsigned int)p2 << 8) | (unsigned int)p1;
            frame_out.at<unsigned int>(i, j) = pix;
        }
    }

    sprintf(out_string, "out_%d.png", cnt);
    cv::imwrite(out_string, frame_out);

    free(flowx_copy);
    free(flowy_copy);
    return 0;
}
