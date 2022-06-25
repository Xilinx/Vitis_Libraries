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
 * @file XAcc_dc.hpp
 * @brief lepton dc function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XAcc_dc.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XACC_DC_HPP_
#define _XACC_DC_HPP_
#include <ap_int.h>
#include <hls_stream.h>
#include "XAcc_common.hpp"

namespace xf {
namespace codec {
namespace details {

void hls_serialize_tokens_dc(bool above_present,
                             ap_uint<2> id_cmp,
                             uint16_t block_width,
                             uint8_t q_tables0[MAX_NUM_COLOR][8][8],
                             uint8_t q0,

                             hls::stream<coef_t> str_rast8[8],
                             hls::stream<coef_t>& str_dc_in,

                             hls::stream<ap_uint<4> >& strm_sel_tab,
                             hls::stream<bool>& strm_cur_bit,
                             hls::stream<short>& strm_len,
                             //		hls::stream<bool>		 & strm_e,
                             hls::stream<ap_uint<16> >& strm_addr1,
                             hls::stream<ap_uint<16> >& strm_addr2,
                             hls::stream<ap_uint<16> >& strm_addr3);

void pre_serialize_tokens_dc(bool above_present,
                             ap_uint<2> id_cmp,
                             uint16_t block_width,
                             uint8_t q_tables0[MAX_NUM_COLOR][8][8],
                             uint8_t q0,

                             hls::stream<coef_t> strm_in[8],
                             hls::stream<coef_t>& str_dc_in,

                             hls::stream<int16_t>& strm_coef,
                             hls::stream<int>& strm_uncertainty,
                             hls::stream<int>& strm_uncertainty2);

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

                 );

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

                    );

} // namespace details
} // namespace codec
} // namespace xf
#endif
