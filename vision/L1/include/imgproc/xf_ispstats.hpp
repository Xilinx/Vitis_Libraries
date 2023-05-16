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
          int PLANES,
          int NUM_OUT_CH>
void xfSTATSKernel_bgr(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src_mat,
                       uint32_t stats_array[MAX_ZONES][NUM_OUT_CH][STATS_SIZE],
                       uint16_t& imgheight,
                       uint16_t& imgwidth,
                       uint16_t& roi_tlx,
                       uint16_t& roi_tly,
                       uint16_t& roi_brx,
                       uint16_t& roi_bry,
                       uint16_t& zone_col_num,
                       uint16_t& zone_row_num,
                       float inputMin,
                       float inputMax,
                       float outputMin,
                       float outputMax) {
    int num_bins = STATS_SIZE;
    int num_zones = zone_row_num * zone_col_num;

    XF_SNAME(WORDWIDTH) in_buf;

STATS_INIT_LOOP:
    for (ap_uint<7> k = 0; k < num_zones; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_ZONES
        // clang-format on
        for (ap_uint<5> j = 0; j < ((1 << XF_BITSHIFT(NPC)) * PLANES); j++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (ap_uint<13> i = 0; i < STATS_SIZE; i++) { //
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=STATS_SIZE max=STATS_SIZE
                // clang-format on
                stats_array[k][j][i] = 0;
            }
        }
    }

    int zone_width = int((roi_brx - roi_tlx + 1) / zone_col_num);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / zone_row_num); // roi_height / M

    int bins = STATS_SIZE;
    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
    ap_fixed<STEP + 8, STEP + 2> min_vals = inputMin - 0.5f;
    ap_fixed<STEP + 8, STEP + 2> max_vals = inputMax + 0.5f;

    ap_fixed<STEP + 8, STEP + 2> minValue = min_vals, minValue1 = min_vals;
    ap_fixed<STEP + 8, STEP + 2> maxValue = max_vals, maxValue1 = max_vals;

    ap_fixed<STEP + 8, STEP + 2> interval = ap_fixed<STEP + 8, STEP + 2>(maxValue - minValue) / bins;

    ap_fixed<STEP + 8, 2> internal_inv = ((ap_fixed<STEP + 8, 2>)1 / interval);

    int currentBin[3] = {0};

    int zone_idx = 0; // = (zone_row * zone_col_num) + zone_col;
    int zone_idx_prev = 0;

    ap_uint<16> src_pix_val[PLANES] = {0};
    ap_uint<16> wr_stats_idx[PLANES] = {0};

    uint32_t rd_stats_val[PLANES] = {0};
    uint32_t wr_stats_val[PLANES] = {0};

STATS_ROW_LOOP:
    for (uint16_t row = 0; row < imgheight; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on

    STATS_COL_LOOP:
        for (uint16_t col = 0; col < (imgwidth + 1); col++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
            // clang-format on

            // Data always need to be read out from buffer
            // Read BGR, 8-bit per channel at once
            if (col < imgwidth) {
                in_buf = _src_mat.read(row * (imgwidth) + col); //.data[row*(imgwidth) + col];
            }

            // Check if part of zone
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                zone_idx = (zone_row * zone_col_num) + zone_col;

            EXTRACT_UPDATE:
                for (ap_uint<9> ch = 0; ch < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); ch++) {
// clang-format off
#pragma HLS UNROLL
                    // clang-format on

                    src_pix_val[ch] = in_buf.range(ch * STEP + STEP - 1, ch * STEP); // (B)0:7, (G)8:15, (R)16:23
                    currentBin[ch] = int((src_pix_val[ch] - minValue) * internal_inv);
                    // If same zone
                    if (zone_idx == zone_idx_prev) {
                        // Check if write and read address is the same
                        if (currentBin[ch] == wr_stats_idx[ch]) {
                            rd_stats_val[ch] = wr_stats_val[ch];
                        } else {
                            // Read operation from BRAM only if there's no conflict
                            rd_stats_val[ch] = stats_array[zone_idx][ch][currentBin[ch]];
                        }

                    } else {
                        // Need to read if zone change
                        rd_stats_val[ch] = stats_array[zone_idx][ch][currentBin[ch]];
                    }
                    // Write operation to BRAM
                    stats_array[zone_idx_prev][ch][wr_stats_idx[ch]] = wr_stats_val[ch];

                    wr_stats_val[ch] = rd_stats_val[ch] + 1;
                    wr_stats_idx[ch] = currentBin[ch];
                }
            } else if (col == imgwidth) {
            LAST_COL_WR:
                for (ap_uint<3> ch = 0; ch < PLANES; ch++) {
// clang-format off
#pragma HLS UNROLL
// clang-format off       
                    // Write operation to BRAM
                    stats_array[zone_idx][ch][wr_stats_idx[ch]] = wr_stats_val[ch];
                }
            }

            zone_idx_prev = zone_idx;
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
          int PLANES,
          int NUM_OUT_CH>
void xfSTATSKernel_bayer(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                         uint32_t stats_array[MAX_ZONES][NUM_OUT_CH][STATS_SIZE],
                         uint16_t& height,
                         uint16_t& width,
                         uint16_t& roi_tlx,
                         uint16_t& roi_tly,
                         uint16_t& roi_brx,
                         uint16_t& roi_bry,
                         uint16_t& zone_col_num,
                         uint16_t& zone_row_num,
                         float inputMin,
                         float inputMax,
                         float outputMin,
                         float outputMax) {
    XF_TNAME(SRC_T, NPC) in_buf, in_pix_red, in_pix_green, in_pix_blue;

    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
    
    int num_bins = STATS_SIZE;
    int num_zones = zone_row_num * zone_col_num;

STATS_INITIALIZE_LOOP:
    for (ap_uint<7> i = 0; i < num_zones; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min = 1 max = MAX_ZONES
        // clang-format on
        for (ap_uint<13> j = 0; j < STATS_SIZE; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=STATS_SIZE
            // clang-format on
            for (ap_uint<2> k = 0; k < 3; k++) {
// clang-format off
#pragma HLS UNROLL
                // clang-format on
                stats_array[i][k][j] = 0;
            }
        }
    }

    int16_t k = 0;

    int zone_width = int((roi_brx - roi_tlx + 1) / zone_col_num);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / zone_row_num); // roi_height / M

    int bins = STATS_SIZE;
    ap_fixed<STEP + 8, STEP + 2> min_vals = inputMin - 0.5f;
    ap_fixed<STEP + 8, STEP + 2> max_vals = inputMax + 0.5f;

    ap_fixed<STEP + 8, STEP + 2> minValue = min_vals, minValue1 = min_vals;
    ap_fixed<STEP + 8, STEP + 2> maxValue = max_vals, maxValue1 = max_vals;

    ap_fixed<STEP + 8, STEP + 2> interval = ap_fixed<STEP + 8, STEP + 2>(maxValue - minValue) / bins;

    ap_fixed<STEP + 8, 2> internal_inv = ((ap_fixed<STEP + 8, 2>)1 / interval);

    int zone_col;
    int zone_row;

    int currentBin[3] = {0};

    int prev_row_id = 0;
    int row_id = 0;

    int zone_idx = 0; // = (zone_row * zone_col_num) + zone_col;
    int zone_idx_prev = 0;

    ap_uint<16> src_pix_val = 0;
    ap_uint<16> wr_stats_idx[3] = {0};

    int rd_ptr = 0, wr_ptr = 0, row_idx, col_idx;
    ap_uint<2> row_rem, col_rem, sum_rem, row_incr, col_incr, color_idx;

    uint32_t rd_stats_val[3] = {0};
    uint32_t wr_stats_val[3] = {0};

    ap_uint<2> ch = 0;
    ap_uint<2> ch_prev = 0;

    col_incr = 1;

    row_incr = 1;

STATS_ROW_LOOP:
    for (ap_uint<13> row = 0; row < src.rows; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
        // clang-format on

        row_idx = row + row_incr;
        row_rem = row_idx & 0x00000001;

    STATS_COL_LOOP:
        for (ap_uint<13> col = 0; col < (src.cols + 1); col++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
#pragma HLS LOOP_FLATTEN off
            // clang-format on

            // Data always need to be read out from buffer
            // Read BGR, 8-bit per channel at once
            if (col < src.cols) {
                in_buf = src.read(rd_ptr++); //.data[row*(imgwidth) + col];
            }

            // Check if part of zone
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                zone_idx = (zone_row * zone_col_num) + zone_col;

                col_idx = col * NPC + 0 + col_incr;

                col_rem = col_idx & 0x00000001;

                int rem = row_rem + col_rem;

                if (rem == 0) {
                    ch = 0;
                } else if (rem == 1) {
                    ch = 1;
                } else if (rem == 2) {
                    ch = 2;
                }

                src_pix_val = in_buf.range(STEP - 1, 0); // (B)0:7, (G)8:15, (R)16:23
                currentBin[ch] = int((src_pix_val - minValue) * internal_inv);

                // Read stats
                rd_stats_val[ch] = stats_array[zone_idx][ch][currentBin[ch]];
                // Write stats
                stats_array[zone_idx_prev][ch_prev][wr_stats_idx[ch_prev]] = wr_stats_val[ch_prev];

                wr_stats_val[ch] = rd_stats_val[ch] + 1;
                wr_stats_idx[ch] = currentBin[ch];

            } else if (col == src.cols) {
                // Write operation to BRAM at last column
                stats_array[zone_idx][ch][wr_stats_idx[ch]] = wr_stats_val[ch];
            }

            zone_idx_prev = zone_idx;
            ch_prev = ch;
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
          int PLANES,
          int NUM_OUT_CH>
void xfSTATSKernel_gray(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src_mat,
                        uint32_t stats_array[MAX_ZONES][NUM_OUT_CH][STATS_SIZE],
                        uint16_t& imgheight,
                        uint16_t& imgwidth,
                        uint16_t& roi_tlx,
                        uint16_t& roi_tly,
                        uint16_t& roi_brx,
                        uint16_t& roi_bry,
                        uint16_t& zone_col_num,
                        uint16_t& zone_row_num,
                        float inputMin,
                        float inputMax,
                        float outputMin,
                        float outputMax) {
    int num_zones = zone_row_num * zone_col_num;

    XF_SNAME(WORDWIDTH) in_buf;

STATS_INIT:
    for (ap_uint<7> k = 0; k < num_zones; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_ZONES
        // clang-format on
        for (ap_uint<5> j = 0; j < ((1 << XF_BITSHIFT(NPC)) * PLANES); j++) {
// clang-format off
#pragma HLS UNROLL
            // clang-format on
            for (ap_uint<13> i = 0; i < STATS_SIZE; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=STATS_SIZE max=STATS_SIZE
                // clang-format on
                stats_array[k][j][i] = 0;
            }
        }
    }

    int zone_width = int((roi_brx - roi_tlx + 1) / zone_col_num);  // roi_width / N
    int zone_height = int((roi_bry - roi_tly + 1) / zone_row_num); // roi_height / M

    int zone_idx = 0; // = (zone_row * zone_col_num) + zone_col;
    int zone_idx_prev = 0;

    ap_uint<16> src_pix_val[PLANES] = {0};
    ap_uint<16> wr_stats_idx[PLANES] = {0};

    uint32_t rd_stats_val[PLANES] = {0};
    uint32_t wr_stats_val[PLANES] = {0};

    int bins = STATS_SIZE;
    const int STEP = XF_DTPIXELDEPTH(SRC_T, NPC);
    ap_fixed<STEP + 8, STEP + 2> min_vals = inputMin - 0.5f;
    ap_fixed<STEP + 8, STEP + 2> max_vals = inputMax + 0.5f;

    ap_fixed<STEP + 8, STEP + 2> minValue = min_vals, minValue1 = min_vals;
    ap_fixed<STEP + 8, STEP + 2> maxValue = max_vals, maxValue1 = max_vals;

    ap_fixed<STEP + 8, STEP + 2> interval = ap_fixed<STEP + 8, STEP + 2>(maxValue - minValue) / bins;

    ap_fixed<STEP + 8, 2> internal_inv = ((ap_fixed<STEP + 8, 2>)1 / interval);

    int currentBin[3] = {0};

ROW_LOOP:
    for (uint16_t row = 0; row < imgheight; row++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
    // clang-format on

    COL_LOOP:
        for (uint16_t col = 0; col < (imgwidth + 1); col++) {
// clang-format off
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
            // clang-format on

            // Data always need to be read out from buffer
            // Read BGR, 8-bit per channel at once
            if (col < imgwidth) {
                in_buf = _src_mat.read(row * (imgwidth) + col); //.data[row*(imgwidth) + col];
            }

            // Check if part of zone
            if ((row >= roi_tlx) && (row <= (roi_brx)) && (col >= roi_tly) && (col <= (roi_bry))) {
                int zone_col = int((col - roi_tlx) / zone_width);
                int zone_row = int((row - roi_tly) / zone_height);
                zone_idx = (zone_row * zone_col_num) + zone_col;

            EXTRACT_UPDATE:

                for (ap_uint<9> ch = 0; ch < XF_NPIXPERCYCLE(NPC) * XF_CHANNELS(SRC_T, NPC); ch++) {
// clang-format off
#pragma HLS UNROLL
                    // clang-format on

                    src_pix_val[ch] = in_buf.range(ch * STEP + STEP - 1, ch * STEP); // (B)0:7, (G)8:15, (R)16:23
                    currentBin[ch] = int((src_pix_val[ch] - minValue) * internal_inv);

                    // If same zone
                    if (zone_idx == zone_idx_prev) {
                        // Check if write and read address is the same
                        if (currentBin[ch] == wr_stats_idx[ch]) {
                            rd_stats_val[ch] = wr_stats_val[ch];
                        } else {
                            // Read operation from BRAM only if there's no conflict
                            rd_stats_val[ch] = stats_array[zone_idx][ch][currentBin[ch]];
                        }
                    } else {
                        // Need to read if zone change
                        rd_stats_val[ch] = stats_array[zone_idx][ch][currentBin[ch]];
                    }
                    // Write operation to BRAM
                    stats_array[zone_idx_prev][ch][wr_stats_idx[ch]] = wr_stats_val[ch];

                    wr_stats_val[ch] = rd_stats_val[ch] + 1;
                    wr_stats_idx[ch] = currentBin[ch];
                }
            } else if (col == imgwidth) {
            LAST_COL_WR:
                for (ap_uint<3> ch = 0; ch < PLANES; ch++) {
// clang-format off
#pragma HLS UNROLL
                    // clang-format on
                    // Write operation to BRAM
                    stats_array[zone_idx][ch][wr_stats_idx[ch]] = wr_stats_val[ch];
                }
            }
            zone_idx_prev = zone_idx;
        }
    }
}

template <int MAX_ZONES,
          int STATS_SIZE,
          int FINAL_BINS_NUM,
          int MERGE_BINS,
          int SRC_T,
          int NUM_OUT_CH,
          int ROWS,
          int COLS,
          int NPC = 1,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT>
void ispStats(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src,
              uint32_t* stats,
              ap_uint<13>* max_bins_list,
              uint16_t roi_tlx,
              uint16_t roi_tly,
              uint16_t roi_brx,
              uint16_t roi_bry,
              uint16_t zone_col_num,
              uint16_t zone_row_num,
              float inputMin,
              float inputMax,
              float outputMin,
              float outputMax) {
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
    // clang-format on

    uint16_t width = _src.cols >> (XF_BITSHIFT(NPC));
    uint16_t height = _src.rows;

    ap_uint<2> num_out_ch = NUM_OUT_CH; // XF_CHANNELS(SRC_T, NPC);

    uint32_t stats_array[MAX_ZONES][NUM_OUT_CH][STATS_SIZE]; // = {0};

    if (zone_col_num == 0) {
        zone_col_num = 1;
    }
    if (zone_row_num == 0) {
        zone_row_num = 1;
    }

    if (((SRC_T == XF_8UC1) && (NUM_OUT_CH == 1)) || ((SRC_T == XF_16UC1) && (NUM_OUT_CH == 1))) {
        xfSTATSKernel_gray<SRC_T, ROWS, COLS, MAX_ZONES, STATS_SIZE, XF_DEPTH(SRC_T, NPC), NPC, XFCVDEPTH_IN,
                           XF_WORDWIDTH(SRC_T, NPC), ((COLS + 1) >> (XF_BITSHIFT(NPC))), XF_CHANNELS(SRC_T, NPC),
                           NUM_OUT_CH>(_src, stats_array, height, width, roi_tlx, roi_tly, roi_brx, roi_bry,
                                       zone_col_num, zone_row_num, inputMin, inputMax, outputMin, outputMax);

    } else if (((SRC_T == XF_8UC3) && (NUM_OUT_CH == 3)) || ((SRC_T == XF_16UC3) && (NUM_OUT_CH == 3))) {
        xfSTATSKernel_bgr<SRC_T, ROWS, COLS, MAX_ZONES, STATS_SIZE, XF_DEPTH(SRC_T, NPC), NPC, XFCVDEPTH_IN,
                          XF_WORDWIDTH(SRC_T, NPC), ((COLS + 1) >> (XF_BITSHIFT(NPC))), XF_CHANNELS(SRC_T, NPC),
                          NUM_OUT_CH>(_src, stats_array, height, width, roi_tlx, roi_tly, roi_brx, roi_bry,
                                      zone_col_num, zone_row_num, inputMin, inputMax, outputMin, outputMax);

    } else if (((SRC_T == XF_8UC1) && (NUM_OUT_CH == 3)) || ((SRC_T == XF_16UC1) && (NUM_OUT_CH == 3)) ||
               ((SRC_T == XF_14UC1) && (NUM_OUT_CH == 3))) {
        xfSTATSKernel_bayer<SRC_T, ROWS, COLS, MAX_ZONES, STATS_SIZE, XF_DEPTH(SRC_T, NPC), NPC, XFCVDEPTH_IN,
                            XF_WORDWIDTH(SRC_T, NPC), COLS, XF_CHANNELS(SRC_T, NPC), NUM_OUT_CH>(
            _src, stats_array, height, width, roi_tlx, roi_tly, roi_brx, roi_bry, zone_col_num, zone_row_num, inputMin,
            inputMax, outputMin, outputMax);
    }

    ap_uint<7> num_zones = zone_row_num * zone_col_num;
    ap_uint<13> num_final_bins;

    if (MERGE_BINS == 0) {
        num_final_bins = STATS_SIZE;
    } else {
        num_final_bins = FINAL_BINS_NUM;
    }

    ap_uint<13> max_bins[FINAL_BINS_NUM] = {0};

    uint32_t temp_val;

MAX_BINS_READ:
    for (ap_uint<3> i = 0; i < FINAL_BINS_NUM; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=FINAL_BINS_NUM max=FINAL_BINS_NUM
        // clang-format on
        max_bins[i] = max_bins_list[i];
    }

MERGE_ZONE:
    for (ap_uint<7> k = 0; k < num_zones; k++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAX_ZONES
    // clang-format on
    MERGE_CH:
        for (ap_uint<2> j = 0; j < num_out_ch; j++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=1 max=NUM_OUT_CH
            // clang-format on              
            ap_uint<13> hi_bin = max_bins[0];
            ap_uint<13> bin_group = 0;
            uint32_t bin_acc = 0;
        MERGE_BIN:
            for (ap_uint<13> i = 0; i < STATS_SIZE; i++) {
// clang-format off
#pragma HLS LOOP_TRIPCOUNT min=STATS_SIZE max=STATS_SIZE
#pragma HLS LOOP_FLATTEN OFF
                // clang-format on

                temp_val = stats_array[k][j][i];

                if (MERGE_BINS == 0) {
                    stats[(k * num_final_bins * num_out_ch) + (j * num_final_bins) + i] = temp_val;
                } else {
                    bin_acc += temp_val;
                    if (i == hi_bin) {
                        stats[(k * num_final_bins * num_out_ch) + (j * num_final_bins) + bin_group] = bin_acc;

                        if (bin_group < num_final_bins) {
                            bin_group = bin_group + 1;
                            hi_bin = max_bins[bin_group];
                            bin_acc = 0;
                        }
                    }
                }
            }
        }
    }
}
} // namespace cv
} // namespace xf
#endif // _XF_ISPSTATS_HPP_