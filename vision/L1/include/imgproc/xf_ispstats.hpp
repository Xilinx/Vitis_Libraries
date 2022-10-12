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

#ifndef _XF_ISPSTATS_HPP_
#define _XF_ISPSTATS_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "common/xf_common.hpp"
#include "hls_stream.h"

using namespace std;
namespace xf {
namespace cv {
template <int SRC_T,
          int ROWS,
          int COLS,
          int MAX_ZONES,
          int STATS_SIZE,
          int DEPTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int WORDWIDTH,
          int SRC_TC,
          int PLANES>
void xfSTATSKernel_bgr(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _src_mat,
                       uint32_t stats_array[MAX_ZONES][3][STATS_SIZE],
                       uint16_t& imgheight,
                       uint16_t& imgwidth,
                       uint16_t& roi_tlx,
                       uint16_t& roi_tly,
                       uint16_t& roi_brx,
                       uint16_t& roi_bry,
                       uint16_t& zone_col_num,
                       uint16_t& zone_row_num) {
    // Temporary array used while computing STATS
    uint32_t tmp_stats[MAX_ZONES][(PLANES << XF_BITSHIFT(NPC))][STATS_SIZE];
    uint32_t tmp_stats1[MAX_ZONES][(PLANES << XF_BITSHIFT(NPC))][STATS_SIZE];

    int num_bins = STATS_SIZE;
    int num_zones = zone_row_num * zone_col_num;

// clang-format off
#pragma HLS ARRAY_PARTITION variable=tmp_stats complete dim=2
#pragma HLS ARRAY_PARTITION variable=tmp_stats1 complete dim=2
    // clang-format on

    XF_SNAME(WORDWIDTH) in_buf, in_buf1;

STATS_INITIALIZE_LOOP:
    for (ap_uint<7> k = 0; k < num_zones; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_ZONES
#pragma HLS PIPELINE
        // clang-format on
        for (ap_uint<5> j = 0; j < ((1 << XF_BITSHIFT(NPC)) * PLANES); j++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (ap_uint<10> i = 0; i < STATS_SIZE; i++) { //
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=STATS_SIZE max=STATS_SIZE
                // clang-format on
                tmp_stats[k][j][i] = 0;
                tmp_stats1[k][j][i] = 0;
            }
        }
    }

    int zone_width = int((roi_brx - roi_tlx + 1) / zone_col_num);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / zone_row_num); // roi_height / M

STATS_ROW_LOOP:
    for (uint16_t row = 0; row < imgheight; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on
    STATS_COL_LOOP:
        for (uint16_t col = 0; col < (imgwidth); col = col + 2) {
// clang-format off
#pragma HLS PIPELINE II=2
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
            // clang-format on

            // Data always need to be read out from buffer
            in_buf = _src_mat.read(row * (imgwidth) + col); //.data[row*(imgwidth) + col];
            if (col == (imgwidth - 1)) {
                in_buf1 = 0;
            } else {
                in_buf1 = _src_mat.read(row * (imgwidth) + col + 1); //.data[row*(imgwidth) + col+1];
            }

            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                int zone_idx = (zone_row * zone_col_num) + zone_col;

            EXTRACT_UPDATE:
                for (ap_uint<9> i = 0, j = 0; i < ((8 << XF_BITSHIFT(NPC)) * PLANES); j++, i += 8) {
// clang-format off
#pragma HLS DEPENDENCE variable = tmp_stats array intra false
#pragma HLS DEPENDENCE variable = tmp_stats1 array intra false
#pragma HLS UNROLL
                    // clang-format on

                    ap_uint<8> val = 0, val1 = 0;

                    val = in_buf.range(i + 7, i);
                    val1 = in_buf1.range(i + 7, i);

                    uint32_t tmpval = tmp_stats[zone_idx][j][val];
                    uint32_t tmpval1 = tmp_stats1[zone_idx][j][val1];

                    tmp_stats[zone_idx][j][val] = tmpval + 1;
                    if (!(col == (imgwidth - 1))) {
                        tmp_stats1[zone_idx][j][val1] = tmpval1 + 1;
                    }
                }
            }
        }
    }

    const int num_ch = XF_CHANNELS(SRC_T, NPC);

MERGE_ZONE:
    for (ap_uint<8> zone = 0; zone < num_zones; zone++) {
    MERGE_STATS_LOOP:
        for (ap_uint<32> i = 0; i < STATS_SIZE; i++) {
        MERGE_STATS_CH_UNROLL:
            for (ap_uint<5> ch = 0; ch < num_ch; ch++) {
#pragma HLS UNROLL

                uint32_t value = 0;
                uint32_t value1 = 0;

            MERGE_STATS_NPPC_UNROLL:
                for (ap_uint<5> p = 0; p < XF_NPIXPERCYCLE(NPC); p++) {
#pragma HLS UNROLL
                    value += tmp_stats[zone][p * num_ch + ch][i];
                    value1 += tmp_stats1[zone][p * num_ch + ch][i];
                }
                stats_array[zone][ch][i] = value + value1;
            }
        }
    }
}

template <int SRC_T,
          int ROWS,
          int COLS,
          int MAX_ZONES,
          int STATS_SIZE,
          int DEPTH,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int WORDWIDTH,
          int SRC_TC,
          int PLANES>
void xfSTATSKernel_bayer(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& src,
                         uint32_t stats_array[MAX_ZONES][3][STATS_SIZE],
                         uint16_t& height,
                         uint16_t& width,
                         int roi_tlx,
                         int roi_tly,
                         int roi_brx,
                         int roi_bry,
                         int zone_col_num,
                         int zone_row_num) {
    XF_TNAME(SRC_T, NPC) in_pix_red, in_pix_green, in_pix_blue;

    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);

    int num_bins = STATS_SIZE;
    int num_zones = zone_row_num * zone_col_num;

    for (ap_uint<7> i = 0; i < num_zones; i++) {
#pragma HLS PIPELINE
        for (ap_uint<5> j = 0; j < 3; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=3    
#pragma HLS PIPELINE
            // clang-format on
            for (ap_uint<10> k = 0; k < STATS_SIZE; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=STATS_SIZE
#pragma HLS UNROLL
                // clang-format on
                stats_array[i][j][k] = 0;
            }
        }
    }

    XF_CTUNAME(SRC_T, NPC) kr = 0, kb = 0, kg = 0;

    int zone_width = int((roi_brx - roi_tlx + 1) / zone_col_num);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / zone_row_num); // roi_height / M

    for (int i = 0; i < src.rows; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
#pragma HLS pipeline
        // clang-format on
        for (int j = 0; j < src.cols; j = j + 2) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/2
#pragma HLS UNROLL
            // clang-format on
            if (i % 2 == 0) // EVEN ROW
            {
                in_pix_red = src.read(i * width + j);
                in_pix_green = src.read((i * width + j) + 1);
                if ((i >= roi_tlx) && (i <= roi_brx) && (j >= roi_tly) && (j <= roi_bry)) {
                    int zone_col = int((i - roi_tlx) / zone_width);
                    int zone_row = int((j - roi_tly) / zone_height);
                    int zone_idx = (zone_row * zone_col_num) + zone_col;

                    for (int r = 0; r < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); r++) {
#pragma HLS UNROLL
                        kr = in_pix_red.range(r * STEP + STEP - 1, r * STEP);
                        stats_array[zone_idx][0][kr] = stats_array[zone_idx][0][kr] + 1;
                    }

                    for (int g = 0; g < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); g++) {
#pragma HLS UNROLL
                        kg = in_pix_green.range(g * STEP + STEP - 1, g * STEP);
                        stats_array[zone_idx][1][kg] = stats_array[zone_idx][1][kg] + 1;
                    }
                }
            } else // ODD ROW (i%2!=0)
            {
                in_pix_green = src.read(i * width + j);
                in_pix_blue = src.read((i * width + j) + 1);
                if ((i >= roi_tlx) && (i <= roi_brx) && (j >= roi_tly) && (j <= roi_bry)) {
                    int zone_col = int((i - roi_tlx) / zone_width);
                    int zone_row = int((j - roi_tly) / zone_height);
                    int zone_idx = (zone_row * zone_col_num) + zone_col;

                    for (int g = 0; g < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); g++) {
#pragma HLS UNROLL
                        kg = in_pix_green.range(g * STEP + STEP - 1, g * STEP);
                        stats_array[zone_idx][1][kg] = stats_array[zone_idx][1][kg] + 1;
                    }

                    for (int b = 0; b < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); b++) {
#pragma HLS UNROLL
                        kb = in_pix_blue.range(b * STEP + STEP - 1, b * STEP);
                        stats_array[zone_idx][2][kb] = stats_array[zone_idx][2][kb] + 1;
                    }
                }
            }
        }
    }
}

template <int MAX_ZONES,
          int STATS_SIZE,
          int FINAL_BINS_NUM,
          int MERGE_BINS,
          int SRC_T,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT>
void ispStats(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _src,
              uint32_t* stats,
              uint32_t* max_bins_list,
              uint16_t roi_tlx,
              uint16_t roi_tly,
              uint16_t roi_brx,
              uint16_t roi_bry,
              uint16_t zone_col_num,
              uint16_t zone_row_num) {
#ifndef __SYNTHESIS__
    assert(((NPC == XF_NPPC1)) && "NPC must be XF_NPPC1");
    assert(((_src.rows <= ROWS) && (_src.cols <= COLS)) && "ROWS and COLS should be greater than input image");
    assert(((roi_brx < _src.cols) && (roi_bry < _src.rows)) &&
           "ROI bottom-right coordinates should be less than input image");
    assert(((roi_tlx < roi_brx) && (roi_tly < roi_bry)) &&
           "ROI top-left coordinates should be less bottom-right coordinates");
    assert((((roi_brx - roi_tlx + 1) % zone_col_num) == 0 && ((roi_bry - roi_tly + 1) % zone_row_num) == 0) &&
           "ROI Sub-zones should be divisible by N(zone_row_num) x M (zone_row_num) setting");
    assert(((zone_col_num >= 0) && (zone_col_num <= 8)) &&
           "Number of sub-zones columns should be greater than or equal to 0 but less than or equal to 8");
    assert(((zone_row_num >= 0) && (zone_row_num <= 8)) &&
           "Number of sub-zones rows should be greater than or equal to 0 but less than or equal to 8");
#endif

// clang-format off
#pragma HLS INLINE OFF
#pragma HLS ARRAY_PARTITION variable=max_bins_list dim=1 type=complete
    // clang-format on

    uint16_t width = _src.cols >> (XF_BITSHIFT(NPC));
    uint16_t height = _src.rows;

    uint32_t stats_array[MAX_ZONES][3][STATS_SIZE] = {0};

    // Setting of zone col/row num to 0 will automatically be changed to 1
    // This setting allows the user to control the rows and columns independently
    if (zone_col_num == 0) {
        zone_col_num = 1;
    }
    if (zone_row_num == 0) {
        zone_row_num = 1;
    }

    if (SRC_T == XF_8UC3) {
        xfSTATSKernel_bgr<SRC_T, ROWS, COLS, MAX_ZONES, STATS_SIZE, XF_DEPTH(SRC_T, NPC), NPC, XFCVDEPTH_IN,
                          XF_WORDWIDTH(SRC_T, NPC), ((COLS >> (XF_BITSHIFT(NPC))) >> 1), XF_CHANNELS(SRC_T, NPC)>(
            _src, stats_array, height, width, roi_tlx, roi_tly, roi_brx, roi_bry, zone_col_num, zone_row_num);
    }

    if (SRC_T == XF_8UC1) {
        xfSTATSKernel_bayer<SRC_T, ROWS, COLS, MAX_ZONES, STATS_SIZE, XF_DEPTH(SRC_T, NPC), NPC, XFCVDEPTH_IN,
                            XF_WORDWIDTH(SRC_T, NPC), ((COLS >> (XF_BITSHIFT(NPC))) >> 1), XF_CHANNELS(SRC_T, NPC)>(
            _src, stats_array, height, width, roi_tlx, roi_tly, roi_brx, roi_bry, zone_col_num, zone_row_num);
    }

    int num_ch = 3; // XF_CHANNELS(SRC_T, NPC);
    int num_zones = zone_row_num * zone_col_num;
    int total_bins = STATS_SIZE;
    int num_final_bins;

    if (MERGE_BINS == 0) {
        num_final_bins = total_bins;
    } else {
        num_final_bins = FINAL_BINS_NUM;
    }

STATS_INIT_LOOP:
    for (ap_uint<16> idx = 0; idx < (num_zones * num_final_bins * 3); idx++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=3*FINAL_BINS_NUM max=MAX_ZONES*3*STATS_SIZE
        // clang-format on
        stats[idx] = 0;
    }

STATS_MERGE_ZONE_LOOP:
    for (int k = 0; k < num_zones; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_ZONES
    // clang-format on
    STATS_MERGE_CH_LOOP:
        for (int j = 0; j < num_ch; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=3
            // clang-format on              
            uint32_t hi_bin = max_bins_list[0];
            uint32_t bin_group = 0;
        STATS_MERGE_BINS:
            for (int i = 0; i < total_bins; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=STATS_SIZE max=STATS_SIZE
#pragma HLS LOOP_FLATTEN
                // clang-format on

                uint32_t temp_val = stats_array[k][j][i];

                if (MERGE_BINS == 0) {
                    stats[(k * STATS_SIZE * num_ch) + (j * STATS_SIZE) + i] = temp_val;
                } else {
                    stats[((k * num_ch * num_final_bins) + (j * num_final_bins) + bin_group)] += temp_val;

                    if (i == hi_bin) {
                        bin_group = bin_group + 1;
                        hi_bin = max_bins_list[bin_group];
                    }
                }
            }
        }
    }
}

} // namespace cv
} // namespace xf
#endif // _XF_ISPSTATS_HPP_
