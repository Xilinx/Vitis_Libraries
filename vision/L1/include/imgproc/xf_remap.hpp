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

#ifndef _XF_REMAP_HPP_
#define _XF_REMAP_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header.
#endif

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "hls_stream.h"
#include <algorithm>

#define XF_RESIZE_INTER_TAB_SIZE 32
#define XF_RESIZE_INTER_BITS 5

#define XF_INTERPOLATION_BICUBIC 2

template <int SRC_T,
          int DST_T,
          int PLANES,
          int MAP_T,
          int WIN_ROW,
          int ROWS,
          int COLS,
          int HLS_REMAPED_ROWS,
          int HLS_REMAPED_COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT,
          bool USE_URAM>
void xFRemapCI(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
               xf::cv::Mat<DST_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_Remapped>& dst,
               xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPX>& mapx,
               xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPY>& mapy,
               uint16_t rows,
               uint16_t cols,
               uint16_t hls_remaped_rows,
               uint16_t hls_remaped_cols) {
#pragma HLS DATAFLOW
    ap_uint<48> line_buffer[WIN_ROW >> 2][4][COLS >> 1];

    // Select storage for line_buffer based on USE_URAM.
    // URAM: larger capacity, good for wide images, single-cycle latency configured.
    // BRAM: default on-chip memory, also set to two-port with same latency.
    if (USE_URAM) {
#pragma HLS bind_storage variable = line_buffer type = RAM_T2P impl = URAM latency = 1
// Partition the 2nd dimension (kernel row within the 4-row block)
// to enable parallel access to each of the 4 rows.
#pragma HLS array_partition complete variable = line_buffer dim = 2
    } else {
#pragma HLS bind_storage variable = line_buffer type = RAM_T2P impl = BRAM latency = 1
// Same partitioning when using BRAM to keep identical access semantics.
#pragma HLS array_partition complete variable = line_buffer dim = 2
    }

    XF_TNAME(SRC_T, NPC) s;
    ap_uint<48> temp_buf_uram_npc1 = 0;
    ap_uint<48> temp_buf_uram_npc2 = 0;

    float one = 1.0f;
    float two = 2.0f;
    float one_point_five = 1.5f;
    float two_point_five = 2.5f;
    float four = 4.0f;
    float zero = 0.0f;

    XF_TNAME(DST_T, NPC) output_pixel = 0;

    float fraction_x[2];
    float fraction_y[2];

    ap_ufixed<32, 24> x_int[2], y_int[2];

    ap_uint<7> f1_msb[2];
    ap_uint<7> f1_lsb[2];

    ap_ufixed<32, 24> x_fl[2];
    ap_ufixed<32, 24> y_fl[2];
    int wbase[2];

    ap_int<XF_DTPIXELDEPTH(MAP_T, NPC) * NPC> mapped_x, mapped_y;

    int bit_depth = XF_PIXELDEPTH(SRC_T);

    int x_fix[2];
    int y_fix[2];

    int x[2];
    int y[2];
    int x_frac[2];
    int y_frac[2];
    int read_pointer_src = 0, read_pointer_map = 0, write_pointer = 0;

    float weights_x[2][4], weights_y[2][4];
    float accumulated_sum[2][PLANES];
    ap_uint<16> win_row_by_four = WIN_ROW >> 2;

    // local storage: for each of 4 kernel rows preload 3 words (each 48-bit)
    ap_uint<48> line_buffer_words[4][2][2];
#pragma HLS ARRAY_PARTITION variable = line_buffer_words complete dim = 1
#pragma HLS ARRAY_PARTITION variable = line_buffer_words complete dim = 2
#pragma HLS ARRAY_PARTITION variable = line_buffer_words complete dim = 3

    int row_iteration = 0;
    if (hls_remaped_rows > rows) {
        row_iteration = hls_remaped_rows;
    } else {
        row_iteration = rows;
    }

    int col_iteration = 0;
    if (hls_remaped_cols > cols) {
        col_iteration = hls_remaped_cols;
    } else {
        col_iteration = cols;
    }

    int ISHIFT;
    if (WIN_ROW >= ROWS) {
        ISHIFT = WIN_ROW;
    } else if (WIN_ROW < ROWS) {
        ISHIFT = WIN_ROW / 2;
    }

    const int ROW_TRIPCOUNT = HLS_REMAPED_ROWS + WIN_ROW / 2 - 1;
    const int COL_TRIPCOUNT = HLS_REMAPED_COLS / NPC;

loop_height:
    for (int i = 0; i < row_iteration + ISHIFT; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = ROW_TRIPCOUNT

    loop_width:
        for (int j = 0; j < col_iteration; ++j) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = COL_TRIPCOUNT
#pragma HLS PIPELINE II = 1
#pragma HLS LOOP_FLATTEN OFF

#pragma HLS dependence variable = line_buffer inter RAW false
#pragma HLS dependence variable = line_buffer intra false

            // Read input pixel if within source image, else zero
            if (i < rows && j < cols) {
                s = src.read(read_pointer_src++);
            } else {
                s = 0;
            }

            // Pack two pixels into a 48-bit word and write to URAM line buffer
            if (i < rows && j < cols) {
                if (NPC > 1) {
                    line_buffer[(i >> 2) % win_row_by_four][i & 3][j] = s;
                } else {
                    if ((j & 1) == 0) {
                        temp_buf_uram_npc1 = s;
                    } else {
                        temp_buf_uram_npc2 = s;
                        ap_uint<48> packed = ((temp_buf_uram_npc2 << bit_depth) | temp_buf_uram_npc1);
                        line_buffer[(i >> 2) % win_row_by_four][i & 3][j >> 1] = packed;
                    }
                }
            }

            // Output when enough rows have been buffered
            if (i >= ISHIFT && i < hls_remaped_rows + ISHIFT && j < hls_remaped_cols) {
                int current_row = i - ISHIFT;

                bool is_in_bounds =
                    (current_row >= 0 && current_row < hls_remaped_rows && j >= 0 && j < hls_remaped_cols);

                if (is_in_bounds) {
                    // NOTE: keep same fixed-point scale you used previously
                    mapped_x = mapx.read(read_pointer_map);
                    mapped_y = mapy.read(read_pointer_map++);
                }

                for (int n = 0; n < NPC; n++) {
                    f1_msb[n] = ((n + 1) << 5) - 1;
                    f1_lsb[n] = (n << 5);

                    x_int[n] = mapped_x.range(f1_msb[n], f1_lsb[n]);
                    y_int[n] = mapped_y.range(f1_msb[n], f1_lsb[n]);

                    x_fl[n] = (x_int[n] >> 8);
                    y_fl[n] = (y_int[n] >> 8);

                    x_fix[n] = (x_fl[n] << XF_RESIZE_INTER_BITS); // mapx data in
                                                                  // A16.XF_RESIZE_INTER_TAB_SIZE
                                                                  // format
                    y_fix[n] = (y_fl[n] << XF_RESIZE_INTER_BITS); // mapy data in
                                                                  // A16.XF_RESIZE_INTER_TAB_SIZE
                                                                  // format

                    x[n] = x_fix[n] >> XF_RESIZE_INTER_BITS;
                    y[n] = y_fix[n] >> XF_RESIZE_INTER_BITS;
                    x_frac[n] = x_fix[n] & (XF_RESIZE_INTER_TAB_SIZE - 1);
                    y_frac[n] = y_fix[n] & (XF_RESIZE_INTER_TAB_SIZE - 1);

                    // compute base word index that will allow loading three consecutive words safely
                    int first_needed_col = x[n];
                    if (first_needed_col < 0) first_needed_col = 0;
                    int first_word = first_needed_col >> 1;

                    int max_word_index = (cols)-1;
                    if (first_word > max_word_index - 2) {
                        first_word = (max_word_index - 2 > 0) ? (max_word_index - 2) : 0;
                    }
                    wbase[n] = first_word;
                }

                // preload three words for each of the 4 kernel rows
                if (y[0] <= 0) {
                    line_buffer_words[0][0][0] = line_buffer[0][0][wbase[0]];
                    line_buffer_words[0][1][0] = line_buffer[0][0][wbase[0] + 1];
                } else {
                    line_buffer_words[0][0][0] =
                        line_buffer[((y[0] - 1) >> 2) % win_row_by_four][((y[0] - 1) & 3)][wbase[0]];
                    line_buffer_words[0][1][0] =
                        line_buffer[((y[0] - 1) >> 2) % win_row_by_four][((y[0] - 1) & 3)][wbase[0] + 1];
                }
                line_buffer_words[1][0][0] = line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][wbase[0]];
                line_buffer_words[2][0][0] =
                    line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][wbase[0]];
                line_buffer_words[3][0][0] =
                    line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][wbase[0]];
                line_buffer_words[1][1][0] = line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][wbase[0] + 1];
                line_buffer_words[2][1][0] =
                    line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][wbase[0] + 1];
                line_buffer_words[3][1][0] =
                    line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][wbase[0] + 1];
                if (NPC > 1) {
                    if (y[1] > y[0]) {
                        if (wbase[1] > wbase[0]) {
                            line_buffer_words[0][0][1] = line_buffer[(((y[0] + 1) - 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) - 1) & 3)][(wbase[0] + 1)];
                            line_buffer_words[1][0][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] + 1)];
                            line_buffer_words[2][0][1] = line_buffer[(((y[0] + 1) + 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 1) & 3)][(wbase[0] + 1)];
                            line_buffer_words[3][0][1] = line_buffer[(((y[0] + 1) + 2) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 2) & 3)][(wbase[0] + 1)];
                            line_buffer_words[0][1][1] = line_buffer[(((y[0] + 1) - 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) - 1) & 3)][(wbase[0] + 1) + 1];
                            line_buffer_words[1][1][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] + 1) + 1];
                            line_buffer_words[2][1][1] = line_buffer[(((y[0] + 1) + 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 1) & 3)][(wbase[0] + 1) + 1];
                            line_buffer_words[3][1][1] = line_buffer[(((y[0] + 1) + 2) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 2) & 3)][(wbase[0] + 1) + 1];
                        } else if (wbase[1] == wbase[0]) {
                            line_buffer_words[0][0][1] = line_buffer[(((y[0] + 1) - 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) - 1) & 3)][wbase[0]];
                            line_buffer_words[1][0][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][wbase[0]];
                            line_buffer_words[2][0][1] = line_buffer[(((y[0] + 1) + 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 1) & 3)][wbase[0]];
                            line_buffer_words[3][0][1] = line_buffer[(((y[0] + 1) + 2) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 2) & 3)][wbase[0]];
                            line_buffer_words[0][1][1] = line_buffer[(((y[0] + 1) - 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) - 1) & 3)][wbase[0] + 1];
                            line_buffer_words[1][1][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][wbase[0] + 1];
                            line_buffer_words[2][1][1] = line_buffer[(((y[0] + 1) + 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 1) & 3)][wbase[0] + 1];
                            line_buffer_words[3][1][1] = line_buffer[(((y[0] + 1) + 2) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 2) & 3)][wbase[0] + 1];
                        } else if (wbase[1] < wbase[0]) {
                            line_buffer_words[0][0][1] = line_buffer[(((y[0] + 1) - 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) - 1) & 3)][(wbase[0] - 1)];
                            line_buffer_words[1][0][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] - 1)];
                            line_buffer_words[2][0][1] = line_buffer[(((y[0] + 1) + 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 1) & 3)][(wbase[0] - 1)];
                            line_buffer_words[3][0][1] = line_buffer[(((y[0] + 1) + 2) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 2) & 3)][(wbase[0] - 1)];
                            line_buffer_words[0][1][1] = line_buffer[(((y[0] + 1) - 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) - 1) & 3)][(wbase[0] - 1) + 1];
                            line_buffer_words[1][1][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] - 1) + 1];
                            line_buffer_words[2][1][1] = line_buffer[(((y[0] + 1) + 1) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 1) & 3)][(wbase[0] - 1) + 1];
                            line_buffer_words[3][1][1] = line_buffer[(((y[0] + 1) + 2) >> 2) % win_row_by_four]
                                                                    [(((y[0] + 1) + 2) & 3)][(wbase[0] - 1) + 1];
                        }
                    } else if (y[1] < y[0]) {
                        if (wbase[1] > wbase[0]) {
                            if (y[1] <= 0) {
                                line_buffer_words[0][0][1] = line_buffer[0][0][(wbase[0] + 1)];
                                line_buffer_words[1][0][1] = line_buffer[0][0][(wbase[0] + 1)];
                                line_buffer_words[2][0][1] = line_buffer[0][1][(wbase[0] + 1)];
                                line_buffer_words[3][0][1] = line_buffer[0][2][(wbase[0] + 1)];
                                line_buffer_words[0][1][1] = line_buffer[0][0][(wbase[0] + 1) + 1];
                                line_buffer_words[1][1][1] = line_buffer[0][0][(wbase[0] + 1) + 1];
                                line_buffer_words[2][1][1] = line_buffer[0][1][(wbase[0] + 1) + 1];
                                line_buffer_words[3][1][1] = line_buffer[0][2][(wbase[0] + 1) + 1];
                            } else {
                                line_buffer_words[0][0][1] = line_buffer[(((y[0] - 1) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) - 1) & 3)][(wbase[0] + 1)];
                                line_buffer_words[1][0][1] =
                                    line_buffer[((y[0] - 1) >> 2) % win_row_by_four][((y[0] - 1) & 3)][(wbase[0] + 1)];
                                line_buffer_words[2][0][1] = line_buffer[(((y[0] - 1) + 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 1) & 3)][(wbase[0] + 1)];
                                line_buffer_words[3][0][1] = line_buffer[(((y[0] - 1) + 2) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 2) & 3)][(wbase[0] + 1)];
                                line_buffer_words[0][1][1] = line_buffer[(((y[0] - 1) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) - 1) & 3)][(wbase[0] + 1) + 1];
                                line_buffer_words[1][1][1] = line_buffer[((y[0] - 1) >> 2) % win_row_by_four]
                                                                        [((y[0] - 1) & 3)][(wbase[0] + 1) + 1];
                                line_buffer_words[2][1][1] = line_buffer[(((y[0] - 1) + 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 1) & 3)][(wbase[0] + 1) + 1];
                                line_buffer_words[3][1][1] = line_buffer[(((y[0] - 1) + 2) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 2) & 3)][(wbase[0] + 1) + 1];
                            }

                        } else if (wbase[1] < wbase[0]) {
                            if (y[1] <= 0) {
                                line_buffer_words[0][0][1] = line_buffer[0][0][(wbase[0] - 1)];
                                line_buffer_words[1][0][1] = line_buffer[0][0][(wbase[0] - 1)];
                                line_buffer_words[2][0][1] = line_buffer[0][1][(wbase[0] - 1)];
                                line_buffer_words[3][0][1] = line_buffer[0][2][(wbase[0] - 1)];
                                line_buffer_words[0][1][1] = line_buffer[0][0][(wbase[0] - 1) + 1];
                                line_buffer_words[1][1][1] = line_buffer[0][0][(wbase[0] - 1) + 1];
                                line_buffer_words[2][1][1] = line_buffer[0][1][(wbase[0] - 1) + 1];
                                line_buffer_words[3][1][1] = line_buffer[0][2][(wbase[0] - 1) + 1];
                            } else {
                                line_buffer_words[0][0][1] = line_buffer[(((y[0] - 1) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) - 1) & 3)][(wbase[0] - 1)];
                                line_buffer_words[1][0][1] =
                                    line_buffer[((y[0] - 1) >> 2) % win_row_by_four][((y[0] - 1) & 3)][(wbase[0] - 1)];
                                line_buffer_words[2][0][1] = line_buffer[(((y[0] - 1) + 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 1) & 3)][(wbase[0] - 1)];
                                line_buffer_words[3][0][1] = line_buffer[(((y[0] - 1) + 2) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 2) & 3)][(wbase[0] - 1)];
                                line_buffer_words[0][1][1] = line_buffer[(((y[0] - 1) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) - 1) & 3)][(wbase[0] - 1) + 1];
                                line_buffer_words[1][1][1] = line_buffer[((y[0] - 1) >> 2) % win_row_by_four]
                                                                        [((y[0] - 1) & 3)][(wbase[0] - 1) + 1];
                                line_buffer_words[2][1][1] = line_buffer[(((y[0] - 1) + 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 1) & 3)][(wbase[0] - 1) + 1];
                                line_buffer_words[3][1][1] = line_buffer[(((y[0] - 1) + 2) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 2) & 3)][(wbase[0] - 1) + 1];
                            }

                        } else {
                            if (y[1] <= 0) {
                                line_buffer_words[0][0][1] = line_buffer[0][0][wbase[0]];
                                line_buffer_words[1][0][1] = line_buffer[0][0][wbase[0]];
                                line_buffer_words[2][0][1] = line_buffer[0][1][wbase[0]];
                                line_buffer_words[3][0][1] = line_buffer[0][2][wbase[0]];
                                line_buffer_words[0][1][1] = line_buffer[0][0][wbase[0] + 1];
                                line_buffer_words[1][1][1] = line_buffer[0][0][wbase[0] + 1];
                                line_buffer_words[2][1][1] = line_buffer[0][1][wbase[0] + 1];
                                line_buffer_words[3][1][1] = line_buffer[0][2][wbase[0] + 1];

                            } else {
                                line_buffer_words[0][0][1] = line_buffer[(((y[0] - 1) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) - 1) & 3)][wbase[0]];
                                line_buffer_words[1][0][1] =
                                    line_buffer[((y[0] - 1) >> 2) % win_row_by_four][((y[0] - 1) & 3)][wbase[0]];
                                line_buffer_words[2][0][1] = line_buffer[(((y[0] - 1) + 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 1) & 3)][wbase[0]];
                                line_buffer_words[3][0][1] = line_buffer[(((y[0] - 1) + 2) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 2) & 3)][wbase[0]];
                                line_buffer_words[0][1][1] = line_buffer[(((y[0] - 1) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) - 1) & 3)][wbase[0] + 1];
                                line_buffer_words[1][1][1] =
                                    line_buffer[((y[0] - 1) >> 2) % win_row_by_four][((y[0] - 1) & 3)][wbase[0] + 1];
                                line_buffer_words[2][1][1] = line_buffer[(((y[0] - 1) + 1) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 1) & 3)][wbase[0] + 1];
                                line_buffer_words[3][1][1] = line_buffer[(((y[0] - 1) + 2) >> 2) % win_row_by_four]
                                                                        [(((y[0] - 1) + 2) & 3)][wbase[0] + 1];
                            }
                        }
                    } else if (y[0] == y[1]) {
                        if (wbase[1] > wbase[0]) {
                            if (y[0] <= 0) {
                                line_buffer_words[0][0][1] = line_buffer[0][0][(wbase[0] + 1)];
                                line_buffer_words[0][1][1] = line_buffer[0][0][(wbase[0] + 1) + 1];

                            } else {
                                line_buffer_words[0][0][1] = line_buffer[(((y[0]) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0]) - 1) & 3)][(wbase[0] + 1)];
                                line_buffer_words[0][1][1] = line_buffer[(((y[0]) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0]) - 1) & 3)][(wbase[0] + 1) + 1];
                            }

                            line_buffer_words[1][0][1] =
                                line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][(wbase[0] + 1)];
                            line_buffer_words[2][0][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] + 1)];
                            line_buffer_words[3][0][1] =
                                line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][(wbase[0] + 1)];
                            line_buffer_words[1][1][1] =
                                line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][(wbase[0] + 1) + 1];
                            line_buffer_words[2][1][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] + 1) + 1];
                            line_buffer_words[3][1][1] =
                                line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][(wbase[0] + 1) + 1];

                        } else if (wbase[1] < wbase[0]) {
                            if (y[0] <= 0) {
                                line_buffer_words[0][0][1] = line_buffer[0][0][(wbase[0] - 1)];
                                line_buffer_words[0][1][1] = line_buffer[0][0][(wbase[0] - 1) + 1];

                            } else {
                                line_buffer_words[0][0][1] = line_buffer[(((y[0]) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0]) - 1) & 3)][(wbase[0] - 1)];
                                line_buffer_words[0][1][1] = line_buffer[(((y[0]) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0]) - 1) & 3)][(wbase[0] - 1) + 1];
                            }

                            line_buffer_words[1][0][1] =
                                line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][(wbase[0] - 1)];
                            line_buffer_words[2][0][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] - 1)];
                            line_buffer_words[3][0][1] =
                                line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][(wbase[0] - 1)];
                            line_buffer_words[1][1][1] =
                                line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][(wbase[0] - 1) + 1];
                            line_buffer_words[2][1][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][(wbase[0] - 1) + 1];
                            line_buffer_words[3][1][1] =
                                line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][(wbase[0] - 1) + 1];

                        } else {
                            if (y[0] <= 0) {
                                line_buffer_words[0][0][1] = line_buffer[0][0][wbase[0]];
                                line_buffer_words[0][1][1] = line_buffer[0][0][wbase[0] + 1];
                            } else {
                                line_buffer_words[0][0][1] =
                                    line_buffer[(((y[0]) - 1) >> 2) % win_row_by_four][(((y[0]) - 1) & 3)][wbase[0]];
                                line_buffer_words[0][1][1] = line_buffer[(((y[0]) - 1) >> 2) % win_row_by_four]
                                                                        [(((y[0]) - 1) & 3)][wbase[0] + 1];
                            }

                            line_buffer_words[1][0][1] =
                                line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][wbase[0]];
                            line_buffer_words[2][0][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][wbase[0]];
                            line_buffer_words[3][0][1] =
                                line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][wbase[0]];
                            line_buffer_words[1][1][1] =
                                line_buffer[(y[0] >> 2) % win_row_by_four][(y[0] & 3)][wbase[0] + 1];
                            line_buffer_words[2][1][1] =
                                line_buffer[((y[0] + 1) >> 2) % win_row_by_four][((y[0] + 1) & 3)][wbase[0] + 1];
                            line_buffer_words[3][1][1] =
                                line_buffer[((y[0] + 2) >> 2) % win_row_by_four][((y[0] + 2) & 3)][wbase[0] + 1];
                        }
                    }
                }

                for (int n = 0; n < NPC; n++) {
                    bool is_valid = (x[n] >= 0 && x[n] <= (COLS - 1) && y[n] >= 0 && y[n] <= (ROWS - 1));

                    if (is_in_bounds && is_valid) {
                        fraction_x[n] = (float)(x_fl[n] - x[n]);
                        fraction_y[n] = (float)(y_fl[n] - y[n]);

                    // compute cubic weights (Catmull-Rom, a = -0.5)
                    loop_weights:
                        for (int k = 0; k < 4; ++k) {
#pragma HLS UNROLL
                            float x = k - 1 - fraction_x[n];
                            float y = k - 1 - fraction_y[n];

                            x = (x < (0)) ? (-x) : x;
                            y = (y < (0)) ? (-y) : y;

                            if (x <= one)
                                weights_x[n][k] = ((one_point_five)*x - (two_point_five)) * x * x + (one);
                            else if (x < two)
                                weights_x[n][k] = (((-0.5f) * x + (two_point_five)) * x - (four)) * x + (two);
                            else
                                weights_x[n][k] = zero;

                            if (y <= one)
                                weights_y[n][k] = ((one_point_five)*y - (two_point_five)) * y * y + (one);
                            else if (y < two)
                                weights_y[n][k] = (((-0.5f) * y + (two_point_five)) * y - (four)) * y + (two);
                            else
                                weights_y[n][k] = zero;
                        }

                    loop_reset_accumulated_sum:
                        for (int ch = 0; ch < PLANES; ++ch) {
#pragma HLS UNROLL
                            accumulated_sum[n][ch] = 0;
                        }

                        for (int kr = 0; kr < 4; ++kr) {
#pragma HLS UNROLL
                            int source_y = y[n] + kr - 1;
                            // clamp source_y
                            if (source_y < 0)
                                source_y = 0;
                            else if (source_y >= ROWS)
                                source_y = ROWS - 1;

                            for (int kc = 0; kc < 4; ++kc) {
#pragma HLS UNROLL
                                int source_x = x[n] + kc - 1;
                                // clamp source_x
                                if (source_x < 0)
                                    source_x = 0;
                                else if (source_x >= (cols * NPC))
                                    source_x = (cols * NPC) - 1;

                                int word_index = (source_x >> 1) - wbase[n];
                                if (word_index < 0) word_index = 0;
                                if (word_index > 2) word_index = 2;

                                int pixel_offset = (source_x & 1) * bit_depth; // 0 or 24
                                ap_uint<48> source_word = line_buffer_words[kr][word_index][n];

                                ap_uint<24> pix24 = source_word.range(pixel_offset + bit_depth - 1, pixel_offset);

                                float combined_weight = weights_x[n][kc] * weights_y[n][kr];

                            // accumulate per channel
                            loop_accumulate:
                                for (int channel = 0; channel < PLANES; ++channel) {
#pragma HLS UNROLL
                                    float pixel_value = (pix24 >> (channel << 3)) & 0xFF;
                                    accumulated_sum[n][channel] += pixel_value * combined_weight;
                                }
                            }
                        }

                    // Write final interpolated pixel
                    loop_write_pixel:
                        for (int channel = 0; channel < PLANES; ++channel) {
#pragma HLS UNROLL

                            if (accumulated_sum[n][channel] < 0) accumulated_sum[n][channel] = 0;
                            if (accumulated_sum[n][channel] > 255) accumulated_sum[n][channel] = 255;
                            uint8_t final_value = (uint8_t)(accumulated_sum[n][channel] + (0.5f));
                            if (final_value < 0) final_value = 0;
                            if (final_value > 255) final_value = 255;
                            output_pixel.range(((n * PLANES + channel + 1) * 8) - 1, (n * PLANES + channel) * 8) =
                                final_value;
                        }
                    } else {
                        // Out of bounds -> write black/zero
                        output_pixel = 0;
                    }
                }

                dst.write(write_pointer++, output_pixel);
            }
        }
    }
}

namespace xf {
namespace cv {

template <int SRC_T,
          int DST_T,
          int PLANES,
          int MAP_T,
          int WIN_ROW,
          int ROWS,
          int COLS,
          int HLS_REMAPED_ROWS,
          int HLS_REMAPED_COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT,
          bool USE_URAM,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST>
void xFRemapNNI(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                xf::cv::Mat<DST_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_Remapped>& dst,
                xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPX>& mapx,
                xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPY>& mapy,
                uint16_t rows,
                uint16_t cols,
                uint16_t hls_remaped_rows,
                uint16_t hls_remaped_cols) {
#pragma HLS DATAFLOW

    XF_TNAME(DST_T, NPC) buf[WIN_ROW][COLS >> XF_BITSHIFT(NPC)];

    if (!USE_URAM) {
// Store the buffer in BRAM with two-port access for simultaneous read/write.
#pragma HLS bind_storage variable = buf type = RAM_T2P impl = BRAM

// Fully partition the first dimension (WIN_ROW) to allow parallel access to rows.
#pragma HLS ARRAY_PARTITION variable = buf complete dim = 1
    } else {
// Store the buffer in URAM with a one-write, multiple-read configuration.
// Reduces BRAM usage and improves memory efficiency for large images.
#pragma HLS bind_storage variable = buf type = RAM_1WNR impl = URAM latency = 1
    }

    XF_TNAME(SRC_T, NPC) s;
    int read_pointer_src = 0, read_pointer_map = 0, write_pointer = 0;

    XF_TNAME(DST_T, NPC) d;

#ifndef __SYNTHESIS__
    assert(rows <= ROWS);
    assert(cols <= COLS);
#endif

    int ISHIFT;
    if (WIN_ROW >= ROWS) {
        ISHIFT = WIN_ROW;
    } else if (WIN_ROW < ROWS) {
        ISHIFT = WIN_ROW / 2;
    }
    int r[WIN_ROW] = {};
    const int ROW_TRIPCOUNT = HLS_REMAPED_ROWS + WIN_ROW;
    const int COL_TRIPCOUNT = HLS_REMAPED_COLS / NPC;
    const int BIT_DEPTH_NPC1 = PLANES << 3;
    const int BIT_DEPTH_NPC = BIT_DEPTH_NPC1 << XF_BITSHIFT(NPC);
    int row_iteration = 0;
    if (hls_remaped_rows > rows) {
        row_iteration = hls_remaped_rows;
    } else {
        row_iteration = rows;
    }

    int col_iteration = 0;
    if (hls_remaped_cols > cols) {
        col_iteration = hls_remaped_cols;
    } else {
        col_iteration = cols;
    }

loop_height:
    for (int i = 0; i < row_iteration + ISHIFT; i++) {
// clang-format off
         #pragma HLS LOOP_TRIPCOUNT min=1 max=ROW_TRIPCOUNT
    // clang-format on

    loop_width:
        for (int j = 0; j < col_iteration; j++) {
// clang-format off
             #pragma HLS PIPELINE II=1
             #pragma HLS dependence variable=buf     inter false
             #pragma HLS dependence variable=r       inter false
             #pragma HLS LOOP_TRIPCOUNT min=1 max=COL_TRIPCOUNT
            // clang-format on

            if (i < rows && j < cols) {
                s = src.read(read_pointer_src++);
                int i_mod_WIN_ROW = i % WIN_ROW;
                buf[i_mod_WIN_ROW][j] = s;
            }

            // r[i_mod_WIN_ROW] = i;

            if (i >= ISHIFT && i < hls_remaped_rows + ISHIFT && j < hls_remaped_cols) {
                XF_TNAME(MAP_T, NPC) mx_fl;
                XF_TNAME(MAP_T, NPC) my_fl;
                XF_TNAME(MAP_T, NPC) x[2], y[2];
                bool in_range;
                mx_fl = mapx.read(read_pointer_map);
                my_fl = mapy.read(read_pointer_map++);

                for (int i = 0; i < NPC; i++) {
                    // Extract 32-bit segments from `mx_fl` and `my_fl` corresponding to pixel `i`.
                    // Convert them to floating-point, normalize by dividing by 256, and apply rounding.
                    x[i] = (int)((((float)mx_fl.range(((i + 1) << 5) - 1, i << 5)) / 256) + 0.5f);
                    y[i] = (int)((((float)my_fl.range(((i + 1) << 5) - 1, i << 5)) / 256) + 0.5f);
                }

                int y0_mod_WIN_ROW = y[0] % WIN_ROW;
                int x0_mod_NPC = x[0] % NPC;
                in_range = (y[0] >= 0 && y[0] <= (rows - 1) && x[0] >= 0 && x[0] <= ((cols << XF_BITSHIFT(NPC)) - 1));

                if (in_range) {
                    ap_uint<BIT_DEPTH_NPC> temp1[5][2];
#pragma HLS bind_storage variable = temp1 type = RAM_T2P impl = BRAM
#pragma HLS ARRAY_PARTITION variable = temp1 complete dim = 0

                    for (int m = 4; m >= 0; m--) {
#pragma HLS unroll // Fully unroll the loop for better parallelism and performance.

                        /*   Collecting 3 pixels before and 3 pixels after the selected pixel
                             along the vertical direction from the buffer for processing.*/
                        temp1[m][0] = buf[((y0_mod_WIN_ROW) + (m - 2)) % WIN_ROW][(x[0] >> XF_BITSHIFT(NPC))];
                    }

                    if ((NPC > 1) &&
                        (x[0] != x[1])) { // Check multiple pixels per clock first, then x[0] and x[1] differ

                        /*  Collecting 3 pixels before and 3 pixels after the selected pixel
                            along the vertical direction from the buffer for processing.*/
                        for (int m = 4; m >= 0; m--) {
#pragma HLS unroll // Fully unroll the loop to improve performance by parallelizing row accesses.

                            temp1[m][1] = buf[((y0_mod_WIN_ROW) + (m - 2)) % WIN_ROW][(x[0] >> XF_BITSHIFT(NPC)) + 1];
                        }
                    }
                    ap_uint<24> d_out[NPC];
                    for (int k = 0; k < NPC; k++) {
                        int x_l, z_l;

                        // Ensuring x_l is always in the valid range [0,6]
                        x_l = (y[k] % WIN_ROW) - (y0_mod_WIN_ROW) + 2;

                        if ((x[0] == 0) || ((x[k] / NPC) == (x[0] / NPC))) {
                            z_l = 0;
                        } else {
                            z_l = 1;
                        }

                        int z_l_r = (x[k] % NPC);

                        d_out[k] = temp1[x_l][z_l].range((z_l_r + 1) * BIT_DEPTH_NPC1 - 1, z_l_r * BIT_DEPTH_NPC1);
                    }
                    for (int k = 0; k < NPC; k++) {
                        d.range((k + 1) * BIT_DEPTH_NPC1 - 1, k * BIT_DEPTH_NPC1) = d_out[k];
                    }
                } else
                    d = 0;

                dst.write(write_pointer++, d);
            }
        }
    }
}

#define TWO_POW_16 65536
template <int SRC_T,
          int DST_T,
          int PLANES,
          int MAP_T,
          int WIN_ROW,
          int ROWS,
          int COLS,
          int HLS_REMAPED_ROWS,
          int HLS_REMAPED_COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT,
          bool USE_URAM>
void xFRemapLI(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
               xf::cv::Mat<DST_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_Remapped>& dst,
               xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPX>& mapx,
               xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPY>& mapy,
               uint16_t rows,
               uint16_t cols,
               uint16_t hls_remaped_rows,
               uint16_t hls_remaped_cols) {
#pragma HLS DATAFLOW

    ap_uint<48> buf_uram[(WIN_ROW >> 1) + 1][2][(COLS >> 1) + 2];

    XF_TNAME(DST_T, NPC) buf_bram[WIN_ROW + 1][(COLS >> XF_BITSHIFT(NPC)) + 1];

    if (USE_URAM) {
#pragma HLS bind_storage variable = buf_uram type = RAM_T2P impl = URAM latency = 1
#pragma HLS array_partition complete variable = buf_uram dim = 2
    } else {
#pragma HLS bind_storage variable = buf_bram type = RAM_T2P impl = BRAM
#pragma HLS array_partition complete variable = buf_bram dim = 1
    }

    // clang-format on
    XF_TNAME(SRC_T, NPC) s;

    int read_pointer_src = 0, read_pointer_map = 0, write_pointer = 0;

#ifndef __SYNTHESIS__
    assert(rows <= ROWS);
    assert(cols <= COLS);
#endif

    int ISHIFT;
    if (WIN_ROW >= ROWS) {
        ISHIFT = WIN_ROW;
    } else if (WIN_ROW < ROWS) {
        ISHIFT = WIN_ROW / 2;
    }
    int r1[WIN_ROW] = {};
    int r2[WIN_ROW] = {};
    const int ROW_TRIPCOUNT = HLS_REMAPED_ROWS + WIN_ROW;
    const int COL_TRIPCOUNT = (HLS_REMAPED_COLS / NPC) + 1;
    int row_iteration = 0;
    if (hls_remaped_rows > rows) {
        row_iteration = hls_remaped_rows;
    } else {
        row_iteration = rows;
    }

    int col_iteration = 0;
    if (hls_remaped_cols > cols) {
        col_iteration = hls_remaped_cols;
    } else {
        col_iteration = cols;
    }

    ap_uint<7> f1_msb[NPC];
    ap_uint<7> f1_lsb[NPC];

    ap_ufixed<32, 24> x_fl[NPC];
    ap_ufixed<32, 24> y_fl[NPC];

    int x_fix[NPC];
    int y_fix[NPC];

    int x[NPC];
    int y[NPC];
    int x_frac[NPC];
    int y_frac[NPC];

    ap_int<XF_DTPIXELDEPTH(MAP_T, NPC) * NPC> x_fl_int, y_fl_int;
    ap_ufixed<32, 24> x_int[NPC], y_int[NPC];
    ap_ufixed<XF_RESIZE_INTER_BITS, 0> iu[NPC], iv[NPC];

    XF_TNAME(DST_T, NPC) d;

    int xa0[NPC], xa1[NPC], ya0[NPC], ya1[NPC];
    const int BIT_DEPTH_NPC1 = PLANES << 3;
    const int BIT_DEPTH_NPC = BIT_DEPTH_NPC1 << XF_BITSHIFT(NPC);

loop_height:
    for (int i = 0; i < row_iteration + ISHIFT; i++) {
// clang-format off
         
        #pragma HLS LOOP_TRIPCOUNT min=1 max=ROW_TRIPCOUNT
        // clang-format on
        int x_a0, y_a0, y_a1;
        ap_uint<48> temp_1[3][3];
        ap_uint<48> temp_2[3][3];
        ap_uint<48> temp_buf_uram_npc1;
    loop_width:
        for (int j = 0; j < col_iteration + 2; j++) {
// clang-format off
            
             #pragma HLS PIPELINE II=1
             #pragma HLS LOOP_FLATTEN OFF
             #pragma HLS dependence variable=buf_uram     inter false
 
             #pragma HLS LOOP_TRIPCOUNT min=1 max=COL_TRIPCOUNT
            // clang-format on
            int i_mod_WIN_ROW = i % WIN_ROW;
            if (i < rows && j < cols) {
                s = src.read(read_pointer_src++);
                if (USE_URAM) {
                    if (NPC > 1) {
                        buf_uram[(i_mod_WIN_ROW) >> 1][(i_mod_WIN_ROW)&1][j] = s;
                    } else {
                        temp_buf_uram_npc1.range(((j & 1) + 1) * BIT_DEPTH_NPC - 1, (j & 1) * BIT_DEPTH_NPC) = s;
                        if (j & 1) {
                            buf_uram[(i_mod_WIN_ROW) >> 1][(i_mod_WIN_ROW)&1][j >> 1] = temp_buf_uram_npc1;
                        }
                    }
                } else {
                    buf_bram[(i_mod_WIN_ROW)][j] = s;
                }
            }

            r1[i_mod_WIN_ROW] = i;
            r2[i_mod_WIN_ROW] = i;

            if (i >= ISHIFT && i < hls_remaped_rows + ISHIFT && j < hls_remaped_cols) {
                x_fl_int = mapx.read(read_pointer_map);
                y_fl_int = mapy.read(read_pointer_map++);

                for (int n = 0; n < NPC; n++) {
#pragma HLS unroll
                    f1_msb[n] = ((n + 1) << 5) - 1;
                    f1_lsb[n] = (n << 5);

                    x_int[n] = x_fl_int.range(f1_msb[n], f1_lsb[n]);
                    y_int[n] = y_fl_int.range(f1_msb[n], f1_lsb[n]);

                    x_fl[n] = (x_int[n] >> 8);
                    y_fl[n] = (y_int[n] >> 8);

                    x_fix[n] = (x_fl[n] << XF_RESIZE_INTER_BITS); // mapx data in
                                                                  // A16.XF_RESIZE_INTER_TAB_SIZE
                                                                  // format
                    y_fix[n] = (y_fl[n] << XF_RESIZE_INTER_BITS); // mapy data in
                                                                  // A16.XF_RESIZE_INTER_TAB_SIZE
                                                                  // format

                    x[n] = x_fix[n] >> XF_RESIZE_INTER_BITS;
                    y[n] = y_fix[n] >> XF_RESIZE_INTER_BITS;
                    x_frac[n] = x_fix[n] & (XF_RESIZE_INTER_TAB_SIZE - 1);
                    y_frac[n] = y_fix[n] & (XF_RESIZE_INTER_TAB_SIZE - 1);

                    iu[n](XF_RESIZE_INTER_BITS - 1, 0) = x_frac[n];
                    iv[n](XF_RESIZE_INTER_BITS - 1, 0) = y_frac[n];
                }

#ifndef __SYNTHESIS__
                assert(((WIN_ROW & 1) == 0) && "WIN_ROW must be a multiple of two");
#endif
                int xa0_bram[NPC], xa1_bram[NPC], ya1_bram[NPC];
                for (int n = 0; n < NPC; n++) {
                    // uram
                    xa0[n] = (x[n] >> 1) + (x[n] & 1);
                    xa1[n] = (x[n] >> 1);
                    ya0[n] = ((y[n] >> 1) + (y[n] & 1)) % (ISHIFT);
                    ya1[n] = (y[n] >> 1) % (ISHIFT);

                    // bram
                    if (NPC < 2) {
                        xa0_bram[n] = (x[n] >> XF_BITSHIFT(NPC)) + 1;
                    } else {
                        xa0_bram[n] = (x[n] >> XF_BITSHIFT(NPC)) + (x[n] & 1);
                    }
                    xa1_bram[n] = (x[n] >> XF_BITSHIFT(NPC));
                    ya1_bram[n] = (y[n]) % (WIN_ROW);
                }
                ap_uint<BIT_DEPTH_NPC> temp1[7][2];
#pragma HLS bind_storage variable = temp1 type = RAM_T2P impl = BRAM
#pragma HLS ARRAY_PARTITION variable = temp1 complete dim = 0

                if (!USE_URAM) {
                    /*  Collecting 3 pixels before and 3 pixels after the selected pixel
                        along the vertical direction from the buffer for processing.*/
                    for (int m = 6; m >= 0; m--) {
#pragma HLS unroll
                        temp1[m][0] = buf_bram[(y[0] + (m - 3)) % (WIN_ROW)][xa1_bram[0]];

                        temp1[m][1] = buf_bram[(y[0] + (m - 3)) % (WIN_ROW)][xa1_bram[0] + 1];
                    }
                }

                XF_TNAME(SRC_T, NPC) d00, d01, d10, d11;
                if (USE_URAM) {
                    bool in_range_1 = (y[0] >= 0 && y_fl[0] <= (rows - 1) && x[0] >= 0 &&
                                       x_fl[0] <= ((cols << XF_BITSHIFT(NPC)) - 1));
                    if (in_range_1) {
                        if (NPC > 1) {
                            for (int i_buf = -1; i_buf <= 1; i_buf++) {
                                for (int j_buf = -1; j_buf <= 1; j_buf++) {
#pragma HLS unroll
                                    temp_1[i_buf + 1][j_buf + 1] = buf_uram[ya0[0] + i_buf][0][xa0[0] + j_buf];
                                    temp_2[i_buf + 1][j_buf + 1] = buf_uram[ya1[0] + i_buf][1][xa0[0] + j_buf];
                                }
                            }
                        } else {
                            if (!(j & 1)) {
                                x_a0 = xa0[0];
                                y_a0 = ya0[0];
                                y_a1 = ya1[0];

                                for (int i_buf = -1; i_buf <= 1; i_buf++) {
                                    for (int j_buf = -1; j_buf <= 1; j_buf++) {
#pragma HLS unroll
                                        temp_1[i_buf + 1][j_buf + 1] = buf_uram[ya0[0] + i_buf][0][xa0[0] + j_buf];
                                        temp_2[i_buf + 1][j_buf + 1] = buf_uram[ya1[0] + i_buf][1][xa0[0] + j_buf];
                                    }
                                }
                            }
                        }
                    }
                }
                for (int n = 0; n < NPC; n++) {
                    bool in_range = (y[n] >= 0 && y_fl[n] <= (rows - 1) && x[n] >= 0 &&
                                     x_fl[n] <= ((cols << XF_BITSHIFT(NPC)) - 1));
#pragma HLS unroll
                    if (in_range) {
                        if (USE_URAM) {
                            int x0, y0;
                            int x1, y1;
                            if (NPC > 1) {
                                x0 = xa0[n] - xa0[0] + 1;
                                x1 = xa1[n] - xa0[0] + 1;
                                y0 = ya0[n] - ya0[0] + 1;
                                y1 = ya1[n] - ya1[0] + 1;
                            } else {
                                x0 = xa0[n] - x_a0 + 1;
                                x1 = xa1[n] - x_a0 + 1;
                                y0 = ya0[n] - y_a0 + 1;
                                y1 = ya1[n] - y_a1 + 1;
                            }

                            d00 = temp_1[y0][x0].range(BIT_DEPTH_NPC1 - 1, 0);
                            d01 = temp_1[y0][x1].range(2 * BIT_DEPTH_NPC1 - 1, BIT_DEPTH_NPC1);
                            d10 = temp_2[y1][x0].range(BIT_DEPTH_NPC1 - 1, 0);
                            d11 = temp_2[y1][x1].range(2 * BIT_DEPTH_NPC1 - 1, BIT_DEPTH_NPC1);
                            if (x[n] & 1) {
                                int t = d00;
                                d00 = d01;
                                d01 = t;

                                int t2 = d10;
                                d10 = d11;
                                d11 = t2;
                            }
                            if (y[n] & 1) {
                                int t = d00;
                                d00 = d10;
                                d10 = t;

                                int t2 = d01;
                                d01 = d11;
                                d11 = t2;
                            }

                        } else {
                            int t_c1, t_c2;
                            int t_r1, t_r2;
                            if (xa1_bram[0] == 0) {
                                t_c1 = 1;
                                t_c2 = 0;
                            } else {
                                t_c1 = xa0_bram[n] % xa1_bram[0];
                                t_c2 = xa1_bram[n] % xa1_bram[0];
                            }

                            t_r1 = y[n] - y[0] + 3;
                            t_r2 = y[n] - y[0] + 4;

                            d00 = temp1[t_r1][t_c1].range(BIT_DEPTH_NPC1 - 1, 0);
                            d01 = temp1[t_r1][t_c2].range(BIT_DEPTH_NPC - 1, (NPC - 1) * BIT_DEPTH_NPC1);
                            d10 = temp1[t_r2][t_c1].range(BIT_DEPTH_NPC1 - 1, 0);
                            d11 = temp1[t_r2][t_c2].range(BIT_DEPTH_NPC - 1, (NPC - 1) * BIT_DEPTH_NPC1);

                            if (NPC < 2) {
                                int t = d00;
                                d00 = d01;
                                d01 = t;

                                int t2 = d10;
                                d10 = d11;
                                d11 = t2;
                            } else {
                                if (x[n] & 1) {
                                    int t = d00;
                                    d00 = d01;
                                    d01 = t;

                                    int t2 = d10;
                                    d10 = d11;
                                    d11 = t2;
                                }
                            }
                        }
                    }
                    ap_ufixed<2 * XF_RESIZE_INTER_BITS + 1, 1> k01 = (1 - iv[n]) * (iu[n]); // iu-iu*iv
                    ap_ufixed<2 * XF_RESIZE_INTER_BITS + 1, 1> k10 = (iv[n]) * (1 - iu[n]); // iv-iu*iv
                    ap_ufixed<2 * XF_RESIZE_INTER_BITS + 1, 1> k11 = (iv[n]) * (iu[n]);     // iu*iv
                    ap_ufixed<2 * XF_RESIZE_INTER_BITS + 1, 1> k00 =
                        1 - iv[n] - k01; //(1-iv)*(1-iu) = 1-iu-iv+iu*iv = 1-iv-k01
#ifndef __SYNTHESIS__
                    assert(k00 + k01 + k10 + k11 == 1);
#endif
                    int d_bit = (n * BIT_DEPTH_NPC1);
                    for (int ch = 0, bit = 0; ch < PLANES; ch++, bit += 8) {
                        XF_TNAME(SRC_T, NPC) d_range[4];
                        int d_msb = ((ch + 1) << 3) - 1;
                        int d_lsb = (ch << 3);
                        d_range[0] = d00.range(d_msb, d_lsb);
                        d_range[1] = d01.range(d_msb, d_lsb);
                        d_range[2] = d10.range(d_msb, d_lsb);
                        d_range[3] = d11.range(d_msb, d_lsb);

                        if (in_range)
                            d.range(d_bit + bit + 7, d_bit + bit) =
                                d_range[0] * k00 + d_range[1] * k01 + d_range[2] * k10 + d_range[3] * k11;
                        else
                            d.range(d_bit + 7, d_bit) = 0;
                    }
                }
                dst.write(write_pointer++, d);
            }
        }
    }
}

template <int WIN_ROWS,
          int INTERPOLATION_TYPE,
          int SRC_T,
          int MAP_T,
          int DST_T,
          int ROWS,
          int COLS,
          int HLS_REMAPED_ROWS,
          int HLS_REMAPED_COLS,
          int NPC,
          bool USE_URAM = false,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT>
void remap(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src_mat,
           xf::cv::Mat<DST_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_Remapped>& _remapped_mat,
           xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPX>& _mapx_mat,
           xf::cv::Mat<MAP_T, HLS_REMAPED_ROWS, HLS_REMAPED_COLS, NPC, XFCVDEPTH_MAPY>& _mapy_mat) {
// clang-format off
     #pragma HLS inline off

// clang-format on

#ifndef __SYNTHESIS__
    assert((MAP_T == XF_32SC1) && "The MAP_T must be XF_32SC1");
    assert(((SRC_T == XF_8UC1) || (SRC_T == XF_8UC3)) && "The SRC_T must be XF_8UC1 or XF_8UC3");
    assert(((DST_T == XF_8UC1) || (SRC_T == XF_8UC3)) && "The DST_T must be XF_8UC1 or XF_8UC3");
    assert((SRC_T == DST_T) && "Source Mat type and Destination Mat type must be the same");
    assert((NPC == XF_NPPC1) || (NPC == XF_NPPC2) || (NPC == XF_NPPC4) ||
           (NPC == XF_NPPC8) && "The NPC must be XF_NPPC1 or XF_NPPC2 or XF_NPPC4 or XF_NPPC8");
#endif

    int depth_est = WIN_ROWS * _src_mat.cols;

    uint16_t rows = _src_mat.rows;
    uint16_t cols = _src_mat.cols >> XF_BITSHIFT(NPC);
    uint16_t hls_remaped_rows = _remapped_mat.rows;
    uint16_t hls_remaped_cols = _remapped_mat.cols >> XF_BITSHIFT(NPC);
    if (INTERPOLATION_TYPE == XF_INTERPOLATION_NN) {
        xFRemapNNI<SRC_T, DST_T, XF_CHANNELS(SRC_T, NPC), MAP_T, WIN_ROWS, ROWS, COLS, HLS_REMAPED_ROWS,
                   HLS_REMAPED_COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_Remapped, XFCVDEPTH_MAPX, XFCVDEPTH_MAPY, USE_URAM,
                   XF_WORDWIDTH(SRC_T, NPC), XF_WORDWIDTH(DST_T, NPC)>(_src_mat, _remapped_mat, _mapx_mat, _mapy_mat,
                                                                       rows, cols, hls_remaped_rows, hls_remaped_cols);
    } else if (INTERPOLATION_TYPE == XF_INTERPOLATION_BILINEAR) {
        xFRemapLI<SRC_T, DST_T, XF_CHANNELS(SRC_T, NPC), MAP_T, WIN_ROWS, ROWS, COLS, HLS_REMAPED_ROWS,
                  HLS_REMAPED_COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_Remapped, XFCVDEPTH_MAPX, XFCVDEPTH_MAPY, USE_URAM>(
            _src_mat, _remapped_mat, _mapx_mat, _mapy_mat, rows, cols, hls_remaped_rows, hls_remaped_cols);
    } else if (INTERPOLATION_TYPE == XF_INTERPOLATION_BICUBIC) {
        xFRemapCI<SRC_T, DST_T, XF_CHANNELS(SRC_T, NPC), MAP_T, WIN_ROWS, ROWS, COLS, HLS_REMAPED_ROWS,
                  HLS_REMAPED_COLS, NPC, XFCVDEPTH_IN, XFCVDEPTH_Remapped, XFCVDEPTH_MAPX, XFCVDEPTH_MAPY, USE_URAM>(
            _src_mat, _remapped_mat, _mapx_mat, _mapy_mat, rows, cols, hls_remaped_rows, hls_remaped_cols);
    } else {
#ifndef __SYNTHESIS__
        assert(((INTERPOLATION_TYPE == XF_INTERPOLATION_NN) || (INTERPOLATION_TYPE == XF_INTERPOLATION_BILINEAR) ||
                (INTERPOLATION_TYPE == XF_INTERPOLATION_BICUBIC)) &&
               "The INTERPOLATION_TYPE must be either XF_INTERPOLATION_NN or "
               "XF_INTERPOLATION_BILINEAR or XF_INTERPOLATION_BICUBIC");
#endif
    }
}
} // namespace cv
} // namespace xf

#endif //_XF_REMAP_HPP_