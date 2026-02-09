/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#include <vector>
#include "opencv2/opencv.hpp"
#include "common/xf_headers.hpp"
#include <stdlib.h>
#include <ap_int.h>
#include <math.h>
#include "common/xf_types.hpp"
#include "common/xf_infra.hpp"

void wgtd_subtract_ref(float wgts[4], cv::Mat input1, cv::Mat input2, cv::Mat& output) {
    for (int row = 0; row < input1.rows; row++) {
        for (int col = 0; col < input1.cols; col++) {
            int val1 = input1.at<CVTYPE>(row, col);
            int val2 = input2.at<CVTYPE>(row, col);

            if (XF_BAYER_PATTERN == XF_BAYER_GR) {
                if ((((row & 0x0001) == 0) && ((col & 0x0001) == 0)) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 1))) {        // G Pixel
                    val2 = val2 * wgts[0];                                     // G has medium level of reduced weight
                } else if ((((row & 0x0001) == 0) && ((col & 0x0001) == 1))) { // R Pixel
                    val2 = val2 * wgts[1];                                     // R has lowest level of reduced weight
                } else if (((((row - 1) % 4) == 0) && (((col) % 4) == 0)) ||
                           ((((row + 1) % 4) == 0) && (((col - 2) % 4) == 0))) { // B Pixel
                    val2 = val2 * wgts[2];                                       // B has low level of reduced weight
                } else if ((((((row - 1) % 4)) == 0) && (((col - 2) % 4) == 0)) ||
                           (((((row + 1) % 4)) == 0) && (((col) % 4) == 0))) { // Calculated B Pixel
                    val2 = val2 * wgts[3];                                     // B has highest level of reduced weight
                }
            }
            if (XF_BAYER_PATTERN == XF_BAYER_BG) {
                if ((((row & 0x0001) == 0) && ((col & 0x0001) == 1)) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 0))) {        // G Pixel
                    val2 = val2 * wgts[0];                                     // G has medium level of reduced weight
                } else if ((((row & 0x0001) == 1) && ((col & 0x0001) == 1))) { // R Pixel
                    val2 = val2 * wgts[1];                                     // R has lowest level of reduced weight
                } else if (((((row) % 4) == 0) && (((col) % 4) == 0)) ||
                           ((((row - 2) % 4) == 0) && (((col - 2) % 4) == 0))) { // B Pixel
                    val2 = val2 * wgts[2];                                       // B has low level of reduced weight
                } else if ((((((row) % 4)) == 0) && (((col - 2) % 4) == 0)) ||
                           (((((row - 2) % 4)) == 0) && (((col) % 4) == 0))) { // Calculated B Pixel
                    val2 = val2 * wgts[3];                                     // B has highest level of reduced weight
                }
            }
            int val = val1 - val2;

            if (val < 0) {
                val = 0;
            }
            output.at<CVTYPE>(row, col) = (CVTYPE)val;
        }
    }
}

CVTYPE bilinear_interp(CVTYPE val0, CVTYPE val1, CVTYPE val2, CVTYPE val3) {
    int ret = 0.25 * (val0 + val1 + val2 + val3);

    return (CVTYPE)ret;
}

void apply_bilinear_ref(CVTYPE patch[3][3], CVTYPE& out) {
    CVTYPE res = bilinear_interp(patch[0][1], patch[1][0], patch[1][2], patch[2][1]);

    out = res;
}

void ir_bilinear_ref(cv::Mat half_ir, cv::Mat& full_ir) {
    CVTYPE block_half_ir[3][3];

    for (int row = 0; row < half_ir.rows; row++) {
        for (int col = 0; col < half_ir.cols; col++) {
            for (int k = -1, ki = 0; k < 2; k++, ki++) {
                for (int l = -1, li = 0; l < 2; l++, li++) {
                    if (row + k >= 0 && row + k < half_ir.rows && col + l >= 0 && col + l < half_ir.cols) {
                        block_half_ir[ki][li] = (CVTYPE)half_ir.at<CVTYPE>(row + k, col + l);
                    } else {
                        block_half_ir[ki][li] = 0;
                    }
                }
            }

            CVTYPE out_pix = block_half_ir[1][1];
            if ((XF_BAYER_PATTERN == XF_BAYER_BG) || (XF_BAYER_PATTERN == XF_BAYER_RG)) {
                if ((((row & 0x0001) == 0) && (col & 0x0001) == 1) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 0))) // BG, RG Mode - Even row, odd column and
                {                                                     //				odd row, even column
                    apply_bilinear_ref(block_half_ir, out_pix);
                }
                full_ir.at<CVTYPE>(row, col) = out_pix;
            } else if ((XF_BAYER_PATTERN == XF_BAYER_GR)) {
                if ((((row & 0x0001) == 0) && ((col & 0x0001) == 0)) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 1))) // GR Mode - Even row, odd column
                {
                    apply_bilinear_ref(block_half_ir, out_pix);
                }
                full_ir.at<CVTYPE>(row, col) = out_pix;
            } else {
                if ((((row & 0x0001) == 0) && (col & 0x0001) == 0) ||
                    (((row & 0x0001) == 1) && ((col & 0x0001) == 1))) // GB Mode - Even row, even column and
                {                                                     //				odd row, odd column
                    apply_bilinear_ref(block_half_ir, out_pix);
                }
                full_ir.at<CVTYPE>(row, col) = out_pix;
            }
        }
    }
}

template <int FROWS, int FCOLS>
void apply_filter_ref(CVTYPE patch[FROWS][FCOLS], float weights[FROWS][FCOLS], CVTYPE& out) {
    float out_pix = 0;
    for (int fh = 0; fh < FROWS; fh++) {
        for (int fw = 0; fw < FCOLS; fw++) {
            out_pix += (patch[fh][fw] * weights[fh][fw]);
        }
    }
    if (out_pix < 0) {
        out_pix = 0;
    }
    out = (CVTYPE)out_pix;
}

void rgb_ir_ref(cv::Mat in, cv::Mat& output_image, cv::Mat& output_image_ir) {
    CVTYPE block_rgb[5][5];
    CVTYPE block_ir[3][3];

    float R_IR_C1_wgts[5][5] = {{-1.0 / 32.0f, -1.0 / 32.0f, 0, -1.0 / 32.0f, -1.0 / 32.0f},
                                {0, 1.0 / 2.0f, -1.0 / 16.0f, -1.0 / 4.0f, 0},
                                {0, -1.0 / 32.0f, -1.0 / 16.0f, -1.0 / 32.0f, 0},
                                {0, -1.0 / 4.0f, -1.0 / 16.0f, 1.0 / 2.0f, 0},
                                {-1.0 / 32.0f, -1.0 / 32.0f, 0, -1.0 / 32.0f, -1.0 / 32.0f}};
    float R_IR_C2_wgts[5][5] = {{-1.0 / 32.0f, -1.0 / 32.0f, 0, -1.0 / 32.0f, -1.0 / 32.0f},
                                {0, -1.0 / 4.0f, -1.0 / 16.0f, 1.0 / 2.0f, 0},
                                {0, -1.0 / 32.0f, -1.0 / 16.0f, -1.0 / 32.0f, 0},
                                {0, 1.0 / 2.0f, -1.0 / 16.0f, -1.0 / 4.0f, 0},
                                {-1.0 / 32.0f, -1.0 / 32.0f, 0, -1.0 / 32.0f, -1.0 / 32.0f}};
    float B_at_R_wgts[5][5] = {{1.0 / 8.0f, 0, -1.0 / 8.0f, 0, 1.0 / 8.0f},
                               {0, 0, 0, 0, 0},
                               {1.0 / 8.0f, 0, -1.0 / 2.0f, 0, 1.0 / 8.0f},
                               {0, 0, 0, 0, 0},
                               {1.0 / 8.0f, 0, -1.0 / 8.0f, 0, 1.0 / 8.0f}};
    float IR_at_R_wgts[3][3] = {{1.0 / 4.0f, 0, 1.0 / 4.0f}, {0, -1.0 / 16.0f, 0}, {1.0 / 4.0f, 0, 1.0 / 4.0f}};
    float IR_at_B_wgts[3][3] = {{1.0 / 4.0f, 0, 1.0 / 4.0f}, {0, -1.0 / 16.0f, 0}, {1.0 / 4.0f, 0, 1.0 / 4.0f}};

    int a = 0;
    // Extracting a 5x5 block from input image
    for (int row = 0; row < in.rows; row++) {
        for (int col = 0; col < in.cols; col++) {
            for (int k = -2, ki = 0; k < 3; k++, ki++) {
                for (int l = -2, li = 0; l < 3; l++, li++) {
                    if (row + k >= 0 && row + k < in.rows && col + l >= 0 && col + l < in.cols) {
                        block_rgb[ki][li] = (CVTYPE)in.at<CVTYPE>(row + k, col + l);
                    } else {
                        block_rgb[ki][li] = 0;
                    }
                }
            }
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    block_ir[i][j] = block_rgb[i + 1][j + 1];
                }
            }

            CVTYPE out_pix = block_rgb[2][2], out_pix_ir = block_ir[1][1];

            if (XF_BAYER_PATTERN == XF_BAYER_BG) {
                if (((((row - 2) % 4) == 0) && (((col) % 4) == 0)) ||
                    ((((row) % 4) == 0) &&
                     (((col - 2) % 4) == 0))) // BG Mode - This is even row, R location. Compute B here with 5x5 filter
                {
                    apply_filter_ref<5, 5>(block_rgb, B_at_R_wgts, out_pix); // B at R
                }

                else if (((row - 1) % 4) == 0) // BG Mode - This is odd row, odd column,
                // hence IR location. Compute R here with 5x5
                // filter
                {
                    if (((col - 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C2_wgts,
                                               out_pix); // R at IR - Constellation-2 (Red on the left)

                    } else if (((col + 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C1_wgts,
                                               out_pix); // R at IR - Constellation-1 (Blue on the left)
                    }
                } else if (((row + 1) % 4) == 0) {
                    if (((col - 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C1_wgts,
                                               out_pix); // R at IR - Constellation-1 (Blue on the left)
                    } else if (((col + 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C2_wgts,
                                               out_pix); // R at IR - Constellation-2 (Red on the left)
                    }
                }

                // IR-calculations
                if ((((row % 4) == 0) && (((col) % 4) == 0)) ||
                    ((((row - 2) % 4) == 0) &&
                     (((col - 2) % 4) == 0))) // BG Mode - This is even row, R location. compute IR with 3x3 filter
                {
                    apply_filter_ref<3, 3>(block_ir, IR_at_B_wgts, out_pix_ir); // IR at R
                } else if (((((row - 2) % 4) == 0) && (((col) % 4) == 0)) ||
                           ((((row) % 4) == 0) &&
                            (((col - 2) % 4) ==
                             0))) { // BG Mode - Even row, even column, B location, apply 3x3 IR filter

                    apply_filter_ref<3, 3>(block_ir, IR_at_R_wgts, out_pix_ir); // IR at B location
                }
            } else if (XF_BAYER_PATTERN == XF_BAYER_GR) { // For GR format

                if ((((row - 1) % 4 == 0) &&
                     ((col - 2) % 4 == 0)) || // GR Mode - This is R location. Compute B here with 5x5 filter
                    (((row + 1) % 4 == 0) && (col % 4) == 0)) {
                    // Apply the filter on the NxM_src_blk
                    apply_filter_ref<5, 5>(block_rgb, B_at_R_wgts, out_pix); // R at B
                }

                else if (((row - 2) % 4) == 0) // BG Mode - This is odd row, odd column,
                // hence IR location. Compute R here with 5x5
                // filter
                {
                    if (((col - 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C2_wgts,
                                               out_pix); // R at IR - Constellation-2 (Red on the left)

                    } else if (((col + 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C1_wgts,
                                               out_pix); // R at IR - Constellation-1 (Blue on the left)
                    }
                } else if (((row) % 4) == 0) {
                    if (((col - 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C1_wgts,
                                               out_pix); // R at IR - Constellation-1 (Blue on the left)
                    } else if (((col + 1) % 4) == 0) {
                        apply_filter_ref<5, 5>(block_rgb, R_IR_C2_wgts,
                                               out_pix); // R at IR - Constellation-2 (Red on the left)
                    }
                }

                // IR-calculations
                if (((((row - 1) % 4) == 0) &&
                     (((col - 2) % 4) == 0)) || // GR Mode - Odd row, R location. Apply 3x3 IR filter
                    ((((row + 1) % 4) == 0) && ((col % 4) == 0))) {             //- Next Odd row, R location
                    apply_filter_ref<3, 3>(block_ir, IR_at_R_wgts, out_pix_ir); // Calculating IR at R location
                } else if (((((row - 1) % 4) == 0) &&
                            ((col % 4) == 0)) || // GR Mode - Odd row, B location, apply 3x3 IR filter
                           ((((row + 1) % 4) == 0) && (((col - 2) % 4) == 0))) { //- Next Odd row, B location
                    apply_filter_ref<3, 3>(block_ir, IR_at_B_wgts, out_pix_ir);  // Calculating IR at B location
                }
            }
            output_image.at<CVTYPE>(row, col) = out_pix;
            output_image_ir.at<CVTYPE>(row, col) = out_pix_ir;
        }
    }
    cv::imwrite("WithIR_RGGB_ref.png", output_image);
    cv::imwrite("Half_ir_ref.png", output_image_ir);
}

void ref_rgb_ir(cv::Mat in, cv::Mat& rggb_output_image, cv::Mat& output_image_ir, int in_rows, int in_cols) {
    cv::Mat rggb_out_ref, half_ir_out_ref;

    rggb_out_ref.create(in_rows, in_cols, CV_IN_TYPE);
    half_ir_out_ref.create(in_rows, in_cols, CV_IN_TYPE);
    float wgts[4] = {0.125, 0.5, 0.25, 0.03125}; // GR

    rgb_ir_ref(in, rggb_out_ref, half_ir_out_ref);
    ir_bilinear_ref(half_ir_out_ref, output_image_ir);
    // subtract_ref(rggb_out_ref, output_image_ir, rggb_output_image);
    wgtd_subtract_ref(wgts, rggb_out_ref, output_image_ir, rggb_output_image);

#ifndef __SYNTHESIS
#ifdef __DEBUG

    FILE* fp1 = fopen("rggb_with_ir_ref.csv", "w");
    for (int i = 0; i < in_rows; ++i) {
        for (int j = 0; j < in_cols; ++j) {
            CVTYPE val = rggb_out_ref.at<CVTYPE>(i, j);
            fprintf(fp1, "%d ", val);
        }
        fprintf(fp1, "\n");
    }
    fclose(fp1);

    FILE* fp4 = fopen("half_ir_out_ref.csv", "w");
    for (int i = 0; i < in_rows; ++i) {
        for (int j = 0; j < in_cols; ++j) {
            CVTYPE val = half_ir_out_ref.at<CVTYPE>(i, j);
            fprintf(fp4, "%d ", val);
        }
        fprintf(fp4, "\n");
    }
    fclose(fp4);

    FILE* fp5 = fopen("sub_out_img_ref.csv", "w");
    for (int i = 0; i < in_rows; ++i) {
        for (int j = 0; j < in_cols; ++j) {
            CVTYPE val = rggb_output_image.at<CVTYPE>(i, j);
            fprintf(fp5, "%d ", val);
        }
        fprintf(fp5, "\n");
    }
    fclose(fp5);

    FILE* fpO1 = fopen("output_image_ir.csv", "w");
    for (int i = 0; i < in_rows; ++i) {
        for (int j = 0; j < in_cols; ++j) {
            CVTYPE val = output_image_ir.at<CVTYPE>(i, j);
            fprintf(fpO1, "%d ", val);
        }
        fprintf(fpO1, "\n");
    }
    fclose(fpO1);
#endif
#endif
}