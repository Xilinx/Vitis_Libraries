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
#include <bitset>

void maxrgb_image_ref(
    cv::Mat& channelsB, cv::Mat& channelsG, cv::Mat& channelsR, cv::Mat& maxrgb_image, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int b_value = (int)channelsB.at<CVTYPE>(i, j);
            int g_value = (int)channelsG.at<CVTYPE>(i, j);
            int r_value = (int)channelsR.at<CVTYPE>(i, j);

            if (b_value >= g_value && b_value >= r_value)
                // printf("%d is the largest number.", b_value);
                maxrgb_image.at<CVTYPE>(i, j) = b_value;

            if (g_value >= b_value && g_value >= r_value)
                // printf("%d is the largest number.", g_value);
                maxrgb_image.at<CVTYPE>(i, j) = g_value;

            if (r_value >= b_value && r_value >= g_value)
                // printf("%d is the largest number.", r_value);
                maxrgb_image.at<CVTYPE>(i, j) = r_value;
        }
    }
}

void log_ref(cv::Mat& maxrgb_image, double** log_img, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (maxrgb_image.at<CVTYPE>(i, j) == 0) {
                log_img[i][j] = 0;
            } else {
                log_img[i][j] = (float)(std::log(maxrgb_image.at<CVTYPE>(i, j)));
            }
        }
    }
}

double bilinear_ref(double in1, double in2, float width, float off_set) {
    double op1 = in1;
    double op2 = (in2 - in1);
    double op3 = off_set / width;
    double op4 = op3 * op2;
    double ret = op1 + op4;
    return ret;
}

void interpolation_ref(cv::Mat& in_img,
                       int rows,
                       int cols,
                       int blk_height,
                       int blk_width,
                       int maxmin_img_row_size,
                       int maxmin_img_col_size,
                       double** max_image_block,
                       double** min_image_block,
                       int mVBlkSize[2],
                       int mHBlkSize[2],
                       int mHBlkCount,
                       int mVBlkCount,
                       double** interpolation_ref_max,
                       double** interpolation_ref_min) {
    double max_a[4], min_a[4];
    int clk_blk = 0, off_set = 0;
    int row_height;
    int col_width;
    int inter_row_count = 0;
    for (int row_blk = 0; row_blk <= mVBlkCount - 1; row_blk++) {
        if (row_blk == 0) {
            row_height = mVBlkSize[0];
        } else if (row_blk == (mVBlkCount - 1)) {
            row_height = mVBlkSize[0];
        } else {
            row_height = mVBlkSize[1];
        }

        for (int inter_rows = 0; inter_rows < row_height; inter_rows++) {
            for (int inter_cols = 0; inter_cols < cols; inter_cols++) {
                if (inter_cols < mHBlkSize[0]) {
                    col_width = mHBlkSize[0];
                } else if (inter_cols >= (cols - mHBlkSize[0])) {
                    col_width = mHBlkSize[0];
                } else {
                    col_width = mHBlkSize[1];
                }

                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        max_a[(m * 2) + n] = max_image_block[row_blk + m][clk_blk + n];
                        min_a[(m * 2) + n] = min_image_block[row_blk + m][clk_blk + n];
                    }
                }

                double max_x1 = bilinear_ref(max_a[0], max_a[1], col_width, off_set);
                double max_x2 = bilinear_ref(max_a[2], max_a[3], col_width, off_set);
                double max_y = bilinear_ref(max_x1, max_x2, row_height, inter_rows);

                interpolation_ref_max[inter_row_count][inter_cols] = max_y;

                double min_x1 = bilinear_ref(min_a[0], min_a[1], col_width, off_set);
                double min_x2 = bilinear_ref(min_a[2], min_a[3], col_width, off_set);
                double min_y = bilinear_ref(min_x1, min_x2, row_height, inter_rows);

                interpolation_ref_min[inter_row_count][inter_cols] = min_y;
                // std::cout<<max_y <<" "<<min_y<<std::endl;
                off_set++;
                if (off_set >= col_width) {
                    off_set = 0;
                    clk_blk++;
                }
            }
            inter_row_count++;
            clk_blk = 0;
        }
    }
}

void saturation_ref(double final, cv::Mat& c, int i, int j) {
    if (final < 0)
        c.at<unsigned char>(i, j) = 0;
    else if (final > 255)
        c.at<unsigned char>(i, j) = 255;
    else
        c.at<unsigned char>(i, j) = (unsigned char)final;
}

void tone_mapped_pixel_ref(cv::Mat& in_img,
                           int rows,
                           int cols,
                           double** interpolation_ref_max,
                           double** interpolation_ref_min,
                           cv::Mat& maxrgb_image,
                           double** inter_tone_ref,
                           cv::Mat& out_image_ref) {
    std::vector<cv::Mat> channels(3), channels_1(3);
    cv::split(in_img, channels);
    cv::split(out_image_ref, channels_1);
    cv::Mat c0, c1, c2;
    c0.create(rows, cols, CV_8UC1);
    c1.create(rows, cols, CV_8UC1);
    c2.create(rows, cols, CV_8UC1);

    FILE* fp3 = fopen("out_image_ref.csv", "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double in1 = (float)std::log(maxrgb_image.at<CVTYPE>(i, j) + std::numeric_limits<float>::epsilon());
            double N1 = (in1 - interpolation_ref_min[i][j]) * 255;
            double D1 =
                (interpolation_ref_max[i][j] - interpolation_ref_min[i][j]) + std::numeric_limits<float>::epsilon();
            inter_tone_ref[i][j] = (N1 / D1);

            double intensity_ratio;
            intensity_ratio = (double)(inter_tone_ref[i][j] /
                                       (double)(maxrgb_image.at<CVTYPE>(i, j) + std::numeric_limits<float>::epsilon()));

            double final0, final1, final2;
#if T_8U
            final0 = (double)(channels[0].at<CVTYPE>(i, j) * intensity_ratio);
            final1 = (double)(channels[1].at<CVTYPE>(i, j) * intensity_ratio);
            final2 = (double)(channels[2].at<CVTYPE>(i, j) * intensity_ratio);
#else
            final0 = (double)(channels[0].at<CVTYPE>(i, j) * intensity_ratio);
            final1 = (double)(channels[1].at<CVTYPE>(i, j) * intensity_ratio);
            final2 = (double)(channels[2].at<CVTYPE>(i, j) * intensity_ratio);
#endif

            // if (final0 < 0)
            //     c0.at<unsigned char>(i,j) = 0;
            // else if (final0 > 255)
            //     c0.at<unsigned char>(i,j) = 255;
            // else
            //     c0.at<unsigned char>(i,j) = (unsigned char)final0;

            // if (final1 < 0)
            //     c1.at<unsigned char>(i,j) = 0;
            // else if (final1 > 255)
            //     c1.at<unsigned char>(i,j) = 255;
            // else
            //     c1.at<unsigned char>(i,j) = (unsigned char)final1;

            // if (final2 < 0)
            //     c2.at<unsigned char>(i,j) = 0;
            // else if (final2 > 255)
            //     c2.at<unsigned char>(i,j) = 255;
            // else
            //     c2.at<unsigned char>(i,j) = (unsigned char)final2;

            saturation_ref(final0, c0, i, j);
            saturation_ref(final1, c1, i, j);
            saturation_ref(final2, c2, i, j);

#if T_8U
            channels_1[0].at<CVTYPE>(i, j) = c0.at<unsigned char>(i, j);
            channels_1[1].at<CVTYPE>(i, j) = c1.at<unsigned char>(i, j);
            channels_1[2].at<CVTYPE>(i, j) = c2.at<unsigned char>(i, j);
#else
            channels_1[0].at<unsigned char>(i, j) = c0.at<unsigned char>(i, j);
            channels_1[1].at<unsigned char>(i, j) = c1.at<unsigned char>(i, j);
            channels_1[2].at<unsigned char>(i, j) = c2.at<unsigned char>(i, j);
#endif

            fprintf(fp3, "(%d,%d %d %d %d)", i, j, c2.at<unsigned char>(i, j), c1.at<unsigned char>(i, j),
                    c0.at<unsigned char>(i, j));
        }
        fprintf(fp3, "\n");
    }
    fclose(fp3);

    // OpenCV reference:
    std::vector<cv::Mat> bgr_planes;

    bgr_planes.push_back(channels_1[0]);
    bgr_planes.push_back(channels_1[1]);
    bgr_planes.push_back(channels_1[2]);

    cv::merge(bgr_planes, out_image_ref);
}

void ltm_ref(cv::Mat& in_img, cv::Mat& ltm_ref_out, int rows, int cols, int blk_height, int blk_width) {
    std::vector<cv::Mat> channels(3);
    cv::split(in_img, channels);
#if T_8U
    cv::Mat maxrgb_image(in_img.rows, in_img.cols, CV_8UC1);
#else
    cv::Mat maxrgb_image(in_img.rows, in_img.cols, CV_16UC1);
#endif
    double* log_img[rows];
    for (int i = 0; i < rows; i++) log_img[i] = (double*)malloc(cols * sizeof(double));

    ///////////////reference function to collect the max value in the 3channels of the input image per
    /// pixel////////////////////

    maxrgb_image_ref(channels[0], channels[1], channels[2], maxrgb_image, rows, cols);
    cv::imwrite("maxrgb_image.png", maxrgb_image);
    FILE* fp3 = fopen("maxrgb_image.csv", "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            CVTYPE val = maxrgb_image.at<CVTYPE>(i, j);
            fprintf(fp3, "%d ", val);
        }
        fprintf(fp3, "\n");
    }
    fclose(fp3);

    //////////////reference function to change the maxrgb_image into log domine///////////////////////////
    log_ref(maxrgb_image, log_img, rows, cols);
    FILE* fp4 = fopen("log_img.csv", "w");
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double val = log_img[i][j];
            // log_ref_img[i][j] = log_img[i][j];
            fprintf(fp4, "(%d %d %f) ", i, j, val);
        }
        fprintf(fp4, "\n");
    }
    fclose(fp4);

    ////////////refernece function to collect the max and min log values in blocks which are divided into specific
    /// sizes////////

    int corner_row_blk_size = blk_height / 2;
    int corner_col_blk_size = blk_width / 2;

    const int maxmin_img_row_size =
        ((rows % blk_height) == 0) ? (rows / blk_height) + 1 : (((rows + blk_height - 1) / blk_height) + 1) + 1;
    const int maxmin_img_col_size =
        ((cols % blk_width) == 0) ? (cols / blk_width) + 1 : (((cols + blk_width - 1) / blk_width) + 1) + 1;

    double* max_image_block[maxmin_img_row_size];
    for (int i = 0; i < maxmin_img_row_size; i++)
        max_image_block[i] = (double*)malloc(maxmin_img_col_size * sizeof(double));

    double* min_image_block[maxmin_img_row_size];
    for (int i = 0; i < maxmin_img_row_size; i++)
        min_image_block[i] = (double*)malloc(maxmin_img_col_size * sizeof(double));

    for (int i = 0; i < maxmin_img_row_size; i++) {
        for (int j = 0; j < maxmin_img_col_size; j++) {
            min_image_block[i][j] = MAX_PIX;
            max_image_block[i][j] = 0.0;
        }
    }

    /////////////mid tile logic////////////////

    int mVBlkSize[2]; // 0 - First / Last, 1 - Rest
    int mHBlkSize[2]; // 0 - First / Last, 1 - Rest
    int mHBlkCount = 0;
    int mVBlkCount = 0;

    int mColsNPCAligned = cols;
    int blkColsLog2;
    int row_start = std::log2(BLOCK_WIDTH);
    int row_max_limit = std::log2(XF_WIDTH) + 1;
    for (blkColsLog2 = row_start; blkColsLog2 <= row_max_limit; blkColsLog2++) {
        if ((1 << blkColsLog2) >= blk_width) break;
    }
    int mBlkColsNPCAligned = 1 << blkColsLog2;
    mHBlkCount = ((mColsNPCAligned + (mBlkColsNPCAligned - 1)) >> blkColsLog2);
    int th = mHBlkCount << blkColsLog2;
    mHBlkSize[0] = mBlkColsNPCAligned;
    mHBlkSize[1] = mBlkColsNPCAligned;
    unsigned int temp1, t;
    if (th > mColsNPCAligned) {
        temp1 = mColsNPCAligned >> blkColsLog2;
        t = mColsNPCAligned - (temp1 << blkColsLog2);
        mHBlkSize[0] = (t >> 1);
        mHBlkCount++;
    }

    int blkRowsLog2;
    int row_start_1 = std::log2(BLOCK_HEIGHT);
    int row_max_limit_1 = std::log2(XF_HEIGHT) + 1;
    for (blkRowsLog2 = row_start_1; blkRowsLog2 <= row_max_limit_1; blkRowsLog2++) {
        if ((1 << blkRowsLog2) >= blk_height) break;
    }
    mVBlkCount = ((rows + (blk_height - 1)) >> blkRowsLog2);
    int tv = (mVBlkCount << blkRowsLog2);
    mVBlkSize[0] = blk_height;
    mVBlkSize[1] = blk_height;
    if (rows < tv) {
        // In case mRows is not perfectly divisible by VBlk count then add 2 boundary blocks
        int t = rows - ((rows >> blkRowsLog2) << blkRowsLog2);
        mVBlkSize[0] = (t >> 1);
        mVBlkCount++;
    }
    std::cout << "mVBlkCount = " << mVBlkCount << std::endl;
    std::cout << "maxmin_img_col_size = " << maxmin_img_col_size << std::endl;

    int row_input_count, col_input_count;
    int hblksize;
    int offset = 0;
    row_input_count = 0;
    double omin_l[maxmin_img_col_size], omax_l[maxmin_img_col_size];
    for (int k = 0; k <= mVBlkCount; k++) {
        bool bStart = true;
        int row_block_size;

        if (k == 0) {
            row_block_size = (mVBlkSize[0] >> 1);
        } else if (k == mVBlkCount) {
            row_block_size = (mVBlkSize[0] >> 1);
        } else if (k == 1) {
            row_block_size = (mVBlkSize[0] + mVBlkSize[1]) >> 1;
        } else if (k == (mVBlkCount - 1)) {
            row_block_size = (mVBlkSize[0] + mVBlkSize[1]) >> 1;
        } else if (k < (mVBlkCount - 1)) {
            row_block_size = mVBlkSize[1];
        }

        int row_mid_tile_size = k;
        int col_mid_tile_size = 0;

        for (int i = 0; i < row_block_size; i++) {
            col_mid_tile_size = 0;
            col_input_count = 0;
            offset = 0;
            for (int j = 0; j < cols; j++) {
                if (j < mHBlkSize[0]) {
                    hblksize = mHBlkSize[0];
                } else if (j >= (cols - mHBlkSize[0])) {
                    hblksize = mHBlkSize[0];
                } else {
                    hblksize = mHBlkSize[1];
                }

                double pinValMin = MAX_PIX;
                double pinValMax = 0.0;
                double val = log_img[row_input_count][col_input_count];

                if (pinValMax < val) {
                    pinValMax = val;
                }

                if (pinValMin > val) {
                    pinValMin = val;
                }

                double min = omin_l[col_mid_tile_size];
                double max = omax_l[col_mid_tile_size];

                if (bStart) {
                    min = pinValMin;
                    max = pinValMin;
                } else {
                    min = (pinValMin < min) ? pinValMin : min;
                    max = (max < pinValMax) ? pinValMax : max;
                }
                offset++;
                bStart = false;
                if (offset == (hblksize / 2)) {
                    col_mid_tile_size++;
                    bStart = (i == 0);
                }
                if (offset == hblksize) offset = 0;

                omin_l[col_mid_tile_size] = min;
                omax_l[col_mid_tile_size] = max;

                max_image_block[row_mid_tile_size][col_mid_tile_size] = max;
                min_image_block[row_mid_tile_size][col_mid_tile_size] = min;
                col_input_count++;
            }
            row_input_count++;
        }
    }

    FILE* fp5 = fopen("max_image_block.csv", "w");
    FILE* fp6 = fopen("min_image_block.csv", "w");
    for (int i = 0; i < maxmin_img_row_size; i++) {
        for (int j = 0; j < maxmin_img_col_size; j++) {
            double max_value = max_image_block[i][j];
            double min_value = min_image_block[i][j];
            fprintf(fp5, "%f ", max_value);
            fprintf(fp6, "%f ", min_value);
        }
        fprintf(fp5, "\n");
        fprintf(fp6, "\n");
    }
    fclose(fp5);
    fclose(fp6);

    // /////////////////////interpolation to expand max and min matrices to input frame size//////////////

    double* interpolation_ref_max[rows];
    for (int i = 0; i < rows; i++) interpolation_ref_max[i] = (double*)malloc(cols * sizeof(double));

    double* interpolation_ref_min[rows];
    for (int i = 0; i < rows; i++) interpolation_ref_min[i] = (double*)malloc(cols * sizeof(double));

    interpolation_ref(in_img, rows, cols, blk_height, blk_width, maxmin_img_row_size, maxmin_img_col_size,
                      max_image_block, min_image_block, mVBlkSize, mHBlkSize, mHBlkCount, mVBlkCount,
                      interpolation_ref_max, interpolation_ref_min);

    FILE* fp7 = fopen("interpolation_ref_max.csv", "w");
    FILE* fp8 = fopen("interpolation_ref_min.csv", "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double max_value = interpolation_ref_max[i][j];
            double min_value = interpolation_ref_min[i][j];
            fprintf(fp7, "%f ", max_value);
            fprintf(fp8, "%f ", min_value);
        }
        fprintf(fp7, "\n");
        fprintf(fp8, "\n");
    }
    fclose(fp7);
    fclose(fp8);

    // ////////////////Intermediate tone-mapped pixel reference function////////

    double* inter_tone_ref[rows];
    for (int i = 0; i < rows; i++) inter_tone_ref[i] = (double*)malloc(cols * sizeof(double));

    tone_mapped_pixel_ref(in_img, rows, cols, interpolation_ref_max, interpolation_ref_min, maxrgb_image,
                          inter_tone_ref, ltm_ref_out);

    FILE* fp9 = fopen("inter_tone_ref.csv", "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double max_value = inter_tone_ref[i][j];
            fprintf(fp9, "%f ", max_value);
        }
        fprintf(fp9, "\n");
    }
    fclose(fp9);

    cv::imwrite("ltm_ref_out.jpg", ltm_ref_out);
}
