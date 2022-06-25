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
 * @file XAcc_edge.hpp
 * @brief lepton edge function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XAcc_edge.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XACC_EDGES_HPP_
#define _XACC_EDGES_HPP_
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>
#include "XAcc_common.hpp"

namespace xf {
namespace codec {
namespace details {

template <bool is_horizontal>
void prepare_edge(ap_uint<32> block_width,
                  bool ap_color,
                  uint8_t min_nois_thld[MAX_NUM_COLOR][64],
                  bool above_present,
                  hls::stream<ap_uint<3> >& strm_lane,
                  int32_t idct_q_table_x[MAX_NUM_COLOR][8][8],

                  hls::stream<ap_int<11> > strm_coef_here[8],
                  hls::stream<ap_int<11> > strm_coef_above[8],
                  hls::stream<bool>& strm_has_left,
                  hls::stream<bool>& strm_coef_end,

                  hls::stream<ap_uint<4> >& strm_length_exp_out,

                  hls::stream<ap_uint<4> >& strm_best_prior_exp,

                  hls::stream<bool>& strm_cur_bit_sign,
                  hls::stream<ap_uint<2> >& strm_tri_sign,

                  hls::stream<ap_uint<11> >& strm_abs_coef_nois,
                  hls::stream<ap_uint<8> >& strm_ctx_nois,
                  hls::stream<ap_uint<8> >& strm_min_nois,
                  hls::stream<ap_uint<6> >& strm_coord_nois

                  ) {
    bool e;

    e = strm_coef_end.read();

    while (!e) {
#pragma HLS pipeline II = 1
        // by zyl ii=1
        ap_uint<3> aligned_block_offset;
        if (is_horizontal)
            aligned_block_offset = 50;
        else
            aligned_block_offset = 57;

        int lane = strm_lane.read();

        ap_int<11> lak_coef_here[8];
        ap_int<11> lak_coef_above[8];
        lak_coef_here[0] = 0;

        ap_int<11> coef = strm_coef_here[0].read(); // by zyl
        lak_coef_above[0] = strm_coef_above[0].read();

        for (int i = 1; i < 8; i++) {
#pragma HLS unroll
            lak_coef_here[i] = strm_coef_here[i].read();
            lak_coef_above[i] = strm_coef_above[i].read();
        }

        ap_int<32> best_prior;
        ap_uint<3> num_nonzeros_bin;
        ap_int<32> bsr_best_prior;

        int sum[7];

        if ((!strm_has_left.read() && !is_horizontal) || (is_horizontal && above_present)) {
            best_prior = lak_coef_above[0] * idct_q_table_x[ap_color][lane + 1][0];
            for (int i = 1; i < 8; ++i) {
#pragma HLS unroll
                int sign = (i & 1) ? 1 : -1;
                sum[i - 1] = idct_q_table_x[ap_color][lane + 1][i] * (lak_coef_here[i] + sign * lak_coef_above[i]);
            }
            best_prior = (best_prior - sum[0] - sum[1] - sum[2] - sum[3] - sum[4] - sum[5] - sum[6]) /
                         idct_q_table_x[ap_color][lane + 1][0];
        } else {
            best_prior = 0;
        }

        bsr_best_prior = hls::min(32 - hls::abs(best_prior).countLeadingZeros(), 11);

        // ap_int<11> coef = strm_coef_here[0].read();

        ap_uint<11> abs_coef = hls::abs(coef);
        ap_uint<4> length;
        length = 11 - abs_coef.countLeadingZeros();

        strm_length_exp_out.write(length);
        strm_best_prior_exp.write(bsr_best_prior);

        uint8_t min_threshold;

        if (is_horizontal)
            min_threshold = min_nois_thld[ap_color][lane + 1];
        else
            min_threshold = min_nois_thld[ap_color][(lane + 1) << 3];

        if (length != 0) {
            strm_cur_bit_sign.write(coef >= 0);
            strm_tri_sign.write(best_prior == 0 ? 0 : (best_prior > 0 ? 1 : 2));
        }

        strm_abs_coef_nois.write(abs_coef);
        uint16_t ctx_abs = hls::abs(best_prior);
        if (is_horizontal)
            strm_coord_nois.write(lane + 1);
        else
            strm_coord_nois.write((lane + 1) << 3);
        strm_ctx_nois.write(hls::min(ctx_abs >> min_threshold, 255));
        strm_min_nois.write(min_threshold);
        e = strm_coef_end.read();
    }
}

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
                     hls::stream<ap_uint<16> >& strm_addr4);

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
                                hls::stream<ap_uint<16> >& strm_addr4);

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
                                hls::stream<ap_uint<6> >& strm_coord_nois_v

                                );

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
                   hls::stream<ap_uint<16> >& strm_addr4);

} // namespace details
} // namespace codec
} // namespace xf

#endif
