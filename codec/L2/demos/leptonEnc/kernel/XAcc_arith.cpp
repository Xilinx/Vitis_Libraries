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
// XAcc_arith.cpp
#include "XAcc_arith.hpp"

////TMEP
namespace xf {
namespace codec {
namespace details {

// ------------------------------------------------------------
void vpx_enc_range_noDSP(
    // void vpx_enc_range(

    // input
    unsigned char* br_range,
    hls::stream<bool>& strm_bit,
    hls::stream<uint8_t>& strm_prob,
    hls::stream<bool>& strm_e_range,
    hls::stream<uint8_t>& strm_tab_dbg,
    // output
    hls::stream<bool>& strm_range_o_e,
    hls::stream<ap_uint<3> >& strm_range_o_shift,
    hls::stream<unsigned char>& strm_range_o_split) { // Iteration for variable range
    unsigned char range = *br_range;
    bool e_range = strm_e_range.read();
    while (!e_range) {
#pragma HLS pipeline II = 1
        e_range = strm_e_range.read();
        bool bit1 = strm_bit.read();
        uint8_t prob = strm_prob.read();

        unsigned char split1, split_1;
        ap_uint<3> shift1;
        //#pragma HLS bind_storage variable = split_1 core = Mul_LUT
        split_1 = (((range - 1) * prob) >> 8);

        if (bit1)
            range = range - split_1 - 1;
        else
            range = split_1 + 1;

        ap_uint<8> range2 = range;
        shift1 = range2.countLeadingZeros();
        range <<= shift1;

        if (bit1 || shift1) {
            strm_range_o_shift.write(shift1);
            strm_range_o_split.write(bit1 == true ? split_1 + 1 : 0);
            strm_range_o_e.write(false);
        }
        uint8_t dbg = strm_tab_dbg.read();
    }
    strm_range_o_e.write(true);
    *br_range = range;
} // End of iteration for range /////////////////////////////////////////////////

// ------------------------------------------------------------
void vpx_enc_range_org_DSP_NorLzd(
    // input
    unsigned char* br_range,
    hls::stream<bool>& strm_bit,
    hls::stream<uint8_t>& strm_prob,
    hls::stream<bool>& strm_e_range,
    hls::stream<uint8_t>& strm_tab_dbg,
    // output
    hls::stream<bool>& strm_range_o_e,
    hls::stream<ap_uint<3> >& strm_range_o_shift,
    hls::stream<unsigned char>& strm_range_o_split) { // Iteration for variable range
    unsigned char range = *br_range;
    bool e_range = strm_e_range.read();
    while (!e_range) {
#pragma HLS pipeline II = 1
        e_range = strm_e_range.read();
        bool bit1 = strm_bit.read();
        uint8_t prob = strm_prob.read();

        unsigned char split1;
        ap_uint<3> shift1;
        split1 = 1 + (((range - 1) * prob) >> 8);

        if (bit1)
            range = range - split1;
        else
            range = split1;

        ap_uint<8> range2 = range;
        shift1 = range2.countLeadingZeros();
        range <<= shift1;

        if (bit1 || shift1) {
            strm_range_o_shift.write(shift1);
            strm_range_o_split.write(bit1 == true ? split1 : 0);
            strm_range_o_e.write(false);
        }
        uint8_t dbg = strm_tab_dbg.read();
    }
    strm_range_o_e.write(true);
    *br_range = range;
} // End of iteration for range /////////////////////////////////////////////////

// ------------------------------------------------------------
void vpx_enc_range_DSP_fastLzd(
    // input
    unsigned char* br_range,
    hls::stream<bool>& strm_bit,
    hls::stream<uint8_t>& strm_prob,
    hls::stream<bool>& strm_e_range,
    hls::stream<uint8_t>& strm_tab_dbg,
    // output
    hls::stream<bool>& strm_range_o_e,
    hls::stream<ap_uint<3> >& strm_range_o_shift,
    hls::stream<unsigned char>& strm_range_o_split) { // Iteration for variable range
    const ap_uint<2> tt[16] = {0, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char range = *br_range;
    bool e_range = strm_e_range.read();
    while (!e_range) {
#pragma HLS pipeline II = 1
        e_range = strm_e_range.read();
        bool bit1 = strm_bit.read();
        uint8_t prob = strm_prob.read();

        unsigned char split1;
        ap_uint<3> shift1;
        split1 = 1 + (((range - 1) * prob) >> 8);

        unsigned char range0, range1;
        range1 = range - split1;
        range0 = split1;

        ap_uint<3> s0 = 0;
        ap_uint<3> s1 = 0;
        if (range0 & 0xF0)
            s0 = tt[range0 >> 4];
        else
            s0 = 4 + tt[range0];
        if (range1 & 0xF0)
            s1 = tt[range1 >> 4];
        else
            s1 = 4 + tt[range1];
        range0 <<= s0;
        range1 <<= s1;

        if (bit1) {
            range = range1;
            shift1 = s1;
        } else {
            range = range0;
            shift1 = s0;
        }

        if (bit1 || shift1) {
            strm_range_o_shift.write(shift1);
            strm_range_o_split.write(bit1 == true ? split1 : 0);
            strm_range_o_e.write(false);
        }
        uint8_t dbg = strm_tab_dbg.read();
    }
    strm_range_o_e.write(true);
    *br_range = range;
} // End of iteration for range /////////////////////////////////////////////////
// void vpx_enc_range_NoDsp_tt8Lzd(// No dsp, faster Leading zero detecting

// ------------------------------------------------------------
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
    hls::stream<unsigned char>& strm_range_o_split) { // Iteration for variable range
    const ap_uint<2> tt[16] = {0, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char range = *br_range;
    bool e_range = strm_e_range.read();
    while (!e_range) {
#pragma HLS pipeline II = 1
        e_range = strm_e_range.read();
        bool bit1 = strm_bit.read();
        uint8_t prob = strm_prob.read();

        unsigned char split_1;
        ap_uint<3> shift1;
        //#pragma HLS bind_storage variable = split_1 core = Mul_LUT
        // split1 = 1 + (((range - 1) * prob) >> 8);
        split_1 = (((range - 1) * prob) >> 8);
        unsigned char range0, range1;
        range1 = range - split_1 - 1;
        range0 = split_1 + 1;

        ap_uint<3> s0 = 0;
        ap_uint<3> s1 = 0;
        if (range0 & 0xF0)
            s0 = tt[range0 >> 4];
        else
            s0 = 4 + tt[range0];
        if (range1 & 0xF0)
            s1 = tt[range1 >> 4];
        else
            s1 = 4 + tt[range1];
        range0 <<= s0;
        range1 <<= s1;

        if (bit1) {
            range = range1;
            shift1 = s1;
        } else {
            range = range0;
            shift1 = s0;
        }

        if (bit1 || shift1) {
            strm_range_o_shift.write(shift1);
            strm_range_o_split.write(bit1 == true ? split_1 + 1 : 0);
            strm_range_o_e.write(false);
        }
        uint8_t dbg = strm_tab_dbg.read();
    }
    strm_range_o_e.write(true);
    *br_range = range;
} // End of iteration for range /////////////////////////////////////////////////

// void vpx_enc_range(

// ------------------------------------------------------------
void vpx_enc_range_lut_lzd(
    // input
    unsigned char* br_range,
    hls::stream<bool>& strm_bit,
    hls::stream<uint8_t>& strm_prob,
    hls::stream<bool>& strm_e_range,
    hls::stream<uint8_t>& strm_tab_dbg,
    // output
    hls::stream<bool>& strm_range_o_e,
    hls::stream<ap_uint<3> >& strm_range_o_shift,
    hls::stream<unsigned char>& strm_range_o_split) { // Iteration for variable range
    const ap_uint<2> tt[16] = {0, 3, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0};
    const ap_uint<3> tt256[256] = {
        0, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const ap_uint<3> tt128[128] = {0, 6, 5, 5, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
                                   2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    const ap_uint<3> tt64[64] = {0, 5, 4, 4, 3, 3, 3, 3, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
                                 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    unsigned char range = *br_range;
    bool e_range = strm_e_range.read();
    while (!e_range) {
#pragma HLS pipeline II = 1
        e_range = strm_e_range.read();
        bool bit1 = strm_bit.read();
        uint8_t prob = strm_prob.read();

        unsigned char split_1;
        ap_uint<3> shift1;
        //#pragma HLS bind_storage variable = split_1 core = Mul_LUT
        // split1 = 1 + (((range - 1) * prob) >> 8);
        split_1 = (((range - 1) * prob) >> 8);
        unsigned char range0, range1;
        range1 = range - split_1 - 1;
        range0 = split_1 + 1;

        ap_uint<3> s0 = 0;
        ap_uint<3> s1 = 0;
        // if (range0&0xF0) s0 = tt[range0>>4];
        // else s0 = 4 + tt[range0];
        // if (range1&0xF0) s1 = tt[range1>>4];
        // else s1 = 4 + tt[range1];
        ////////////////////////////
        s0 = tt256[range0];
        s1 = tt256[range1];
        ///////////////////////////////////////
        // if (range0&0xFc) s0 = tt64[range0>>2];
        // else s0 = 2 + tt64[range0];
        // if (range1&0xFc) s1 = tt64[range1>>2];
        // else s1 = 2 + tt64[range1];

        range0 <<= s0;
        range1 <<= s1;

        if (bit1) {
            range = range1;
            shift1 = s1;
        } else {
            range = range0;
            shift1 = s0;
        }

        if (bit1 || shift1) {
            strm_range_o_shift.write(shift1);
            strm_range_o_split.write(bit1 == true ? split_1 + 1 : 0);
            strm_range_o_e.write(false);
        }
        uint8_t dbg = strm_tab_dbg.read();
    }
    strm_range_o_e.write(true);
    *br_range = range;
} // End of iteration for range /////////////////////////////////////////////////

// ------------------------------------------------------------
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
    hls::stream<unsigned char>& strm_value_o_byte) { // Iteration for variable value and count
    unsigned char cnt24 = *br_count + 24;
    ap_uint<32> value = *br_lowvalue;
    // Pre-reading//////////////////////
    bool e_value = strm_range_o_e.read();
    while (!e_value) {
#pragma HLS pipeline II = 1
        e_value = strm_range_o_e.read();
        ap_uint<3> shift = strm_range_o_shift.read();
        unsigned char split = strm_range_o_split.read();

        value += split;
        unsigned char pre_byte = value(cnt24 + 7, cnt24);
        bool cy = value[cnt24 + 8];
        ap_uint<32> value2;

        if (cnt24 > 0) value2(cnt24 - 1, 0) = value(cnt24 - 1, 0);

        value2(31, cnt24) = 0;

        bool isBE24 = (cnt24 + shift) >= 24;
        int cnt_sh = cnt24 + shift;

        if (isBE24) {
            cnt24 = cnt24 + shift - 8;
            value2 <<= shift;
            value = value2;

            strm_value_o_e.write(false);
            strm_value_o_cy.write(cy);
            strm_value_o_byte.write(pre_byte);
        } else {
            cnt24 = cnt24 + shift;
            value <<= shift;
        }
    }
    strm_value_o_e.write(true);
    *br_lowvalue = value;
    *br_count = (int)cnt24 - 24;
}

// ------------------------------------------------------------
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
                 hls::stream<unsigned short>& strm_CyByte_o_run) { // Iteration for variable
    unsigned char pre_byte = *br_pre_byte;
    unsigned short run = *br_run;
    // Pre-reading//////////////////////
    bool e_cy_preByte = strm_value_o_e.read();
    while (!e_cy_preByte) {
        e_cy_preByte = strm_value_o_e.read();
        bool cy = strm_value_o_cy.read();
        unsigned char new_byte = strm_value_o_byte.read();
        if (*br_isFirst) {
            pre_byte = new_byte;
            *br_isFirst = false;
        } else {
            if (new_byte == 0xff) {
                run++;
            } else { // if not 0xff, must emit pre_byte and run-byte if any
                // br.buffer[pos++] = pre_byte + (cy?1:0);
                // for(; run > 0; run--)
                //	br.buffer[pos++] = cy?0:0xff;
                strm_CyByte_o_cy.write(cy);
                strm_CyByte_o_byte.write(pre_byte);
                strm_CyByte_o_run.write(run);
                strm_CyByte_o_e.write(false);
                pre_byte = new_byte;
                run = 0;
            }
        }

    } // while
    strm_CyByte_o_e.write(true);
    *br_pre_byte = pre_byte;
    *br_run = run;
}

// ------------------------------------------------------------
void vpx_enc_pos(unsigned int* br_pos,
                 hls::stream<bool>& strm_CyByte_o_e,
                 hls::stream<bool>& strm_CyByte_o_cy,
                 hls::stream<unsigned char>& strm_CyByte_o_byte,
                 hls::stream<unsigned short>& strm_CyByte_o_run,
                 // Outout ////////////////////
                 hls::stream<bool>& strm_pos_o_e,
                 hls::stream<ap_uint<8> >& strm_pos_o_byte) { // Iteration for variable pos
    unsigned int pos = *br_pos;
    // Pre-reading//////////////////////
    bool e_pos = strm_CyByte_o_e.read();
    while (!e_pos) {
        e_pos = strm_CyByte_o_e.read();
        bool cy = strm_CyByte_o_cy.read();
        unsigned char byte = strm_CyByte_o_byte.read();
        unsigned char rn = strm_CyByte_o_run.read();
        strm_pos_o_byte.write(byte + (cy ? 1 : 0));
#ifndef __SYNTHESIS__
//				uint8_t test1 = byte + (cy?1:0);
//				fprintf( stderr,  " %.4x\n" , test1);
#endif
        pos++;
        strm_pos_o_e.write(false);
        for (; rn > 0; rn--) {
            strm_pos_o_byte.write(cy ? 0 : 0xff);
            pos++;
            strm_pos_o_e.write(false);
        }
    }
    *br_pos = pos;
    // strm_pos_o_e.write(true);
}

// ------------------------------------------------------------
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
    hls::stream<ap_uint<8> >& strm_pos_o_byte) {
#pragma HLS dataflow
    // clang-format off
	 hls::stream< bool >          strm_range_o_e;
	#pragma HLS stream depth=64 variable=strm_range_o_e
	#pragma HLS bind_storage variable=strm_range_o_e type=FIFO impl=LUTRAM
	    hls::stream< ap_uint<3> >    strm_range_o_shift;
	#pragma HLS stream depth=64 variable=strm_range_o_shift
	#pragma HLS bind_storage variable=strm_range_o_shift type=FIFO impl=LUTRAM
	    hls::stream< unsigned char > strm_range_o_split;
	#pragma HLS stream depth=64 variable=strm_range_o_split
	#pragma HLS bind_storage variable=strm_range_o_split type=FIFO impl=LUTRAM
    // clang-format on

    vpx_enc_range(
        // input
        range,        // unsigned char* br_range,
        strm_bit,     // hls::stream<bool>&    strm_bit,
        strm_prob,    // hls::stream<uint8_t>& strm_prob,
        strm_e_range, // hls::stream<bool>&    strm_e_range,
        strm_tab_dbg, // hls::stream<uint8_t>& strm_tab_dbg,
        // output
        strm_range_o_e,     // hls::stream< bool >          &strm_range_o_e,
        strm_range_o_shift, // hls::stream< ap_uint<3> >    &strm_range_o_shift,
        strm_range_o_split  // hls::stream< unsigned char > &strm_range_o_split
        );

    // clang-format off
    hls::stream< bool >          strm_value_o_e;
#pragma HLS stream depth=64 variable=strm_value_o_e
#pragma HLS bind_storage variable=strm_value_o_e type=FIFO impl=LUTRAM
    hls::stream< bool >          strm_value_o_cy;
#pragma HLS stream depth=64 variable=strm_value_o_cy
#pragma HLS bind_storage variable=strm_value_o_cy type=FIFO impl=LUTRAM
    hls::stream< unsigned char > strm_value_o_byte;
#pragma HLS stream depth=64 variable=strm_value_o_byte
#pragma HLS bind_storage variable=strm_value_o_byte type=FIFO impl=LUTRAM
    // clang-format on

    vpx_enc_value(cnt, value,
                  strm_range_o_e,     // hls::stream< bool >          &strm_range_o_e,
                  strm_range_o_shift, // hls::stream< ap_uint<3> >    &strm_range_o_shift,
                  strm_range_o_split, // hls::stream< unsigned char > &strm_range_o_split,
                  // Outout ////////////////////
                  strm_value_o_e,   // hls::stream< bool >          &strm_value_o_e,
                  strm_value_o_cy,  // hls::stream< bool >          &strm_value_o_cy,
                  strm_value_o_byte // hls::stream< unsigned char > &strm_value_o_byte
                  );

    // clang-format off
    hls::stream< bool >          strm_CyByte_o_e;
#pragma HLS stream depth=64 variable=strm_CyByte_o_e
#pragma HLS bind_storage variable=strm_CyByte_o_e type=FIFO impl=LUTRAM
    hls::stream< bool >          strm_CyByte_o_cy;
#pragma HLS stream depth=64 variable=strm_CyByte_o_cy
#pragma HLS bind_storage variable=strm_CyByte_o_cy type=FIFO impl=LUTRAM
    hls::stream< unsigned char > strm_CyByte_o_byte;
#pragma HLS stream depth=64 variable=strm_CyByte_o_byte
#pragma HLS bind_storage variable=strm_CyByte_o_byte type=FIFO impl=LUTRAM
    hls::stream< unsigned short> strm_CyByte_o_run;
#pragma HLS stream depth=64 variable=strm_CyByte_o_run
#pragma HLS bind_storage variable=strm_CyByte_o_run type=FIFO impl=LUTRAM
    // clang-format on

    vpx_enc_run(pre_byte,          // unsigned char  *br_pre_byte,
                run,               // unsigned short *br_run,
                br_isFirst,        // bool* br_isFirst,
                strm_value_o_e,    // hls::stream< bool >          &strm_value_o_e,
                strm_value_o_cy,   // hls::stream< bool >          &strm_value_o_cy,
                strm_value_o_byte, // hls::stream< unsigned char > &strm_value_o_byte,
                // Outout ////////////////////
                strm_CyByte_o_e,    // hls::stream< bool >          &strm_CyByte_o_e,
                strm_CyByte_o_cy,   // hls::stream< bool >          &strm_CyByte_o_cy,
                strm_CyByte_o_byte, // hls::stream< unsigned char > &strm_CyByte_o_byte,
                strm_CyByte_o_run   // hls::stream< unsigned short> &strm_CyByte_o_run
                );

    vpx_enc_pos(pos,                // unsigned int* br_pos,
                strm_CyByte_o_e,    // hls::stream< bool >          &strm_CyByte_o_e,
                strm_CyByte_o_cy,   // hls::stream< bool >          &strm_CyByte_o_cy,
                strm_CyByte_o_byte, // hls::stream< unsigned char > &strm_CyByte_o_byte,
                strm_CyByte_o_run,  // hls::stream< unsigned short> &strm_CyByte_o_run,
                // Outout ////////////////////
                strm_pos_o_e,   // hls::stream< bool >          strm_pos_o_e,
                strm_pos_o_byte // hls::stream< unsigned char > strm_pos_o_byte
                );
}

} // namespace details
} // namespace codec
} // namespace xf
