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
//==========================      kernel_IntraPredLoop2_NoOut             ==============================//
//////////////////////////////////////////////////////////////////////////////////////////////////////////
// kernel_IntraPredLoop2_NoOut
//|-memcpy
//|-TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO
//  |-TopVp8_read_2_32bit_NoStruct
//  | |-TopVp8_read__dataflow_32bit...
//  |-TopVp8_compute...
//  |-TopVp8_RecordCoeff_hls_cnt
//  | |-FindLast
//  | |-VP8RecordCoeffs_hls_str_w_cnt
//  |   |-Record_str
//  |   |-VP8EncBands_hls
//  |-TopVp8_RecordProb_hls_cnt
//  | |-RecordPorb_ReadCoeff_dataflow2_cnt
//  |   |-RecordPorb_ReadCoeff_dataflow_dc_cnt
//  |     |-RecordPorb_ReadCoeff_dataflow_ac_cnt
//  |     | |-VP8RecordCoeffs_hls_str_r_cnt
//  |     |-RecordPorb_ReadCoeff_dataflow_uv_cnt...
//  |     |-RecordPorb_ReadCoeff_dataflow2_cnt...
//  |-TopVp8_send_32bit
void kernel_IntraPredLoop2_NoOut_core(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
    int p_readinfo[64];
    memcpy(p_readinfo, p_info, 64 * sizeof(int));
    ap_uint<32> id_pic;
    ap_uint<32> mb_line;
    ap_uint<LG2_MAX_W_PIX> y_stride;
    ap_uint<LG2_MAX_W_PIX> uv_stride;
    ap_uint<LG2_MAX_W_PIX> width;
    ap_uint<LG2_MAX_W_PIX> height;
    ap_uint<LG2_MAX_NUM_MB_W> mb_w;
    ap_uint<LG2_MAX_NUM_MB_H> mb_h;
    ap_uint<WD_LMD> lambda_p16;
    ap_uint<WD_LMD> lambda_p44;
    ap_uint<WD_LMD> tlambda;
    ap_uint<WD_LMD> lambda_uv;
    ap_uint<WD_LMD> tlambda_m;
    hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
    ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;

    // Initializing image variables, once for one picture
    { // For convenience, extend the code at top module to show all parameters used by kernel of intra-prediction
        id_pic = p_readinfo[0];  // reserved for future
        mb_line = p_readinfo[1]; // reserved for future, to show current line number of mb
        y_stride = p_readinfo[2];
        uv_stride = p_readinfo[3];
        width = p_readinfo[4];
        height = p_readinfo[5];
        mb_w = p_readinfo[2 + 2 + 2];
        mb_h = p_readinfo[3 + 2 + 2];
        lambda_p16 = p_readinfo[4 + 2 + 2];
        lambda_p44 = p_readinfo[5 + 2 + 2];
        tlambda = p_readinfo[6 + 2 + 2];
        lambda_uv = p_readinfo[7 + 2 + 2];
        tlambda_m = p_readinfo[8 + 2 + 2];

        hls_qm1.q_0 = p_readinfo[11 + 2]; // quantizer steps
        hls_qm1.q_n = p_readinfo[12 + 2];
        hls_qm1.iq_0 = p_readinfo[13 + 2]; // reciprocals fixed point.
        hls_qm1.iq_n = p_readinfo[14 + 2];
        hls_qm1.bias_0 = p_readinfo[15 + 2]; // rounding bias
        hls_qm1.bias_n = p_readinfo[16 + 2];

        hls_qm2.q_0 = p_readinfo[17 + 2]; // quantizer steps
        hls_qm2.q_n = p_readinfo[18 + 2];
        hls_qm2.iq_0 = p_readinfo[19 + 2]; // reciprocals fixed point.
        hls_qm2.iq_n = p_readinfo[20 + 2];
        hls_qm2.bias_0 = p_readinfo[21 + 2]; // rounding bias
        hls_qm2.bias_n = p_readinfo[22 + 2];

        hls_qm_uv.q_0 = p_readinfo[23 + 2]; // quantizer steps
        hls_qm_uv.q_n = p_readinfo[24 + 2];
        hls_qm_uv.iq_0 = p_readinfo[25 + 2]; // reciprocals fixed point.
        hls_qm_uv.iq_n = p_readinfo[26 + 2];
        hls_qm_uv.bias_0 = p_readinfo[27 + 2]; // rounding bias
        hls_qm_uv.bias_n = p_readinfo[28 + 2];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_readinfo[29 + 2 + 16 + i];
    } // end of initialization
    int dirty = 0;
    TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(id_pic,     // p_info[0],
                                                      mb_line,    // p_info[1],
                                                      y_stride,   // p_info[2],  // ,//pic->y_stride,
                                                      uv_stride,  // p_info[3], // ,//pic->uv_stride
                                                      width,      // p_info[4],  // ,//pic->width
                                                      height,     // p_info[5],  // ,//pic->height
                                                      mb_w,       // p_info[2+2+2],///,
                                                      mb_h,       // p_info[3+2+2],//,
                                                      lambda_p16, // p_info[4+2+2],//dqm->lambda_i16_,
                                                      lambda_p44, // p_info[5+2+2],//dqm->lambda_i4_,
                                                      tlambda,    // p_info[6+2+2],//dqm->tlambda_,
                                                      lambda_uv,  // p_info[7+2+2],//dqm->lambda_uv_,
                                                      tlambda_m,  // p_info[8+2+2],//dqm->lambda_mode_,
                                                      hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv,
                                                      ysrc,       // 4096x4096
                                                      usrc,       // 2048x2048
                                                      vsrc,       // 2048x2048
                                                      pout_level, // 65536*512
                                                      pout_prob, &dirty);
}

extern "C" {
void kernel_IntraPredLoop2_NoOut(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
}
}

void TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(ap_uint<32> id_pic,
                                                       ap_uint<32> mb_line,
                                                       ap_uint<LG2_MAX_W_PIX> y_stride,
                                                       ap_uint<LG2_MAX_W_PIX> uv_stride,
                                                       ap_uint<LG2_MAX_W_PIX> width,
                                                       ap_uint<LG2_MAX_W_PIX> height,
                                                       ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                                                       ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                                                       ap_uint<WD_LMD> lambda_p16,
                                                       ap_uint<WD_LMD> lambda_p44,
                                                       ap_uint<WD_LMD> tlambda,
                                                       ap_uint<WD_LMD> lambda_uv,
                                                       ap_uint<WD_LMD> tlambda_m,
                                                       hls_QMatrix hls_qm1,
                                                       hls_QMatrix hls_qm2,
                                                       hls_QMatrix hls_qm_uv,
                                                       ap_int<WD_sharpen * 16> ap_sharpen,
                                                       ap_int<WD_sharpen * 16> ap_sharpen_uv,
                                                       uint32_t* ysrc,
                                                       uint32_t* usrc,
                                                       uint32_t* vsrc,
                                                       int32_t* pout_level,
                                                       uint8_t* pout_prob,
                                                       int* dirty) {
    // hls::stream<ap_uint<WD_PIX * 16> > str_out_inst;
    // hls::stream<ap_uint<WD_PIX* 16> >* str_out = &str_out_inst;
    hls::stream<ap_uint<WD_PIX * 16> > str_out;
#pragma HLS STREAM variable = str_out depth = 8 * 8
#pragma HLS DATAFLOW
    hls::stream<ap_uint<WD_PIX * 16> > str_din_y;
    hls::stream<ap_uint<WD_PIX * 16> > str_din_uv;
#pragma HLS STREAM variable = str_din_y depth = 16 * 128
#pragma HLS STREAM variable = str_din_uv depth = 8 * 128
    TopVp8_read_2_32bit_NoStruct( // For 4k,
        ysrc, usrc, vsrc, y_stride, uv_stride, width, height, mb_w, mb_h, &str_din_y, &str_din_uv);

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_y;
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
    hls::stream<ap_int<64> > str_pred;
    hls::stream<ap_int<6> > str_ret;
#pragma HLS STREAM variable = str_level_y depth = 16 * 4 * 4 // Deep
#pragma HLS STREAM variable = str_level_dc depth = 1 * 4 * 4 // Deep
#pragma HLS STREAM variable = str_level_uv depth = 8 * 4 * 4 // Deep
#pragma HLS STREAM variable = str_pred depth = 1 * 4 * 4     // Deep
#pragma HLS STREAM variable = str_ret depth = 1 * 4 * 4      // Deep
    TopVp8_compute(mb_w, mb_h, &str_din_y, &str_din_uv, lambda_p16, lambda_p44, tlambda, lambda_uv, tlambda_m, hls_qm1,
                   hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv, &str_out, &str_level_dc, &str_level_y, &str_level_uv,
                   &str_pred, &str_ret);

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_y2;
#pragma HLS STREAM variable = str_level_y2 depth = 4 * 16 * 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc2;
#pragma HLS STREAM variable = str_level_dc2 depth = 4 * 1 * 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv2;
#pragma HLS STREAM variable = str_level_uv2 depth = 4 * 8 * 4
    hls::stream<ap_int<64> > str_pred2;
#pragma HLS STREAM variable = str_pred2 depth = 4 * 1 * 4
    hls::stream<ap_int<6> > str_ret2;
#pragma HLS STREAM variable = str_ret2 depth = 4 * 1 * 4
    hls::stream<ap_uint<1> > str_mb_type;
#pragma HLS STREAM variable = str_mb_type depth = 16 * 1
    hls::stream<ap_uint<11> > str_rec_dc;
#pragma HLS STREAM variable = str_rec_dc depth = 2 * 1 * 16 * 64
    hls::stream<ap_uint<11> > str_rec_ac;
#pragma HLS STREAM variable = str_rec_ac depth = 4 * 16 * 16 * 8
    hls::stream<ap_uint<11> > str_rec_uv;
#pragma HLS STREAM variable = str_rec_uv depth = 2 * 8 * 16 * 8
    hls::stream<ap_uint<8> > str_cnt_dc;
#pragma HLS STREAM variable = str_cnt_dc depth = 2 * 1 * 1 * 64
    hls::stream<ap_uint<8> > str_cnt_ac;
#pragma HLS STREAM variable = str_cnt_ac depth = 2 * 16 * 1 * 8
    hls::stream<ap_uint<8> > str_cnt_uv;
#pragma HLS STREAM variable = str_cnt_uv depth = 2 * 8 * 1 * 8
    TopVp8_RecordCoeff_hls_cnt(mb_w, mb_h, &str_level_dc, &str_level_y, &str_level_uv, &str_pred, &str_ret, str_mb_type,
                               &str_level_dc2, &str_level_y2, &str_level_uv2, &str_pred2, &str_ret2, str_rec_dc,
                               str_rec_ac, str_rec_uv, str_cnt_dc, str_cnt_ac, str_cnt_uv);

    *dirty = TopVp8_RecordProb_hls_cnt(mb_w, mb_h, str_mb_type, str_rec_dc, str_rec_ac, str_rec_uv, str_cnt_dc,
                                       str_cnt_ac, str_cnt_uv, pout_prob);

    TopVp8_send_32bit(mb_w, mb_h, &str_level_dc2, &str_level_y2, &str_level_uv2, &str_pred2, &str_ret2, pout_level);

SEND_OUT_READ:
    for (int y = 0; y < mb_h; y++)
    X:
        for (int x = 0; x < mb_w; x++)
        I:
            for (int i = 0; i < 24; i++) str_out.read();
}

void TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO_HideDirty(ap_uint<32> id_pic,
                                                                 ap_uint<32> mb_line,
                                                                 ap_uint<LG2_MAX_W_PIX> y_stride,
                                                                 ap_uint<LG2_MAX_W_PIX> uv_stride,
                                                                 ap_uint<LG2_MAX_W_PIX> width,
                                                                 ap_uint<LG2_MAX_W_PIX> height,
                                                                 ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                                                                 ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                                                                 ap_uint<WD_LMD> lambda_p16,
                                                                 ap_uint<WD_LMD> lambda_p44,
                                                                 ap_uint<WD_LMD> tlambda,
                                                                 ap_uint<WD_LMD> lambda_uv,
                                                                 ap_uint<WD_LMD> tlambda_m,
                                                                 hls_QMatrix hls_qm1,
                                                                 hls_QMatrix hls_qm2,
                                                                 hls_QMatrix hls_qm_uv,
                                                                 ap_int<WD_sharpen * 16> ap_sharpen,
                                                                 ap_int<WD_sharpen * 16> ap_sharpen_uv,
                                                                 uint32_t* ysrc,
                                                                 uint32_t* usrc,
                                                                 uint32_t* vsrc,
                                                                 int32_t* pout_level,
                                                                 uint8_t* pout_prob) {
#pragma HLS DATAFLOW
    hls::stream<ap_uint<WD_PIX * 16> > str_din_y;
    hls::stream<ap_uint<WD_PIX * 16> > str_din_uv;
#pragma HLS STREAM variable = str_din_y depth = 16 * 128
#pragma HLS STREAM variable = str_din_uv depth = 8 * 128
    TopVp8_read_2_32bit_NoStruct( // For 4k,
        ysrc, usrc, vsrc, y_stride, uv_stride, width, height, mb_w, mb_h, &str_din_y, &str_din_uv);

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_y;
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc;
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv;
    hls::stream<ap_int<64> > str_pred;
    hls::stream<ap_int<6> > str_ret;
#pragma HLS STREAM variable = str_level_y depth = 16 * 4 * 4 // Deep
#pragma HLS STREAM variable = str_level_dc depth = 1 * 4 * 4 // Deep
#pragma HLS STREAM variable = str_level_uv depth = 8 * 4 * 4 // Deep
#pragma HLS STREAM variable = str_pred depth = 1 * 4 * 4     // Deep
#pragma HLS STREAM variable = str_ret depth = 1 * 4 * 4      // Deep
    TopVp8_compute_NoOut(mb_w, mb_h, &str_din_y, &str_din_uv, lambda_p16, lambda_p44, tlambda, lambda_uv, tlambda_m,
                         hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv, &str_level_dc, &str_level_y,
                         &str_level_uv, &str_pred, &str_ret);

    hls::stream<ap_int<WD_LEVEL * 16> > str_level_y2;
#pragma HLS STREAM variable = str_level_y2 depth = 4 * 16 * 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_dc2;
#pragma HLS STREAM variable = str_level_dc2 depth = 4 * 1 * 4
    hls::stream<ap_int<WD_LEVEL * 16> > str_level_uv2;
#pragma HLS STREAM variable = str_level_uv2 depth = 4 * 8 * 4
    hls::stream<ap_int<64> > str_pred2;
#pragma HLS STREAM variable = str_pred2 depth = 4 * 1 * 4
    hls::stream<ap_int<6> > str_ret2;
#pragma HLS STREAM variable = str_ret2 depth = 4 * 1 * 4
    hls::stream<ap_uint<1> > str_mb_type;
#pragma HLS STREAM variable = str_mb_type depth = 16 * 1
    hls::stream<ap_uint<11> > str_rec_dc;
#pragma HLS STREAM variable = str_rec_dc depth = 2 * 1 * 16 * 64
    hls::stream<ap_uint<11> > str_rec_ac;
#pragma HLS STREAM variable = str_rec_ac depth = 4 * 16 * 16 * 8
    hls::stream<ap_uint<11> > str_rec_uv;
#pragma HLS STREAM variable = str_rec_uv depth = 2 * 8 * 16 * 8
    hls::stream<ap_uint<8> > str_cnt_dc;
#pragma HLS STREAM variable = str_cnt_dc depth = 2 * 1 * 1 * 64
    hls::stream<ap_uint<8> > str_cnt_ac;
#pragma HLS STREAM variable = str_cnt_ac depth = 2 * 16 * 1 * 8
    hls::stream<ap_uint<8> > str_cnt_uv;
#pragma HLS STREAM variable = str_cnt_uv depth = 2 * 8 * 1 * 8
    TopVp8_RecordCoeff_hls_cnt(mb_w, mb_h, &str_level_dc, &str_level_y, &str_level_uv, &str_pred, &str_ret, str_mb_type,
                               &str_level_dc2, &str_level_y2, &str_level_uv2, &str_pred2, &str_ret2, str_rec_dc,
                               str_rec_ac, str_rec_uv, str_cnt_dc, str_cnt_ac, str_cnt_uv);

    TopVp8_RecordProb_hls_cnt_HideDirty(mb_w, mb_h, str_mb_type, str_rec_dc, str_rec_ac, str_rec_uv, str_cnt_dc,
                                        str_cnt_ac, str_cnt_uv, pout_prob);

    TopVp8_send_32bit(mb_w, mb_h, &str_level_dc2, &str_level_y2, &str_level_uv2, &str_pred2, &str_ret2, pout_level);
}
//////////======================================================================/////////////////////////////
//////////====================  TopVp8_send_32bit  =================/////////////////////////////
//////////======================================================================/////////////////////////////
void TopVp8_send_32bit(ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                       ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                       hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                       hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                       hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                       hls::stream<ap_int<64> >* str_pred,
                       hls::stream<ap_int<6> >* str_ret,
                       // output
                       int32_t* pout_level) {
#pragma HLS dataflow
    hls::stream<int16_t> tmp_str;
    int16_t tmp_arr[512];

SEND_ARRAY:
    for (int y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
        for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
            TopVp8_send__strs_to_array(tmp_arr,
                                       //&tmp_str,
                                       str_level_dc, str_level_y, str_level_uv, str_pred, str_ret);
            tmp_arr[420] = mb_w;
            tmp_arr[421] = mb_h;
        SEND_256:
            for (int i = 0; i < 256; i++) {
#pragma HLS pipeline
                ap_uint<32> tmp;
                tmp(15, 0) = tmp_arr[i * 2];
                tmp(31, 16) = tmp_arr[i * 2 + 1];
                pout_level[i] = tmp;
            }
            pout_level += 256;
        }
    }
}
//////////====================  TopVp8_send_32bit  =================/////////////////////////////
void TopVp8_send__strs_to_array(short int* pout,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                                hls::stream<ap_int<64> >* str_pred,
                                hls::stream<ap_int<6> >* str_ret) {
#pragma HLS PIPELINE
    int x, y, ch;
    ap_int<WD_LEVEL* 16> tmp16 = str_level_dc->read();

    short int* y_dc_levels = pout; //[16];
    CPY16(y_dc_levels, tmp16, WD_LEVEL);
    pout += 16;

    // luma-AC
    for (y = 0; y < 4; ++y) {
    SEND_ARRAY_LU:
        for (x = 0; x < 4; ++x) {
#pragma HLS PIPELINE
            short int* y_ac_levels = pout; //;[16]
            ap_int<WD_LEVEL* 16> tmp = str_level_y->read();
            CPY16(y_ac_levels, tmp, WD_LEVEL);
            pout += 16;
        }
    }

    // U/V
    for (ch = 0; ch <= 2; ch += 2) {
        for (y = 0; y < 2; ++y) {
        SEND_ARRAY_UV:
            for (x = 0; x < 2; ++x) {
#pragma HLS PIPELINE
                short int* uv_levels = pout; //;[16];
                ap_int<WD_LEVEL* 16> tmp = str_level_uv->read();
                CPY16(uv_levels, tmp, WD_LEVEL);
                pout += 16;
            }
        }
    }

    uint16_t* pred16 = (uint16_t*)pout;
    ap_uint<64> pred = str_pred->read();
#pragma HLS PIPELINE
    CPY16U(pred16, pred, 4);
    pout += 16;
    ap_uint<6> ret = str_ret->read();
    *pout = (uint16_t)ret;
}
//////////====================  TopVp8_send_32bit  =================/////////////////////////////

//////////======================================================================/////////////////////////////
//////////====================  TopVp8_read_2_32bit_NoStruct  =================/////////////////////////////
//////////======================================================================/////////////////////////////
void TopVp8_read_2_32bit_NoStruct(
    // input
    uint32_t* ysrc,
    uint32_t* usrc,
    uint32_t* vsrc,
    ap_uint<LG2_MAX_W_PIX> y_stride,
    ap_uint<LG2_MAX_W_PIX> uv_stride,
    ap_uint<LG2_MAX_W_PIX> width,
    ap_uint<LG2_MAX_W_PIX> height,
    ap_uint<LG2_MAX_NUM_MB_W> mb_w,
    ap_uint<LG2_MAX_NUM_MB_H> mb_h,
    // output
    hls::stream<ap_uint<WD_PIX * 16> >* str_din_y,
    hls::stream<ap_uint<WD_PIX * 16> >* str_din_uv) {
    TopVp8_read__dataflow_32bit( //
        // input
        y_stride, uv_stride, width, height, mb_w, mb_h, ysrc, usrc, vsrc,
        // output
        str_din_y, str_din_uv);
}

void TopVp8_read__dataflow_32bit(
    // input
    ap_uint<LG2_MAX_W_PIX> y_stride,  //
    ap_uint<LG2_MAX_W_PIX> uv_stride, //
    ap_uint<LG2_MAX_W_PIX> width,     //
    ap_uint<LG2_MAX_W_PIX> height,    //
    ap_uint<LG2_MAX_NUM_MB_W> mb_w,   //
    ap_uint<LG2_MAX_NUM_MB_H> mb_h,   //
    uint32_t ysrc[MAX_W_PIX * MAX_H_PIX / 4],
    uint32_t usrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
    uint32_t vsrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
    // output
    hls::stream<ap_uint<WD_PIX * 16> >* str_din_y,
    hls::stream<ap_uint<WD_PIX * 16> >* str_din_uv) {
    /* MB Line buffer */
    uint32_t buff_line_mb_y[MAX_W_PIX * 16 / 4];
#pragma HLS RESOURCE variable = buff_line_mb_y core = XPM_MEMORY uram // 32bb
    uint32_t buff_line_mb_u[MAX_W_PIX * 4 / 4];                       // 32bb
#pragma HLS RESOURCE variable = buff_line_mb_y core = XPM_MEMORY uram // 32bb
    uint32_t buff_line_mb_v[MAX_W_PIX * 4 / 4];                       // 32bb
#pragma HLS RESOURCE variable = buff_line_mb_y core = XPM_MEMORY uram // 32bb
// uint32_t  buff_line_mb_y2[MAX_W_PIX*16/4];//32bb
// uint32_t  buff_line_mb_u2[MAX_W_PIX*4/4];//32bb
// uint32_t  buff_line_mb_v2[MAX_W_PIX*4/4];//32bb
TOPVP8_DATAFOW:
    for (int y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
#pragma HLS dataflow
        hls_ReadMBLine_32bit_const(ysrc, //[MAX_W_PIX*MAX_H_PIX],
                                   usrc, //[MAX_W_PIX*MAX_H_PIX/4],
                                   vsrc, //[MAX_W_PIX*MAX_H_PIX/4],
                                   y_mb, y_stride, uv_stride,
                                   // output
                                   buff_line_mb_y, //[MAX_W_PIX*16],
                                   buff_line_mb_u, //[MAX_W_PIX*4],
                                   buff_line_mb_v  //[MAX_W_PIX*4]
                                   );

        TopVp8_read_MB_32bit_const( // about 650 * mb_w;
            width,                  //      = p_info[4];  // = pic->width
            height,                 //      = p_info[5];  // = pic->height
            mb_w,                   // = p_info[2+2+2];///;
            mb_h,                   // = p_info[3+2+2];//;
            y_mb, buff_line_mb_y, buff_line_mb_u, buff_line_mb_v, y_stride, uv_stride,
            // output
            str_din_y, str_din_uv);
    }
}

void hls_ReadMBLine_32bit_const_old(uint32_t ysrc[MAX_W_PIX * MAX_H_PIX / 4],
                                    uint32_t usrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                    uint32_t vsrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                    int y_mb,
                                    int y_stride,
                                    int uv_stride,
                                    // output
                                    uint32_t buff_line_mb_y[MAX_W_PIX * 16 / 4], // 32bb
                                    uint32_t buff_line_mb_u[MAX_W_PIX * 4 / 4],  // 32bb
                                    uint32_t buff_line_mb_v[MAX_W_PIX * 4 / 4]   // 32bb
                                    ) {
    int offset_y = y_mb * y_stride * 16 / 4;
    int offset_uv = y_mb * uv_stride * 8 / 4;
#pragma HLS dataflow
    hls_CopyMBLine_y_32bit_const(buff_line_mb_y, ysrc + offset_y, y_stride * 16 / 4);
    hls_CopyMBLine_uv_32bit_const(buff_line_mb_u, usrc + offset_uv, uv_stride * 8 / 4);
    hls_CopyMBLine_uv_32bit_const(buff_line_mb_v, vsrc + offset_uv, uv_stride * 8 / 4);
}

void hls_CopyMBLine_yuv_32bit_const(uint32_t ydes[MAX_W_PIX * 16 / 4],
                                    uint32_t udes[MAX_W_PIX / 2 * 8 / 4],
                                    uint32_t vdes[MAX_W_PIX / 2 * 8 / 4],
                                    uint32_t ysrc[MAX_W_PIX * MAX_H_PIX / 4],
                                    uint32_t usrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                    uint32_t vsrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                    int num_read_y,
                                    int num_read_uv) {
    int num_loop = (num_read_uv + NUM_BURST_READ - 1) >> LG2_NUM_BURST_READ; //
    int start_addr_y = 0;
    int start_addr_u = 0;
    int start_addr_v = 0;
COPY_YUV_Y32:
    for (int line = 0; line < num_loop; line++) {
#pragma HLS LOOP_TRIPCOUNT min = 1920 / 2 * 8 / 4 / 64 max = 4096 / 2 * 8 / 4 / 64
        //#pragma HLS PIPELINE
        //        memcpy(ydes, ysrc, NUM_BURST_READ * sizeof(uint32_t) * 4);
        //        ydes += (NUM_BURST_READ << 2);
        //        ysrc += (NUM_BURST_READ << 2);
        for (int i = 0; i < NUM_BURST_READ * 4; i++) {
#pragma HLS PIPELINE
            ydes[i + start_addr_y] = ysrc[i + start_addr_y];
        }
        start_addr_y += (NUM_BURST_READ << 2);
    }
COPY_YUV_U32:
    for (int line = 0; line < num_loop; line++) {
#pragma HLS LOOP_TRIPCOUNT min = 1920 / 2 * 8 / 4 / 64 max = 4096 / 2 * 8 / 4 / 64
        //#pragma HLS PIPELINE
        //        memcpy(udes, usrc, NUM_BURST_READ * sizeof(uint32_t));
        //        udes += NUM_BURST_READ;
        //        usrc += NUM_BURST_READ;
        for (int i = 0; i < NUM_BURST_READ; i++) {
#pragma HLS PIPELINE
            udes[i + start_addr_u] = usrc[i + start_addr_u];
        }
        start_addr_u += NUM_BURST_READ;
    }
COPY_YUV_V32:
    for (int line = 0; line < num_loop; line++) {
#pragma HLS LOOP_TRIPCOUNT min = 1920 / 2 * 8 / 4 / 64 max = 4096 / 2 * 8 / 4 / 64
        //#pragma HLS PIPELINE
        //        memcpy(vdes, vsrc, NUM_BURST_READ * sizeof(uint32_t));
        //        vdes += NUM_BURST_READ;
        //        vsrc += NUM_BURST_READ;
        for (int i = 0; i < NUM_BURST_READ; i++) {
#pragma HLS PIPELINE
            vdes[i + start_addr_v] = vsrc[i + start_addr_v];
        }
        start_addr_v += NUM_BURST_READ;
    }
}
void hls_ReadMBLine_32bit_const(uint32_t ysrc[MAX_W_PIX * MAX_H_PIX / 4],
                                uint32_t usrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                uint32_t vsrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                int y_mb,
                                int y_stride,
                                int uv_stride,
                                // output
                                uint32_t buff_line_mb_y[MAX_W_PIX * 16 / 4],
                                uint32_t buff_line_mb_u[MAX_W_PIX * 4 / 4],
                                uint32_t buff_line_mb_v[MAX_W_PIX * 4 / 4]) {
#pragma HLS INLINE OFF
    int offset_y = y_mb * y_stride * 16 / 4;
    int offset_uv = y_mb * uv_stride * 8 / 4;
    //#pragma HLS dataflow
    hls_CopyMBLine_yuv_32bit_const(buff_line_mb_y, buff_line_mb_u, buff_line_mb_v, ysrc + offset_y, usrc + offset_uv,
                                   vsrc + offset_uv, y_stride * 16 / 4, uv_stride * 8 / 4);
}
void hls_CopyMBLine_y_32bit_const(uint32_t ydes[MAX_W_PIX * 16 / 4],
                                  uint32_t ysrc[MAX_W_PIX * MAX_H_PIX / 4],
                                  int num_read) {
    int num_loop = (num_read + NUM_BURST_READ - 1) >> LG2_NUM_BURST_READ; //
COPY_Y32:
    for (int line = 0; line < num_loop; line++) {
#pragma HLS LOOP_TRIPCOUNT min = 1920 * 16 / 4 / 64 max = 4096 * 16 / 4 / 64
#pragma HLS PIPELINE
        memcpy(ydes, ysrc, NUM_BURST_READ * sizeof(uint32_t));
        ydes += NUM_BURST_READ;
        ysrc += NUM_BURST_READ;
    }
}
void hls_CopyMBLine_uv_32bit_const(uint32_t uvdes[MAX_W_PIX / 2 * 8 / 4],
                                   uint32_t uvsrc[MAX_W_PIX * MAX_H_PIX / 4 / 4],
                                   int num_read) {
    int num_loop = (num_read + NUM_BURST_READ - 1) >> LG2_NUM_BURST_READ; //
COPY_UV32:
    for (int line = 0; line < num_loop; line++) {
#pragma HLS LOOP_TRIPCOUNT min = 1920 / 2 * 8 / 4 / 64 max = 4096 / 2 * 8 / 4 / 64
#pragma HLS PIPELINE
        memcpy(uvdes, uvsrc, NUM_BURST_READ * sizeof(uint32_t));
        uvdes += NUM_BURST_READ;
        uvsrc += NUM_BURST_READ;
    }
}

void TopVp8_read_MB_32bit_const(ap_uint<LG2_MAX_W_PIX> width,   //      = p_info[4];  // = pic->width
                                ap_uint<LG2_MAX_W_PIX> height,  //      = p_info[5];  // = pic->height
                                ap_uint<LG2_MAX_NUM_MB_W> mb_w, // = p_info[2+2+2];///;
                                ap_uint<LG2_MAX_NUM_MB_H> mb_h, // = p_info[3+2+2];//;
                                int y_mb,
                                uint32_t buff_line_mb_y[MAX_W_PIX * 16 / 4],
                                uint32_t buff_line_mb_u[MAX_W_PIX * 4 / 4],
                                uint32_t buff_line_mb_v[MAX_W_PIX * 4 / 4],
                                // uint32_t  buff_line_mb_y2[MAX_W_PIX*16/4],
                                // uint32_t  buff_line_mb_u2[MAX_W_PIX*4/4],
                                // uint32_t  buff_line_mb_v2[MAX_W_PIX*4/4],
                                int stride_y,
                                int stride_uv,
                                // output
                                hls::stream<ap_uint<WD_PIX * 16> >* str_din_y,
                                hls::stream<ap_uint<WD_PIX * 16> >* str_din_uv) {
#pragma HLS INLINE OFF
TOP_READ32:
    for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
        ap_uint<WD_PIX * 16> ap_yuv_in_[24];
        ap_uint<WD_PIX * 16> ap_y_in_[16];
        ap_uint<WD_PIX * 16> ap_u_in_[4];
        ap_uint<WD_PIX * 16> ap_v_in_[4];
        hls_GetMB_parallel_32bit_const( // 599 cycyle dataflow
            buff_line_mb_y,             //[MAX_W_PIX*16],
            buff_line_mb_u,             //[MAX_W_PIX*4],
            buff_line_mb_v,             //[MAX_W_PIX*4],
            x_mb, y_mb, width, height, stride_y, stride_uv,
            ap_y_in_, //[16],
            ap_u_in_, //[4],
            ap_v_in_  //[4]
            );

        for (int i = 0; i < 16; i++) str_din_y->write(ap_y_in_[i]);
        for (int i = 0; i < 4; i++) str_din_uv->write(ap_u_in_[i]);
        for (int i = 0; i < 4; i++) str_din_uv->write(ap_v_in_[i]);
    }
}
void hls_GetMB_parallel_32bit_const(uint32_t ysrc_MBline[MAX_W_PIX * 16 / 4],
                                    uint32_t usrc_MBline[MAX_W_PIX * 4 / 4],
                                    uint32_t vsrc_MBline[MAX_W_PIX * 4 / 4],
                                    int x_mb,
                                    int y_mb,
                                    int width,
                                    int height,
                                    int stride_y,
                                    int stride_uv,
                                    ap_uint<WD_PIX * 16> ap_y_in_[16],
                                    ap_uint<WD_PIX * 16> ap_u_in_[4],
                                    ap_uint<WD_PIX * 16> ap_v_in_[4]) {
#pragma HLS INLINE OFF
    //#pragma HLS DATAFLOW
    hls_GetMB_y_32bit_const(ysrc_MBline, x_mb, y_mb, width, height, stride_y, ap_y_in_);
    hls_GetMB_uv_32bit_const(usrc_MBline, x_mb, y_mb, width, height, stride_uv, ap_u_in_);
    hls_GetMB_uv_32bit_const(vsrc_MBline, x_mb, y_mb, width, height, stride_uv, ap_v_in_);
}

static int MinSize32(int a, int b) {
    return (a < b) ? a : b;
};
void hls_GetMB_y_32bit_const(uint32_t src[MAX_W_PIX * 16 / 4],
                             int x_mb,
                             int y_mb,
                             int width,
                             int height,
                             int stride,
                             ap_uint<WD_PIX * 16> ap_y_in_[16]) {
    int x = x_mb;
    int y = y_mb;
    const int w = MinSize32(width - x * 16, 16);
    const int h = MinSize32(height - y * 16, 16);
    int off = (w - 1) % 4;
    uint32_t* ysrc = src + x_mb * 16 / 4; // 32bb
    // Two following variables create a slide window
    uint32_t rem_dat;
    uint32_t crt_dat;
    uint32_t w32;
    int addr8 = 0;
    for (int i = 0; i < 16; ++i) {
        int addr32 = (addr8) >> 2;
        int num_rem = 4 - (addr8 & 3);
        rem_dat = ysrc[addr32++];
    GET_Y32_IN:
        for (int base = 0; base < 4; base++) { // j= 0, 4, 8, 12; base = 0,1,2,3
#pragma HLS PIPELINE II = 1
            int j = base << 2;
            bool isAllIn = base < (w >> 2); //((j+3)<=(w-1));
            bool isAllOut = (j >= w);
            bool isOver = (num_rem + j) >= w;
            uint32_t rem_dat_2 = rem_dat;
            if (isOver)
                crt_dat = rem_dat;
            else
                crt_dat = rem_dat = ysrc[addr32++];

            w32 = get32bits_2_const(num_rem, rem_dat_2, crt_dat);
            ap_uint<32> tmp = GetEdgeImage(w32, off, isAllIn, isAllOut);
            VCT_GET(ap_y_in_[(i & 12) + base], i % 4, WD_PIX * 4) = tmp(31, 0);
        }
        if (i < (h - 1)) addr8 += stride;
    }
}

ap_uint<32> GetEdgeImage(ap_uint<32> org, int off, bool isAllIn, bool isAllOut) {
#pragma HLS PIPELINE
    ap_uint<32> tmp = org;
    ap_uint<8> edge = org(7 + off * 8, off * 8);
    if (isAllIn) {
        tmp = org;
    } else if (isAllOut) {
        tmp(7, 0) = edge;
        tmp(15, 8) = edge;
        tmp(23, 16) = edge;
        tmp(31, 24) = edge;
    } else { // at the edge
        if (off == 0) {
            tmp(15, 8) = edge;
            tmp(23, 16) = edge;
            tmp(31, 24) = edge;
        } else if (off == 1) {
            tmp(23, 16) = edge;
            tmp(31, 24) = edge;
        } else if (off == 2) {
            tmp(31, 24) = edge;
        } else {
            tmp(7, 0) = edge;
            tmp(15, 8) = edge;
            tmp(23, 16) = edge;
            tmp(31, 24) = edge;
        }
    }
    return tmp;
}

void hls_GetMB_uv_32bit_const(uint32_t src[MAX_W_PIX * 4 / 4],
                              int x_mb,
                              int y_mb,
                              int width,
                              int height,
                              int stride,
                              ap_uint<WD_PIX * 16> ap_uv_in_[4]) {
#pragma HLS INLINE OFF
    int x = x_mb;
    int y = y_mb;
    const int w = MinSize32(width - x * 16, 16);
    const int h = MinSize32(height - y * 16, 16);
    const int uv_w = (w + 1) >> 1;
    const int uv_h = (h + 1) >> 1;
    int off = (uv_w - 1) % 4;
    uint32_t* uvsrc = src + x_mb * 8 / 4; // 32bb
    uint32_t rem_dat;
    uint32_t crt_dat;
    uint32_t w32;
    int addr8 = 0;
    int rem_num = 0;

    for (int i = 0; i < 8; ++i) {
        int addr32 = (addr8 + 0) / 4;
        int num_rem = 4 - (addr8 & 3);
        rem_dat = uvsrc[addr32++];
    GET_UV32_IN:
        for (int base = 0; base < 2; base++) {
#pragma HLS PIPELINE
            int j = base * 4;
            bool isAllIn = ((j + 3) <= (uv_w - 1));
            bool isAllOut = (j > (uv_w - 1));
            bool isOver = (num_rem + j) >= uv_w;
            { // base = 0, 1
                uint32_t rem_dat_2 = rem_dat;
                if (isOver)
                    crt_dat = rem_dat;
                else
                    crt_dat = rem_dat = uvsrc[addr32++];
                w32 = get32bits_2_const(num_rem, rem_dat_2, crt_dat);
                ap_uint<32> tmp = GetEdgeImage(w32, off, isAllIn, isAllOut);
                VCT_GET(ap_uv_in_[(i & 4) / 2 + base], i % 4, WD_PIX * 4) = tmp(31, 0);
            }
        }
        if (i < (uv_h - 1)) addr8 += stride;
    }
}

ap_uint<32> get32bits_2_const(ap_uint<3> n_rem, ap_uint<32> rem, ap_uint<32> crt) {
#pragma HLS PIPELINE
    if (n_rem == 4) return rem;
    if (n_rem == 0) return crt;
    ap_uint<32> tmp;
    ap_uint<5> bits = 8 * n_rem;
    rem = rem >> (32 - bits);
    tmp = rem;
    crt = crt << bits;
    tmp(31, bits) = crt(31, bits);
    return tmp;
}
//////////======================================================================/////////////////////////////
//////////=====================   TopVp8_compute      ==========================/////////////////////////////
//////////======================================================================/////////////////////////////
// TopVp8_compute_NoOut===========================================================================/
//(Note, following names of functions may has already changed but not updated)
//-Intraprediction_mb_syn_str2
//--hls_LoadPre_out
//--hls_LoadPre_mode
//--Pickup_dataflow3
//---Pickup_Y44
//----hls_p4_test
//----hls_GetCost
//----hls_channel_p44
//-----hls_FTransform
//-----hls_QuantizeBlock
//-----hls_ITransformOne
//-----hls_SSE4X4
//-----hls_Disto4x4
//-----hls_fast_cost
//---Pickup_Y16
//----hls_channel_p16
//-----hls_p16_test
//-----hls_FTransform
//-----hls_FTransformWHT
//-----hls_QuantizeBlockWHT
//-----hls_IFTransformWHT
//-----hls_QuantizeBlock
//-----hls_ITransformOne
//-----hls_SSE4X4
//-----hls_Disto4x4
//-----hls_fast_cost
//-----hls_ca_score
//---Pickup_UV
//----hls_p8_test
//----hls_channel_uv_8
//-----hls_p8_test
//-----hls_FTransform
//-----hls_QuantizeBlock
//-----hls_ITransformOne
//-----hls_fast_cost
//-----hls_ca_score
//--hls_SetBestAs4_mode
void TopVp8_compute(ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                    ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                    hls::stream<ap_uint<WD_PIX * 16> >* str_din_y,
                    hls::stream<ap_uint<WD_PIX * 16> >* str_din_uv,
                    ap_uint<WD_LMD> lambda_p16,
                    ap_uint<WD_LMD> lambda_p44,
                    ap_uint<WD_LMD> tlambda,
                    ap_uint<WD_LMD> lambda_uv,
                    ap_uint<WD_LMD> tlambda_m,
                    hls_QMatrix hls_qm1,
                    hls_QMatrix hls_qm2,
                    hls_QMatrix hls_qm_uv,
                    ap_int<WD_sharpen * 16> ap_sharpen,
                    ap_int<WD_sharpen * 16> ap_sharpen_uv,
                    hls::stream<ap_uint<WD_PIX * 16> >* str_out,
                    hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                    hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                    hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                    hls::stream<ap_int<64> >* str_pred,
                    hls::stream<ap_int<6> >* str_ret) {
#pragma HLS interface ap_stable port = lambda_p16
#pragma HLS interface ap_stable port = lambda_p44
#pragma HLS interface ap_stable port = tlambda
#pragma HLS interface ap_stable port = lambda_uv
#pragma HLS interface ap_stable port = tlambda_m

    for (int y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
        for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
            // Intraprediction_mb_syn_str2(// &it,
            Intraprediction_mb_syn_str2_widen( // &it,
                x_mb, y_mb, mb_w, str_din_y, str_din_uv, lambda_p16, lambda_p44, tlambda, lambda_uv, tlambda_m, hls_qm1,
                hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv, str_out, str_level_dc, str_level_y, str_level_uv,
                str_pred, str_ret);
        }
    }
}

void TopVp8_compute_NoOut(ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                          ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                          hls::stream<ap_uint<WD_PIX * 16> >* str_din_y,
                          hls::stream<ap_uint<WD_PIX * 16> >* str_din_uv,
                          ap_uint<WD_LMD> lambda_p16,
                          ap_uint<WD_LMD> lambda_p44,
                          ap_uint<WD_LMD> tlambda,
                          ap_uint<WD_LMD> lambda_uv,
                          ap_uint<WD_LMD> tlambda_m,
                          hls_QMatrix hls_qm1,
                          hls_QMatrix hls_qm2,
                          hls_QMatrix hls_qm_uv,
                          ap_int<WD_sharpen * 16> ap_sharpen,
                          ap_int<WD_sharpen * 16> ap_sharpen_uv,
                          hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                          hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                          hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                          hls::stream<ap_int<64> >* str_pred,
                          hls::stream<ap_int<6> >* str_ret) {
#pragma HLS interface ap_stable port = lambda_p16
#pragma HLS interface ap_stable port = lambda_p44
#pragma HLS interface ap_stable port = tlambda
#pragma HLS interface ap_stable port = lambda_uv
#pragma HLS interface ap_stable port = tlambda_m

VP8_COMPUTE_NOOUT:
    for (int y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
        for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
            // Intraprediction_mb_syn_str2(// &it,
            Intraprediction_mb_syn_str2_widen_NoOut( // &it,
                x_mb, y_mb, mb_w, str_din_y, str_din_uv, lambda_p16, lambda_p44, tlambda, lambda_uv, tlambda_m, hls_qm1,
                hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv, str_level_dc, str_level_y, str_level_uv, str_pred,
                str_ret);
        }
    }
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

void Intraprediction_mb_syn_str2_widen(ap_uint<LG2_MAX_NUM_MB_W> x_mb,
                                       ap_uint<LG2_MAX_NUM_MB_W> y_mb,
                                       ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                                       hls::stream<ap_uint<WD_PIX * 16> >* str_ap_yuv_in_y,
                                       hls::stream<ap_uint<WD_PIX * 16> >* str_ap_yuv_in_uv,
                                       ap_uint<WD_LMD> lambda_p16,
                                       ap_uint<WD_LMD> lambda_p44,
                                       ap_uint<WD_LMD> tlambda,
                                       ap_uint<WD_LMD> lambda_uv,
                                       ap_uint<WD_LMD> tlambda_m,
                                       hls_QMatrix hls_qm1,
                                       hls_QMatrix hls_qm2,
                                       hls_QMatrix hls_qm_uv,
                                       ap_int<WD_sharpen * 16> ap_sharpen,
                                       ap_int<WD_sharpen * 16> ap_sharpen_uv,
                                       hls::stream<ap_uint<WD_PIX * 16> >* str_out,
                                       hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                                       hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                                       hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                                       hls::stream<ap_int<64> >* str_pred,
                                       hls::stream<ap_int<6> >* str_ret) {
    ap_uint<WD_PIX * 16> ap_y_in_y44[16 + 0];
    ap_uint<WD_PIX * 16> ap_y_in_y16[16 + 0];
    ap_uint<WD_PIX * 16> ap_uv_in_[8];
READ_AP_Y_WIDEN:
    for (int i = 0; i < 16; i++) {
#pragma HLS PIPELINE
        ap_y_in_y44[i] = str_ap_yuv_in_y->read();
        ap_y_in_y16[i] = ap_y_in_y44[i];
        if (i & 1) ap_uv_in_[i / 2] = str_ap_yuv_in_uv->read();
    }

    static ap_uint<WD_PIX * 4> static_ap_y_top_[MAX_NUM_MB_W * 4];
    //#pragma HLS RESOURCE  variable=static_ap_y_top_ core=RAM_2P_LUTRAM
    static ap_uint<WD_PIX * 4> static_ap_uv_top_[MAX_NUM_MB_W * 4];
    //#pragma HLS RESOURCE  variable=static_ap_uv_top_ core=RAM_2P_LUTRAM
    static ap_uint<WD_PIX * 4> static_ap_y_left_[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_ complete dim=1
    static ap_uint<WD_PIX * 4> static_ap_uv_left_[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_uv_left_ complete dim=1
    static ap_uint<WD_PIX * 4> static_ap_y_top_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_top_c complete dim=1
    ap_uint<WD_PIX * 4> ap_y_left_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_c complete dim=1
    ap_uint<WD_PIX * 4> ap_y4_top_c[4];
#pragma HLS ARRAY_PARTITION variable = ap_y4_top_c complete dim = 1
    ap_uint<WD_PIX * 4> ap_y4_left_c[4];
#pragma HLS ARRAY_PARTITION variable = ap_y4_left_c complete dim = 1
    static ap_uint<WD_PIX * 4> static_ap_uv_top_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_uv_top_c complete dim=1
    ap_uint<WD_PIX * 4> ap_uv_left_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_uv_left_c complete dim=1
    // Variables for mode
    static ap_uint<WD_MODE> static_ap_y_top_mode[MAX_NUM_MB_W * 4];
    // storage for past, updated when bottoms are available
    static ap_uint<WD_MODE> static_ap_y_left_mode[4]; // storage for past, updated when right are available
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_mode complete dim=1
    ap_uint<WD_MODE> ap_y_top_c_mode[4]; // at beginning, default is DC
    //#pragma HLS ARRAY_PARTITION variable=ap_y_top_c_mode complete dim=1
    ap_uint<WD_MODE> ap_y_left_c_mode[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_c_mode complete dim=1
    ap_uint<WD_MODE> ap_y4_top_c_mode[16];
#pragma HLS ARRAY_PARTITION variable = ap_y4_top_c_mode complete dim = 1
    ap_uint<WD_MODE> ap_y16_mode_c;
    ap_uint<WD_MODE> ap_uv_mode_c;
    ap_uint<WD_MODE> ap_y_m_mode;

    // Variables for rd and nz
    ap_uint<25> ap_nz_all = 0;
    str_rd rd_y16_cb[2];
    str_rd rd_uv_cb[2];
    // str_rd_i4       rd_y4_acc;
    ap_uint<25> rd_y4_acc_nz = 0;
    ap_uint<WD_RD_SCORE + 4> rd_y4_acc_score = -1;

    ap_uint<1> istop = (y_mb == 0);
    ap_uint<1> isleft = (x_mb == 0);
    ap_uint<1> isright = (x_mb == mb_w - 1);

    ap_int<WD_LEVEL * 16> ap_y_level_cb[2][17];
    ap_int<WD_LEVEL * 16> ap_y16dc_level_cb[2];
    ap_int<WD_LEVEL * 16> ap_y4_level_cb[16];
#pragma HLS ARRAY_PARTITION variable = ap_y4_level_cb complete dim = 1
    ap_int<WD_LEVEL * 16> ap_uv_level_cb[2][16];
    // it_o->hls_LoadPre( ap_y_top_, ap_uv_top_, x_mb, y_mb, mb_w);
    ap_uint<WD_PIX * 16> ap_y_out_cb[2][17];
    ap_uint<WD_PIX * 16> ap_y4_out_cb[16];
#pragma HLS ARRAY_PARTITION variable = ap_y4_out_cb complete dim = 1
    ap_uint<WD_PIX * 16> ap_uv_out_cb[2][17];
    ap_uint<WD_PIX * 4> ap_y4_topright_c;
    ap_uint<WD_PIX> ap_y_m;
    ap_uint<WD_PIX> ap_u_m;
    ap_uint<WD_PIX> ap_v_m;

    hls_LoadPre_out_widen(&ap_y_m, &ap_u_m, &ap_v_m,
                          static_ap_y_top_c,  //[4],
                          ap_y4_top_c,        //[4],
                          static_ap_uv_top_c, //[4],
                          &ap_y4_topright_c,
                          ap_y_left_c,        //[4],
                          ap_y4_left_c,       //[4],
                          ap_uv_left_c,       //[4],
                          static_ap_y_left_,  //[4],
                          static_ap_uv_left_, //[4],
                          static_ap_y_top_, static_ap_uv_top_, x_mb, y_mb, mb_w);

    ap_uint<WD_PIX * 4> ap_y_top_c_y44[16];
#pragma HLS ARRAY_PARTITION variable = ap_y_top_c_y44 complete dim = 1

    ap_uint<WD_PIX * 4> ap_y_top_c_y16[4];
#pragma HLS ARRAY_PARTITION variable = ap_y_top_c_y16 complete dim = 1

    ap_uint<WD_PIX * 4> ap_y_left_c_y44[16];
#pragma HLS ARRAY_PARTITION variable = ap_y_left_c_y44 complete dim = 1

    ap_uint<WD_PIX * 4> ap_y_left_c_y16[4];
#pragma HLS ARRAY_PARTITION variable = ap_y_left_c_y16 complete dim = 1

    for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
        ap_y_top_c_y44[i] = static_ap_y_top_c[i];
        ap_y_top_c_y16[i] = static_ap_y_top_c[i];
        ap_y_left_c_y44[i << 2] = ap_y_left_c[i];
        ap_y_left_c_y16[i] = ap_y_left_c[i];
        ;
    }
    hls_LoadPre_mode_widen(static_ap_y_top_mode, static_ap_y_left_mode, ap_y_top_c_mode, ap_y4_top_c_mode,
                           ap_y_left_c_mode, &ap_y_m_mode, x_mb, y_mb, mb_w);
    // it_m->hls_LoadPre( x_mb, y_mb, mb_w);
    // rd_y4_acc.init();
    int mode_p16;
    int mode_uv;
    // for 4x4
    int n_sb;
    int mode;
    ap_uint<1> b_uv = 0;
    ap_uint<2> b_y = 0;
    /*******************************/
    /*       Pickup Y44             */
    /*******************************/
    Pickup_dataflow3_widen(
        // Parameters unParameters changed for one picture/segment
        tlambda,       //                  :ap_uint<WD_LMD>         I__
        tlambda_m,     //                 ap_uint<WD_LMD>         I__
        lambda_p44,    //                ap_uint<WD_LMD>        I__
        lambda_p16,    //                ap_uint<WD_LMD>         I__
        lambda_uv,     //                 ap_uint<WD_LMD>         I__
        hls_qm1,       // y44,y16            hls_QMatrix             I__
        hls_qm2,       // y16                hls_QMatrix             I__
        hls_qm_uv,     //                 hls_QMatrix             I__
        ap_sharpen,    //                ap_int<WD_sharpen*16>   I__
        ap_sharpen_uv, //             ap_int<WD_sharpen*16>   I__
        // Parameters changed for each MB
        ap_y_in_y44, //[16],//         ap_uint<WD_PIX*16>      I__
        ap_y_in_y16, //[16],//         ap_uint<WD_PIX*16>      I__
        ap_uv_in_,   //[8]
        istop,       //                     ap_uint<1>             I__
        isleft,      //                    ap_uint<1>             I__
        isright,     //                   ap_uint<1>             I__
        // image context
        ap_y4_top_c,        // ap_y_top_c_y44,//[16],//          ap_uint<WD_PIX*4>       I__
        ap_y_top_c_y16,     // ap_y_top_c,//[4],//          ap_uint<WD_PIX*4>       I__
        ap_y4_left_c,       // ap_y_left_c_y44,//[16],//         ap_uint<WD_PIX*4>       I__
        ap_y_left_c_y16,    // ap_y_left_c,//[4],//         ap_uint<WD_PIX*4>       I__
        static_ap_uv_top_c, //[4],//         ap_uint<WD_PIX*4>       I__
        ap_uv_left_c,       //[4],//        ap_uint<WD_PIX*4>       I__
        ap_y_m,             //                    ap_uint<WD_PIX>         I__
        ap_u_m,             //                    ap_uint<WD_PIX>         I__
        ap_v_m,             //                    ap_uint<WD_PIX>         I__
        ap_y4_topright_c,   //          ap_uint<WD_PIX*4>       I__
                            // mode context
        ap_y_top_c_mode,    //[4],//     ap_uint<WD_MODE>        I__
        ap_y_left_c_mode,   //[4],//    ap_uint<WD_MODE>        I__
                            // OUTPUT
        ap_y4_out_cb,       //[16],//       ap_uint<WD_PIX*16>      O__
        ap_y_out_cb,        //[2][17],//     ap_uint<WD_PIX*16>      O__
        ap_uv_out_cb,       //[2][17],//    ap_uint<WD_PIX*16>      O__
        ap_y4_level_cb,     //[17],//     ap_int<WD_LEVEL*16>     O__
        ap_y_level_cb,      //[2][17],//   ap_int<WD_LEVEL*16>     O__
        ap_y16dc_level_cb,  //[2] //   ap_int<WD_LEVEL*16>     O__
        ap_uv_level_cb,     //[2][16],//  ap_int<WD_LEVEL*16>     O__
        //&rd_y4_acc,//                str_rd_i4*              OP_
        &rd_y4_acc_score, &rd_y4_acc_nz,
        rd_y16_cb,        //[2],//           str_rd                  O__
        rd_uv_cb,         //[2],//            str_rd                  O__
        ap_y4_top_c_mode, //[16],//   ap_uint<WD_MODE>        IO_
        &ap_y16_mode_c,   //            ap_uint<WD_MODE>*       OP_
        &ap_uv_mode_c,    //             ap_uint<WD_MODE>*       OP_
        &b_uv,            //                     ap_uint<1>*             OP_
        &b_y              //                       ap_uint<2>*             OP_
        );
    ap_uint<6> ret;
    ap_uint<WD_MODE * 16> ap_y_mode_b;
    // ret[5]==1;
    ret[5] = ret[5] & (rd_uv_cb[b_uv].nz(23, 16) == 0);
    // ap_nz_all(23,16)    = rd_uv_cb[b_uv].nz(23,16);
    /**********************************************/
    /* Pickup the best mode for y
     * Set nz, level, out, preds, mb_->type       */
    /**********************************************/
    if (rd_y4_acc_score < rd_y16_cb[(1 & b_y)].score) {
        int x_sb_w = (int)x_mb << 2;
        ret[5] = ret[5] & (rd_y4_acc_nz(15, 0) == 0);
        hls_SetBestAs4_mode_widen(static_ap_y_top_mode, static_ap_y_left_mode, ap_y4_top_c_mode, ap_y_left_c_mode,
                                  &ap_y_mode_b, x_sb_w);
        b_y = 2;
        hls_StoreTopLeft_y(static_ap_y_top_, static_ap_y_left_, ap_y4_out_cb, x_mb);
        for (int n = 0; n < 16; n++) {
#pragma HLS UNROLL
            str_out->write(ap_y4_out_cb[n]);
            str_level_y->write(ap_y4_level_cb[n]);
        }
    } else {
        int x_sb_w = (int)x_mb << 2;
        ret[5] = ret[5] & (rd_y16_cb[(1 & b_y)].nz(15, 0) == 0);
        ret[5] = ret[5] & (rd_y16_cb[(1 & b_y)].nz[24] == 0);
        hls_SetBestAs16_mode_widen(static_ap_y_top_mode, static_ap_y_left_mode, ap_y16_mode_c, &ap_y_mode_b, x_sb_w);
        b_y &= 1;
        hls_StoreTopLeft_y(static_ap_y_top_, static_ap_y_left_, ap_y_out_cb[b_y], x_mb);
        for (int n = 0; n < 16; n++) {
#pragma HLS UNROLL
            str_out->write(ap_y_out_cb[b_y][n]);
            str_level_y->write(ap_y_level_cb[b_y][n]);
        }
    }
    hls_StoreTopLeft_uv(static_ap_uv_top_, static_ap_uv_left_, ap_uv_out_cb[b_uv], x_mb);
    str_level_dc->write(ap_y16dc_level_cb[b_y & 1]); //[16]);
    // str_level_dc->write(ap_y_level_cb[b_y&1][16]);
    for (int n = 0; n < 8; n += 1) {
#pragma HLS UNROLL
        str_out->write(ap_uv_out_cb[b_uv][n]); // b[n]);//,it.yuv_out_ + U_OFF_ENC+ VP8ScanUV[n],32);
        str_level_uv->write(ap_uv_level_cb[b_uv][n]);
    }

    /**********************************************/
    /* write return value                         */
    /**********************************************/
    str_pred->write(ap_y_mode_b);
    ret(3, 0) = ap_uv_mode_c(1, 0);
    ret(4, 4) = ~b_y(1, 1); // it.mb_->type_ = 0;
    str_ret->write(ret);
}

void Intraprediction_mb_syn_str2_widen_NoOut(ap_uint<LG2_MAX_NUM_MB_W> x_mb,
                                             ap_uint<LG2_MAX_NUM_MB_W> y_mb,
                                             ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                                             hls::stream<ap_uint<WD_PIX * 16> >* str_ap_yuv_in_y,
                                             hls::stream<ap_uint<WD_PIX * 16> >* str_ap_yuv_in_uv,
                                             ap_uint<WD_LMD> lambda_p16,
                                             ap_uint<WD_LMD> lambda_p44,
                                             ap_uint<WD_LMD> tlambda,
                                             ap_uint<WD_LMD> lambda_uv,
                                             ap_uint<WD_LMD> tlambda_m,
                                             hls_QMatrix hls_qm1,
                                             hls_QMatrix hls_qm2,
                                             hls_QMatrix hls_qm_uv,
                                             ap_int<WD_sharpen * 16> ap_sharpen,
                                             ap_int<WD_sharpen * 16> ap_sharpen_uv,
                                             // NoOut: hls::stream<ap_uint<WD_PIX * 16> >* str_out,
                                             hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                                             hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                                             hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                                             hls::stream<ap_int<64> >* str_pred,
                                             hls::stream<ap_int<6> >* str_ret) {
    ap_uint<WD_PIX * 16> ap_y_in_y44[16 + 0];
    ap_uint<WD_PIX * 16> ap_y_in_y16[16 + 0];
    ap_uint<WD_PIX * 16> ap_uv_in_[8];

READ_AP_UV_WIDEN:
    for (int i = 0; i < 16; i++) {
#pragma HLS PIPELINE
        ap_y_in_y44[i] = str_ap_yuv_in_y->read();
        ap_y_in_y16[i] = ap_y_in_y44[i];
        if (i & 1) ap_uv_in_[i / 2] = str_ap_yuv_in_uv->read();
    }

    static ap_uint<WD_PIX * 4> static_ap_y_top_[MAX_NUM_MB_W * 4];
#pragma HLS RESOURCE variable = static_ap_y_top_ core = RAM_2P_LUTRAM
    static ap_uint<WD_PIX * 4> static_ap_uv_top_[MAX_NUM_MB_W * 4];
#pragma HLS RESOURCE variable = static_ap_uv_top_ core = RAM_2P_LUTRAM
    static ap_uint<WD_PIX * 4> static_ap_y_left_[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_ complete dim=1
    static ap_uint<WD_PIX * 4> static_ap_uv_left_[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_uv_left_ complete dim=1
    static ap_uint<WD_PIX * 4> static_ap_y_top_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_top_c complete dim=1
    ap_uint<WD_PIX * 4> ap_y_left_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_c complete dim=1
    ap_uint<WD_PIX * 4> ap_y4_top_c[4];
#pragma HLS ARRAY_PARTITION variable = ap_y4_top_c complete dim = 1
    ap_uint<WD_PIX * 4> ap_y4_left_c[4];
#pragma HLS ARRAY_PARTITION variable = ap_y4_left_c complete dim = 1
    static ap_uint<WD_PIX * 4> static_ap_uv_top_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_uv_top_c complete dim=1
    ap_uint<WD_PIX * 4> ap_uv_left_c[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_uv_left_c complete dim=1
    // Variables for mode
    static ap_uint<WD_MODE> static_ap_y_top_mode[MAX_NUM_MB_W * 4];
    // storage for past, updated when bottoms are available
    static ap_uint<WD_MODE> static_ap_y_left_mode[4]; // storage for past, updated when right are available
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_mode complete dim=1
    ap_uint<WD_MODE> ap_y_top_c_mode[4]; // at beginning, default is DC
    //#pragma HLS ARRAY_PARTITION variable=ap_y_top_c_mode complete dim=1
    ap_uint<WD_MODE> ap_y_left_c_mode[4];
    //#pragma HLS ARRAY_PARTITION variable=ap_y_left_c_mode complete dim=1
    ap_uint<WD_MODE> ap_y4_top_c_mode[16];
#pragma HLS ARRAY_PARTITION variable = ap_y4_top_c_mode complete dim = 1
    ap_uint<WD_MODE> ap_y16_mode_c;
    ap_uint<WD_MODE> ap_uv_mode_c;
    ap_uint<WD_MODE> ap_y_m_mode;

    // Variables for rd and nz
    ap_uint<25> ap_nz_all = 0;
    str_rd rd_y16_cb[2];
    str_rd rd_uv_cb[2];
    // str_rd_i4       rd_y4_acc;
    ap_uint<25> rd_y4_acc_nz = 0;
    ap_uint<WD_RD_SCORE + 4> rd_y4_acc_score = -1;

    ap_uint<1> istop = (y_mb == 0);
    ap_uint<1> isleft = (x_mb == 0);
    ap_uint<1> isright = (x_mb == mb_w - 1);

    ap_int<WD_LEVEL * 16> ap_y_level_cb[2][17];
    ap_int<WD_LEVEL * 16> ap_y16dc_level_cb[2];
    ap_int<WD_LEVEL * 16> ap_y4_level_cb[16];
#pragma HLS ARRAY_PARTITION variable = ap_y4_level_cb complete dim = 1
    ap_int<WD_LEVEL * 16> ap_uv_level_cb[2][16];
    // it_o->hls_LoadPre( ap_y_top_, ap_uv_top_, x_mb, y_mb, mb_w);
    ap_uint<WD_PIX * 16> ap_y_out_cb[2][17];
    ap_uint<WD_PIX * 16> ap_y4_out_cb[16];
#pragma HLS ARRAY_PARTITION variable = ap_y4_out_cb complete dim = 1
    ap_uint<WD_PIX * 16> ap_uv_out_cb[2][17];
    ap_uint<WD_PIX * 4> ap_y4_topright_c;
    ap_uint<WD_PIX> ap_y_m;
    ap_uint<WD_PIX> ap_u_m;
    ap_uint<WD_PIX> ap_v_m;

    hls_LoadPre_out_widen(&ap_y_m, &ap_u_m, &ap_v_m,
                          static_ap_y_top_c,  //[4],
                          ap_y4_top_c,        //[4],
                          static_ap_uv_top_c, //[4],
                          &ap_y4_topright_c,
                          ap_y_left_c,        //[4],
                          ap_y4_left_c,       //[4],
                          ap_uv_left_c,       //[4],
                          static_ap_y_left_,  //[4],
                          static_ap_uv_left_, //[4],
                          static_ap_y_top_, static_ap_uv_top_, x_mb, y_mb, mb_w);

    ap_uint<WD_PIX * 4> ap_y_top_c_y44[16];
#pragma HLS ARRAY_PARTITION variable = ap_y_top_c_y44 complete dim = 1

    ap_uint<WD_PIX * 4> ap_y_top_c_y16[4];
#pragma HLS ARRAY_PARTITION variable = ap_y_top_c_y16 complete dim = 1

    ap_uint<WD_PIX * 4> ap_y_left_c_y44[16];
#pragma HLS ARRAY_PARTITION variable = ap_y_left_c_y44 complete dim = 1

    ap_uint<WD_PIX * 4> ap_y_left_c_y16[4];
#pragma HLS ARRAY_PARTITION variable = ap_y_left_c_y16 complete dim = 1

    for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
        ap_y_top_c_y44[i] = static_ap_y_top_c[i];
        ap_y_top_c_y16[i] = static_ap_y_top_c[i];
        ap_y_left_c_y44[i << 2] = ap_y_left_c[i];
        ap_y_left_c_y16[i] = ap_y_left_c[i];
        ;
    }
    hls_LoadPre_mode_widen(static_ap_y_top_mode, static_ap_y_left_mode, ap_y_top_c_mode, ap_y4_top_c_mode,
                           ap_y_left_c_mode, &ap_y_m_mode, x_mb, y_mb, mb_w);
    // it_m->hls_LoadPre( x_mb, y_mb, mb_w);
    // rd_y4_acc.init();
    int mode_p16;
    int mode_uv;
    // for 4x4
    int n_sb;
    int mode;
    ap_uint<1> b_uv = 0;
    ap_uint<2> b_y = 0;
    /*******************************/
    /*       Pickup Y44             */
    /*******************************/
    Pickup_dataflow3_widen(
        // Parameters unParameters changed for one picture/segment
        tlambda,       //                  :ap_uint<WD_LMD>         I__
        tlambda_m,     //                 ap_uint<WD_LMD>         I__
        lambda_p44,    //                ap_uint<WD_LMD>        I__
        lambda_p16,    //                ap_uint<WD_LMD>         I__
        lambda_uv,     //                 ap_uint<WD_LMD>         I__
        hls_qm1,       // y44,y16            hls_QMatrix             I__
        hls_qm2,       // y16                hls_QMatrix             I__
        hls_qm_uv,     //                 hls_QMatrix             I__
        ap_sharpen,    //                ap_int<WD_sharpen*16>   I__
        ap_sharpen_uv, //             ap_int<WD_sharpen*16>   I__
        // Parameters changed for each MB
        ap_y_in_y44, //[16],//         ap_uint<WD_PIX*16>      I__
        ap_y_in_y16, //[16],//         ap_uint<WD_PIX*16>      I__
        ap_uv_in_,   //[8]
        istop,       //                     ap_uint<1>             I__
        isleft,      //                    ap_uint<1>             I__
        isright,     //                   ap_uint<1>             I__
        // image context
        ap_y4_top_c,        // ap_y_top_c_y44,//[16],//          ap_uint<WD_PIX*4>       I__
        ap_y_top_c_y16,     // ap_y_top_c,//[4],//          ap_uint<WD_PIX*4>       I__
        ap_y4_left_c,       // ap_y_left_c_y44,//[16],//         ap_uint<WD_PIX*4>       I__
        ap_y_left_c_y16,    // ap_y_left_c,//[4],//         ap_uint<WD_PIX*4>       I__
        static_ap_uv_top_c, //[4],//         ap_uint<WD_PIX*4>       I__
        ap_uv_left_c,       //[4],//        ap_uint<WD_PIX*4>       I__
        ap_y_m,             //                    ap_uint<WD_PIX>         I__
        ap_u_m,             //                    ap_uint<WD_PIX>         I__
        ap_v_m,             //                    ap_uint<WD_PIX>         I__
        ap_y4_topright_c,   //          ap_uint<WD_PIX*4>       I__
                            // mode context
        ap_y_top_c_mode,    //[4],//     ap_uint<WD_MODE>        I__
        ap_y_left_c_mode,   //[4],//    ap_uint<WD_MODE>        I__
                            // OUTPUT
        ap_y4_out_cb,       //[16],//       ap_uint<WD_PIX*16>      O__
        ap_y_out_cb,        //[2][17],//     ap_uint<WD_PIX*16>      O__
        ap_uv_out_cb,       //[2][17],//    ap_uint<WD_PIX*16>      O__
        ap_y4_level_cb,     //[17],//     ap_int<WD_LEVEL*16>     O__
        ap_y_level_cb,      //[2][17],//   ap_int<WD_LEVEL*16>     O__
        ap_y16dc_level_cb,  //[2] //   ap_int<WD_LEVEL*16>     O__
        ap_uv_level_cb,     //[2][16],//  ap_int<WD_LEVEL*16>     O__
        //&rd_y4_acc,//                str_rd_i4*              OP_
        &rd_y4_acc_score, &rd_y4_acc_nz,
        rd_y16_cb,        //[2],//           str_rd                  O__
        rd_uv_cb,         //[2],//            str_rd                  O__
        ap_y4_top_c_mode, //[16],//   ap_uint<WD_MODE>        IO_
        &ap_y16_mode_c,   //            ap_uint<WD_MODE>*       OP_
        &ap_uv_mode_c,    //             ap_uint<WD_MODE>*       OP_
        &b_uv,            //                     ap_uint<1>*             OP_
        &b_y              //                       ap_uint<2>*             OP_
        );
    ap_uint<6> ret;
    ap_uint<WD_MODE * 16> ap_y_mode_b;
    // ret[5]==1;
    ret[5] = ret[5] & (rd_uv_cb[b_uv].nz(23, 16) == 0);
    // ap_nz_all(23,16)    = rd_uv_cb[b_uv].nz(23,16);
    /**********************************************/
    /* Pickup the best mode for y
     * Set nz, level, out, preds, mb_->type       */
    /**********************************************/
    if (rd_y4_acc_score < rd_y16_cb[(1 & b_y)].score) {
        int x_sb_w = (int)x_mb << 2;
        ret[5] = ret[5] & (rd_y4_acc_nz(15, 0) == 0);
        hls_SetBestAs4_mode_widen(static_ap_y_top_mode, static_ap_y_left_mode, ap_y4_top_c_mode, ap_y_left_c_mode,
                                  &ap_y_mode_b, x_sb_w);
        b_y = 2;
        hls_StoreTopLeft_y(static_ap_y_top_, static_ap_y_left_, ap_y4_out_cb, x_mb);
        for (int n = 0; n < 16; n++) {
#pragma HLS UNROLL
            // str_out->write(ap_y4_out_cb[n]);
            str_level_y->write(ap_y4_level_cb[n]);
        }
    } else {
        int x_sb_w = (int)x_mb << 2;
        ret[5] = ret[5] & (rd_y16_cb[(1 & b_y)].nz(15, 0) == 0);
        ret[5] = ret[5] & (rd_y16_cb[(1 & b_y)].nz[24] == 0);
        hls_SetBestAs16_mode_widen(static_ap_y_top_mode, static_ap_y_left_mode, ap_y16_mode_c, &ap_y_mode_b, x_sb_w);
        b_y &= 1;
        hls_StoreTopLeft_y(static_ap_y_top_, static_ap_y_left_, ap_y_out_cb[b_y], x_mb);
        for (int n = 0; n < 16; n++) {
#pragma HLS UNROLL
            // str_out->write(ap_y_out_cb[b_y][n]);
            str_level_y->write(ap_y_level_cb[b_y][n]);
        }
    }
    hls_StoreTopLeft_uv(static_ap_uv_top_, static_ap_uv_left_, ap_uv_out_cb[b_uv], x_mb);
    str_level_dc->write(ap_y16dc_level_cb[b_y & 1]); //[16]);
    // str_level_dc->write(ap_y_level_cb[b_y&1][16]);
    for (int n = 0; n < 8; n += 1) {
#pragma HLS UNROLL
        // str_out->write(ap_uv_out_cb[b_uv][n]);//b[n]);//,it.yuv_out_ + U_OFF_ENC+ VP8ScanUV[n],32);
        str_level_uv->write(ap_uv_level_cb[b_uv][n]);
    }

    /**********************************************/
    /* write return value                         */
    /**********************************************/
    str_pred->write(ap_y_mode_b);
    ret(3, 0) = ap_uv_mode_c(1, 0);
    ret(4, 4) = ~b_y(1, 1); // it.mb_->type_ = 0;
    str_ret->write(ret);
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void hls_StoreTopLeft_uv(ap_uint<WD_PIX * 4> ap_uv_top_[MAX_NUM_MB_W * 4],
                         ap_uint<WD_PIX * 4> ap_uv_left_[4],
                         ap_uint<WD_PIX * 16> ap_uv_out_cb[8],
                         ap_uint<LG2_MAX_NUM_MB_W> x_mb) {
    for (int i = 0; i < 4; i++) {
        int x_sb_w = (int)x_mb << 2;
#pragma HLS UNROLL
        VCT_GET(ap_uv_left_[i], 0, WD_PIX) = SB_GET(ap_uv_out_cb[i * 2 + 1], 0, 3, WD_PIX);
        VCT_GET(ap_uv_left_[i], 1, WD_PIX) = SB_GET(ap_uv_out_cb[i * 2 + 1], 1, 3, WD_PIX);
        VCT_GET(ap_uv_left_[i], 2, WD_PIX) = SB_GET(ap_uv_out_cb[i * 2 + 1], 2, 3, WD_PIX);
        VCT_GET(ap_uv_left_[i], 3, WD_PIX) = SB_GET(ap_uv_out_cb[i * 2 + 1], 3, 3, WD_PIX);
        VCT_GET(ap_uv_top_[x_sb_w + 0], i, WD_PIX) = SB_GET(ap_uv_out_cb[2], 3, i, WD_PIX);
        VCT_GET(ap_uv_top_[x_sb_w + 1], i, WD_PIX) = SB_GET(ap_uv_out_cb[3], 3, i, WD_PIX);
        VCT_GET(ap_uv_top_[x_sb_w + 2], i, WD_PIX) = SB_GET(ap_uv_out_cb[6], 3, i, WD_PIX);
        VCT_GET(ap_uv_top_[x_sb_w + 3], i, WD_PIX) = SB_GET(ap_uv_out_cb[7], 3, i, WD_PIX);
    }
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void hls_StoreTopLeft_y(ap_uint<WD_PIX * 4> ap_y_top_[MAX_NUM_MB_W * 4],
                        ap_uint<WD_PIX * 4> ap_y_left_[4],
                        ap_uint<WD_PIX * 16> ap_y_out_cb[16],
                        ap_uint<LG2_MAX_NUM_MB_W> x_mb) {
    for (int i = 0; i < 4; i++) {
        int x_sb_w = (int)x_mb << 2;
#pragma HLS UNROLL
        VCT_GET(ap_y_left_[i], 0, WD_PIX) = SB_GET(ap_y_out_cb[i * 4 + 3], 0, 3, WD_PIX);
        VCT_GET(ap_y_left_[i], 1, WD_PIX) = SB_GET(ap_y_out_cb[i * 4 + 3], 1, 3, WD_PIX);
        VCT_GET(ap_y_left_[i], 2, WD_PIX) = SB_GET(ap_y_out_cb[i * 4 + 3], 2, 3, WD_PIX);
        VCT_GET(ap_y_left_[i], 3, WD_PIX) = SB_GET(ap_y_out_cb[i * 4 + 3], 3, 3, WD_PIX);

        VCT_GET(ap_y_top_[x_sb_w + 0], i, WD_PIX) = SB_GET(ap_y_out_cb[12], 3, i, WD_PIX);
        VCT_GET(ap_y_top_[x_sb_w + 1], i, WD_PIX) = SB_GET(ap_y_out_cb[13], 3, i, WD_PIX);
        VCT_GET(ap_y_top_[x_sb_w + 2], i, WD_PIX) = SB_GET(ap_y_out_cb[14], 3, i, WD_PIX);
        VCT_GET(ap_y_top_[x_sb_w + 3], i, WD_PIX) = SB_GET(ap_y_out_cb[15], 3, i, WD_PIX);
    }
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

void hls_LoadPre_out_widen(ap_uint<WD_PIX>* ap_y_m,
                           ap_uint<WD_PIX>* ap_u_m,
                           ap_uint<WD_PIX>* ap_v_m,
                           ap_uint<WD_PIX * 4> ap_y_top_c[4],
                           ap_uint<WD_PIX * 4> ap_y4_top_c[4],
                           ap_uint<WD_PIX * 4> ap_uv_top_c[4],
                           ap_uint<WD_PIX * 4>* ap_y4_topright_c,
                           ap_uint<WD_PIX * 4> ap_y_left_c[4],
                           ap_uint<WD_PIX * 4> ap_y4_left_c[4],
                           ap_uint<WD_PIX * 4> ap_uv_left_c[4],
                           ap_uint<WD_PIX * 4> ap_y_left_[4],
                           ap_uint<WD_PIX * 4> ap_uv_left_[4],
                           ap_uint<WD_PIX * 4> ap_y_top_[MAX_NUM_MB_W * 4],
                           ap_uint<WD_PIX * 4> ap_uv_top_[MAX_NUM_MB_W * 4],
                           ap_uint<LG2_MAX_NUM_MB_W> x_mb,
                           ap_uint<LG2_MAX_NUM_MB_W> y_mb,
                           ap_uint<LG2_MAX_NUM_MB_W> mb_w) {
    int x = (int)x_mb << 2;
    int y = (int)y_mb << 2;
    ap_uint<1> istop = (y_mb == 0);
    ap_uint<1> isleft = (x_mb == 0);

    if (y > 0) {
        *ap_y_m = VCT_GET(ap_y_top_c[3], 3, WD_PIX);
        *ap_u_m = VCT_GET(ap_uv_top_c[1], 3, WD_PIX);
        *ap_v_m = VCT_GET(ap_uv_top_c[3], 3, WD_PIX);
    } else {
        *ap_y_m = 0x7f;
        *ap_u_m = 0x7f;
        *ap_v_m = 0x7f;
    }

    if (y > 0) {
        ap_y_top_c[0] = ap_y4_top_c[0] = ap_y_top_[x + 0];
        ap_y_top_c[1] = ap_y4_top_c[1] = ap_y_top_[x + 1];
        ap_y_top_c[2] = ap_y4_top_c[2] = ap_y_top_[x + 2];
        ap_y_top_c[3] = ap_y4_top_c[3] = ap_y_top_[x + 3];
        if (x_mb < mb_w - 1)
            (*ap_y4_topright_c) = ap_y_top_[x + 4];
        else {
            VCT_GET((*ap_y4_topright_c), 0, WD_PIX) = VCT_GET(ap_y4_top_c[3], 3, WD_PIX);
            VCT_GET((*ap_y4_topright_c), 1, WD_PIX) = VCT_GET(ap_y4_top_c[3], 3, WD_PIX);
            VCT_GET((*ap_y4_topright_c), 2, WD_PIX) = VCT_GET(ap_y4_top_c[3], 3, WD_PIX);
            VCT_GET((*ap_y4_topright_c), 3, WD_PIX) = VCT_GET(ap_y4_top_c[3], 3, WD_PIX);
        }
    } else {
        ap_y_top_c[0] = ap_y4_top_c[0] = 0x7f7f7f7f;
        ap_y_top_c[1] = ap_y4_top_c[1] = 0x7f7f7f7f;
        ap_y_top_c[2] = ap_y4_top_c[2] = 0x7f7f7f7f;
        ap_y_top_c[3] = ap_y4_top_c[3] = 0x7f7f7f7f;
        (*ap_y4_topright_c) = 0x7f7f7f7f;
    }
    if (x > 0) {
        ap_y_left_c[0] = ap_y4_left_c[0] = ap_y_left_[0];
        ap_y_left_c[1] = ap_y4_left_c[1] = ap_y_left_[1];
        ap_y_left_c[2] = ap_y4_left_c[2] = ap_y_left_[2];
        ap_y_left_c[3] = ap_y4_left_c[3] = ap_y_left_[3];
    } else {
        ap_y_left_c[0] = ap_y4_left_c[0] = 0x81818181;
        ap_y_left_c[1] = ap_y4_left_c[1] = 0x81818181;
        ap_y_left_c[2] = ap_y4_left_c[2] = 0x81818181;
        ap_y_left_c[3] = ap_y4_left_c[3] = 0x81818181;
    }
    ap_uv_top_c[0] = ap_uv_top_[x + 0];
    ap_uv_top_c[1] = ap_uv_top_[x + 1];
    ap_uv_top_c[2] = ap_uv_top_[x + 2];
    ap_uv_top_c[3] = ap_uv_top_[x + 3];
    ap_uv_left_c[0] = ap_uv_left_[0];
    ap_uv_left_c[1] = ap_uv_left_[1];
    ap_uv_left_c[2] = ap_uv_left_[2];
    ap_uv_left_c[3] = ap_uv_left_[3];
}

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void hls_LoadPre_mode_widen(ap_uint<WD_MODE> ap_y_top_mode[MAX_NUM_MB_W * 4],
                            ap_uint<WD_MODE> ap_y_left_mode[4],
                            ap_uint<WD_MODE> ap_y_top_c_mode[4],
                            ap_uint<WD_MODE> ap_y4_top_c_mode[16],
                            ap_uint<WD_MODE> ap_y_left_c_mode[4],
                            ap_uint<WD_MODE>* ap_y_m_mode,
                            ap_uint<LG2_MAX_NUM_MB_W> x_mb,
                            ap_uint<LG2_MAX_NUM_MB_W> y_mb,
                            ap_uint<LG2_MAX_NUM_MB_W> mb_w) {
    int x = (int)x_mb << 2;
    int y = (int)y_mb << 2;
    if (y > 0) {
        *ap_y_m_mode = ap_y_top_c_mode[3];
    } else {
        *ap_y_m_mode = DC_PRED; // 0
    }
    if (y > 0) {
        ap_y_top_c_mode[0] = ap_y4_top_c_mode[0] = ap_y_top_mode[x + 0];
        ap_y_top_c_mode[1] = ap_y4_top_c_mode[1] = ap_y_top_mode[x + 1];
        ap_y_top_c_mode[2] = ap_y4_top_c_mode[2] = ap_y_top_mode[x + 2];
        ap_y_top_c_mode[3] = ap_y4_top_c_mode[3] = ap_y_top_mode[x + 3];
    } else {
        ap_y_top_c_mode[0] = ap_y4_top_c_mode[0] = DC_PRED; // 0
        ap_y_top_c_mode[1] = ap_y4_top_c_mode[1] = DC_PRED; // 0
        ap_y_top_c_mode[2] = ap_y4_top_c_mode[2] = DC_PRED; // 0
        ap_y_top_c_mode[3] = ap_y4_top_c_mode[3] = DC_PRED; // 0
    }
    if (x > 0) {
        ap_y_left_c_mode[0] = ap_y_left_mode[0];
        ap_y_left_c_mode[1] = ap_y_left_mode[1];
        ap_y_left_c_mode[2] = ap_y_left_mode[2];
        ap_y_left_c_mode[3] = ap_y_left_mode[3];
    } else {
        ap_y_left_c_mode[0] = DC_PRED;
        ap_y_left_c_mode[1] = DC_PRED; // 0
        ap_y_left_c_mode[2] = DC_PRED; // 0
        ap_y_left_c_mode[3] = DC_PRED; // 0
    }
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

ap_uint<12> hls_GetCost_widen(ap_uint<4> n_sb,
                              ap_uint<4> mode,
                              ap_uint<WD_MODE> ap_y_top_c_mode[4], // at beginning, default is DC
                              ap_uint<WD_MODE> ap_y_left_c_mode[4],
                              ap_uint<WD_MODE> local_mod) {
#pragma HLS PIPELINE
    const int x_sb = (n_sb & 3), y_sb = n_sb >> 2;
    int left2 = ap_y_left_c_mode[y_sb];
    int top2 = (y_sb == 0) ? (int)(ap_y_top_c_mode[x_sb]) : (int)(local_mod);
    return my_VP8FixedCostsI4[top2][left2][mode];
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void Pickup_dataflow3_widen(
    // Parameters unParameters changed for one picture/segment
    ap_uint<WD_LMD> I__tlambda,               //              :
    ap_uint<WD_LMD> I__tlambda_m,             //
    ap_uint<WD_LMD> I__lambda_p44,            //
    ap_uint<WD_LMD> I__lambda_p16,            //
    ap_uint<WD_LMD> I__lambda_uv,             //
    hls_QMatrix I__hls_qm1,                   // y44,y16
    hls_QMatrix I__hls_qm2,                   // y16
    hls_QMatrix I__hls_qm_uv,                 //
    ap_int<WD_sharpen * 16> I__ap_sharpen,    //
    ap_int<WD_sharpen * 16> I__ap_sharpen_uv, //
    // Parameters changed for each MB
    ap_uint<WD_PIX * 16> I__ap_yuv_in_y44[16], //
    ap_uint<WD_PIX * 16> I__ap_yuv_in_y16[16], //
    ap_uint<WD_PIX * 16> I__ap_uv_in_[8],      //
    ap_uint<1> I__istop,                       //
    ap_uint<1> I__isleft,                      //
    ap_uint<1> I__isright,                     //
    // image context
    ap_uint<WD_PIX * 4> I__ap_y_top_c_y44[4],  //
    ap_uint<WD_PIX * 4> I__ap_y_top_c_y16[4],  //
    ap_uint<WD_PIX * 4> I__ap_y_left_c_y44[4], //
    ap_uint<WD_PIX * 4> I__ap_y_left_c_y16[4], //
    ap_uint<WD_PIX * 4> I__ap_uv_top_c[4],     //
    ap_uint<WD_PIX * 4> I__ap_uv_left_c[4],    //
    ap_uint<WD_PIX> I__ap_y_m,                 //
    ap_uint<WD_PIX> I__ap_u_m,                 //
    ap_uint<WD_PIX> I__ap_v_m,                 //
    ap_uint<WD_PIX * 4> I__ap_y4_topright_c,   //
    // mode context
    ap_uint<WD_MODE> I__ap_y_top_c_mode[4],  //
    ap_uint<WD_MODE> I__ap_y_left_c_mode[4], //
    // OUTPUT
    ap_uint<WD_PIX * 16> O__ap_y4_out_cb[16],       //
    ap_uint<WD_PIX * 16> O__ap_y_out_cb[2][17],     //
    ap_uint<WD_PIX * 16> O__ap_uv_out_cb[2][17],    //
    ap_int<WD_LEVEL * 16> O__ap_y4_level_cb[17],    //
    ap_int<WD_LEVEL * 16> O__ap_y_level_cb[2][17],  //
    ap_int<WD_LEVEL * 16> O__ap_y16dc_level_cb[2],  //
    ap_int<WD_LEVEL * 16> O__ap_uv_level_cb[2][16], //
    // str_rd_i4*              OP_rd_y4_acc,//
    ap_uint<WD_RD_SCORE + 4>* O__score_acc,
    ap_uint<25>* O__nz_mb,
    str_rd O__rd_y16_cb[2],                  //
    str_rd O__rd_uv_cb[2],                   //
    ap_uint<WD_MODE> O_ap_y4_top_c_mode[16], //
    ap_uint<WD_MODE>* OP_ap_y16_mode_c,      //
    ap_uint<WD_MODE>* OP_ap_uv_mode_c,       //
    ap_uint<1>* OP_b_uv,                     //
    ap_uint<2>* OP_b_y                       //
    ) {
#pragma HLS DATAFLOW
    Pickup_Y44_widen(
        // OP_rd_y4_acc->nz = Pickup_Y44_new(
        I__istop, I__isleft,
        I__ap_y_top_c_y44,   //[4],
        I__ap_y_left_c_y44,  //[4],
        I__ap_y_top_c_mode,  //[4],// at beginning, default is DC
        I__ap_y_left_c_mode, //[4],
        //  IO_ap_y4_top_c_mode,//[16],
        I__ap_y4_topright_c, I__ap_y_m,
        I__ap_yuv_in_y44, //[16],
        I__hls_qm1, I__ap_sharpen, I__lambda_p44, I__tlambda, I__tlambda_m,
        // OUTPUT
        O__ap_y4_out_cb, O__ap_y4_level_cb, O__score_acc, O__nz_mb, O_ap_y4_top_c_mode);

    Pickup_Y16(I__tlambda,    //     = dqm->tlambda_;
               I__tlambda_m,  //   = dqm->lambda_mode_;
               I__lambda_p16, //  = dqm->lambda_i16_;
               I__hls_qm1,    //
               I__hls_qm2,    //
               I__ap_sharpen,
               // Parameters changed for each MB
               I__ap_yuv_in_y16, //[16],
               I__istop, I__isleft, I__isright,
               // image context
               I__ap_y_top_c_y16,  //[4],
               I__ap_y_left_c_y16, //[4],
               I__ap_y_m,
               // OUTPUT
               O__ap_y_out_cb,       //[2][17],
               O__ap_y_level_cb,     //[2][17],
               O__ap_y16dc_level_cb, //[2],
               O__rd_y16_cb,         //[2],
               OP_ap_y16_mode_c,     //
               OP_b_y                //
               );

    Pickup_UV(
        // Parameters unParameters changed for one picture/segment
        I__tlambda,   //     = dqm->tlambda_;
        I__tlambda_m, //   = dqm->lambda_mode_;
        I__lambda_uv, //   = dqm->lambda_uv_;
        I__hls_qm_uv, I__ap_sharpen_uv,
        // Parameters changed for each MB
        I__ap_uv_in_, //[8],
        I__istop, I__isleft, I__isright,
        // image context
        I__ap_uv_top_c,  //[4],
        I__ap_uv_left_c, //[4],
        I__ap_u_m, I__ap_v_m,
        // OUTPUT
        O__ap_uv_out_cb,   //[2][17],
        O__ap_uv_level_cb, //[2][16],
        O__rd_uv_cb,       //[2],
        OP_ap_uv_mode_c, OP_b_uv);
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<4> Map_k_2_n_sb_0(int k) {
// 0, 1, 2, 3, 4,
#pragma HLS INLINE
    ap_uint<4> ret;
    if (k <= 3)
        return k;
    else if (k <= 5)
        return k + 2;
    else if (k <= 7)
        return k + 4;
    else
        return k + 6;
}
ap_uint<4> Map_k_2_n_sb(int k, ap_uint<1> idx) {
#pragma HLS INLINE
    ap_uint<4> ret;
    if (k <= 1)
        return k;
    else if (k <= 7)
        return Map_k_2_n_sb_0(k) + (ap_uint<4>)idx * 2;
    else
        return Map_k_2_n_sb_0(k);
};
void Pickup_Y44_widen(ap_uint<1> istop,
                      ap_uint<1> isleft,
                      ap_uint<WD_PIX * 4> ap_y_top_c[4],
                      ap_uint<WD_PIX * 4> ap_y_left_c[4],
                      ap_uint<WD_MODE> ap_y_top_c_mode[4], // at beginning, default is DC
                      ap_uint<WD_MODE> ap_y_left_c_mode[4],
                      // ap_uint<WD_MODE>        ap_y4_top_c_mode[16],
                      ap_uint<WD_PIX * 4> ap_y4_topright_c,
                      ap_uint<WD_PIX * 4> ap_y_m,
                      // ap_uint<4>              MACRO_n_sb,
                      ap_uint<WD_PIX * 16> ap_yuv_in[16],
                      hls_QMatrix hls_qm1,
                      ap_int<WD_sharpen * 16> ap_sharpen,
                      ap_uint<WD_LMD> lambda_p44,
                      ap_uint<WD_LMD> tlambda,
                      ap_uint<WD_LMD> tlambda_m,
                      ap_uint<WD_PIX * 16> ap_y4_out_mb[16],
                      ap_int<WD_LEVEL * 16> ap_y4_level_mb[16],
                      ap_uint<WD_RD_SCORE + 4>* score_acc,
                      ap_uint<25>* nz_mb,
                      ap_uint<WD_MODE> O__modes_mb[16]) {
    //#pragma HLS INLINE
    /////////////////////////////////////////////
    // slow updated in loop without pipeline
    ap_uint<WD_MODE> local_mode_array[16];
    ap_uint<WD_PIX * 4> local_y4_top_c[16];
#pragma HLS ARRAY_PARTITION variable = local_y4_top_c complete dim = 1
    ap_uint<WD_PIX * 4> local_left_c[16];
#pragma HLS ARRAY_PARTITION variable = local_left_c complete dim = 1
    for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
        local_y4_top_c[i] = ap_y_top_c[i];
        local_left_c[i * 4] = ap_y_left_c[i];
        // local_mode_array[i] = ap_y_top_c_mode[i];
    }
#pragma HLS ARRAY_PARTITION variable = local_mode_array complete dim = 1
    //////////////////////////////////////////////
    // fast updated (read and wrote) in pipeline
    ap_uint<WD_RD_SCORE + 4> score_b_array[16];
#pragma HLS ARRAY_PARTITION variable = score_b_array complete dim = 1
    /////////////////////////////////////////////
    // simple output, only be wrote in loop
    ap_uint<25> nz_b = 0;
    ap_int<WD_LEVEL * 16> local_level_array[16];
#pragma HLS ARRAY_PARTITION variable = local_level_array complete dim = 1
    ap_uint<WD_LEVEL * 16> local_out_array[16];
#pragma HLS ARRAY_PARTITION variable = local_out_array complete dim = 1
    for (int k_p44 = 0; k_p44 < 10; k_p44++) {
        //#pragma HLS PIPELINE
        ap_uint<4> mode_b[2];
        ap_uint<4> n_sb2[2];
#pragma HLS ARRAY_PARTITION variable = mode_b complete dim = 1
#pragma HLS ARRAY_PARTITION variable = n_sb2 complete dim = 1
        n_sb2[0] = Map_k_2_n_sb(k_p44, 0);
        n_sb2[1] = Map_k_2_n_sb(k_p44, 1);
        const int loop1 = (k_p44 < 2 || k_p44 > 13) ? 10 : 20;
    PICKUP_Y44:
        for (int fmod = 0; fmod < loop1; fmod++) {
#pragma HLS PIPELINE
            //#pragma HLS dependence array inter false
            int set = fmod >= 10 ? 1 : 0;
            ap_uint<4> n_sb = Map_k_2_n_sb(k_p44, set);
            ap_uint<4> MACRO_mode = fmod >= 10 ? fmod - 10 : fmod; // fmod(4,1);
            ap_uint<WD_PIX* 16> ap_yuv_in_sb = ap_yuv_in[n_sb];
            ap_uint<WD_PIX * 4> abcd, efgh, ijkl;
            ap_uint<WD_PIX> x44;
            hls_LoadPreds4_ins(local_y4_top_c, local_left_c, ap_y4_topright_c, ap_y_m, &abcd, &efgh, &ijkl, &x44,
                               isleft, istop, n_sb);

            ap_uint<1> MACRO_isfirst = (MACRO_mode == 0);
            ap_uint<WD_PIX * 16> ap_ref_p44;
            ap_ref_p44 = hls_p4_test(abcd, efgh, ijkl, x44, MACRO_mode);
            ap_uint<WD_MODE> mode_up;
            // if(n_sb>3)
            // mode_up = local_mode_array[n_sb-4];
            ap_uint<12> pre_dis_h;
            pre_dis_h = hls_GetCost(n_sb, MACRO_mode, ap_y_top_c_mode, ap_y_left_c_mode, local_mode_array);

            ap_uint<WD_PIX * 16> ap_y4_out_tmp;
            ap_int<WD_LEVEL * 16> ap_y4_level_tmp;
            ap_uint<WD_RD_SCORE + 4> score_sb;
            ap_uint<25> nz_sb;
            ap_uint<4> mode_out;
            hls_channel_p44(MACRO_mode, ap_yuv_in_sb, ap_ref_p44, hls_qm1, ap_sharpen, lambda_p44, tlambda, tlambda_m,
                            pre_dis_h, &ap_y4_out_tmp, &ap_y4_level_tmp, &score_sb, &nz_sb, &mode_out);
            // if (MACRO_isfirst || score_sb < score_b_tmp)
            if (MACRO_isfirst || score_sb < score_b_array[n_sb]) {
                score_b_array[n_sb] = score_sb;
                local_out_array[n_sb] = ap_y4_out_tmp;
                local_level_array[n_sb] = ap_y4_level_tmp;
                nz_b[n_sb] = nz_sb;
                mode_b[set] = mode_out;
                // local_mode_array[n_sb] = mode_out;
            }
        } // for mode

        local_mode_array[n_sb2[0]] = mode_b[0];
        local_mode_array[n_sb2[1]] = (loop1 == 20) ? mode_b[1] : mode_b[0];
        if (n_sb2[0] < 12) local_y4_top_c[n_sb2[0] + 4] = VCT_GET(local_out_array[n_sb2[0]], 3, WD_PIX * 4);
        if (n_sb2[1] < 12) local_y4_top_c[n_sb2[1] + 4] = VCT_GET(local_out_array[n_sb2[1]], 3, WD_PIX * 4);
        if ((n_sb2[0] & 3) != 3) { // 3,7,11,15 //VCT_SET_COL_SB(sb, col, wd, vect)
            VCT_GET(local_left_c[n_sb2[0] + 1], 0, WD_PIX) = VCT_GET(local_out_array[n_sb2[0]], 3, WD_PIX);
            VCT_GET(local_left_c[n_sb2[0] + 1], 1, WD_PIX) = VCT_GET(local_out_array[n_sb2[0]], 7, WD_PIX);
            VCT_GET(local_left_c[n_sb2[0] + 1], 2, WD_PIX) = VCT_GET(local_out_array[n_sb2[0]], 11, WD_PIX);
            VCT_GET(local_left_c[n_sb2[0] + 1], 3, WD_PIX) = VCT_GET(local_out_array[n_sb2[0]], 15, WD_PIX);
        }
        if ((n_sb2[1] & 3) != 3) { // 3,7,11,15 //VCT_SET_COL_SB(sb, col, wd, vect)
            VCT_GET(local_left_c[n_sb2[1] + 1], 0, WD_PIX) = VCT_GET(local_out_array[n_sb2[1]], 3, WD_PIX);
            VCT_GET(local_left_c[n_sb2[1] + 1], 1, WD_PIX) = VCT_GET(local_out_array[n_sb2[1]], 7, WD_PIX);
            VCT_GET(local_left_c[n_sb2[1] + 1], 2, WD_PIX) = VCT_GET(local_out_array[n_sb2[1]], 11, WD_PIX);
            VCT_GET(local_left_c[n_sb2[1] + 1], 3, WD_PIX) = VCT_GET(local_out_array[n_sb2[1]], 15, WD_PIX);
        }

    }                                           // for n_sb;
    ap_uint<WD_RD_SCORE + 4> score_acc_tmp = 0; //(1<<WD_RD_SCORE+2);
    for (int i = 0; i < 16; i++) {
#pragma HLS UNROLL
        O__modes_mb[i] = local_mode_array[i];
        score_acc_tmp += score_b_array[i];
        ap_y4_level_mb[i] = local_level_array[i];
        ap_y4_out_mb[i] = local_out_array[i];
    }
    *nz_mb |= nz_b;
    *score_acc = score_acc_tmp;
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<12> hls_GetCost(ap_uint<4> n_sb,
                        ap_uint<4> mode,
                        ap_uint<WD_MODE> ap_y_top_c_mode[4], // at beginning, default is DC
                        ap_uint<WD_MODE> ap_y_left_c_mode[4],
                        ap_uint<WD_MODE> ap_y4_top_c_mode[16]) {
#pragma HLS PIPELINE
    const int x_sb = (n_sb & 3), y_sb = n_sb >> 2;
    int left2 = ap_y_left_c_mode[y_sb];
    int top2 = (y_sb == 0) ? (int)(ap_y_top_c_mode[x_sb]) : (int)(ap_y4_top_c_mode[n_sb - 4]);
    return my_VP8FixedCostsI4[top2][left2][mode];
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void hls_LoadPreds4_ins(ap_uint<WD_PIX * 4> ap_y4_top_c[16],
                        ap_uint<WD_PIX * 4> ap_y4_left_c[16],
                        ap_uint<WD_PIX * 4> ap_y4_topright_c,
                        ap_uint<WD_PIX * 4> ap_y_m,
                        ap_uint<WD_PIX * 4>* abcd,
                        ap_uint<WD_PIX * 4>* efgh,
                        ap_uint<WD_PIX * 4>* ijkl,
                        ap_uint<WD_PIX>* x44,
                        ap_uint<1> isleft,
                        ap_uint<1> istop,
                        ap_uint<4> n_sb) {
#pragma HLS PIPELINE
    *abcd = ap_y4_top_c[n_sb];
    if ((n_sb & 3) != 3) // 3,7,11,15
        *efgh = ap_y4_top_c[n_sb + 1];
    else
        *efgh = ap_y4_topright_c;
    *ijkl = ap_y4_left_c[n_sb];
    if (n_sb == 0) {
        if (!isleft)
            *x44 = ap_y_m;
        else if (!istop)
            *x44 = 0X81;
        else
            *x44 = 0X7f;
    } else if ((n_sb & 3) != 0) //! 0,4,8,12
        *x44 = VCT_GET(ap_y4_top_c[n_sb - 1], 3, WD_PIX);
    else // 4,8,12
        *x44 = VCT_GET(ap_y4_left_c[n_sb - 4], 3, WD_PIX);
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_PIX * 16> hls_p4_test(ap_uint<WD_PIX * 4> abcd,
                                 ap_uint<WD_PIX * 4> efgh,
                                 ap_uint<WD_PIX * 4> ijkl,
                                 ap_uint<WD_PIX> x44,
                                 ap_uint<4> mode) {
    switch (mode) {
        case B_DC_PRED:
            return hls_DC4(abcd, ijkl);
        case B_VE_PRED:
            return hls_VE4(abcd, efgh, x44);
        case B_HE_PRED:
            return hls_HE4(ijkl, x44);
        case B_RD_PRED:
            return hls_RD4(abcd, ijkl, x44);
        case B_LD_PRED:
            return hls_LD4(abcd, efgh);
        case B_VR_PRED:
            return hls_VR4(abcd, ijkl, x44);
        case B_VL_PRED:
            return hls_VL4(abcd, efgh);
        case B_HU_PRED:
            return hls_HU4(ijkl);
        case B_HD_PRED:
            return hls_HD4(abcd, ijkl, x44); // ref:544 vs lut:100 ,3.19+1.25 ,
        default:
            return hls_TM4(abcd, ijkl, x44);
    } // case
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void hls_channel_p44(ap_uint<4> mode_in,
                     ap_uint<WD_PIX * 16> ap_yuv_in_sb,
                     ap_uint<WD_PIX * 16> ap_ref_p44,
                     hls_QMatrix hls_qm1,
                     ap_int<WD_sharpen * 16> ap_sharpen,
                     ap_uint<WD_LMD> lambda_p44,
                     ap_uint<WD_LMD> tlambda,
                     ap_uint<WD_LMD> tlambda_m,
                     ap_uint<12> pre_dis_h,
                     ap_uint<WD_PIX * 16>* ap_y4_out_cb_n_sb2,
                     ap_int<WD_LEVEL * 16>* ap_y4_level_cb_n_sb2,
                     ap_uint<WD_RD_SCORE + 4>* score_sb,
                     ap_uint<25>* nz_sb,
                     ap_uint<4>* mode_out) {
#pragma HLS INLINE
    //#pragma HLS PIPELINE
    ap_uint<WD_PIX* 16> ap_src_p44 = ap_yuv_in_sb;
    str_dis rd_dis4;
    rd_dis4.init();
    ap_uint<WD_DCT* 16> ap_dct_p44 = hls_FTransform(ap_src_p44, ap_ref_p44);
    ap_int<WD_IQT * 16> ap_iqt_p44;
    ap_uint<5> ap_nz;
    ap_uint<WD_PIX * 16> ap_y4_out_tmp;
    ap_int<WD_LEVEL * 16> ap_y4_level_tmp;
    ap_nz = hls_QuantizeBlock(ap_dct_p44, &ap_y4_level_tmp, &ap_iqt_p44, &hls_qm1, ap_sharpen, 0);
    rd_dis4.nz = (ap_nz != 0); //<<n_sb;
    if (rd_dis4.nz) ap_nz -= (VCT_GET(ap_y4_level_tmp, 0, WD_LEVEL) != 0);
    ap_y4_out_tmp = hls_ITransformOne(ap_ref_p44, ap_iqt_p44);
    rd_dis4.d = hls_SSE4X4(ap_src_p44, ap_y4_out_tmp);
    rd_dis4.sd = hls_Disto4x4(ap_src_p44, ap_y4_out_tmp);
    rd_dis4.sd = (rd_dis4.sd * tlambda + 128) >> 8;
    rd_dis4.h = pre_dis_h; // hls_GetCost(n_sb, mode,  ap_y_top_c_mode, ap_y_left_c_mode,  ap_y4_top_c_mode);
    if ((mode_in > 0) && (ap_nz <= 3))
        rd_dis4.r = 140;
    else
        rd_dis4.r = 0;
    rd_dis4.r += hls_fast_cost(ap_y4_level_tmp, 0);
    rd_dis4.ca_score(tlambda_m);
    // output
    *score_sb = rd_dis4.score;
    *nz_sb = rd_dis4.nz;
    *ap_y4_out_cb_n_sb2 = ap_y4_out_tmp;
    *ap_y4_level_cb_n_sb2 = ap_y4_level_tmp;
    *mode_out = mode_in;
}

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_TTR> hls_TTransform(ap_uint<WD_PIX * 16> in) {
#pragma HLS INLINE
    //#pragma HLS PIPELINE
    ap_uint<WD_TTR> sum(0);
    ap_int<WD_PIX + 3> tmp[16];
    ap_uint<WD_TTW> WeightY[16] = {38, 32, 20, 9, 32, 28, 17, 7, 20, 17, 10, 4, 9, 7, 4, 2};
#pragma HLS ARRAY_PARTITION variable = WeightY complete dim = 1
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_PIX + 2> a0 = ((ap_int<WD_PIX + 2>)VCT_GET(in, 0 + i * 4, WD_PIX) +
                                 (ap_int<WD_PIX + 2>)VCT_GET(in, 2 + i * 4, WD_PIX)); // 10b
        ap_int<WD_PIX + 2> a1 =
            ((ap_int<WD_PIX + 2>)VCT_GET(in, 1 + i * 4, WD_PIX) + (ap_int<WD_PIX + 2>)VCT_GET(in, 3 + i * 4, WD_PIX));
        ap_int<WD_PIX + 2> a2 =
            ((ap_int<WD_PIX + 2>)VCT_GET(in, 1 + i * 4, WD_PIX) - (ap_int<WD_PIX + 2>)VCT_GET(in, 3 + i * 4, WD_PIX));
        ap_int<WD_PIX + 2> a3 =
            ((ap_int<WD_PIX + 2>)VCT_GET(in, 0 + i * 4, WD_PIX) - (ap_int<WD_PIX + 2>)VCT_GET(in, 2 + i * 4, WD_PIX));
        tmp[0 + i * 4] = a0 + a1; // 11b
        tmp[1 + i * 4] = a3 + a2;
        tmp[2 + i * 4] = a3 - a2;
        tmp[3 + i * 4] = a0 - a1;
    }
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_PIX + 4> a0 = (tmp[0 + i] + tmp[8 + i]); // 12b
        ap_int<WD_PIX + 4> a1 = (tmp[4 + i] + tmp[12 + i]);
        ap_int<WD_PIX + 4> a2 = (tmp[4 + i] - tmp[12 + i]);
        ap_int<WD_PIX + 4> a3 = (tmp[0 + i] - tmp[8 + i]);
        ap_int<WD_PIX + 5> b0 = a0 + a1; // 13b
        ap_int<WD_PIX + 5> b1 = a3 + a2;
        ap_int<WD_PIX + 5> b2 = a3 - a2;
        ap_int<WD_PIX + 5> b3 = a0 - a1;
        // sum += (ap_uint<WD_TTR>)WeightY[ 0 + i] * b0.abs();     //error: no member named 'abs' in 'ap_int<13>???
        // sum += (ap_uint<WD_TTR>)WeightY[ 4 + i] * b1.abs();
        // sum += (ap_uint<WD_TTR>)WeightY[ 8 + i] * b2.abs();
        // sum += (ap_uint<WD_TTR>)WeightY[12 + i] * b3.abs();
        if (b0 < 0) b0 = -b0;
        if (b1 < 0) b1 = -b1;
        if (b2 < 0) b2 = -b2;
        if (b3 < 0) b3 = -b3;
        sum += (ap_uint<WD_TTR>)WeightY[0 + i] * b0;  //.abs();     //
        sum += (ap_uint<WD_TTR>)WeightY[4 + i] * b1;  //.abs();
        sum += (ap_uint<WD_TTR>)WeightY[8 + i] * b2;  //.abs();
        sum += (ap_uint<WD_TTR>)WeightY[12 + i] * b3; //.abs();
    }
    return sum;
}

/* FOR SD CACULATION */
ap_uint<WD_DISTO> hls_Disto4x4(ap_uint<WD_PIX * 16> a, ap_uint<WD_PIX * 16> b) {
#pragma HLS INLINE
    ap_uint<WD_TTR> sum1 = hls_TTransform(a);
    ap_uint<WD_TTR> sum2 = hls_TTransform(b);
    ap_int<WD_TTR + 2> tmp = (ap_int<WD_TTR + 2>)sum1 - sum2;
    //   ap_uint<WD_DISTO>   val     = (__abs(tmp))>>5;
    // ap_uint<WD_DISTO>   val     = (tmp.abs())>>5; //error: no member named 'abs' in 'ap_int<13>???
    if (tmp < 0) tmp = -tmp;
    ap_uint<WD_DISTO> val = (tmp) >> 5;
    return val;
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

/* FOR R CACULATION */
ap_uint<WD_LEVEL + 4> hls_LV0(ap_uint<WD_LEVEL - 1> lv) {
    if (lv == 0) return 0;
    if (lv == 1) return 760;
    if (lv == 2) return 1500;
    if (lv == 3) return 2000;
    if (lv == 4) return 2300;
    if (lv == 5) return 2500;
    ap_uint<WD_LEVEL + 4> tmp = lv;
    tmp = ((ap_uint<WD_LEVEL + 4>)lv) << 7;
    tmp += ((ap_uint<WD_LEVEL + 4>)lv) << 6;
    tmp += ((ap_uint<WD_LEVEL + 4>)lv) << 3;
    return 1500 + tmp;
}
ap_uint<WD_LEVEL> hls_LV1(ap_uint<WD_LEVEL - 1> lv) {
    if (lv == 0) return 0;
    if (lv == 1) return 1000;
    if (lv == 2) return 1100;
    return 1200;
}

ap_uint<WD_LEVEL> hls_LV2(ap_uint<WD_LEVEL - 1> lv) {
    if (lv == 0) return 0;
    if (lv == 1) return 1000;
    if (lv == 2) return 1100;
    if (lv == 3) return 1150;
    return 1180;
}
ap_uint<WD_LEVEL> hls_LVn(ap_uint<WD_LEVEL - 1> lv) {
    if (lv == 0) return 0;
    return 500;
}

ap_uint<WD_FAST> hls_fast_cost(ap_int<WD_LEVEL * 16> vlevel, ap_uint<2> type) {
    ap_uint<WD_FAST * 16> r_fast;
    ap_uint<WD_LEVEL - 1> levels[16];
#pragma HLS INLINE
    //#pragma HLS pipeline
    for (int i = 0; i < 13; i++) {
#pragma HLS unroll
        ap_int<WD_LEVEL> alevel = (ap_int<WD_LEVEL>)VCT_GET(vlevel, (i + type[1]), WD_LEVEL);
        if (alevel < 0)
            levels[i] = (0 - alevel);
        else
            levels[i] = alevel;
    }
    ap_uint<7> offset;
    if (type == 0) // NORMAL
        offset = 0;
    else if (type == 1) // DC
        offset = 96;
    else
        offset = 113;                                   // AC
    ap_uint<WD_FAST> tmp = hls_LV0((levels[0]));        // +
    tmp += hls_LV1((levels[1]));                        // +
    tmp += hls_LV2((levels[2]));                        // +
    tmp += 2 * hls_LVn((levels[3]));                    // +
    tmp += 2 * hls_LVn((levels[4]));                    // +
    tmp += 3 * hls_LVn((levels[5]));                    // +
    tmp += 3 * hls_LVn((levels[6]));                    // +
    tmp += 4 * hls_LVn((levels[7]));                    // +
    tmp += 4 * hls_LVn((levels[8]));                    // +
    tmp += 5 * hls_LVn((levels[9]));                    // +
    tmp += 5 * hls_LVn((levels[10]));                   // +
    tmp += 5 * hls_LVn((levels[11]));                   // +
    if (type[1] != 1) tmp += 5 * hls_LVn((levels[12])); // +
    //                  tmp += 5*hls_LVn((levels[13]));// +
    //                  tmp += 5*hls_LVn((levels[14]));// +
    //                  tmp += 5*hls_LVn((levels[15]));//;
    return tmp + offset;
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

/* FOR S CACULATION */
ap_uint<WD_SSE4> hls_SSE4X4(ap_uint<WD_PIX * 16> src, ap_uint<WD_PIX * 16> rec) {
#pragma HLS INLINE
    ap_uint<WD_SSE4> sse4 = 0;
    //#pragma HLS pipeline
    for (int i = 0; i < 16; i++) {
#pragma HLS unroll
        ap_int<WD_PIX + 1> sub = (ap_int<WD_PIX + 1>)VCT_GET(src, i, WD_PIX) - VCT_GET(rec, i, WD_PIX);
        sse4 += sub * sub;
    }
    return sse4;
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

/* NORMAL TRANSFORM */
ap_int<WD_DCT * 16> hls_FTransform(ap_uint<WD_PIX * 16> src_ap, ap_uint<WD_PIX * 16> ref_ap) {
    /*FF:1531; LUT:1749; DSP:33;4.89+0.62ns; 7 cycles */
    ap_int<WD_DCT * 16> out_ap;
    ap_int<(14) * 16> tmp_ap;
#pragma HLS INLINE
    //#pragma HLS PIPELINE
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_SUB> d0_ap = SB_GET(src_ap, i, 0, WD_PIX) - SB_GET(ref_ap, i, 0, WD_PIX); //
        ap_int<WD_SUB> d1_ap = SB_GET(src_ap, i, 1, WD_PIX) - SB_GET(ref_ap, i, 1, WD_PIX);
        ap_int<WD_SUB> d2_ap = SB_GET(src_ap, i, 2, WD_PIX) - SB_GET(ref_ap, i, 2, WD_PIX);
        ap_int<WD_SUB> d3_ap = SB_GET(src_ap, i, 3, WD_PIX) - SB_GET(ref_ap, i, 3, WD_PIX);
        ap_int<WD_SUB + 1> a0_ap = (d0_ap + d3_ap); // 10b                      [-510,510]
        ap_int<WD_SUB + 1> a1_ap = (d1_ap + d2_ap);
        ap_int<WD_SUB + 1> a2_ap = (d1_ap - d2_ap);
        ap_int<WD_SUB + 1> a3_ap = (d0_ap - d3_ap);
        VCT_GET(tmp_ap, i * 4 + 0, 14) = (a0_ap + a1_ap) * 8;                       // 14b   [-8160,8160]
        VCT_GET(tmp_ap, i * 4 + 1, 14) = (a2_ap * 2217 + a3_ap * 5352 + 1812) >> 9; // [-7536,7542]
        VCT_GET(tmp_ap, i * 4 + 2, 14) = (a0_ap - a1_ap) * 8;
        VCT_GET(tmp_ap, i * 4 + 3, 14) = (a3_ap * 2217 - a2_ap * 5352 + 937) >> 9;
    }
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<15> a0_ap = (ap_int<14>)VCT_GET(tmp_ap, (0 + i), 14) +
                           (ap_int<14>)VCT_GET(tmp_ap, (12 + i), 14); //(tmp_ap[0 + i] + tmp_ap[12 + i]);  // 15b
        ap_int<15> a1_ap = (ap_int<14>)VCT_GET(tmp_ap, (4 + i), 14) +
                           (ap_int<14>)VCT_GET(tmp_ap, (8 + i), 14); //(tmp_ap[4 + i] + tmp_ap[8 + i]);
        ap_int<15> a2_ap = (ap_int<14>)VCT_GET(tmp_ap, (4 + i), 14) -
                           (ap_int<14>)VCT_GET(tmp_ap, (8 + i), 14); //(tmp_ap[4 + i] - tmp_ap[8 + i]);
        ap_int<15> a3_ap = (ap_int<14>)VCT_GET(tmp_ap, (0 + i), 14) -
                           (ap_int<14>)VCT_GET(tmp_ap, (12 + i), 14); //(tmp_ap[0 + i] - tmp_ap[12 + i]);
        VCT_GET(out_ap, 0 + i, WD_DCT) = (a0_ap + a1_ap + 7) >> 4;    // 12b
        VCT_GET(out_ap, 4 + i, WD_DCT) = ((a2_ap * 2217 + a3_ap * 5352 + 12000) >> 16) + (a3_ap != 0);
        VCT_GET(out_ap, 8 + i, WD_DCT) = (a0_ap - a1_ap + 7) >> 4;
        VCT_GET(out_ap, 12 + i, WD_DCT) = ((a3_ap * 2217 - a2_ap * 5352 + 51000) >> 16);
    }
    return out_ap;
}

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

ap_uint<5> hls_QuantizeBlock(ap_int<WD_DCT * 16> in,
                             ap_int<WD_LEVEL * 16>* out,
                             ap_int<WD_DCT * 16>* out2,
                             hls_QMatrix* pQM, // frequency boosters for slight sharpening
                             ap_uint<WD_sharpen * 16> sharpen_,
                             ap_uint<1> is16) // frequency boosters for slight sharpening
{
#pragma HLS INLINE
    return hls_QuantizeBlock_old(in, out, out2, pQM->q_0, pQM->q_n, pQM->iq_0, pQM->iq_n, pQM->bias_0, pQM->bias_n,
                                 sharpen_, is16);
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

/* QUANTITION FOR NORMAL AND DC */
ap_uint<5> hls_QuantizeBlock_old(ap_int<WD_DCT * 16> in,
                                 ap_int<WD_LEVEL * 16>* out,
                                 ap_int<WD_DCT * 16>* out2,
                                 ap_uint<WD_q> q_0, // quantizer steps
                                 ap_uint<WD_q> q_n,
                                 ap_uint<WD_iq> iq_0, // reciprocals, fixed point.
                                 ap_uint<WD_iq> iq_n,
                                 ap_uint<WD_bias> bias_0, // rounding bias
                                 ap_uint<WD_bias> bias_n,
                                 ap_uint<WD_sharpen * 16> sharpen_,
                                 ap_uint<1> is16) // frequency boosters for slight sharpening
{
#pragma HLS INLINE
    //#pragma HLS pipeline
    ap_uint<5> last = 0;
    for (int n = 0; n < 16; ++n) {
#pragma HLS unroll
        if (is16 && n == 0) {
            VCT_GET((*out2), 0, WD_DCT) = 0;
            VCT_GET((*out), 0, WD_LEVEL) = 0;
            continue;
        }
        const ap_uint<4> j = ZIGZAG(n);
        const ap_int<WD_DCT> coeffs = (ap_int<WD_DCT>)VCT_GET(in, j, WD_DCT);
        const ap_uint<1> sign = coeffs[WD_DCT - 1];
        const ap_uint<WD_DCT - 1> coeff = (sign == 1 ? (ap_uint<WD_DCT - 1>)(-coeffs) : (ap_uint<WD_DCT - 1>)(coeffs)) +
                                          VCT_GET(sharpen_, j, WD_sharpen);
        const ap_uint<WD_q> Q = n == 0 ? q_0 : q_n;
        const ap_uint<WD_iq> iQ = n == 0 ? iq_0 : iq_n;
        const ap_uint<WD_bias> B = n == 0 ? bias_0 : bias_n;
        ap_uint<WD_MLEVEL> level = (ap_uint<WD_MLEVEL>)((coeff * iQ + B) >> 17);
        if (level > MY_MAX_LEVEL) level = MY_MAX_LEVEL;
        if (sign) level = -level;
        ap_int<WD_DCT> rec = level * Q;
        VCT_GET((*out2), j, WD_DCT) = level * Q;
        VCT_GET((*out), n, WD_LEVEL) = level;
        if (level) last += 1;
    }
    return (last);
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

/*Invers  Transforms */
const ap_uint<17> kC1 = 20091 + (1 << 16);
const ap_uint<17> kC2 = 35468;
#define MUL(a, b) (((a) * (b)) >> 16)
ap_uint<WD_PIX * 16> hls_ITransformOne(ap_uint<WD_PIX * 16> ap_ref, ap_int<WD_IQT * 16> ap_in) {
    ap_uint<WD_PIX * 16> ap_des;
    ap_int<WD_IQT + 3> ap_tmp[16];
#pragma HLS INLINE
    //#pragma HLS pipeline
    for (int i = 0; i < 4; ++i) { // vertical pass
#pragma HLS unroll
        ap_int<WD_IQT + 2> ap_a, ap_b, ap_c, ap_d;
        ap_a = (ap_int<WD_IQT>)VCT_GET(ap_in, i + 0, WD_IQT) + (ap_int<WD_IQT>)VCT_GET(ap_in, i + 8, WD_IQT);
        ap_b = (ap_int<WD_IQT>)VCT_GET(ap_in, i + 0, WD_IQT) - (ap_int<WD_IQT>)VCT_GET(ap_in, i + 8, WD_IQT);
        ap_c = MUL((ap_int<WD_IQT>)VCT_GET(ap_in, i + 4, WD_IQT), kC2) -
               MUL((ap_int<WD_IQT>)VCT_GET(ap_in, i + 12, WD_IQT), kC1);
        ap_d = MUL((ap_int<WD_IQT>)VCT_GET(ap_in, i + 4, WD_IQT), kC1) +
               MUL((ap_int<WD_IQT>)VCT_GET(ap_in, i + 12, WD_IQT), kC2);
        ap_tmp[i * 4 + 0] = ap_a + ap_d;
        ap_tmp[i * 4 + 1] = ap_b + ap_c;
        ap_tmp[i * 4 + 2] = ap_b - ap_c;
        ap_tmp[i * 4 + 3] = ap_a - ap_d;
    }
    for (int i = 0; i < 4; ++i) { // horizontal pass
#pragma HLS unroll
        ap_int<WD_IQT + 4> ap_dc, ap_a, ap_b, ap_c, ap_d;
        ap_int<WD_IQT + 1> s0, s1, s2, s3;
        ap_int<WD_IQT + 2> r0, r1, r2, r3;
        ap_dc = 4 + ap_tmp[i + 0];
        ap_a = ap_dc + ap_tmp[i + 8];
        ap_b = ap_dc - ap_tmp[i + 8];
        ap_c = MUL(ap_tmp[i + 4], kC2) - MUL(ap_tmp[i + 12], kC1);
        ap_d = MUL(ap_tmp[i + 4], kC1) + MUL(ap_tmp[i + 12], kC2);
        s0 = (ap_a + ap_d) >> 3;
        s1 = (ap_b + ap_c) >> 3;
        s2 = (ap_b - ap_c) >> 3;
        s3 = (ap_a - ap_d) >> 3;
        r0 = (ap_uint<WD_IQT + 2>)VCT_GET(ap_ref, 0 + i * 4, WD_PIX) + s0;
        r1 = (ap_uint<WD_IQT + 2>)VCT_GET(ap_ref, 1 + i * 4, WD_PIX) + s1;
        r2 = (ap_uint<WD_IQT + 2>)VCT_GET(ap_ref, 2 + i * 4, WD_PIX) + s2;
        r3 = (ap_uint<WD_IQT + 2>)VCT_GET(ap_ref, 3 + i * 4, WD_PIX) + s3;
        VCT_GET(ap_des, 0 + i * 4, WD_PIX) = (r0 < 0) ? 0 : (r0 > 255) ? 255 : r0(WD_PIX - 1, 0);
        VCT_GET(ap_des, 1 + i * 4, WD_PIX) = (r1 < 0) ? 0 : (r1 > 255) ? 255 : r1(WD_PIX - 1, 0);
        VCT_GET(ap_des, 2 + i * 4, WD_PIX) = (r2 < 0) ? 0 : (r2 > 255) ? 255 : r2(WD_PIX - 1, 0);
        VCT_GET(ap_des, 3 + i * 4, WD_PIX) = (r3 < 0) ? 0 : (r3 > 255) ? 255 : r3(WD_PIX - 1, 0);
    }
    return ap_des;
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void Pickup_Y16(ap_uint<WD_LMD> I__tlambda,            //              :
                ap_uint<WD_LMD> I__tlambda_m,          //
                ap_uint<WD_LMD> I__lambda_p16,         //
                hls_QMatrix I__hls_qm1,                // y44,y16
                hls_QMatrix I__hls_qm2,                // y16
                ap_int<WD_sharpen * 16> I__ap_sharpen, //
                // Parameters changed for each MB
                ap_uint<WD_PIX * 16> I__ap_y_in_[16], //
                ap_uint<1> I__istop,                  //
                ap_uint<1> I__isleft,                 //
                ap_uint<1> I__isright,                //
                // image context
                ap_uint<WD_PIX * 4> I__ap_y_top_c[4],  //
                ap_uint<WD_PIX * 4> I__ap_y_left_c[4], //
                ap_uint<WD_PIX> I__ap_y_m,             //
                // OUTPUT
                ap_uint<WD_PIX * 16> O__ap_y_out_cb[2][17],    //
                ap_int<WD_LEVEL * 16> O__ap_y_level_cb[2][17], //
                ap_int<WD_LEVEL * 16> O__ap_y16dc_level_cb[2], //
                str_rd O__rd_y16_cb[2],                        //
                ap_uint<WD_MODE>* OP_ap_y16_mode_c,            //
                ap_uint<2>* OP_b_y                             //
                ) {
    // Pickup Best Y16x16, less than 400 cycles
    *OP_b_y = 0;
PICKUP_Y16:
    for (int mode_p16 = 0; mode_p16 < 4; mode_p16++) {
//#pragma HLS DATAFLOW
#pragma HLS latency max = 80
        int mode_uv = mode_p16;
        ap_uint<25> nz_y16_tmp;
        O__rd_y16_cb[1 - (1 & (*OP_b_y))].score =
            hls_channel_p16(mode_p16, I__istop, I__isleft, I__ap_y_top_c, I__ap_y_left_c, I__ap_y_m, I__ap_y_in_,
                            I__hls_qm1, I__hls_qm2, I__ap_sharpen, I__tlambda, I__tlambda_m,
                            // OUTPUT
                            O__ap_y_level_cb[1 - (1 & *OP_b_y)], &O__ap_y16dc_level_cb[1 - (1 & *OP_b_y)],
                            O__ap_y_out_cb[1 - (1 & *OP_b_y)], &nz_y16_tmp);
        O__rd_y16_cb[1 - (1 & (*OP_b_y))].nz = &nz_y16_tmp;
        if (mode_p16 == 0 || O__rd_y16_cb[1 - (1 & (*OP_b_y))].score < O__rd_y16_cb[(1 & *OP_b_y)].score) {
            *OP_ap_y16_mode_c = mode_p16;
            *OP_b_y = 1 - (1 & *OP_b_y);
        }
    }
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

ap_uint<WD_RD_SCORE + 4> hls_channel_p16(ap_uint<4> mode_p16,
                                         ap_uint<1> istop,
                                         ap_uint<1> isleft,
                                         ap_uint<WD_PIX * 4> ap_y_top_c[4],
                                         ap_uint<WD_PIX * 4> ap_y_left_c[4],
                                         ap_uint<WD_PIX> ap_y_m,
                                         ap_uint<WD_PIX * 16> ap_yuv_in_[24],
                                         hls_QMatrix hls_qm1,
                                         hls_QMatrix hls_qm2,
                                         ap_int<WD_sharpen * 16> ap_sharpen,
                                         ap_uint<WD_LMD> tlambda,   //     = dqm->tlambda_;
                                         ap_uint<WD_LMD> tlambda_m, //   = dqm->lambda_mode_;
                                         ap_int<WD_LEVEL * 16> ap_y16_level_c[17],
                                         ap_int<WD_LEVEL * 16>* ap_y16dc_level_c,
                                         ap_uint<WD_PIX * 16> ap_y16_out_c[17],
                                         ap_uint<25>* nz) {
#pragma HLS INLINE

    str_dis rd_dis16;
    rd_dis16.init();
    ap_uint<WD_PIX * 16> ap_ref_p16[16];
    ap_int<WD_IWHT * 16> ap_iwht_dc;

    ap_int<WD_DCT * 16> ap_dct_out[16];
    ap_int<WD_DCT * 16> ap_wht_in;
    ap_int<WD_WHT * 16> ap_wht_out;
    ap_int<WD_IQT * 16> ap_iqt_ac[16];
    ap_int<WD_WHT * 16> ap_iqt_dc;

CHANNEL_P16_WHT:
    for (int n = 0; n < 16; n += 1) {
#pragma HLS PIPELINE
        ap_ref_p16[n] = hls_p16_test(mode_p16, n, istop, isleft, ap_y_top_c, ap_y_left_c, ap_y_m);
        ap_dct_out[n] = hls_FTransform(ap_yuv_in_[n], ap_ref_p16[n]);
        ap_uint<5> score_nz =
            hls_QuantizeBlock(ap_dct_out[n], &ap_y16_level_c[n], &ap_iqt_ac[n], &hls_qm1, ap_sharpen, 1);
        rd_dis16.nz |= (ap_uint<25>)((score_nz != 0) << n);
        VCT_GET(ap_wht_in, n, WD_DCT) = (ap_int<WD_DCT>)VCT_GET(ap_dct_out[n], 0, WD_DCT);
    } // for n
    ap_wht_out = hls_FTransformWHT(ap_wht_in);
    ap_int<WD_LEVEL * 16> tmp_level;
    rd_dis16.nz(24, 24) = hls_QuantizeBlockWHT(ap_wht_out, &tmp_level, &ap_iqt_dc, &hls_qm2);
    ap_y16_level_c[16] = tmp_level;
    ap_y16dc_level_c[0] = tmp_level;
    ap_iwht_dc = hls_ITransformWHT(ap_iqt_dc);
CHANNEL_P16_ONE:
    for (int n = 0; n < 16; n += 1) {
#pragma HLS PIPELINE
        ap_int<WD_IQT* 16> ap_dcac = ap_iqt_ac[n];
        VCT_GET(ap_dcac, 0, WD_IQT) = (ap_int<WD_IWHT>)VCT_GET(ap_iwht_dc, n, WD_IWHT);
        ap_y16_out_c[n] = hls_ITransformOne(ap_ref_p16[n], ap_dcac);
        rd_dis16.d += hls_SSE4X4(ap_yuv_in_[n], ap_y16_out_c[n]);
        rd_dis16.sd += hls_Disto4x4(ap_yuv_in_[n], ap_y16_out_c[n]);
        rd_dis16.r += hls_fast_cost(ap_y16_level_c[n], 2);
    }

    rd_dis16.r += hls_fast_cost(ap_y16dc_level_c[16 - 16], 1);
    rd_dis16.sd = (rd_dis16.sd * tlambda + 128) >> 8;
    const ap_uint<10> my_VP8FixedCostsI16[4] = {663, 919, 872, 919};
    rd_dis16.h = my_VP8FixedCostsI16[mode_p16];

    *nz = rd_dis16.nz;
    return hls_ca_score(tlambda_m, &rd_dis16, mode_p16);
}

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
/*Invers  Transforms */
ap_int<WD_IWHT * 16> hls_ITransformWHT(ap_int<WD_WHT * 16> in) {
// FF:0, lut:1248; 4.12+0.62ns; Latency:1
// input is 12b signed
#pragma HLS INLINE
    //#pragma HLS pipeline
    ap_int<WD_IWHT * 16> out;
    ap_int<WD_WHT + 2> tmp[16];
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_WHT + 1> a0 =
            ((ap_int<WD_WHT>)VCT_GET(in, 0 + i, WD_WHT) + (ap_int<WD_WHT>)VCT_GET(in, 12 + i, WD_WHT)); // 16b
        ap_int<WD_WHT + 1> a1 =
            ((ap_int<WD_WHT>)VCT_GET(in, 4 + i, WD_WHT) + (ap_int<WD_WHT>)VCT_GET(in, 8 + i, WD_WHT));
        ap_int<WD_WHT + 1> a2 =
            ((ap_int<WD_WHT>)VCT_GET(in, 4 + i, WD_WHT) - (ap_int<WD_WHT>)VCT_GET(in, 8 + i, WD_WHT));
        ap_int<WD_WHT + 1> a3 =
            ((ap_int<WD_WHT>)VCT_GET(in, 0 + i, WD_WHT) - (ap_int<WD_WHT>)VCT_GET(in, 12 + i, WD_WHT));
        tmp[0 + i] = a0 + a1; // 17b
        tmp[8 + i] = a0 - a1;
        tmp[4 + i] = a3 + a2;
        tmp[12 + i] = a3 - a2;
    }
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_WHT + 3> dc = (tmp[0 + i * 4] + 3);  // 18b
        ap_int<WD_WHT + 3> a0 = (dc + tmp[3 + i * 4]); // 18b
        ap_int<WD_WHT + 3> a1 = (tmp[1 + i * 4] + tmp[2 + i * 4]);
        ap_int<WD_WHT + 3> a2 = (tmp[1 + i * 4] - tmp[2 + i * 4]);
        ap_int<WD_WHT + 3> a3 = (dc - tmp[3 + i * 4]);
        ap_int<WD_WHT + 4> b0 = a0 + a1; // 19b
        ap_int<WD_WHT + 4> b1 = a3 + a2;
        ap_int<WD_WHT + 4> b2 = a0 - a1;
        ap_int<WD_WHT + 4> b3 = a3 - a2;
        VCT_GET(out, 0 + i * 4, WD_IWHT) = b0 >> 3; // 16b
        VCT_GET(out, 1 + i * 4, WD_IWHT) = b1 >> 3;
        VCT_GET(out, 2 + i * 4, WD_IWHT) = b2 >> 3;
        VCT_GET(out, 3 + i * 4, WD_IWHT) = b3 >> 3;
    }
    return out;
}

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

/* QUANTITION FOR DC */
ap_uint<1> hls_QuantizeBlockWHT_old(ap_int<WD_WHT * 16> in,
                                    ap_int<WD_LEVEL * 16>* out,
                                    ap_int<WD_WHT * 16>* out2,
                                    ap_uint<WD_q> q_0, // quantizer steps
                                    ap_uint<WD_q> q_n,
                                    ap_uint<WD_iq> iq_0, // reciprocals, fixed point.
                                    ap_uint<WD_iq> iq_n,
                                    ap_uint<WD_bias> bias_0, // rounding bias
                                    ap_uint<WD_bias> bias_n) // frequency boosters for slight sharpening
{
    ap_uint<1> last = 0;
    ap_int<WD_LEVEL * 16> xout;
    ap_int<WD_WHT * 16> xout2;
#pragma HLS pipeline
    for (int n = 0; n < 16; ++n) {
#pragma HLS unroll
        const ap_uint<4> j = ZIGZAG(n);
        const ap_int<WD_WHT> coeffs = (ap_int<WD_WHT>)VCT_GET(in, j, WD_WHT);
        const ap_uint<1> sign = coeffs[WD_WHT - 1];
        const ap_uint<WD_WHT - 1> coeff = (sign == 1 ? (ap_uint<WD_WHT - 1>)(-coeffs) : (ap_uint<WD_WHT - 1>)(coeffs));
        const ap_uint<WD_q> Q = n == 0 ? q_0 : q_n;
        const ap_uint<WD_iq> iQ = n == 0 ? iq_0 : iq_n;
        const ap_uint<WD_bias> B = n == 0 ? bias_0 : bias_n;
        ap_uint<WD_MLEVEL> level = (ap_uint<WD_MLEVEL>)((coeff * iQ + B) >> 17);
        if (level > MY_MAX_LEVEL) level = MY_MAX_LEVEL;
        if (sign) level = -level;
        ap_int<WD_WHT> rec = level * Q;
        VCT_GET((*out2), j, WD_WHT) = level * Q; //(ap_int<WD_WHT>)(level * Q);
        VCT_GET((xout2), j, WD_WHT) = level * Q; //(ap_int<WD_WHT>)(level * Q);
        VCT_GET((xout), n, WD_LEVEL) = level;    //(ap_int<WD_LEVEL>)level;
        VCT_GET((*out), n, WD_LEVEL) = level;    //(ap_int<WD_LEVEL>)level;
        if (level) last = 1;
    }
    return last;
}
ap_uint<1> hls_QuantizeBlockWHT(ap_int<WD_WHT * 16> in,
                                ap_int<WD_LEVEL * 16>* out,
                                ap_int<WD_WHT * 16>* out2,
                                hls_QMatrix* pQM) // frequency boosters for slight sharpening
{
    return hls_QuantizeBlockWHT_old(in, out, out2, pQM->q_0, pQM->q_n, pQM->iq_0, pQM->iq_n, pQM->bias_0, pQM->bias_n);
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

ap_uint<WD_PIX * 16> hls_DC16_4_y( // ref: lut:56, 3.19+1.25
    ap_uint<WD_PIX * 4> top0,
    ap_uint<WD_PIX * 4> top1,
    ap_uint<WD_PIX * 4> top2,
    ap_uint<WD_PIX * 4> top3,
    ap_uint<WD_PIX * 4> left0,
    ap_uint<WD_PIX * 4> left1,
    ap_uint<WD_PIX * 4> left2,
    ap_uint<WD_PIX * 4> left3,
    ap_uint<1> istop,
    ap_uint<1> isleft) {
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    ap_uint<WD_PIX + 5> DC = 0;
    ap_uint<WD_PIX> tmp;

    if (istop == 0) {
        DC += (AP_TREEADD4_VCT(top0, WD_PIX));
        DC += (AP_TREEADD4_VCT(top1, WD_PIX));
        DC += (AP_TREEADD4_VCT(top2, WD_PIX));
        DC += (AP_TREEADD4_VCT(top3, WD_PIX));
        if (isleft == 0) {
            DC += (AP_TREEADD4_VCT(left0, WD_PIX));
            DC += (AP_TREEADD4_VCT(left1, WD_PIX));
            DC += (AP_TREEADD4_VCT(left2, WD_PIX));
            DC += (AP_TREEADD4_VCT(left3, WD_PIX));
        } else
            DC += DC;
        DC = (DC + (8 << 1)) >> (4 + 1);
    } else if (isleft == 0) {
        DC += (AP_TREEADD4_VCT(left0, WD_PIX));
        DC += (AP_TREEADD4_VCT(left1, WD_PIX));
        DC += (AP_TREEADD4_VCT(left2, WD_PIX));
        DC += (AP_TREEADD4_VCT(left3, WD_PIX));
        DC += DC;
        DC = (DC + (8 << 1)) >> (4 + 1);
    } else
        DC = 0X80;
    tmp = DC(WD_PIX - 1, 0);
    SB_SET_LINE_VAL(sb, 0, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 1, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 2, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 3, WD_PIX, tmp);
    return sb;
};

ap_uint<WD_PIX * 16> hls_p16_test(ap_uint<2> mode,
                                  ap_uint<4> n,
                                  ap_uint<1> istop,
                                  ap_uint<1> isleft,
                                  ap_uint<WD_PIX * 4> ap_y_top_c[4],
                                  ap_uint<WD_PIX * 4> ap_y_left_c[4],
                                  ap_uint<WD_PIX> ap_y_m) {
#pragma HLS PIPELINE
    // ap_uint<WD_PIX*4>* top  = ap_y_top_c;
    // ap_uint<WD_PIX*4>* left = ap_y_left_c;
    ap_uint<WD_PIX* 4> abcd = ap_y_top_c[n & 3];
    ap_uint<WD_PIX* 4> ijkl = ap_y_left_c[n >> 2];
    ap_uint<WD_PIX> X44 = ap_y_m;
    switch (mode) {
        case B_DC_PRED:
            return hls_DC16_4_y(ap_y_top_c[0], ap_y_top_c[1], ap_y_top_c[2], ap_y_top_c[3], ap_y_left_c[0],
                                ap_y_left_c[1], ap_y_left_c[2], ap_y_left_c[3], istop, isleft);
        case B_TM_PRED:
            return hls_TM16_4(abcd, ijkl, X44, istop, isleft);
        case B_VE_PRED:
            return hls_VE16_4(abcd, istop);
        default:
            return hls_HE16_4(ijkl, isleft);
    } // case
}

ap_uint<WD_RD_SCORE + 4> hls_ca_score(ap_uint<WD_LMD> lmbda, str_dis* dis, ap_uint<4> m) {
#pragma HLS PIPELINE
    return (((ap_uint<WD_RD_SCORE + 4>)(dis->d + (ap_uint<WD_SSE4 + 4>)(dis->sd))) << 8) +
           ((ap_uint<WD_RD_SCORE + 4>)(dis->r + dis->h)) * lmbda;
}; // ca_score

//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

void Pickup_UV(
    // Parameters unParameters changed for one picture/segment
    ap_uint<WD_LMD> I__tlambda,               //              :
    ap_uint<WD_LMD> I__tlambda_m,             //
    ap_uint<WD_LMD> I__lambda_uv,             //
    hls_QMatrix I__hls_qm_uv,                 //
    ap_int<WD_sharpen * 16> I__ap_sharpen_uv, //
    // Parameters changed for each MB
    ap_uint<WD_PIX * 16> I__ap_uv_in_[8], //
    ap_uint<1> I__istop,                  //
    ap_uint<1> I__isleft,                 //
    ap_uint<1> I__isright,                //
    // image context
    ap_uint<WD_PIX * 4> I__ap_uv_top_c[4],          //
    ap_uint<WD_PIX * 4> I__ap_uv_left_c[4],         //
    ap_uint<WD_PIX> I__ap_u_m,                      //
    ap_uint<WD_PIX> I__ap_v_m,                      //
    ap_uint<WD_PIX * 16> O__ap_uv_out_cb[2][17],    //
    ap_int<WD_LEVEL * 16> O__ap_uv_level_cb[2][16], //
    str_rd O__rd_uv_cb[2],                          //
    ap_uint<WD_MODE>* OP_ap_uv_mode_c,              //
    ap_uint<1>* OP_b_uv                             //
    ) {
    // Pickup Best Y16x16, less than 400 cycles
    *OP_b_uv = 0;
PICKUP_UV:
    for (int mode_uv = 0; mode_uv < 4; mode_uv++) {
        // Pickup Best UV, less than 400 cycles
        ap_uint<25> nz_tmp;
        O__rd_uv_cb[1 - *OP_b_uv].score =
            hls_channel_uv_8(mode_uv, I__istop, I__isleft, I__ap_uv_top_c, I__ap_uv_left_c, I__ap_u_m, I__ap_v_m,
                             I__ap_uv_in_, I__hls_qm_uv, I__ap_sharpen_uv, I__lambda_uv,
                             // OUTPUT
                             O__ap_uv_level_cb[1 - *OP_b_uv], O__ap_uv_out_cb[1 - *OP_b_uv], &nz_tmp);
        O__rd_uv_cb[1 - *OP_b_uv].nz = nz_tmp;
        if (mode_uv == 0 || O__rd_uv_cb[1 - *OP_b_uv].score < O__rd_uv_cb[*OP_b_uv].score) {
            *OP_ap_uv_mode_c = mode_uv;
            *OP_b_uv = 1 - *OP_b_uv;
        }
    }
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

ap_uint<WD_RD_SCORE + 4> hls_channel_uv_8(ap_uint<4> mode_uv,
                                          ap_uint<1> istop,
                                          ap_uint<1> isleft,
                                          ap_uint<WD_PIX * 4> ap_uv_top_c[4],
                                          ap_uint<WD_PIX * 4> ap_uv_left_c[4],
                                          ap_uint<WD_PIX> ap_u_m,
                                          ap_uint<WD_PIX> ap_v_m,
                                          ap_uint<WD_PIX * 16> ap_uv_in_[8],
                                          hls_QMatrix hls_qm_uv,
                                          ap_int<WD_sharpen * 16> ap_sharpen_uv,
                                          ap_uint<WD_LMD> lambda_uv, //     = dqm->tlambda_;
                                          ap_int<WD_LEVEL * 16> ap_uv_level_c[8],
                                          ap_uint<WD_PIX * 16> ap_uv_out_c[8],
                                          ap_uint<25>* nz) {
#pragma HLS INLINE
    str_dis rd_dis;
    const ap_uint<10> my_VP8FixedCostsUV[4] = {302, 984, 439, 642};
    ap_uint<WD_PIX * 16> ap_ref_uv;
    ap_int<WD_IQT * 16> ap_iqt_uv;
    ap_int<WD_DCT * 16> ap_dct_out;
    ap_uint<5> score_nz;
    rd_dis.init();
    rd_dis.h = my_VP8FixedCostsUV[mode_uv];
CHANNEL_UV_8:
    for (int n = 0; n < 8; n += 1) {
#pragma HLS PIPELINE
        ap_ref_uv = hls_p8_test(mode_uv, n, istop, isleft, ap_uv_top_c, ap_uv_left_c, ap_u_m, ap_v_m);
        ap_dct_out = hls_FTransform(ap_uv_in_[n], ap_ref_uv);
        score_nz = hls_QuantizeBlock(ap_dct_out, &ap_uv_level_c[n], &ap_iqt_uv, &hls_qm_uv, ap_sharpen_uv, 0);
        ap_uv_out_c[n] = hls_ITransformOne(ap_ref_uv, ap_iqt_uv);
        rd_dis.nz |= (ap_uint<25>)((score_nz != 0) << (n + 16));
        rd_dis.d += hls_SSE4X4(ap_uv_in_[n], ap_uv_out_c[n]);
        rd_dis.r += hls_fast_cost(ap_uv_level_c[n], 2);
    }
    *nz = rd_dis.nz;
    return hls_ca_score(lambda_uv, &rd_dis, mode_uv);
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
/*******************************************/
/* Prediction img Generation: Y44          */
/*******************************************/
/* 1 */
ap_uint<WD_PIX * 16> hls_DC4( // ref:581  lut 56, 4.46+1.25 ,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> ijkl //,
    ) {                      //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    ap_uint<WD_PIX> tmp = ap_uint<WD_PIX>(
        (AP_TREEADD2((AP_TREEADD4_VCT(abcd, WD_PIX)), (AP_TREEADD4_VCT(ijkl, WD_PIX)), (WD_PIX + 2)) + 4) >> 3);
    SB_SET_LINE_VAL(sb, 0, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 1, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 2, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 3, WD_PIX, tmp);
    return sb;
};

/* 2 */
ap_uint<WD_PIX * 16> hls_VE4( // ref: lut:56, 3.19+1.25//lut 452vs997, 2.72+1.25ns,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> efgh,
    ap_uint<WD_PIX> X44) { // vertical
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    const ap_uint<WD_PIX> val0(ap_uint<WD_PIX>(AP_AVG3(X44, A44, B44, WD_PIX)));
    const ap_uint<WD_PIX> val1(ap_uint<WD_PIX>(AP_AVG3(A44, B44, C44, WD_PIX)));
    const ap_uint<WD_PIX> val2(ap_uint<WD_PIX>(AP_AVG3(B44, C44, D44, WD_PIX)));
    const ap_uint<WD_PIX> val3(ap_uint<WD_PIX>(AP_AVG3(C44, D44, E44, WD_PIX)));

    SB_SET_COL_VAL(sb, 0, WD_PIX, val0);
    SB_SET_COL_VAL(sb, 1, WD_PIX, val1);
    SB_SET_COL_VAL(sb, 2, WD_PIX, val2);
    SB_SET_COL_VAL(sb, 3, WD_PIX, val3);
    return sb;
};

/* 3 */
ap_uint<WD_PIX * 16> hls_HE4( // ref: lut:56, 3.19+1.25
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<WD_PIX> X44) {
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    SB_SET_LINE_VAL(sb, 0, WD_PIX, ap_uint<WD_PIX>(AP_AVG3(X44, I44, J44, WD_PIX)));
    SB_SET_LINE_VAL(sb, 1, WD_PIX, ap_uint<WD_PIX>(AP_AVG3(I44, J44, K44, WD_PIX)));
    SB_SET_LINE_VAL(sb, 2, WD_PIX, ap_uint<WD_PIX>(AP_AVG3(J44, K44, L44, WD_PIX)));
    SB_SET_LINE_VAL(sb, 3, WD_PIX, ap_uint<WD_PIX>(AP_AVG3(K44, L44, L44, WD_PIX)));
    return sb;
};

/* 4 */
ap_uint<WD_PIX * 16> hls_RD4( // ref: lut:98  3.19+1.25, ,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<WD_PIX> X44) {
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    AP_DST(sb, 0, 3, WD_PIX) = AP_AVG3(J44, K44, L44, WD_PIX);
    AP_DST(sb, 0, 2, WD_PIX) = AP_DST(sb, 1, 3, WD_PIX) = AP_AVG3(I44, J44, K44, WD_PIX);
    AP_DST(sb, 0, 1, WD_PIX) = AP_DST(sb, 1, 2, WD_PIX) = AP_DST(sb, 2, 3, WD_PIX) = AP_AVG3(X44, I44, J44, WD_PIX);
    AP_DST(sb, 0, 0, WD_PIX) = AP_DST(sb, 1, 1, WD_PIX) = AP_DST(sb, 2, 2, WD_PIX) = AP_DST(sb, 3, 3, WD_PIX) =
        AP_AVG3(A44, X44, I44, WD_PIX);
    AP_DST(sb, 1, 0, WD_PIX) = AP_DST(sb, 2, 1, WD_PIX) = AP_DST(sb, 3, 2, WD_PIX) = AP_AVG3(B44, A44, X44, WD_PIX);
    AP_DST(sb, 2, 0, WD_PIX) = AP_DST(sb, 3, 1, WD_PIX) = AP_AVG3(C44, B44, A44, WD_PIX);
    AP_DST(sb, 3, 0, WD_PIX) = AP_AVG3(D44, C44, B44, WD_PIX);
    return sb;
};

/* 5 */
ap_uint<WD_PIX * 16> hls_LD4( // ref: lut:98  3.19+1.25  , ,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> efgh) { //
#pragma HLS PIPELINE

    ap_uint<WD_PIX * 16> sb;
    AP_DST(sb, 0, 0, WD_PIX) = AP_AVG3(A44, B44, C44, WD_PIX);
    AP_DST(sb, 1, 0, WD_PIX) = AP_DST(sb, 0, 1, WD_PIX) = AP_AVG3(B44, C44, D44, WD_PIX);
    AP_DST(sb, 2, 0, WD_PIX) = AP_DST(sb, 1, 1, WD_PIX) = AP_DST(sb, 0, 2, WD_PIX) = AP_AVG3(C44, D44, E44, WD_PIX);
    AP_DST(sb, 3, 0, WD_PIX) = AP_DST(sb, 2, 1, WD_PIX) = AP_DST(sb, 1, 2, WD_PIX) = AP_DST(sb, 0, 3, WD_PIX) =
        AP_AVG3(D44, E44, F44, WD_PIX);
    AP_DST(sb, 3, 1, WD_PIX) = AP_DST(sb, 2, 2, WD_PIX) = AP_DST(sb, 1, 3, WD_PIX) = AP_AVG3(E44, F44, G44, WD_PIX);
    AP_DST(sb, 3, 2, WD_PIX) = AP_DST(sb, 2, 3, WD_PIX) = AP_AVG3(F44, G44, H44, WD_PIX);
    AP_DST(sb, 3, 3, WD_PIX) = AP_AVG3(G44, H44, H44, WD_PIX);
    return sb;
};

/* 6 */
ap_uint<WD_PIX * 16> hls_VR4( // ref: lut: 100  3.19+1.25 , ,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<WD_PIX> X44) { //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    AP_DST(sb, 0, 0, WD_PIX) = AP_DST(sb, 1, 2, WD_PIX) = AP_AVG2(X44, A44, WD_PIX);
    AP_DST(sb, 1, 0, WD_PIX) = AP_DST(sb, 2, 2, WD_PIX) = AP_AVG2(A44, B44, WD_PIX);
    AP_DST(sb, 2, 0, WD_PIX) = AP_DST(sb, 3, 2, WD_PIX) = AP_AVG2(B44, C44, WD_PIX);
    AP_DST(sb, 3, 0, WD_PIX) = AP_AVG2(C44, D44, WD_PIX);

    AP_DST(sb, 0, 3, WD_PIX) = AP_AVG3(K44, J44, I44, WD_PIX);
    AP_DST(sb, 0, 2, WD_PIX) = AP_AVG3(J44, I44, X44, WD_PIX);
    AP_DST(sb, 0, 1, WD_PIX) = AP_DST(sb, 1, 3, WD_PIX) = AP_AVG3(I44, X44, A44, WD_PIX);
    AP_DST(sb, 1, 1, WD_PIX) = AP_DST(sb, 2, 3, WD_PIX) = AP_AVG3(X44, A44, B44, WD_PIX);
    AP_DST(sb, 2, 1, WD_PIX) = AP_DST(sb, 3, 3, WD_PIX) = AP_AVG3(A44, B44, C44, WD_PIX);
    AP_DST(sb, 3, 1, WD_PIX) = AP_AVG3(B44, C44, D44, WD_PIX);
    return sb;
};

/* 7 */
ap_uint<WD_PIX * 16> hls_VL4( // ref: lut: 100 3.19+1.25 , ,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> efgh) { //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    AP_DST(sb, 0, 0, WD_PIX) = AP_AVG2(A44, B44, WD_PIX);
    AP_DST(sb, 1, 0, WD_PIX) = AP_DST(sb, 0, 2, WD_PIX) = AP_AVG2(B44, C44, WD_PIX);
    AP_DST(sb, 2, 0, WD_PIX) = AP_DST(sb, 1, 2, WD_PIX) = AP_AVG2(C44, D44, WD_PIX);
    AP_DST(sb, 3, 0, WD_PIX) = AP_DST(sb, 2, 2, WD_PIX) = AP_AVG2(D44, E44, WD_PIX);

    AP_DST(sb, 0, 1, WD_PIX) = AP_AVG3(A44, B44, C44, WD_PIX);
    AP_DST(sb, 1, 1, WD_PIX) = AP_DST(sb, 0, 3, WD_PIX) = AP_AVG3(B44, C44, D44, WD_PIX);
    AP_DST(sb, 2, 1, WD_PIX) = AP_DST(sb, 1, 3, WD_PIX) = AP_AVG3(C44, D44, E44, WD_PIX);
    AP_DST(sb, 3, 1, WD_PIX) = AP_DST(sb, 2, 3, WD_PIX) = AP_AVG3(D44, E44, F44, WD_PIX);
    AP_DST(sb, 3, 2, WD_PIX) = AP_AVG3(E44, F44, G44, WD_PIX);
    AP_DST(sb, 3, 3, WD_PIX) = AP_AVG3(F44, G44, H44, WD_PIX);
    return sb;
};

/* 8 */
ap_uint<WD_PIX * 16> hls_HU4(   // ref: lut 54 3.19+1.25 , ,
    ap_uint<WD_PIX * 4> ijkl) { //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    AP_DST(sb, 0, 0, WD_PIX) = AP_AVG2(I44, J44, WD_PIX);
    AP_DST(sb, 2, 0, WD_PIX) = AP_DST(sb, 0, 1, WD_PIX) = AP_AVG2(J44, K44, WD_PIX);
    AP_DST(sb, 2, 1, WD_PIX) = AP_DST(sb, 0, 2, WD_PIX) = AP_AVG2(K44, L44, WD_PIX);
    AP_DST(sb, 1, 0, WD_PIX) = AP_AVG3(I44, J44, K44, WD_PIX);
    AP_DST(sb, 3, 0, WD_PIX) = AP_DST(sb, 1, 1, WD_PIX) = AP_AVG3(J44, K44, L44, WD_PIX);
    AP_DST(sb, 3, 1, WD_PIX) = AP_DST(sb, 1, 2, WD_PIX) = AP_AVG3(K44, L44, L44, WD_PIX);
    AP_DST(sb, 3, 2, WD_PIX) = AP_DST(sb, 2, 2, WD_PIX) = AP_DST(sb, 0, 3, WD_PIX) = AP_DST(sb, 1, 3, WD_PIX) =
        AP_DST(sb, 2, 3, WD_PIX) = AP_DST(sb, 3, 3, WD_PIX) = L44;
    return sb;
};

/* 9 */
ap_uint<WD_PIX * 16> hls_HD4( // ref:544 vs lut:100 ,3.19+1.25 ,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<WD_PIX> X44) { //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    AP_DST(sb, 0, 0, WD_PIX) = AP_DST(sb, 2, 1, WD_PIX) = AP_AVG2(I44, X44, WD_PIX);
    AP_DST(sb, 0, 1, WD_PIX) = AP_DST(sb, 2, 2, WD_PIX) = AP_AVG2(J44, I44, WD_PIX);
    AP_DST(sb, 0, 2, WD_PIX) = AP_DST(sb, 2, 3, WD_PIX) = AP_AVG2(K44, J44, WD_PIX);
    AP_DST(sb, 0, 3, WD_PIX) = AP_AVG2(L44, K44, WD_PIX);

    AP_DST(sb, 3, 0, WD_PIX) = AP_AVG3(A44, B44, C44, WD_PIX);
    AP_DST(sb, 2, 0, WD_PIX) = AP_AVG3(X44, A44, B44, WD_PIX);
    AP_DST(sb, 1, 0, WD_PIX) = AP_DST(sb, 3, 1, WD_PIX) = AP_AVG3(I44, X44, A44, WD_PIX);
    AP_DST(sb, 1, 1, WD_PIX) = AP_DST(sb, 3, 2, WD_PIX) = AP_AVG3(J44, I44, X44, WD_PIX);
    AP_DST(sb, 1, 2, WD_PIX) = AP_DST(sb, 3, 3, WD_PIX) = AP_AVG3(K44, J44, I44, WD_PIX);
    AP_DST(sb, 1, 3, WD_PIX) = AP_AVG3(L44, K44, J44, WD_PIX);

    return sb;
};

/* 10 */
ap_uint<WD_PIX * 16> hls_TM4( // ref: lut 516, 4.07+1.25,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<WD_PIX> X44) { //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    ap_int<WD_PIX + 2> tmp;
    for (int i = 0; i < 4; i++)
#pragma HLS unroll
        for (int j = 0; j < 4; j++) {
#pragma HLS unroll
            tmp = AP_TREEADD2((VCT_GET(abcd, j, WD_PIX)), (VCT_GET(ijkl, i, WD_PIX)), WD_PIX) - X44;
            if (tmp > 255)
                tmp = 255;
            else if (tmp < 0)
                tmp = 0;
            SB_GET(sb, i, j, WD_PIX) = tmp;
        }
    return sb;
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_PIX * 16> hls_p8_test(ap_uint<2> mode,
                                 ap_uint<3> n,
                                 ap_uint<1> istop,
                                 ap_uint<1> isleft,
                                 ap_uint<WD_PIX * 4> ap_uv_top_c[4],
                                 ap_uint<WD_PIX * 4> ap_uv_left_c[4],
                                 ap_uint<WD_PIX> ap_u_m,
                                 ap_uint<WD_PIX> ap_v_m) {
    ap_uint<WD_PIX * 4> top[2];
    ap_uint<WD_PIX * 4> left[2];
    ap_uint<WD_PIX * 4> abcd;
    ap_uint<WD_PIX * 4> ijkl;
    ap_uint<WD_PIX> X44;
    if (n < 4) {
        top[0] = ap_uv_top_c[0];
        top[1] = ap_uv_top_c[1];
        left[0] = ap_uv_left_c[0];
        left[1] = ap_uv_left_c[1];
        abcd = ap_uv_top_c[n & 1];
        ijkl = ap_uv_left_c[n >> 1];
        X44 = ap_u_m;
    } else {
        n -= 4;
        top[0] = ap_uv_top_c[2];
        top[1] = ap_uv_top_c[3];
        left[0] = ap_uv_left_c[2];
        left[1] = ap_uv_left_c[3];
        abcd = ap_uv_top_c[2 + (n & 1)];
        ijkl = ap_uv_left_c[2 + (n >> 1)];
        X44 = ap_v_m;
    }
    switch (mode) {
        case B_DC_PRED:
            return hls_DC16_4_uv_old(top, left, istop, isleft);
        case B_TM_PRED:
            return hls_TM16_4(abcd, ijkl, X44, istop, isleft);
        case B_VE_PRED:
            return hls_VE16_4(abcd, istop);
        default:
            return hls_HE16_4(ijkl, isleft);
    } // case
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_PIX * 16> hls_DC16_4_uv_old( // ref: lut:56, 3.19+1.25
    ap_uint<WD_PIX * 4> top[2],
    ap_uint<WD_PIX * 4> left[2],
    ap_uint<1> istop,
    ap_uint<1> isleft) {
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    ap_uint<WD_PIX + 5> DC = 0;
    ap_uint<WD_PIX> tmp;

    if (istop == 0) {
        for (int j = 0; j < (2 << 0); ++j) DC += (AP_TREEADD4_VCT(top[j], WD_PIX));
        if (isleft == 0)
            for (int j = 0; j < (2 << 0); ++j) DC += (AP_TREEADD4_VCT(left[j], WD_PIX));
        else
            DC += DC;
        DC = (DC + (8 << 0)) >> (4 + 0);
    } else if (isleft == 0) {
        for (int j = 0; j < (2 << 0); ++j) DC += (AP_TREEADD4_VCT(left[j], WD_PIX));
        DC += DC;
        DC = (DC + (8 << 0)) >> (4 + 0);
    } else
        DC = 0X80;
    tmp = DC(WD_PIX - 1, 0);
    SB_SET_LINE_VAL(sb, 0, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 1, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 2, WD_PIX, tmp);
    SB_SET_LINE_VAL(sb, 3, WD_PIX, tmp);
    return sb;
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_PIX * 16> hls_TM16_4( // ref: lut:56, 3.19+1.25
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<WD_PIX> X44,
    ap_uint<1> istop,
    ap_uint<1> isleft) { //
#pragma HLS PIPELINE
    if (isleft == 0) {
        if (istop == 0)
            return hls_TM4(abcd, ijkl, X44);
        else
            return hls_HE16_4(ijkl, isleft);
    } else {
        if (istop == 0)
            return hls_VE16_4(abcd, istop);
        else
            return hls_HE16_4(ijkl, isleft);
    }
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_PIX * 16> hls_VE16_4( // ref: lut:56, 3.19+1.25//lut 452vs997, 2.72+1.25ns,
    ap_uint<WD_PIX * 4> abcd,
    ap_uint<1> istop) { // vertical
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    const ap_uint<WD_PIX> val0 = (istop != 1) ? (A44) : 127;
    const ap_uint<WD_PIX> val1 = (istop != 1) ? (B44) : 127;
    const ap_uint<WD_PIX> val2 = (istop != 1) ? (C44) : 127;
    const ap_uint<WD_PIX> val3 = (istop != 1) ? (D44) : 127;

    SB_SET_COL_VAL(sb, 0, WD_PIX, val0);
    SB_SET_COL_VAL(sb, 1, WD_PIX, val1);
    SB_SET_COL_VAL(sb, 2, WD_PIX, val2);
    SB_SET_COL_VAL(sb, 3, WD_PIX, val3);
    return sb;
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_uint<WD_PIX * 16> hls_HE16_4( // ref: lut:56, 3.19+1.25
    ap_uint<WD_PIX * 4> ijkl,
    ap_uint<1> isleft) { //
#pragma HLS PIPELINE
    ap_uint<WD_PIX * 16> sb;
    const ap_uint<WD_PIX> val0 = (isleft != 1) ? (I44) : 129;
    const ap_uint<WD_PIX> val1 = (isleft != 1) ? (J44) : 129;
    const ap_uint<WD_PIX> val2 = (isleft != 1) ? (K44) : 129;
    const ap_uint<WD_PIX> val3 = (isleft != 1) ? (L44) : 129;
    SB_SET_LINE_VAL(sb, 0, WD_PIX, val0);
    SB_SET_LINE_VAL(sb, 1, WD_PIX, val1);
    SB_SET_LINE_VAL(sb, 2, WD_PIX, val2);
    SB_SET_LINE_VAL(sb, 3, WD_PIX, val3);
    return sb;
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
ap_int<WD_WHT * 16> hls_FTransformWHT(ap_int<WD_DCT * 16> in) {
    // FF:0, lut:1248; 4.12+0.62ns; Latency:1
    // input is 12b signed
    ap_int<WD_WHT * 16> out;
    ap_int<WD_DCT + 2> tmp[16];
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_DCT + 1> a0 =
            ((ap_int<WD_DCT>)VCT_GET(in, 0 + i * 4, WD_DCT) + (ap_int<WD_DCT>)VCT_GET(in, 2 + i * 4, WD_DCT)); // 13b
        ap_int<WD_DCT + 1> a1 =
            ((ap_int<WD_DCT>)VCT_GET(in, 1 + i * 4, WD_DCT) + (ap_int<WD_DCT>)VCT_GET(in, 3 + i * 4, WD_DCT));
        ap_int<WD_DCT + 1> a2 =
            ((ap_int<WD_DCT>)VCT_GET(in, 1 + i * 4, WD_DCT) - (ap_int<WD_DCT>)VCT_GET(in, 3 + i * 4, WD_DCT));
        ap_int<WD_DCT + 1> a3 =
            ((ap_int<WD_DCT>)VCT_GET(in, 0 + i * 4, WD_DCT) - (ap_int<WD_DCT>)VCT_GET(in, 2 + i * 4, WD_DCT));
        tmp[0 + i * 4] = a0 + a1; // 14b
        tmp[1 + i * 4] = a3 + a2;
        tmp[2 + i * 4] = a3 - a2;
        tmp[3 + i * 4] = a0 - a1;
    }
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        ap_int<WD_DCT + 3> a0 = (tmp[0 + i] + tmp[8 + i]); // 15b
        ap_int<WD_DCT + 3> a1 = (tmp[4 + i] + tmp[12 + i]);
        ap_int<WD_DCT + 3> a2 = (tmp[4 + i] - tmp[12 + i]);
        ap_int<WD_DCT + 3> a3 = (tmp[0 + i] - tmp[8 + i]);
        ap_int<WD_DCT + 4> b0 = a0 + a1; // 16b
        ap_int<WD_DCT + 4> b1 = a3 + a2;
        ap_int<WD_DCT + 4> b2 = a3 - a2;
        ap_int<WD_DCT + 4> b3 = a0 - a1;
        VCT_GET(out, 0 + i, WD_WHT) = b0 >> 1; // 15b
        VCT_GET(out, 4 + i, WD_WHT) = b1 >> 1;
        VCT_GET(out, 8 + i, WD_WHT) = b2 >> 1;
        VCT_GET(out, 12 + i, WD_WHT) = b3 >> 1;
    }
    return out;
}
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////
void hls_SetBestAs4_mode_widen(ap_uint<WD_MODE> ap_y_top_mode[MAX_NUM_MB_W * 4],
                               ap_uint<WD_MODE> ap_y_left_mode[4],
                               ap_uint<WD_MODE> ap_y4_top_c_mode[16],
                               ap_uint<WD_MODE> ap_y_left_c_mode[4],
                               ap_uint<WD_MODE * 16>* ap_y_mode_b,
                               ap_uint<LG2_MAX_NUM_MB_W + 2> x_sb_w) {
    for (int y = 0; y < 4; y++) {
#pragma HLS unroll
        for (int x = 0; x < 4; x++) {
#pragma HLS unroll
            SB_GET((*ap_y_mode_b), y, x, WD_MODE) = ap_y4_top_c_mode[x + y * 4];
            if (x == 3) ap_y_left_mode[y] = ap_y4_top_c_mode[x + y * 4];
            if (y == 3) ap_y_top_mode[x_sb_w + x] = ap_y4_top_c_mode[x + y * 4];
        }
    }
};
//////////=====================   TopVp8_compute_NoOut==========================/////////////////////////////

void hls_SetBestAs16_mode_widen(ap_uint<WD_MODE> ap_y_top_mode[MAX_NUM_MB_W * 4],
                                ap_uint<WD_MODE> ap_y_left_mode[4],
                                ap_uint<WD_MODE> ap_y16_mode_c,
                                ap_uint<WD_MODE * 16>* ap_y_mode_b,
                                ap_uint<LG2_MAX_NUM_MB_W + 2> x_sb_w) {
    for (int y = 0; y < 4; y++) {
#pragma HLS unroll
        for (int x = 0; x < 4; x++) {
#pragma HLS unroll
            SB_GET((*ap_y_mode_b), y, x, WD_MODE) = ap_y16_mode_c; // it->ap_rd_y16_b->mode;
            if (x == 3) ap_y_left_mode[y] = ap_y16_mode_c;         // it->ap_rd_y16_b->mode;
            if (y == 3) ap_y_top_mode[x_sb_w + x] = ap_y16_mode_c; // it->ap_rd_y16_b->mode;
        }
    }
};
//////////======================================================================/////////////////////////////
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////
//////////======================================================================/////////////////////////////
void TopVp8_RecordCoeff_hls_cnt(ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                                ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                                hls::stream<ap_int<64> >* str_pred,
                                hls::stream<ap_int<6> >* str_ret,
                                // output
                                hls::stream<ap_uint<1> >& str_mb_type,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc2,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y2,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv2,
                                hls::stream<ap_int<64> >* str_pred2,
                                hls::stream<ap_int<6> >* str_ret2,
                                hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_dc,
                                hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_ac,
                                hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_uv,
                                hls::stream<ap_uint<8> >& str_cnt_dc,
                                hls::stream<ap_uint<8> >& str_cnt_ac,
                                hls::stream<ap_uint<8> >& str_cnt_uv) {
    hls::stream<ap_uint<2> > str_dc_ctx;
    hls::stream<ap_uint<2> > str_ac_ctx;
    hls::stream<ap_uint<2> > str_uv_ctx;
    hls::stream<ap_int<5> > str_dc_last;
    hls::stream<ap_int<5> > str_ac_last;
    hls::stream<ap_int<5> > str_uv_last;
    hls::stream<ap_int<WD_LEVEL> > str_dc;
    hls::stream<ap_int<WD_LEVEL> > str_ac;
    hls::stream<ap_int<WD_LEVEL> > str_uv;

    ap_NoneZero ap_nz;
    ap_uint<9> left_nz_dc = 0;
    ap_uint<9> ap_left_nz = 0;
    for (int y_mb = 0; y_mb < mb_h; y_mb++) { // printf("\ny=%2d: ", y_mb);
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
#pragma HLS PIPELINE off
    RECORD_COEFF:
        for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
#pragma HLS PIPELINE
            if (x_mb == 0) {
                left_nz_dc = 0;
                ap_left_nz = 0;
            }
            // loading the constx about nz
            ap_uint<9> ap_top_nz = ap_nz.load_top9(x_mb, y_mb);
            // ap_uint<9> ap_left_nz = ap_nz.load_left9(x_mb);

            // ap_uint<9> top_nz_ = ap_top_nz;
            ap_uint<9> top_nz_dc = ap_top_nz;
            ap_uint<9> top_nz_y = ap_top_nz;
            ap_uint<9> top_nz_uv = ap_top_nz;

            // ap_uint<9> left_nz_ = ap_left_nz;
            ap_uint<9> left_nz_y = ap_left_nz;
            ap_uint<9> left_nz_uv = ap_left_nz;

            left_nz_dc =
                RecordCoeff_dataflow(str_level_dc, str_level_y, str_level_uv, str_pred, str_ret,
                                     // output
                                     str_mb_type, str_level_dc2, str_level_y2, str_level_uv2, str_pred2, str_ret2,
                                     str_rec_dc, str_rec_ac, str_rec_uv, str_cnt_dc, str_cnt_ac, str_cnt_uv,
                                     top_nz_dc,  //
                                     left_nz_dc, // = ap_left_nz;
                                     top_nz_y,   // = ap_top_nz;
                                     left_nz_y,  // = ap_left_nz;
                                     top_nz_uv,  // = ap_top_nz;
                                     left_nz_uv  // = ap_left_nz;
                                     );
            top_nz_dc[8] = left_nz_dc[0];
            left_nz_dc[0] = 0;
            ap_uint<9> top_nz_;
            top_nz_(3, 0) = top_nz_y(3, 0);
            top_nz_(7, 4) = top_nz_uv(7, 4);
            top_nz_[8] = top_nz_dc[8];

            ap_uint<9> left_nz_;
            left_nz_(3, 0) = left_nz_y(3, 0);
            left_nz_(7, 4) = left_nz_uv(7, 4);
            ap_uint<25> nz = 0;
            nz |= (ap_uint<25>)((top_nz_[0] << 12) | (top_nz_[1] << 13));
            nz |= (ap_uint<25>)((top_nz_[2] << 14) | (top_nz_[3] << 15));
            nz |= (ap_uint<25>)((top_nz_[4] << 18) | (top_nz_[5] << 19));
            nz |= (ap_uint<25>)((top_nz_[6] << 22) | (top_nz_[7] << 23));
            nz |= (ap_uint<25>)((top_nz_[8] << 24)); // we propagate the _top_ bit, esp. for intra4
            // left
            nz |= (ap_uint<25>)((left_nz_[0] << 3) | (left_nz_[1] << 7));
            nz |= (ap_uint<25>)((left_nz_[2] << 11));
            nz |= (ap_uint<25>)((left_nz_[4] << 17) | (left_nz_[6] << 21));

            ap_nz.left_nz[8] = left_nz_[8];
            ap_nz.nz_current = nz; //*it->nz_;
            ap_nz.store_nz(x_mb);

            ap_left_nz[0] = nz(3, 3);
            ap_left_nz[1] = nz(7, 7);
            ap_left_nz[2] = nz(11, 11);
            ap_left_nz[3] = nz(15, 15);
            // left-U
            ap_left_nz[4] = nz(17, 17);
            ap_left_nz[5] = nz(19, 19);
            // left-V
            ap_left_nz[6] = nz(21, 21);
            ap_left_nz[7] = nz(23, 23);
        }
    }
}

//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////
ap_uint<9> RecordCoeff_dataflow(hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                                hls::stream<ap_int<64> >* str_pred,
                                hls::stream<ap_int<6> >* str_ret,
                                // output
                                hls::stream<ap_uint<1> >& str_mb_type,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc2,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y2,
                                hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv2,
                                hls::stream<ap_int<64> >* str_pred2,
                                hls::stream<ap_int<6> >* str_ret2,
                                hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_dc,
                                hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_ac,
                                hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_uv,
                                hls::stream<ap_uint<8> >& str_cnt_dc,
                                hls::stream<ap_uint<8> >& str_cnt_ac,
                                hls::stream<ap_uint<8> >& str_cnt_uv,
                                ap_uint<9>& top_nz_dc, //
                                ap_uint<9> left_nz_dc, // = ap_left_nz;
                                ap_uint<9>& top_nz_y,  // = ap_top_nz;
                                ap_uint<9>& left_nz_y, // = ap_left_nz;
                                ap_uint<9>& top_nz_uv, // = ap_top_nz;
                                ap_uint<9>& left_nz_uv // = ap_left_nz;
                                ) {
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
    // for old  pred pass
    ap_uint<64> pred = str_pred->read();
    str_pred2->write(pred);
    // for old ret pass
    ap_uint<6> ret = str_ret->read();
    str_ret2->write(ret);
    // get mb_type
    ap_uint<1> mb_type = ret(4, 4);
    str_mb_type.write(mb_type);
    ap_uint<9> leftreturn = RecordCoeff_dataflow_dc(mb_type, str_level_dc, str_level_dc2, str_rec_dc, str_cnt_dc,
                                                    top_nz_dc, //
                                                    left_nz_dc);
    RecordCoeff_dataflow_y(mb_type, str_level_y, str_level_y2, str_rec_ac, str_cnt_ac,
                           top_nz_y, // = ap_top_nz;
                           left_nz_y);
    RecordCoeff_dataflow_uv(str_level_uv, str_level_uv2, str_rec_uv, str_cnt_uv,
                            top_nz_uv, // = ap_top_nz;
                            left_nz_uv // = ap_left_nz;
                            );
    return leftreturn;
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////
ap_uint<9> RecordCoeff_dataflow_dc(ap_uint<1> mb_type,
                                   hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc,
                                   // output
                                   hls::stream<ap_int<WD_LEVEL * 16> >* str_level_dc2,
                                   hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_dc,
                                   hls::stream<ap_uint<8> >& str_cnt_dc,
                                   ap_uint<9>& top_nz_dc, //
                                   ap_uint<9> left_nz_dc  // = ap_left_nz;
                                   ) {
#pragma HLS INLINE OFF
    ap_int<WD_LEVEL* 16> tmp16 = str_level_dc->read();
    str_level_dc2->write(tmp16);
    if (mb_type == 1) { // i16x16
        ap_uint<2> ctx = top_nz_dc[8] + left_nz_dc[8];
        ap_int<5> last = FindLast(tmp16);
        VP8RecordCoeffs_hls_str_w_cnt(ctx, tmp16, 0, last, str_rec_dc, str_cnt_dc);
        top_nz_dc[8] = left_nz_dc[8] = last < 0 ? 0 : 1;
        int b = left_nz_dc[8];
        // printf("%d",b);
    } // else printf(" ");//int a=top_nz_dc;int b=left_nz_dc;printf("%d",b);
    return left_nz_dc & 256 | top_nz_dc[8];
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////

void RecordCoeff_dataflow_y(ap_uint<1> mb_type,
                            hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y,
                            // output
                            hls::stream<ap_int<WD_LEVEL * 16> >* str_level_y2,
                            hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_ac,
                            hls::stream<ap_uint<8> >& str_cnt_ac,
                            ap_uint<9>& top_nz_y, // = ap_top_nz;
                            ap_uint<9>& left_nz_y // = ap_left_nz;
                            ) {
#pragma HLS INLINE OFF
    int x, y;
    // luma-AC
    for (y = 0; y < 4; ++y) {
#pragma HLS PIPELINE
    RECORD_COEFF_Y_IN:
        for (x = 0; x < 4; ++x) {
#pragma HLS PIPELINE
            ap_uint<2> ctx = top_nz_y[x] + left_nz_y[y];
            ap_int<WD_LEVEL* 16> tmp = str_level_y->read();
            str_level_y2->write(tmp); // for old
            ap_int<5> last = FindLast(tmp);
            VP8RecordCoeffs_hls_str_w_cnt(ctx, tmp, mb_type == 1, last, str_rec_ac, str_cnt_ac);
            top_nz_y[x] = left_nz_y[y] = last < 0 ? 0 : 1;
        }
    }
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////

void RecordCoeff_dataflow_uv(hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv,
                             // output
                             hls::stream<ap_int<WD_LEVEL * 16> >* str_level_uv2,
                             hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_uv,
                             hls::stream<ap_uint<8> >& str_cnt_uv,
                             ap_uint<9>& top_nz_uv, // = ap_top_nz;
                             ap_uint<9>& left_nz_uv // = ap_left_nz;
                             ) {
#pragma HLS INLINE OFF
    int x, y, ch;
// U/V
RECORD_COEFF_UV_0:
    for (ch = 0; ch <= 2; ch += 2) {
    RECORD_COEFF_UV_1:
        for (y = 0; y < 2; ++y) {
        RECORD_COEFF_UV_2:
            for (x = 0; x < 2; ++x) {
#pragma HLS PIPELINE
                ap_uint<2> ctx = top_nz_uv[4 + ch + x] + left_nz_uv[4 + ch + y];
                ap_int<WD_LEVEL* 16> tmp = str_level_uv->read();
                str_level_uv2->write(tmp); // for old
                ap_int<5> last = FindLast(tmp);
                VP8RecordCoeffs_hls_str_w_cnt(ctx, tmp, 0, last, str_rec_uv, str_cnt_uv);
                top_nz_uv[4 + ch + x] = left_nz_uv[4 + ch + y] = last < 0 ? 0 : 1;
            }
        }
    }
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////

static ap_int<5> FindLast(ap_int<WD_LEVEL * 16> level) {
#pragma HLS PIPELINE II = 1
    ap_int<5> ret = 15;
    for (ret = 15; ret > -1; ret--) {
        ap_int<WD_LEVEL> tmp = VCT_GET(level, ret, WD_LEVEL);
        if (tmp != 0) return ret;
    }
    return ret;
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////
int VP8RecordCoeffs_hls_str_w_cnt(ap_uint<2> ctx,
                                  ap_int<WD_LEVEL * 16> coeffs,
                                  ap_uint<1> first,
                                  ap_int<5> last,
                                  hls::stream<ap_uint<11> >& str_rec,
                                  hls::stream<ap_uint<8> >& str_cnt) {
    ap_uint<8> cnt = 0;
    int n = first;
    ap_uint<3> band_a = first;
    ap_uint<2> ctx_a = ctx;
    ap_uint<4> off_a = 0;
    if (last < 0) {
        Record_str(str_rec, 1, 0, band_a, ctx_a, 0);
        cnt++; // printf("cnt=%d \n",cnt.VAL );
        str_cnt.write(cnt);
        return 0;
    }
    ap_uint<1> isEealy_0 = 0;
RECORD_COEFF_STR:
    for (; n <= last; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 0 max = 16
#pragma HLS PIPELINE
        ap_int<WD_LEVEL> v;
        if (isEealy_0 == 0) {
            Record_str(str_rec, 0, 1, band_a, ctx_a, 0);
            cnt++; // printf("cnt=%d \n;",cnt.VAL );
        }
        v = (ap_int<WD_LEVEL>)VCT_GET(coeffs, n, WD_LEVEL);
        if (v == 0) {
            isEealy_0 = 1;
            Record_str(str_rec, 0, 0, band_a, ctx_a, 1);
            cnt++; // printf("cnt=%d\n ;",cnt.VAL );
            band_a = VP8EncBands_hls(n + 1);
            ctx_a = 0;
            continue;
        }
        isEealy_0 = 0;
        Record_str(str_rec, 0, 1, band_a, ctx_a, 1);
        cnt++; // printf("cnt=%d \n;",cnt.VAL );
        Record_str(str_rec, 0, 2u < (unsigned int)(v + 1), band_a, ctx_a, 2);
        cnt++;                               // printf("cnt=%d \n;",cnt.VAL );
        if (!(2u < (unsigned int)(v + 1))) { // v = -1 or 1
            band_a = VP8EncBands_hls(n + 1);
            ctx_a = 1;
        } else {
            if (v < 0) v = -v;
            if (v > 67) v = 67;

            ap_uint<9> bits = VP8LevelCodes_hls[v - 1][1];
            int pattern = VP8LevelCodes_hls[v - 1][0];
            int i;
        RECORD_COEFF_STR_INNER:
            for (i = 0; (pattern >>= 1) != 0; ++i) {
#pragma HLS LOOP_TRIPCOUNT min = 1 max = 8
#pragma HLS PIPELINE
                const int mask = 2 << i;
                if (pattern & 1) {
                    Record_str(str_rec, 0, !!(bits & mask), band_a, ctx_a, 3 + i);
                    cnt++; // printf("cnt=%d\n ;",cnt.VAL );
                }
            }
            band_a = VP8EncBands_hls(n + 1);
            ctx_a = 2;
        }
    } // while
    if (n < 16) {
        Record_str(str_rec, 1, 0, band_a, ctx_a, 0);
        cnt++; // printf("cnt=%d \n",cnt.VAL );
    }
    str_cnt.write(cnt);
    return 1;
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////
void Record_str(hls::stream<ap_uint<11> >& str_rec,
                ap_uint<1> isEnd,
                ap_uint<1> bit,
                ap_uint<3> band,
                ap_uint<2> ctx,
                ap_uint<4> off) {
    //#pragma HLS PIPELINE
    ap_uint<11> tmp;
    tmp(10, 10) = isEnd;
    tmp(9, 9) = bit;
    tmp(8, 6) = band;
    tmp(5, 4) = ctx;
    tmp(3, 0) = off;
    str_rec.write(tmp);
}
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////
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
//////////=====================   TopVp8_RecordCoeff_hls_cnt          ===========/////////////////////////////

//////////======================================================================/////////////////////////////
//////////============  TopVp8_RecordProb_hls_cnt                    ===========/////////////////////////////
//////////======================================================================/////////////////////////////
int TopVp8_RecordProb_hls_cnt(ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                              ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                              hls::stream<ap_uint<1> >& str_mb_type,
                              hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_dc,
                              hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_ac,
                              hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_uv,
                              hls::stream<ap_uint<8> >& str_cnt_dc,
                              hls::stream<ap_uint<8> >& str_cnt_ac,
                              hls::stream<ap_uint<8> >& str_cnt_uv,
                              uint8_t* pout_prob // 4, 8, 3,11
                              ) {
    //#pragma HLS INTERFACE m_axi port=pout_prob offset=slave bundle=gmem depth=4*8*3*11
    //#pragma HLS INTERFACE s_axilite port=pout_prob bundle=control
    //#pragma HLS INTERFACE s_axilite port=return bundle=control
    uint32_t stats[4][8][3][11];
#pragma HLS ARRAY_PARTITION variable = stats complete dim = 1
    uint8_t p_coeffs[4][8][3][11];
#pragma HLS ARRAY_PARTITION variable = p_coeffs complete dim = 1
    int t, b, c, p;
    for (t = 0; t < 4; ++t)
#pragma HLS UNROLL
    RECORD_PROB_INIT_B:
        for (b = 0; b < 8; ++b)
        C:
            for (c = 0; c < 3; ++c)
            P:
                for (p = 0; p < 11; ++p) {
#pragma HLS PIPELINE
                    stats[t][b][c][p] = 0;
                    p_coeffs[t][b][c][p] = hls_VP8CoeffsProba0[t][b][c][p];
                }

RECORD_PROB_READ_Y:
    for (int y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
    X:
        for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
            //#pragma HLS PIPELINE
            ap_uint<1> type_mb = str_mb_type.read();
            RecordPorb_ReadCoeff_dataflow2_cnt(type_mb, str_rec_dc, str_rec_ac, str_rec_uv, str_cnt_dc, str_cnt_ac,
                                               str_cnt_uv, stats[1], stats[0], stats[3], stats[2]);
        }
    }
    int dirty = 1;
    int size = sizeof(p_coeffs);
    FinalizeTokenProbas_hls(stats, p_coeffs, &dirty);
    memcpy(pout_prob, p_coeffs, size);
    pout_prob[SIZE8_MEM_PROB - 1] = dirty;
    return dirty;
}

void TopVp8_RecordProb_hls_cnt_HideDirty(ap_uint<LG2_MAX_NUM_MB_W> mb_w,
                                         ap_uint<LG2_MAX_NUM_MB_H> mb_h,
                                         hls::stream<ap_uint<1> >& str_mb_type,
                                         hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_dc,
                                         hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_ac,
                                         hls::stream<ap_uint<1 + 1 + 3 + 2 + 4> >& str_rec_uv,
                                         hls::stream<ap_uint<8> >& str_cnt_dc,
                                         hls::stream<ap_uint<8> >& str_cnt_ac,
                                         hls::stream<ap_uint<8> >& str_cnt_uv,
                                         uint8_t* pout_prob // 4, 8, 3,11
                                         ) {
    //#pragma HLS INTERFACE m_axi port=pout_prob offset=slave bundle=gmem depth=4*8*3*11
    //#pragma HLS INTERFACE s_axilite port=pout_prob bundle=control
    //#pragma HLS INTERFACE s_axilite port=return bundle=control
    uint32_t stats[4][8][3][11];
#pragma HLS ARRAY_PARTITION variable = stats complete dim = 1
    uint8_t p_coeffs[4][8][3][11];
#pragma HLS ARRAY_PARTITION variable = p_coeffs complete dim = 1
    int t, b, c, p;
    for (t = 0; t < 4; ++t)
#pragma HLS UNROLL
        for (b = 0; b < 8; ++b)
            for (c = 0; c < 3; ++c)
            HIDEDIRTY_INIT:
                for (p = 0; p < 11; ++p) {
#pragma HLS PIPELINE
                    stats[t][b][c][p] = 0;
                    p_coeffs[t][b][c][p] = hls_VP8CoeffsProba0[t][b][c][p];
                }

    for (int y_mb = 0; y_mb < mb_h; y_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 68 max = 256
        for (int x_mb = 0; x_mb < mb_w; x_mb++) {
#pragma HLS LOOP_TRIPCOUNT min = 120 max = 256
            //#pragma HLS PIPELINE
            ap_uint<1> type_mb = str_mb_type.read();
            RecordPorb_ReadCoeff_dataflow2_cnt(type_mb, str_rec_dc, str_rec_ac, str_rec_uv, str_cnt_dc, str_cnt_ac,
                                               str_cnt_uv, stats[1], stats[0], stats[3], stats[2]);
        }
    }
    int dirty = 1;
    int size = sizeof(p_coeffs);
    FinalizeTokenProbas_hls(stats, p_coeffs, &dirty);
    memcpy(pout_prob, p_coeffs, size);
    pout_prob[SIZE8_MEM_PROB - 1] = dirty;
}

//////////============  TopVp8_RecordProb_hls_cnt_HideDirty          ===========/////////////////////////////
void RecordPorb_ReadCoeff_dataflow2_cnt(ap_uint<1> mb_type,
                                        hls::stream<ap_uint<11> >& str_rec_dc,
                                        hls::stream<ap_uint<11> >& str_rec_ac,
                                        hls::stream<ap_uint<11> >& str_rec_uv,
                                        hls::stream<ap_uint<8> >& str_cnt_dc,
                                        hls::stream<ap_uint<8> >& str_cnt_ac,
                                        hls::stream<ap_uint<8> >& str_cnt_uv,
                                        uint32_t stats_dc[8][3][11],
                                        uint32_t stats_ac0_dc[8][3][11],
                                        uint32_t stats_ac3[8][3][11],
                                        uint32_t stats_uv[8][3][11]) {
#pragma HLS DATAFLOW
    RecordPorb_ReadCoeff_dataflow_dc_cnt(mb_type, str_rec_dc, str_cnt_dc, stats_dc);
    RecordPorb_ReadCoeff_dataflow_ac_cnt(mb_type, str_rec_ac, str_cnt_ac, stats_ac0_dc, stats_ac3);
    RecordPorb_ReadCoeff_dataflow_uv_cnt(str_rec_uv, str_cnt_uv, stats_uv);
}

//////////============  TopVp8_RecordProb_hls_cnt_HideDirty          ===========/////////////////////////////
////RecordPorb_ReadCoeff_dataflow_dc_cnt//////////////////////////
void RecordPorb_ReadCoeff_dataflow_dc_cnt(ap_uint<1> mb_type,
                                          hls::stream<ap_uint<11> >& str_rec_dc,
                                          hls::stream<ap_uint<8> >& str_cnt,
                                          uint32_t stats_dc[8][3][11]) {
    if (mb_type == 1) VP8RecordCoeffs_hls_str_r_cnt(str_rec_dc, str_cnt, stats_dc);
}

////RecordPorb_ReadCoeff_dataflow_ac_cnt//////////////////////////
void RecordPorb_ReadCoeff_dataflow_ac_cnt(ap_uint<1> mb_type,
                                          hls::stream<ap_uint<11> >& str_rec_ac,
                                          hls::stream<ap_uint<8> >& str_cnt,
                                          uint32_t stats_ac0_dc[8][3][11],
                                          uint32_t stats_ac3[8][3][11]) {
    //#pragma HLS PIPELINE
    for (int i = 0; i < 16; i++)
        if (mb_type)
            VP8RecordCoeffs_hls_str_r_cnt(str_rec_ac, str_cnt, stats_ac0_dc);
        else
            VP8RecordCoeffs_hls_str_r_cnt(str_rec_ac, str_cnt, stats_ac3);
}
////RecordPorb_ReadCoeff_dataflow_uv_cnt//////////////////////////
void RecordPorb_ReadCoeff_dataflow_uv_cnt(hls::stream<ap_uint<11> >& str_rec_uv,
                                          hls::stream<ap_uint<8> >& str_cnt,
                                          uint32_t stats_uv[8][3][11]) {
    //#pragma HLS PIPELINE
    for (int i = 0; i < 8; i++) VP8RecordCoeffs_hls_str_r_cnt(str_rec_uv, str_cnt, stats_uv);
}

//////////============  TopVp8_RecordProb_hls_cnt_HideDirty          ===========/////////////////////////////

//////VP8RecordCoeffs_hls_str_r_cnt//////////////////
void VP8RecordCoeffs_hls_str_r_cnt(hls::stream<ap_uint<11> >& str_rec,
                                   hls::stream<ap_uint<8> >& str_cnt,
                                   uint32_t stats[8][3][11]) {
#pragma HLS INLINE OFF
    uint32_t state, state0;
    ap_uint<1> bit, bit0;
    ap_uint<9> addr, addr0;
    addr0 = 0x1ff; // seens never be access
    addr = 0;

    ap_uint<8> cnt = str_cnt.read();
RECORD_COEFFS_INNER:
    for (int i = 0; i < cnt; i++) {
#pragma HLS dependence array inter false
#pragma HLS LOOP_TRIPCOUNT min = 18 max = 127
#pragma HLS PIPELINE II = 1
        ap_uint<11> tmp = str_rec.read();
        addr = tmp(8, 0);
        ap_uint<1> bit = tmp(9, 9);

        ap_uint<3> band = addr(8, 6);
        ap_uint<2> ctx = addr(5, 4);
        ap_uint<4> off = addr(3, 0);
        if (addr != addr0) {
            state = stats[band][ctx][off];
        } else {
            state = state0;
        }
        ap_uint<3> band0 = addr0(8, 6);
        ap_uint<2> ctx0 = addr0(5, 4);
        ap_uint<4> off0 = addr0(3, 0);
        if (i != 0) stats[band0][ctx0][off0] = state0;

        state0 = Record_hls(bit, state);
        addr0 = addr;
    }
    ap_uint<3> band0 = addr0(8, 6);
    ap_uint<2> ctx0 = addr0(5, 4);
    ap_uint<4> off0 = addr0(3, 0);
    stats[band0][ctx0][off0] = state0;
}
void VP8RecordCoeffs_hls_str_r_cnt_old(hls::stream<ap_uint<11> >& str_rec,
                                       hls::stream<ap_uint<8> >& str_cnt,
                                       uint32_t stats[8][3][11]) {
    ap_uint<8> cnt = str_cnt.read();
RECORD_COEFFS_OLD:
    for (int i = 0; i < cnt; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 18 max = 127
#pragma HLS PIPELINE II = 2
        ap_uint<11> tmp = str_rec.read();
        ap_uint<1> isEnd = tmp(10, 10); // = isEnd;
        ap_uint<1> bit = tmp(9, 9);     // = bit;
        ap_uint<3> band = tmp(8, 6);    // = band;
        ap_uint<2> ctx = tmp(5, 4);     // = ctx;
        ap_uint<4> off = tmp(3, 0);     // = off;
        uint32_t state_old = stats[band][ctx][off];
        stats[band][ctx][off] = Record_hls(bit, state_old);
    }
}
//////////============  TopVp8_RecordProb_hls_cnt_HideDirty          ===========/////////////////////////////
ap_uint<32> Record_hls(ap_uint<1> bit, ap_uint<32> p) {
#pragma HLS PIPELINE
    // ap_uint<32> p = *stats;
    ap_uint<16> p_h = p(31, 16);
    ap_uint<16> p_l = p(15, 0);
    if (p_h == 0xffff) { // an overflow is inbound.
        p_h = 0x7fff;
        p_l = (p_l + 1 + (bit << 1)) >> 1;
    } else {
        p_h += 1;
        p_l += bit;
    }
    p(31, 16) = p_h;
    p(15, 0) = p_l;
    //*stats = p;
    return p;
}
//////////============  TopVp8_RecordProb_hls_cnt_HideDirty          ===========/////////////////////////////
static uint8_t hls_CalcTokenProba(int nb, int total) { // in fact return value range from 0~255, only needs  8 bits
    return nb ? (255 - nb * 255 / total) : 255;
}

static int hls_VP8BitCost(int bit, uint8_t proba) {
    return !bit ? hls_VP8EntropyCost[proba] : hls_VP8EntropyCost[255 - proba];
}

static int hls_BranchCost(int nb, int total, int proba) {
    return nb * hls_VP8BitCost(1, proba) + (total - nb) * hls_VP8BitCost(0, proba);
}
int FinalizeTokenProbas_hls(uint32_t p_stats[4][8][3][11], uint8_t p_coeffs_[4][8][3][11], int* dirty) {
    int has_changed = 0;
    int size = 0;
    int t, b, c, p;
INA_TOKEN_PROB:
    for (t = 0; t < 4; ++t) {
        //#pragma HLS UNROLL
        for (b = 0; b < 8; ++b) {
        //#pragma HLS PIPELINE
        C:
            for (c = 0; c < 3; ++c) {
            //#pragma HLS PIPELINE
            P:
                for (p = 0; p < 11; ++p) {
#pragma HLS PIPELINE
                    uint32_t stats = p_stats[t][b][c][p];
                    // wr//if(stats!=0)// printf("%s [%d][%d][%d][%d]stats:%d\n",
                    // wr//  printf("t=%d, b=%d, c=%d, p= %d, stats=%x\n",t,b,c,p,stats);//     __FUNCTION__, t, b, c,
                    // p, stats);
                    const int nb = (stats >> 0) & 0xffff;
                    const int total = (stats >> 16) & 0xffff;
                    const int update_proba = hls_VP8CoeffsUpdateProba[t][b][c][p];
                    const int old_p = hls_VP8CoeffsProba0[t][b][c][p];
                    const int new_p = hls_CalcTokenProba(nb, total);
                    const int old_cost = hls_BranchCost(nb, total, old_p) + hls_VP8BitCost(0, update_proba);
                    const int new_cost = hls_BranchCost(nb, total, new_p) + hls_VP8BitCost(1, update_proba) + 8 * 256;
                    const int use_new_p = (old_cost > new_cost);
                    // printf("%s use_new_p:%d old_cost:%d new_cost:%d\n",
                    //     __FUNCTION__, use_new_p, old_cost, new_cost);
                    size += hls_VP8BitCost(use_new_p, update_proba);
                    if (use_new_p) { // only use proba that seem meaningful enough.
                        p_coeffs_[t][b][c][p] = new_p;
                        has_changed |= (new_p != old_p);
                        // printf("%s has_changed:%d new_p:%d old_p:%d\n",
                        //   __FUNCTION__, has_changed, new_p, old_p);
                        size += 8 * 256;
                    } else {
                        p_coeffs_[t][b][c][p] = old_p;
                    }
                }
            }
        }
    }
    // printf("%d %d==========================\n", __LINE__, has_changed);
    *dirty = has_changed;
    return size;
}
//////////============  TopVp8_RecordProb_hls_cnt_HideDirty          ===========/////////////////////////////

extern "C" {
void kernel_IntraPredLoop2_NoOut_1_6axi(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem1 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem0 depth = 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem2 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem3 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem4 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem5 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32

#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int p_readinfo[64];
    memcpy(p_readinfo, p_info, 64 * sizeof(int));
    ap_uint<32> id_pic;
    ap_uint<32> mb_line;
    ap_uint<LG2_MAX_W_PIX> y_stride;
    ap_uint<LG2_MAX_W_PIX> uv_stride;
    ap_uint<LG2_MAX_W_PIX> width;
    ap_uint<LG2_MAX_W_PIX> height;
    ap_uint<LG2_MAX_NUM_MB_W> mb_w;
    ap_uint<LG2_MAX_NUM_MB_H> mb_h;
    ap_uint<WD_LMD> lambda_p16;
    ap_uint<WD_LMD> lambda_p44;
    ap_uint<WD_LMD> tlambda;
    ap_uint<WD_LMD> lambda_uv;
    ap_uint<WD_LMD> tlambda_m;
    hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
    ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;

    // Initializing image variables, once for one picture
    { // For convenience, extend the code at top module to show all parameters used by kernel of intra-prediction
        id_pic = p_readinfo[0];  // reserved for future
        mb_line = p_readinfo[1]; // reserved for future, to show current line number of mb
        y_stride = p_readinfo[2];
        uv_stride = p_readinfo[3];
        width = p_readinfo[4];
        height = p_readinfo[5];
        mb_w = p_readinfo[2 + 2 + 2];
        mb_h = p_readinfo[3 + 2 + 2];
        lambda_p16 = p_readinfo[4 + 2 + 2];
        lambda_p44 = p_readinfo[5 + 2 + 2];
        tlambda = p_readinfo[6 + 2 + 2];
        lambda_uv = p_readinfo[7 + 2 + 2];
        tlambda_m = p_readinfo[8 + 2 + 2];

        hls_qm1.q_0 = p_readinfo[11 + 2]; // quantizer steps
        hls_qm1.q_n = p_readinfo[12 + 2];
        hls_qm1.iq_0 = p_readinfo[13 + 2]; // reciprocals fixed point.
        hls_qm1.iq_n = p_readinfo[14 + 2];
        hls_qm1.bias_0 = p_readinfo[15 + 2]; // rounding bias
        hls_qm1.bias_n = p_readinfo[16 + 2];

        hls_qm2.q_0 = p_readinfo[17 + 2]; // quantizer steps
        hls_qm2.q_n = p_readinfo[18 + 2];
        hls_qm2.iq_0 = p_readinfo[19 + 2]; // reciprocals fixed point.
        hls_qm2.iq_n = p_readinfo[20 + 2];
        hls_qm2.bias_0 = p_readinfo[21 + 2]; // rounding bias
        hls_qm2.bias_n = p_readinfo[22 + 2];

        hls_qm_uv.q_0 = p_readinfo[23 + 2]; // quantizer steps
        hls_qm_uv.q_n = p_readinfo[24 + 2];
        hls_qm_uv.iq_0 = p_readinfo[25 + 2]; // reciprocals fixed point.
        hls_qm_uv.iq_n = p_readinfo[26 + 2];
        hls_qm_uv.bias_0 = p_readinfo[27 + 2]; // rounding bias
        hls_qm_uv.bias_n = p_readinfo[28 + 2];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_readinfo[29 + 2 + 16 + i];
    } // end of initialization
    int dirty = 0;
    TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(id_pic,     // p_info[0],
                                                      mb_line,    // p_info[1],
                                                      y_stride,   // p_info[2],  // ,//pic->y_stride,
                                                      uv_stride,  // p_info[3], // ,//pic->uv_stride
                                                      width,      // p_info[4],  // ,//pic->width
                                                      height,     // p_info[5],  // ,//pic->height
                                                      mb_w,       // p_info[2+2+2],///,
                                                      mb_h,       // p_info[3+2+2],//,
                                                      lambda_p16, // p_info[4+2+2],//dqm->lambda_i16_,
                                                      lambda_p44, // p_info[5+2+2],//dqm->lambda_i4_,
                                                      tlambda,    // p_info[6+2+2],//dqm->tlambda_,
                                                      lambda_uv,  // p_info[7+2+2],//dqm->lambda_uv_,
                                                      tlambda_m,  // p_info[8+2+2],//dqm->lambda_mode_,
                                                      hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv,
                                                      ysrc,       // 4096x4096
                                                      usrc,       // 2048x2048
                                                      vsrc,       // 2048x2048
                                                      pout_level, // 65536*512
                                                      pout_prob, &dirty);
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_2_6axi(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem1 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem0 depth = 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem2 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem3 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem4 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem5 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32

#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int p_readinfo[64];
    memcpy(p_readinfo, p_info, 64 * sizeof(int));
    ap_uint<32> id_pic;
    ap_uint<32> mb_line;
    ap_uint<LG2_MAX_W_PIX> y_stride;
    ap_uint<LG2_MAX_W_PIX> uv_stride;
    ap_uint<LG2_MAX_W_PIX> width;
    ap_uint<LG2_MAX_W_PIX> height;
    ap_uint<LG2_MAX_NUM_MB_W> mb_w;
    ap_uint<LG2_MAX_NUM_MB_H> mb_h;
    ap_uint<WD_LMD> lambda_p16;
    ap_uint<WD_LMD> lambda_p44;
    ap_uint<WD_LMD> tlambda;
    ap_uint<WD_LMD> lambda_uv;
    ap_uint<WD_LMD> tlambda_m;
    hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
    ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;

    // Initializing image variables, once for one picture
    { // For convenience, extend the code at top module to show all parameters used by kernel of intra-prediction
        id_pic = p_readinfo[0];  // reserved for future
        mb_line = p_readinfo[1]; // reserved for future, to show current line number of mb
        y_stride = p_readinfo[2];
        uv_stride = p_readinfo[3];
        width = p_readinfo[4];
        height = p_readinfo[5];
        mb_w = p_readinfo[2 + 2 + 2];
        mb_h = p_readinfo[3 + 2 + 2];
        lambda_p16 = p_readinfo[4 + 2 + 2];
        lambda_p44 = p_readinfo[5 + 2 + 2];
        tlambda = p_readinfo[6 + 2 + 2];
        lambda_uv = p_readinfo[7 + 2 + 2];
        tlambda_m = p_readinfo[8 + 2 + 2];

        hls_qm1.q_0 = p_readinfo[11 + 2]; // quantizer steps
        hls_qm1.q_n = p_readinfo[12 + 2];
        hls_qm1.iq_0 = p_readinfo[13 + 2]; // reciprocals fixed point.
        hls_qm1.iq_n = p_readinfo[14 + 2];
        hls_qm1.bias_0 = p_readinfo[15 + 2]; // rounding bias
        hls_qm1.bias_n = p_readinfo[16 + 2];

        hls_qm2.q_0 = p_readinfo[17 + 2]; // quantizer steps
        hls_qm2.q_n = p_readinfo[18 + 2];
        hls_qm2.iq_0 = p_readinfo[19 + 2]; // reciprocals fixed point.
        hls_qm2.iq_n = p_readinfo[20 + 2];
        hls_qm2.bias_0 = p_readinfo[21 + 2]; // rounding bias
        hls_qm2.bias_n = p_readinfo[22 + 2];

        hls_qm_uv.q_0 = p_readinfo[23 + 2]; // quantizer steps
        hls_qm_uv.q_n = p_readinfo[24 + 2];
        hls_qm_uv.iq_0 = p_readinfo[25 + 2]; // reciprocals fixed point.
        hls_qm_uv.iq_n = p_readinfo[26 + 2];
        hls_qm_uv.bias_0 = p_readinfo[27 + 2]; // rounding bias
        hls_qm_uv.bias_n = p_readinfo[28 + 2];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_readinfo[29 + 2 + 16 + i];
    } // end of initialization
    int dirty = 0;
    TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(id_pic,     // p_info[0],
                                                      mb_line,    // p_info[1],
                                                      y_stride,   // p_info[2],  // ,//pic->y_stride,
                                                      uv_stride,  // p_info[3], // ,//pic->uv_stride
                                                      width,      // p_info[4],  // ,//pic->width
                                                      height,     // p_info[5],  // ,//pic->height
                                                      mb_w,       // p_info[2+2+2],///,
                                                      mb_h,       // p_info[3+2+2],//,
                                                      lambda_p16, // p_info[4+2+2],//dqm->lambda_i16_,
                                                      lambda_p44, // p_info[5+2+2],//dqm->lambda_i4_,
                                                      tlambda,    // p_info[6+2+2],//dqm->tlambda_,
                                                      lambda_uv,  // p_info[7+2+2],//dqm->lambda_uv_,
                                                      tlambda_m,  // p_info[8+2+2],//dqm->lambda_mode_,
                                                      hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv,
                                                      ysrc,       // 4096x4096
                                                      usrc,       // 2048x2048
                                                      vsrc,       // 2048x2048
                                                      pout_level, // 65536*512
                                                      pout_prob, &dirty);
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_3_6axi(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem1 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem0 depth = 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem2 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem3 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem4 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem5 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32

#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int p_readinfo[64];
    memcpy(p_readinfo, p_info, 64 * sizeof(int));
    ap_uint<32> id_pic;
    ap_uint<32> mb_line;
    ap_uint<LG2_MAX_W_PIX> y_stride;
    ap_uint<LG2_MAX_W_PIX> uv_stride;
    ap_uint<LG2_MAX_W_PIX> width;
    ap_uint<LG2_MAX_W_PIX> height;
    ap_uint<LG2_MAX_NUM_MB_W> mb_w;
    ap_uint<LG2_MAX_NUM_MB_H> mb_h;
    ap_uint<WD_LMD> lambda_p16;
    ap_uint<WD_LMD> lambda_p44;
    ap_uint<WD_LMD> tlambda;
    ap_uint<WD_LMD> lambda_uv;
    ap_uint<WD_LMD> tlambda_m;
    hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
    ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;

    // Initializing image variables, once for one picture
    { // For convenience, extend the code at top module to show all parameters used by kernel of intra-prediction
        id_pic = p_readinfo[0];  // reserved for future
        mb_line = p_readinfo[1]; // reserved for future, to show current line number of mb
        y_stride = p_readinfo[2];
        uv_stride = p_readinfo[3];
        width = p_readinfo[4];
        height = p_readinfo[5];
        mb_w = p_readinfo[2 + 2 + 2];
        mb_h = p_readinfo[3 + 2 + 2];
        lambda_p16 = p_readinfo[4 + 2 + 2];
        lambda_p44 = p_readinfo[5 + 2 + 2];
        tlambda = p_readinfo[6 + 2 + 2];
        lambda_uv = p_readinfo[7 + 2 + 2];
        tlambda_m = p_readinfo[8 + 2 + 2];

        hls_qm1.q_0 = p_readinfo[11 + 2]; // quantizer steps
        hls_qm1.q_n = p_readinfo[12 + 2];
        hls_qm1.iq_0 = p_readinfo[13 + 2]; // reciprocals fixed point.
        hls_qm1.iq_n = p_readinfo[14 + 2];
        hls_qm1.bias_0 = p_readinfo[15 + 2]; // rounding bias
        hls_qm1.bias_n = p_readinfo[16 + 2];

        hls_qm2.q_0 = p_readinfo[17 + 2]; // quantizer steps
        hls_qm2.q_n = p_readinfo[18 + 2];
        hls_qm2.iq_0 = p_readinfo[19 + 2]; // reciprocals fixed point.
        hls_qm2.iq_n = p_readinfo[20 + 2];
        hls_qm2.bias_0 = p_readinfo[21 + 2]; // rounding bias
        hls_qm2.bias_n = p_readinfo[22 + 2];

        hls_qm_uv.q_0 = p_readinfo[23 + 2]; // quantizer steps
        hls_qm_uv.q_n = p_readinfo[24 + 2];
        hls_qm_uv.iq_0 = p_readinfo[25 + 2]; // reciprocals fixed point.
        hls_qm_uv.iq_n = p_readinfo[26 + 2];
        hls_qm_uv.bias_0 = p_readinfo[27 + 2]; // rounding bias
        hls_qm_uv.bias_n = p_readinfo[28 + 2];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_readinfo[29 + 2 + 16 + i];
    } // end of initialization
    int dirty = 0;
    TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(id_pic,     // p_info[0],
                                                      mb_line,    // p_info[1],
                                                      y_stride,   // p_info[2],  // ,//pic->y_stride,
                                                      uv_stride,  // p_info[3], // ,//pic->uv_stride
                                                      width,      // p_info[4],  // ,//pic->width
                                                      height,     // p_info[5],  // ,//pic->height
                                                      mb_w,       // p_info[2+2+2],///,
                                                      mb_h,       // p_info[3+2+2],//,
                                                      lambda_p16, // p_info[4+2+2],//dqm->lambda_i16_,
                                                      lambda_p44, // p_info[5+2+2],//dqm->lambda_i4_,
                                                      tlambda,    // p_info[6+2+2],//dqm->tlambda_,
                                                      lambda_uv,  // p_info[7+2+2],//dqm->lambda_uv_,
                                                      tlambda_m,  // p_info[8+2+2],//dqm->lambda_mode_,
                                                      hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv,
                                                      ysrc,       // 4096x4096
                                                      usrc,       // 2048x2048
                                                      vsrc,       // 2048x2048
                                                      pout_level, // 65536*512
                                                      pout_prob, &dirty);
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_4_6axi(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem1 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem0 depth = 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem2 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem3 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem4 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem5 depth = 2048 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32

#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    int p_readinfo[64];
    memcpy(p_readinfo, p_info, 64 * sizeof(int));
    ap_uint<32> id_pic;
    ap_uint<32> mb_line;
    ap_uint<LG2_MAX_W_PIX> y_stride;
    ap_uint<LG2_MAX_W_PIX> uv_stride;
    ap_uint<LG2_MAX_W_PIX> width;
    ap_uint<LG2_MAX_W_PIX> height;
    ap_uint<LG2_MAX_NUM_MB_W> mb_w;
    ap_uint<LG2_MAX_NUM_MB_H> mb_h;
    ap_uint<WD_LMD> lambda_p16;
    ap_uint<WD_LMD> lambda_p44;
    ap_uint<WD_LMD> tlambda;
    ap_uint<WD_LMD> lambda_uv;
    ap_uint<WD_LMD> tlambda_m;
    hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
    ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;

    // Initializing image variables, once for one picture
    { // For convenience, extend the code at top module to show all parameters used by kernel of intra-prediction
        id_pic = p_readinfo[0];  // reserved for future
        mb_line = p_readinfo[1]; // reserved for future, to show current line number of mb
        y_stride = p_readinfo[2];
        uv_stride = p_readinfo[3];
        width = p_readinfo[4];
        height = p_readinfo[5];
        mb_w = p_readinfo[2 + 2 + 2];
        mb_h = p_readinfo[3 + 2 + 2];
        lambda_p16 = p_readinfo[4 + 2 + 2];
        lambda_p44 = p_readinfo[5 + 2 + 2];
        tlambda = p_readinfo[6 + 2 + 2];
        lambda_uv = p_readinfo[7 + 2 + 2];
        tlambda_m = p_readinfo[8 + 2 + 2];

        hls_qm1.q_0 = p_readinfo[11 + 2]; // quantizer steps
        hls_qm1.q_n = p_readinfo[12 + 2];
        hls_qm1.iq_0 = p_readinfo[13 + 2]; // reciprocals fixed point.
        hls_qm1.iq_n = p_readinfo[14 + 2];
        hls_qm1.bias_0 = p_readinfo[15 + 2]; // rounding bias
        hls_qm1.bias_n = p_readinfo[16 + 2];

        hls_qm2.q_0 = p_readinfo[17 + 2]; // quantizer steps
        hls_qm2.q_n = p_readinfo[18 + 2];
        hls_qm2.iq_0 = p_readinfo[19 + 2]; // reciprocals fixed point.
        hls_qm2.iq_n = p_readinfo[20 + 2];
        hls_qm2.bias_0 = p_readinfo[21 + 2]; // rounding bias
        hls_qm2.bias_n = p_readinfo[22 + 2];

        hls_qm_uv.q_0 = p_readinfo[23 + 2]; // quantizer steps
        hls_qm_uv.q_n = p_readinfo[24 + 2];
        hls_qm_uv.iq_0 = p_readinfo[25 + 2]; // reciprocals fixed point.
        hls_qm_uv.iq_n = p_readinfo[26 + 2];
        hls_qm_uv.bias_0 = p_readinfo[27 + 2]; // rounding bias
        hls_qm_uv.bias_n = p_readinfo[28 + 2];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
        for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
            VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_readinfo[29 + 2 + 16 + i];
    } // end of initialization
    int dirty = 0;
    TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(id_pic,     // p_info[0],
                                                      mb_line,    // p_info[1],
                                                      y_stride,   // p_info[2],  // ,//pic->y_stride,
                                                      uv_stride,  // p_info[3], // ,//pic->uv_stride
                                                      width,      // p_info[4],  // ,//pic->width
                                                      height,     // p_info[5],  // ,//pic->height
                                                      mb_w,       // p_info[2+2+2],///,
                                                      mb_h,       // p_info[3+2+2],//,
                                                      lambda_p16, // p_info[4+2+2],//dqm->lambda_i16_,
                                                      lambda_p44, // p_info[5+2+2],//dqm->lambda_i4_,
                                                      tlambda,    // p_info[6+2+2],//dqm->tlambda_,
                                                      lambda_uv,  // p_info[7+2+2],//dqm->lambda_uv_,
                                                      tlambda_m,  // p_info[8+2+2],//dqm->lambda_mode_,
                                                      hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv,
                                                      ysrc,       // 4096x4096
                                                      usrc,       // 2048x2048
                                                      vsrc,       // 2048x2048
                                                      pout_level, // 65536*512
                                                      pout_prob, &dirty);
}
}
void kernel_IntraPredLoop2_NoOut_core_wrapper( // 1)	Set protocol of Id_pic : 63take fully use of 4k x 4k buff)
    int32_t* p_info_mult,                      // 2)	Enlarge size of p_info 64 times
    uint32_t* ysrc_mult,                       // 3)	The largest size of pout_level, ysrc, usrc and vsrc do not change.
    uint32_t* usrc_mult,                       // 3)	Offset stride is the MB number x 256/4 for y, x 64/4 for u and v;
    uint32_t* vsrc_mult,
    int32_t* pout_level_mult, // 4)	Change protocol of pout_level:
    uint8_t* pout_prob_mult)  // 5)	Enlarge size of pout_prob 64 times;
{
    int pid_mult = p_info_mult[0];
    ;
    int32_t* p_info = p_info_mult;
    uint32_t* ysrc = ysrc_mult; //,             //3)	The largest size of pout_level, ysrc, usrc and vsrc do not
                                // change.
    uint32_t* usrc = usrc_mult; //,				//3)	Offset stride is the MB number x 256/4 for y, x
                                // 64/4 for u and v;
    uint32_t* vsrc = vsrc_mult; //,
    int32_t* pout_level = pout_level_mult; //,        //4)	Change protocol of pout_level:
    uint8_t* pout_prob = pout_prob_mult;   //
TOP_LOOP:
    for (int toploop = 0; toploop <= pid_mult; toploop++) {
        // do{
        int p_readinfo[64];
        memcpy(p_readinfo, p_info, 64 * sizeof(int));
        ap_uint<32> id_pic;
        ap_uint<32> mb_line;
        ap_uint<LG2_MAX_W_PIX> y_stride;
        ap_uint<LG2_MAX_W_PIX> uv_stride;
        ap_uint<LG2_MAX_W_PIX> width;
        ap_uint<LG2_MAX_W_PIX> height;
        ap_uint<LG2_MAX_NUM_MB_W> mb_w;
        ap_uint<LG2_MAX_NUM_MB_H> mb_h;
        ap_uint<WD_LMD> lambda_p16;
        ap_uint<WD_LMD> lambda_p44;
        ap_uint<WD_LMD> tlambda;
        ap_uint<WD_LMD> lambda_uv;
        ap_uint<WD_LMD> tlambda_m;
        hls_QMatrix hls_qm1, hls_qm2, hls_qm_uv;
        ap_int<WD_sharpen * 16> ap_sharpen, ap_sharpen_uv;

        // Initializing image variables, once for one picture
        { // For convenience, extend the code at top module to show all parameters used by kernel of intra-prediction
            id_pic = p_readinfo[0];  // reserved for future
            mb_line = p_readinfo[1]; // reserved for future, to show current line number of mb
            y_stride = p_readinfo[2];
            uv_stride = p_readinfo[3];
            width = p_readinfo[4];
            height = p_readinfo[5];
            mb_w = p_readinfo[2 + 2 + 2];
            mb_h = p_readinfo[3 + 2 + 2];
            lambda_p16 = p_readinfo[4 + 2 + 2];
            lambda_p44 = p_readinfo[5 + 2 + 2];
            tlambda = p_readinfo[6 + 2 + 2];
            lambda_uv = p_readinfo[7 + 2 + 2];
            tlambda_m = p_readinfo[8 + 2 + 2];

            hls_qm1.q_0 = p_readinfo[11 + 2]; // quantizer steps
            hls_qm1.q_n = p_readinfo[12 + 2];
            hls_qm1.iq_0 = p_readinfo[13 + 2]; // reciprocals fixed point.
            hls_qm1.iq_n = p_readinfo[14 + 2];
            hls_qm1.bias_0 = p_readinfo[15 + 2]; // rounding bias
            hls_qm1.bias_n = p_readinfo[16 + 2];

            hls_qm2.q_0 = p_readinfo[17 + 2]; // quantizer steps
            hls_qm2.q_n = p_readinfo[18 + 2];
            hls_qm2.iq_0 = p_readinfo[19 + 2]; // reciprocals fixed point.
            hls_qm2.iq_n = p_readinfo[20 + 2];
            hls_qm2.bias_0 = p_readinfo[21 + 2]; // rounding bias
            hls_qm2.bias_n = p_readinfo[22 + 2];

            hls_qm_uv.q_0 = p_readinfo[23 + 2]; // quantizer steps
            hls_qm_uv.q_n = p_readinfo[24 + 2];
            hls_qm_uv.iq_0 = p_readinfo[25 + 2]; // reciprocals fixed point.
            hls_qm_uv.iq_n = p_readinfo[26 + 2];
            hls_qm_uv.bias_0 = p_readinfo[27 + 2]; // rounding bias
            hls_qm_uv.bias_n = p_readinfo[28 + 2];
            for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
                VCT_GET(ap_sharpen, i, WD_sharpen) = p_info[29 + 2 + i];
            for (int i = 0; i < 16; i++)
#pragma HLS UNROLL
                VCT_GET(ap_sharpen_uv, i, WD_sharpen) = p_readinfo[29 + 2 + 16 + i];
        } // end of initialization

        int dirty = 0;
        TopVp8_top_dataflow_32bit_k1NoStruct_cnt_DeepFIFO(id_pic,     // p_info[0],
                                                          mb_line,    // p_info[1],
                                                          y_stride,   // p_info[2],  // ,//pic->y_stride,
                                                          uv_stride,  // p_info[3], // ,//pic->uv_stride
                                                          width,      // p_info[4],  // ,//pic->width
                                                          height,     // p_info[5],  // ,//pic->height
                                                          mb_w,       // p_info[2+2+2],///,
                                                          mb_h,       // p_info[3+2+2],//,
                                                          lambda_p16, // p_info[4+2+2],//dqm->lambda_i16_,
                                                          lambda_p44, // p_info[5+2+2],//dqm->lambda_i4_,
                                                          tlambda,    // p_info[6+2+2],//dqm->tlambda_,
                                                          lambda_uv,  // p_info[7+2+2],//dqm->lambda_uv_,
                                                          tlambda_m,  // p_info[8+2+2],//dqm->lambda_mode_,
                                                          hls_qm1, hls_qm2, hls_qm_uv, ap_sharpen, ap_sharpen_uv,
                                                          ysrc,       // 4096x4096
                                                          usrc,       // 2048x2048
                                                          vsrc,       // 2048x2048
                                                          pout_level, // 65536*512
                                                          pout_prob, &dirty);
        int num_mb = mb_w * mb_h;
        int* ptmp_prob = (int*)pout_prob;
        ptmp_prob[OFF_NUM_MB_32] = num_mb; // will be sued by kernel 2
        ptmp_prob[OFF_PID_PROB_8BIT / 4] = pid_mult - toploop;
        int offset_info = Get_Busoffset_info_32bits();
        int offset_ysrc = Get_Busoffset_ysrc(width * height);
        int offset_uv = Get_Busoffset_uvsrc(((width + 1) >> 1) * ((height + 1) >> 1));
        int offset_level = Get_Busoffset_level(num_mb); // 32bits only
        int offset_prob = 2048;                         // 8 bits only
        p_info += offset_info;
        ysrc += offset_ysrc;
        usrc += offset_uv;
        vsrc += offset_uv;
        pout_level += offset_level;
        pout_prob += offset_prob;
    } // while( pid_mult!=0);
}

// namespace xf {
// namespace codec {
extern "C" {
void webp_IntraPredLoop2_NoOut_1(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    /*    for(int i=0;i<5;i++){
            for(int j=0;j<256;j++){
                    pout_prob[i*2048+j]=p_info[j]>>24;
                    pout_prob[i*2048+j]=p_info[j]>>16;
                    pout_prob[i*2048+j]=p_info[j]>>8;
                    pout_prob[i*2048+j]=p_info[j];

            }
            for(int j=0;j<1024;j++){

            if(j==1200-1024)
                    pout_prob[i*2048+1200]=5-1-i;
            else
                    pout_prob[i*2048+1024+j]=(j&(15<<4))+i;
            }
        }*/
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
}
}
//} // namespace codec
//} // namespace xf

extern "C" {
void kernel_IntraPredLoop2_NoOut_2(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifdef USING_PIC_BURST
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#else
    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#endif
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_3(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifdef USING_PIC_BURST
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#else
    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#endif
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_4(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifdef USING_PIC_BURST
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#else
    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#endif
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_5(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifdef USING_PIC_BURST
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#else
    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#endif
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_6(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_7(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifdef USING_PIC_BURST
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#else
    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#endif
}
}

extern "C" {
void kernel_IntraPredLoop2_NoOut_8(
    int32_t* p_info, uint32_t* ysrc, uint32_t* usrc, uint32_t* vsrc, int32_t* pout_level, uint8_t* pout_prob) {
#pragma HLS INTERFACE m_axi port = pout_level offset = slave bundle = gmem0 depth =              \
    65536 * 512 / 2 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = p_info offset = slave bundle = gmem1 depth = 64 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = ysrc offset = slave bundle = gmem1 depth =                    \
    4096 * 4096 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = usrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = vsrc offset = slave bundle = gmem1 depth =                    \
    2048 * 2048 / 4 num_read_outstanding = 32 num_write_outstanding = 32 max_read_burst_length = \
        16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE m_axi port = pout_prob offset = slave bundle = gmem2 depth = 2048 * 64 num_read_outstanding = \
    32 num_write_outstanding = 32 max_read_burst_length = 16 max_write_burst_length = 16 max_widen_bitwidth = 32
#pragma HLS INTERFACE s_axilite port = p_info bundle = control
#pragma HLS INTERFACE s_axilite port = ysrc bundle = control
#pragma HLS INTERFACE s_axilite port = usrc bundle = control
#pragma HLS INTERFACE s_axilite port = vsrc bundle = control
#pragma HLS INTERFACE s_axilite port = pout_level bundle = control
#pragma HLS INTERFACE s_axilite port = pout_prob bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifdef USING_PIC_BURST
    kernel_IntraPredLoop2_NoOut_core_wrapper(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#else
    kernel_IntraPredLoop2_NoOut_core(p_info, ysrc, usrc, vsrc, pout_level, pout_prob);
#endif
}
}
