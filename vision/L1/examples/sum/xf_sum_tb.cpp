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
#include "xf_sum_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> \n");
        return -1;
    }

    cv::Mat in_gray, in_gray1, out_gray, diff;
#if GRAY
    in_gray = cv::imread(argv[1], 0); // read image
#endif
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }

    int channels = in_gray.channels();

    double ocv_ref[3];
    // OpenCV function

    ocv_ref[0] = cv::sum(in_gray)[0];

    //	 ocv_ref[1]=cv::sum(in_gray)[1];
    //	 ocv_ref[2]=cv::sum(in_gray)[2];

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray.rows, in_gray.cols);

    double* scl = (double*)malloc(XF_CHANNELS(IN_TYPE, NPC1) * sizeof(double));

    imgInput1.copyTo(in_gray.data);

    sum_accel(imgInput1, scl);

    for (int i = 0; i < in_gray.channels(); i++) {
        printf("sum of opencv is=== %lf\n", ocv_ref[i]);
        printf("sum of hls is====== %lf\n", scl[i]);
    }

    return 0;
}
