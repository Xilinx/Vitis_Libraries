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

#include "common/xf_headers.hpp"
#include "xf_ispstats_tb_config.h"
#include <iostream>
using namespace std;
int main(int argc, char** argv) {
    if (argc != 3) { // BGR, BAYER and GRAY can be run without changing Makefile
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2> \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img;

#if T_8U

    ap_uint<13> final_bins[FINAL_BINS_NUM] = {63, 127, 191, 255};
// This means the
// merged_bin[0] = bin[  0 to  63]
// merged_bin[1] = bin[ 64 to 127]
// merged_bin[2] = bin[127 to 191]
// merged_bin[3] = bin[192 to 255]
#else
    ap_uint<13> final_bins[FINAL_BINS_NUM] = {1023, 2047, 3069, 4095};
// This means the
// merged_bin[0] = bin[  0 to  1023]
// merged_bin[1] = bin[ 64 to 2047]
// merged_bin[2] = bin[127 to 3069]
// merged_bin[3] = bin[192 to 4095]
#endif

    int roi_tlx = 0;
    int roi_tly = 0;
    int roi_brx = 127;
    int roi_bry = 127;
    int stats_size = STATS_SIZE;
    const int N = 8;
    const int M = 8;

    uint32_t ind_ref_stats[N * M][3][STATS_SIZE] = {0};
    uint32_t mrg_ref_stats[N * M][3][FINAL_BINS_NUM] = {0};

    uint32_t mrg_ref_stats_new[N * M][3][FINAL_BINS_NUM] = {0};

    int zone_width = int((roi_brx - roi_tlx + 1) / N);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / M); // roi_height / M

#if T_8U

    float inputMin = 0.0f;
    float inputMax = 255.0f;
    float outputMin = 0.0f;
    float outputMax = 255.0f;

#else

    float inputMin = 0.0f;
    float inputMax = 65535.0f;
    float outputMin = 0.0f;
    float outputMax = 65535.0f;

#endif

    ap_uint<13> stats_bins = STATS_SIZE;
    float min_vals = inputMin - 0.5f;
    float max_vals = inputMax + 0.5f;

    float minValue = min_vals, minValue1 = min_vals;
    float maxValue = max_vals, maxValue1 = max_vals;

    float interval = float(maxValue - minValue) / stats_bins;

    float internal_inv = ((float)1 / interval);

#if BGR

#if T_8U
    in_img = cv::imread(argv[1], 1);
#else
    in_img = cv::imread(argv[1], -1);
#endif
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return 0;
    }

    int currentBin = 0;

    // Get histogram per ROI zones
    for (int row = 0; row < in_img.rows; row++) {
        for (int col = 0; col < in_img.cols; col++) {
// Get data from in_img
#if T_8U
            cv::Vec3b bgr = in_img.at<cv::Vec3b>(row, col);
#else
            cv::Vec3w bgr = in_img.at<cv::Vec3w>(row, col);
#endif
            // Count if inside ROI
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                int zone_idx = (zone_row * N) + zone_col;

                // Extract per channel
                for (int ch = 0; ch < 3; ch++) {
                    uint32_t val = 0;
                    val = bgr[ch];
                    currentBin = int((val - minValue) * internal_inv);
                    uint32_t tmpval = ind_ref_stats[zone_idx][ch][currentBin];
                    ind_ref_stats[zone_idx][ch][currentBin] = tmpval + 1;
                }
            }
        }
    }

#endif

#if BAYER

#if T_8U
    in_img = cv::imread(argv[2], 0);
#else
    in_img = cv::imread(argv[2], -1); // bayer_pattern, 0: Gray
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[2]);
        return 0;
    }

    uint32_t k = 0;

    int currentBin_bayer[3] = {0};

    int prev_row_id = 0;
    int row_id = 0;

    int zone_idx = 0; // = (zone_row * zone_col_num) + zone_col;
    int zone_idx_prev = 0;

    ap_uint<16> src_pix_val = 0;
    ap_uint<16> wr_stats_idx[3] = {0};

    uint32_t rd_stats_val[3] = {0};
    uint32_t wr_stats_val[3] = {0};

    ap_uint<2> ch = 0;
    ap_uint<2> ch_prev = 0;

STATS_ROW_LOOP:
    for (ap_uint<13> row = 0; row < in_img.rows; row++) {
    // clang-format off
    STATS_COL_LOOP:
        for (ap_uint<13> col = 0; col < (in_img.cols + 1); col++) {

            // Data always need to be read out from buffer
            // Read BGR, 8-bit per channel at once

            // Check if part of zone
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                zone_idx = (zone_row * N) + zone_col;


                if (row % 2 == 0 && col % 2 ==0){
                    ch = 2; //RED
                } else if (row % 2 == 0 && col % 2 !=0) {
                    ch = 1; //GREEN
                } else if (row % 2 != 0 && col % 2 ==0) {
                    ch = 1; //GREEN
                } else if(row % 2 != 0 && col % 2 !=0) {
                    ch = 0; //BLUE
                }

#if T_8U               
                k = in_img.at<uchar>(row, col);
#else
                k = in_img.at<ushort>(row, col);
#endif    
                currentBin_bayer[ch] = int((k - minValue) * internal_inv);
                
                // Read stats
                rd_stats_val[ch] = ind_ref_stats[zone_idx][ch][currentBin_bayer[ch]];
                // Write stats
                ind_ref_stats[zone_idx_prev][ch_prev][wr_stats_idx[ch_prev]] = wr_stats_val[ch_prev];

                wr_stats_val[ch] = rd_stats_val[ch] + 1;
                wr_stats_idx[ch] = currentBin_bayer[ch];
   
              
            } else if (col == in_img.cols) {
                // Write operation to BRAM at last column
                ind_ref_stats[zone_idx][ch][wr_stats_idx[ch]] = wr_stats_val[ch];
            }

            zone_idx_prev = zone_idx;
            ch_prev = ch;
 
        }
      
    }
#endif

#if GRAY

#if T_8U
    in_img = cv::imread(argv[2], 0); // bayer_pattern, -1: 16bit image
#else
    in_img = cv::imread(argv[2], -1);
#endif

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[2]);
        return 0;
    }

    int currentBin_aec = 0;
    uint32_t val=0;
    // Get histogram per ROI zones
    for (int row = 0; row < in_img.rows; row++) {
        for (int col = 0; col < in_img.cols; col++) {
            // Get data from in_img
#if T_8U
            val = in_img.at<uchar>(row, col);
#else
            val = in_img.at<ushort>(row, col);
#endif
            currentBin_aec = int((val - minValue) * internal_inv);
            // Count if inside ROI
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                int zone_idx = (zone_row * N) + zone_col;
                uint32_t tmpval = ind_ref_stats[zone_idx][0][currentBin_aec];
                ind_ref_stats[zone_idx][0][currentBin_aec] = tmpval + 1;
            }
        }
    }
#endif

    int out_ch = NUM_OUT_CH;
    int num_zones = N * M;

#if MERGE_BINS

    for (int zone_idx = 0; zone_idx < num_zones; zone_idx++) {
        for (int ch = 0; ch < out_ch; ch++) {
            ap_uint<13> hi_bin = final_bins[0];
            ap_uint<13> bin_group = 0;

            for (ap_uint<13> i = 0; i < stats_size; i++) {
                uint32_t tmp_val = ind_ref_stats[zone_idx][ch][i];

                mrg_ref_stats[zone_idx][ch][bin_group] += tmp_val;

                if (i == hi_bin) {
                    bin_group = bin_group + 1;
                    hi_bin = *(final_bins + bin_group);
                }
            }
        }
    }

#endif

    // Create a memory to hold HLS implementation output:
    unsigned int* hlsstats = (unsigned int*)malloc(stats_size * MAX_ROWS * MAX_COLS * NUM_OUT_CH * sizeof(unsigned int));

    int rows = in_img.rows;
    int cols = in_img.cols;
    std::cout << "Input image height : " << rows << std::endl;
    std::cout << "Input image width  : " << cols << std::endl;

    ispstats_accel((ap_uint<INPUT_PTR_WIDTH>*)in_img.data, hlsstats, final_bins, rows, cols, roi_tlx, roi_tly, roi_brx,
                   roi_bry, N, M, inputMin, inputMax, outputMin, outputMax);

    FILE *out_hls, *out_ref;
    out_hls = fopen("out_hls.txt", "w");
    out_ref = fopen("out_ref.txt", "w");

#if MERGE_BINS
    stats_size = FINAL_BINS_NUM;
#else
    stats_size = STATS_SIZE;
#endif

    // Total number of bins for all channels
    int total_bins = stats_size * out_ch;

    std::cout << "out_ch = " << out_ch << std::endl;
    std::cout << "stats_size = " << stats_size << std::endl;
    std::cout << "total_bins = " << total_bins << std::endl;

    if (out_ch == 3) {
        // Checking of output vs reference
        for (int zone = 0; zone < num_zones; zone++) {
            fprintf(out_ref, "zone %d\n", zone);
            fprintf(out_hls, "zone %d\n", zone);
            for (int cnt = 0; cnt < stats_size; cnt++) {
                uint32_t bstat_hls = hlsstats[(zone * total_bins) + cnt];
                uint32_t gstat_hls = hlsstats[(zone * total_bins) + cnt + stats_size];
                uint32_t rstat_hls = hlsstats[(zone * total_bins) + cnt + stats_size * 2];

#if MERGE_BINS
                uint32_t bstat_ref = mrg_ref_stats[zone][0][cnt];
                uint32_t gstat_ref = mrg_ref_stats[zone][1][cnt];
                uint32_t rstat_ref = mrg_ref_stats[zone][2][cnt];
#else
                uint32_t bstat_ref = ind_ref_stats[zone][0][cnt];
                uint32_t gstat_ref = ind_ref_stats[zone][1][cnt];
                uint32_t rstat_ref = ind_ref_stats[zone][2][cnt];
#endif

                // Reference
                fprintf(out_ref, "bin%03d %u %u %u\n", cnt, bstat_ref, gstat_ref, rstat_ref);
                // HLS Output
                fprintf(out_hls, "bin%03d %u %u %u\n", cnt, bstat_hls, gstat_hls, rstat_hls);

                // Data checker
                if ((bstat_ref != bstat_hls) || (gstat_ref != gstat_hls) || (rstat_ref != rstat_hls)) {
                    fprintf(stderr, "\nTest Failed\n");
                    return 1;
                }
            }
        }
    } else {
        for (int zone = 0; zone < num_zones; zone++) {
            fprintf(out_ref, "zone %d\n", zone);
            fprintf(out_hls, "zone %d\n", zone);
            for (int cnt = 0; cnt < stats_size; cnt++) {
                uint32_t stat_hls = hlsstats[(zone * total_bins) + cnt];
#if MERGE_BINS
                uint32_t stat_ref = mrg_ref_stats[zone][0][cnt];
#else
                uint32_t stat_ref = ind_ref_stats[zone][0][cnt];
#endif
                // Reference
                fprintf(out_ref, "bin%03d %u\n", cnt, stat_ref);
                // HLS Output
                fprintf(out_hls, "bin%03d %u\n", cnt, stat_hls);

                // Data checker
                if (stat_ref != stat_hls) {
                    fprintf(stderr, "\nTest Failed\n");
                    return 1;
                }
            }
        }
    }
    fclose(out_hls);
    fclose(out_ref);

    std::cout << "Test Passed " << std::endl;

    return 0;
}
