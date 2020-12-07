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
#include "xf_badpixelcorrection_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path1> \n");
        return -1;
    }

    cv::Mat in_gray, in_gray1, ocv_ref, out_gray, diff, ocv_ref_in1, ocv_ref_in2, inout_gray1, ocv_ref_gw;
#if T_8U
    in_gray = cv::imread(argv[1], 0); // read image
#else
    in_gray = cv::imread(argv[1], -1); // read image
#endif
    if (in_gray.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return -1;
    }

    ocv_ref.create(in_gray.rows, in_gray.cols, in_gray.type());
    ocv_ref_gw.create(in_gray.rows, in_gray.cols, in_gray.type());
    out_gray.create(in_gray.rows, in_gray.cols, in_gray.type());
    diff.create(in_gray.rows, in_gray.cols, in_gray.type());

    /////////////////////////////////////// CL ////////////////////////

    int height = in_gray.rows;
    int width = in_gray.cols;

    //////////////////////////Top function call ///////////////////////
    badpixelcorrection_accel((ap_uint<INPUT_PTR_WIDTH>*)in_gray.data, (ap_uint<INPUT_PTR_WIDTH>*)out_gray.data, height,
                             width);

    imwrite("out_hls.jpg", out_gray);

    return 0;
}
