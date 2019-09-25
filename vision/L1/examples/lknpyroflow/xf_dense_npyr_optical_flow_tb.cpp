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
#include "xf_dense_npyr_optical_flow_config.h"

int main(int argc, char* argv[]) {
    cv::Mat frame0, frame1;
    cv::Mat frame_out;

    if (argc != 3) {
        std::cout << "Usage incorrect. Correct usage: ./exe <current frame> <next frame>" << std::endl;
        return -1;
    }
    frame0 = cv::imread(argv[1], 0);
    frame1 = cv::imread(argv[2], 0);

    if (frame0.empty() || frame1.empty()) {
        std::cout << "input files not found!" << std::endl;
        return -1;
    }

    frame_out.create(frame0.rows, frame0.cols, CV_8UC4);
    int cnt = 0;
    unsigned char p1, p2, p3, p4;
    unsigned int pix = 0;

    char out_string[200];
    static xf::cv::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> buf0(frame0.rows, frame0.cols);
    static xf::cv::Mat<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC> buf1(frame0.rows, frame0.cols);
    static xf::cv::Mat<XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC> flowx(frame0.rows, frame0.cols);
    static xf::cv::Mat<XF_32FC1, MAX_HEIGHT, MAX_WIDTH, NPPC> flowy(frame0.rows, frame0.cols);

    buf0.copyTo(frame0.data);
    buf1.copyTo(frame1.data);
    // buf0 = xf::cv::imread<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(argv[1], 0);
    // buf1 = xf::cv::imread<XF_8UC1, MAX_HEIGHT, MAX_WIDTH, NPPC>(argv[2], 0);

    dense_non_pyr_of_accel(buf0, buf1, flowx, flowy);

    /* getting the flow vectors from hardware and colorcoding the vectors on a canvas of the size of the input*/
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
    unsigned int* outputBuffer;
    outputBuffer = (unsigned int*)malloc(MAX_HEIGHT * MAX_WIDTH * (sizeof(unsigned int)));
    if (outputBuffer == NULL) {
        fprintf(stderr, "\nFailed to allocate memory for outputBuffer\n");
    }

    flowx_copy = (float*)flowx.copyFrom();
    flowy_copy = (float*)flowy.copyFrom();
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
    return 0;
}
