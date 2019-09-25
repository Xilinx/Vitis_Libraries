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
#include "xf_reduce_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, dst_hls, ocv_ref, in_gray, diff, in_mask;

    unsigned short in_width, in_height;

#if GRAY
    /*  reading in the color image  */
    in_img = cv::imread(argv[1], 0);
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    in_width = in_img.cols;
    in_height = in_img.rows;

#if DIM
    dst_hls.create(in_img.rows, 1, CV_8UC1);
    ocv_ref.create(in_img.rows, 1, CV_8UC1);
#else
    dst_hls.create(1, in_img.cols, CV_8UC1);
    ocv_ref.create(1, in_img.cols, CV_8UC1);
#endif

    int bytes;

    ////////////////  reference code  ////////////////
    if ((REDUCTION_OP == XF_REDUCE_AVG) || (REDUCTION_OP == XF_REDUCE_SUM)) {
        bytes = 4;
        cv::reduce(in_img, ocv_ref, DIM, REDUCTION_OP, CV_32SC1); // avg,sum

    } else {
        bytes = 1;

        cv::reduce(in_img, ocv_ref, DIM, REDUCTION_OP, CV_8UC1);
    }

    ////////////////////// HLS TOP function call ////////////////////////////

    static xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows, in_img.cols);
#if DIM
    static xf::cv::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, XF_NPPC1> imgOutput(in_img.rows, 1);
#else
    static xf::cv::Mat<DST_T, ONE_D_HEIGHT, ONE_D_WIDTH, XF_NPPC1> imgOutput(1, in_img.cols);
#endif

    imgInput.copyTo(in_img.data);

    reduce_accel(imgInput, imgOutput, DIM);

    dst_hls.data = imgOutput.copyFrom();

    FILE* fp = fopen("hls", "w");
    FILE* fp1 = fopen("cv", "w");
#if DIM == 1
    for (unsigned int i = 0; i < dst_hls.rows; i++) {
        fprintf(fp, "%d\n", (unsigned char)dst_hls.data[i]);
        fprintf(fp1, "%d\n", ocv_ref.data[i]);
        unsigned int diff = ocv_ref.data[i] - (unsigned char)dst_hls.data[i];
        if (diff > 1) {
            printf("Output is not matched with opnecv\n");
        }
    }
#endif
#if DIM == 0
    for (unsigned int i = 0; i < dst_hls.cols; i++) {
        fprintf(fp, "%d\n", (unsigned char)dst_hls.data[i]);
        fprintf(fp1, "%d\n", ocv_ref.data[i]);
        unsigned int diff = ocv_ref.data[i] - (unsigned char)dst_hls.data[i];
        if (diff > 1) {
            printf("Output is not matched with opnecv\n");
        }
    }
#endif
    fclose(fp);
    fclose(fp1);

    return 0;
}
