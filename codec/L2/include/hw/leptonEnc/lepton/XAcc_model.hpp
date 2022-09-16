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
/**
 * @file XAcc_model.hpp
 * @brief lepton model include init ram, collect 77/dc/edge and probability look up function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XAcc_model.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XACC_MODEL_HPP_
#define _XACC_MODEL_HPP_
#include <stdint.h>
#include <stdio.h>
#include "XAcc_common.hpp"

class stt_range {
#define STTRANGE (5)
   public:
    uint8_t cnt_min[STTRANGE];
    uint8_t cnt_max[STTRANGE];
    stt_range() {
        for (int i = 0; i < STTRANGE; i++) {
            cnt_min[i] = 255;
            cnt_max[i] = 0;
        }
    }
    void print_range(int i) { printf("s%d: [ %d, %d ] \n", i + 1, cnt_min[i], cnt_max[i]); }
    void print_range(char* name) {
        printf("%s\n", name);
        for (int i = 0; i < STTRANGE; i++) printf("s%d: [ %d, %d ] \n", i + 1, cnt_min[i], cnt_max[i]);
    }
    void update(int i, uint8_t s) {
        if (s > cnt_max[i]) cnt_max[i] = s;
        if (s < cnt_min[i]) cnt_min[i] = s;
    }
    void update5(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5) {
        update(0, s1);
        update(1, s2);
        update(2, s3);
        update(3, s4);
        update(4, s5);
    }
};
struct hls_Model {
    hls_Branch num_nonzeros_counts_7x7_[2][26][6][32]; // 9984

    hls_Branch num_nonzeros_counts_1x8_[2][8][8][3][4]; // 1536
    hls_Branch num_nonzeros_counts_8x1_[2][8][8][3][4]; // 1536

    hls_Branch residual_noise_counts_[2][64][10][10];      // 2	64	10	10 =12800
    hls_Branch residual_noise_counts_dc_[12][10];          // 2244 12=12
    hls_Branch residual_threshold_counts_[2][256][8][128]; // 2*256*8*128 = 524288

    hls_Branch exponent_counts_[2][10][49][12][11];   // 2*10*49*12*11 = 129360
    hls_Branch exponent_counts_x_[2][10][15][12][11]; // 2*10*15*12*11 = 39600
    hls_Branch exponent_counts_dc_[12][17][11];       // 12*17*11 = 2244

    hls_Branch sign_counts_[2][4][12];   // 2*4*12 = 96
    hls_Branch sign_counts_77[2][4][12]; // 2*4*12 = 96

    hls_Branch* num_nonzeros_counts_7x7_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4) {
#pragma HLS INLINE
        return &num_nonzeros_counts_7x7_[s1][s2][s3][s4];
    }

    hls_Branch* num_nonzeros_counts_1x8_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5) {
#pragma HLS INLINE
        return &num_nonzeros_counts_1x8_[s1][s2][s3][s4][s5];
    }
    hls_Branch* num_nonzeros_counts_8x1_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5) {
#pragma HLS INLINE
        return &num_nonzeros_counts_8x1_[s1][s2][s3][s4][s5];
    } //[2][8][8][3][4];

    hls_Branch* residual_noise_counts_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4) {
#pragma HLS INLINE
        return &residual_noise_counts_[s1][s2][s3][s4];
    } //[2][64][10][10];

    hls_Branch* residual_noise_counts_dc_at(uint8_t s1, uint8_t s2) { // uint8_t s3, uint8_t s4){
#pragma HLS INLINE
        return &residual_noise_counts_dc_[s1][s2]; //[s3][s4];
    }                                              //[12][10];

    uint8_t cnt_s1_min;
    uint8_t cnt_s2_min;
    uint8_t cnt_s3_min;
    uint8_t cnt_s4_min;
    uint8_t cnt_s1_max;
    uint8_t cnt_s2_max;
    uint8_t cnt_s3_max;
    uint8_t cnt_s4_max;
    void init_1() {
        cnt_s1_min = 255;
        cnt_s2_min = 255;
        cnt_s3_min = 255;
        cnt_s4_min = 255;
        cnt_s1_max = 000;
        cnt_s2_max = 000;
        cnt_s3_max = 000;
        cnt_s4_max = 000;
    }
    void print_range() {
        printf("s1: [ %d, %d ] \n", cnt_s1_min, cnt_s1_max);
        printf("s2: [ %d, %d ] \n", cnt_s2_min, cnt_s2_max);
        printf("s3: [ %d, %d ] \n", cnt_s3_min, cnt_s3_max);
        printf("s4: [ %d, %d ] \n", cnt_s4_min, cnt_s4_max);
    }
    hls_Branch* residual_threshold_counts_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4) {
#pragma HLS INLINE

        if (s1 > cnt_s1_max) cnt_s1_max = s1;
        if (s1 < cnt_s1_min) cnt_s1_min = s1;
        if (s2 > cnt_s2_max) cnt_s2_max = s2;
        if (s2 < cnt_s2_min) cnt_s2_min = s2;

        if (s3 > cnt_s3_max) cnt_s3_max = s3;
        if (s3 < cnt_s3_min) cnt_s3_min = s3;

        if (s4 > cnt_s4_max) cnt_s4_max = s4;
        if (s4 < cnt_s4_min) cnt_s4_min = s4;

        return &residual_threshold_counts_[s1][s2][s3][s4];
    } //[2][(1<<(1 + 7))][1 + 7][1<<7 ];

    stt_range stt_counts;
    hls_Branch* exponent_counts_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5) {
#pragma HLS INLINE
        stt_counts.update5(s1, s2, s3, s4, s5);
        return &exponent_counts_[s1][s2][s3][s4][s5];
    } //[2][10][15][12][12];

    hls_Branch* exponent_counts_x_at(uint8_t s1, uint8_t s2, uint8_t s3, uint8_t s4, uint8_t s5) {
#pragma HLS INLINE
        return &exponent_counts_x_[s1][s2][s3][s4][s5];
    } //[2][10][49][12][12];

    hls_Branch* exponent_counts_dc_at(uint8_t s1, uint8_t s2, uint8_t s3) {
#pragma HLS INLINE
        return &exponent_counts_dc_[s1][s2][s3];
    } //[12][17][12];

    hls_Branch* sign_counts_at(uint8_t s1, uint8_t s2, uint8_t s3) {
#pragma HLS INLINE
        return &sign_counts_[s1][s2][s3];
    } //[2][4][12];

    hls_Branch* sign_counts_77_at(uint8_t s1, uint8_t s2, uint8_t s3) {
#pragma HLS INLINE
        // return &sign_counts_77[s1][s2][s3];
        return &sign_counts_[s1][s2][s3];
    } //[2][4][12];

    enum Printability { PRINTABLE_INSIGNIFICANT = 1, PRINTABLE_OK = 2, CLOSE_TO_50 = 4, CLOSE_TO_ONE_ANOTHER = 8 };
};

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void init_hlsmodel();

// ------------------------------------------------------------
void probability_look_up(ap_uint<1> ap_color,

                         hls::stream<ap_uint<4> >& strm_sel_tab,
                         hls::stream<bool>& strm_cur_bit,
                         hls::stream<bool>& strm_e_in,
                         hls::stream<ap_uint<16> >& strm_addr1,
                         hls::stream<ap_uint<16> >& strm_addr2,
                         hls::stream<ap_uint<16> >& strm_addr3,
                         hls::stream<ap_uint<16> >& strm_addr4,

                         hls::stream<bool>& strm_bit,
                         hls::stream<uint8_t>& strm_prob,
                         hls::stream<bool>& strm_e,
                         hls::stream<uint8_t>& strm_tab_dbg);

// ------------------------------------------------------------
template <class T0, class T1, class T2>
void collect(uint16_t num_blk,

             hls::stream<ap_uint<4> >& strm_sel_tab_77,
             hls::stream<bool>& strm_cur_bit_77,
             hls::stream<T0>& strm_len0_77,
             hls::stream<ap_uint<16> >& strm_addr1_77,
             hls::stream<ap_uint<16> >& strm_addr2_77,
             hls::stream<ap_uint<16> >& strm_addr3_77,
             hls::stream<ap_uint<16> >& strm_addr4_77,

             hls::stream<ap_uint<4> >& strm_sel_tab_edge,
             hls::stream<bool>& strm_cur_bit_edge,
             hls::stream<T1>& strm_len1_edge,
             hls::stream<ap_uint<16> >& strm_addr1_edge,
             hls::stream<ap_uint<16> >& strm_addr2_edge,
             hls::stream<ap_uint<16> >& strm_addr3_edge,
             hls::stream<ap_uint<16> >& strm_addr4_edge,

             hls::stream<ap_uint<4> >& strm_sel_tab_dc,
             hls::stream<bool>& strm_cur_bit_dc,
             hls::stream<T2>& strm_len2_dc,
             hls::stream<ap_uint<16> >& strm_addr1_dc,
             hls::stream<ap_uint<16> >& strm_addr2_dc,
             hls::stream<ap_uint<16> >& strm_addr3_dc,

             hls::stream<ap_uint<4> >& strm_sel_tab,
             hls::stream<bool>& strm_cur_bit,
             hls::stream<bool>& strm_out_e,
             hls::stream<ap_uint<16> >& strm_addr1,
             hls::stream<ap_uint<16> >& strm_addr2,
             hls::stream<ap_uint<16> >& strm_addr3,
             hls::stream<ap_uint<16> >& strm_addr4) {
    int next_blk = 0;
    T0 len0;
    T1 len1;
    T2 len2;
    len0 = strm_len0_77.read();
    len1 = strm_len1_edge.read();
    len2 = strm_len2_dc.read();

    while (next_blk < num_blk) { //
#pragma HLS pipeline II = 1
        int data_w;
        ap_uint<4> data_w_sel_t;
        bool data_w_value;
        ap_uint<16> data_w_addr1;
        ap_uint<16> data_w_addr2;
        ap_uint<16> data_w_addr3;
        ap_uint<16> data_w_addr4;
        if (len0 != 0) {
            data_w_sel_t = strm_sel_tab_77.read();
            data_w_value = strm_cur_bit_77.read();
            data_w_addr1 = strm_addr1_77.read();
            data_w_addr2 = strm_addr2_77.read();
            data_w_addr3 = strm_addr3_77.read();
            data_w_addr4 = strm_addr4_77.read();
            len0--;
        } else if (len1 != 0) {
            data_w_sel_t = strm_sel_tab_edge.read();
            data_w_value = strm_cur_bit_edge.read();
            data_w_addr1 = strm_addr1_edge.read();
            data_w_addr2 = strm_addr2_edge.read();
            data_w_addr3 = strm_addr3_edge.read();
            data_w_addr4 = strm_addr4_edge.read();
            len1--;
        } else if (len2 != 0) {
            data_w_sel_t = strm_sel_tab_dc.read();
            data_w_value = strm_cur_bit_dc.read();
            data_w_addr1 = strm_addr1_dc.read();
            data_w_addr2 = strm_addr2_dc.read();
            data_w_addr3 = strm_addr3_dc.read();
            data_w_addr4 = 0; // strm_addr4_dc.read();
            len2--;
            if (len0 == 0 && len1 == 0 && len2 == 0) {
                if (next_blk < num_blk - 1) {
                    len0 = strm_len0_77.read();
                    len1 = strm_len1_edge.read();
                    len2 = strm_len2_dc.read();
                }
                next_blk++;
            }
        }
        strm_sel_tab.write(data_w_sel_t); //,
        strm_cur_bit.write(data_w_value); //,
        strm_addr1.write(data_w_addr1);   //,
        strm_addr2.write(data_w_addr2);   //,
        strm_addr3.write(data_w_addr3);   //,
        strm_addr4.write(data_w_addr4);   //
        strm_out_e.write(false);
        char tmp[1024];
    } // while(cnt_blk < num_blk)
    strm_out_e.write(true);
};

} // namespace details
} // namespace codec
} // namespace xf

#endif
