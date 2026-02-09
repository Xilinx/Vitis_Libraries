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

void maxrgb_image_ref(
    cv::Mat& channelsB, cv::Mat& channelsG, cv::Mat& channelsR, cv::Mat& maxrgb_image, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int b_value = (int)channelsB.at<unsigned char>(i, j);
            int g_value = (int)channelsG.at<unsigned char>(i, j);
            int r_value = (int)channelsR.at<unsigned char>(i, j);

            if (b_value >= g_value && b_value >= r_value)
                // printf("%d is the largest number.", b_value);
                maxrgb_image.at<unsigned char>(i, j) = b_value;

            if (g_value >= b_value && g_value >= r_value)
                // printf("%d is the largest number.", g_value);
                maxrgb_image.at<unsigned char>(i, j) = g_value;

            if (r_value >= b_value && r_value >= g_value)
                // printf("%d is the largest number.", r_value);
                maxrgb_image.at<unsigned char>(i, j) = r_value;
        }
    }
}

void log_ref(cv::Mat& maxrgb_image, double** log_img, int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            log_img[i][j] = (double)(std::log(maxrgb_image.at<unsigned char>(i, j)));
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
                       double** interpolation_ref_max,
                       double** interpolation_ref_min) {
    double max_a[4], min_a[4];
    int clk_blk = 0, off_set = 0;
    for (int row_blk = 0; row_blk < maxmin_img_row_size - 1; row_blk++) {
        for (int inter_rows = 0; inter_rows < blk_height; inter_rows++) {
            for (int inter_cols = 0; inter_cols < cols; inter_cols++) {
                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        max_a[(m * 2) + n] = max_image_block[row_blk + m][clk_blk + n];
                        min_a[(m * 2) + n] = min_image_block[row_blk + m][clk_blk + n];
                    }
                }

                double max_x1 = bilinear_ref(max_a[0], max_a[1], blk_width, off_set);
                double max_x2 = bilinear_ref(max_a[2], max_a[3], blk_width, off_set);
                double max_y = bilinear_ref(max_x1, max_x2, blk_height, inter_rows);

                interpolation_ref_max[(row_blk * blk_height) + inter_rows][inter_cols] = max_y;

                double min_x1 = bilinear_ref(min_a[0], min_a[1], blk_width, off_set);
                double min_x2 = bilinear_ref(min_a[2], min_a[3], blk_width, off_set);
                double min_y = bilinear_ref(min_x1, min_x2, blk_height, inter_rows);

                interpolation_ref_min[(row_blk * blk_height) + inter_rows][inter_cols] = min_y;
                // std::cout<<max_y <<" "<<min_y<<std::endl;
                off_set++;
                if (off_set >= blk_width) {
                    off_set = 0;
                    clk_blk++;
                }
            }
            clk_blk = 0;
        }
    }
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
    // cv::Mat channels(in_img.rows, in_img.cols, CV_8UC3);
    cv::split(in_img, channels);
    cv::split(out_image_ref, channels_1);
    int c0[rows][cols], c1[rows][cols], c2[rows][cols];

    FILE* fp3 = fopen("out_image_ref.csv", "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double in1 = log(maxrgb_image.at<unsigned char>(i, j));
            double N1 = (in1 - interpolation_ref_min[i][j]) * 255;
            double D1 = (interpolation_ref_max[i][j] - interpolation_ref_min[i][j]);
            inter_tone_ref[i][j] = (N1 / D1);
            double intensity_ratio = inter_tone_ref[i][j] / (maxrgb_image.at<unsigned char>(i, j));
            // c0[i][j] = channels[0].at<unsigned char>(i,j);
            // c1[i][j] = channels[1].at<unsigned char>(i,j);
            // c2[i][j] = channels[2].at<unsigned char>(i,j);

            c0[i][j] = (channels[0].at<unsigned char>(i, j) * intensity_ratio);
            c1[i][j] = (channels[1].at<unsigned char>(i, j) * intensity_ratio);
            c2[i][j] = (channels[2].at<unsigned char>(i, j) * intensity_ratio);

            if (c0[i][j] < 0)
                c0[i][j] = 0;
            else if (c0[i][j] > 255)
                c0[i][j] = 255;
            else
                c0[i][j] = c0[i][j];

            if (c1[i][j] < 0)
                c1[i][j] = 0;
            else if (c1[i][j] > 255)
                c1[i][j] = 255;
            else
                c1[i][j] = c1[i][j];

            if (c2[i][j] < 0)
                c2[i][j] = 0;
            else if (c2[i][j] > 255)
                c2[i][j] = 255;
            else
                c2[i][j] = c2[i][j];

            channels_1[0].at<unsigned char>(i, j) = c0[i][j];
            channels_1[1].at<unsigned char>(i, j) = c1[i][j];
            channels_1[2].at<unsigned char>(i, j) = c2[i][j];

            fprintf(fp3, "(%d %d %d)", c2[i][j], c1[i][j], c0[i][j]);
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

    // FILE* fp4 = fopen("out_image_ref.csv", "w");
    // for (int i = 0; i < rows; ++i) {
    //     for (int j = 0; j < cols; ++j) {
    //         fprintf(fp4, "(%d %d %d)",channels_1[0].at<unsigned char>(i,j),channels_1[1].at<unsigned
    //         char>(i,j),channels_1[2].at<unsigned char>(i,j) );
    //     }
    //     fprintf(fp4, "\n");
    // }
    // fclose(fp4);
}

void ltm_ref(cv::Mat& in_img, cv::Mat& ltm_ref_out, int rows, int cols, int blk_height, int blk_width) {
    std::vector<cv::Mat> channels(3);
    // cv::Mat channels(in_img.rows, in_img.cols, CV_8UC3);
    cv::split(in_img, channels);
    cv::Mat maxrgb_image(in_img.rows, in_img.cols, CV_8UC1);

    double* log_img[rows];
    for (int i = 0; i < rows; i++) log_img[i] = (double*)malloc(cols * sizeof(double));

    ///////////////reference function to collect the max value in the 3channels of the input image per
    /// pixel////////////////////

    maxrgb_image_ref(channels[0], channels[1], channels[2], maxrgb_image, rows, cols);
    cv::imwrite("maxrgb_image.png", maxrgb_image);
    FILE* fp3 = fopen("maxrgb_image.csv", "w");
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            unsigned char val = maxrgb_image.at<unsigned char>(i, j);
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

    const int maxmin_img_row_size = (rows / blk_height) + 1;
    const int maxmin_img_col_size = (cols / blk_width) + 1;

    double* max_image_block[maxmin_img_row_size];
    for (int i = 0; i < maxmin_img_row_size; i++)
        max_image_block[i] = (double*)malloc(maxmin_img_col_size * sizeof(double));

    double* min_image_block[maxmin_img_row_size];
    for (int i = 0; i < maxmin_img_row_size; i++)
        min_image_block[i] = (double*)malloc(maxmin_img_col_size * sizeof(double));

    // double max_image_block[maxmin_img_row_size][maxmin_img_col_size];
    // double min_image_block[maxmin_img_row_size][maxmin_img_col_size];

    for (int i = 0; i < maxmin_img_row_size; i++) {
        for (int j = 0; j < maxmin_img_col_size; j++) {
            min_image_block[i][j] = 255.0;
            max_image_block[i][j] = 0.0;
        }
    }

    /////////////mid tile logic////////////////

    int corner_row_blk_size = blk_height / 2;
    int corner_col_blk_size = blk_width / 2;
    int row_input_count, col_input_count;
    row_input_count = 0;
    for (int k = 0; k < maxmin_img_row_size; k++) {
        bool bStart = true;
        int row_block_size;
        if (k < 1 || (k == maxmin_img_row_size - 1)) row_block_size = blk_height / 2;
        if (k >= 1 && k < (maxmin_img_row_size - 1)) row_block_size = blk_height;
        int row_mid_tile_size = k;
        double omin_l[maxmin_img_col_size], omax_l[maxmin_img_col_size];
        for (int i = 0; i < row_block_size; i++) {
            int col_mid_tile_size = 0;
            col_input_count = 0;
            for (int j = 0, offset = 0; j < cols; j++) {
                int hblksize;

                if (j < (corner_col_blk_size) || j >= cols - (corner_col_blk_size)) hblksize = blk_width;
                if (j >= (corner_col_blk_size) && j < cols - (corner_col_blk_size)) hblksize = blk_width;

                double val;
                double pinValMin = 255.0;
                double pinValMax = 0.0;
                val = log_img[row_input_count][col_input_count];

                if (pinValMax < val) pinValMax = val;
                if (pinValMin > val) pinValMin = val;

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

    /////////////////////interpolation to expand max and min matrices to input frame size//////////////

    double* interpolation_ref_max[rows];
    for (int i = 0; i < rows; i++) interpolation_ref_max[i] = (double*)malloc(cols * sizeof(double));

    double* interpolation_ref_min[rows];
    for (int i = 0; i < rows; i++) interpolation_ref_min[i] = (double*)malloc(cols * sizeof(double));

    interpolation_ref(in_img, rows, cols, blk_height, blk_width, maxmin_img_row_size, maxmin_img_col_size,
                      max_image_block, min_image_block, interpolation_ref_max, interpolation_ref_min);

    FILE* fp7 = fopen("interpolation_ref_max.csv", "w");
    FILE* fp8 = fopen("interpolation_ref_min.csv", "w");
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            double max_value = interpolation_ref_max[i][j];
            double min_value = interpolation_ref_min[i][j];
            // fprintf(fp7, "(%d %d %f) ",i,j,max_value);
            // fprintf(fp8, "(%d %d %f) ",i,j,min_value);
            fprintf(fp7, "%f ", max_value);
            fprintf(fp8, "%f ", min_value);
        }
        fprintf(fp7, "\n");
        fprintf(fp8, "\n");
    }
    fclose(fp7);
    fclose(fp8);

    ////////////////Intermediate tone-mapped pixel reference function////////

    cv::Mat out_image_ref(in_img.rows, in_img.cols, CV_8UC3);
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

    cv::imwrite("out_image_ref.jpg", ltm_ref_out);
}
