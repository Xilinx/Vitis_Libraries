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
#include "xf_lensshading_config.h"
#include <math.h>

void bayerizeImage(cv::Mat in_img, cv::Mat& bayer_image, cv::Mat& cfa_output, int code) {
    for (int i = 0; i < in_img.rows; i++) {
        for (int j = 0; j < in_img.cols; j++) {
#if T_8U
            cv::Vec3b in = in_img.at<cv::Vec3b>(i, j);
            cv::Vec3b b;
#else
            cv::Vec3w in = in_img.at<cv::Vec3w>(i, j);
            cv::Vec3w b;
#endif
            b[0] = 0;
            b[1] = 0;
            b[2] = 0;

            if (code == 0) {            // BG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                }
            }
            if (code == 1) {            // GB
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 2) {            // GR
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                }
            }
            if (code == 3) {            // RG
                if ((i & 1) == 0) {     // even row
                    if ((j & 1) == 0) { // even col
                        b[2] = in[2];
                        cfa_output.at<pxltype>(i, j) = in[2];
                    } else { // odd col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    }
                } else {                // odd row
                    if ((j & 1) == 0) { // even col
                        b[1] = in[1];
                        cfa_output.at<pxltype>(i, j) = in[1];
                    } else { // odd col
                        b[0] = in[0];
                        cfa_output.at<pxltype>(i, j) = in[0];
                    }
                }
            }
#if T_8U
            bayer_image.at<cv::Vec3b>(i, j) = b;
#else
            bayer_image.at<cv::Vec3w>(i, j) = b;
#endif
        }
    }
}
// OpenCV reference function:
void LSC_ref(cv::Mat& _src, cv::Mat& _dst) {
    int center_pixel_pos_x = (_src.cols / 2);
    int center_pixel_pos_y = (_src.rows / 2);
    float max_distance = std::sqrt((_src.rows - center_pixel_pos_y) * (_src.rows - center_pixel_pos_y) +
                                   (_src.cols - center_pixel_pos_x) * (_src.cols - center_pixel_pos_x));

    for (int i = 0; i < _src.rows; i++) {
        for (int j = 0; j < _src.cols; j++) {
            float distance = std::sqrt((center_pixel_pos_y - i) * (center_pixel_pos_y - i) +
                                       (center_pixel_pos_x - j) * (center_pixel_pos_x - j)) /
                             max_distance;

            float gain = (0.01759 * ((distance + 28.37) * (distance + 28.37))) - 13.36;
#if T_8U
            int value = (_src.at<unsigned char>(i, j) * gain);
            if (value > 255) {
                value = 255;
            }
            _dst.at<unsigned char>(i, j) = (unsigned char)value;
#else
            int value = (_src.at<unsigned short>(i, j) * gain);
            if (value > 65535) {
                value = 65535;
            }
            _dst.at<unsigned short>(i, j) = (unsigned short)value;

#endif
        }
    }
}

int main(int argc, char** argv) {
    cv::Mat in_img, out_img, out_img_hls, diff, cfa_bayer_output;

#if T_8U
    in_img = cv::imread(argv[1], 1); // read image
#else
    in_img = cv::imread(argv[1], -1); // read image
#endif
    if (!in_img.data) {
        return -1;
    }

    imwrite("in_img.png", in_img);

#if T_8U
    out_img.create(in_img.rows, in_img.cols, CV_8UC1);
    out_img_hls.create(in_img.rows, in_img.cols, CV_8UC1);
    diff.create(in_img.rows, in_img.cols, CV_8UC1);
    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_8UC1);
#else
    out_img.create(in_img.rows, in_img.cols, CV_16UC1);
    out_img_hls.create(in_img.rows, in_img.cols, CV_16UC1);
    diff.create(in_img.rows, in_img.cols, CV_16UC1);
    cfa_bayer_output.create(in_img.rows, in_img.cols, CV_16UC1);
#endif

    int height = in_img.rows;
    int width = in_img.cols;

    cv::Mat color_cfa_bayer_output(in_img.rows, in_img.cols, in_img.type()); // Bayer pattern CFA output in color
    unsigned short bformat = BPATTERN;                                       // Bayer format BG-0; GB-1; GR-2; RG-3

    bayerizeImage(in_img, color_cfa_bayer_output, cfa_bayer_output, bformat);
    cv::imwrite("bayer_image.png", color_cfa_bayer_output);
    cv::imwrite("cfa_output.png", cfa_bayer_output);

    LSC_ref(cfa_bayer_output, out_img);

    lensshading_accel((ap_uint<INPUT_PTR_WIDTH>*)cfa_bayer_output.data, (ap_uint<OUTPUT_PTR_WIDTH>*)out_img_hls.data,
                      height, width);

    // Write output image
    cv::imwrite("hls_out.jpg", out_img_hls);
    cv::imwrite("ocv_out.jpg", out_img);

    // Compute absolute difference image
    cv::absdiff(out_img_hls, out_img, diff);
    // Save the difference image for debugging purpose:
    cv::imwrite("error.png", diff);
    float err_per;
    xf::cv::analyzeDiff(diff, 1, err_per);

    if (err_per > 1) {
        std::cerr << "ERROR: Test Failed." << std::endl;
        return 1;
    } else
        std::cout << "Test Passed " << std::endl;
    return 0;
}
