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
#include "xf_ispstats_config.h"
#include "iostream"

using namespace std;
int main(int argc, char** argv) {
    if (argc != 3) { // BGR and BAYER can be run without changing Makefile
        fprintf(stderr, "Usage: %s <INPUT IMAGE PATH 1> <INPUT IMAGE PATH 2> \n", argv[0]);
        return EXIT_FAILURE;
    }

    cv::Mat in_img;

    unsigned int final_bins[FINAL_BINS_NUM] = {63, 127, 191, 255};

    // This means the
    // merged_bin[0] = bin[  0 to  63]
    // merged_bin[1] = bin[ 64 to 127]
    // merged_bin[2] = bin[127 to 191]
    // merged_bin[3] = bin[192 to 255]

    // Set ROI
    int roi_tlx = 0;
    int roi_tly = 0;
    int roi_brx = 127;
    int roi_bry = 127;
    int stats_size = 256;

    int N = 2;
    int M = 2;

    if (N == 0) { // N = 0 or M = 0 is not possible so assigning it to 1 if value is 0.
        N = 1;
    }
    if (M == 0) {
        M = 1;
    }

    uint32_t ind_ref_stats[N * M][3][256] = {0};
    uint32_t mrg_ref_stats[N * M][3][FINAL_BINS_NUM] = {0};

    int zone_width = int((roi_brx - roi_tlx + 1) / N);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / M); // roi_height / M

#if BGR
    // Read image
    in_img = cv::imread(argv[1], 1);
    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[1]);
        return 0;
    }

    // Get histogram per ROI zones
    for (int row = 0; row < in_img.rows; row++) {
        for (int col = 0; col < in_img.cols; col++) {
            // Get data from in_img
            cv::Vec3b bgr = in_img.at<cv::Vec3b>(row, col);

            // Count if inside ROI
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                int zone_idx = (zone_row * N) + zone_col;

                // Extract per channel
                for (int ch = 0; ch < 3; ch++) {
                    uint8_t val = 0;
                    val = bgr[ch];

                    uint32_t tmpval = ind_ref_stats[zone_idx][ch][val];
                    ind_ref_stats[zone_idx][ch][val] = tmpval + 1;
                }
            }
        }
    }
#endif

#if BAYER
    in_img = cv::imread(argv[2], 0); // bayer_pattern, 0: Gray

    if (in_img.data == NULL) {
        fprintf(stderr, "Cannot open image %s\n", argv[2]);
        return 0;
    }

    uint8_t kr = 0, kb = 0, kg = 0;

    for (int row = 0; row < in_img.rows; row++) {
        for (int col = 0; col < in_img.cols; col = col + 2) {
            if (row % 2 == 0) // EVEN ROW
            {
                if ((row >= roi_tlx) && (row <= roi_brx) && (col >= roi_tly) && (col <= roi_bry)) {
                    int zone_col = int((row - roi_tlx) / zone_width);
                    int zone_row = int((col - roi_tly) / zone_height);
                    int zone_idx = (zone_row * N) + zone_col;

                    kr = in_img.at<uchar>(row, col);
                    ind_ref_stats[zone_idx][0][kr] = ind_ref_stats[zone_idx][0][kr] + 1;

                    kg = in_img.at<uchar>(row, col + 1);
                    ind_ref_stats[zone_idx][1][kg] = ind_ref_stats[zone_idx][1][kg] + 1;
                }
            } else // ODD ROW (i%2!=0)
            {
                if ((row >= roi_tlx) && (row <= roi_brx) && (col >= roi_tly) && (col <= roi_bry)) {
                    int zone_col = int((row - roi_tlx) / zone_width);
                    int zone_row = int((col - roi_tly) / zone_height);
                    int zone_idx = (zone_row * N) + zone_col;

                    kg = in_img.at<uchar>(row, col);
                    ind_ref_stats[zone_idx][1][kg] = ind_ref_stats[zone_idx][1][kg] + 1;

                    kb = in_img.at<uchar>(row, col + 1);
                    ind_ref_stats[zone_idx][2][kb] = ind_ref_stats[zone_idx][2][kb] + 1;
                }
            }
        }
    }
#endif

    int num_ch = 3;
    int num_zones = N * M;

#if MERGE_BINS
    for (int zone_idx = 0; zone_idx < num_zones; zone_idx++) {
        for (int ch = 0; ch < num_ch; ch++) {
            uint32_t hi_bin = final_bins[0];
            uint32_t bin_group = 0;

            for (int i = 0; i < stats_size; i++) {
                uint32_t tmp_val = ind_ref_stats[zone_idx][ch][i];

                mrg_ref_stats[zone_idx][ch][bin_group] += tmp_val;

                if ((uint32_t)i == hi_bin) {
                    bin_group = bin_group + 1;
                    hi_bin = *(final_bins + bin_group);
                }
            }
        }
    }
#endif

    // Create a memory to hold HLS implementation output:
    unsigned int* stats_bins = (unsigned int*)malloc(stats_size * 3 * sizeof(unsigned int));
    unsigned int* hlsstats = (unsigned int*)malloc(stats_size * MAX_ROWS * MAX_COLS * 3 * sizeof(unsigned int));

    int rows = in_img.rows;
    int cols = in_img.cols;

    //////////////// Top function call ///////////////////////
    ispstats_accel((ap_uint<PTR_WIDTH>*)in_img.data, hlsstats, final_bins, rows, cols, roi_tlx, roi_tly, roi_brx,
                   roi_bry, N, M);

    FILE *out_hls, *out_ref;
    out_hls = fopen("out_hls.txt", "w");
    out_ref = fopen("out_ref.txt", "w");

#if MERGE_BINS
    stats_size = FINAL_BINS_NUM;
#else
    stats_size = 256;
#endif

    // Total number of bins for all channels
    int total_bins = stats_size * 3;

    // Checking of output vs reference
    for (int zone = 0; zone < N * M; zone++) {
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
            if ((bstat_ref != bstat_hls) || (gstat_ref != gstat_hls) || (gstat_ref != gstat_hls)) {
                fprintf(stderr, "\nTest Failed\n");
                return 1;
            }
        }
    }

    fclose(out_hls);
    fclose(out_ref);

    std::cout << "Test Passed " << std::endl;

    return 0;
}
