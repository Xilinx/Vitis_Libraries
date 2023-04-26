/**********
           Copyright (c) 2017, Xilinx, Inc.
           All rights reserved.
           Redistribution and use in source and binary forms, with or without modification,
           are permitted provided that the following conditions are met:
           1. Redistributions of source code must retain the above copyright notice,
           this list of conditions and the following disclaimer.
           2. Redistributions in binary form must reproduce the above copyright notice,
           this list of conditions and the following disclaimer in the documentation
           and/or other materials provided with the distribution.
           3. Neither the name of the copyright holder nor the names of its contributors
           may be used to endorse or promote products derived from this software
           without specific prior written permission.
           THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
           ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
           THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
           IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
           INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
           PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
           HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
           OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
           EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********/

#include <ap_int.h>
#include <hls_stream.h>
#include "vp8_hls_syn.h"
#include "vp8_hls_syn2.h"
#include <stdio.h>
#include <string.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//==================================kernel_2_ArithmeticCoding===========================================//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// kernel_2_ArithmeticCoding
//|-memcpy
//|-Kernel2_top_read
//|-kernel_2_RecordTokens_pre
//|-kernel_2_CreateTokens_with_isFinal
//|-VP8EmitTokens_str_hls_4stages
//|-PackStr2Mem32_t_NoLast
//|-PackWideStr2Mem32_t_NoLast

void kernel_2_ArithmeticCoding(uint32_t pin_level[SIZE32_MEM_BW],
                               uint8_t* pin_prob, // 2048 instead of [4 * 8 * 3 * 11],
                               uint32_t pout_bw[SIZE32_MEM_BW],
                               uint32_t pout_ret[SIZE32_MEM_RET],
                               uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem3 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem4 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    uint8_t prob[4 * 8 * 3 * 11];
    memcpy(prob, pin_prob, sizeof(prob));

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level,

                     str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h, str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    VP8EmitTokens_str_hls_4stages(pout_bw, tokens_str_final,
                                  (uint8_t*)prob); // VP8EmitTokens_hls(pout_bw, &tokens, (uint8_t*)prob);
    PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
}

//==================================kernel_2_ArithmeticCoding===========================================//
void Kernel2_top_read(uint32_t pin_level[SIZE32_MEM_LEVEL],
                      // output
                      hls::stream<ap_int<WD_LEVEL * 16> >& str_level_dc,
                      hls::stream<ap_int<WD_LEVEL * 16> >& str_level_ac,
                      hls::stream<ap_int<WD_LEVEL * 16> >& str_level_uv,
                      hls::stream<ap_uint<64> >& str_pred,
                      hls::stream<ap_uint<6> >& str_ret,
                      hls::stream<ap_uint<1> >& str_type_mb,
                      hls::stream<uint16_t>& str_mb_h,
                      hls::stream<uint16_t>& str_mb_w) {
    //#pragma HLS INTERFACE m_axi port=pin_level    offset=slave bundle=gmem0 depth=65536*512/2
    // num_read_outstanding=32 num_write_outstanding=32 max_read_burst_length=16 max_write_burst_length=16
    //#pragma HLS INTERFACE s_axilite port=pin_level bundle=control
    //#pragma HLS INTERFACE s_axilite port=return bundle=control
    uint16_t y_mb = 0;
    uint16_t x_mb = 0;
    uint16_t mb_h = 1;
    uint16_t mb_w = 1;
    uint32_t tmp_arr[256];
    uint32_t* psrc = pin_level;
    uint32_t num_mb = 0;
READ_ARRAY_TO_STR:
    do {
#pragma HLS LOOP_TRIPCOUNT min = 120 * 68 max = 256 * 256
#pragma HLS PIPELINE
        memcpy(tmp_arr, psrc, 256 * sizeof(uint32_t));
        psrc += 256;
        if (num_mb == 0) {
            mb_h = tmp_arr[420 / 2] >> 16;
            mb_w = tmp_arr[420 / 2] & 0xffff;
            str_mb_h.write(mb_h);
            str_mb_w.write(mb_w);
        }
        Kernel2_read__array_to_str(tmp_arr, str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb);
        if (x_mb != (mb_w - 1))
            x_mb++;
        else {
            x_mb = 0;
            y_mb++;
        }
        num_mb++;
    } while (y_mb != (mb_h) || x_mb != 0);
}
//==================================kernel_2_ArithmeticCoding===========================================//
void Kernel2_read__array_to_str(uint32_t pin[256],
                                hls::stream<ap_int<WD_LEVEL * 16> >& str_level_dc,
                                hls::stream<ap_int<WD_LEVEL * 16> >& str_level_ac,
                                hls::stream<ap_int<WD_LEVEL * 16> >& str_level_uv,
                                hls::stream<ap_uint<64> >& str_pred,
                                hls::stream<ap_uint<6> >& str_ret,
                                hls::stream<ap_uint<1> >& str_type_mb) {
#pragma HLS PIPELINE
    uint32_t* plevel = pin;
    int x, y, ch;
    ap_int<WD_LEVEL* 16> tmp = SetVectFrom32bit(plevel);
    str_level_dc.write(tmp);
    plevel += 16 / 2;

    // luma-AC
    for (y = 0; y < 4; ++y) {
    READ_ARRAY_16:
        for (x = 0; x < 4; ++x) {
#pragma HLS PIPELINE
            ap_int<WD_LEVEL* 16> tmp = SetVectFrom32bit(plevel);
            str_level_ac.write(tmp);
            int16_t test[16];
            CPY16(test, tmp, WD_LEVEL);
            plevel += 16 / 2;
        }
    }

    // U/V
    for (ch = 0; ch <= 2; ch += 2) {
        for (y = 0; y < 2; ++y) {
            for (x = 0; x < 2; ++x) {
#pragma HLS PIPELINE
                ap_int<WD_LEVEL* 16> tmp = SetVectFrom32bit(plevel);
                str_level_uv.write(tmp);
                plevel += 16 / 2;
            }
        }
    }

    ap_uint<64> vct_pred = SetVect64From32bit(pin + 200);
    str_pred.write(vct_pred);
    ap_uint<6> ret = (ap_uint<6>)pin[416 / 2];
    str_ret.write(ret);
    str_type_mb.write(ret(4, 4));
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_int<WD_LEVEL * 16> SetVectFrom32bit(uint32_t* pin) {
#pragma HLS INLINE
    ap_int<WD_LEVEL * 16> ret;
    for (int i = 0; i < 8; i++) {
#pragma HLS PIPELINE
        ap_int<32> tmp32 = pin[i];
        ap_int<WD_LEVEL> tmp_l = tmp32(WD_LEVEL - 1, 0);
        ap_int<WD_LEVEL> tmp_h = tmp32(WD_LEVEL - 1 + 16, 16);
        ret((i * 2 + 1) * WD_LEVEL - 1, (i * 2 + 0) * WD_LEVEL) = tmp_l(WD_LEVEL - 1, 0);
        ret((i * 2 + 2) * WD_LEVEL - 1, (i * 2 + 1) * WD_LEVEL) = tmp_h(WD_LEVEL - 1, 0);
    }
    return ret;
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<4 * 16> SetVect64From32bit(uint32_t* pin) {
#pragma HLS INLINE
    ap_uint<4 * 16> ret;
    for (int i = 0; i < 8; i++) {
#pragma HLS PIPELINE
        ap_uint<32> tmp32 = pin[i];
        ap_uint<4> tmp_l = tmp32(4 - 1, 0);
        ap_uint<4> tmp_h = tmp32(4 - 1 + 16, 16);
        ret((i * 2 + 1) * 4 - 1, i * 2 * 4) = tmp_l(4 - 1, 0);
        ret((i * 2 + 2) * 4 - 1, (i * 2 + 1) * 4) = tmp_h(4 - 1, 0);
    }
    return ret;
}

static ap_int<5> FindLast(ap_int<WD_LEVEL * 16> level) {
#pragma HLS PIPELINE II = 1
    ap_int<5> ret = 15;
FIND_LAST:
    for (ret = 15; ret > -1; ret--) {
        ap_int<WD_LEVEL> tmp = VCT_GET(level, ret, WD_LEVEL);
        if (tmp != 0) return ret;
    }
    return ret;
}
static ap_uint<3> VP8EncBands_hls(ap_uint<5> n) {
/*const uint8_t VP8EncBands[16 + 1] = { 0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 0  };*/
#pragma HLS INLINE
    if (n < 4)
        return n;
    else if (n == 4)
        return 6;
    else if (n == 5)
        return 4;
    else if (n == 6)
        return 5;
    else if (n == 15)
        return 7;
    else if (n == 16)
        return 0;
    else
        return 6;
}
//==================================kernel_2_ArithmeticCoding===========================================//
void kernel_2_RecordTokens_pre(hls::stream<uint16_t>& str_mb_h,
                               hls::stream<uint16_t>& str_mb_w,
                               hls::stream<ap_uint<1> >& str_type_mb,
                               hls::stream<ap_int<WD_LEVEL * 16> >& str_level_dc,
                               hls::stream<ap_int<WD_LEVEL * 16> >& str_level_ac,
                               hls::stream<ap_int<WD_LEVEL * 16> >& str_level_uv,
                               hls::stream<ap_uint<64> >& str_0_dc,
                               hls::stream<ap_uint<64> >& str_1_dc,
                               hls::stream<ap_uint<64> >& str_2_dc,
                               hls::stream<ap_uint<64> >& str_3_dc,
                               hls::stream<ap_uint<64> >& str_0_ac,
                               hls::stream<ap_uint<64> >& str_1_ac,
                               hls::stream<ap_uint<64> >& str_2_ac,
                               hls::stream<ap_uint<64> >& str_3_ac,
                               hls::stream<ap_uint<64> >& str_0_uv,
                               hls::stream<ap_uint<64> >& str_1_uv,
                               hls::stream<ap_uint<64> >& str_2_uv,
                               hls::stream<ap_uint<64> >& str_3_uv,
                               hls::stream<uint16_t>& str_mb_h_out,
                               hls::stream<uint16_t>& str_mb_w_out,
                               hls::stream<ap_uint<1> >& str_type_mb_out) {
    static ap_NoneZero ap_nz;
    uint16_t mb_h = str_mb_h.read();
    uint16_t mb_w = str_mb_w.read();
    str_mb_h_out.write(mb_h);
    str_mb_w_out.write(mb_w);
RECORD_TOKENS_Y:
    for (uint16_t y_mb = 0; y_mb < mb_h; y_mb++)
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 68
    X:
        for (uint16_t x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 120
            RecordTokens_nrd2_mb_w(&ap_nz, x_mb, y_mb, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                                   str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv,
                                   str_1_uv, str_2_uv, str_3_uv, str_type_mb_out);
        }
}
//==================================kernel_2_ArithmeticCoding===========================================//
/////RecordTokens_nrd2_mb_w////////////////////////////////
void RecordTokens_nrd2_mb_w(ap_NoneZero* ap_nz,
                            int x_,
                            int y_,
                            hls::stream<ap_uint<1> >& str_type_mb,
                            hls::stream<ap_int<WD_LEVEL * 16> >& str_level_dc,
                            hls::stream<ap_int<WD_LEVEL * 16> >& str_level_ac,
                            hls::stream<ap_int<WD_LEVEL * 16> >& str_level_uv,
                            hls::stream<ap_uint<64> >& str_0_dc,
                            hls::stream<ap_uint<64> >& str_1_dc,
                            hls::stream<ap_uint<64> >& str_2_dc,
                            hls::stream<ap_uint<64> >& str_3_dc,
                            hls::stream<ap_uint<64> >& str_0_ac,
                            hls::stream<ap_uint<64> >& str_1_ac,
                            hls::stream<ap_uint<64> >& str_2_ac,
                            hls::stream<ap_uint<64> >& str_3_ac,
                            hls::stream<ap_uint<64> >& str_0_uv,
                            hls::stream<ap_uint<64> >& str_1_uv,
                            hls::stream<ap_uint<64> >& str_2_uv,
                            hls::stream<ap_uint<64> >& str_3_uv,
                            hls::stream<ap_uint<1> >& str_type_mb_out) {
    int x, y, ch;
    ap_uint<9> ap_top_nz = ap_nz->load_top9(x_, y_);
    ap_uint<9> ap_left_nz = ap_nz->load_left9(x_);
    ap_uint<9> top_nz_ = ap_top_nz;   //=ap_top_nz[i];f
    ap_uint<9> left_nz_ = ap_left_nz; //= ap_left_nz[i];

    ap_uint<1> type_ = str_type_mb.read();
    str_type_mb_out.write(type_);
    ap_int<WD_LEVEL* 16> c_hls = str_level_dc.read();
    if (type_ == 1) { // i16x16
        const int ctx = top_nz_[8] + left_nz_[8];
        int last = FindLast(c_hls);
        top_nz_[8] = left_nz_[8] = last < 0 ? 0 : 1;
        VP8RecordCoeffTokens_hls_w(ctx, 1, last, c_hls, str_0_dc, str_1_dc, str_2_dc, str_3_dc);
    }

// luma-AC
VP8_RECORD_COEFF_TOKENS_W_LUMA:
    for (y = 0; y < 4; ++y) {
    LUMA_X:
        for (x = 0; x < 4; ++x) {
            const int ctx = top_nz_[x] + left_nz_[y];
            ap_int<WD_LEVEL* 16> c_hls = str_level_ac.read();
            int16_t test[16];
            CPY16(test, c_hls, WD_LEVEL);
            int last = FindLast(c_hls);
            int coeff_type = type_ == 1 ? 0 : 3;
            top_nz_[x] = left_nz_[y] = last < 0 ? 0 : 1;
            VP8RecordCoeffTokens_hls_w(ctx, coeff_type, last, c_hls, str_0_ac, str_1_ac, str_2_ac, str_3_ac);
        }
    }

// U/V
VP8_RECORD_COEFF_TOKENS_W_UV:
    for (ch = 0; ch <= 2; ch += 2) {
    UV_Y:
        for (y = 0; y < 2; ++y) {
        UV_X:
            for (x = 0; x < 2; ++x) {
                const int ctx = top_nz_[4 + ch + x] + left_nz_[4 + ch + y];
                ap_int<WD_LEVEL* 16> c_hls = str_level_uv.read();
                int last = FindLast(c_hls);
                top_nz_[4 + ch + x] = left_nz_[4 + ch + y] = last < 0 ? 0 : 1;
                VP8RecordCoeffTokens_hls_w(ctx, 2, last, c_hls, str_0_uv, str_1_uv, str_2_uv, str_3_uv);
            }
        }
    }

    uint32_t nz = 0;
    nz |= (ap_uint<25>)((top_nz_[0] << 12) | (top_nz_[1] << 13));
    nz |= (ap_uint<25>)((top_nz_[2] << 14) | (top_nz_[3] << 15));
    nz |= (ap_uint<25>)((top_nz_[4] << 18) | (top_nz_[5] << 19));
    nz |= (ap_uint<25>)((top_nz_[6] << 22) | (top_nz_[7] << 23));
    nz |= (ap_uint<25>)((top_nz_[8] << 24)); // we propagate the _top_ bit, esp. for intra4
    // left
    nz |= (ap_uint<25>)((left_nz_[0] << 3) | (left_nz_[1] << 7));
    nz |= (ap_uint<25>)((left_nz_[2] << 11));
    nz |= (ap_uint<25>)((left_nz_[4] << 17) | (left_nz_[6] << 21));

    ap_nz->left_nz[8] = left_nz_[8];
    ap_nz->nz_current = nz; //*it->nz_;
    ap_nz->store_nz(x_);
}
//==================================kernel_2_ArithmeticCoding===========================================//
void VP8RecordCoeffTokens_hls_w(ap_uint<2> ctx,
                                ap_uint<2> coeff_type,
                                ap_int<5> last,
                                ap_int<WD_LEVEL * 16> coeffs,
                                hls::stream<ap_uint<64> >& str_0,
                                hls::stream<ap_uint<64> >& str_1,
                                hls::stream<ap_uint<64> >& str_2,
                                hls::stream<ap_uint<64> >& str_3) {
    TokensStr0_hls(ctx, coeff_type, last, str_0);
    ap_uint<11> base_id_last = TOKEN_ID2((ap_uint<11>)coeff_type, (coeff_type == 0 ? 1 : 0)) +
                               ctx * 11; // TOKEN_ID0(coeff_type, coeff_type==0, ctx);
    ap_uint<11> base_id = base_id_last;
TOKEN_ID2:
    for (int i = 0; i <= last; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 16
#pragma HLS PIPELINE
        if (i == 0 && coeff_type == 0) // first==1)
            continue;
        ap_int<WD_LEVEL> c = (ap_int<WD_LEVEL>)VCT_GET(coeffs, i, WD_LEVEL); // coeffs[i];
        ap_uint<1> sign = c < 0;
        ap_uint<WD_LEVEL> v;
        if (c < 0)
            v = (-c);
        else
            v = c;
        // str_1-----------------------------
        // sign
        ap_uint<1> isV_N0 = v != 0;
        ap_uint<1> isLastBEi = i < last;
        ap_uint<1> isV_B1 = v > 1;
        // str_2-------------------------------------------
        ap_uint<1> isV_B4 = v > 4;
        ap_uint<1> isV_N2 = v != 2;
        ap_uint<1> isV_4 = v == 4;
        ap_uint<1> isV_B10 = v > 10;
        // str_3-------------------------------------------
        ap_uint<1> isV_B6 = v > 6;
        ap_uint<1> isV_6 = v == 6;
        ap_uint<1> isV_BE9 = v >= 9;
        ap_uint<1> isV_even = 1 - v & 1; //!(v & 1)
        //-------------------------------------------------

        ap_uint<11> base_id_next;
        const uint8_t VP8EncBands[16 + 1] = {0, 1, 2, 3, 6, 4, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 0};
        uint8_t VP8EncBands_ = VP8EncBands_hls(i + 1);
        if (v == 0)
            base_id_next = TOKEN_ID2((ap_uint<11>)coeff_type, VP8EncBands_); // VP8EncBands[i + 1]);//
        else if (v == 1)
            base_id_next = TOKEN_ID2((ap_uint<11>)coeff_type, VP8EncBands_) + 11;
        else
            base_id_next = TOKEN_ID2((ap_uint<11>)coeff_type, VP8EncBands_) + 22;

        TokensStr1_hls(isV_N0, isV_B1, sign, isLastBEi, base_id, base_id_next, v, str_1);
        TokensStr2_hls(isV_B4, isV_N2, isV_4, isV_B10, base_id, str_2);
        TokensStr3_hls(isV_B6, isV_6, isV_BE9, isV_even, base_id, str_3);
        base_id = base_id_next;
    }
}
//==================================kernel_2_ArithmeticCoding===========================================//

//==================================kernel_2_ArithmeticCoding===========================================//
void PackToken_hls(ap_uint<64>& w, ap_uint<2> be, uint32_t bit, uint32_t proba_idx) {
    ap_uint<16> tmp = (bit << 15) | proba_idx;
    w(be * 16 + 15, be * 16) = tmp(15, 0);
}
/////PackConstantToken_hls////////////////
void PackConstantToken_hls(ap_uint<64>& w, ap_uint<2> be, uint32_t bit, uint32_t proba_idx) {
    ap_uint<16> tmp = (bit << 15) | (1u << 14) | proba_idx;
    w(be * 16 + 15, be * 16) = tmp(15, 0);
}
/////TokensStr0_hls////////////////////////
void TokensStr0_hls(ap_uint<2> ctx, ap_uint<2> coeff_type, ap_int<5> last, hls::stream<ap_uint<64> >& str_0) {
#pragma HLS PIPELINE II = 1
    ap_uint<64> w = 0;
    ap_uint<1> isLastN = last < 0;
    ap_uint<11> base_id_last = TOKEN_ID2((ap_uint<11>)coeff_type, (coeff_type == 0 ? 1 : 0)) + ctx * 11;
    w(16 + 4, 16) = last;
    w(16 + 8 + 2, 16 + 8) = coeff_type;
    PackToken_hls(w, 0, isLastN, base_id_last);
    str_0.write(w);
}
/////TokensStr1_hls//////////////////////////////////////

void TokensStr1_hls(ap_uint<1> isV_N0, // = v!=0,
                    ap_uint<1> isV_B1, // = v>1
                    ap_uint<1> sign,
                    ap_uint<1> isLastBEi, // = i<last,
                    ap_uint<11> base_id,
                    ap_uint<11> base_id_next,
                    ap_uint<11> v,
                    hls::stream<ap_uint<64> >& str_1) {
#pragma HLS PIPELINE II = 1
    ap_uint<64> w = 0;
    PackToken_hls(w, 0, isV_N0, base_id + 1);
    PackToken_hls(w, 1, isV_B1, base_id + 2);
    PackConstantToken_hls(w, 2, sign, v);
    PackToken_hls(w, 3, isLastBEi, base_id_next);
    str_1.write(w);
}
/////TokensStr2_hls/////////////////////////////////////////
void TokensStr2_hls(ap_uint<1> isV_B4,  // = v>4;
                    ap_uint<1> isV_N2,  // = v!=2;
                    ap_uint<1> isV_4,   // = v==4;
                    ap_uint<1> isV_B10, // = v>10;
                    ap_uint<11> base_id,
                    hls::stream<ap_uint<64> >& str_2) {
#pragma HLS PIPELINE II = 1
    ap_uint<64> w = 0;
    PackToken_hls(w, 0, isV_B4, base_id + 3);
    PackToken_hls(w, 1, isV_N2, base_id + 4);
    PackToken_hls(w, 2, isV_4, base_id + 5);
    PackToken_hls(w, 3, isV_B10, base_id + 6);
    str_2.write(w);
}
/////TokensStr3_hls//////////////////////////////////////
void TokensStr3_hls(ap_uint<1> isV_B6,   // = v>6;
                    ap_uint<1> isV_6,    // = v==6;
                    ap_uint<1> isV_BE9,  // = v>=9;
                    ap_uint<1> isV_even, // = 1-v&1;//!(v & 1)
                    ap_uint<11> base_id,
                    hls::stream<ap_uint<64> >& str_3) {
#pragma HLS PIPELINE II = 1
    ap_uint<64> w = 0;
    PackToken_hls(w, 0, isV_B6, base_id + 7);
    PackToken_hls(w, 1, isV_6, 159);
    PackToken_hls(w, 2, isV_BE9, 165);
    PackToken_hls(w, 3, isV_even, 145);
    str_3.write(w);
}
//==================================kernel_2_ArithmeticCoding===========================================//

//==================================kernel_2_ArithmeticCoding===========================================//
void kernel_2_CreateTokens_with_isFinal(hls::stream<uint16_t>& str_mb_h,
                                        hls::stream<uint16_t>& str_mb_w,
                                        hls::stream<ap_uint<1> >& str_type_mb,
                                        hls::stream<ap_uint<64> >& str_0_dc,
                                        hls::stream<ap_uint<64> >& str_1_dc,
                                        hls::stream<ap_uint<64> >& str_2_dc,
                                        hls::stream<ap_uint<64> >& str_3_dc,
                                        hls::stream<ap_uint<64> >& str_0_ac,
                                        hls::stream<ap_uint<64> >& str_1_ac,
                                        hls::stream<ap_uint<64> >& str_2_ac,
                                        hls::stream<ap_uint<64> >& str_3_ac,
                                        hls::stream<ap_uint<64> >& str_0_uv,
                                        hls::stream<ap_uint<64> >& str_1_uv,
                                        hls::stream<ap_uint<64> >& str_2_uv,
                                        hls::stream<ap_uint<64> >& str_3_uv,
                                        hls::stream<uint16_t>& str_mb_h_out,
                                        hls::stream<uint16_t>& str_mb_w_out,
                                        hls::stream<ap_uint<16> >& str_tokens_final) {
    uint16_t mb_h = str_mb_h.read();
    uint16_t mb_w = str_mb_w.read();
    str_mb_h_out.write(mb_h);
    str_mb_w_out.write(mb_w);
RECORD_TOKENS_ADD_FINAL:
    for (uint16_t y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 68
        for (uint16_t x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 16 max = 120
            RecordTokens_nrd2_mb_r_str_AddFinal(str_type_mb, str_0_dc, str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac,
                                                str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv, str_3_uv,
                                                str_tokens_final,
                                                y_mb == (mb_h - 1) && (x_mb == (mb_w - 1))); //&tokens);
        }
    }
}
//==================================kernel_2_ArithmeticCoding===========================================//
void RecordTokens_nrd2_mb_r_str_AddFinal(hls::stream<ap_uint<1> >& str_type_mb,
                                         hls::stream<ap_uint<64> >& str_0_dc,
                                         hls::stream<ap_uint<64> >& str_1_dc,
                                         hls::stream<ap_uint<64> >& str_2_dc,
                                         hls::stream<ap_uint<64> >& str_3_dc,
                                         hls::stream<ap_uint<64> >& str_0_ac,
                                         hls::stream<ap_uint<64> >& str_1_ac,
                                         hls::stream<ap_uint<64> >& str_2_ac,
                                         hls::stream<ap_uint<64> >& str_3_ac,
                                         hls::stream<ap_uint<64> >& str_0_uv,
                                         hls::stream<ap_uint<64> >& str_1_uv,
                                         hls::stream<ap_uint<64> >& str_2_uv,
                                         hls::stream<ap_uint<64> >& str_3_uv,
                                         hls::stream<ap_uint<16> >& tokens,
                                         bool isFinal) {
    int x, y, ch;
    ap_uint<1> type_mb = str_type_mb.read();
    //#pragma HLS DATAFLOW
    if (type_mb == 1) {
        VP8RecordCoeffTokens_hls_r_str_AddFanel(str_0_dc, str_1_dc, str_2_dc, str_3_dc, tokens, false);
    }
    for (y = 0; y < 4; ++y)
    ADD_FINAL_4X4:
        for (x = 0; x < 4; ++x) {
            VP8RecordCoeffTokens_hls_r_str_AddFanel(str_0_ac, str_1_ac, str_2_ac, str_3_ac, tokens, false);
        }
    for (ch = 0; ch <= 2; ch += 2)
        for (y = 0; y < 2; ++y)
        ADD_FINAL_2X2X2:
            for (x = 0; x < 2; ++x) {
                VP8RecordCoeffTokens_hls_r_str_AddFanel(str_0_uv, str_1_uv, str_2_uv, str_3_uv, tokens,
                                                        isFinal && (ch == 2) && (y == 1) && (x == 1));
            }
}
//==================================kernel_2_ArithmeticCoding===========================================//
////////VP8RecordCoeffTokens_hls_r_str_AddFanel///////////////////////////////
#define GET_isLastN_B(w) (w(15 + 0 * 16, 15 + 0 * 16))
#define GET_isLastN_P(w) (w(10 + 0 * 16, 0 + 0 * 16))
#define GET_last(w) ((ap_int<5>)w(16 + 4, 16))
#define GET_coeff_type(w) (w(16 + 8 + 2, 16 + 8))
#define GET_isV_N0_B(w) (w(15 + 0 * 16, 15 + 0 * 16))
#define GET_isV_N0_P(w) (w(10 + 0 * 16, 0 + 0 * 16))
#define GET_isV_B1_B(w) (w(15 + 1 * 16, 15 + 1 * 16))
#define GET_isV_B1_P(w) (w(10 + 1 * 16, 0 + 1 * 16))
#define GET_sign_B(w) (w(15 + 2 * 16, 15 + 2 * 16))
#define GET_sign_v(w) (w(10 + 2 * 16, 0 + 2 * 16))
#define GET_isLastBEi_B(w) (w(15 + 3 * 16, 15 + 3 * 16))
#define GET_isLastBEi_P(w) (w(10 + 3 * 16, 0 + 3 * 16))
#define GET_isV_B4_B(w) (w(15 + 0 * 16, 15 + 0 * 16))
#define GET_isV_B4_P(w) (w(10 + 0 * 16, 0 + 0 * 16))
#define GET_isV_N2_B(w) (w(15 + 1 * 16, 15 + 1 * 16))
#define GET_isV_N2_P(w) (w(10 + 1 * 16, 0 + 1 * 16))
#define GET_isV_4_B(w) (w(15 + 2 * 16, 15 + 2 * 16))
#define GET_isV_4_P(w) (w(10 + 2 * 16, 0 + 2 * 16))
#define GET_isV_B10_B(w) (w(15 + 3 * 16, 15 + 3 * 16))
#define GET_isV_B10_P(w) (w(10 + 3 * 16, 0 + 3 * 16))
#define GET_isV_B6_B(w) (w(15 + 0 * 16, 15 + 0 * 16))
#define GET_isV_B6_P(w) (w(10 + 0 * 16, 0 + 0 * 16))
#define GET_isV_6_B(w) (w(15 + 1 * 16, 15 + 1 * 16))
#define GET_isV_6_P(w) (w(10 + 1 * 16, 0 + 1 * 16))
#define GET_isV_BE9_B(w) (w(15 + 2 * 16, 15 + 2 * 16))
#define GET_isV_BE9_P(w) (w(10 + 2 * 16, 0 + 2 * 16))
#define GET_isV_even_B(w) (w(15 + 3 * 16, 15 + 3 * 16))
#define GET_isV_even_P(w) (w(10 + 3 * 16, 0 + 3 * 16))
int VP8RecordCoeffTokens_hls_r_str_AddFanel(hls::stream<ap_uint<64> >& str_0,
                                            hls::stream<ap_uint<64> >& str_1,
                                            hls::stream<ap_uint<64> >& str_2,
                                            hls::stream<ap_uint<64> >& str_3,
                                            hls::stream<ap_uint<16> >& tokens,
                                            bool isFinal) {
    ap_uint<64> w0 = str_0.read();
    ap_uint<1> b_w0 = GET_isLastN_B(w0);
    ap_uint<11> p_w0 = GET_isLastN_P(w0);
    ap_uint<5> last_w0 = GET_last(w0);
    ap_uint<5> type_w0 = GET_coeff_type(w0);
    ap_uint<11> base_id = p_w0;

    if (!AddToken_hls_AddFanel(tokens, !b_w0, p_w0, isFinal & b_w0)) { // last==-1
        return 0;
    }

    for (int i = 0; i <= last_w0; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 16
#pragma HLS PIPELINE off
        if (i == 0 && type_w0 == 0) // first==1)
            continue;
        ap_uint<64> w1 = str_1.read();
        ap_uint<64> w2 = str_2.read();
        ap_uint<64> w3 = str_3.read();
        ap_uint<1> sign_b = GET_sign_B(w1);            // c < 0;
        ap_uint<WD_LEVEL - 1> v = GET_sign_v(w1);      // c < 0;w4s(WD_LEVEL-2,0) ;
        ap_uint<1> isV_N0 = GET_isV_N0_B(w1);          // v!=0;//
        ap_uint<1> isLastBEi = GET_isLastBEi_B(w1);    //  i<last_w0;//;i<last;//
        ap_uint<1> isV_B1 = GET_isV_B1_B(w1);          // v>1;//
        ap_uint<11> isV_N0_p = GET_isV_N0_P(w1);       // v!=0;//
        ap_uint<11> isLastBEi_p = GET_isLastBEi_P(w1); //  i<last_w0;//;i<last;//
        ap_uint<11> isV_B1_p = GET_isV_B1_P(w1);       // v>1;//
        // str_2-------------------------------------------
        ap_uint<1> isV_B4 = GET_isV_B4_B(w2);      // v>4;
        ap_uint<1> isV_N2 = GET_isV_N2_B(w2);      // v!=2;
        ap_uint<1> isV_4 = GET_isV_4_B(w2);        // v==4;
        ap_uint<1> isV_B10 = GET_isV_B10_B(w2);    // v>10;
        ap_uint<11> isV_B4_p = GET_isV_B4_P(w2);   // v>4;
        ap_uint<11> isV_N2_p = GET_isV_N2_P(w2);   // v!=2;
        ap_uint<11> isV_4_p = GET_isV_4_P(w2);     // v==4;
        ap_uint<11> isV_B10_p = GET_isV_B10_P(w2); // v>10;
        // str_3-------------------------------------------
        ap_uint<1> isV_B6 = GET_isV_B6_B(w3);        // v>6;
        ap_uint<1> isV_6 = GET_isV_6_B(w3);          // v==6;
        ap_uint<1> isV_BE9 = GET_isV_BE9_B(w3);      // v>=9;
        ap_uint<1> isV_even = GET_isV_even_B(w3);    // 1-v&1;//!(v & 1)
        ap_uint<11> isV_B6_p = GET_isV_B6_P(w3);     // v>6;
        ap_uint<11> isV_6_p = GET_isV_6_P(w3);       // v==6;
        ap_uint<11> isV_BE9_p = GET_isV_BE9_P(w3);   // v>=9;
        ap_uint<11> isV_even_p = GET_isV_even_P(w3); // 1-v&1;//!(v & 1)
        //-------------------------------------------------
        ap_uint<1> isV_S19 = v < 19; // residue < (8 << 1)
        ap_uint<1> isV_S35 = v < 35; // residue < (8 << 2)
        ap_uint<1> isV_S67 = v < 67; // residue < (8 << 2)

        AddToken_hls_AddFanel(tokens, isV_N0, isV_N0_p, 0);
        base_id = isV_N0_p - 1;
        if (v != 0) {
            if (AddToken_hls_AddFanel(tokens, isV_B1, isV_B1_p, 0)) {       // v=[2,2047]
                if (!AddToken_hls_AddFanel(tokens, isV_B4, isV_B4_p, 0)) {  // v=[2,4]
                    if (AddToken_hls_AddFanel(tokens, isV_N2, isV_N2_p, 0)) // v=[3,4]
                        AddToken_hls_AddFanel(tokens, isV_4, isV_4_p, 0);   // v=[4,4]
                } else if (!AddToken_hls_AddFanel(tokens, isV_B10, isV_B10_p,
                                                  0)) { // base_id + 6)) {//v=[5,10]//GET__B, GET__P
                    if (!AddToken_hls_AddFanel(tokens, isV_B6, isV_B6_p,
                                               0)) { // base_id + 7)) {//v=[5,6]//GET__B, GET__P
                        AddConstantToken_hls_AddFanel(tokens, isV_6, 159, 0);    // v=[6]//GET__B, GET__P
                    } else {                                                     // v=[7,10]
                        AddConstantToken_hls_AddFanel(tokens, isV_BE9, 165, 0);  // v=[9,10]//GET__B, GET__P
                        AddConstantToken_hls_AddFanel(tokens, isV_even, 145, 0); // v=[8,10]//GET__B, GET__P
                    }
                } else { // v=[11~2047]
                    const uint8_t* tab;
                    uint16_t residue = v - 3; //[8~2044]
                    if (isV_S19) {            //[8 15]        // VP8Cat3  (3b)
                        AddToken_hls_AddFanel(tokens, 0, base_id + 8, 0);
                        AddToken_hls_AddFanel(tokens, 0, base_id + 9, 0);
                        residue -= (8 << 0);
                        const uint8_t VP8Cat3[] = {173, 148, 140};
                        tab = VP8Cat3;
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 11 & 4), 173, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 11 & 2), 148, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 11 & 1), 140, 0);
                    } else if (isV_S35) { //[16,31]// VP8Cat4  (4b)
                        AddToken_hls_AddFanel(tokens, 0, base_id + 8, 0);
                        AddToken_hls_AddFanel(tokens, 1, base_id + 9, 0);
                        residue -= (8 << 1);
                        const uint8_t VP8Cat4[] = {176, 155, 140, 135};
                        tab = VP8Cat4;
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 19 & 8), 176, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 19 & 4), 155, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 19 & 2), 140, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 19 & 1), 135, 0);
                    } else if (isV_S67) { // [32,63] VP8Cat5  (5b)
                        AddToken_hls_AddFanel(tokens, 1, base_id + 8, 0);
                        AddToken_hls_AddFanel(tokens, 0, base_id + 10, 0);
                        residue -= (8 << 2);
                        const uint8_t VP8Cat5[] = {180, 157, 141, 134, 130};
                        tab = VP8Cat5;
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 35 & 16), 180, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 35 & 8), 157, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 35 & 4), 141, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 35 & 2), 134, 0);
                        AddConstantToken_hls_AddFanel(tokens, !!(v - 35 & 1), 130, 0);
                    } else { // [64,2048)VP8Cat6 (11b)
                        AddToken_hls_AddFanel(tokens, 1, base_id + 8, 0);
                        AddToken_hls_AddFanel(tokens, 1, base_id + 10, 0);
                        residue -= (8 << 3);
                        const uint8_t VP8Cat6[] = {254, 254, 243, 230, 196, 177, 153, 140, 133, 130, 129};
                        tab = VP8Cat6;
                    ADD_CONSTANT_TOKEN:
                        for (int k = 10; k >= 0; k--)
                            AddConstantToken_hls_AddFanel(tokens, !!(v - 67 & (1 << k)), *tab++, 0);
                    } //[64,2048)
                }     // v=[11~2047]
            }         // v=[2~2047]
        }             // v!=0
        if (v == 0) continue;
        AddConstantToken_hls_AddFanel(tokens, sign_b, 128, isFinal & (15 == i));
        if (i == 15 || !AddToken_hls_AddFanel(tokens, isLastBEi, isLastBEi_p, isFinal && (!isLastBEi))) {
            return 1; // EOB
        }
    } // for
    return 1;
}

//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<1> AddConstantToken_hls_AddFanel(hls::stream<ap_uint<16> >& str_tokens,
                                         ap_uint<1> bit,
                                         ap_uint<11> proba_idx,
                                         ap_uint<1> isFinal) {
#pragma HLS PIPELINE
    ap_uint<16> tmp;
    tmp[15] = bit;
    tmp[14] = 1;
    tmp[13] = 0;
    tmp[12] = isFinal;
    tmp[11] = 0;
    tmp(10, 0) = proba_idx;
    str_tokens.write(tmp);
    return bit;
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<1> AddToken_hls_AddFanel(hls::stream<ap_uint<16> >& str_tokens,
                                 ap_uint<1> bit,
                                 ap_uint<11> proba_idx,
                                 ap_uint<1> isFinal) {
#pragma HLS PIPELINE
    ap_uint<16> tmp;
    tmp[15] = bit;
    tmp[14] = 0;
    tmp[13] = 0;
    tmp[12] = isFinal;
    tmp[11] = 0;
    tmp(10, 0) = proba_idx;
    str_tokens.write(tmp);
    return bit;
}
//==================================kernel_2_ArithmeticCoding===========================================//
//==================================kernel_2_ArithmeticCoding===========================================//

void VP8EmitTokens_str_hls_4stages(uint32_t pout_bw[SIZE32_MEM_BW],
                                   hls::stream<ap_uint<16> >& str_token,
                                   uint8_t probas[4 * 8 * 3 * 11]) {
    hls::stream<ap_uint<18> > str_Last_isBit_Bits;
#pragma HLS STREAM variable = str_Last_isBit_Bits depth = 64
    ap_uint<8> bw_range;  // = 254;      // range-1
    ap_uint<24> bw_value; // = 0;
    ap_int<4> bw_nb_bits; // = -8;
    ap_uint<32> bw_pos;   // = 0;
    ap_uint<16> bw_run;   // = 0;

    VP8EmitTokens_allstr_hls_dataflow_4stages(pout_bw, str_token, probas, bw_range, bw_value, bw_nb_bits, bw_pos,
                                              bw_run);

    uint32_t* p_bw = pout_bw + SIZE32_MEM_BW - SIZE32_AC_STATE;
    p_bw[0] = bw_range;
    p_bw[1] = bw_value;
    p_bw[2] = bw_nb_bits;
    p_bw[3] = bw_pos;
    p_bw[4] = bw_run;
    p_bw[5] = MAX_NUM_MB_W * MAX_NUM_MB_H * 384 / SYSTEM_MIN_COMP_RATIO - 1; // max_pos
    p_bw[6] = 0;                                                             // error
    p_bw[7] = 0;                                                             // index_ac_encoder / num_segment
}

//==================================kernel_2_ArithmeticCoding===========================================//
void VP8EmitTokens_allstr_hls_dataflow_4stages(uint32_t pout_bw[SIZE32_MEM_BW],
                                               hls::stream<ap_uint<16> >& str_token,
                                               uint8_t probas[4 * 8 * 3 * 11],
                                               ap_uint<8>& bw_range,  // = 254;      // range-1
                                               ap_uint<24>& bw_value, //= 0;
                                               ap_int<4>& bw_nb_bits, // = -8;
                                               ap_uint<32>& bw_pos,   //= 0
                                               ap_uint<16>& bw_run)   // = 0,
{
#pragma HLS DATAFLOW
    // range loop (a loop)
    hls::stream<ap_uint<2 + 3 + 8> > str_fnl_bit_shift_split_1;
#pragma HLS STREAM variable = str_fnl_bit_shift_split_1 depth = 64
    bw_range = hls_AC_range_str(str_token, probas, str_fnl_bit_shift_split_1);

    // Value loop (c loop)
    hls::stream<ap_uint<18> > str_Last_isBit_Bits;
#pragma HLS STREAM variable = str_Last_isBit_Bits depth = 64
    ap_uint<4 + 24> nb_value = hls_AC_value_str(str_fnl_bit_shift_split_1, str_Last_isBit_Bits);
    bw_nb_bits = nb_value(27, 24);
    bw_value = nb_value(23, 0);

    // Package loop-1
    hls::stream<ap_uint<26> > str_isFinal_run_cy_pre;
#pragma HLS STREAM variable = str_isFinal_run_cy_pre depth = 1024
    ap_uint<16> run = VP8PutBit_hls_BytePackage_str_run(str_Last_isBit_Bits, str_isFinal_run_cy_pre);

    // Package loop-2
    hls::stream<ap_uint<9> > str_Last_byte;
#pragma HLS STREAM variable = str_Last_byte depth = 1024
    ap_uint<32> pos = VP8PutBit_hls_BytePackage_str_pos(str_isFinal_run_cy_pre, str_Last_byte);

    bw_run = run(15, 0);
    bw_pos = pos(31, 0);
    PackStr2Mem_t<9, 8, 256>(pout_bw, str_Last_byte);
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<8> hls_AC_range_str(hls::stream<ap_uint<16> >& str_token,
                            uint8_t probas[4 * 8 * 3 * 11],
                            hls::stream<ap_uint<2 + 3 + 8> >& str_fnl_bit_shift_split_1) {
    ap_uint<8> range_old = 254;
    ap_uint<8> split_1;
    ap_uint<3> shift;
    ap_uint<2 + 3 + 8> tmp;
    ap_uint<1> isFinal = 0;
AC_RANGE_STR:
    do {
#pragma HLS LOOP_TRIPCOUNT min = 1920 * 1088 / 256 * 384 * 2 max = 4096 * 4096 / 256 * 384 * 2
#pragma HLS PIPELINE II = 1
        ap_uint<16> token = str_token.read(); //[i];
        isFinal = token[12];
        ap_uint<1> bit = token[15];
        ap_uint<1> isFix = token[14];
        ap_uint<8> p;
        if (isFix)
            p = token(7, 0);
        else
            p = probas[token(10, 0)];
        ap_uint<8> tmp_p = (range_old * p) >> 8;
        split_1 = tmp_p + 1;

        ap_uint<8> range_new;
        ap_uint<8> range_nor1 = range_old - tmp_p;
        ap_uint<8> range_nor2 = tmp_p + 1;

        if (bit) {
            shift = range_nor1.countLeadingZeros();
            range_new = (range_nor1 << range_nor1.countLeadingZeros()) - 1;
        } else {
            shift = range_nor2.countLeadingZeros();
            range_new = (range_nor2 << range_nor2.countLeadingZeros()) - 1;
        }
        tmp[12] = isFinal;
        tmp[11] = bit;
        tmp(10, 8) = shift;
        tmp(7, 0) = split_1;
        str_fnl_bit_shift_split_1.write(tmp);
        range_old = range_new;
    } while (isFinal == 0);
    return range_old;
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<4 + 24> hls_AC_value_str(hls::stream<ap_uint<2 + 3 + 8> >& str_fnl_bit_shift_split_1,
                                 hls::stream<ap_uint<18> >& str_fnl_isBit_Bits) {
#pragma HLS INLINE OFF
    ap_uint<24> v_old = 0;
    ap_int<4> nb_old = -8;

    ap_uint<1> isFinal = 0;
    ap_uint<1> bit;
    ap_uint<3> shift;
    ap_uint<8> split_1;

    ap_uint<16> bits;
    ap_uint<1> isBits;

AC_VALUE_STR:
    do {
#pragma HLS LOOP_TRIPCOUNT min = 1920 * 1088 / 256 * 384 * 2 max = 4096 * 4096 / 256 * 384 * 2
#pragma HLS PIPELINE II = 1
        ap_uint<2 + 3 + 8> fnl_bit_shift_split_1 = str_fnl_bit_shift_split_1.read();
        isFinal = fnl_bit_shift_split_1[12];
        bit = fnl_bit_shift_split_1[11];
        shift = fnl_bit_shift_split_1(10, 8);
        split_1 = fnl_bit_shift_split_1(7, 0);
        isBits = 0;
        ap_uint<24> v_new = v_old; //
        ap_int<4> nb_new = nb_old; //
        if (bit)
            // v_old += split_1;
            v_new += split_1;
        v_new <<= shift;
        nb_new += shift;
        if (nb_new > 0) {
            isBits = 1;
            ap_uint<4> s = 8 + nb_new;
            bits = v_new(23, s);
            v_new(23, s) = 0; // v_old -= bits << s;
            nb_new -= 8;
        }
        ap_uint<18> Last_isBit_Bits;
        Last_isBit_Bits(17, 17) = isFinal;
        Last_isBit_Bits(16, 16) = isBits;
        Last_isBit_Bits(15, 0) = bits;
        if (isBits || isFinal) str_fnl_isBit_Bits.write(Last_isBit_Bits);
        v_old = v_new;
        nb_old = nb_new;
    } while (isFinal == 0);
    ap_uint<4 + 24> ret;
    ret(27, 24) = nb_old;
    ret(23, 0) = v_old;
    return ret;
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<16> VP8PutBit_hls_BytePackage_str_run(hls::stream<ap_uint<18> >& str_Last_isBit_Bits,
                                              hls::stream<ap_uint<26> >& str_isFinal_run_cy_pre) {
    // hls::stream<ap_uint<9+16> > str_isFinal_run_cy_pre;
    ap_uint<26> isFinal_run_cy_pre; // 1+16+1+8
    ap_uint<16> p_run_ = 0;
    ap_uint<8> byte_pre = 0xff; // 0xff is the initial value that means byte_pre is never used
    ap_uint<1> isLast;
BYTE_PACKAGE:
    do { /*This loop iterates p_run_ and byte_pre*/
#pragma HLS PIPELINE
        ap_uint<18> Last_isBit_Bits = str_Last_isBit_Bits.read();
        isLast = Last_isBit_Bits(17, 17);
        ap_uint<1> isBits = Last_isBit_Bits(16, 16);
        ap_uint<16> bits = Last_isBit_Bits(15, 0);

        if (isBits) {
            if (byte_pre == 0xff) {
                byte_pre(7, 0) = bits(7, 0);
            } else if ((bits & 0xff) != 0xff) {
                isFinal_run_cy_pre(7, 0) = byte_pre(7, 0);
                isFinal_run_cy_pre(8, 8) = bits(8, 8);
                isFinal_run_cy_pre(16 + 8, 9) = p_run_;
                isFinal_run_cy_pre[25] = 0;
                p_run_ = 0;
                byte_pre(7, 0) = bits(7, 0);
                str_isFinal_run_cy_pre.write(isFinal_run_cy_pre);
            } else {
                p_run_++;
            }
        }
    } while (isLast == 0);

    if (isLast && byte_pre != 0xff) {
        isFinal_run_cy_pre(7, 0) = byte_pre(7, 0);
        isFinal_run_cy_pre(8, 8) = 0;      // cy
        isFinal_run_cy_pre(16 + 8, 9) = 0; // run
        isFinal_run_cy_pre[25] = 1;        // Final
        str_isFinal_run_cy_pre.write(isFinal_run_cy_pre);
    }

    return p_run_;
}
//==================================kernel_2_ArithmeticCoding===========================================//
ap_uint<32> VP8PutBit_hls_BytePackage_str_pos(hls::stream<ap_uint<26> >& str_isFinal_run_cy_pre,
                                              hls::stream<ap_uint<9> >& str_Last_byte) {
    ap_uint<32> p_pos_ = 0;
    ap_uint<1> isLast;
DO_BYTE_PACKAGE_POS:
    do {
        ap_uint<1 + 16 + 9> isFinal_run_cy_pre = str_isFinal_run_cy_pre.read();
        isLast = isFinal_run_cy_pre[25];
        ap_uint<16> run = isFinal_run_cy_pre(24, 9);
        ap_uint<1> cy = isFinal_run_cy_pre[8];
        ap_uint<9> byte = isFinal_run_cy_pre(7, 0) + cy;
        byte[8] = isLast;
        str_Last_byte.write(byte);
        p_pos_++;
        ap_uint<9> stuff;
        if (cy)
            stuff = 0;
        else
            stuff = 0x0ff;
    BYTE_PACKAGE_POS:
        for (int i = 0; i < run; i++) {
#pragma HLS PIPELINE
            str_Last_byte.write(stuff);
            p_pos_++;
        }
    } while (isLast == 0);

    return p_pos_;
}
//==================================kernel_2_ArithmeticCoding===========================================//
/*
 * //Other used for host convenience
 */
void set_vect_to(ap_uint<8 * 16> src, unsigned char* des, int strip) {
    ap_uint<8 * 16> sb;
SET_VECT_FUNC:
    for (int i = 0; i < 4; i++)
    SET_VECT_FUNC_IN:
        for (int j = 0; j < 4; j++) {
            des[j + strip * i] = SB_GET(src, i, j, 8);
        }
}
//////////////////////////////////////////////////////////////////////////////
extern "C" {
void kernel_2_ArithmeticCoding_1_5axi(uint32_t pin_level[SIZE32_MEM_BW],
                                      uint8_t* pin_prob, // 2048 instead of [4 * 8 * 3 * 11],
                                      uint32_t pout_bw[SIZE32_MEM_BW],
                                      uint32_t pout_ret[SIZE32_MEM_RET],
                                      uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem3 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem4 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    uint8_t prob[4 * 8 * 3 * 11];
    memcpy(prob, pin_prob, sizeof(prob));

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level,

                     str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h, str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    VP8EmitTokens_str_hls_4stages(pout_bw, tokens_str_final,
                                  (uint8_t*)prob); // VP8EmitTokens_hls(pout_bw, &tokens, (uint8_t*)prob);
    PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
}
}

extern "C" {
void kernel_2_ArithmeticCoding_2_5axi(uint32_t pin_level[SIZE32_MEM_BW],
                                      uint8_t* pin_prob, // 2048 instead of [4 * 8 * 3 * 11],
                                      uint32_t pout_bw[SIZE32_MEM_BW],
                                      uint32_t pout_ret[SIZE32_MEM_RET],
                                      uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem3 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem4 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    uint8_t prob[4 * 8 * 3 * 11];
    memcpy(prob, pin_prob, sizeof(prob));

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level,

                     str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h, str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    VP8EmitTokens_str_hls_4stages(pout_bw, tokens_str_final,
                                  (uint8_t*)prob); // VP8EmitTokens_hls(pout_bw, &tokens, (uint8_t*)prob);
    PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
}
}

extern "C" {
void kernel_2_ArithmeticCoding_3_5axi(uint32_t pin_level[SIZE32_MEM_BW],
                                      uint8_t* pin_prob, // 2048 instead of [4 * 8 * 3 * 11],
                                      uint32_t pout_bw[SIZE32_MEM_BW],
                                      uint32_t pout_ret[SIZE32_MEM_RET],
                                      uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem3 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem4 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    uint8_t prob[4 * 8 * 3 * 11];
    memcpy(prob, pin_prob, sizeof(prob));

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level,

                     str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h, str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    VP8EmitTokens_str_hls_4stages(pout_bw, tokens_str_final,
                                  (uint8_t*)prob); // VP8EmitTokens_hls(pout_bw, &tokens, (uint8_t*)prob);
    PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
}
}

extern "C" {
void kernel_2_ArithmeticCoding_4_5axi(uint32_t pin_level[SIZE32_MEM_BW],
                                      uint8_t* pin_prob, // 2048 instead of [4 * 8 * 3 * 11],
                                      uint32_t pout_bw[SIZE32_MEM_BW],
                                      uint32_t pout_ret[SIZE32_MEM_RET],
                                      uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem3 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem4 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    uint8_t prob[4 * 8 * 3 * 11];
    memcpy(prob, pin_prob, sizeof(prob));

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level,

                     str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h, str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    VP8EmitTokens_str_hls_4stages(pout_bw, tokens_str_final,
                                  (uint8_t*)prob); // VP8EmitTokens_hls(pout_bw, &tokens, (uint8_t*)prob);
    PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
}
}

/****************************************************/
// changes
// The old versions will be added with a sufix '_old'
// to disable them
/****************************************************/
ap_uint<8> hls_AC_range_str_32bits(hls::stream<ap_uint<16> >& str_token,
                                   // uint8_t probas[4 * 8 * 3 * 11],
                                   uint32_t* probas, //[1 * 8 * 3 * 11],
                                   hls::stream<ap_uint<2 + 3 + 8> >& str_fnl_bit_shift_split_1) {
    ap_uint<8> range_old = 254;
    ap_uint<8> split_1;
    ap_uint<3> shift;
    ap_uint<2 + 3 + 8> tmp;
    ap_uint<1> isFinal = 0;
AC_RANGE_STR32:
    do {
#pragma HLS LOOP_TRIPCOUNT min = 512 * 512 / 256 * 384 * 2 max = 4096 * 4096 / 256 * 384 * 2
#pragma HLS PIPELINE II = 1
        ap_uint<16> token = str_token.read();
        isFinal = token[12];
        ap_uint<1> bit = token[15];
        ap_uint<1> isFix = token[14];
        ap_uint<8> p;
        if (isFix)
            p = token(7, 0);
        else {
            // p = probas[token(10, 0)];
            ap_uint<32> tmp_p = probas[token(10, 2)];
            ap_uint<2> be = token(1, 0);
            p = tmp_p(be * 8 + 7, be * 8);
        }
        ap_uint<8> tmp_p = (range_old * p) >> 8;
        split_1 = tmp_p + 1;

        ap_uint<8> range_new;
        ap_uint<8> range_nor1 = range_old - tmp_p;
        ap_uint<8> range_nor2 = tmp_p + 1;

        if (bit) {
            shift = range_nor1.countLeadingZeros();
            range_new = (range_nor1 << range_nor1.countLeadingZeros()) - 1;
        } else {
            shift = range_nor2.countLeadingZeros();
            range_new = (range_nor2 << range_nor2.countLeadingZeros()) - 1;
        }
        tmp[12] = isFinal;
        tmp[11] = bit;
        tmp(10, 8) = shift;
        tmp(7, 0) = split_1;
        str_fnl_bit_shift_split_1.write(tmp);
        range_old = range_new;
    } while (isFinal == 0);
    return range_old;
}
void VP8EmitTokens_allstr_hls_dataflow_4stages_32bits(uint32_t* pout_bw, //[SIZE32_MEM_BW],
                                                      hls::stream<ap_uint<16> >& str_token,
                                                      uint32_t* probas, //[1 * 8 * 3 * 11],
                                                      ap_uint<8>& bw_range,
                                                      ap_uint<24>& bw_value,
                                                      ap_int<4>& bw_nb_bits,
                                                      ap_uint<32>& bw_pos,
                                                      ap_uint<16>& bw_run) {
#pragma HLS DATAFLOW
    // range loop (a loop)
    hls::stream<ap_uint<2 + 3 + 8> > str_fnl_bit_shift_split_1;
#pragma HLS STREAM variable = str_fnl_bit_shift_split_1 depth = 64
    bw_range = hls_AC_range_str_32bits(str_token, probas, str_fnl_bit_shift_split_1);

    // Value loop (c loop)
    hls::stream<ap_uint<18> > str_Last_isBit_Bits;
#pragma HLS STREAM variable = str_Last_isBit_Bits depth = 64
    ap_uint<4 + 24> nb_value = hls_AC_value_str(str_fnl_bit_shift_split_1, str_Last_isBit_Bits);
    bw_nb_bits = nb_value(27, 24);
    bw_value = nb_value(23, 0);

    // Package loop-1
    hls::stream<ap_uint<26> > str_isFinal_run_cy_pre;
#pragma HLS STREAM variable = str_isFinal_run_cy_pre depth = 1024
    ap_uint<16> run = VP8PutBit_hls_BytePackage_str_run(str_Last_isBit_Bits, str_isFinal_run_cy_pre);

    // Package loop-2
    hls::stream<ap_uint<9> > str_Last_byte;
#pragma HLS STREAM variable = str_Last_byte depth = 1024
    ap_uint<32> pos = VP8PutBit_hls_BytePackage_str_pos(str_isFinal_run_cy_pre, str_Last_byte);

    bw_run = run(15, 0);
    bw_pos = pos(31, 0);
PACK_STR2:
    PackStr2Mem_t<9, 8, 256>(pout_bw, str_Last_byte);
}
void VP8EmitTokens_str_hls_4stages_32bits(uint32_t* pout_bw, //[SIZE32_MEM_BW],
                                          hls::stream<ap_uint<16> >& str_token,
                                          uint32_t* probas) {
    hls::stream<ap_uint<18> > str_Last_isBit_Bits;
#pragma HLS STREAM variable = str_Last_isBit_Bits depth = 64
    ap_uint<8> bw_range;  // = 254;
    ap_uint<24> bw_value; // = 0;
    ap_int<4> bw_nb_bits; // = -8;
    ap_uint<32> bw_pos;   // = 0;
    ap_uint<16> bw_run;   // = 0;

    VP8EmitTokens_allstr_hls_dataflow_4stages_32bits(pout_bw, str_token, probas, bw_range, bw_value, bw_nb_bits, bw_pos,
                                                     bw_run);

    uint32_t* p_bw = probas + 2048 / 4 - SIZE32_AC_STATE;
    p_bw[0] = bw_range;
    p_bw[1] = bw_value;
    p_bw[2] = bw_nb_bits;
    p_bw[3] = bw_pos;
    p_bw[4] = bw_run;
    p_bw[5] = (1024 + bw_pos) << 2; // MAX_NUM_MB_W * MAX_NUM_MB_H * 384 / SYSTEM_MIN_COMP_RATIO - 1;
    p_bw[6] = 0;
    p_bw[7] = 0;
}
void Pack_ret6_pred64_to32bits(uint32_t* pdes_ret,
                               uint32_t* pdes_pred,
                               hls::stream<ap_uint<6> >& str_ret,
                               hls::stream<ap_uint<64> >& str_pred,
                               int num_str) {
    const int BURST_32 = 256;        // Just use 1 2KB-BLOCK RAM
    const int B_RET = BURST_32 * 4;  // 2048 Bytes
    const int B_PRED = BURST_32 / 2; // 256 DWords

    uint32_t buff_ret[BURST_32];
    uint32_t buff_pred[BURST_32];
    ap_uint<8> tmp_ret_r = 0;
    ap_uint<32> tmp_ret32;
    int cnt_ret = 0;          // 0~BURST_32-1
    int cnt_pred = 0;         // 0~BURST_32-1
    int offset_ret_buff = 0;  // 0,1,2,3
    int offset_pred_read = 0; // 0,1
PRED64_TO32:
    for (int i_read = 0; i_read < num_str; i_read++) {
#pragma HLS PIPELINE off
        ap_uint<1> isFnl;
        if (i_read == num_str - 1)
            isFnl = 1;
        else
            isFnl = 0;
        tmp_ret_r(5, 0) = str_ret.read();
        ap_uint<64> pred_r = str_pred.read();
        int off_ret = i_read & 3;
        tmp_ret32(off_ret * 8 + 7, off_ret * 8) = tmp_ret_r(7, 0);
        if ((i_read & 3) == 3 || isFnl) buff_ret[cnt_ret++] = tmp_ret32;
        buff_pred[cnt_pred++] = pred_r(31, 0);
        buff_pred[cnt_pred++] = pred_r(63, 32);

        if (cnt_ret == BURST_32 || isFnl) {
            memcpy(pdes_ret, buff_ret, cnt_ret * 4);
            pdes_ret += cnt_ret;
            cnt_ret = 0;
        }
        if (cnt_pred == BURST_32 || isFnl) {
            memcpy(pdes_pred, buff_pred, cnt_pred * 4);
            pdes_pred += cnt_pred;
            cnt_pred = 0;
        }
    } // for i_read;
}

void kernel_2_Top_dataflow(uint32_t* pin_level, //[SIZE32_MEM_BW],
                           uint32_t* prob,
                           uint32_t* pout_bw,   //[SIZE32_MEM_BW],
                           uint32_t* pout_ret,  //[SIZE32_MEM_RET],
                           uint32_t* pout_pred) //[SIZE32_MEM_PRED])
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level, str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h,
                     str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    VP8EmitTokens_str_hls_4stages_32bits(pout_bw, tokens_str_final, (uint32_t*)prob);
    // PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    // PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
    Pack_ret6_pred64_to32bits(pout_ret,     // uint32_t* pdes_ret,
                              pout_pred,    // uint32_t* pdes_pred,
                              str_ret,      // hls::stream<ap_uint<6> > str_ret,
                              str_pred,     // hls::stream<ap_uint<64> > str_pred,
                              mb_h * mb_w); // int num_str)
}

void kernel_2_Top_dataflow(uint32_t pin_level[SIZE32_MEM_BW],
                           uint32_t* prob,
                           uint32_t pout_bw[SIZE32_MEM_BW],
                           uint32_t pout_ret[SIZE32_MEM_RET],
                           uint32_t pout_pred[SIZE32_MEM_PRED],
                           uint32_t* pt_num_mb) {
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
#pragma HLS STREAM variable = str_level_dc depth = 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_ac;
#pragma HLS STREAM variable = str_level_ac depth = 8 * 16
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
#pragma HLS STREAM variable = str_level_uv depth = 8 * 8
    hls::stream<ap_uint<64> > str_pred;
#pragma HLS STREAM variable = str_pred depth = 64
    hls::stream<ap_uint<6> > str_ret;
#pragma HLS STREAM variable = str_ret depth = 64
    hls::stream<ap_uint<1> > str_type_mb;
#pragma HLS STREAM variable = str_type_mb depth = 64
    hls::stream<uint16_t> str_mb_h;
#pragma HLS STREAM variable = str_mb_h depth = 64
    hls::stream<uint16_t> str_mb_w;
#pragma HLS STREAM variable = str_mb_w depth = 64
    Kernel2_top_read(pin_level, str_level_dc, str_level_ac, str_level_uv, str_pred, str_ret, str_type_mb, str_mb_h,
                     str_mb_w);
    hls::stream<ap_uint<64> > str_0_dc;
#pragma HLS STREAM variable = str_0_dc depth = 64
    hls::stream<ap_uint<64> > str_1_dc;
#pragma HLS STREAM variable = str_1_dc depth = 64
    hls::stream<ap_uint<64> > str_2_dc;
#pragma HLS STREAM variable = str_2_dc depth = 64
    hls::stream<ap_uint<64> > str_3_dc;
#pragma HLS STREAM variable = str_3_dc depth = 64
    hls::stream<ap_uint<64> > str_0_ac;
#pragma HLS STREAM variable = str_0_ac depth = 64
    hls::stream<ap_uint<64> > str_1_ac;
#pragma HLS STREAM variable = str_1_ac depth = 64
    hls::stream<ap_uint<64> > str_2_ac;
#pragma HLS STREAM variable = str_2_ac depth = 64
    hls::stream<ap_uint<64> > str_3_ac;
#pragma HLS STREAM variable = str_3_ac depth = 64
    hls::stream<ap_uint<64> > str_0_uv;
#pragma HLS STREAM variable = str_0_uv depth = 64
    hls::stream<ap_uint<64> > str_1_uv;
#pragma HLS STREAM variable = str_1_uv depth = 64
    hls::stream<ap_uint<64> > str_2_uv;
#pragma HLS STREAM variable = str_2_uv depth = 64
    hls::stream<ap_uint<64> > str_3_uv;
#pragma HLS STREAM variable = str_3_uv depth = 64
    hls::stream<ap_uint<1> > str_type_mb_out;
#pragma HLS STREAM variable = str_type_mb_out depth = 64

    hls::stream<uint16_t> str_mb_h_out;
#pragma HLS STREAM variable = str_mb_h_out depth = 64
    hls::stream<uint16_t> str_mb_w_out;
#pragma HLS STREAM variable = str_mb_w_out depth = 64
    kernel_2_RecordTokens_pre(str_mb_h, str_mb_w, str_type_mb, str_level_dc, str_level_ac, str_level_uv, str_0_dc,
                              str_1_dc, str_2_dc, str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv,
                              str_2_uv, str_3_uv, str_mb_h_out, str_mb_w_out, str_type_mb_out);

    hls::stream<uint16_t> str_mb_h_out2;
#pragma HLS STREAM variable = str_mb_h_out2 depth = 64
    hls::stream<uint16_t> str_mb_w_out2;
#pragma HLS STREAM variable = str_mb_w_out2 depth = 64
    hls::stream<ap_uint<16> > tokens_str_final;
#pragma HLS STREAM variable = tokens_str_final depth = 1024
    kernel_2_CreateTokens_with_isFinal(str_mb_h_out, str_mb_w_out, str_type_mb_out, str_0_dc, str_1_dc, str_2_dc,
                                       str_3_dc, str_0_ac, str_1_ac, str_2_ac, str_3_ac, str_0_uv, str_1_uv, str_2_uv,
                                       str_3_uv, str_mb_h_out2, str_mb_w_out2, tokens_str_final);

    uint16_t mb_h = str_mb_h_out2.read();
    uint16_t mb_w = str_mb_w_out2.read();
    uint32_t num_mb = mb_h * mb_w;
    VP8EmitTokens_str_hls_4stages_32bits(pout_bw, tokens_str_final, (uint32_t*)prob);
    // PackStr2Mem32_t_NoLast<6, 256>(pout_ret, str_ret, mb_h * mb_w);
    // PackWideStr2Mem32_t_NoLast<64, 256>(pout_pred, str_pred, mb_h * mb_w);
    Pack_ret6_pred64_to32bits(pout_ret,  // uint32_t* pdes_ret,A
                              pout_pred, // uint32_t* pdes_pred,
                              str_ret,   // hls::stream<ap_uint<6> > str_ret,
                              str_pred,  // hls::stream<ap_uint<64> > str_pred,
                              num_mb);   // int num_str)
    *pt_num_mb = num_mb;
}
#define USING_PIC_BURST
void kernel_2_ArithmeticCoding_core( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
    uint32_t prob[1 * 8 * 3 * 11];
    memcpy(prob, pin_prob, sizeof(prob));
    kernel_2_Top_dataflow(pin_level,  // uint32_t pin_level[SIZE32_MEM_BW],
                          prob,       // uint32_t* prob,
                          pout_bw,    // uint32_t pout_bw[SIZE32_MEM_BW],
                          pout_ret,   // uint32_t pout_ret[SIZE32_MEM_RET],
                          pout_pred); // uint32_t pout_pred[SIZE32_MEM_PRED])
}
void kernel_2_ArithmeticCoding_core_wrapper_bad_bad(uint32_t pin_level_mult[SIZE32_MEM_BW],
                                                    uint32_t pin_prob_mult[2048 / 4 * 64],
                                                    uint32_t pout_bw_mult[SIZE32_MEM_BW],
                                                    uint32_t pout_ret_mult[SIZE32_MEM_RET],
                                                    uint32_t pout_pred_mult[SIZE32_MEM_PRED]) {
    uint32_t* pin_level = pin_level_mult; //_mult[SIZE32_MEM_BW],
    uint32_t* pin_prob = pin_prob_mult;   //_mult[2048/4*64],
    uint32_t* pout_bw = pout_bw_mult;     //_mult[SIZE32_MEM_BW],
    uint32_t* pout_ret = pout_ret_mult;   //_mult[SIZE32_MEM_RET],
    uint32_t* pout_pred = pout_pred_mult; //_mult[SIZE32_MEM_PRED])

    uint32_t offset_pin_level = 0; // = Get_Busoffset_level(num_mb);
    uint32_t offset_prob = 0;      // Get_Busoffset_prob_32bits();
    uint32_t offset_pout_bw = 0;   // Get_Busoffset_pout_bw(num_mb);
    uint32_t offset_pout_ret = 0;  // Get_Busoffset_pout_ret(num_mb);
    uint32_t offset_pout_pred = 0; // Get_Busoffset_pout_pred(num_mb);

    uint32_t pid_mult = pin_prob[OFF_PID_PROB_8BIT / 4];
CORE_WRAPPER_BAD:
    for (int myloop = 0; myloop <= pid_mult; myloop++) {
        uint32_t prob[512]; //[1 * 8 * 3 * 11];

        memcpy(prob, pin_prob, (OFF_NUM_MB_32 + 1) * 4);
        for (int k = 256; k < (OFF_NUM_MB_32 + 1); k++) printf("%d, ", prob[k]);
        printf("\n");

        uint32_t num_mb = prob[OFF_NUM_MB_32];
        kernel_2_Top_dataflow(pin_level + offset_pin_level, // uint32_t pin_level[SIZE32_MEM_BW],
                              prob,                         // uint32_t* prob,
                              pout_bw + offset_pout_bw,     // uint32_t pout_bw[SIZE32_MEM_BW],
                              pout_ret + offset_pout_ret,   // uint32_t pout_ret[SIZE32_MEM_RET],
                              pout_pred + offset_pout_pred);
    SHIFT_PROB_504:
        for (int k = 0; k < 7; k++) pin_prob[offset_prob + 504 + k] = prob[504 + k];
        for (int k = 0; k < 7; k++) printf("%d, ", pin_prob[offset_prob + 504 + k]);
        printf("\n");
        // memcpy(pin_prob, prob, (7)*4 );
        // pin_prob[500] = i;
        offset_pin_level += Get_Busoffset_level(num_mb);
        offset_prob += Get_Busoffset_prob_32bits();
        offset_pout_bw += Get_Busoffset_pout_bw(num_mb);
        offset_pout_ret += Get_Busoffset_pout_ret(num_mb);
        offset_pout_pred += Get_Busoffset_pout_pred(num_mb);

    } // while( pid_mult!=0);
}
void kernel_2_ArithmeticCoding_core_wrapper(uint32_t pin_level_mult[SIZE32_MEM_BW],
                                            uint32_t pin_prob_mult[2048 / 4 * 64],
                                            uint32_t pout_bw_mult[SIZE32_MEM_BW],
                                            uint32_t pout_ret_mult[SIZE32_MEM_RET],
                                            uint32_t pout_pred_mult[SIZE32_MEM_PRED]) {
    uint32_t* pin_level = pin_level_mult; //_mult[SIZE32_MEM_BW],
    uint32_t* pin_prob = pin_prob_mult;   //_mult[2048/4*64],
    uint32_t* pout_bw = pout_bw_mult;     //_mult[SIZE32_MEM_BW],
    uint32_t* pout_ret = pout_ret_mult;   //_mult[SIZE32_MEM_RET],
    uint32_t* pout_pred = pout_pred_mult; //_mult[SIZE32_MEM_PRED])

    uint32_t pid_mult = pin_prob[OFF_PID_PROB_8BIT / 4];
CORE_WRAPPER:
    for (int myloop = 0; myloop <= pid_mult; myloop++) {
        //	do{
        uint32_t prob[512]; //[1 * 8 * 3 * 11];
    COPY_PROB_256:
        for (int k = 0; k < 265; k++) prob[k] = pin_prob[k];
        // memcpy(prob, pin_prob,4 * 8 * 3 * 11);// sizeof(prob));
        // pid_mult = pin_prob[OFF_PID_PROB_8BIT/4];
        // pid_mult -= 1;
        uint32_t num_mb = prob[264];
        kernel_2_Top_dataflow(pin_level,  // uint32_t pin_level[SIZE32_MEM_BW],
                              prob,       // uint32_t* prob,
                              pout_bw,    // uint32_t pout_bw[SIZE32_MEM_BW],
                              pout_ret,   // uint32_t pout_ret[SIZE32_MEM_RET],
                              pout_pred); //,	//uint32_t pout_pred[SIZE32_MEM_PRED])
                                          //&num_mb);
                                          // memcpy(pin_prob+504, prob+504, 28);//the last DWord is used for dirty
    MEMCPY_504:
        for (int k = 0; k < 7; k++) pin_prob[504 + k] = prob[504 + k];
        // pin_prob[500] = i;
        uint32_t offset_pin_level = Get_Busoffset_level(num_mb);
        uint32_t offset_prob = Get_Busoffset_prob_32bits();
        uint32_t offset_pout_bw = Get_Busoffset_pout_bw(num_mb);
        uint32_t offset_pout_ret = Get_Busoffset_pout_ret(num_mb);
        uint32_t offset_pout_pred = Get_Busoffset_pout_pred(num_mb);
        pin_level += offset_pin_level; //_mult[SIZE32_MEM_BW],
        pin_prob += offset_prob;       //_mult[2048/4*64],
        pout_bw += offset_pout_bw;     //_mult[SIZE32_MEM_BW],
        pout_ret += offset_pout_ret;   //_mult[SIZE32_MEM_RET],
        pout_pred += offset_pout_pred; //_mult[SIZE32_MEM_PRED])
    }                                  // while( pid_mult!=0);
}

// namespace xf {
// namespace codec {

extern "C" {
void webp_2_ArithmeticCoding_1( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
//#pragma HLS DATAFLOW
// memcpy(pout_bw, pin_prob, 2048*5);
// memcpy(pout_bw+2048*5, pin_level, 1024*5);
/*	uint32_t tmp[2048*8];
        memcpy(tmp, pin_prob, 2048*5);
                memcpy(pout_bw, tmp, 2048*5);

                memcpy(tmp, pin_level, 1024*5);

                memcpy(pout_bw+2048*5, tmp, 1024*5);*/
/*		for(int i=0;i<2048/4*5;i++)
                        pout_bw[i] = pin_prob[i];

                for(int i=0;i<1024/4*5;i++)
                                pout_bw[2048/4*5+i] = pin_level[i];

*/
#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}
//} // namespace codec
//} // namespace xf

extern "C" {
void kernel_2_ArithmeticCoding_2( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}

extern "C" {
void kernel_2_ArithmeticCoding_3( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}

extern "C" {
void kernel_2_ArithmeticCoding_4( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}

extern "C" {
void kernel_2_ArithmeticCoding_5( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW
#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}

extern "C" {
void kernel_2_ArithmeticCoding_6( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}

extern "C" {
void kernel_2_ArithmeticCoding_7( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}

extern "C" {
void kernel_2_ArithmeticCoding_8( // NoWrapper
    uint32_t pin_level[SIZE32_MEM_BW],
    uint32_t pin_prob[2048 / 4 * 64],
    uint32_t pout_bw[SIZE32_MEM_BW],
    uint32_t pout_ret[SIZE32_MEM_RET],
    uint32_t pout_pred[SIZE32_MEM_PRED]) {
#pragma HLS INTERFACE m_axi port = pin_level offset = slave bundle = gmem0 depth =               \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pin_prob offset = slave bundle = gmem1 depth =              \
    2048 / 4 * 64 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_bw offset = slave bundle = gmem2 depth =                     \
    65536 * 384 / 4 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_ret offset = slave bundle = gmem1 depth =              \
    65536 * 1 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb
#pragma HLS INTERFACE m_axi port = pout_pred offset = slave bundle = gmem1 depth =                  \
    65536 * 16 / 2 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32 // 32bb

#pragma HLS INTERFACE s_axilite port = pin_level bundle = control
#pragma HLS INTERFACE s_axilite port = pin_prob bundle = control
#pragma HLS INTERFACE s_axilite port = pout_bw bundle = control
#pragma HLS INTERFACE s_axilite port = pout_ret bundle = control
#pragma HLS INTERFACE s_axilite port = pout_pred bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS DATAFLOW

#ifdef USING_PIC_BURST
    kernel_2_ArithmeticCoding_core_wrapper(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#else
    kernel_2_ArithmeticCoding_core(pin_level, pin_prob, pout_bw, pout_ret, pout_pred);
#endif
}
}
