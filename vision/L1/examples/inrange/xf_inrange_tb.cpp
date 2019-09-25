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
#include "xf_inrange_config.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid Number of Arguments!\nUsage:\n");
        fprintf(stderr, "<Executable Name> <input image path> \n");
        return -1;
    }

    cv::Mat in_img, out_img, ocv_ref, in_gray, diff;

    unsigned short in_width, in_height;
#if GRAY
    /*  reading in the color image  */
    in_img = cv::imread(argv[1], 0);
#else
    /*  reading in the color image  */
    in_img = cv::imread(argv[1], 1);
#endif
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image at %s\n", argv[1]);
        return 0;
    }

    in_width = in_img.cols;
    in_height = in_img.rows;
#if GRAY
    ocv_ref.create(in_img.rows, in_img.cols, in_img.depth());
    out_img.create(in_img.rows, in_img.cols, in_img.depth());
    diff.create(in_img.rows, in_img.cols, in_img.depth());
#else
    ocv_ref.create(in_img.rows, in_img.cols, CV_8UC1);
    out_img.create(in_img.rows, in_img.cols, CV_8UC3);
    diff.create(in_img.rows, in_img.cols, CV_8UC3);
#endif

    ////////////////  reference code  ////////////////

    unsigned char* lower_thresh = (unsigned char*)malloc(XF_CHANNELS(IN_TYPE, NPIX) * sizeof(unsigned char));
    unsigned char* upper_thresh = (unsigned char*)malloc(XF_CHANNELS(IN_TYPE, NPIX) * sizeof(unsigned char));

#if GRAY
    lower_thresh[0] = 50;
    upper_thresh[0] = 100;
#else
    lower_thresh[0] = 50;
    upper_thresh[0] = 100;
    lower_thresh[1] = 0;
    upper_thresh[1] = 150;
    lower_thresh[2] = 50;
    upper_thresh[2] = 150;
#endif
#if GRAY
    cv::inRange(in_img, lower_thresh[0], upper_thresh[0], ocv_ref);
#else
    cv::inRange(in_img, cv::Scalar(lower_thresh[0], lower_thresh[1], lower_thresh[2]),
                cv::Scalar(upper_thresh[0], upper_thresh[1], upper_thresh[2]), ocv_ref);
#endif
    //////////////////  end opencv reference code//////////

    ////////////////////// HLS TOP function call ////////////////////////////

    static xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPIX> imgInput(in_img.rows, in_img.cols);
    static xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPIX> imgOutput(in_img.rows, in_img.cols);

    imgInput.copyTo(in_img.data);

    inrange_accel(imgInput, lower_thresh, upper_thresh, imgOutput);

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
