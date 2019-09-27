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
#include "xf_convert_bitdepth_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img;
    cv::Mat in_gray, input_img; //, ocv_ref;

    // reading in the color image
    in_img = cv::imread(argv[1], 0);

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

// cvtColor(in_img,in_gray,CV_BGR2GRAY);

#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
    in_img.convertTo(input_img, OCV_INTYPE);
#endif

    // create memory for output image
    cv::Mat ocv_ref(in_img.rows, in_img.cols, OCV_OUTTYPE);
    cv::Mat diff(in_img.rows, in_img.cols, OCV_OUTTYPE);
    unsigned short int height = in_img.rows;
    unsigned short int width = in_img.cols;

///////////////// 	Opencv  Reference  ////////////////////////

#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
    input_img.convertTo(ocv_ref, OCV_OUTTYPE);
#else
    in_img.convertTo(ocv_ref, OCV_OUTTYPE);
#endif

    cv::imwrite("out_ocv.jpg", ocv_ref);
    //////////////////////////////////////////////////////////////

    ap_int<4> _convert_type = CONVERT_TYPE;
    int shift = 0;

    static xf::cv::Mat<_SRC_T, HEIGHT, WIDTH, _NPC> imgInput(in_img.rows, in_img.cols);
    static xf::cv::Mat<_DST_T, HEIGHT, WIDTH, _NPC> imgOutput(in_img.rows, in_img.cols);
    static xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, _NPC> in_8bit(in_img.rows, in_img.cols);

    // in_8bit = xf::cv::imread<XF_8UC1, HEIGHT, WIDTH, _NPC>(argv[1], 0);
    in_8bit.copyTo(in_img.data);

#if (XF_CONVERT16STO8U)
    // imgInput.copyTo((IN_TYPE *) input_img.data);
    in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_16S);
#elif (XF_CONVERT16UTO8U || XF_CONVERT16UTO32S || XF_CONVERT16STO32S)
    in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_16U);
#elif (XF_CONVERT32STO8U || XF_CONVERT32STO16U || XF_CONVERT32STO16S)
    in_8bit.convertTo(imgInput, XF_CONVERT_8U_TO_32S);
#else
    // imgInput.copyTo((IN_TYPE *)in_img.data);
    imgInput = in_8bit;
#endif

    convert_bitdepth_accel(imgInput, imgOutput, _convert_type, shift);

    xf::cv::imwrite("hls_out.png", imgOutput);

    //////////////////  Compute Absolute Difference ////////////////////
    xf::cv::absDiff(ocv_ref, imgOutput, diff);
    imwrite("out_err.png", diff);

    // Find minimum and maximum differences.
    double minval = 256, maxval = 0;
    int cnt = 0;
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
            uchar v = diff.at<uchar>(i, j);
            if (v > 0) cnt++;
            if (minval > v) minval = v;
            if (maxval < v) maxval = v;
        }
    }
    float err_per = 100.0 * (float)cnt / (in_img.rows * in_img.cols);
    fprintf(stderr,
            "Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error "
            "threshold = %f\n",
            minval, maxval, err_per);

    if (err_per > 0.0f) {
        printf("\nTest Failed\n");
        return -1;
    }

    return 0;
}
