/*
 * Copyright 2021 Xilinx, Inc.
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
#include "XAcc_dc.hpp"
#include "hls_math.h"
#define DEPTH_LBUFF_READ (1024)
#define NUM_BURST_READ (64)
#define LG2_NUM_BURST_READ (6)
enum {
    w1 = 2841, // 2048*sqrt(2)*cos(1*pi/16)
    w2 = 2676, // 2048*sqrt(2)*cos(2*pi/16)
    w3 = 2408, // 2048*sqrt(2)*cos(3*pi/16)
    w5 = 1609, // 2048*sqrt(2)*cos(5*pi/16)
    w6 = 1108, // 2048*sqrt(2)*cos(6*pi/16)
    w7 = 565,  // 2048*sqrt(2)*cos(7*pi/16)

    w1pw7 = w1 + w7,
    w1mw7 = w1 - w7,
    w2pw6 = w2 + w6,
    w2mw6 = w2 - w6,
    w3pw5 = w3 + w5,
    w3mw5 = w3 - w5,

    r2 = 181 // 256/sqrt(2)
};
typedef ap_int<32> idct1_t;
typedef ap_int<24> idctm_t;

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void hls_read_ddr_to_buff2(WD_AXI des[DEPTH_LBUFF_READ], WD_AXI src[MAX_COEF_AXI], unsigned long offset) {
#pragma HLS DATA_PACK variable = des
#pragma HLS DATA_PACK variable = src
    WD_AXI* p_des = des;
    WD_AXI* p_src = src;
    int num_loop = NUM_BURST_READ >> LG2_NUM_BURST_READ;
    for (int line = 0; line < num_loop; line++) {
        for (int i = 0; i < NUM_BURST_READ; i++)
#pragma HLS PIPELINE II = 1
            p_des[i] = p_src[offset + i];
        p_des += NUM_BURST_READ;
        p_src += NUM_BURST_READ;
    }
}

// ------------------------------------------------------------
void hls_read_ddr_to_buff(int in[65536], hls::stream<int>& str_out, int cnt) {
    int buff[2048];
    int pass = (cnt + 2047) / 2048;
    for (int i = 0; i < cnt * cnt; i++) {
#pragma HLS dataflow
        for (int k = 0; k < 2048; k++)
#pragma HLS pipeline
            buff[k] = in[i * 2048 + k];

        for (int k = 0; k < 2048; k++)
#pragma HLS pipeline II = 1
            str_out.write(buff[k]);
    }
}

// ------------------------------------------------------------
void hls_get_estimate_v(uint16_t block_width,
                        hls::stream<int16_t> strm_v[8],
                        hls::stream<int16_t> strm_pixels_sans_dc[8][2],
                        hls::stream<pix_edge_t>& strm_est_v) {
    pix_edge_t est_v;
    int16_t pixels_sans_dc[8][8];
    uint16_t vertical_lft[8];
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
#pragma HLS PIPELINE II = 1
        for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
            for (int j = 0; j < 2; j++) {
#pragma HLS UNROLL
                pixels_sans_dc[i][j] = strm_pixels_sans_dc[i][j].read();
            }
        }

        for (int i = 0; i < 8; ++i) {
#pragma HLS UNROLL
            int16_t a = pixels_sans_dc[i][0] + 1024;
            int16_t d = pixels_sans_dc[i][0] - pixels_sans_dc[i][1];
            int16_t b = strm_v[i].read() - (d / 2);
            est_v.data[i] = b - a;
        }
        strm_est_v.write(est_v);
    }
}

// ------------------------------------------------------------
void hls_get_estimate_h(uint16_t block_width,
                        hls::stream<int16_t> strm_h[8],
                        hls::stream<int16_t> strm_pixels_sans_dc[2][8],
                        hls::stream<pix_edge_t>& strm_est_h) {
    int16_t pixels_sans_dc[2][8];
    pix_edge_t est_h;
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
#pragma HLS PIPELINE II = 1
        for (int i = 0; i < 2; i++) {
#pragma HLS UNROLL
            for (int j = 0; j < 8; j++) {
#pragma HLS UNROLL
                pixels_sans_dc[i][j] = strm_pixels_sans_dc[i][j].read();
            }
        }
        for (int i = 0; i < 8; ++i) {
#pragma HLS UNROLL
            int16_t a = pixels_sans_dc[0][i] + 1024;
            int16_t d = pixels_sans_dc[0][i] - pixels_sans_dc[1][i];
            int16_t b = strm_h[i].read() - d / 2;
            est_h.data[i] = b - a;
        }
        strm_est_h.write(est_h);
    }
}

// ------------------------------------------------------------
int16_t hls_adv_predict_dc_pix(bool left_present,
                               bool above_present,
                               uint16_t q0,
                               hls::stream<pix_edge_t>& str_est_v,
                               hls::stream<pix_edge_t>& str_est_h,
                               int32_t* uncertainty_val,
                               int32_t* uncertainty2_val) {
#pragma HLS inline
    int16_t avgmed = 0;
    int16_t avg_h_v[2] = {0, 0};
#pragma HLS ARRAY_PARTITION variable = avg_h_v complete dim = 0
    int16_t min_dc = 32767;
    int16_t max_dc = -32768;
    int16_t min_dc_v = 32767;
    int16_t max_dc_v = -32768;
    int16_t min_dc_h = 32767;
    int16_t max_dc_h = -32768;
    pix_edge_t est_v = str_est_v.read();
    pix_edge_t est_h = str_est_h.read();
    /*
    ADV_PREDICT1:
        for (int i = 0; i < 8; ++i) {
    #pragma HLS UNROLL
            min_dc_v = min_dc_v > est_v.data[i] ? est_v.data[i] : min_dc_v;
            max_dc_v = max_dc_v < est_v.data[i] ? est_v.data[i] : max_dc_v;
        }
    ADV_PREDICT2:
        for (int i = 0; i < 8; ++i) {
    #pragma HLS UNROLL
            min_dc_h = min_dc_h > est_h.data[i] ? est_h.data[i] : min_dc_h;
            max_dc_h = max_dc_h < est_h.data[i] ? est_h.data[i] : max_dc_h;
        }
    */

    int16_t min_dc_v_0, min_dc_v_1, min_dc_v_2, min_dc_v_3, min_dc_v_4, min_dc_v_5, min_dc_v_6;
    int16_t max_dc_v_0, max_dc_v_1, max_dc_v_2, max_dc_v_3, max_dc_v_4, max_dc_v_5, max_dc_v_6;
    int16_t min_dc_h_0, min_dc_h_1, min_dc_h_2, min_dc_h_3, min_dc_h_4, min_dc_h_5, min_dc_h_6;
    int16_t max_dc_h_0, max_dc_h_1, max_dc_h_2, max_dc_h_3, max_dc_h_4, max_dc_h_5, max_dc_h_6;
    min_dc_v_0 = hls::min(est_v.data[0], est_v.data[1]);
    min_dc_v_1 = hls::min(est_v.data[2], est_v.data[3]);
    min_dc_v_2 = hls::min(est_v.data[4], est_v.data[5]);
    min_dc_v_3 = hls::min(est_v.data[6], est_v.data[7]);
    min_dc_v_4 = hls::min(min_dc_v_0, min_dc_v_1);
    min_dc_v_5 = hls::min(min_dc_v_2, min_dc_v_3);
    min_dc_v = hls::min(min_dc_v_4, min_dc_v_5);
    max_dc_v_0 = hls::max(est_v.data[0], est_v.data[1]);
    max_dc_v_1 = hls::max(est_v.data[2], est_v.data[3]);
    max_dc_v_2 = hls::max(est_v.data[4], est_v.data[5]);
    max_dc_v_3 = hls::max(est_v.data[6], est_v.data[7]);
    max_dc_v_4 = hls::max(max_dc_v_0, max_dc_v_1);
    max_dc_v_5 = hls::max(max_dc_v_2, max_dc_v_3);
    max_dc_v = hls::max(max_dc_v_4, max_dc_v_5);
    min_dc_h_0 = hls::min(est_h.data[0], est_h.data[1]);
    min_dc_h_1 = hls::min(est_h.data[2], est_h.data[3]);
    min_dc_h_2 = hls::min(est_h.data[4], est_h.data[5]);
    min_dc_h_3 = hls::min(est_h.data[6], est_h.data[7]);
    min_dc_h_4 = hls::min(min_dc_h_0, min_dc_h_1);
    min_dc_h_5 = hls::min(min_dc_h_2, min_dc_h_3);
    min_dc_h = hls::min(min_dc_h_4, min_dc_h_5);
    max_dc_h_0 = hls::max(est_h.data[0], est_h.data[1]);
    max_dc_h_1 = hls::max(est_h.data[2], est_h.data[3]);
    max_dc_h_2 = hls::max(est_h.data[4], est_h.data[5]);
    max_dc_h_3 = hls::max(est_h.data[6], est_h.data[7]);
    max_dc_h_4 = hls::max(max_dc_h_0, max_dc_h_1);
    max_dc_h_5 = hls::max(max_dc_h_2, max_dc_h_3);
    max_dc_h = hls::max(max_dc_h_4, max_dc_h_5);

    int16_t sum_v = est_v.data[0] + est_v.data[1] + est_v.data[2] + est_v.data[3] + est_v.data[4] + est_v.data[5] +
                    est_v.data[6] + est_v.data[7];
    int16_t sum_h = est_h.data[0] + est_h.data[1] + est_h.data[2] + est_h.data[3] + est_h.data[4] + est_h.data[5] +
                    est_h.data[6] + est_h.data[7];

    if (left_present && above_present) {
        min_dc = min_dc_v < min_dc_h ? min_dc_v : min_dc_h;
        max_dc = max_dc_v > max_dc_h ? max_dc_v : max_dc_h;
        avg_h_v[0] = sum_v;
        avg_h_v[1] = sum_h;
    } else if (left_present && !above_present) {
        min_dc = min_dc_v;
        max_dc = max_dc_v;
        avg_h_v[0] = sum_v;
        avg_h_v[1] = sum_v;
    } else if (!left_present && above_present) {
        min_dc = min_dc_h;
        max_dc = max_dc_h;
        avg_h_v[0] = sum_h;
        avg_h_v[1] = sum_h;
    }

    if (left_present || above_present) {
        avgmed = (avg_h_v[0] + avg_h_v[1]) >> 1;
        *uncertainty_val = (max_dc - min_dc) >> 3;
        avg_h_v[0] -= avgmed;
        avg_h_v[1] -= avgmed;
        *uncertainty2_val = _MACRO_ABS(avg_h_v[0]) < _MACRO_ABS(avg_h_v[1]) ? (avg_h_v[0] >> 3) : (avg_h_v[1] >> 3);
    }

    //    int16_t tmp;
    //    #pragma HLS resource core=divider_ip variable=tmp
    int16_t tmp = avgmed / q0;
    int16_t tmp2 = (tmp + 4) >> 3;
    ;
    return tmp2;
    // return ((avgmed / q0 + 4) >> 3);
}

// ------------------------------------------------------------
void hls_idct_h(uint16_t block_width,
                hls::stream<coef_t> str_rast8[8],
                const uint8_t q[8][8],
                hls::stream<idctm_t> strm_intermed[8][8]) {
    bool ignore_dc = true;
#pragma HLS ARRAY_PARTITION variable = str_rast8 complete dim = 1
#pragma HLS ARRAY_PARTITION variable = q complete dim = 2

    // Horizontal 1-D IDCT.
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
        for (int y = 0; y < 8; ++y) {
#pragma HLS PIPELINE II = 1
            coef_t c0 = str_rast8[0].read();
            int y8 = y * 8;
            int32_t x0 = (((ignore_dc && y == 0) ? 0 : (c0 * q[y][0]) << 11)) + 128;
            int32_t x1 = (str_rast8[4].read() * q[y][4]) << 11;
            int32_t x2 = str_rast8[6].read() * q[y][6];
            int32_t x3 = str_rast8[2].read() * q[y][2];
            int32_t x4 = str_rast8[1].read() * q[y][1];
            int32_t x5 = str_rast8[7].read() * q[y][7];
            int32_t x6 = str_rast8[5].read() * q[y][5];
            int32_t x7 = str_rast8[3].read() * q[y][3];

            // Prescale.

            // Stage 1.
            int32_t x8 = w7 * (x4 + x5);
            x4 = x8 + w1mw7 * x4;
            x5 = x8 - w1pw7 * x5;
            x8 = w3 * (x6 + x7);
            x6 = x8 - w3mw5 * x6;
            x7 = x8 - w3pw5 * x7;

            // Stage 2.
            x8 = x0 + x1;
            x0 -= x1;
            x1 = w6 * (x3 + x2);
            x2 = x1 - w2pw6 * x2;
            x3 = x1 + w2mw6 * x3;
            x1 = x4 + x6;
            x4 -= x6;
            x6 = x5 + x7;
            x5 -= x7;

            // Stage 3.
            x7 = x8 + x3;
            x8 -= x3;
            x3 = x0 + x2;
            x0 -= x2;
            x2 = (r2 * (x4 + x5) + 128) >> 8;
            x4 = (r2 * (x4 - x5) + 128) >> 8;

            // Stage 4.

            strm_intermed[y][0].write((x7 + x1) >> 8);
            strm_intermed[y][1].write((x3 + x2) >> 8);
            strm_intermed[y][2].write((x0 + x4) >> 8);
            strm_intermed[y][3].write((x8 + x6) >> 8);
            strm_intermed[y][4].write((x8 - x6) >> 8);
            strm_intermed[y][5].write((x0 - x4) >> 8);
            strm_intermed[y][6].write((x3 - x2) >> 8);
            strm_intermed[y][7].write((x7 - x1) >> 8);
        }
    }
}

// ------------------------------------------------------------
void hls_idct_v(int16_t block_width,
                hls::stream<idctm_t> strm_intermed[8][8],
                hls::stream<int16_t> strm_outp0[8][2],
                hls::stream<int16_t> strm_outp1[2][8],
                hls::stream<int16_t> strm_outp2[8][8]) {
    // Vertical 1-D IDCT.
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
        for (int32_t x = 0; x < 8; ++x) {
#pragma HLS PIPELINE II = 1
            // clang-format off
			// Similar to the horizontal 1-D IDCT case, if all the AC components are zero, then the IDCT is trivial.
			// However, after performing the horizontal 1-D IDCT, there are typically non-zero AC components, so
			// we do not bother to check for the all-zero case.
            // clang-format on

            // Prescale.
            int32_t y0 = (strm_intermed[0][x].read() << 8) + 8192;
            int32_t y1 = strm_intermed[4][x].read() << 8;
            int32_t y2 = strm_intermed[6][x].read();
            int32_t y3 = strm_intermed[2][x].read();
            int32_t y4 = strm_intermed[1][x].read();
            int32_t y5 = strm_intermed[7][x].read();
            int32_t y6 = strm_intermed[5][x].read();
            int32_t y7 = strm_intermed[3][x].read();

            // Stage 1.
            int32_t y8 = w7 * (y4 + y5) + 4;
            y4 = (y8 + w1mw7 * y4) >> 3;
            y5 = (y8 - w1pw7 * y5) >> 3;
            y8 = w3 * (y6 + y7) + 4;
            y6 = (y8 - w3mw5 * y6) >> 3;
            y7 = (y8 - w3pw5 * y7) >> 3;

            // Stage 2.
            y8 = y0 + y1;
            y0 -= y1;
            y1 = w6 * (y3 + y2) + 4;
            y2 = (y1 - w2pw6 * y2) >> 3;
            y3 = (y1 + w2mw6 * y3) >> 3;
            y1 = y4 + y6;
            y4 -= y6;
            y6 = y5 + y7;
            y5 -= y7;

            // Stage 3.
            y7 = y8 + y3;
            y8 -= y3;
            y3 = y0 + y2;
            y0 -= y2;
            y2 = (r2 * (y4 + y5) + 128) >> 8;
            y4 = (r2 * (y4 - y5) + 128) >> 8;

            // Stage 4.
            if (x < 2) {
                strm_outp0[0][x].write((y7 + y1) >> 11);
                strm_outp0[1][x].write((y3 + y2) >> 11);
                strm_outp0[2][x].write((y0 + y4) >> 11);
                strm_outp0[3][x].write((y8 + y6) >> 11);
                strm_outp0[4][x].write((y8 - y6) >> 11);
                strm_outp0[5][x].write((y0 - y4) >> 11);
                strm_outp0[6][x].write((y3 - y2) >> 11);
                strm_outp0[7][x].write((y7 - y1) >> 11);
            }

            strm_outp1[0][x].write((y7 + y1) >> 11);
            strm_outp1[1][x].write((y3 + y2) >> 11);
            /*			strm_outp1[2][x].write((y0 + y4) >> 11);
                                    strm_outp1[3][x].write((y8 + y6) >> 11);
                                    strm_outp1[4][x].write((y8 - y6) >> 11);
                                    strm_outp1[5][x].write((y0 - y4) >> 11);
                                    strm_outp1[6][x].write((y3 - y2) >> 11);
                                    strm_outp1[7][x].write((y7 - y1) >> 11);*/

            strm_outp2[0][x].write((y7 + y1) >> 11);
            strm_outp2[1][x].write((y3 + y2) >> 11);
            strm_outp2[2][x].write((y0 + y4) >> 11);
            strm_outp2[3][x].write((y8 + y6) >> 11);
            strm_outp2[4][x].write((y8 - y6) >> 11);
            strm_outp2[5][x].write((y0 - y4) >> 11);
            strm_outp2[6][x].write((y3 - y2) >> 11);
            strm_outp2[7][x].write((y7 - y1) >> 11);
        }
    }
}

// ------------------------------------------------------------
void hls_set_edge_here(uint16_t block_width,
                       hls::stream<coef_t> strm_vertical_left[8],
                       hls::stream<int16_t> strm_here_h[8],
                       hls::stream<int16_t> strm_data[8][8],
                       uint8_t quantization_table_0,
                       hls::stream<coef_t>& str_dc,
                       // output
                       hls::stream<coef_t>& str_dc2) {
    int16_t data_buf[8][8];
#pragma HLS ARRAY_PARTITION variable = data_buf complete

    static int16_t left[8];
#pragma HLS ARRAY_PARTITION variable = left complete

    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
        for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
            for (int j = 0; j < 8; j++) {
#pragma HLS UNROLL
                data_buf[i][j] = strm_data[i][j].read();
            }
        }

        int16_t dc = str_dc.read();
        str_dc2.write(dc);
        for (int i = 0; i < 8; ++i) {
#pragma HLS PIPELINE II = 1
            int delta = data_buf[7][i] - data_buf[6][i];
            strm_here_h[i].write(dc * quantization_table_0 + data_buf[7][i] + 128 * 8 + (delta / 2));
            int delta2 = data_buf[i][7] - data_buf[i][6];
            strm_vertical_left[i].write(left[i]);
            left[i] = dc * quantization_table_0 + data_buf[i][7] + 128 * 8 + (delta2 / 2);
        }
    }
}

// ------------------------------------------------------------
void lb_ctrl_dc(ap_uint<2> id_cmp,
                uint16_t block_width,
                bool above_present,
                hls::stream<coef_t> strm_in[8],
                hls::stream<coef_t> strm_out[8]) {
    // clang-format off
	static int16_t array_edge_above_uram_low[MAX_NUM_COLOR][MAX_NUM_BLOCK88_W][4];
#pragma HLS bind_storage variable=array_edge_above_uram_low type=RAM_2P impl=URAM
#pragma HLS ARRAY_RESHAPE variable=array_edge_above_uram_low complete dim=3

	static int16_t array_edge_above_uram_high[MAX_NUM_COLOR][MAX_NUM_BLOCK88_W][4];
#pragma HLS bind_storage variable=array_edge_above_uram_high type=RAM_2P impl=URAM
#pragma HLS ARRAY_RESHAPE variable=array_edge_above_uram_high complete dim=3
    // clang-format on

    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
#pragma HLS PIPELINE II = 1
        strm_out[0].write(array_edge_above_uram_low[id_cmp][jpeg_x][0]);
        strm_out[1].write(array_edge_above_uram_low[id_cmp][jpeg_x][1]);
        strm_out[2].write(array_edge_above_uram_low[id_cmp][jpeg_x][2]);
        strm_out[3].write(array_edge_above_uram_low[id_cmp][jpeg_x][3]);
        strm_out[4].write(array_edge_above_uram_high[id_cmp][jpeg_x][0]);
        strm_out[5].write(array_edge_above_uram_high[id_cmp][jpeg_x][1]);
        strm_out[6].write(array_edge_above_uram_high[id_cmp][jpeg_x][2]);
        strm_out[7].write(array_edge_above_uram_high[id_cmp][jpeg_x][3]);

        array_edge_above_uram_low[id_cmp][jpeg_x][0] = strm_in[0].read();
        array_edge_above_uram_low[id_cmp][jpeg_x][1] = strm_in[1].read();
        array_edge_above_uram_low[id_cmp][jpeg_x][2] = strm_in[2].read();
        array_edge_above_uram_low[id_cmp][jpeg_x][3] = strm_in[3].read();
        array_edge_above_uram_high[id_cmp][jpeg_x][0] = strm_in[4].read();
        array_edge_above_uram_high[id_cmp][jpeg_x][1] = strm_in[5].read();
        array_edge_above_uram_high[id_cmp][jpeg_x][2] = strm_in[6].read();
        array_edge_above_uram_high[id_cmp][jpeg_x][3] = strm_in[7].read();
    }
}

// ------------------------------------------------------------
void hls_dc_stage1(uint16_t block_width,
                   bool above_present,

                   hls::stream<coef_t> str_rast8[8],
                   uint8_t q_tables0[MAX_NUM_COLOR][8][8],
                   uint8_t q0,
                   ap_uint<2> id_cmp,
                   hls::stream<coef_t>& str_dc_in,
                   // output
                   hls::stream<coef_t>& str_dc_out,
                   hls::stream<pix_edge_t>& str_est_v,
                   hls::stream<pix_edge_t>& str_est_h) {
#pragma HLS INLINE
#pragma HLS dataflow
    // clang-format off
	static hls::stream<idctm_t> strm_intermed[8][8];
#pragma HLS STREAM variable=strm_intermed depth=2
#pragma HLS ARRAY_PARTITION variable=strm_intermed complete dim=0

	static hls::stream<int16_t> strm_outp_sans_dc0[8][2];
#pragma HLS STREAM variable=strm_outp_sans_dc0 depth=4
#pragma HLS ARRAY_PARTITION variable=strm_outp_sans_dc0 complete dim=0

	static hls::stream<int16_t> strm_outp_sans_dc1[2][8];
#pragma HLS STREAM variable=strm_outp_sans_dc1 depth=4
#pragma HLS ARRAY_PARTITION variable=strm_outp_sans_dc1 complete dim=1

	static hls::stream<int16_t> strm_outp_sans_dc2[8][8];
#pragma HLS STREAM variable=strm_outp_sans_dc2 depth=4
#pragma HLS ARRAY_PARTITION variable=strm_outp_sans_dc2 complete dim=0
    // clang-format on

    hls_idct_h(block_width, str_rast8, q_tables0[id_cmp], strm_intermed);
    hls_idct_v(block_width, strm_intermed, strm_outp_sans_dc0, strm_outp_sans_dc1, strm_outp_sans_dc2);

    // clang-format off
	static hls::stream< coef_t> strm_lb_in[8];
#pragma HLS STREAM variable=strm_lb_in depth=32 dim=1
#pragma HLS bind_storage variable=strm_lb_in type=FIFO impl=LUTRAM
	static hls::stream< coef_t> strm_lb_out[8];
#pragma HLS STREAM variable=strm_lb_out depth=32 dim=1
#pragma HLS bind_storage variable=strm_lb_out type=FIFO impl=LUTRAM

	static hls::stream< coef_t> strm_vertical_left[8];
#pragma HLS STREAM variable=strm_vertical_left depth=32 dim=1
#pragma HLS bind_storage variable=strm_vertical_left type=FIFO impl=LUTRAM
    // clang-format on

    hls_set_edge_here(block_width, strm_vertical_left, strm_lb_in, strm_outp_sans_dc2, q0, str_dc_in, str_dc_out);
    lb_ctrl_dc(id_cmp, block_width, above_present, strm_lb_in, strm_lb_out);

    hls_get_estimate_v(block_width, strm_vertical_left, strm_outp_sans_dc0, str_est_v);
    hls_get_estimate_h(block_width, strm_lb_out, strm_outp_sans_dc1, str_est_h);
}

// ------------------------------------------------------------
int16_t hls_adv_predict_or_unpredict_dc(int16_t saved_dc, bool recover_original, int16_t predicted_val) {
    int16_t max_value = (1 << (MAX_EXPONENT_PIX - 1));
    int16_t min_value = -max_value;
    int16_t adjustment_factor = 2 * max_value + 1;
    int16_t retval = predicted_val;
    retval = saved_dc + (recover_original ? retval : -retval);
    if (retval < min_value) retval += adjustment_factor;
    if (retval > max_value) retval -= adjustment_factor;
    return retval;
}

// ------------------------------------------------------------
void dc_push_bit(uint16_t block_width,

                 hls::stream<int16_t>& strm_coef,
                 hls::stream<int>& strm_uncertainty,
                 hls::stream<int>& strm_uncertainty2,

                 hls::stream<ap_uint<4> >& strm_sel_tab,
                 hls::stream<bool>& strm_cur_bit,
                 hls::stream<bool>& strm_e,
                 hls::stream<ap_uint<16> >& strm_addr1,
                 hls::stream<ap_uint<16> >& strm_addr2,
                 hls::stream<ap_uint<16> >& strm_addr3

                 ) {
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
        int16_t coef = strm_coef.read();
        int uncertainty = strm_uncertainty.read();
        int uncertainty2 = strm_uncertainty2.read();

        ap_int<16> abs_coef = _MACRO_ABS(coef);
        uint8_t length = 16 - abs_coef.countLeadingZeros();
        ap_int<16> tmp1 = _MACRO_ABS(uncertainty);
        ap_int<16> tmp2 = _MACRO_ABS(uncertainty2);
        uint16_t len_abs_mxm = len_abs_mxm = (16 - tmp1.countLeadingZeros());
        uint16_t len_abs_offset_to_closest_edge = (16 - tmp2.countLeadingZeros());
        ap_uint<4> addr_2 = _MACRO_MIN(len_abs_mxm, 11); //(uint16_t)(Model::ExponentCountsDC::size0 - 1));
        ap_uint<5> addr_1 =
            _MACRO_MIN(len_abs_offset_to_closest_edge, 16); //(uint16_t)(Model::ExponentCountsDC::size1 - 1));
        ap_uint<4> addr_0;
    CUR_BIT_OUT1:
        for (unsigned int i = 0; i < length + 1; ++i) {
#pragma HLS PIPELINE II = 1
            addr_0 = i;
            bool cur_bit = (length != i);
            strm_sel_tab.write(EXP_CNT_DC);
            strm_cur_bit.write(cur_bit);
            strm_e.write(false);
            strm_addr1.write(addr_2);
            strm_addr2.write(addr_1);
            strm_addr3.write(i);
        }

        if (length != 0) {
            ap_uint<2> addr_1 = 0;
            ap_uint<4> addr_0 = uncertainty2 >= 0 ? uncertainty2 == 0 ? 3 : 2 : 1;
            strm_sel_tab.write(SIGN_CNT);
            strm_cur_bit.write(coef >= 0);
            strm_e.write(false);
            strm_addr1.write(addr_1);
            strm_addr2.write(addr_0);
            strm_addr3.write(0);
        }
        if (length > 1) {
            ap_uint<4> addr_1 = _MACRO_MIN(11, len_abs_mxm); // std::min((uint16_t)(11), len_abs_mxm);
        CUR_BIT_OUT2:
            for (int i = length - 2; i >= 0; --i) {
#pragma HLS PIPELINE II = 1
                ap_uint<4> addr_0 = i;

                strm_sel_tab.write(NOIS_CNT_DC);
                strm_cur_bit.write(abs_coef[i]);
                strm_e.write(false);
                strm_addr1.write(addr_1);
                strm_addr2.write(i);
                strm_addr3.write(0);
            }
        }
        strm_e.write(true);
    }
}

// ------------------------------------------------------------
void dc_push_bit_v2(uint16_t block_width,

                    hls::stream<int16_t>& strm_coef,
                    hls::stream<int>& strm_uncertainty,
                    hls::stream<int>& strm_uncertainty2,

                    hls::stream<ap_uint<4> >& strm_sel_tab,
                    hls::stream<bool>& strm_cur_bit,
                    hls::stream<short>& strm_len,
                    //		hls::stream<bool>		 & strm_e,
                    hls::stream<ap_uint<16> >& strm_addr1,
                    hls::stream<ap_uint<16> >& strm_addr2,
                    hls::stream<ap_uint<16> >& strm_addr3

                    ) {
    int state;
    int cnt = 0;
    int i = 0;
    int j = 0;
    int jpeg_x = 0;
    int16_t coef;
    int uncertainty;
    int uncertainty2;

    ap_int<16> abs_coef;
    uint8_t length;
    ap_int<16> tmp1;
    ap_int<16> tmp2;
    uint16_t len_abs_mxm;
    uint16_t len_abs_offset_to_closest_edge;

    ap_uint<4> addr_2;
    ap_uint<5> addr_1;
    ap_uint<4> addr_0;

    coef = strm_coef.read();
    uncertainty = strm_uncertainty.read();
    uncertainty2 = strm_uncertainty2.read();

    abs_coef = _MACRO_ABS(coef);
    length = 16 - abs_coef.countLeadingZeros();
    tmp1 = _MACRO_ABS(uncertainty);
    tmp2 = _MACRO_ABS(uncertainty2);
    len_abs_mxm = len_abs_mxm = (16 - tmp1.countLeadingZeros());
    len_abs_offset_to_closest_edge = (16 - tmp2.countLeadingZeros());
    addr_2 = _MACRO_MIN(len_abs_mxm, 11);                    //(uint16_t)(Model::ExponentCountsDC::size0 - 1));
    addr_1 = _MACRO_MIN(len_abs_offset_to_closest_edge, 16); //(uint16_t)(Model::ExponentCountsDC::size1 - 1));
    i = 0;
    state = 1;

    while (jpeg_x < block_width) {
#pragma HLS PIPELINE II = 1
        if (state == 1) {
            addr_0 = i;
            bool cur_bit = (length != i);
            strm_sel_tab.write(EXP_CNT_DC);
            strm_cur_bit.write(cur_bit);
            //                strm_e.write(false);
            cnt++;
            strm_addr1.write(addr_2);
            strm_addr2.write(addr_1);
            strm_addr3.write(i);
            if (i == length && length != 0)
                state = 2;
            else if (i == length && length == 0) {
                strm_len.write(cnt);
                cnt = 0;
                jpeg_x++;
                if (jpeg_x < block_width) {
                    coef = strm_coef.read();
                    uncertainty = strm_uncertainty.read();
                    uncertainty2 = strm_uncertainty2.read();

                    abs_coef = _MACRO_ABS(coef);
                    length = 16 - abs_coef.countLeadingZeros();
                    tmp1 = _MACRO_ABS(uncertainty);
                    tmp2 = _MACRO_ABS(uncertainty2);
                    len_abs_mxm = len_abs_mxm = (16 - tmp1.countLeadingZeros());
                    len_abs_offset_to_closest_edge = (16 - tmp2.countLeadingZeros());
                    addr_2 = _MACRO_MIN(len_abs_mxm, 11); //(uint16_t)(Model::ExponentCountsDC::size0 - 1));
                    addr_1 = _MACRO_MIN(len_abs_offset_to_closest_edge,
                                        16); //(uint16_t)(Model::ExponentCountsDC::size1 - 1));
                    i = 0;
                    state = 1;
                }
            } else {
                i++;
            }
        } else if (state == 2) {
            addr_1 = 0;
            addr_0 = uncertainty2 >= 0 ? uncertainty2 == 0 ? 3 : 2 : 1;
            strm_sel_tab.write(SIGN_CNT);
            strm_cur_bit.write(coef >= 0);
            //                strm_e.write(false);
            cnt++;
            strm_addr1.write(addr_1);
            strm_addr2.write(addr_0);
            strm_addr3.write(0);

            if (length > 1) {
                j = length - 2;
                state = 4;
            } else {
                strm_len.write(cnt);
                cnt = 0;
                jpeg_x++;
                if (jpeg_x < block_width) {
                    coef = strm_coef.read();
                    uncertainty = strm_uncertainty.read();
                    uncertainty2 = strm_uncertainty2.read();

                    abs_coef = _MACRO_ABS(coef);
                    length = 16 - abs_coef.countLeadingZeros();
                    tmp1 = _MACRO_ABS(uncertainty);
                    tmp2 = _MACRO_ABS(uncertainty2);
                    len_abs_mxm = len_abs_mxm = (16 - tmp1.countLeadingZeros());
                    len_abs_offset_to_closest_edge = (16 - tmp2.countLeadingZeros());
                    addr_2 = _MACRO_MIN(len_abs_mxm, 11); //(uint16_t)(Model::ExponentCountsDC::size0 - 1));
                    addr_1 = _MACRO_MIN(len_abs_offset_to_closest_edge,
                                        16); //(uint16_t)(Model::ExponentCountsDC::size1 - 1));
                    i = 0;
                    state = 1;
                }
            }
        } else if (state == 4) {
            addr_1 = _MACRO_MIN(11, len_abs_mxm);

            strm_sel_tab.write(NOIS_CNT_DC);
            strm_cur_bit.write(abs_coef[j]);
            //                    strm_e.write(false);
            cnt++;
            strm_addr1.write(addr_1);
            strm_addr2.write(j);
            strm_addr3.write(0);

            if (j == 0) {
                strm_len.write(cnt);
                cnt = 0;
                jpeg_x++;
                if (jpeg_x < block_width) {
                    coef = strm_coef.read();
                    uncertainty = strm_uncertainty.read();
                    uncertainty2 = strm_uncertainty2.read();

                    abs_coef = _MACRO_ABS(coef);
                    length = 16 - abs_coef.countLeadingZeros();
                    tmp1 = _MACRO_ABS(uncertainty);
                    tmp2 = _MACRO_ABS(uncertainty2);
                    len_abs_mxm = len_abs_mxm = (16 - tmp1.countLeadingZeros());
                    len_abs_offset_to_closest_edge = (16 - tmp2.countLeadingZeros());
                    addr_2 = _MACRO_MIN(len_abs_mxm, 11); //(uint16_t)(Model::ExponentCountsDC::size0 - 1));
                    addr_1 = _MACRO_MIN(len_abs_offset_to_closest_edge,
                                        16); //(uint16_t)(Model::ExponentCountsDC::size1 - 1));
                    i = 0;
                    state = 1;
                }
            }
            j--;
        }
    }
}

// ------------------------------------------------------------
void dc_out(uint16_t block_width,
            bool above_present,
            hls::stream<coef_t>& str_dc_in,
            uint16_t q0,
            hls::stream<pix_edge_t>& str_est_v,
            hls::stream<pix_edge_t>& str_est_h,
            hls::stream<int16_t>& strm_coef,
            hls::stream<int>& strm_uncertainty,
            hls::stream<int>& strm_uncertainty2) {
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
#pragma HLS PIPELINE II = 1

        int16_t dc = str_dc_in.read();
        int uncertainty = 0; // this is how far off our max estimate vs min estimate is
        int uncertainty2 = 0;
        int predicted_val;
        bool left = (jpeg_x == 0) ? false : (block_width > 1);
        // clang-format off
            predicted_val = hls_adv_predict_dc_pix(
                                                                    left,
                                                                    above_present,
                                                                    q0,
                                                                    str_est_v,
                                                                    str_est_h,
                                                                    &uncertainty,
                                                                    &uncertainty2);
        // clang-format on
        int16_t adv_predicted_dc = hls_adv_predict_or_unpredict_dc(dc, false, predicted_val);
        strm_coef.write(adv_predicted_dc);
        strm_uncertainty.write(uncertainty);
        strm_uncertainty2.write(uncertainty2);
    }
}

// ------------------------------------------------------------
void jpeg_zigzag_to_array(uint16_t block_width, hls::stream<coef_t> strm_in[8], coef_t coef_buff[64]) {
    for (int j = 0; j < 8; j++) {
#pragma HLS PIPELINE II = 1
        for (int i = 0; i < 8; i++) {
#pragma HLS UNROLL
            coef_buff[8 * j + i] = strm_in[i].read();
        }
    }
}

// ------------------------------------------------------------
void array_to_raster(uint16_t block_width, coef_t coef_buff[64], hls::stream<coef_t> strm_out[8]) {
    int cnt = 0;
    while (cnt < 8) {
#pragma HLS PIPELINE II = 1
        if (cnt == 0) {
            strm_out[0].write(coef_buff[0]);
            strm_out[1].write(coef_buff[1]);
            strm_out[2].write(coef_buff[5]);
            strm_out[3].write(coef_buff[6]);
            strm_out[4].write(coef_buff[14]);
            strm_out[5].write(coef_buff[15]);
            strm_out[6].write(coef_buff[27]);
            strm_out[7].write(coef_buff[28]);
        } else if (cnt == 1) {
            strm_out[0].write(coef_buff[2]);
            strm_out[1].write(coef_buff[4]);
            strm_out[2].write(coef_buff[7]);
            strm_out[3].write(coef_buff[13]);
            strm_out[4].write(coef_buff[16]);
            strm_out[5].write(coef_buff[26]);
            strm_out[6].write(coef_buff[29]);
            strm_out[7].write(coef_buff[42]);
        } else if (cnt == 2) {
            strm_out[0].write(coef_buff[3]);
            strm_out[1].write(coef_buff[8]);
            strm_out[2].write(coef_buff[12]);
            strm_out[3].write(coef_buff[17]);
            strm_out[4].write(coef_buff[25]);
            strm_out[5].write(coef_buff[30]);
            strm_out[6].write(coef_buff[41]);
            strm_out[7].write(coef_buff[43]);
        } else if (cnt == 3) {
            strm_out[0].write(coef_buff[9]);
            strm_out[1].write(coef_buff[11]);
            strm_out[2].write(coef_buff[18]);
            strm_out[3].write(coef_buff[24]);
            strm_out[4].write(coef_buff[31]);
            strm_out[5].write(coef_buff[40]);
            strm_out[6].write(coef_buff[44]);
            strm_out[7].write(coef_buff[53]);
        } else if (cnt == 4) {
            strm_out[0].write(coef_buff[10]);
            strm_out[1].write(coef_buff[19]);
            strm_out[2].write(coef_buff[23]);
            strm_out[3].write(coef_buff[32]);
            strm_out[4].write(coef_buff[39]);
            strm_out[5].write(coef_buff[45]);
            strm_out[6].write(coef_buff[52]);
            strm_out[7].write(coef_buff[54]);
        } else if (cnt == 5) {
            strm_out[0].write(coef_buff[20]);
            strm_out[1].write(coef_buff[22]);
            strm_out[2].write(coef_buff[33]);
            strm_out[3].write(coef_buff[38]);
            strm_out[4].write(coef_buff[46]);
            strm_out[5].write(coef_buff[51]);
            strm_out[6].write(coef_buff[55]);
            strm_out[7].write(coef_buff[60]);
        } else if (cnt == 6) {
            strm_out[0].write(coef_buff[21]);
            strm_out[1].write(coef_buff[34]);
            strm_out[2].write(coef_buff[37]);
            strm_out[3].write(coef_buff[47]);
            strm_out[4].write(coef_buff[50]);
            strm_out[5].write(coef_buff[56]);
            strm_out[6].write(coef_buff[59]);
            strm_out[7].write(coef_buff[61]);
        } else if (cnt == 7) {
            strm_out[0].write(coef_buff[35]);
            strm_out[1].write(coef_buff[36]);
            strm_out[2].write(coef_buff[48]);
            strm_out[3].write(coef_buff[49]);
            strm_out[4].write(coef_buff[57]);
            strm_out[5].write(coef_buff[58]);
            strm_out[6].write(coef_buff[62]);
            strm_out[7].write(coef_buff[63]);
        }
        cnt++;
    }
}

// ------------------------------------------------------------
void jpeg_zigzag_to_raster(uint16_t block_width, hls::stream<coef_t> strm_in[8], hls::stream<coef_t> strm_out[8]) {
//#pragma HLS INLINE
#pragma HLS DATAFLOW

    coef_t coef_buff[64];
#pragma HLS ARRAY_PARTITION variable = coef_buff complete dim = 0

    jpeg_zigzag_to_array(block_width, strm_in, coef_buff);
    array_to_raster(block_width, coef_buff, strm_out);
}

// ------------------------------------------------------------
void hls_serialize_tokens_dc(bool above_present,
                             ap_uint<2> id_cmp,
                             uint16_t block_width,
                             uint8_t q_tables0[MAX_NUM_COLOR][8][8],
                             uint8_t q0,

                             hls::stream<coef_t> strm_in[8],
                             hls::stream<coef_t>& str_dc_in,

                             hls::stream<ap_uint<4> >& strm_sel_tab,
                             hls::stream<bool>& strm_cur_bit,
                             hls::stream<short>& strm_len,
                             //		hls::stream<bool>		 & strm_e,
                             hls::stream<ap_uint<16> >& strm_addr1,
                             hls::stream<ap_uint<16> >& strm_addr2,
                             hls::stream<ap_uint<16> >& strm_addr3

                             ) {
#pragma HLS INLINE
#pragma HLS DATAFLOW
    // clang-format off
    hls::stream< coef_t> str_dc2;
#pragma HLS STREAM variable=str_dc2 depth=32 dim=1
#pragma HLS bind_storage variable=str_dc2 type=FIFO impl=LUTRAM

	hls::stream< pix_edge_t> str_est_v;
#pragma HLS STREAM variable=str_est_v depth=32 dim=1
#pragma HLS bind_storage variable=str_est_v type=FIFO impl=LUTRAM

	hls::stream< pix_edge_t> str_est_h;
#pragma HLS STREAM variable=str_est_h depth=32 dim=1
#pragma HLS bind_storage variable=str_est_h type=FIFO impl=LUTRAM

    hls::stream<int16_t> strm_coef;
#pragma HLS STREAM variable=strm_coef depth=32 dim=1
#pragma HLS bind_storage variable=strm_coef type=FIFO impl=LUTRAM

    hls::stream<int>     strm_uncertainty("uncertainty");
#pragma HLS bind_storage variable=strm_uncertainty type=FIFO impl=LUTRAM
#pragma HLS STREAM variable=strm_uncertainty depth=32 dim=1

    hls::stream<int>     strm_uncertainty2("uncertainty2");
#pragma HLS bind_storage variable=strm_uncertainty2 type=FIFO impl=LUTRAM
#pragma HLS STREAM variable=strm_uncertainty2 depth=32 dim=1

	hls::stream< coef_t>  str_rast8[8];
#pragma HLS bind_storage variable=str_rast8 type=FIFO impl=LUTRAM
#pragma HLS ARRAY_PARTITION variable=str_rast8 complete dim=1
#pragma HLS STREAM variable=strm_in depth=32 dim=1
    // clang-format on
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
        jpeg_zigzag_to_raster(block_width, strm_in, str_rast8);
    }
#pragma HLS ARRAY_PARTITION variable = q_tables0 complete dim = 2

    hls_dc_stage1(block_width, above_present, str_rast8, q_tables0, q0, id_cmp,

                  str_dc_in, str_dc2,
                  // Output
                  str_est_v, str_est_h);

    dc_out(block_width, above_present, str_dc2, q0, str_est_v, str_est_h, strm_coef, strm_uncertainty,
           strm_uncertainty2);

    dc_push_bit_v2(block_width, strm_coef, strm_uncertainty, strm_uncertainty2,

                   strm_sel_tab, strm_cur_bit, strm_len,
                   //		strm_e,
                   strm_addr1, strm_addr2, strm_addr3

                   );
}

// ------------------------------------------------------------
void pre_serialize_tokens_dc(bool above_present,
                             ap_uint<2> id_cmp,
                             uint16_t block_width,
                             uint8_t q_tables0[MAX_NUM_COLOR][8][8],
                             uint8_t q0,

                             hls::stream<coef_t> strm_in[8],
                             hls::stream<coef_t>& str_dc_in,

                             hls::stream<int16_t>& strm_coef,
                             hls::stream<int>& strm_uncertainty,
                             hls::stream<int>& strm_uncertainty2

                             ) {
#pragma HLS INLINE
#pragma HLS DATAFLOW
    // clang-format off
    hls::stream< coef_t> str_dc2;
#pragma HLS STREAM variable=str_dc2 depth=32 dim=1
#pragma HLS bind_storage variable=str_dc2 type=FIFO impl=LUTRAM

	hls::stream< pix_edge_t> str_est_v;
#pragma HLS STREAM variable=str_est_v depth=32 dim=1
#pragma HLS bind_storage variable=str_est_v type=FIFO impl=LUTRAM

	hls::stream< pix_edge_t> str_est_h;
#pragma HLS STREAM variable=str_est_h depth=32 dim=1
#pragma HLS bind_storage variable=str_est_h type=FIFO impl=LUTRAM

	hls::stream< coef_t>  str_rast8[8];
#pragma HLS bind_storage variable=str_rast8 type=FIFO impl=LUTRAM
#pragma HLS ARRAY_PARTITION variable=str_rast8 complete dim=1
#pragma HLS STREAM variable=strm_in depth=32 dim=1
    // clang-format on
    for (int jpeg_x = 0; jpeg_x + 0 < block_width; jpeg_x++) {
        jpeg_zigzag_to_raster(block_width, strm_in, str_rast8);
    }
#pragma HLS ARRAY_PARTITION variable = q_tables0 complete dim = 2

    hls_dc_stage1(block_width, above_present, str_rast8, q_tables0, q0, id_cmp,

                  str_dc_in, str_dc2,
                  // Output
                  str_est_v, str_est_h);

    dc_out(block_width, above_present, str_dc2, q0, str_est_v, str_est_h, strm_coef, strm_uncertainty,
           strm_uncertainty2);
}

} // namespace details
} // namespace codec
} // namespace xf
