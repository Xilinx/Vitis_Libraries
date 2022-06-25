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
#include "XAcc_77.hpp"

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void line_buf_write_h(ap_uint<32> len, hls::stream<ap_int<11> > coef[7], hls::stream<ap_uint<77> >& lb_write) {
    ap_int<11> coef_h_buff[7];
#pragma HLS ARRAY_PARTITION variable = coef_h_buff complete dim = 1

    for (int j = 0; j < len; j++) {
#pragma HLS pipeline II = 1
        for (int i = 0; i < 7; i++) {
#pragma HLS unroll
            coef_h_buff[i] = coef[i].read();
        }

        lb_write.write((coef_h_buff[6], coef_h_buff[5], coef_h_buff[4], coef_h_buff[3], coef_h_buff[2], coef_h_buff[1],
                        coef_h_buff[0]));
    }
}

// ------------------------------------------------------------
void line_buf_ctrl_h(ap_uint<32> len,
                     ap_uint<3> id_cmp,
                     bool is_top_row,
                     hls::stream<ap_uint<77> >& strm_write,
                     hls::stream<ap_uint<77> >& strm_read) {
    static ap_uint<72> coef_abv_uram[3][1024];
#pragma HLS bind_storage variable = coef_abv_uram type = RAM_2P impl = URAM
    static ap_uint<5> coef_abv_bram[3][1024];

    ap_uint<77> coef_abv_r;
    ap_uint<77> coef_abv_w;

    ap_uint<32> cnt = 0;

    while (cnt < len) {
#pragma HLS pipeline II = 1
        if (!is_top_row) {
            coef_abv_r(76, 5) = coef_abv_uram[id_cmp][cnt];
            coef_abv_r(4, 0) = coef_abv_bram[id_cmp][cnt];
            strm_read.write(coef_abv_r);
        }

        coef_abv_w = strm_write.read();
        coef_abv_uram[id_cmp][cnt] = coef_abv_w(76, 5);
        coef_abv_bram[id_cmp][cnt] = coef_abv_w(4, 0);

        cnt++;
    }
}

// ------------------------------------------------------------
void line_buf_ctrl_nz(ap_uint<32> len,
                      ap_uint<3> id_cmp,
                      bool is_top_row,
                      hls::stream<ap_uint<6> >& strm_write,
                      hls::stream<ap_uint<6> >& strm_read) {
    static ap_uint<6> nz_abv_ram[3][1024];
    ap_uint<6> nz = 0;
    ap_uint<32> cnt = 0;

    while (cnt < len) {
#pragma HLS pipeline II = 1
        if (!is_top_row) {
            strm_read.write(nz_abv_ram[id_cmp][cnt]);
        } else {
            strm_read.write(0);
        }

        nz_abv_ram[id_cmp][cnt] = strm_write.read();
        cnt++;
    }
}

// ------------------------------------------------------------
void line_buf_ctrl_77(ap_uint<32> len,
                      ap_uint<3> id_cmp,
                      bool is_top_row,
                      hls::stream<ap_uint<77> >& strm_write,
                      hls::stream<ap_uint<77> >& strm_read) {
    static ap_uint<72> coef_abv_uram[3][7168];
#pragma HLS bind_storage variable = coef_abv_uram type = RAM_2P impl = URAM
    static ap_uint<5> coef_abv_bram[3][7168];

    ap_uint<77> coef_abv_r;
    ap_uint<77> coef_abv_w;

    ap_uint<32> cnt = 0;

    while (cnt < len * 7) {
#pragma HLS pipeline II = 1
        if (!is_top_row) {
            coef_abv_r(76, 5) = coef_abv_uram[id_cmp][cnt];
            coef_abv_r(4, 0) = coef_abv_bram[id_cmp][cnt];
            strm_read.write(coef_abv_r);
        }

        coef_abv_w = strm_write.read();
        coef_abv_uram[id_cmp][cnt] = coef_abv_w(76, 5);
        coef_abv_bram[id_cmp][cnt] = coef_abv_w(4, 0);

        cnt++;
    }
}

// ------------------------------------------------------------
void line_buf_read_77(ap_uint<32> len,
                      bool is_top,

                      hls::stream<ap_uint<77> >& strm_read,

                      hls::stream<ap_int<11> > coef_abv[49],
                      hls::stream<ap_int<11> > coef_abv_h[49]) {
    ap_int<11> abv_buff[49];
#pragma HLS array_partition variable = abv_buff complete dim = 0
    ap_int<77> lb_read;

    ap_uint<32> cnt = 0;
    ap_uint<32> i = 0;

    while (i < len * 7) {
#pragma HLS pipeline II = 1
        if (!is_top)
            lb_read = strm_read.read();
        else
            lb_read = 0;

        for (int j = 0; j < 7; j++) {
#pragma HLS unroll
            abv_buff[cnt * 7 + j] = lb_read(j * 11 + 10, j * 11);
        }

        i++;
        if (cnt != 6)
            cnt++;
        else {
            for (int j = 0; j < 49; j++) {
#pragma HLS unroll
                coef_abv[j].write(abv_buff[j]);
                coef_abv_h[j].write(abv_buff[j]);
            }
            cnt = 0;
        }
    }
}

// ------------------------------------------------------------
void store_7x7(ap_uint<32> len,
               hls::stream<ap_int<11> > coef[8],

               hls::stream<ap_uint<6> >& non_zero_cnt,
               hls::stream<ap_uint<3> >& eob_x,
               hls::stream<ap_uint<3> >& eob_y,
               hls::stream<ap_int<11> > coef_out[49],
               hls::stream<ap_int<11> > coef_out_h[49],
               hls::stream<ap_int<11> > coef_out_v[49],

               hls::stream<ap_uint<77> >& lb_write,
               hls::stream<ap_uint<6> >& lb_nz_write) {
    ap_int<11> reg_coef_7x7[8];
    ap_int<11> lb_w_coef[7];

    ap_int<11> coef_7x7_buff[49];
#pragma HLS array_partition variable = coef_7x7_buff complete dim = 0

    ap_uint<3> reg_eob_x;
    ap_uint<3> reg_eob_y;
    ap_uint<1> non_zero_h[7] = {0, 0, 0, 0, 0, 0, 0};
    ap_uint<1> non_zero_v[7] = {0, 0, 0, 0, 0, 0, 0};
    ap_uint<6> reg_non_zero_cnt = 0;
    int block_cnt = 0;
    int cnt = 0;

READ_8_COEF7x7_LOOP:
    while (block_cnt < len) {
#pragma HLS pipeline II = 1

        if (cnt == 0) {
            for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                reg_coef_7x7[j] = coef[j].read();
                coef_7x7_buff[8 * cnt + j] = reg_coef_7x7[j];
                reg_non_zero_cnt = reg_non_zero_cnt + (reg_coef_7x7[j].or_reduce());
            }
            non_zero_h[0] = (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[1].or_reduce()) ||
                            (reg_coef_7x7[5].or_reduce()) || (reg_coef_7x7[6].or_reduce());
            non_zero_h[1] =
                (reg_coef_7x7[2].or_reduce()) || (reg_coef_7x7[4].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_h[2] = (reg_coef_7x7[3].or_reduce());

            non_zero_v[0] =
                (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[2].or_reduce()) || (reg_coef_7x7[3].or_reduce());
            non_zero_v[1] = (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[4].or_reduce());
            non_zero_v[2] = (reg_coef_7x7[5].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_v[3] = (reg_coef_7x7[6].or_reduce());

            lb_write.write((reg_coef_7x7[6], reg_coef_7x7[5], reg_coef_7x7[4], reg_coef_7x7[3], reg_coef_7x7[2],
                            reg_coef_7x7[1], reg_coef_7x7[0]));
            lb_w_coef[0] = reg_coef_7x7[7];

            cnt++;

        }

        else if (cnt == 1) {
            for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                reg_coef_7x7[j] = coef[j].read();
                coef_7x7_buff[8 * cnt + j] = reg_coef_7x7[j];
                reg_non_zero_cnt = reg_non_zero_cnt + (reg_coef_7x7[j].or_reduce());
            }
            non_zero_h[0] = non_zero_h[0] || (reg_coef_7x7[6].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_h[1] = non_zero_h[1] || (reg_coef_7x7[5].or_reduce());
            non_zero_h[2] = non_zero_h[2] || (reg_coef_7x7[0] || reg_coef_7x7[4].or_reduce());
            non_zero_h[3] = (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[3].or_reduce());
            non_zero_h[4] = (reg_coef_7x7[2].or_reduce());

            non_zero_v[0] = non_zero_v[0] || (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[2].or_reduce());
            non_zero_v[1] = non_zero_v[1] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[3].or_reduce());
            non_zero_v[2] = non_zero_v[2] || (reg_coef_7x7[4].or_reduce());
            non_zero_v[3] = non_zero_v[3] || (reg_coef_7x7[5].or_reduce());
            non_zero_v[4] = (reg_coef_7x7[6].or_reduce());
            non_zero_v[5] = (reg_coef_7x7[7].or_reduce());

            lb_write.write((reg_coef_7x7[5], reg_coef_7x7[4], reg_coef_7x7[3], reg_coef_7x7[2], reg_coef_7x7[1],
                            reg_coef_7x7[0], lb_w_coef[0]));
            lb_w_coef[0] = reg_coef_7x7[6];
            lb_w_coef[1] = reg_coef_7x7[7];

            cnt++;
        }

        else if (cnt == 2) {
            for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                reg_coef_7x7[j] = coef[j].read();
                coef_7x7_buff[8 * cnt + j] = reg_coef_7x7[j];
                reg_non_zero_cnt = reg_non_zero_cnt + (reg_coef_7x7[j].or_reduce());
            }
            non_zero_h[1] = non_zero_h[1] || (reg_coef_7x7[0].or_reduce());
            non_zero_h[2] = non_zero_h[2] || (reg_coef_7x7[1].or_reduce());
            non_zero_h[3] = non_zero_h[3] || (reg_coef_7x7[2].or_reduce());
            non_zero_h[4] = non_zero_h[4] || (reg_coef_7x7[3].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_h[5] = (reg_coef_7x7[4].or_reduce()) || (reg_coef_7x7[6].or_reduce());
            non_zero_h[6] = (reg_coef_7x7[5].or_reduce());

            non_zero_v[0] = non_zero_v[0] || (reg_coef_7x7[4].or_reduce()) || (reg_coef_7x7[5].or_reduce());
            non_zero_v[1] = non_zero_v[1] || (reg_coef_7x7[3].or_reduce()) || (reg_coef_7x7[6].or_reduce());
            non_zero_v[2] = non_zero_v[2] || (reg_coef_7x7[2].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_v[3] = non_zero_v[3] || (reg_coef_7x7[1].or_reduce());
            non_zero_v[4] = non_zero_v[4] || (reg_coef_7x7[0].or_reduce());

            lb_write.write((reg_coef_7x7[4], reg_coef_7x7[3], reg_coef_7x7[2], reg_coef_7x7[1], reg_coef_7x7[0],
                            lb_w_coef[1], lb_w_coef[0]));
            lb_w_coef[0] = reg_coef_7x7[5];
            lb_w_coef[1] = reg_coef_7x7[6];
            lb_w_coef[2] = reg_coef_7x7[7];

            cnt++;
        }

        else if (cnt == 3) {
            for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                reg_coef_7x7[j] = coef[j].read();
                coef_7x7_buff[8 * cnt + j] = reg_coef_7x7[j];
                reg_non_zero_cnt = reg_non_zero_cnt + (reg_coef_7x7[j].or_reduce());
            }
            non_zero_h[0] = non_zero_h[0] || (reg_coef_7x7[3].or_reduce());
            non_zero_h[1] = non_zero_h[1] || (reg_coef_7x7[2].or_reduce()) || (reg_coef_7x7[4].or_reduce());
            non_zero_h[2] = non_zero_h[2] || (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[5].or_reduce());
            non_zero_h[3] = non_zero_h[3] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[6].or_reduce());
            non_zero_h[4] = non_zero_h[4] || (reg_coef_7x7[7].or_reduce());

            non_zero_v[3] = non_zero_v[3] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_v[4] = non_zero_v[4] || (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[6].or_reduce());
            non_zero_v[5] = non_zero_v[5] || (reg_coef_7x7[2].or_reduce()) || (reg_coef_7x7[5].or_reduce());
            non_zero_v[6] = (reg_coef_7x7[3].or_reduce()) || (reg_coef_7x7[4].or_reduce());

            lb_write.write((reg_coef_7x7[3], reg_coef_7x7[2], reg_coef_7x7[1], reg_coef_7x7[0], lb_w_coef[2],
                            lb_w_coef[1], lb_w_coef[0]));
            lb_w_coef[0] = reg_coef_7x7[4];
            lb_w_coef[1] = reg_coef_7x7[5];
            lb_w_coef[2] = reg_coef_7x7[6];
            lb_w_coef[3] = reg_coef_7x7[7];

            cnt++;
        }

        else if (cnt == 4) {
            for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                reg_coef_7x7[j] = coef[j].read();
                coef_7x7_buff[8 * cnt + j] = reg_coef_7x7[j];
                reg_non_zero_cnt = reg_non_zero_cnt + (reg_coef_7x7[j].or_reduce());
            }
            non_zero_h[2] = non_zero_h[2] || (reg_coef_7x7[6].or_reduce());
            non_zero_h[3] = non_zero_h[3] || (reg_coef_7x7[5].or_reduce()) || (reg_coef_7x7[7].or_reduce());
            non_zero_h[4] = non_zero_h[4] || (reg_coef_7x7[4].or_reduce());
            non_zero_h[5] = non_zero_h[5] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[3].or_reduce());
            non_zero_h[6] = non_zero_h[6] || (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[2].or_reduce());

            non_zero_v[1] = non_zero_v[1] || (reg_coef_7x7[1].or_reduce());
            non_zero_v[2] = non_zero_v[2] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[2].or_reduce());
            non_zero_v[3] = non_zero_v[3] || (reg_coef_7x7[3].or_reduce());
            non_zero_v[4] = non_zero_v[4] || (reg_coef_7x7[4].or_reduce());
            non_zero_v[5] = non_zero_v[5] || (reg_coef_7x7[5].or_reduce());
            non_zero_v[6] = non_zero_v[6] || (reg_coef_7x7[6].or_reduce()) || (reg_coef_7x7[7].or_reduce());

            lb_write.write((reg_coef_7x7[2], reg_coef_7x7[1], reg_coef_7x7[0], lb_w_coef[3], lb_w_coef[2], lb_w_coef[1],
                            lb_w_coef[0]));
            lb_w_coef[0] = reg_coef_7x7[3];
            lb_w_coef[1] = reg_coef_7x7[4];
            lb_w_coef[2] = reg_coef_7x7[5];
            lb_w_coef[3] = reg_coef_7x7[6];
            lb_w_coef[4] = reg_coef_7x7[7];

            cnt++;
        }

        else if (cnt == 5) {
            for (int j = 0; j < 8; j++) {
#pragma HLS unroll
                reg_coef_7x7[j] = coef[j].read();
                coef_7x7_buff[8 * cnt + j] = reg_coef_7x7[j];
                reg_non_zero_cnt = reg_non_zero_cnt + (reg_coef_7x7[j].or_reduce());
            }
            non_zero_h[4] = non_zero_h[4] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[5].or_reduce());
            non_zero_h[5] = non_zero_h[5] || (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[4].or_reduce()) ||
                            (reg_coef_7x7[6].or_reduce());
            non_zero_h[6] = non_zero_h[6] || (reg_coef_7x7[2].or_reduce()) || (reg_coef_7x7[3].or_reduce()) ||
                            (reg_coef_7x7[7].or_reduce());

            non_zero_v[3] = non_zero_v[3] || (reg_coef_7x7[2].or_reduce());
            non_zero_v[4] = non_zero_v[4] || (reg_coef_7x7[1].or_reduce()) || (reg_coef_7x7[3].or_reduce());
            non_zero_v[5] = non_zero_v[5] || (reg_coef_7x7[0].or_reduce()) || (reg_coef_7x7[4].or_reduce()) ||
                            (reg_coef_7x7[7].or_reduce());
            non_zero_v[6] = non_zero_v[6] || (reg_coef_7x7[5].or_reduce()) || (reg_coef_7x7[6].or_reduce());

            lb_write.write((reg_coef_7x7[1], reg_coef_7x7[0], lb_w_coef[4], lb_w_coef[3], lb_w_coef[2], lb_w_coef[1],
                            lb_w_coef[0]));
            lb_w_coef[0] = reg_coef_7x7[2];
            lb_w_coef[1] = reg_coef_7x7[3];
            lb_w_coef[2] = reg_coef_7x7[4];
            lb_w_coef[3] = reg_coef_7x7[5];
            lb_w_coef[4] = reg_coef_7x7[6];
            lb_w_coef[5] = reg_coef_7x7[7];

            cnt++;
        }

        else if (cnt == 6) {
            coef_7x7_buff[48] = coef[0].read();

            lb_write.write((coef_7x7_buff[48], lb_w_coef[5], lb_w_coef[4], lb_w_coef[3], lb_w_coef[2], lb_w_coef[1],
                            lb_w_coef[0]));

            non_zero_h[6] = non_zero_h[6] || (coef_7x7_buff[48].or_reduce());
            non_zero_v[6] = non_zero_v[6] || (coef_7x7_buff[48].or_reduce());
            if (coef_7x7_buff[48].or_reduce()) reg_non_zero_cnt = reg_non_zero_cnt + 1;

            non_zero_cnt.write(reg_non_zero_cnt);
            lb_nz_write.write(reg_non_zero_cnt);

            /*
                        for(int i=0;i<7;i++){
            #pragma HLS unroll
                            for(int j=0;j<7;j++){
            #pragma HLS unroll
                                non_zero_h[i]=non_zero_h[i]||(coef_7x7_buff[unzigzag_7x7[7*i+j]].or_reduce());
                                non_zero_v[j]=non_zero_v[j]||(coef_7x7_buff[unzigzag_7x7[7*i+j]].or_reduce());
                            }
                        }*/

            if (non_zero_h[6] == 1)
                reg_eob_y = 7;
            else if (non_zero_h[5] == 1)
                reg_eob_y = 6;
            else if (non_zero_h[4] == 1)
                reg_eob_y = 5;
            else if (non_zero_h[3] == 1)
                reg_eob_y = 4;
            else if (non_zero_h[2] == 1)
                reg_eob_y = 3;
            else if (non_zero_h[1] == 1)
                reg_eob_y = 2;
            else if (non_zero_h[0] == 1)
                reg_eob_y = 1;
            else
                reg_eob_y = 0;

            if (non_zero_v[6] == 1)
                reg_eob_x = 7;
            else if (non_zero_v[5] == 1)
                reg_eob_x = 6;
            else if (non_zero_v[4] == 1)
                reg_eob_x = 5;
            else if (non_zero_v[3] == 1)
                reg_eob_x = 4;
            else if (non_zero_v[2] == 1)
                reg_eob_x = 3;
            else if (non_zero_v[1] == 1)
                reg_eob_x = 2;
            else if (non_zero_v[0] == 1)
                reg_eob_x = 1;
            else
                reg_eob_x = 0;

            eob_x.write(reg_eob_x);
            eob_y.write(reg_eob_y);
            for (int i = 0; i < 49; i++) {
#pragma HLS unroll
                coef_out[i].write(coef_7x7_buff[i]);
                coef_out_h[i].write(coef_7x7_buff[i]);
                coef_out_v[i].write(coef_7x7_buff[i]);
            }
            block_cnt++;
            cnt = 0;
            reg_non_zero_cnt = 0;
        }
    }
}

// ------------------------------------------------------------

void forward_7x7(ap_uint<32> len,
                 hls::stream<ap_int<11> > coef[49],
                 hls::stream<ap_int<11> > coef_above[49],
                 hls::stream<ap_uint<6> >& non_zero_cnt_in,

                 hls::stream<ap_int<11> >& coef_7x7,
                 hls::stream<ap_int<11> >& coef_lft,
                 hls::stream<ap_int<11> >& coef_abv,
                 hls::stream<ap_int<11> >& coef_abv_lft,
                 hls::stream<ap_uint<6> >& non_zero_cnt_out,
                 hls::stream<ap_uint<6> >& non_zero_cnt_lft_out,
                 hls::stream<ap_uint<6> >& non_zero_7x7_out,
                 hls::stream<ap_uint<6> >& non_zero_h_out
                 //    hls::stream<ap_uint<6> >& non_zero_v_out
                 ) {
    ap_uint<6> reg_non_zero = 0;
    ap_uint<6> nz_lft = 0;
    ap_int<11> coef_buff[49];
#pragma HLS array_partition variable = coef_buff complete dim = 0
    ap_int<11> coef_lft_buff[49];
#pragma HLS array_partition variable = coef_lft_buff complete dim = 0
    ap_int<11> coef_abv_buff[49];
#pragma HLS array_partition variable = coef_abv_buff complete dim = 0
    ap_int<11> coef_abv_left_buff[49];
#pragma HLS array_partition variable = coef_abv_left_buff complete dim = 0

    // for(int j=0;j<len;j++){
    // 1. init
    for (int i = 0; i < 49; i++) {
#pragma HLS unroll
        coef_lft_buff[i] = coef_buff[i];
        coef_buff[i] = coef[i].read();
        coef_abv_left_buff[i] = coef_abv_buff[i];
        coef_abv_buff[i] = coef_above[i].read();
    }
    non_zero_cnt_lft_out.write(nz_lft);
    reg_non_zero = non_zero_cnt_in.read();
    non_zero_cnt_out.write(reg_non_zero);
    non_zero_7x7_out.write(reg_non_zero);
    non_zero_h_out.write(reg_non_zero);
    nz_lft = reg_non_zero;

    int j = 0;
    int i = 0;

PUSH_COEF7x7_LOOP:
    while (j < len) {
#pragma HLS pipeline II = 1

        //        for(int i=0;reg_non_zero.or_reduce();i++){

        //        	_XF_IMAGE_PRINT("Forw num = %d , reduce = %d, %d\n",\
//        			(int)reg_non_zero, (int)reg_non_zero.or_reduce(), (int)coef_buff[i].or_reduce());

        // 2. write out
        if (reg_non_zero.or_reduce()) {
            coef_7x7.write(coef_buff[i]);
            coef_abv.write(coef_abv_buff[i]);
            if (j != 0) {
                coef_lft.write(coef_lft_buff[i]);
                coef_abv_lft.write(coef_abv_left_buff[i]);
            } else {
                coef_lft.write(0);
                coef_abv_lft.write(0);
            }
            if (coef_buff[i].or_reduce()) reg_non_zero--;
            i++;
        } // loop0

        // 3. update
        if (!reg_non_zero.or_reduce()) {
            i = 0;
            if (j < len - 1) {
                for (int i = 0; i < 49; i++) {
#pragma HLS unroll
                    coef_lft_buff[i] = coef_buff[i];
                    coef_buff[i] = coef[i].read();
                    coef_abv_left_buff[i] = coef_abv_buff[i];
                    coef_abv_buff[i] = coef_above[i].read();
                }
                non_zero_cnt_lft_out.write(nz_lft);
                reg_non_zero = non_zero_cnt_in.read();
                non_zero_cnt_out.write(reg_non_zero);
                non_zero_7x7_out.write(reg_non_zero);
                non_zero_h_out.write(reg_non_zero);
                nz_lft = reg_non_zero;
            }
            j++;
        }
    }
}

// ------------------------------------------------------------
void push_h_edge(ap_uint<32> len,
                 bool is_top_row,
                 hls::stream<ap_int<11> > coef[7],
                 hls::stream<ap_int<11> > coef_77_here[49],
                 hls::stream<ap_int<11> > coef_77_above[49],
                 hls::stream<ap_int<11> > coef_here_h[8],
                 hls::stream<ap_int<11> > coef_abov_h[8],
                 hls::stream<bool>& has_left,
                 hls::stream<bool>& coef_end,

                 hls::stream<ap_uint<3> >& coef_cnt_h_len,
                 hls::stream<ap_uint<3> >& lane_h,
                 hls::stream<ap_uint<77> >& lb_read) {
    ap_int<11> coef_buff[49];
#pragma HLS array_partition variable = coef_buff complete dim = 0
    ap_int<11> coef_buff_abv[49];
#pragma HLS array_partition variable = coef_buff complete dim = 0
    ap_int<11> coef_h_buff[7];
#pragma HLS ARRAY_PARTITION variable = coef_h_buff complete dim = 1
    ap_int<11> coef_abv_h_buff[7];
#pragma HLS ARRAY_PARTITION variable = coef_abv_h_buff complete dim = 1
    ap_uint<3> reg_coef_cnt_h = 0;

    ap_uint<77> lb_read_reg;

    // 1.init
    for (int i = 0; i < 49; i++) {
#pragma HLS unroll
        coef_buff[i] = coef_77_here[i].read();
        coef_buff_abv[i] = coef_77_above[i].read();
    }

    for (int i = 0; i < 7; i++) {
#pragma HLS unroll
        coef_h_buff[i] = coef[i].read();
        reg_coef_cnt_h = reg_coef_cnt_h + (coef_h_buff[i].or_reduce());
    }
    coef_cnt_h_len.write(reg_coef_cnt_h);

    if (!is_top_row) {
        lb_read_reg = lb_read.read();
    } else {
        lb_read_reg = 0;
    }

// 2.loop
PUSH_HOR_EDGE_LOOP:
    int cnt = 0;
    int j = 0;
    while (j < len) {
#pragma HLS pipeline II = 1

        // PUSH_HOR_EDGE_LOOP:
        // for(int i=0;reg_coef_cnt_h.or_reduce();i++){

        //        	_XF_IMAGE_PRINT("edge_h num = %d , reduce = %d, %d\n",\
//        			(int)reg_coef_cnt_h, (int)reg_coef_cnt_h.or_reduce(), (int)coef_h_buff[cnt].or_reduce() );
        if (reg_coef_cnt_h.or_reduce()) {
            lane_h.write(cnt);
            coef_end.write(false);

            if (j == 0)
                has_left.write(true);
            else
                has_left.write(false);

            if (cnt == 0) {
                coef_here_h[1].write(coef_buff[0]);
                coef_here_h[2].write(coef_buff[2]);
                coef_here_h[3].write(coef_buff[3]);
                coef_here_h[4].write(coef_buff[9]);
                coef_here_h[5].write(coef_buff[10]);
                coef_here_h[6].write(coef_buff[20]);
                coef_here_h[7].write(coef_buff[21]);

                coef_abov_h[1].write(coef_buff_abv[0]);
                coef_abov_h[2].write(coef_buff_abv[2]);
                coef_abov_h[3].write(coef_buff_abv[3]);
                coef_abov_h[4].write(coef_buff_abv[9]);
                coef_abov_h[5].write(coef_buff_abv[10]);
                coef_abov_h[6].write(coef_buff_abv[20]);
                coef_abov_h[7].write(coef_buff_abv[21]);
            } else if (cnt == 1) {
                coef_here_h[1].write(coef_buff[1]);
                coef_here_h[2].write(coef_buff[4]);
                coef_here_h[3].write(coef_buff[8]);
                coef_here_h[4].write(coef_buff[11]);
                coef_here_h[5].write(coef_buff[19]);
                coef_here_h[6].write(coef_buff[22]);
                coef_here_h[7].write(coef_buff[33]);

                coef_abov_h[1].write(coef_buff_abv[1]);
                coef_abov_h[2].write(coef_buff_abv[4]);
                coef_abov_h[3].write(coef_buff_abv[8]);
                coef_abov_h[4].write(coef_buff_abv[11]);
                coef_abov_h[5].write(coef_buff_abv[19]);
                coef_abov_h[6].write(coef_buff_abv[22]);
                coef_abov_h[7].write(coef_buff_abv[33]);
            } else if (cnt == 2) {
                coef_here_h[1].write(coef_buff[5]);
                coef_here_h[2].write(coef_buff[7]);
                coef_here_h[3].write(coef_buff[12]);
                coef_here_h[4].write(coef_buff[18]);
                coef_here_h[5].write(coef_buff[23]);
                coef_here_h[6].write(coef_buff[32]);
                coef_here_h[7].write(coef_buff[34]);

                coef_abov_h[1].write(coef_buff_abv[5]);
                coef_abov_h[2].write(coef_buff_abv[7]);
                coef_abov_h[3].write(coef_buff_abv[12]);
                coef_abov_h[4].write(coef_buff_abv[18]);
                coef_abov_h[5].write(coef_buff_abv[23]);
                coef_abov_h[6].write(coef_buff_abv[32]);
                coef_abov_h[7].write(coef_buff_abv[34]);
            } else if (cnt == 3) {
                coef_here_h[1].write(coef_buff[6]);
                coef_here_h[2].write(coef_buff[13]);
                coef_here_h[3].write(coef_buff[17]);
                coef_here_h[4].write(coef_buff[24]);
                coef_here_h[5].write(coef_buff[31]);
                coef_here_h[6].write(coef_buff[35]);
                coef_here_h[7].write(coef_buff[42]);

                coef_abov_h[1].write(coef_buff_abv[6]);
                coef_abov_h[2].write(coef_buff_abv[13]);
                coef_abov_h[3].write(coef_buff_abv[17]);
                coef_abov_h[4].write(coef_buff_abv[24]);
                coef_abov_h[5].write(coef_buff_abv[31]);
                coef_abov_h[6].write(coef_buff_abv[35]);
                coef_abov_h[7].write(coef_buff_abv[42]);
            } else if (cnt == 4) {
                coef_here_h[1].write(coef_buff[14]);
                coef_here_h[2].write(coef_buff[16]);
                coef_here_h[3].write(coef_buff[25]);
                coef_here_h[4].write(coef_buff[30]);
                coef_here_h[5].write(coef_buff[36]);
                coef_here_h[6].write(coef_buff[41]);
                coef_here_h[7].write(coef_buff[43]);

                coef_abov_h[1].write(coef_buff_abv[14]);
                coef_abov_h[2].write(coef_buff_abv[16]);
                coef_abov_h[3].write(coef_buff_abv[25]);
                coef_abov_h[4].write(coef_buff_abv[30]);
                coef_abov_h[5].write(coef_buff_abv[36]);
                coef_abov_h[6].write(coef_buff_abv[41]);
                coef_abov_h[7].write(coef_buff_abv[43]);
            } else if (cnt == 5) {
                coef_here_h[1].write(coef_buff[15]);
                coef_here_h[2].write(coef_buff[26]);
                coef_here_h[3].write(coef_buff[29]);
                coef_here_h[4].write(coef_buff[37]);
                coef_here_h[5].write(coef_buff[40]);
                coef_here_h[6].write(coef_buff[44]);
                coef_here_h[7].write(coef_buff[47]);

                coef_abov_h[1].write(coef_buff_abv[15]);
                coef_abov_h[2].write(coef_buff_abv[26]);
                coef_abov_h[3].write(coef_buff_abv[29]);
                coef_abov_h[4].write(coef_buff_abv[37]);
                coef_abov_h[5].write(coef_buff_abv[40]);
                coef_abov_h[6].write(coef_buff_abv[44]);
                coef_abov_h[7].write(coef_buff_abv[47]);
            } else if (cnt == 6) {
                coef_here_h[1].write(coef_buff[27]);
                coef_here_h[2].write(coef_buff[28]);
                coef_here_h[3].write(coef_buff[38]);
                coef_here_h[4].write(coef_buff[39]);
                coef_here_h[5].write(coef_buff[45]);
                coef_here_h[6].write(coef_buff[46]);
                coef_here_h[7].write(coef_buff[48]);

                coef_abov_h[1].write(coef_buff_abv[27]);
                coef_abov_h[2].write(coef_buff_abv[28]);
                coef_abov_h[3].write(coef_buff_abv[38]);
                coef_abov_h[4].write(coef_buff_abv[39]);
                coef_abov_h[5].write(coef_buff_abv[45]);
                coef_abov_h[6].write(coef_buff_abv[46]);
                coef_abov_h[7].write(coef_buff_abv[48]);
            }
            // 3.write out
            coef_here_h[0].write(coef_h_buff[cnt]);
            coef_abov_h[0].write(lb_read_reg(cnt * 11 + 10, cnt * 11));
            if (coef_h_buff[cnt].or_reduce()) reg_coef_cnt_h--;
            cnt++;

        } // loop0

        // 4.update
        if (!reg_coef_cnt_h.or_reduce()) {
            if (j < len - 1) { // read

                for (int i = 0; i < 49; i++) {
#pragma HLS unroll
                    coef_buff[i] = coef_77_here[i].read();
                    coef_buff_abv[i] = coef_77_above[i].read();
                }
                for (int i = 0; i < 7; i++) {
#pragma HLS unroll
                    coef_h_buff[i] = coef[i].read();
                    reg_coef_cnt_h = reg_coef_cnt_h + (coef_h_buff[i].or_reduce());
                }
                coef_cnt_h_len.write(reg_coef_cnt_h);
                if (!is_top_row) {
                    lb_read_reg = lb_read.read();
                } else {
                    lb_read_reg = 0;
                }
            }

            cnt = 0;
            j++;
        } // loop1
    }
    coef_end.write(true);
}

// ------------------------------------------------------------
void push_v_edge(ap_uint<32> len,
                 hls::stream<ap_int<11> > coef[7],
                 hls::stream<ap_int<11> > coef_77_here[49],

                 hls::stream<ap_int<11> > coef_here_v[8],
                 hls::stream<ap_int<11> > coef_left_v[8],
                 hls::stream<bool>& has_left,
                 hls::stream<bool>& coef_end,

                 //    hls::stream<ap_uint<3> >& coef_cnt_exp_v,
                 //    hls::stream<ap_uint<3> >& coef_cnt_sign_v,
                 //    hls::stream<ap_uint<3> >& coef_cnt_nois_v,
                 hls::stream<ap_uint<3> >& coef_cnt_v_len,
                 hls::stream<ap_uint<3> >& lane_v) {
    ap_int<11> coef_v_buff[7];
#pragma HLS ARRAY_PARTITION variable = coef_v_buff complete dim = 1
    ap_int<11> coef_v_buff_lft[7];
#pragma HLS ARRAY_PARTITION variable = coef_v_buff_lft complete dim = 1
    ap_int<11> coef_buff[49];
#pragma HLS array_partition variable = coef_buff complete dim = 0
    ap_int<11> coef_buff_lft[49];
#pragma HLS array_partition variable = coef_buff complete dim = 0

    ap_uint<3> reg_coef_cnt_v = 0;

    // 1.init
    for (int i = 0; i < 49; i++) {
#pragma HLS unroll
        coef_buff_lft[i] = coef_buff[i];
        coef_buff[i] = coef_77_here[i].read();
    }

    for (int i = 0; i < 7; i++) {
#pragma HLS unroll
        coef_v_buff_lft[i] = coef_v_buff[i];
        coef_v_buff[i] = coef[i].read();
        reg_coef_cnt_v = reg_coef_cnt_v + (coef_v_buff[i].or_reduce());
    }
    coef_cnt_v_len.write(reg_coef_cnt_v);

    // 2.loop
    int j = 0;
    int cnt = 0;
PUSH_VER_EDGE_LOOP:
    while (j < len) {
#pragma HLS pipeline II = 1

        if (reg_coef_cnt_v.or_reduce()) {
            coef_end.write(false);
            lane_v.write(cnt);

            if (j == 0)
                has_left.write(true);
            else
                has_left.write(false);

            if (cnt == 0) {
                coef_here_v[1].write(coef_buff[0]);
                coef_here_v[2].write(coef_buff[1]);
                coef_here_v[3].write(coef_buff[5]);
                coef_here_v[4].write(coef_buff[6]);
                coef_here_v[5].write(coef_buff[14]);
                coef_here_v[6].write(coef_buff[15]);
                coef_here_v[7].write(coef_buff[27]);

                coef_left_v[1].write(coef_buff_lft[0]);
                coef_left_v[2].write(coef_buff_lft[1]);
                coef_left_v[3].write(coef_buff_lft[5]);
                coef_left_v[4].write(coef_buff_lft[6]);
                coef_left_v[5].write(coef_buff_lft[14]);
                coef_left_v[6].write(coef_buff_lft[15]);
                coef_left_v[7].write(coef_buff_lft[27]);
            } else if (cnt == 1) {
                coef_here_v[1].write(coef_buff[2]);
                coef_here_v[2].write(coef_buff[4]);
                coef_here_v[3].write(coef_buff[7]);
                coef_here_v[4].write(coef_buff[13]);
                coef_here_v[5].write(coef_buff[16]);
                coef_here_v[6].write(coef_buff[26]);
                coef_here_v[7].write(coef_buff[28]);

                coef_left_v[1].write(coef_buff_lft[2]);
                coef_left_v[2].write(coef_buff_lft[4]);
                coef_left_v[3].write(coef_buff_lft[7]);
                coef_left_v[4].write(coef_buff_lft[13]);
                coef_left_v[5].write(coef_buff_lft[16]);
                coef_left_v[6].write(coef_buff_lft[26]);
                coef_left_v[7].write(coef_buff_lft[28]);
            } else if (cnt == 2) {
                coef_here_v[1].write(coef_buff[3]);
                coef_here_v[2].write(coef_buff[8]);
                coef_here_v[3].write(coef_buff[12]);
                coef_here_v[4].write(coef_buff[17]);
                coef_here_v[5].write(coef_buff[25]);
                coef_here_v[6].write(coef_buff[29]);
                coef_here_v[7].write(coef_buff[38]);

                coef_left_v[1].write(coef_buff_lft[3]);
                coef_left_v[2].write(coef_buff_lft[8]);
                coef_left_v[3].write(coef_buff_lft[12]);
                coef_left_v[4].write(coef_buff_lft[17]);
                coef_left_v[5].write(coef_buff_lft[25]);
                coef_left_v[6].write(coef_buff_lft[29]);
                coef_left_v[7].write(coef_buff_lft[38]);
            } else if (cnt == 3) {
                coef_here_v[1].write(coef_buff[9]);
                coef_here_v[2].write(coef_buff[11]);
                coef_here_v[3].write(coef_buff[18]);
                coef_here_v[4].write(coef_buff[24]);
                coef_here_v[5].write(coef_buff[30]);
                coef_here_v[6].write(coef_buff[37]);
                coef_here_v[7].write(coef_buff[39]);

                coef_left_v[1].write(coef_buff_lft[9]);
                coef_left_v[2].write(coef_buff_lft[11]);
                coef_left_v[3].write(coef_buff_lft[18]);
                coef_left_v[4].write(coef_buff_lft[24]);
                coef_left_v[5].write(coef_buff_lft[30]);
                coef_left_v[6].write(coef_buff_lft[37]);
                coef_left_v[7].write(coef_buff_lft[39]);

            } else if (cnt == 4) {
                coef_here_v[1].write(coef_buff[10]);
                coef_here_v[2].write(coef_buff[19]);
                coef_here_v[3].write(coef_buff[23]);
                coef_here_v[4].write(coef_buff[31]);
                coef_here_v[5].write(coef_buff[36]);
                coef_here_v[6].write(coef_buff[40]);
                coef_here_v[7].write(coef_buff[45]);

                coef_left_v[1].write(coef_buff_lft[10]);
                coef_left_v[2].write(coef_buff_lft[19]);
                coef_left_v[3].write(coef_buff_lft[23]);
                coef_left_v[4].write(coef_buff_lft[31]);
                coef_left_v[5].write(coef_buff_lft[36]);
                coef_left_v[6].write(coef_buff_lft[40]);
                coef_left_v[7].write(coef_buff_lft[45]);
            } else if (cnt == 5) {
                coef_here_v[1].write(coef_buff[20]);
                coef_here_v[2].write(coef_buff[22]);
                coef_here_v[3].write(coef_buff[32]);
                coef_here_v[4].write(coef_buff[35]);
                coef_here_v[5].write(coef_buff[41]);
                coef_here_v[6].write(coef_buff[44]);
                coef_here_v[7].write(coef_buff[46]);

                coef_left_v[1].write(coef_buff_lft[20]);
                coef_left_v[2].write(coef_buff_lft[22]);
                coef_left_v[3].write(coef_buff_lft[32]);
                coef_left_v[4].write(coef_buff_lft[35]);
                coef_left_v[5].write(coef_buff_lft[41]);
                coef_left_v[6].write(coef_buff_lft[44]);
                coef_left_v[7].write(coef_buff_lft[46]);
            } else if (cnt == 6) {
                coef_here_v[1].write(coef_buff[21]);
                coef_here_v[2].write(coef_buff[33]);
                coef_here_v[3].write(coef_buff[34]);
                coef_here_v[4].write(coef_buff[42]);
                coef_here_v[5].write(coef_buff[43]);
                coef_here_v[6].write(coef_buff[47]);
                coef_here_v[7].write(coef_buff[48]);

                coef_left_v[1].write(coef_buff_lft[21]);
                coef_left_v[2].write(coef_buff_lft[33]);
                coef_left_v[3].write(coef_buff_lft[34]);
                coef_left_v[4].write(coef_buff_lft[42]);
                coef_left_v[5].write(coef_buff_lft[43]);
                coef_left_v[6].write(coef_buff_lft[47]);
                coef_left_v[7].write(coef_buff_lft[48]);
            }
            // 3.write out
            coef_here_v[0].write(coef_v_buff[cnt]);
            coef_left_v[0].write(coef_v_buff_lft[cnt]);
            if (coef_v_buff[cnt].or_reduce()) reg_coef_cnt_v--;
            cnt++;

        } // loop0

        // 4.update
        if (!reg_coef_cnt_v.or_reduce()) {
            if (j < len - 1) { // read

                for (int i = 0; i < 49; i++) {
#pragma HLS unroll
                    coef_buff_lft[i] = coef_buff[i];
                    coef_buff[i] = coef_77_here[i].read();
                }

                for (int i = 0; i < 7; i++) {
#pragma HLS unroll
                    coef_v_buff_lft[i] = coef_v_buff[i];
                    coef_v_buff[i] = coef[i].read();
                    reg_coef_cnt_v = reg_coef_cnt_v + (coef_v_buff[i].or_reduce());
                }
                coef_cnt_v_len.write(reg_coef_cnt_v);
            }

            cnt = 0;
            j++;

        } // loop1
    }
    //}
    coef_end.write(true);
}

// ------------------------------------------------------------
void dispatch(ap_uint<32> len,
              hls::stream<ap_int<11> > coef[8],
              hls::stream<ap_int<11> > coef_to_7x7[8],
              hls::stream<ap_int<11> > coef_to_h[7],
              hls::stream<ap_int<11> > coef_to_h_lb[7],
              hls::stream<ap_int<11> > coef_to_v[7]) {
    int cnt = 0;
    int block_cnt = 0;
    ap_int<11> coef_reg[8];
    ap_int<11> h_reg[7];
    ap_int<11> v_reg[7];

    while (block_cnt < len) {
#pragma HLS pipeline II = 1

        coef_reg[0] = coef[0].read();
        coef_reg[1] = coef[1].read();
        coef_reg[2] = coef[2].read();
        coef_reg[3] = coef[3].read();
        coef_reg[4] = coef[4].read();
        coef_reg[5] = coef[5].read();
        coef_reg[6] = coef[6].read();
        coef_reg[7] = coef[7].read();

        if (cnt == 0) {
            coef_to_h[0].write(coef_reg[1]);
            coef_to_h_lb[0].write(coef_reg[1]);
            coef_to_v[0].write(coef_reg[2]);
            coef_to_v[1].write(coef_reg[3]);
            coef_to_7x7[0].write(coef_reg[4]);
            coef_to_h[1].write(coef_reg[5]);
            coef_to_h[2].write(coef_reg[6]);
            coef_to_h_lb[1].write(coef_reg[5]);
            coef_to_h_lb[2].write(coef_reg[6]);
            coef_to_7x7[1].write(coef_reg[7]);
            cnt++;
        }

        else if (cnt == 1) {
            coef_to_7x7[2].write(coef_reg[0]);
            coef_to_v[2].write(coef_reg[1]);
            coef_to_v[3].write(coef_reg[2]);
            coef_to_7x7[3].write(coef_reg[3]);
            coef_to_7x7[4].write(coef_reg[4]);
            coef_to_7x7[5].write(coef_reg[5]);
            coef_to_h[3].write(coef_reg[6]);
            coef_to_h[4].write(coef_reg[7]);
            coef_to_h_lb[3].write(coef_reg[6]);
            coef_to_h_lb[4].write(coef_reg[7]);
            cnt++;
        }

        else if (cnt == 2) {
            coef_to_7x7[6].write(coef_reg[0]);
            coef_to_7x7[7].write(coef_reg[1]);
            coef_to_7x7[0].write(coef_reg[2]);
            coef_to_7x7[1].write(coef_reg[3]);
            coef_to_v[4].write(coef_reg[4]);
            coef_to_v[5].write(coef_reg[5]);
            coef_to_7x7[2].write(coef_reg[6]);
            coef_to_7x7[3].write(coef_reg[7]);
            cnt++;
        }

        else if (cnt == 3) {
            coef_to_7x7[4].write(coef_reg[0]);
            coef_to_7x7[5].write(coef_reg[1]);
            coef_to_7x7[6].write(coef_reg[2]);
            coef_to_h[5].write(coef_reg[3]);
            coef_to_h[6].write(coef_reg[4]);
            coef_to_h_lb[5].write(coef_reg[3]);
            coef_to_h_lb[6].write(coef_reg[4]);
            coef_to_7x7[7].write(coef_reg[5]);
            coef_to_7x7[0].write(coef_reg[6]);
            coef_to_7x7[1].write(coef_reg[7]);
            cnt++;
        }

        else if (cnt == 4) {
            coef_to_7x7[2].write(coef_reg[0]);
            coef_to_7x7[3].write(coef_reg[1]);
            coef_to_7x7[4].write(coef_reg[2]);
            coef_to_v[6].write(coef_reg[3]);
            coef_to_7x7[5].write(coef_reg[4]);
            coef_to_7x7[6].write(coef_reg[5]);
            coef_to_7x7[7].write(coef_reg[6]);
            coef_to_7x7[0].write(coef_reg[7]);
            cnt++;

        }

        else if (cnt == 5) {
            coef_to_7x7[1].write(coef_reg[0]);
            coef_to_7x7[2].write(coef_reg[1]);
            coef_to_7x7[3].write(coef_reg[2]);
            coef_to_7x7[4].write(coef_reg[3]);
            coef_to_7x7[5].write(coef_reg[4]);
            coef_to_7x7[6].write(coef_reg[5]);
            coef_to_7x7[7].write(coef_reg[6]);
            coef_to_7x7[0].write(coef_reg[7]);
            cnt++;

        }

        else if (cnt == 6) {
            coef_to_7x7[1].write(coef_reg[0]);
            coef_to_7x7[2].write(coef_reg[1]);
            coef_to_7x7[3].write(coef_reg[2]);
            coef_to_7x7[4].write(coef_reg[3]);
            coef_to_7x7[5].write(coef_reg[4]);
            coef_to_7x7[6].write(coef_reg[5]);
            coef_to_7x7[7].write(coef_reg[6]);
            coef_to_7x7[0].write(coef_reg[7]);
            cnt++;
        }

        else if (cnt == 7) {
            coef_to_7x7[1].write(coef_reg[0]);
            coef_to_7x7[2].write(coef_reg[1]);
            coef_to_7x7[3].write(coef_reg[2]);
            coef_to_7x7[4].write(coef_reg[3]);
            coef_to_7x7[5].write(coef_reg[4]);
            coef_to_7x7[6].write(coef_reg[5]);
            coef_to_7x7[7].write(coef_reg[6]);
            coef_to_7x7[0].write(coef_reg[7]);
            cnt = 0;
            block_cnt++;
        }
    }
}

// ------------------------------------------------------------
void duplicate_coef(hls::stream<ap_int<11> > coef[8],
                    ap_uint<32> len,
                    hls::stream<ap_int<11> > strm_coef[8],
                    hls::stream<coef_t> str_rast8[8],
                    hls::stream<coef_t>& str_dc_in) {
    ap_uint<32> cnt = 0;
    ap_int<11> coef_reg[8];
#pragma HLS array_partition variable = coef_reg complete dim = 0

    while (cnt < len * 8) {
#pragma HLS pipeline II = 1

        coef_reg[0] = coef[0].read();
        coef_reg[1] = coef[1].read();
        coef_reg[2] = coef[2].read();
        coef_reg[3] = coef[3].read();
        coef_reg[4] = coef[4].read();
        coef_reg[5] = coef[5].read();
        coef_reg[6] = coef[6].read();
        coef_reg[7] = coef[7].read();

        strm_coef[0].write(coef_reg[0]);
        strm_coef[1].write(coef_reg[1]);
        strm_coef[2].write(coef_reg[2]);
        strm_coef[3].write(coef_reg[3]);
        strm_coef[4].write(coef_reg[4]);
        strm_coef[5].write(coef_reg[5]);
        strm_coef[6].write(coef_reg[6]);
        strm_coef[7].write(coef_reg[7]);

        str_rast8[0].write(coef_reg[0]);
        str_rast8[1].write(coef_reg[1]);
        str_rast8[2].write(coef_reg[2]);
        str_rast8[3].write(coef_reg[3]);
        str_rast8[4].write(coef_reg[4]);
        str_rast8[5].write(coef_reg[5]);
        str_rast8[6].write(coef_reg[6]);
        str_rast8[7].write(coef_reg[7]);

        if (cnt(2, 0) == 0) {
            str_dc_in.write(coef_reg[0]);
        }
        cnt++;
    }
}

// ------------------------------------------------------------
void preprocess(ap_uint<32> len,
                ap_uint<3> id_cmp,
                bool is_top_row,
                hls::stream<ap_int<11> > coef[8],

                hls::stream<ap_int<11> >& coef_7x7,
                hls::stream<ap_int<11> >& coef_lft,
                hls::stream<ap_int<11> >& coef_abv,
                hls::stream<ap_int<11> >& coef_abv_lft,
                hls::stream<ap_int<11> > coef_h[8],
                hls::stream<ap_int<11> > coef_above_h[8],
                hls::stream<bool>& strm_has_left_h,
                hls::stream<bool>& coef_h_e,
                hls::stream<ap_int<11> > coef_v[8],
                hls::stream<ap_int<11> > coef_left_v[8],
                hls::stream<bool>& strm_has_left_v,
                hls::stream<bool>& coef_v_e,
                hls::stream<ap_uint<6> >& non_zero_cnt,
                hls::stream<ap_uint<6> >& non_zero_cnt_lft,
                hls::stream<ap_uint<6> >& non_zero_cnt_abv,
                hls::stream<ap_uint<6> >& non_zero_7x7,
                hls::stream<ap_uint<6> >& non_zero_h_out,
                hls::stream<ap_uint<3> >& coef_cnt_h_len,
                hls::stream<ap_uint<3> >& strm_lane_h,
                hls::stream<ap_uint<3> >& coef_cnt_v_len,
                hls::stream<ap_uint<3> >& strm_lane_v,
                hls::stream<ap_uint<3> >& eob_x,
                hls::stream<ap_uint<3> >& eob_y) {
#pragma HLS INLINE
#pragma HLS dataflow

    // clang-format off
    hls::stream<ap_int<11> > coef_to_7x7[8];
#pragma HLS stream depth=8 variable=coef_to_7x7
#pragma HLS array_partition variable=coef_to_7x7 complete dim=0

    hls::stream<ap_int<11> > coef_to_h[7];
#pragma HLS stream depth=2 variable=coef_to_h
#pragma HLS array_partition variable=coef_to_h complete dim=0

    hls::stream<ap_int<11> > coef_to_h_lb[7];
#pragma HLS stream depth=2 variable=coef_to_h
#pragma HLS array_partition variable=coef_to_h_lb complete dim=0

    hls::stream<ap_int<11> > coef_to_v[7];
#pragma HLS stream depth=2 variable=coef_to_v
#pragma HLS array_partition variable=coef_to_v complete dim=0

    hls::stream<ap_int<11> > coef_buff[49];
#pragma HLS stream depth=2 variable=coef_buff
//#pragma HLS array_partition variable=coef_buff complete dim=0

    hls::stream<ap_int<11> > coef_buff_h[49];
#pragma HLS stream depth=2 variable=coef_buff_h

    hls::stream<ap_int<11> > coef_buff_v[49];
#pragma HLS stream depth=2 variable=coef_buff_v

    hls::stream<ap_int<11> > coef_abv_buff[49];
#pragma HLS stream depth=2 variable=coef_abv_buff
//#pragma HLS array_partition variable=coef_abv_buff complete dim=0

    hls::stream<ap_int<11> > coef_abv_buff_h[49];
#pragma HLS stream depth=2 variable=coef_abv_buff_h

    static hls::stream<ap_uint<6> > non_zero_cnt_7x7("midle_non_zero");
#pragma HLS stream depth=2 variable=non_zero_cnt_7x7

	static hls::stream<ap_uint<77> > lb_write("write");
#pragma HLS stream depth=2 variable=lb_write
	static hls::stream<ap_uint<77> > lb_read("read");
#pragma HLS stream depth=2 variable=lb_read

	static hls::stream<ap_uint<6> > lb_nz_write("nz_write");
#pragma HLS stream depth=2 variable=lb_nz_write

	static hls::stream<ap_uint<77> > lb_write_h("h_write");
#pragma HLS stream depth=2 variable=lb_write_h
	static hls::stream<ap_uint<77> > lb_read_h("h_read");
#pragma HLS stream depth=2 variable=lb_read_h
    // clang-format on

    dispatch(len, coef, coef_to_7x7, coef_to_h, coef_to_h_lb, coef_to_v);

    store_7x7(len, coef_to_7x7, non_zero_cnt_7x7, eob_x, eob_y, coef_buff, coef_buff_h, coef_buff_v, lb_write,
              lb_nz_write);
    line_buf_ctrl_77(len, id_cmp, is_top_row, lb_write, lb_read);
    line_buf_read_77(len, is_top_row, lb_read, coef_abv_buff, coef_abv_buff_h);

    line_buf_write_h(len, coef_to_h_lb, lb_write_h);
    line_buf_ctrl_h(len, id_cmp, is_top_row, lb_write_h, lb_read_h);
    push_h_edge(len, is_top_row, coef_to_h, coef_buff_h, coef_abv_buff_h, coef_h, coef_above_h, strm_has_left_h,
                coef_h_e,
                //			coef_cnt_exp_h,
                //			coef_cnt_sign_h,
                //			coef_cnt_nois_h,
                coef_cnt_h_len, strm_lane_h, lb_read_h);

    push_v_edge(len, coef_to_v, coef_buff_v, coef_v, coef_left_v, strm_has_left_v, coef_v_e,
                //			coef_cnt_exp_v,
                //			coef_cnt_sign_v,
                //			coef_cnt_nois_v,
                coef_cnt_v_len, strm_lane_v);

    line_buf_ctrl_nz(len, id_cmp, is_top_row, lb_nz_write, non_zero_cnt_abv);
    forward_7x7(len, coef_buff, coef_abv_buff, non_zero_cnt_7x7, coef_7x7, coef_lft, coef_abv, coef_abv_lft,
                non_zero_cnt, non_zero_cnt_lft, non_zero_7x7, non_zero_h_out);
}

//==========================================================================================

// ------------------------------------------------------------
ap_int<16> compute_aavrg(
    bool above_present, ap_int<11> coef_above, bool left_present, ap_int<11> coef_left, ap_int<11> coef_above_left) {
#pragma HLS INLINE
    ap_int<16> total = 0;

    if (left_present) {
        total = total + hls::abs(coef_left);
    }
    if (above_present) {
        total = total + hls::abs(coef_above);
    }
    if (left_present && above_present) {
        total = total * 13;
        total = total + (6 * hls::abs(coef_above_left));
        return total >> 5;
    } else {
        return total;
    }
    return total;
}

// ------------------------------------------------------------
void encode_num_nonzero_7x7(ap_uint<32> len,
                            bool above_present,
                            hls::stream<ap_uint<6> >& strm_cur_nonzeros_cnt,
                            hls::stream<ap_uint<6> >& strm_lft_nonzeros_cnt,
                            hls::stream<ap_uint<6> >& strm_abv_nonzeros_cnt,

                            hls::stream<ap_uint<4> >& strm_nonzero_bin) {
    ap_uint<5> serialized_so_far = 0;
    ap_uint<6> ap_nz_bin = 0;
    ap_uint<6> num_nonzeros_above;
    ap_uint<6> num_nonzeros_left;
    ap_uint<6> num_nonzeros_7x7;
    ap_uint<32> i = 0;

NONZERO_7X7_LINE:
    while (i < len) {
#pragma HLS pipeline II = 1
        serialized_so_far = 0;
        ap_nz_bin = 0;
        num_nonzeros_above = strm_abv_nonzeros_cnt.read();
        num_nonzeros_left = strm_lft_nonzeros_cnt.read();
        num_nonzeros_7x7 = strm_cur_nonzeros_cnt.read();

        if (above_present && i == 0) {
            ap_nz_bin = (num_nonzeros_above + 1) >> 1;
        } else if (i != 0 && !above_present) {
            ap_nz_bin = (num_nonzeros_left + 1) >> 1;
        } else if (i != 0 && above_present) {
            ap_nz_bin = (num_nonzeros_above + num_nonzeros_left + 2) >> 2;
        }

        strm_nonzero_bin.write(hls_nonzero_to_bin_9[ap_nz_bin]);
        i++;
    }
}

// ------------------------------------------------------------
void prepare_7x7(ap_uint<32> len,
                 hls::stream<ap_int<11> >& strm_coef_here,
                 bool above_present,
                 hls::stream<ap_int<11> >& strm_coef_above,
                 hls::stream<ap_int<11> >& strm_coef_left,
                 hls::stream<ap_int<11> >& strm_coef_above_left,
                 hls::stream<ap_uint<6> >& strm_num_nonzeros,

                 hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,
                 hls::stream<ap_uint<4> >& strm_7x7_length,

                 hls::stream<ap_uint<4> >& strm_num_nonzeros_bin,
                 hls::stream<ap_uint<4> >& strm_bsr_best_prior,

                 hls::stream<bool>& strm_cur_bit_sign,

                 hls::stream<ap_uint<11> >& strm_abs_coef,
                 hls::stream<ap_uint<6> >& strm_coord) {
    ap_int<16> avg;
    ap_uint<6> num_nonzeros;
    unsigned int zz;
    ap_uint<32> i = 0;

    // 1. init
    num_nonzeros = strm_num_nonzeros.read();
    strm_num_nonzeros_7x7.write(num_nonzeros);
    zz = 0;

PREPARE_7X7_LINE:
    while (i < len) {
#pragma HLS pipeline II = 1

        //		while(num_nonzeros){
        //#pragma HLS loop_tripcount max=5 min=0
        //#pragma HLS pipeline II=1
        if (num_nonzeros) {
            ap_uint<6> coord = (ap_uint<6>)hls_unzigzag49[zz];

            avg = compute_aavrg(above_present, strm_coef_above.read(), i != 0, strm_coef_left.read(),
                                strm_coef_above_left.read());
            ap_int<11> coef;
            coef = strm_coef_here.read();

            ap_int<11> abs_coef = hls::abs(coef);

            ap_uint<4> bsr_best_prior;
            bsr_best_prior = hls::min(16 - hls::abs(avg).countLeadingZeros(), 10);

            ap_uint<6> num_nonzeros_bin;
            num_nonzeros_bin = hls_nonzero_to_bin_9[num_nonzeros];

            ap_uint<4> length;
            length = 11 - abs_coef.countLeadingZeros();

            // 2. write out
            strm_7x7_length.write(length);

            strm_num_nonzeros_bin.write(num_nonzeros_bin);
            strm_bsr_best_prior.write(bsr_best_prior);
            strm_abs_coef.write(abs_coef);
            strm_coord.write(coord);
            if (length != 0) {
                strm_cur_bit_sign.write(coef >= 0 ? 1 : 0);
                num_nonzeros--;
            }
            zz++;
        }
        // 3. update
        if (!num_nonzeros) {
            if (i < len - 1) {
                num_nonzeros = strm_num_nonzeros.read();
                strm_num_nonzeros_7x7.write(num_nonzeros);
            }
            zz = 0;
            i++;
        }
        //}
    }
}

// ------------------------------------------------------------
void push_bit_7x7(ap_uint<32> len,
                  hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,
                  hls::stream<ap_uint<4> >& strm_7x7_length,

                  hls::stream<ap_uint<4> >& strm_nonzero_bin,

                  hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
                  hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

                  hls::stream<bool>& strm_cur_bit_sign,

                  hls::stream<ap_uint<11> >& strm_abs_coef,
                  hls::stream<ap_uint<6> >& strm_7x7_coord_nois,

                  hls::stream<ap_uint<4> >& strm_sel_tab,
                  hls::stream<bool>& strm_cur_bit,
                  hls::stream<bool>& strm_e,
                  hls::stream<ap_uint<16> >& strm_addr1,
                  hls::stream<ap_uint<16> >& strm_addr2,
                  hls::stream<ap_uint<16> >& strm_addr3,
                  hls::stream<ap_uint<16> >& strm_addr4) {
    ap_uint<5> serialized_so_far = 0;
    ap_uint<32> j = 0;
    ap_uint<6> nz;
    ap_uint<4> length;
    ap_uint<11> abs_coef;

    while (j < len) {
        serialized_so_far = 0;
        ap_uint<6> nz = strm_num_nonzeros_7x7.read();
        ap_uint<4> nonzero_bin_reg = strm_nonzero_bin.read();
        for (int index = 5; index >= 0; --index) {
#pragma HLS pipeline II = 1
            strm_sel_tab.write(NZ_CNT_7x7);
            bool cur_bit = nz[index];

            strm_cur_bit.write(cur_bit);
            strm_e.write(false);
            strm_addr1.write(nonzero_bin_reg);
            strm_addr2.write(index);
            strm_addr3.write(serialized_so_far);
            strm_addr4.write(0);

            serialized_so_far <<= 1;
            serialized_so_far.set(0, cur_bit);
        }

        int zz = 0;
    EXPONENT_7X7_OUTER:
        while (nz) {
#pragma HLS loop_tripcount max = 5 min = 0
            length = strm_7x7_length.read();
            ap_uint<4> num_nonzeros_bin = strm_7x7_num_nonzero_bin.read();
            ap_uint<4> bsr_best_prior = strm_7x7_bsr_best_prior.read();
            int i = 0;
        EXPONENT_7X7_INNER:
            while (i < length + 1) {
#pragma HLS loop_tripcount max = 3 min = 1
#pragma HLS pipeline II = 1
                bool cur_bit = (length != i);
                strm_sel_tab.write(EXP_CNT);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(num_nonzeros_bin);
                strm_addr2.write(zz);
                strm_addr3.write(bsr_best_prior);
                strm_addr4.write(i);
                i++;
            }

            if (length != 0) {
                strm_sel_tab.write(SIGN_CNT);
                strm_cur_bit.write(strm_cur_bit_sign.read());
                strm_e.write(false);
                strm_addr1.write(0);
                strm_addr2.write(0);
                strm_addr3.write(0);
                strm_addr4.write(0);
                nz--;
            }

            abs_coef = strm_abs_coef.read();
            unsigned int coord = strm_7x7_coord_nois.read();
            int k = 0;
        NOISE_7X7_INNER:
            while (k <= length - 2) {
#pragma HLS loop_tripcount max = 3 min = 1
#pragma HLS pipeline II = 1
                strm_sel_tab.write(NOIS_CNT);
                strm_cur_bit.write(abs_coef[length - 2 - k]);
                strm_e.write(false);
                strm_addr1.write(coord);
                strm_addr2.write(num_nonzeros_bin);
                strm_addr3.write(length - 2 - k);
                strm_addr4.write(0);
                k++;
            }

            zz++;
        }
        strm_e.write(true);
        j++;
    }
}

// ------------------------------------------------------------
void push_bit_7x7_v2(ap_uint<32> len,
                     hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,
                     hls::stream<ap_uint<4> >& strm_7x7_length,

                     hls::stream<ap_uint<4> >& strm_nonzero_bin,

                     hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
                     hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

                     hls::stream<bool>& strm_cur_bit_sign,

                     hls::stream<ap_uint<11> >& strm_abs_coef,
                     hls::stream<ap_uint<6> >& strm_7x7_coord_nois,

                     hls::stream<ap_uint<4> >& strm_sel_tab,
                     hls::stream<bool>& strm_cur_bit,
                     hls::stream<short>& strm_len,
                     //	hls::stream<bool>		 & strm_e,
                     hls::stream<ap_uint<16> >& strm_addr1,
                     hls::stream<ap_uint<16> >& strm_addr2,
                     hls::stream<ap_uint<16> >& strm_addr3,
                     hls::stream<ap_uint<16> >& strm_addr4) {
    ap_uint<5> serialized_so_far = 0;
    ap_uint<32> j = 0;
    ap_uint<6> nz;
    ap_uint<4> length;
    ap_uint<11> abs_coef;
    ap_uint<4> num_nonzeros_bin;
    ap_uint<4> bsr_best_prior;
    unsigned int coord;
    ap_uint<4> nonzero_bin_reg;

    ap_uint<8> state = 1;
    ap_uint<8> index = 5;
    int i = 0;
    int k = 0;
    int zz = 0;
    bool quit = false;
    short cnt = 0;

    quit = false;
    serialized_so_far = 0;
    nz = strm_num_nonzeros_7x7.read();
    nonzero_bin_reg = strm_nonzero_bin.read();
    index = 5;
    zz = 0;
    cnt = 0;
    state = 1;

    while (!quit) {
#pragma HLS pipeline II = 1
        if (state == 1) {
            strm_sel_tab.write(NZ_CNT_7x7);
            bool cur_bit = nz[index];

            strm_cur_bit.write(cur_bit);
            cnt++;
            strm_addr1.write(nonzero_bin_reg);
            strm_addr2.write(index);
            strm_addr3.write(serialized_so_far);
            strm_addr4.write(0);

            serialized_so_far <<= 1;
            serialized_so_far.set(0, cur_bit);
            if (index == 0 && nz != 0) {
                length = strm_7x7_length.read();
                num_nonzeros_bin = strm_7x7_num_nonzero_bin.read();
                bsr_best_prior = strm_7x7_bsr_best_prior.read();

                abs_coef = strm_abs_coef.read();
                coord = strm_7x7_coord_nois.read();
                i = 0;
                state = 3;
            } else if (index == 0 && nz == 0 && j != len - 1) {
                strm_len.write(cnt);
                serialized_so_far = 0;
                nz = strm_num_nonzeros_7x7.read();
                nonzero_bin_reg = strm_nonzero_bin.read();
                index = 5;
                zz = 0;
                cnt = 0;
                j++;
            } else if (index == 0 && nz == 0 && j == len - 1) {
                zz++;
                strm_len.write(cnt);
                cnt = 0;
                quit = true;
            } else {
                index--;
            }
        } else if (state == 3) {
            bool cur_bit = (length != i);
            strm_sel_tab.write(EXP_CNT);
            strm_cur_bit.write(cur_bit);
            cnt++;
            strm_addr1.write(num_nonzeros_bin);
            strm_addr2.write(zz);
            strm_addr3.write(bsr_best_prior);
            strm_addr4.write(i);
            if (i == length && length != 0) {
                state = 4;
            } else if (i == length && length == 0 && nz != 0) {
                zz++;
                length = strm_7x7_length.read();
                num_nonzeros_bin = strm_7x7_num_nonzero_bin.read();
                bsr_best_prior = strm_7x7_bsr_best_prior.read();

                abs_coef = strm_abs_coef.read();
                coord = strm_7x7_coord_nois.read();
                i = 0;
                state = 3;
            } else if (i == length && length == 0 && nz == 0 && j != len - 1) {
                strm_len.write(cnt);
                serialized_so_far = 0;
                nz = strm_num_nonzeros_7x7.read();
                nonzero_bin_reg = strm_nonzero_bin.read();
                index = 5;
                zz = 0;
                cnt = 0;
                j++;
                state = 1;
            } else if (i == length && length == 0 && nz == 0 && j == len - 1) {
                strm_len.write(cnt);
                cnt = 0;
                quit = true;
            } else {
                i++;
            }
        } else if (state == 4) {
            strm_sel_tab.write(SIGN_CNT);
            strm_cur_bit.write(strm_cur_bit_sign.read());
            cnt++;
            strm_addr1.write(0);
            strm_addr2.write(0);
            strm_addr3.write(0);
            strm_addr4.write(0);
            nz--;
            k = 0;
            if (k <= length - 2) {
                state = 5;
            } else if (nz != 0) {
                zz++;
                length = strm_7x7_length.read();
                num_nonzeros_bin = strm_7x7_num_nonzero_bin.read();
                bsr_best_prior = strm_7x7_bsr_best_prior.read();

                abs_coef = strm_abs_coef.read();
                coord = strm_7x7_coord_nois.read();
                i = 0;
                state = 3;
            } else if (nz == 0 && j != len - 1) {
                strm_len.write(cnt);
                serialized_so_far = 0;
                nz = strm_num_nonzeros_7x7.read();
                nonzero_bin_reg = strm_nonzero_bin.read();
                index = 5;
                zz = 0;
                cnt = 0;
                j++;
                state = 1;
            } else if (nz == 0 && j == len - 1) {
                strm_len.write(cnt);
                cnt = 0;
                quit = true;
            }
        } else if (state == 5) {
            strm_sel_tab.write(NOIS_CNT);
            strm_cur_bit.write(abs_coef[length - 2 - k]);
            cnt++;
            strm_addr1.write(coord);
            strm_addr2.write(num_nonzeros_bin);
            strm_addr3.write(length - 2 - k);
            strm_addr4.write(0);
            if (k == length - 2 && nz != 0) {
                zz++;
                length = strm_7x7_length.read();
                num_nonzeros_bin = strm_7x7_num_nonzero_bin.read();
                bsr_best_prior = strm_7x7_bsr_best_prior.read();

                abs_coef = strm_abs_coef.read();
                coord = strm_7x7_coord_nois.read();
                i = 0;
                state = 3;

            } else if (k == length - 2 && nz == 0 && j != len - 1) {
                strm_len.write(cnt);
                serialized_so_far = 0;
                nz = strm_num_nonzeros_7x7.read();
                nonzero_bin_reg = strm_nonzero_bin.read();
                index = 5;
                zz = 0;
                cnt = 0;
                j++;
                state = 1;
            } else if (k == length - 2 && nz == 0 && j == len - 1) {
                strm_len.write(cnt);
                cnt = 0;
                quit = true;
            }
            k++;
        }
    }
}

// ------------------------------------------------------------
void hls_serialize_tokens_77(ap_uint<32> len,
                             bool above_present,
                             hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,
                             hls::stream<ap_int<11> >& strm_coef_here,
                             hls::stream<ap_int<11> >& strm_coef_above,
                             hls::stream<ap_int<11> >& strm_coef_left,
                             hls::stream<ap_int<11> >& strm_coef_above_left,

                             hls::stream<ap_uint<6> >& strm_nz_cur,
                             hls::stream<ap_uint<6> >& strm_nz_abv,
                             hls::stream<ap_uint<6> >& strm_nz_lft,

                             hls::stream<ap_uint<4> >& strm_sel_tab,
                             hls::stream<bool>& strm_cur_bit,
                             hls::stream<short>& strm_len,
                             //	hls::stream<bool>		 & strm_e,
                             hls::stream<ap_uint<16> >& strm_addr1,
                             hls::stream<ap_uint<16> >& strm_addr2,
                             hls::stream<ap_uint<16> >& strm_addr3,
                             hls::stream<ap_uint<16> >& strm_addr4

                             ) {
#pragma HLS INLINE
#pragma HLS dataflow

    // clang-format off
    static hls::stream<ap_uint<11> > strm_abs_coef("coef_abs");
#pragma HLS stream depth=32 variable=strm_abs_coef

    hls::stream<ap_uint<4> > strm_nonzero_bin_tmp("strm_nz_bin");
#pragma HLS stream depth=32 variable=strm_nonzero_bin_tmp
    hls::stream<ap_uint<6> > strm_7x7_nz("strm_77_nz");
#pragma HLS stream depth=32 variable=strm_7x7_nz
    hls::stream<ap_uint<4> > strm_7x7_length("strm_77_len");
#pragma HLS stream depth=32 variable=strm_7x7_length

    hls::stream<ap_uint<4> > strm_7x7_num_nonzero_bin("strm_nz");
#pragma HLS stream depth=32 variable=strm_7x7_num_nonzero_bin
    hls::stream<ap_uint<4> > strm_7x7_bsr_best_prior("strm_bsr");
#pragma HLS stream depth=32 variable=strm_7x7_bsr_best_prior

    hls::stream<bool> strm_7x7_cur_bit_sign_tmp("strm_sign_bit");
#pragma HLS stream depth=32 variable=strm_7x7_cur_bit_sign_tmp
    hls::stream<ap_uint<6> > strm_coord("strm_coord");
#pragma HLS stream depth=32 variable=strm_coord
    // clang-format on

    encode_num_nonzero_7x7(len, above_present, strm_nz_cur, strm_nz_lft, strm_nz_abv, strm_nonzero_bin_tmp);

    prepare_7x7(len, strm_coef_here, above_present, strm_coef_above, strm_coef_left, strm_coef_above_left,
                strm_num_nonzeros_7x7,

                strm_7x7_nz, strm_7x7_length,

                strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                strm_7x7_cur_bit_sign_tmp,

                strm_abs_coef, strm_coord);

    push_bit_7x7_v2(len, strm_7x7_nz, strm_7x7_length,

                    strm_nonzero_bin_tmp,

                    strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                    strm_7x7_cur_bit_sign_tmp,

                    strm_abs_coef, strm_coord,

                    strm_sel_tab, strm_cur_bit, strm_len,
                    //		strm_e,
                    strm_addr1, strm_addr2, strm_addr3, strm_addr4);
}

// ------------------------------------------------------------
void pre_serialize_tokens_77(ap_uint<32> len,
                             bool above_present,
                             hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,
                             hls::stream<ap_int<11> >& strm_coef_here,
                             hls::stream<ap_int<11> >& strm_coef_above,
                             hls::stream<ap_int<11> >& strm_coef_left,
                             hls::stream<ap_int<11> >& strm_coef_above_left,

                             hls::stream<ap_uint<6> >& strm_nz_cur,
                             hls::stream<ap_uint<6> >& strm_nz_abv,
                             hls::stream<ap_uint<6> >& strm_nz_lft,

                             hls::stream<ap_uint<4> >& strm_nonzero_bin_tmp,

                             hls::stream<ap_uint<6> >& strm_7x7_nz,
                             hls::stream<ap_uint<4> >& strm_7x7_length,

                             hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
                             hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

                             hls::stream<bool>& strm_7x7_cur_bit_sign_tmp,

                             hls::stream<ap_uint<11> >& strm_abs_coef,
                             hls::stream<ap_uint<6> >& strm_coord

                             ) {
#pragma HLS INLINE
#pragma HLS DATAFLOW

    encode_num_nonzero_7x7(len, above_present, strm_nz_cur, strm_nz_lft, strm_nz_abv, strm_nonzero_bin_tmp);

    prepare_7x7(len, strm_coef_here, above_present, strm_coef_above, strm_coef_left, strm_coef_above_left,
                strm_num_nonzeros_7x7,

                strm_7x7_nz, strm_7x7_length,

                strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                strm_7x7_cur_bit_sign_tmp,

                strm_abs_coef, strm_coord);
}

} // namespace details
} // namespace codec
} // namespace xf
