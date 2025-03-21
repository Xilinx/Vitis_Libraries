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

namespace xf {
namespace cv {

template <int SRC_T,
          int DST_T,
          int PLANES,
          int MAP_T,
          int WIN_ROW,
          int ROWS,
          int COLS,
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT,
          bool USE_URAM,
          int WORDWIDTH_SRC,
          int WORDWIDTH_DST>
void xFRemapNNI(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
                xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_Remapped>& dst,
                xf::cv::Mat<MAP_T, ROWS, COLS, NPC, XFCVDEPTH_MAPX>& mapx,
                xf::cv::Mat<MAP_T, ROWS, COLS, NPC, XFCVDEPTH_MAPY>& mapy,
                uint16_t rows,
                uint16_t cols) {
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

    const int ISHIFT = WIN_ROW >> 1;
    int r[WIN_ROW] = {};
    const int ROW_TRIPCOUNT = ROWS + WIN_ROW;
    const int BIT_DEPTH_NPC1 = PLANES << 3;
    const int BIT_DEPTH_NPC = BIT_DEPTH_NPC1 << XF_BITSHIFT(NPC);

loop_height:
    for (int i = 0; i < rows + ISHIFT; i++) {
// clang-format off
         #pragma HLS LOOP_TRIPCOUNT min=1 max=ROW_TRIPCOUNT
    // clang-format on

    loop_width:
        for (int j = 0; j < cols; j++) {
// clang-format off
             #pragma HLS PIPELINE II=1
             #pragma HLS dependence variable=buf     inter false
             #pragma HLS dependence variable=r       inter false
             #pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
            // clang-format on

            if (i < rows) {
                s = src.read(read_pointer_src++);
            }
            int i_mod_WIN_ROW = i % WIN_ROW;
            buf[i_mod_WIN_ROW][j] = s;

            r[i_mod_WIN_ROW] = i;

            if (i >= ISHIFT) {
                XF_TNAME(MAP_T, NPC) mx_fl;
                XF_TNAME(MAP_T, NPC) my_fl;
                XF_TNAME(MAP_T, NPC) x[NPC], y[NPC];
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
                in_range = (y[0] >= 0 && y[0] <= (rows - 1) && r[y0_mod_WIN_ROW] == y[0] && x[0] >= 0 &&
                            x[0] <= ((cols << XF_BITSHIFT(NPC)) - 1));
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

                    if (x[0] != x[1] &&
                        (NPC > 1)) { // Check if x[0] and x[1] are different and multiple pixels per clock (NPC > 1).

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
          int NPC,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT,
          bool USE_URAM>
void xFRemapLI(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& src,
               xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_Remapped>& dst,
               xf::cv::Mat<MAP_T, ROWS, COLS, NPC, XFCVDEPTH_MAPX>& mapx,
               xf::cv::Mat<MAP_T, ROWS, COLS, NPC, XFCVDEPTH_MAPY>& mapy,
               uint16_t rows,
               uint16_t cols) {
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

    const int ISHIFT = WIN_ROW >> 1;
    int r1[WIN_ROW] = {};
    int r2[WIN_ROW] = {};
    const int ROW_TRIPCOUNT = ROWS + ISHIFT;

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
    for (int i = 0; i < rows + ISHIFT; i++) {
// clang-format off
         
        #pragma HLS LOOP_TRIPCOUNT min=1 max=ROW_TRIPCOUNT
        // clang-format on
        int x_a0, y_a0, y_a1;
        ap_uint<48> temp_1[3][3];
        ap_uint<48> temp_2[3][3];
        ap_uint<48> temp_buf_uram_npc1;
    loop_width:
        for (int j = 0; j < cols + 2; j++) {
// clang-format off
            
             #pragma HLS PIPELINE II=1
             #pragma HLS LOOP_FLATTEN OFF
             #pragma HLS dependence variable=buf_uram     inter false
 
             #pragma HLS LOOP_TRIPCOUNT min=1 max=(COLS/NPC)+1
            // clang-format on
            if (i < rows && j < cols) {
                s = src.read(read_pointer_src++);
            } else {
                s = 0;
            }
            int i_mod_WIN_ROW = i % WIN_ROW;
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

            r1[i_mod_WIN_ROW] = i;
            r2[i_mod_WIN_ROW] = i;

            if (i >= ISHIFT && j < cols) {
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
          int NPC,
          bool USE_URAM = false,
          int XFCVDEPTH_IN = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_Remapped = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPX = _XFCVDEPTH_DEFAULT,
          int XFCVDEPTH_MAPY = _XFCVDEPTH_DEFAULT>
void remap(xf::cv::Mat<SRC_T, ROWS, COLS, NPC, XFCVDEPTH_IN>& _src_mat,
           xf::cv::Mat<DST_T, ROWS, COLS, NPC, XFCVDEPTH_Remapped>& _remapped_mat,
           xf::cv::Mat<MAP_T, ROWS, COLS, NPC, XFCVDEPTH_MAPX>& _mapx_mat,
           xf::cv::Mat<MAP_T, ROWS, COLS, NPC, XFCVDEPTH_MAPY>& _mapy_mat) {
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

    if (INTERPOLATION_TYPE == XF_INTERPOLATION_NN) {
        xFRemapNNI<SRC_T, DST_T, XF_CHANNELS(SRC_T, NPC), MAP_T, WIN_ROWS, ROWS, COLS, NPC, XFCVDEPTH_IN,
                   XFCVDEPTH_Remapped, XFCVDEPTH_MAPX, XFCVDEPTH_MAPY, USE_URAM, XF_WORDWIDTH(SRC_T, NPC),
                   XF_WORDWIDTH(DST_T, NPC)>(_src_mat, _remapped_mat, _mapx_mat, _mapy_mat, rows, cols);
    } else if (INTERPOLATION_TYPE == XF_INTERPOLATION_BILINEAR) {
        xFRemapLI<SRC_T, DST_T, XF_CHANNELS(SRC_T, NPC), MAP_T, WIN_ROWS, ROWS, COLS, NPC, XFCVDEPTH_IN,
                  XFCVDEPTH_Remapped, XFCVDEPTH_MAPX, XFCVDEPTH_MAPY, USE_URAM>(_src_mat, _remapped_mat, _mapx_mat,
                                                                                _mapy_mat, rows, cols);
    } else {
#ifndef __SYNTHESIS__
        assert(((INTERPOLATION_TYPE == XF_INTERPOLATION_NN) || (INTERPOLATION_TYPE == XF_INTERPOLATION_BILINEAR)) &&
               "The INTERPOLATION_TYPE must be either XF_INTERPOLATION_NN or "
               "XF_INTERPOLATION_BILINEAR");
#endif
    }
}
} // namespace cv
} // namespace xf

#endif //_XF_REMAP_HPP_