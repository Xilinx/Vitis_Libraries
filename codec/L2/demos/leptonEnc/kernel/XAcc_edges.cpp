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
#include "XAcc_edges.hpp"

//-------------------------------------------------------------------------
struct edge_len {
    ap_uint<2> lennz;
    ap_uint<4> lenexp;
    ap_uint<1> lensign;
    ap_uint<4> lenthr;
    ap_uint<2> lennos;
    bool is_h;
};

struct taken_dat {
    ap_uint<4> sel_tab;
    bool cur_bit;
    bool e;
    ap_uint<16> addr1;
    ap_uint<16> addr2;
    ap_uint<16> addr3;
    ap_uint<16> addr4;
};

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void push_bit_edge(ap_uint<32> block_width,
                   hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,

                   hls::stream<ap_uint<3> >& strm_h_nz_len,
                   hls::stream<ap_uint<3> >& strm_eob_x,
                   hls::stream<ap_uint<4> >& strm_length_h,

                   hls::stream<ap_uint<4> >& strm_best_prior_exp_h,

                   hls::stream<bool>& strm_cur_bit_sign_h,
                   hls::stream<ap_uint<2> >& strm_tri_sign_h,

                   hls::stream<ap_uint<11> >& strm_abs_coef_nois_h,
                   hls::stream<ap_uint<8> >& strm_ctx_nois_h,
                   hls::stream<ap_uint<8> >& strm_min_nois_h,
                   hls::stream<ap_uint<6> >& strm_coord_nois_h,

                   hls::stream<ap_uint<3> >& strm_v_nz_len,
                   hls::stream<ap_uint<3> >& strm_eob_y,
                   hls::stream<ap_uint<4> >& strm_length_v,

                   hls::stream<ap_uint<4> >& strm_best_prior_exp_v,

                   hls::stream<bool>& strm_cur_bit_sign_v,
                   hls::stream<ap_uint<2> >& strm_tri_sign_v,

                   hls::stream<ap_uint<11> >& strm_abs_coef_nois_v,
                   hls::stream<ap_uint<8> >& strm_ctx_nois_v,
                   hls::stream<ap_uint<8> >& strm_min_nois_v,
                   hls::stream<ap_uint<6> >& strm_coord_nois_v,

                   hls::stream<ap_uint<4> >& strm_sel_tab,
                   hls::stream<bool>& strm_cur_bit,
                   hls::stream<bool>& strm_e,
                   hls::stream<ap_uint<16> >& strm_addr1,
                   hls::stream<ap_uint<16> >& strm_addr2,
                   hls::stream<ap_uint<16> >& strm_addr3,
                   hls::stream<ap_uint<16> >& strm_addr4) {
    int j = 0;
    int i = 0;
    int cnt = 0;
    while (j < block_width) {
        ap_uint<3> serialized_so_far = 0;
        ap_uint<3> num_nonzeros_edge = strm_h_nz_len.read();
        ap_uint<6> nz_77 = strm_num_nonzeros_7x7.read();
        ap_uint<3> eob_x = strm_eob_x.read();

        i = 2;
        while (i >= 0) {
#pragma HLS pipeline II = 1
            bool cur_bit = (num_nonzeros_edge & (1 << i)) ? 1 : 0;
            strm_sel_tab.write(NZ_CNT_8x1);
            strm_cur_bit.write(cur_bit);
            strm_e.write(false);
            strm_addr1.write(eob_x);
            strm_addr2.write((nz_77 + 3) / 7);
            strm_addr3.write(i);
            strm_addr4.write(serialized_so_far);

            serialized_so_far <<= 1;
            serialized_so_far.set(0, cur_bit);
            i--;
        }

        cnt = 0;
        while (num_nonzeros_edge > 0) {
            ap_uint<4> length = strm_length_h.read();
            ap_uint<4> bsr = strm_best_prior_exp_h.read();

            i = 0;
            while (i < length + 1) {
#pragma HLS pipeline II = 1
                bool cur_bit = (length != i);
                strm_sel_tab.write(EXP_CNT_X);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(num_nonzeros_edge);
                strm_addr2.write(cnt);
                strm_addr3.write(bsr);
                strm_addr4.write(i);
                i++;
            }

            if (length != 0) {
                strm_sel_tab.write(SIGN_CNT);
                strm_cur_bit.write(strm_cur_bit_sign_h.read());
                strm_e.write(false);
                strm_addr1.write(strm_tri_sign_h.read());
                strm_addr2.write(bsr);
                strm_addr3.write(0);
                strm_addr4.write(0);
            }

            i = length - 2;
            ap_uint<8> encoded_so_far = 1;
            ap_uint<11> abs_coef = strm_abs_coef_nois_h.read();
            uint16_t ctx_nois = strm_ctx_nois_h.read();
            uint8_t min_threshold = strm_min_nois_h.read();
            unsigned int coord = strm_coord_nois_h.read();

            while (i >= min_threshold) {
#pragma HLS pipeline II = 1
                bool cur_bit = (abs_coef & (1 << i)) ? 1 : 0;
                strm_sel_tab.write(THRE_CNT);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(ctx_nois);
                strm_addr2.write(length - min_threshold);
                strm_addr3.write(encoded_so_far);
                strm_addr4.write(0);

                encoded_so_far <<= 1;
                if (cur_bit) {
                    encoded_so_far.set(0, cur_bit);
                }
                // since we are not strict about rejecting jpegs with out of range coefs
                // we just make those less efficient by reusing the same probability bucket

                if (encoded_so_far > 127) encoded_so_far = 127;
                i--;
            }

            while (i >= 0) {
#pragma HLS pipeline II = 1
                bool cur_bit = (abs_coef & (1 << i)) ? 1 : 0;
                strm_sel_tab.write(NOIS_CNT);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(coord);
                strm_addr2.write(num_nonzeros_edge);
                strm_addr3.write(i);
                strm_addr4.write(0);
                i--;
            }

            if (length != 0) num_nonzeros_edge--;
            cnt++;
        }

        serialized_so_far = 0;
        num_nonzeros_edge = strm_v_nz_len.read();
        ap_uint<3> eob_y = strm_eob_y.read();

        i = 2;
        while (i >= 0) {
#pragma HLS pipeline II = 1
            bool cur_bit = (num_nonzeros_edge & (1 << i)) ? 1 : 0;
            strm_sel_tab.write(NZ_CNT_1x8);
            strm_cur_bit.write(cur_bit);
            strm_e.write(false);
            strm_addr1.write(eob_y);
            strm_addr2.write((nz_77 + 3) / 7);
            strm_addr3.write(i);
            strm_addr4.write(serialized_so_far);

            serialized_so_far <<= 1;
            serialized_so_far.set(0, cur_bit);
            i--;
        }

        cnt = 7;
        while (num_nonzeros_edge > 0) {
            ap_uint<4> length = strm_length_v.read();
            ap_uint<4> bsr = strm_best_prior_exp_v.read();

            i = 0;
            while (i < length + 1) {
#pragma HLS pipeline II = 1
                bool cur_bit = (length != i);
                strm_sel_tab.write(EXP_CNT_X);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(num_nonzeros_edge);
                strm_addr2.write(cnt);
                strm_addr3.write(bsr);
                strm_addr4.write(i);
                i++;
            }

            if (length != 0) {
                strm_sel_tab.write(SIGN_CNT);
                strm_cur_bit.write(strm_cur_bit_sign_v.read());
                strm_e.write(false);
                strm_addr1.write(strm_tri_sign_v.read());
                strm_addr2.write(bsr);
                strm_addr3.write(0);
                strm_addr4.write(0);
            }

            i = length - 2;
            ap_uint<8> encoded_so_far = 1;
            ap_uint<11> abs_coef = strm_abs_coef_nois_v.read();
            uint16_t ctx_nois = strm_ctx_nois_v.read();
            uint8_t min_threshold = strm_min_nois_v.read();
            unsigned int coord = strm_coord_nois_v.read();

            while (i >= min_threshold) {
#pragma HLS pipeline II = 1
                bool cur_bit = (abs_coef & (1 << i)) ? 1 : 0;
                strm_sel_tab.write(THRE_CNT);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(ctx_nois);
                strm_addr2.write(length - min_threshold);
                strm_addr3.write(encoded_so_far);
                strm_addr4.write(0);

                encoded_so_far <<= 1;
                if (cur_bit) {
                    encoded_so_far.set(0, cur_bit);
                }
                // since we are not strict about rejecting jpegs with out of range coefs
                // we just make those less efficient by reusing the same probability bucket

                if (encoded_so_far > 127) encoded_so_far = 127;
                i--;
            }

            while (i >= 0) {
#pragma HLS pipeline II = 1
                bool cur_bit = (abs_coef & (1 << i)) ? 1 : 0;
                strm_sel_tab.write(NOIS_CNT);
                strm_cur_bit.write(cur_bit);
                strm_e.write(false);
                strm_addr1.write(coord);
                strm_addr2.write(num_nonzeros_edge);
                strm_addr3.write(i);
                strm_addr4.write(0);
                i--;
            }

            if (length != 0) num_nonzeros_edge--;
            cnt++;
        } // num_nonzeros_edge>0
        strm_e.write(true);
        j++;
    }
}

// ------------------------------------------------------------
void collect_len_edge(ap_uint<32> block_width,

                      hls::stream<ap_uint<3> >& strm_h_nz_len,
                      hls::stream<ap_uint<4> >& strm_length_h,
                      hls::stream<ap_uint<8> >& strm_min_nois_h,

                      hls::stream<ap_uint<3> >& strm_v_nz_len,
                      hls::stream<ap_uint<4> >& strm_length_v,
                      hls::stream<ap_uint<8> >& strm_min_nois_v,

                      hls::stream<edge_len>& strm_len,
                      // hls::stream<edge_len >& strm_len_v,
                      hls::stream<ap_uint<3> >& strm_h_nz_pass,
                      hls::stream<ap_uint<3> >& strm_v_nz_pass,
                      hls::stream<ap_uint<8> >& strm_min_h_pass,
                      hls::stream<ap_uint<8> >& strm_min_v_pass) {
    int j = 0;
    edge_len tmp_len;

    ap_uint<3> num_nonzeros_edge_h = strm_h_nz_len.read();
    ap_uint<3> num_nonzeros_edge_v = 0;
    strm_h_nz_pass.write(num_nonzeros_edge_h);
    tmp_len.lennz = 3;
    tmp_len.is_h = true;

    ap_int<5> length; // to imp the -1 logic, use one more bit
    uint8_t min_threshold;
    while (j < block_width) {
#pragma HLS PIPELINE II = 1

        if (num_nonzeros_edge_h > 0) {
            length = strm_length_h.read();
            min_threshold = strm_min_nois_h.read();
            strm_min_h_pass.write(min_threshold);

            ////_XF_IMAGE_PRINT("h: nz=%d, len=%d\n", (int)num_nonzeros_edge_h, (int)length );
        } else if (num_nonzeros_edge_v > 0) {
            length = strm_length_v.read();
            min_threshold = strm_min_nois_v.read();
            strm_min_v_pass.write(min_threshold);

            ////_XF_IMAGE_PRINT("vv: nz=%d, len=%d\n", (int)num_nonzeros_edge_v, (int)length );
        } else { // only nz
            length = -1;
            min_threshold = 0;
        }

        int over_thr = length - 2 - min_threshold;
        int over_nos = min_threshold - 1;
        tmp_len.lenexp = length + 1;
        tmp_len.lensign = (length > 0);
        tmp_len.lenthr = (over_thr >= 0) ? (over_thr + 1) : 0;
        if ((over_thr >= 0 && (over_nos >= 0))) {
            tmp_len.lennos = min_threshold;
        } else if (over_thr < 0 && (length - 2 >= 0)) {
            tmp_len.lennos = length - 1;
        } else {
            tmp_len.lennos = 0;
        }

        strm_len.write(tmp_len);

        // update next loop
        if (tmp_len.is_h) {
            if ((num_nonzeros_edge_h > 0) && (length != 0)) { // keep doing
                num_nonzeros_edge_h--;
                tmp_len.lennz = 0;
                tmp_len.is_h = true;
                if (!num_nonzeros_edge_h) { // read v
                    num_nonzeros_edge_v = strm_v_nz_len.read();
                    strm_v_nz_pass.write(num_nonzeros_edge_v);
                    tmp_len.lennz = 3;
                    tmp_len.is_h = false;
                }
            } else if (!num_nonzeros_edge_h) { // read v
                num_nonzeros_edge_v = strm_v_nz_len.read();
                strm_v_nz_pass.write(num_nonzeros_edge_v);
                tmp_len.lennz = 3;
                tmp_len.is_h = false;
            } else { // keep doing
                tmp_len.lennz = 0;
                tmp_len.is_h = true;
            }
        } else {
            if ((num_nonzeros_edge_v > 0) && (length != 0)) { // keep doing
                num_nonzeros_edge_v--;
                tmp_len.lennz = 0;
                tmp_len.is_h = false;
                if (!num_nonzeros_edge_v) { // read h
                    if ((j < block_width - 1)) {
                        num_nonzeros_edge_h = strm_h_nz_len.read();
                        strm_h_nz_pass.write(num_nonzeros_edge_h);
                        tmp_len.lennz = 3;
                        tmp_len.is_h = true;
                    }
                    j++;
                }
            } else if (!num_nonzeros_edge_v) { // read h
                if ((j < block_width - 1)) {
                    num_nonzeros_edge_h = strm_h_nz_len.read();
                    strm_h_nz_pass.write(num_nonzeros_edge_h);
                    tmp_len.lennz = 3;
                    tmp_len.is_h = true;
                }
                j++;
            } else { // keep doing
                tmp_len.lennz = 0;
                tmp_len.is_h = false;
            }
        }

    } // end while
}

// ------------------------------------------------------------
void push_bit_edge_len(ap_uint<32> block_width,
                       hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,

                       hls::stream<edge_len>& strm_len,
                       //	hls::stream<edge_len >& strm_len_h,
                       //	hls::stream<edge_len >& strm_len_v,

                       hls::stream<ap_uint<3> >& strm_h_nz_len,
                       hls::stream<ap_uint<3> >& strm_eob_x,
                       // hls::stream<ap_uint<4> >& strm_length_h,

                       hls::stream<ap_uint<4> >& strm_best_prior_exp_h,

                       hls::stream<bool>& strm_cur_bit_sign_h,
                       hls::stream<ap_uint<2> >& strm_tri_sign_h,

                       hls::stream<ap_uint<11> >& strm_abs_coef_nois_h,
                       hls::stream<ap_uint<8> >& strm_ctx_nois_h,
                       hls::stream<ap_uint<8> >& strm_min_nois_h,
                       hls::stream<ap_uint<6> >& strm_coord_nois_h,

                       hls::stream<ap_uint<3> >& strm_v_nz_len,
                       hls::stream<ap_uint<3> >& strm_eob_y,
                       // hls::stream<ap_uint<4> >& strm_length_v,

                       hls::stream<ap_uint<4> >& strm_best_prior_exp_v,

                       hls::stream<bool>& strm_cur_bit_sign_v,
                       hls::stream<ap_uint<2> >& strm_tri_sign_v,

                       hls::stream<ap_uint<11> >& strm_abs_coef_nois_v,
                       hls::stream<ap_uint<8> >& strm_ctx_nois_v,
                       hls::stream<ap_uint<8> >& strm_min_nois_v,
                       hls::stream<ap_uint<6> >& strm_coord_nois_v,

                       hls::stream<short>& strm_edge_len,

                       hls::stream<ap_uint<4> >& strm_sel_tab,
                       hls::stream<bool>& strm_cur_bit,
                       // hls::stream<bool>		 & strm_e,
                       hls::stream<ap_uint<16> >& strm_addr1,
                       hls::stream<ap_uint<16> >& strm_addr2,
                       hls::stream<ap_uint<16> >& strm_addr3,
                       hls::stream<ap_uint<16> >& strm_addr4

                       ) {
    int j = 0;

    taken_dat taken_dat;
    // for loop j
    ap_uint<3> serialized_so_far = 0;
    ap_uint<3> num_nonzeros_edge = strm_h_nz_len.read();
    ap_uint<6> nz_77 = strm_num_nonzeros_7x7.read();
    ap_uint<3> eob_x = strm_eob_x.read();
    ap_uint<3> eob_y;

    // for loop taken
    ap_uint<4> bsr;
    ap_uint<11> abs_coef;
    uint16_t ctx_nois;
    uint8_t min_threshold;
    unsigned int coord;
    edge_len tmp_len = strm_len.read();
    ap_uint<4> length = tmp_len.lenexp - 1;

    if (num_nonzeros_edge > 0) {
        bsr = strm_best_prior_exp_h.read();
        abs_coef = strm_abs_coef_nois_h.read();
        ctx_nois = strm_ctx_nois_h.read();
        min_threshold = strm_min_nois_h.read();
        coord = strm_coord_nois_h.read();
    }

    int i_exp = 0;
    int cnt = 0;
    int i_thr = length - 2;
    int i_nos = tmp_len.lenthr.is_zero() ? ((int)length - 2) : ((int)min_threshold - 1);

    ap_uint<8> encoded_so_far = 1;

    short edge_len = 0;

    while (j < block_width) {
#pragma HLS PIPELINE II = 1

        if (tmp_len.lennz != 0) {
            int i_nz = tmp_len.lennz - 1;
            taken_dat.sel_tab = tmp_len.is_h ? NZ_CNT_8x1 : NZ_CNT_1x8;
            taken_dat.cur_bit = (num_nonzeros_edge & (1 << i_nz)) ? 1 : 0;
            taken_dat.e = false;
            taken_dat.addr1 = tmp_len.is_h ? eob_x : eob_y;
            taken_dat.addr2 = (nz_77 + 3) / 7;
            taken_dat.addr3 = i_nz;
            taken_dat.addr4 = serialized_so_far;

            serialized_so_far <<= 1; // init 0
            serialized_so_far.set(0, taken_dat.cur_bit);
            tmp_len.lennz--;

        } else if (tmp_len.lenexp != 0) {
            taken_dat.sel_tab = EXP_CNT_X;
            taken_dat.cur_bit = (length != i_exp); // init i_exp =0
            taken_dat.e = false;
            taken_dat.addr1 = num_nonzeros_edge; //--
            taken_dat.addr2 = cnt;               //++
            taken_dat.addr3 = bsr;
            taken_dat.addr4 = i_exp;
            i_exp++;
            tmp_len.lenexp--;

        } else if (tmp_len.lensign != 0) {
            bool big_sign_hv;
            ap_uint<2> tri_sign_hv;

            if (tmp_len.is_h) {
                big_sign_hv = strm_cur_bit_sign_h.read();
                tri_sign_hv = strm_tri_sign_h.read();
            } else {
                big_sign_hv = strm_cur_bit_sign_v.read();
                tri_sign_hv = strm_tri_sign_v.read();
            }
            taken_dat.sel_tab = SIGN_CNT;
            taken_dat.cur_bit = big_sign_hv;
            taken_dat.e = false;
            taken_dat.addr1 = tri_sign_hv;
            taken_dat.addr2 = bsr;
            taken_dat.addr3 = 0;
            taken_dat.addr4 = 0;
            tmp_len.lensign--;

        } else if (tmp_len.lenthr != 0) {
            taken_dat.sel_tab = THRE_CNT;
            taken_dat.cur_bit = (abs_coef & (1 << i_thr)) ? 1 : 0;
            taken_dat.e = false;
            taken_dat.addr1 = ctx_nois;
            taken_dat.addr2 = length - min_threshold;
            taken_dat.addr3 = encoded_so_far; // init 1
            taken_dat.addr4 = 0;

            encoded_so_far <<= 1;
            if (taken_dat.cur_bit) {
                encoded_so_far.set(0, taken_dat.cur_bit);
            }

            // since we are not strict about rejecting jpegs with out of range coefs
            // we just make those less efficient by reusing the same probability bucket
            if (encoded_so_far > 127) encoded_so_far = 127;

            i_thr--;
            tmp_len.lenthr--;

        } else if (tmp_len.lennos != 0) {
            taken_dat.sel_tab = NOIS_CNT;
            taken_dat.cur_bit = (abs_coef & (1 << i_nos)) ? 1 : 0;
            taken_dat.e = false;
            taken_dat.addr1 = coord;
            taken_dat.addr2 = num_nonzeros_edge;
            taken_dat.addr3 = i_nos;
            taken_dat.addr4 = 0;

            i_nos--;
            tmp_len.lennos--;

        } // end if

        // write out
        edge_len++;
        strm_sel_tab.write(taken_dat.sel_tab);
        strm_cur_bit.write(taken_dat.cur_bit);
        // strm_e.write(taken_dat.e);
        strm_addr1.write(taken_dat.addr1);
        strm_addr2.write(taken_dat.addr2);
        strm_addr3.write(taken_dat.addr3);
        strm_addr4.write(taken_dat.addr4);

        // update next loop
        if ((!tmp_len.lennz) && (!tmp_len.lenexp) && (!tmp_len.lensign) && (!tmp_len.lenthr) && (!tmp_len.lennos)) {
            if (tmp_len.is_h) {
                tmp_len = strm_len.read();
                if ((num_nonzeros_edge > 0) && (length != 0)) { // keep doing
                    num_nonzeros_edge--;
                    cnt++;
                    if (!num_nonzeros_edge) { // read v

                        num_nonzeros_edge = strm_v_nz_len.read();
                        eob_y = strm_eob_y.read();
                        serialized_so_far = 0;
                        cnt = 7;
                        if (num_nonzeros_edge > 0) {
                            bsr = strm_best_prior_exp_v.read();
                            abs_coef = strm_abs_coef_nois_v.read();
                            ctx_nois = strm_ctx_nois_v.read();
                            coord = strm_coord_nois_v.read();
                            min_threshold = strm_min_nois_v.read();
                        }

                        // tmp_len = strm_len.read();
                    } else { // read h
                        // nz_77=strm_num_nonzeros_7x7.read();

                        bsr = strm_best_prior_exp_h.read();
                        abs_coef = strm_abs_coef_nois_h.read();
                        ctx_nois = strm_ctx_nois_h.read();
                        coord = strm_coord_nois_h.read();
                        min_threshold = strm_min_nois_h.read();

                        // tmp_len = strm_len.read();
                    }
                } else if (!num_nonzeros_edge) { // read v
                    num_nonzeros_edge = strm_v_nz_len.read();
                    eob_y = strm_eob_y.read();
                    serialized_so_far = 0;
                    cnt = 7;

                    if (num_nonzeros_edge > 0) {
                        bsr = strm_best_prior_exp_v.read();
                        abs_coef = strm_abs_coef_nois_v.read();
                        ctx_nois = strm_ctx_nois_v.read();
                        coord = strm_coord_nois_v.read();
                        min_threshold = strm_min_nois_v.read();
                    }

                    // tmp_len = strm_len.read();

                } else { // keep doing

                    bsr = strm_best_prior_exp_h.read();
                    abs_coef = strm_abs_coef_nois_h.read();
                    ctx_nois = strm_ctx_nois_h.read();
                    coord = strm_coord_nois_h.read();
                    min_threshold = strm_min_nois_h.read();

                    cnt++;
                    // tmp_len = strm_len.read();
                }

            } else {
                if ((num_nonzeros_edge > 0) && (length != 0)) { // keep doing
                    num_nonzeros_edge--;
                    cnt++;
                    if (!num_nonzeros_edge) { // read h
                        if (j < block_width - 1) {
                            num_nonzeros_edge = strm_h_nz_len.read();
                            nz_77 = strm_num_nonzeros_7x7.read();
                            eob_x = strm_eob_x.read();
                            tmp_len = strm_len.read();
                            serialized_so_far = 0;
                            cnt = 0;
                            if (num_nonzeros_edge > 0) {
                                bsr = strm_best_prior_exp_h.read();
                                abs_coef = strm_abs_coef_nois_h.read();
                                ctx_nois = strm_ctx_nois_h.read();
                                coord = strm_coord_nois_h.read();
                                min_threshold = strm_min_nois_h.read();
                            }
                        }
                        j++;
                        // strm_e.write(true);//todo
                        strm_edge_len.write(edge_len);
                        edge_len = 0;
                    } else {
                        tmp_len = strm_len.read();

                        bsr = strm_best_prior_exp_v.read();
                        abs_coef = strm_abs_coef_nois_v.read();
                        ctx_nois = strm_ctx_nois_v.read();
                        coord = strm_coord_nois_v.read();
                        min_threshold = strm_min_nois_v.read();
                    }
                } else if (!num_nonzeros_edge) { // read h
                    if (j < block_width - 1) {
                        num_nonzeros_edge = strm_h_nz_len.read();
                        eob_x = strm_eob_x.read();
                        nz_77 = strm_num_nonzeros_7x7.read();
                        tmp_len = strm_len.read();
                        serialized_so_far = 0;
                        cnt = 0;
                        if (num_nonzeros_edge) {
                            bsr = strm_best_prior_exp_h.read();
                            abs_coef = strm_abs_coef_nois_h.read();
                            ctx_nois = strm_ctx_nois_h.read();
                            coord = strm_coord_nois_h.read();
                            min_threshold = strm_min_nois_h.read();
                        }
                    }
                    j++;
                    // strm_e.write(true);//todo
                    strm_edge_len.write(edge_len);
                    edge_len = 0;
                } else { // keep doing

                    tmp_len = strm_len.read();
                    cnt++;

                    bsr = strm_best_prior_exp_v.read();
                    abs_coef = strm_abs_coef_nois_v.read();
                    ctx_nois = strm_ctx_nois_v.read();
                    coord = strm_coord_nois_v.read();
                    min_threshold = strm_min_nois_v.read();
                }
            } // end else

            //    		if( j<block_width){
            //    			tmp_len = strm_len.read();
            //    		}
            i_exp = 0;
            length = tmp_len.lenexp - 1;
            i_thr = length - 2;
            i_nos = tmp_len.lenthr.is_zero() ? ((int)length - 2) : ((int)min_threshold - 1);
            encoded_so_far = 1;

            //			if(tmp_len.is_h){
            //				//_XF_IMAGE_PRINT("h: nz=%d, len=%d\n", (int)num_nonzeros_edge, (int)length );
            //			}else{
            //				//_XF_IMAGE_PRINT("vv: nz=%d, len=%d\n", (int)num_nonzeros_edge, (int)length );
            //			}
        } // end update

    } // end while
}

// ------------------------------------------------------------
void push_bit_edge_0(ap_uint<32> block_width,
                     hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7,

                     hls::stream<ap_uint<3> >& strm_h_nz_len,
                     hls::stream<ap_uint<3> >& strm_eob_x,
                     hls::stream<ap_uint<4> >& strm_length_h,

                     hls::stream<ap_uint<4> >& strm_best_prior_exp_h,

                     hls::stream<bool>& strm_cur_bit_sign_h,
                     hls::stream<ap_uint<2> >& strm_tri_sign_h,

                     hls::stream<ap_uint<11> >& strm_abs_coef_nois_h,
                     hls::stream<ap_uint<8> >& strm_ctx_nois_h,
                     hls::stream<ap_uint<8> >& strm_min_nois_h,
                     hls::stream<ap_uint<6> >& strm_coord_nois_h,

                     hls::stream<ap_uint<3> >& strm_v_nz_len,
                     hls::stream<ap_uint<3> >& strm_eob_y,
                     hls::stream<ap_uint<4> >& strm_length_v,

                     hls::stream<ap_uint<4> >& strm_best_prior_exp_v,

                     hls::stream<bool>& strm_cur_bit_sign_v,
                     hls::stream<ap_uint<2> >& strm_tri_sign_v,

                     hls::stream<ap_uint<11> >& strm_abs_coef_nois_v,
                     hls::stream<ap_uint<8> >& strm_ctx_nois_v,
                     hls::stream<ap_uint<8> >& strm_min_nois_v,
                     hls::stream<ap_uint<6> >& strm_coord_nois_v,

                     hls::stream<short>& strm_edge_len,

                     hls::stream<ap_uint<4> >& strm_sel_tab,
                     hls::stream<bool>& strm_cur_bit,
                     // hls::stream<bool>		 & strm_e,
                     hls::stream<ap_uint<16> >& strm_addr1,
                     hls::stream<ap_uint<16> >& strm_addr2,
                     hls::stream<ap_uint<16> >& strm_addr3,
                     hls::stream<ap_uint<16> >& strm_addr4) {
#pragma HLS DATAFLOW
    // clang-format off
	hls::stream<edge_len >           strm_len;
#pragma HLS stream depth=32 variable=strm_len
#pragma HLS bind_storage        variable=strm_len type=FIFO impl=LUTRAM
	hls::stream<edge_len > 			 strm_len_h;
	hls::stream<edge_len > 			 strm_len_v;
#pragma HLS stream depth=32 variable=strm_len_h
#pragma HLS bind_storage        variable=strm_len_h type=FIFO impl=LUTRAM
#pragma HLS stream depth=32 variable=strm_len_v
#pragma HLS bind_storage        variable=strm_len_v type=FIFO impl=LUTRAM
	hls::stream<ap_uint<3> >         strm_h_nz_pass;
	hls::stream<ap_uint<3> >         strm_v_nz_pass;
#pragma HLS stream depth=32 variable=strm_h_nz_pass
#pragma HLS bind_storage        variable=strm_h_nz_pass type=FIFO impl=LUTRAM
#pragma HLS stream depth=32 variable=strm_v_nz_pass
#pragma HLS bind_storage        variable=strm_v_nz_pass type=FIFO impl=LUTRAM

	hls::stream<ap_uint<8> >         strm_min_h_pass;
#pragma HLS stream depth=32 variable=strm_min_h_pass
#pragma HLS bind_storage        variable=strm_min_h_pass type=FIFO impl=LUTRAM

	hls::stream<ap_uint<8> >         strm_min_v_pass;
#pragma HLS stream depth=32 variable=strm_min_v_pass
#pragma HLS bind_storage        variable=strm_min_v_pass type=FIFO impl=LUTRAM

    // clang-format on

    collect_len_edge(block_width,

                     strm_h_nz_len, strm_length_h, strm_min_nois_h,

                     strm_v_nz_len, strm_length_v, strm_min_nois_v,

                     strm_len,
                     //		strm_len_h,
                     //		strm_len_v,
                     strm_h_nz_pass, strm_v_nz_pass, strm_min_h_pass, strm_min_v_pass);

    push_bit_edge_len(block_width, strm_num_nonzeros_7x7,

                      strm_len,
                      //			strm_len_h,
                      //			strm_len_v,

                      strm_h_nz_pass, strm_eob_x,
                      // strm_length_exp_h,

                      strm_best_prior_exp_h,

                      strm_cur_bit_sign_h, strm_tri_sign_h,

                      strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_h_pass, strm_coord_nois_h,

                      strm_v_nz_pass, strm_eob_y,
                      // strm_length_exp_v,

                      strm_best_prior_exp_v,

                      strm_cur_bit_sign_v, strm_tri_sign_v,

                      strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_v_pass, strm_coord_nois_v,

                      strm_edge_len,

                      strm_sel_tab, strm_cur_bit,
                      // strm_e,
                      strm_addr1, strm_addr2, strm_addr3, strm_addr4);
}

//-------------------------------------------------------------------------
void hls_serialize_tokens_edges(ap_uint<32> block_width,
                                bool ap_color,
                                uint8_t min_nois_thld_x[MAX_NUM_COLOR][64],
                                uint8_t min_nois_thld_y[MAX_NUM_COLOR][64],
                                bool left_present,
                                bool above_present,
                                bool above_right_present,
                                hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7_h,
                                hls::stream<ap_uint<3> >& strm_h_nz_len,
                                hls::stream<ap_uint<3> >& strm_lane_h,

                                hls::stream<ap_uint<3> >& strm_v_nz_len,
                                hls::stream<ap_uint<3> >& strm_lane_v,

                                hls::stream<ap_uint<3> >& strm_eob_x,
                                hls::stream<ap_uint<3> >& strm_eob_y,

                                int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],
                                int32_t idct_q_table_y[MAX_NUM_COLOR][8][8],

                                hls::stream<ap_int<11> > strm_coef_h_here[8],
                                hls::stream<ap_int<11> > strm_coef_h_above[8],
                                hls::stream<bool>& strm_has_left_h,
                                hls::stream<bool>& strm_coef_h_end,
                                hls::stream<ap_int<11> > strm_coef_v_here[8],
                                hls::stream<ap_int<11> > strm_coef_v_left[8],
                                hls::stream<bool>& strm_has_left_v,
                                hls::stream<bool>& strm_coef_v_end,

                                hls::stream<ap_uint<4> >& strm_sel_tab,
                                hls::stream<bool>& strm_cur_bit,
                                hls::stream<bool>& strm_e,
                                hls::stream<ap_uint<16> >& strm_addr1,
                                hls::stream<ap_uint<16> >& strm_addr2,
                                hls::stream<ap_uint<16> >& strm_addr3,
                                hls::stream<ap_uint<16> >& strm_addr4) {
#pragma HLS INLINE
#pragma HLS dataflow
    // clang-format off
	static hls::stream<ap_uint<11> > strm_abs_coef_nois_h("abs_coef_h");
#pragma HLS stream depth=32 variable=strm_abs_coef_nois_h

	static hls::stream<ap_uint<11> > strm_abs_coef_nois_v("abs_coef_v");
#pragma HLS stream depth=32 variable=strm_abs_coef_nois_v

    hls::stream<ap_uint<4> > strm_best_prior_exp_h("bsr_exp_h");
#pragma HLS stream depth=32 variable=strm_best_prior_exp_h

    hls::stream<bool> strm_cur_bit_sign_h("sign_bit_h");
#pragma HLS stream depth=32 variable=strm_cur_bit_sign_h
    hls::stream<ap_uint<2> > strm_tri_sign_h("tri_sign_h");
#pragma HLS stream depth=32 variable=strm_tri_sign_h

    hls::stream<ap_uint<8> > strm_ctx_nois_h;
#pragma HLS stream depth=32 variable=strm_ctx_nois_h
    hls::stream<ap_uint<8> > strm_min_nois_h;
#pragma HLS stream depth=32 variable=strm_min_nois_h
//    hls::stream<ap_uint<8> > strm_so_far_nois_h;
//#pragma HLS stream depth=32 variable=strm_so_far_nois_h
    hls::stream<ap_uint<6> > strm_coord_nois_h;
#pragma HLS stream depth=32 variable=strm_coord_nois_h

    hls::stream<ap_uint<4> > strm_best_prior_exp_v;
#pragma HLS stream depth=32 variable=strm_best_prior_exp_v

    hls::stream<bool> strm_cur_bit_sign_v("sign_bit_v");
#pragma HLS stream depth=32 variable=strm_cur_bit_sign_v
    hls::stream<ap_uint<2> > strm_tri_sign_v;
#pragma HLS stream depth=32 variable=strm_tri_sign_v

    hls::stream<ap_uint<8> > strm_ctx_nois_v;
#pragma HLS stream depth=32 variable=strm_ctx_nois_v
    hls::stream<ap_uint<8> > strm_min_nois_v;
#pragma HLS stream depth=32 variable=strm_min_nois_v
//    hls::stream<ap_uint<8> > strm_so_far_nois_v;
//#pragma HLS stream depth=32 variable=strm_so_far_nois_v
    hls::stream<ap_uint<6> > strm_coord_nois_v;
#pragma HLS stream depth=32 variable=strm_coord_nois_v

    hls::stream<ap_uint<4> > strm_length_exp_h("len_exp_h");
#pragma HLS stream depth=32 variable=strm_length_exp_h
    hls::stream<ap_uint<4> > strm_length_exp_v("len_exp_v");
#pragma HLS stream depth=32 variable=strm_length_exp_v

//#pragma HLS ARRAY_PARTITION variable=idct_q_table_x complete dim=3
//#pragma HLS ARRAY_PARTITION variable=idct_q_table_y complete dim=3
    // clang-format on
    prepare_edge<true>(block_width, ap_color, min_nois_thld_x, above_present, strm_lane_h, idct_q_table_x,

                       strm_coef_h_here, strm_coef_h_above, strm_has_left_h, strm_coef_h_end,

                       strm_length_exp_h,

                       strm_best_prior_exp_h,

                       strm_cur_bit_sign_h, strm_tri_sign_h,

                       strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h);

    prepare_edge<false>(block_width, ap_color, min_nois_thld_y, left_present, strm_lane_v, idct_q_table_y,

                        strm_coef_v_here, strm_coef_v_left, strm_has_left_v, strm_coef_v_end,

                        strm_length_exp_v,

                        strm_best_prior_exp_v,

                        strm_cur_bit_sign_v, strm_tri_sign_v,

                        strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v);

    push_bit_edge(block_width, strm_num_nonzeros_7x7_h,

                  strm_h_nz_len, strm_eob_x, strm_length_exp_h,

                  strm_best_prior_exp_h,

                  strm_cur_bit_sign_h, strm_tri_sign_h,

                  strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                  strm_v_nz_len, strm_eob_y, strm_length_exp_v,

                  strm_best_prior_exp_v,

                  strm_cur_bit_sign_v, strm_tri_sign_v,

                  strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v,

                  strm_sel_tab, strm_cur_bit, strm_e, strm_addr1, strm_addr2, strm_addr3, strm_addr4);
}

// ------------------------------------------------------------
void pre_serialize_tokens_edges(ap_uint<32> block_width,
                                bool ap_color,
                                uint8_t min_nois_thld_x[MAX_NUM_COLOR][64],
                                uint8_t min_nois_thld_y[MAX_NUM_COLOR][64],
                                bool left_present,
                                bool above_present,
                                bool above_right_present,
                                // hls::stream<ap_uint<6> >& strm_num_nonzeros_7x7_h,
                                // hls::stream<ap_uint<3> >& strm_h_nz_len,
                                hls::stream<ap_uint<3> >& strm_lane_h,

                                // hls::stream<ap_uint<3> >& strm_v_nz_len,
                                hls::stream<ap_uint<3> >& strm_lane_v,

                                // hls::stream<ap_uint<3> >& strm_eob_x,
                                // hls::stream<ap_uint<3> >& strm_eob_y,

                                int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],
                                int32_t idct_q_table_y[MAX_NUM_COLOR][8][8],

                                hls::stream<ap_int<11> > strm_coef_h_here[8],
                                hls::stream<ap_int<11> > strm_coef_h_above[8],
                                hls::stream<bool>& strm_has_left_h,
                                hls::stream<bool>& strm_coef_h_end,
                                hls::stream<ap_int<11> > strm_coef_v_here[8],
                                hls::stream<ap_int<11> > strm_coef_v_left[8],
                                hls::stream<bool>& strm_has_left_v,
                                hls::stream<bool>& strm_coef_v_end,

                                hls::stream<ap_uint<4> >& strm_length_exp_h,

                                hls::stream<ap_uint<4> >& strm_best_prior_exp_h,

                                hls::stream<bool>& strm_cur_bit_sign_h,
                                hls::stream<ap_uint<2> >& strm_tri_sign_h,

                                hls::stream<ap_uint<11> >& strm_abs_coef_nois_h,
                                hls::stream<ap_uint<8> >& strm_ctx_nois_h,
                                hls::stream<ap_uint<8> >& strm_min_nois_h,
                                hls::stream<ap_uint<6> >& strm_coord_nois_h,

                                hls::stream<ap_uint<4> >& strm_length_exp_v,

                                hls::stream<ap_uint<4> >& strm_best_prior_exp_v,

                                hls::stream<bool>& strm_cur_bit_sign_v,
                                hls::stream<ap_uint<2> >& strm_tri_sign_v,

                                hls::stream<ap_uint<11> >& strm_abs_coef_nois_v,
                                hls::stream<ap_uint<8> >& strm_ctx_nois_v,
                                hls::stream<ap_uint<8> >& strm_min_nois_v,
                                hls::stream<ap_uint<6> >& strm_coord_nois_v) {
#pragma HLS INLINE
#pragma HLS dataflow

    prepare_edge<true>(block_width, ap_color, min_nois_thld_x, above_present, strm_lane_h, idct_q_table_x,

                       strm_coef_h_here, strm_coef_h_above, strm_has_left_h, strm_coef_h_end,

                       strm_length_exp_h,

                       strm_best_prior_exp_h,

                       strm_cur_bit_sign_h, strm_tri_sign_h,

                       strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h);

    prepare_edge<false>(block_width, ap_color, min_nois_thld_y, left_present, strm_lane_v, idct_q_table_y,

                        strm_coef_v_here, strm_coef_v_left, strm_has_left_v, strm_coef_v_end,

                        strm_length_exp_v,

                        strm_best_prior_exp_v,

                        strm_cur_bit_sign_v, strm_tri_sign_v,

                        strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v);
}

} // namespace details
} // namespace codec
} // namespace xf
