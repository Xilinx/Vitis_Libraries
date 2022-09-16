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
 * @file XAcc_77.hpp
 * @brief lepton 7x7 function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XAcc_77.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XACC_77_H_
#define _XACC_77_H_
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_math.h>
#include "XAcc_common.hpp"

static ap_uint<4> hls_nonzero_to_bin_9[50] = {
    0, 1, 2, 3, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
};

static const unsigned char hls_unzigzag49[] = {9,  10, 17, 25, 18, 11, 12, 19, 26, 33, 41, 34, 27, 20, 13, 14, 21,
                                               28, 35, 42, 49, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 58,
                                               59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

namespace xf {
namespace codec {
namespace details {

void duplicate_coef(hls::stream<ap_int<11> > coef[8],
                    ap_uint<32> len,
                    hls::stream<ap_int<11> > strm_coef[8],
                    hls::stream<coef_t> str_rast8[8],
                    hls::stream<coef_t>& str_dc_in);

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
                hls::stream<ap_uint<3> >& eob_y);

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
                             //	    hls::stream<bool>		 & strm_e,
                             hls::stream<ap_uint<16> >& strm_addr1,
                             hls::stream<ap_uint<16> >& strm_addr2,
                             hls::stream<ap_uint<16> >& strm_addr3,
                             hls::stream<ap_uint<16> >& strm_addr4);

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
                             hls::stream<ap_uint<6> >& strm_coord);

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
                  hls::stream<ap_uint<16> >& strm_addr4);

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
                     hls::stream<ap_uint<16> >& strm_addr4);

} // namespace details
} // namespace codec
} // namespace xf
#endif
