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
#include "XAcc_common.hpp"
#include "XAcc_model.hpp"
#include "XAcc_lepjpegdecoder.hpp"
#include "XAcc_lepjfifparser.hpp"
#include "XAcc_arith.hpp"
#include "XAcc_77.hpp"
#include "XAcc_edges.hpp"
#include "XAcc_dc.hpp"
#include "stream_to_axi.hpp"

namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void pre_lepton_encoder_line(ap_uint<32> len,
                             uint8_t id_cmp,
                             bool is_top_row_cmp,

                             uint8_t q_tables0[MAX_NUM_COLOR][8][8],
                             uint8_t q0,
                             int32_t idct_q_table_x[3][8][8],
                             int32_t idct_q_table_y[3][8][8],
                             uint8_t min_nois_thld_x[3][64],
                             uint8_t min_nois_thld_y[3][64],

                             hls::stream<ap_int<11> > coef[8],

                             // 7x7
                             hls::stream<ap_uint<4> >& strm_nonzero_bin_tmp,

                             hls::stream<ap_uint<6> >& strm_7x7_nz,
                             hls::stream<ap_uint<4> >& strm_7x7_length,

                             hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
                             hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

                             hls::stream<bool>& strm_7x7_cur_bit_sign_tmp,

                             hls::stream<ap_uint<11> >& strm_abs_coef,
                             hls::stream<ap_uint<6> >& strm_coord,

                             // edge
                             // from preprossess to edge
                             hls::stream<ap_uint<6> >& non_zero_h_out,
                             hls::stream<ap_uint<3> >& coef_cnt_h_len,
                             hls::stream<ap_uint<3> >& coef_cnt_v_len,
                             hls::stream<ap_uint<3> >& eob_x,
                             hls::stream<ap_uint<3> >& eob_y,

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
                             hls::stream<ap_uint<6> >& strm_coord_nois_v,

                             // dc
                             hls::stream<int16_t>& strm_coef_dc,
                             hls::stream<int>& strm_uncertainty,
                             hls::stream<int>& strm_uncertainty2) {
#pragma HLS DATAFLOW
// clang-format off
 #pragma HLS ARRAY_PARTITION variable = idct_q_table_x complete dim=3
 #pragma HLS ARRAY_PARTITION variable = idct_q_table_y complete dim=3
 #pragma HLS ARRAY_PARTITION variable = q_tables0 		complete dim=2
    // clang-format on

    // clang-format off
     hls::stream<ap_int<11> > 			strm_coef[8];
#pragma HLS stream depth=1024 variable=	strm_coef
#pragma HLS bind_storage variable = 		strm_coef type=FIFO impl=LUTRAM
#pragma HLS array_partition variable=   strm_coef complete dim=0

	hls::stream< coef_t> str_rast8[8];
#pragma HLS stream depth=1024 variable=str_rast8
#pragma HLS bind_storage variable=str_rast8 type=FIFO impl=LUTRAM
#pragma HLS array_partition variable=str_rast8 complete dim=0

	hls::stream< coef_t> str_dc_in;
#pragma HLS stream depth=1024 variable=str_dc_in
#pragma HLS bind_storage variable=str_dc_in type=FIFO impl=LUTRAM
    // clang-format on
    duplicate_coef(coef, len, strm_coef, str_rast8, str_dc_in);

    // clang-format off
     static hls::stream<ap_uint<6> > 	 strm_num_nonzeros_7x7;
#pragma HLS stream depth=256   variable = strm_num_nonzeros_7x7
#pragma HLS bind_storage 		   variable = strm_num_nonzeros_7x7 type=FIFO impl=LUTRAM

     static hls::stream<ap_int<11> > 	   strm_coef_here;
#pragma HLS stream depth=256    variable = strm_coef_here
#pragma HLS bind_storage 			variable = strm_coef_here type=FIFO impl=LUTRAM
     static hls::stream<ap_int<11> > 	 strm_coef_above;
#pragma HLS stream depth=256 	variable=strm_coef_above
#pragma HLS bind_storage 			variable=strm_coef_above type=FIFO impl=LUTRAM
     static hls::stream<ap_int<11> > 	strm_coef_above_left;
#pragma HLS stream depth=256 variable=strm_coef_above_left
#pragma HLS bind_storage variable=strm_coef_above_left type=FIFO impl=LUTRAM
     static hls::stream<ap_int<11> > strm_coef_left;
#pragma HLS stream depth=256 variable=strm_coef_left
#pragma HLS bind_storage variable=strm_coef_left type=FIFO impl=LUTRAM

     static hls::stream<ap_uint<6> > strm_cur_nonzeros_cnt;
#pragma HLS stream depth=256 variable=strm_cur_nonzeros_cnt
#pragma HLS bind_storage variable=strm_cur_nonzeros_cnt type=FIFO impl=LUTRAM
     static hls::stream<ap_uint<6> > strm_lft_nonzeros_cnt;
#pragma HLS stream depth=256 variable=strm_lft_nonzeros_cnt
#pragma HLS bind_storage variable=strm_lft_nonzeros_cnt type=FIFO impl=LUTRAM
     static hls::stream<ap_uint<6> > strm_abv_nonzeros_cnt;
#pragma HLS stream depth=256 variable=strm_abv_nonzeros_cnt
#pragma HLS bind_storage variable=strm_abv_nonzeros_cnt type=FIFO impl=LUTRAM

     hls::stream<ap_int<11> > coef_h[8];
#pragma HLS bind_storage variable=coef_h type=FIFO impl=LUTRAM
#pragma HLS stream depth=64 variable=coef_h
#pragma HLS array_partition variable=coef_h complete dim=0

     hls::stream<ap_int<11> > coef_above_h[8];
#pragma HLS bind_storage variable=coef_above_h type=FIFO impl=LUTRAM
#pragma HLS stream depth=64 variable=coef_above_h
#pragma HLS array_partition variable=coef_above_h complete dim=0

 	hls::stream<bool> strm_has_left_h;
#pragma HLS stream depth=64 variable=strm_has_left_h

     hls::stream<bool> coef_e_h;
#pragma HLS stream depth=64 variable=coef_e_h

     hls::stream<ap_int<11> > coef_v[8];
#pragma HLS bind_storage variable=coef_v type=FIFO impl=LUTRAM
#pragma HLS stream depth=64 variable=coef_v
#pragma HLS array_partition variable=coef_v complete dim=0

     hls::stream<ap_int<11> > coef_left_v[8];
#pragma HLS bind_storage variable=coef_left_v type=FIFO impl=LUTRAM
#pragma HLS stream depth=64 variable=coef_left_v
#pragma HLS array_partition variable=coef_left_v complete dim=0

 	hls::stream<bool> strm_has_left_v;
#pragma HLS stream depth=64 variable=strm_has_left_v

     hls::stream<bool> coef_e_v;
#pragma HLS stream depth=64 variable=coef_e_v

     hls::stream<ap_uint<3> > strm_lane_h("lane_h");
#pragma HLS stream depth=64 variable=strm_lane_h
#pragma HLS bind_storage variable=strm_lane_h type=FIFO impl=LUTRAM

     hls::stream<ap_uint<3> > strm_lane_v("lane_v");
#pragma HLS stream depth=64 variable=strm_lane_v
#pragma HLS bind_storage variable=strm_lane_v type=FIFO impl=LUTRAM
    // clang-format on
    preprocess(len, id_cmp, is_top_row_cmp, strm_coef,

               // to 77
               strm_coef_here, strm_coef_left, strm_coef_above, strm_coef_above_left,

               // to edge
               coef_h, coef_above_h, strm_has_left_h, coef_e_h, coef_v, coef_left_v, strm_has_left_v, coef_e_v,

               // to 77
               strm_cur_nonzeros_cnt, strm_lft_nonzeros_cnt, strm_abv_nonzeros_cnt, strm_num_nonzeros_7x7,

               // to edge
               non_zero_h_out, coef_cnt_h_len, strm_lane_h, coef_cnt_v_len, strm_lane_v, eob_x, eob_y);

    pre_serialize_tokens_77(len, !is_top_row_cmp,

                            strm_num_nonzeros_7x7, strm_coef_here, strm_coef_above, strm_coef_left,
                            strm_coef_above_left,

                            strm_cur_nonzeros_cnt, strm_abv_nonzeros_cnt, strm_lft_nonzeros_cnt,

                            strm_nonzero_bin_tmp,

                            strm_7x7_nz, strm_7x7_length,

                            strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                            strm_7x7_cur_bit_sign_tmp,

                            strm_abs_coef, strm_coord);

    pre_serialize_tokens_edges(len, id_cmp != 0, min_nois_thld_x, min_nois_thld_y, false, !is_top_row_cmp, false,
                               // non_zero_h_out,
                               // coef_cnt_h_len,
                               strm_lane_h,

                               // coef_cnt_v_len,
                               strm_lane_v,

                               // eob_x,
                               // eob_y,

                               idct_q_table_x, idct_q_table_y,

                               coef_h, coef_above_h, strm_has_left_h, coef_e_h, coef_v, coef_left_v, strm_has_left_v,
                               coef_e_v,

                               strm_length_exp_h,

                               strm_best_prior_exp_h,

                               strm_cur_bit_sign_h, strm_tri_sign_h,

                               strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                               ////
                               strm_length_exp_v,

                               strm_best_prior_exp_v,

                               strm_cur_bit_sign_v, strm_tri_sign_v,

                               strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v);

    pre_serialize_tokens_dc(!is_top_row_cmp, id_cmp, len, q_tables0, q0,

                            str_rast8, str_dc_in,

                            strm_coef_dc, strm_uncertainty, strm_uncertainty2);
}
// ------------------------------------------------------------
void push_lepton_encoder_line(ap_uint<32> len,
                              uint8_t id_cmp,
                              // bool is_top_row_cmp,

                              // 7x7
                              hls::stream<ap_uint<4> >& strm_nonzero_bin_tmp,

                              hls::stream<ap_uint<6> >& strm_7x7_nz,
                              hls::stream<ap_uint<4> >& strm_7x7_length,

                              hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
                              hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

                              hls::stream<bool>& strm_7x7_cur_bit_sign_tmp,

                              hls::stream<ap_uint<11> >& strm_abs_coef,
                              hls::stream<ap_uint<6> >& strm_coord,

                              // edge
                              // from preprossess to edge
                              hls::stream<ap_uint<6> >& non_zero_h_out,
                              hls::stream<ap_uint<3> >& coef_cnt_h_len,
                              hls::stream<ap_uint<3> >& coef_cnt_v_len,
                              hls::stream<ap_uint<3> >& eob_x,
                              hls::stream<ap_uint<3> >& eob_y,

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
                              hls::stream<ap_uint<6> >& strm_coord_nois_v,

                              // dc
                              hls::stream<int16_t>& strm_coef,
                              hls::stream<int>& strm_uncertainty,
                              hls::stream<int>& strm_uncertainty2,

                              // output
                              unsigned char* range,
                              int* count,
                              unsigned int* value,
                              unsigned char* pre_byte,
                              unsigned short* run,
                              bool* br_isFirst,
                              unsigned int* pos,

                              hls::stream<bool>& strm_pos_o_e,
                              hls::stream<ap_uint<8> >& strm_pos_o_byte) {
#pragma HLS DATAFLOW
    // clang-format off
	hls::stream<ap_uint<4>  > strm_sel_tab_77;
#pragma HLS stream depth=1024 variable=strm_sel_tab_77
#pragma HLS bind_storage variable=strm_sel_tab_77 type=FIFO impl=LUTRAM
	hls::stream<bool>		  strm_cur_bit_77("bit_77");
#pragma HLS stream depth=1024 variable=strm_cur_bit_77
#pragma HLS bind_storage variable=strm_cur_bit_77 type=FIFO impl=LUTRAM

    hls::stream<short>         strm_len_77("len_77");
#pragma HLS stream depth=128 variable=strm_len_77
#pragma HLS bind_storage variable=strm_len_77 type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr1_77;
#pragma HLS stream depth=1024 variable=strm_addr1_77
#pragma HLS bind_storage variable=strm_addr1_77 type=FIFO impl=BRAM
	hls::stream<ap_uint<16> > strm_addr2_77;
#pragma HLS stream depth=1024 variable=strm_addr2_77
#pragma HLS bind_storage variable=strm_addr2_77 type=FIFO impl=BRAM
	hls::stream<ap_uint<16> > strm_addr3_77;
#pragma HLS stream depth=1024 variable=strm_addr3_77
#pragma HLS bind_storage variable=strm_addr3_77 type=FIFO impl=BRAM
	hls::stream<ap_uint<16> > strm_addr4_77;
#pragma HLS stream depth=1024 variable=strm_addr4_77
#pragma HLS bind_storage variable=strm_addr4_77 type=FIFO impl=BRAM
    // clang-format on

    push_bit_7x7_v2(len, strm_7x7_nz, strm_7x7_length,

                    strm_nonzero_bin_tmp,

                    strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                    strm_7x7_cur_bit_sign_tmp,

                    strm_abs_coef, strm_coord,

                    strm_sel_tab_77, strm_cur_bit_77,
                    //		strm_e_77,
                    strm_len_77, strm_addr1_77, strm_addr2_77, strm_addr3_77, strm_addr4_77);

    // clang-format off
	hls::stream<ap_uint<4>  > strm_sel_tab_edge("sel_tab");
#pragma HLS stream depth=512 variable=strm_sel_tab_edge
#pragma HLS bind_storage variable=strm_sel_tab_edge type=FIFO impl=LUTRAM
	hls::stream<bool>		  strm_cur_bit_edge("bit_edge");
#pragma HLS stream depth=512 variable=strm_cur_bit_edge
#pragma HLS bind_storage variable=strm_cur_bit_edge type=FIFO impl=LUTRAM
	hls::stream<bool>		  strm_e_edge("e_edge");
#pragma HLS stream depth=512 variable=strm_e_edge
#pragma HLS bind_storage variable=strm_e_edge type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr1_edge("addr1");
#pragma HLS stream depth=512 variable=strm_addr1_edge
#pragma HLS bind_storage variable=strm_addr1_edge type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr2_edge;
#pragma HLS stream depth=512 variable=strm_addr2_edge
#pragma HLS bind_storage variable=strm_addr2_edge type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr3_edge;
#pragma HLS stream depth=512 variable=strm_addr3_edge
#pragma HLS bind_storage variable=strm_addr3_edge type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr4_edge;
#pragma HLS stream depth=512 variable=strm_addr4_edge
#pragma HLS bind_storage variable=strm_addr4_edge type=FIFO impl=LUTRAM

	hls::stream<short >         	  strm_edge_len;
#pragma HLS stream depth=128 variable=strm_edge_len
#pragma HLS bind_storage         variable=strm_edge_len type=FIFO impl=LUTRAM
    // clang-format on
    push_bit_edge_0(len, non_zero_h_out,

                    coef_cnt_h_len, eob_x, strm_length_exp_h,

                    strm_best_prior_exp_h,

                    strm_cur_bit_sign_h, strm_tri_sign_h,

                    strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                    coef_cnt_v_len, eob_y,

                    strm_length_exp_v,

                    strm_best_prior_exp_v,

                    strm_cur_bit_sign_v, strm_tri_sign_v,

                    strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v,

                    strm_edge_len,

                    strm_sel_tab_edge, strm_cur_bit_edge,
                    // strm_e_edge,
                    strm_addr1_edge, strm_addr2_edge, strm_addr3_edge, strm_addr4_edge);

    // clang-format off
	hls::stream<ap_uint<4>  > strm_sel_tab_dc("sel_tab");
#pragma HLS stream depth=512 variable=strm_sel_tab_dc
#pragma HLS bind_storage variable=strm_sel_tab_dc type=FIFO impl=LUTRAM
	hls::stream<bool>		  strm_cur_bit_dc("bit_dc");
#pragma HLS stream depth=512 variable=strm_cur_bit_dc
#pragma HLS bind_storage variable=strm_cur_bit_dc type=FIFO impl=LUTRAM
	hls::stream<short>		  strm_len_dc("len_dc");
#pragma HLS stream depth=128 variable=strm_len_dc
#pragma HLS bind_storage variable=strm_len_dc type=FIFO impl=LUTRAM

	hls::stream<ap_uint<16> > strm_addr1_dc("addr1");
#pragma HLS stream depth=512 variable=strm_addr1_dc
#pragma HLS bind_storage variable=strm_addr1_dc type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr2_dc;
#pragma HLS stream depth=512 variable=strm_addr2_dc
#pragma HLS bind_storage variable=strm_addr2_dc type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr3_dc;
#pragma HLS stream depth=512 variable=strm_addr3_dc
#pragma HLS bind_storage variable=strm_addr3_dc type=FIFO impl=LUTRAM
    // clang-format on
    dc_push_bit_v2(len, strm_coef, strm_uncertainty, strm_uncertainty2,

                   strm_sel_tab_dc, strm_cur_bit_dc,
                   //		strm_e_dc,
                   strm_len_dc, strm_addr1_dc, strm_addr2_dc, strm_addr3_dc);

    // clang-format off
	hls::stream<ap_uint<4>  > strm_sel_tab;
#pragma HLS stream depth=32 variable=strm_sel_tab
#pragma HLS bind_storage variable=strm_sel_tab type=FIFO impl=LUTRAM
	hls::stream<bool>		  strm_cur_bit("res_bit");
#pragma HLS stream depth=32 variable=strm_cur_bit
#pragma HLS bind_storage variable=strm_cur_bit type=FIFO impl=LUTRAM
	hls::stream<bool>		  strm_e_in("res_e");
#pragma HLS stream depth=32 variable=strm_e_in
#pragma HLS bind_storage variable=strm_e_in type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr1;
#pragma HLS stream depth=32 variable=strm_addr1
#pragma HLS bind_storage variable=strm_addr1 type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr2;
#pragma HLS stream depth=32 variable=strm_addr2
#pragma HLS bind_storage variable=strm_addr2 type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr3;
#pragma HLS stream depth=32 variable=strm_addr3
#pragma HLS bind_storage variable=strm_addr3_dc type=FIFO impl=LUTRAM
	hls::stream<ap_uint<16> > strm_addr4;
#pragma HLS stream depth=32 variable=strm_addr4
#pragma HLS bind_storage variable=strm_addr4 type=FIFO impl=LUTRAM
    // clang-format on

    // clang-format off
    // clang-format on
    collect<short, short, short>(len,

                                 strm_sel_tab_77, strm_cur_bit_77,
                                 //		strm_e_77,
                                 strm_len_77, strm_addr1_77, strm_addr2_77, strm_addr3_77, strm_addr4_77,

                                 strm_sel_tab_edge, strm_cur_bit_edge, strm_edge_len,
                                 // strm_e_edge,
                                 strm_addr1_edge, strm_addr2_edge, strm_addr3_edge, strm_addr4_edge,

                                 strm_sel_tab_dc, strm_cur_bit_dc,
                                 //		strm_e_dc,
                                 strm_len_dc, strm_addr1_dc, strm_addr2_dc, strm_addr3_dc,

                                 strm_sel_tab, strm_cur_bit, strm_e_in, strm_addr1, strm_addr2, strm_addr3, strm_addr4);

    // clang-format off
    hls::stream<bool>    strm_bit;
#pragma HLS stream depth=256 variable=strm_bit
#pragma HLS bind_storage variable=strm_bit type=FIFO impl=LUTRAM
    hls::stream<uint8_t> strm_prob;
#pragma HLS stream depth=256 variable=strm_prob
#pragma HLS bind_storage variable=strm_prob type=FIFO impl=LUTRAM
    hls::stream<bool>    strm_e;
#pragma HLS stream depth=256 variable=strm_e
#pragma HLS bind_storage variable=strm_e type=FIFO impl=LUTRAM
    hls::stream<uint8_t> strm_tab_dbg;
#pragma HLS stream depth=256 variable=strm_tab_dbg
#pragma HLS bind_storage variable=strm_tab_dbg type=FIFO impl=LUTRAM
    // clang-format on
    probability_look_up(id_cmp != 0,

                        strm_sel_tab, strm_cur_bit, strm_e_in, strm_addr1, strm_addr2, strm_addr3, strm_addr4,

                        strm_bit, strm_prob, strm_e, strm_tab_dbg);

    vpx_enc_syn(
        // Iteration for variable
        range, count, value, pre_byte, run, br_isFirst, pos,
        // input
        strm_bit, strm_prob, strm_e, strm_tab_dbg,
        // output
        strm_pos_o_e, strm_pos_o_byte);
}
// ------------------------------------------------------------
void LeptonE_pre_engine(
    // input
    hls::stream<ap_int<11> > coef[8],

    uint16_t axi_width[MAX_NUM_COLOR], // colldata->block_width(i);
    uint8_t axi_map_row2cmp[4],        //     AXI                   2,1,0,0 2,1,0
    uint8_t min_nois_thld_x[MAX_NUM_COLOR][64],
    uint8_t min_nois_thld_y[MAX_NUM_COLOR][64],
    uint8_t q_tables[MAX_NUM_COLOR][8][8], //[64],
    int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],
    int32_t idct_q_table_y[MAX_NUM_COLOR][8][8],

    uint16_t axi_mcuv,
    uint8_t axi_num_cmp_mcu,

    // output

    // 7x7
    hls::stream<ap_uint<4> >& strm_nonzero_bin_tmp,

    hls::stream<ap_uint<6> >& strm_7x7_nz,
    hls::stream<ap_uint<4> >& strm_7x7_length,

    hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
    hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

    hls::stream<bool>& strm_7x7_cur_bit_sign_tmp,

    hls::stream<ap_uint<11> >& strm_abs_coef,
    hls::stream<ap_uint<6> >& strm_coord,

    // edge
    // from preprossess to edge
    hls::stream<ap_uint<6> >& non_zero_h_out,
    hls::stream<ap_uint<3> >& coef_cnt_h_len,
    hls::stream<ap_uint<3> >& coef_cnt_v_len,
    hls::stream<ap_uint<3> >& eob_x,
    hls::stream<ap_uint<3> >& eob_y,

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
    hls::stream<ap_uint<6> >& strm_coord_nois_v,

    // dc
    hls::stream<int16_t>& strm_coef_dc,
    hls::stream<int>& strm_uncertainty,
    hls::stream<int>& strm_uncertainty2) {
    bool is_top_row[MAX_NUM_COLOR] = {true, true, true};
// clang-format off
 #pragma HLS ARRAY_PARTITION variable=axi_map_row2cmp complete dim=0
 #pragma HLS ARRAY_PARTITION variable=axi_width 		 complete dim=0
 #pragma HLS ARRAY_PARTITION variable=is_top_row      complete dim=0
    // clang-format on

    for (int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++) {
        for (int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu; idx_cmp++) {
            uint8_t id_cmp = axi_map_row2cmp[idx_cmp];
            uint16_t block_width = axi_width[id_cmp];
            bool is_top_row_cmp = is_top_row[id_cmp];
            uint8_t q0 = q_tables[id_cmp][0][0];

            pre_lepton_encoder_line(block_width, id_cmp, is_top_row_cmp,

                                    q_tables, q0, idct_q_table_x, idct_q_table_y, min_nois_thld_x, min_nois_thld_y,

                                    coef,
                                    // 77
                                    strm_nonzero_bin_tmp,

                                    strm_7x7_nz, strm_7x7_length,

                                    strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                                    strm_7x7_cur_bit_sign_tmp,

                                    strm_abs_coef, strm_coord,
                                    // edge
                                    non_zero_h_out, coef_cnt_h_len, coef_cnt_v_len, eob_x, eob_y,

                                    strm_length_exp_h,

                                    strm_best_prior_exp_h,

                                    strm_cur_bit_sign_h, strm_tri_sign_h,

                                    strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                                    ////
                                    strm_length_exp_v,

                                    strm_best_prior_exp_v,

                                    strm_cur_bit_sign_v, strm_tri_sign_v,

                                    strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v,
                                    // dc
                                    strm_coef_dc, strm_uncertainty, strm_uncertainty2);
            is_top_row[id_cmp] = false;
        } // for(int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu ; idx_cmp++)
    }     //("process_row_range_while");for( int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++)
}
// ------------------------------------------------------------
void LeptonE_push_engine(
    // input
    // hls::stream<ap_int<11> >  coef[8],

    uint16_t axi_width[MAX_NUM_COLOR], // colldata->block_width(i);
    uint8_t axi_map_row2cmp[4],        //     AXI                   2,1,0,0 2,1,0

    uint16_t axi_mcuv,
    uint8_t axi_num_cmp_mcu,

    // 7x7
    hls::stream<ap_uint<4> >& strm_nonzero_bin_tmp,

    hls::stream<ap_uint<6> >& strm_7x7_nz,
    hls::stream<ap_uint<4> >& strm_7x7_length,

    hls::stream<ap_uint<4> >& strm_7x7_num_nonzero_bin,
    hls::stream<ap_uint<4> >& strm_7x7_bsr_best_prior,

    hls::stream<bool>& strm_7x7_cur_bit_sign_tmp,

    hls::stream<ap_uint<11> >& strm_abs_coef,
    hls::stream<ap_uint<6> >& strm_coord,

    // edge
    // from preprossess to edge
    hls::stream<ap_uint<6> >& non_zero_h_out,
    hls::stream<ap_uint<3> >& coef_cnt_h_len,
    hls::stream<ap_uint<3> >& coef_cnt_v_len,
    hls::stream<ap_uint<3> >& eob_x,
    hls::stream<ap_uint<3> >& eob_y,

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
    hls::stream<ap_uint<6> >& strm_coord_nois_v,

    // dc
    hls::stream<int16_t>& strm_coef,
    hls::stream<int>& strm_uncertainty,
    hls::stream<int>& strm_uncertainty2,

    // output
    struct_arith& axi_arith,
    hls::stream<bool>& strm_pos_o_e,
    hls::stream<ap_uint<8> >& strm_pos_o_byte

    ) {
    bool is_top_row[MAX_NUM_COLOR] = {true, true, true};
// clang-format off
//#pragma HLS ARRAY_PARTITION variable=axi_map_row2cmp complete dim=0
#pragma HLS ARRAY_PARTITION variable=axi_width 		 complete dim=0
#pragma HLS ARRAY_PARTITION variable=is_top_row      complete dim=0
    // clang-format on

    unsigned char range = 128;  // boolwriter.range;
    int count = -24;            // boolwriter.count;
    unsigned int value = 0;     // boolwriter.lowvalue;
    unsigned char pre_byte = 0; // boolwriter.pre_byte;
    unsigned short run = 0;     // boolwriter.run;
    bool isFirst = 1;           // boolwriter.isFirst;
    unsigned int pos = 0;       // boolwriter.pos;
    unsigned int pos2 = 0;

    for (int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++) {
        for (int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu; idx_cmp++) {
            uint8_t id_cmp = axi_map_row2cmp[idx_cmp];
            uint16_t block_width = axi_width[id_cmp];

            push_lepton_encoder_line(block_width, id_cmp,
                                     // 77
                                     strm_nonzero_bin_tmp,

                                     strm_7x7_nz, strm_7x7_length,

                                     strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                                     strm_7x7_cur_bit_sign_tmp,

                                     strm_abs_coef, strm_coord,
                                     // edge
                                     non_zero_h_out, coef_cnt_h_len, coef_cnt_v_len, eob_x, eob_y,

                                     strm_length_exp_h,

                                     strm_best_prior_exp_h,

                                     strm_cur_bit_sign_h, strm_tri_sign_h,

                                     strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                                     ////
                                     strm_length_exp_v,

                                     strm_best_prior_exp_v,

                                     strm_cur_bit_sign_v, strm_tri_sign_v,

                                     strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v,
                                     // dc
                                     strm_coef, strm_uncertainty, strm_uncertainty2,

                                     &range, &count, &value, &pre_byte, &run, &isFirst, &pos,

                                     strm_pos_o_e, strm_pos_o_byte);

        } // for(int idx_cmp = 0; idx_cmp < axi_num_cmp_mcu ; idx_cmp++)
    }     //("process_row_range_while");for( int i_mcuv = 0; i_mcuv < axi_mcuv; i_mcuv++)

    strm_pos_o_e.write(true);
    axi_arith.count = count;
    axi_arith.value = value;
    axi_arith.pre_byte = pre_byte;
    axi_arith.run = run;
    axi_arith.pos = pos;
    axi_arith.range = range;
    axi_arith.isFirst = isFirst;
}

} // namespace details
} // namespace codec
} // namespace xf

namespace xf {
namespace codec {
namespace details {
void kernel_LeptonE_strmIn_engine(
    // input
    hls::stream<ap_int<11> > coef[8],

    uint16_t axi_width[MAX_NUM_COLOR], // colldata->block_width(i);
    uint8_t axi_map_row2cmp[4],        //     AXI                   2,1,0,0 2,1,0
    uint8_t min_nois_thld_x[MAX_NUM_COLOR][64],
    uint8_t min_nois_thld_y[MAX_NUM_COLOR][64],
    uint8_t q_tables[MAX_NUM_COLOR][8][8], //[64],
    int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],
    int32_t idct_q_table_y[MAX_NUM_COLOR][8][8],

    uint16_t axi_mcuv,
    uint8_t axi_num_cmp_mcu,

    // output
    struct_arith& axi_arith,
    hls::stream<bool>& strm_pos_o_e,
    hls::stream<ap_uint<8> >& strm_pos_o_byte

    ) {
#pragma HLS DATAFLOW

// clang-format off
#pragma HLS ARRAY_PARTITION variable=axi_map_row2cmp complete dim=0
#pragma HLS ARRAY_PARTITION variable=axi_width 		 complete dim=0
//#pragma HLS ARRAY_PARTITION variable=is_top_row      complete dim=0
#pragma HLS ARRAY_PARTITION variable=idct_q_table_x  complete dim=3
#pragma HLS ARRAY_PARTITION variable=idct_q_table_y  complete dim=3
    // clang-format on

    // clang-format off
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

    static hls::stream<ap_uint<11> > strm_abs_coef("coef_abs");
#pragma HLS stream depth=32 variable=strm_abs_coef
    hls::stream<ap_uint<6> > strm_coord("strm_coord");
#pragma HLS stream depth=32 variable=strm_coord
    // clang-format on

    // edge
    // clang-format off
    static hls::stream<ap_uint<3> > coef_cnt_h_len("coef_cnt_h_len");
#pragma HLS bind_storage variable=coef_cnt_h_len type=FIFO impl=LUTRAM
#pragma HLS stream depth=512 variable=coef_cnt_h_len

    static hls::stream<ap_uint<3> > coef_cnt_v_len("coef_cnt_v_len");
#pragma HLS bind_storage variable=coef_cnt_v_len type=FIFO impl=LUTRAM
#pragma HLS stream depth=512 variable=coef_cnt_v_len

    static hls::stream<ap_uint<6> > non_zero_h_out;
#pragma HLS stream depth=512 variable=non_zero_h_out
#pragma HLS bind_storage variable=non_zero_h_out type=FIFO impl=LUTRAM

    hls::stream<ap_uint<3> > eob_x("eob_x");
#pragma HLS stream depth=512 variable=eob_x
#pragma HLS bind_storage variable=eob_x type=FIFO impl=LUTRAM
	hls::stream<ap_uint<3> > eob_y("eob_y");
#pragma HLS stream depth=512 variable=eob_y
#pragma HLS bind_storage variable=eob_y type=FIFO impl=LUTRAM


	hls::stream<ap_uint<4> > strm_best_prior_exp_h("bsr_exp_h");
#pragma HLS stream depth=512 variable=strm_best_prior_exp_h

	hls::stream<bool> strm_cur_bit_sign_h("sign_bit_h");
#pragma HLS stream depth=512 variable=strm_cur_bit_sign_h
	hls::stream<ap_uint<2> > strm_tri_sign_h("tri_sign_h");
#pragma HLS stream depth=512 variable=strm_tri_sign_h

	static hls::stream<ap_uint<11> > strm_abs_coef_nois_h("abs_coef_h");
#pragma HLS stream depth=512 variable=strm_abs_coef_nois_h
	hls::stream<ap_uint<8> > strm_ctx_nois_h;
#pragma HLS stream depth=512 variable=strm_ctx_nois_h
	hls::stream<ap_uint<8> > strm_min_nois_h;
#pragma HLS stream depth=512 variable=strm_min_nois_h
	hls::stream<ap_uint<6> > strm_coord_nois_h;
#pragma HLS stream depth=512 variable=strm_coord_nois_h

	hls::stream<ap_uint<4> > strm_best_prior_exp_v;
#pragma HLS stream depth=512 variable=strm_best_prior_exp_v

	hls::stream<bool> strm_cur_bit_sign_v("sign_bit_v");
#pragma HLS stream depth=512 variable=strm_cur_bit_sign_v
	hls::stream<ap_uint<2> > strm_tri_sign_v;
#pragma HLS stream depth=512 variable=strm_tri_sign_v

	static hls::stream<ap_uint<11> > strm_abs_coef_nois_v("abs_coef_v");
#pragma HLS stream depth=512 variable=strm_abs_coef_nois_v
	hls::stream<ap_uint<8> > strm_ctx_nois_v;
#pragma HLS stream depth=512 variable=strm_ctx_nois_v
	hls::stream<ap_uint<8> > strm_min_nois_v;
#pragma HLS stream depth=512 variable=strm_min_nois_v
	hls::stream<ap_uint<6> > strm_coord_nois_v;
#pragma HLS stream depth=512 variable=strm_coord_nois_v

	hls::stream<ap_uint<4> > strm_length_exp_h("len_exp_h");
#pragma HLS stream depth=512 variable=strm_length_exp_h
	hls::stream<ap_uint<4> > strm_length_exp_v("len_exp_v");
#pragma HLS stream depth=512 variable=strm_length_exp_v
    // clang-format on

    // clang-format off
	hls::stream<int16_t> strm_coef_dc;
#pragma HLS STREAM variable=strm_coef_dc depth=32 dim=1
#pragma HLS bind_storage variable=strm_coef_dc type=FIFO impl=LUTRAM

	hls::stream<int>     strm_uncertainty("uncertainty");
#pragma HLS bind_storage variable=strm_uncertainty type=FIFO impl=LUTRAM
#pragma HLS STREAM variable=strm_uncertainty depth=32 dim=1

	hls::stream<int>     strm_uncertainty2("uncertainty2");
#pragma HLS bind_storage variable=strm_uncertainty2 type=FIFO impl=LUTRAM
#pragma HLS STREAM variable=strm_uncertainty2 depth=32 dim=1
    // clang-format on

    LeptonE_pre_engine(coef, axi_width,
                       axi_map_row2cmp, //     AXI                   2,1,0,0 2,1,0
                       min_nois_thld_x, min_nois_thld_y,
                       q_tables, //[64],
                       idct_q_table_x, idct_q_table_y,

                       axi_mcuv, axi_num_cmp_mcu,

                       // 77
                       strm_nonzero_bin_tmp,

                       strm_7x7_nz, strm_7x7_length,

                       strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                       strm_7x7_cur_bit_sign_tmp,

                       strm_abs_coef, strm_coord,
                       // edge
                       non_zero_h_out, coef_cnt_h_len, coef_cnt_v_len, eob_x, eob_y,

                       strm_length_exp_h,

                       strm_best_prior_exp_h,

                       strm_cur_bit_sign_h, strm_tri_sign_h,

                       strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                       ////
                       strm_length_exp_v,

                       strm_best_prior_exp_v,

                       strm_cur_bit_sign_v, strm_tri_sign_v,

                       strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v,
                       // dc
                       strm_coef_dc, strm_uncertainty, strm_uncertainty2);

    LeptonE_push_engine(axi_width,
                        axi_map_row2cmp, //     AXI                   2,1,0,0 2,1,0

                        axi_mcuv, axi_num_cmp_mcu,

                        // 77
                        strm_nonzero_bin_tmp,

                        strm_7x7_nz, strm_7x7_length,

                        strm_7x7_num_nonzero_bin, strm_7x7_bsr_best_prior,

                        strm_7x7_cur_bit_sign_tmp,

                        strm_abs_coef, strm_coord,
                        // edge
                        non_zero_h_out, coef_cnt_h_len, coef_cnt_v_len, eob_x, eob_y,

                        strm_length_exp_h,

                        strm_best_prior_exp_h,

                        strm_cur_bit_sign_h, strm_tri_sign_h,

                        strm_abs_coef_nois_h, strm_ctx_nois_h, strm_min_nois_h, strm_coord_nois_h,

                        ////
                        strm_length_exp_v,

                        strm_best_prior_exp_v,

                        strm_cur_bit_sign_v, strm_tri_sign_v,

                        strm_abs_coef_nois_v, strm_ctx_nois_v, strm_min_nois_v, strm_coord_nois_v,
                        // dc
                        strm_coef_dc, strm_uncertainty, strm_uncertainty2,

                        axi_arith, strm_pos_o_e, strm_pos_o_byte);
}

} // namespace details
} // namespace codec
} // namespace xf
