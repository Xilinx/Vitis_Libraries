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
 * @file XAcc_arith.hpp
 * @brief lepton arith function API.
 *
 * This file is part of HLS algorithm library.
 */

#ifndef __cplusplus
#error " XAcc_arith.hpp hls::stream<> interface, and thus requires C++"
#endif

#ifndef _XACC_ARITH_H_
#define _XACC_ARITH_H_
#include <ap_int.h>
#include <hls_stream.h>
#include "XAcc_common.hpp"
////TMEP

namespace xf {
namespace codec {
namespace details {

void vpx_enc_range(
    // input
    unsigned char* br_range,
    hls::stream<bool>& strm_bit,
    hls::stream<uint8_t>& strm_prob,
    hls::stream<bool>& strm_e_range,
    hls::stream<uint8_t>& strm_tab_dbg,
    // output
    hls::stream<bool>& strm_range_o_e,
    hls::stream<ap_uint<3> >& strm_range_o_shift,
    hls::stream<unsigned char>& strm_range_o_split);

void vpx_enc_value(
    //
    int* br_count,
    unsigned int* br_lowvalue,
    hls::stream<bool>& strm_range_o_e,
    hls::stream<ap_uint<3> >& strm_range_o_shift,
    hls::stream<unsigned char>& strm_range_o_split,
    // Outout ////////////////////
    hls::stream<bool>& strm_value_o_e,
    hls::stream<bool>& strm_value_o_cy,
    hls::stream<unsigned char>& strm_value_o_byte);

void vpx_enc_run(unsigned char* br_pre_byte,
                 unsigned short* br_run,
                 bool* br_isFirst,
                 hls::stream<bool>& strm_value_o_e,
                 hls::stream<bool>& strm_value_o_cy,
                 hls::stream<unsigned char>& strm_value_o_byte,
                 // Outout ////////////////////
                 hls::stream<bool>& strm_CyByte_o_e,
                 hls::stream<bool>& strm_CyByte_o_cy,
                 hls::stream<unsigned char>& strm_CyByte_o_byte,
                 hls::stream<unsigned short>& strm_CyByte_o_run);

void vpx_enc_pos(unsigned int* br_pos,
                 hls::stream<bool>& strm_CyByte_o_e,
                 hls::stream<bool>& strm_CyByte_o_cy,
                 hls::stream<unsigned char>& strm_CyByte_o_byte,
                 hls::stream<unsigned short>& strm_CyByte_o_run,
                 // Outout ////////////////////
                 hls::stream<bool>& strm_pos_o_e,
                 hls::stream<unsigned char>& strm_pos_o_byte);

void vpx_enc_syn(
    // Iteration for variable
    unsigned char* range,
    int* cnt,
    unsigned int* value,
    unsigned char* pre_byte,
    unsigned short* run,
    bool* br_isFirst,
    unsigned int* pos,
    // input
    hls::stream<bool>& strm_bit,
    hls::stream<uint8_t>& strm_prob,
    hls::stream<bool>& strm_e_range,
    hls::stream<uint8_t>& strm_tab_dbg,
    // output
    hls::stream<bool>& strm_pos_o_e,
    hls::stream<ap_uint<8> >& strm_pos_o_byte);

} // namespace details
} // namespace codec
} // namespace xf
#endif
