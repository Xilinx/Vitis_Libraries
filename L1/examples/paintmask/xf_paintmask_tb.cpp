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
#include "xf_paintmask_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, in_gray, diff, in_mask;

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

    ocv_ref.create(in_img.rows, in_img.cols, in_img.depth());
    out_img.create(in_img.rows, in_img.cols, in_img.depth());
    diff.create(in_img.rows, in_img.cols, in_img.depth());
    in_mask.create(in_img.rows, in_img.cols, CV_8UC1);

    ap_uint<64> q = 0;
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
            for (int c = 0; c < in_img.channels(); c++) {
                if ((j > 250) && (j < 750)) {
                    in_mask.data[q + c] = 255;
                } else {
                    in_mask.data[q + c] = 0;
                }
            }
            q += in_img.channels();
        }
    }

    ////////////////////// HLS TOP function call ////////////////////////////

    static xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows, in_img.cols);
    static xf::cv::Mat<MASK_T, HEIGHT, WIDTH, NPIX> In_mask(in_mask.rows, in_mask.cols);
    static xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows, in_img.cols);

    unsigned char color[XF_CHANNELS(SRC_T, NPIX)];

    for (int i = 0; i < XF_CHANNELS(SRC_T, NPIX); i++) {
        color[i] = 150;
    }

    imgInput.copyTo(in_img.data);
    In_mask.copyTo(in_mask.data);
    imwrite("in_mask.jpg", in_mask);

    paintmask_accel(imgInput, In_mask, imgOutput, color);

    ////////////////  reference code  ////////////////
    unsigned long long int p = 0;
    for (int i = 0; i < ocv_ref.rows; i++) {
        for (int j = 0; j < ocv_ref.cols; j++) {
            for (int c = 0; c < ocv_ref.channels(); c++) {
                if (in_mask.data[p + c] != 0) {
                    ocv_ref.data[p + c] = color[c];
                } else {
                    ocv_ref.data[p + c] = in_img.data[p + c];
                }
            }
            p += in_img.channels();
        }
    }
    //////////////////  end C reference code//////////

    // Write output image
    xf::cv::imwrite("hls_out.jpg", imgOutput);
    cv::imwrite("ref_img.jpg", ocv_ref); // reference image

    xf::cv::absDiff(ocv_ref, imgOutput, diff);
    imwrite("diff_img.jpg", diff); // Save the difference image for debugging purpose

    // Find minimum and maximum differences.
    double minval = 256, maxval1 = 0;
    int cnt = 0;
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
            uchar v = diff.at<uchar>(i, j);
            if (v > 1) cnt++;
            if (minval > v) minval = v;
            if (maxval1 < v) maxval1 = v;
        }
    }
    float err_per = 100.0 * (float)cnt / (in_img.rows * in_img.cols);
    fprintf(stderr,
            "Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error "
            "threshold = %f\n",
            minval, maxval1, err_per);

    if (err_per > 0.0f) {
        return 1;
    }

    return 0;
}
