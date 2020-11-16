/*
 * Copyright 2019 Xilinx, Inc.
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
#ifndef __SYNTHESIS__
#include <stdio.h>
#include <iostream>
#endif

#include "xf_database/gqe_kernel_join_v2.hpp"

#include "xf_database/gqe_blocks_v2/load_config.hpp"
#include "xf_database/gqe_blocks_v2/write_out.hpp"
#include "xf_database/gqe_blocks_v2/scan_col_bufs.hpp"
#include "xf_database/gqe_blocks_v2/hash_join.hpp"

#include "xf_database/gqe_blocks/stream_helper.hpp"
#include "xf_database/gqe_blocks/filter_part.hpp"
#include "xf_database/gqe_blocks/aggr_part.hpp"

#include "xf_utils_hw/stream_shuffle.hpp"

#include <ap_int.h>
#include <hls_stream.h>

namespace xf {
namespace database {
namespace gqe {

static void eval_wrapper(hls::stream<ap_uint<289> >& alu_cfg_strm,
                         hls::stream<ap_uint<8 * TPCH_INT_SZ> >& key0_strm,
                         hls::stream<ap_uint<8 * TPCH_INT_SZ> >& key1_strm,
                         hls::stream<ap_uint<8 * TPCH_INT_SZ> >& key2_strm,
                         hls::stream<ap_uint<8 * TPCH_INT_SZ> >& key3_strm,
                         hls::stream<bool>& e_keys_strm,
                         hls::stream<ap_uint<8 * TPCH_INT_SZ> >& out_strm,
                         hls::stream<bool>& e_out_strm) {
    ap_uint<289> alu_cfg = alu_cfg_strm.read();
    xf::database::dynamicEval<ap_uint<8 * TPCH_INT_SZ>, ap_uint<8 * TPCH_INT_SZ>, ap_uint<8 * TPCH_INT_SZ>,
                              ap_uint<8 * TPCH_INT_SZ>, ap_uint<8 * TPCH_INT_SZ>, ap_uint<8 * TPCH_INT_SZ>,
                              ap_uint<8 * TPCH_INT_SZ>, ap_uint<8 * TPCH_INT_SZ>, ap_uint<8 * TPCH_INT_SZ> >(
        alu_cfg, key0_strm, key1_strm, key2_strm, key3_strm, e_keys_strm, out_strm, e_out_strm);
}

static void dynamic_eval_stage(hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[4],
                               hls::stream<bool>& e_in_strm,
                               hls::stream<ap_uint<8 * TPCH_INT_SZ> > out_strm[5],
                               hls::stream<bool>& e_out_strm,
                               hls::stream<ap_uint<289> >& alu_cfg_strm) {
#pragma HLS dataflow

    hls::stream<bool> e_dummy_strm[1];
#pragma HLS stream variable = e_dummy_strm depth = 4

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > mid_strm[4];
#pragma HLS stream variable = mid_strm depth = 4
#pragma HLS array_partition variable = mid_strm dim = 0
    hls::stream<bool> e_mid_strm;
#pragma HLS stream variable = e_mid_strm depth = 4

    // duplicate 4 col streams
    dup_strm<4>(in_strm, e_in_strm, out_strm, e_dummy_strm[0], mid_strm, e_mid_strm);

    // discard extra flags
    e_sink<1>(e_dummy_strm);

    // evaluate into 5th col stream
    eval_wrapper(alu_cfg_strm,                                                   //
                 mid_strm[0], mid_strm[1], mid_strm[2], mid_strm[3], e_mid_strm, //
                 out_strm[4], e_out_strm);

#ifndef __SYNTHESIS__
    for (int c = 0; c < 4; ++c) {
        size_t s = mid_strm[c].size();
        if (s != 0) {
            printf("mid_strm[%d] has %ld left over data.\n", c, s);
        }
    }
#endif
}

/* read each strm til empty, and then next */
static void merge(hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[4],
                  hls::stream<bool> e_in_strm[4],
                  hls::stream<ap_uint<8 * TPCH_INT_SZ> >& out_strm,
                  hls::stream<bool>& e_out_strm) {
    for (int k = 0; k < 4; ++k) {
        bool e = e_in_strm[k].read();
        while (!e) {
#pragma HLS pipeline II = 1
            e = e_in_strm[k].read();
            ap_uint<8 * TPCH_INT_SZ> in = in_strm[k].read();
            out_strm.write(in);
            e_out_strm.write(false);
        }
    }
    e_out_strm.write(true);
}

void combine(hls::stream<ap_uint<32> > in_strm0[5],
             hls::stream<bool>& e_in_strm0,
             hls::stream<ap_uint<32> > in_strm1[4],
             hls::stream<bool>& e_in_strm1,
             hls::stream<ap_uint<32> > out_strm[9],
             hls::stream<bool>& e_out_strm) {
    bool e = e_in_strm0.read();
    e_in_strm1.read();
    while (!e) {
#pragma HLS PIPELINE II = 1
        for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
            out_strm[i].write(in_strm0[i].read());
            out_strm[i + 4].write(in_strm1[i].read());
        }
        out_strm[8].write(in_strm0[4].read());

        e = e_in_strm0.read();
        e_in_strm1.read();
        e_out_strm.write(false);
    }
    e_out_strm.write(true);
}

void split(hls::stream<ap_uint<32> > in_strm[8],
           hls::stream<bool>& e_in_strm,
           hls::stream<ap_uint<32> > out_strm0[4],
           hls::stream<bool>& e_out_strm0,
           hls::stream<ap_uint<32> > out_strm1[4],
           hls::stream<bool>& e_out_strm1) {
    bool e = e_in_strm.read();
    while (!e) {
#pragma HLS PIPELINE II = 1
        for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
            out_strm0[i].write(in_strm[i].read());
        }
        for (int i = 0; i < 4; i++) {
#pragma HLS UNROLL
            out_strm1[i].write(in_strm[i + 4].read());
        }
        e_out_strm0.write(false);
        e_out_strm1.write(false);
        e = e_in_strm.read();
    }
    e_out_strm0.write(true);
    e_out_strm1.write(true);
}

void shuffle0_wrapper(hls::stream<ap_uint<8 * 8> > order_cfg[4],
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > istrms[4][8],
                      hls::stream<bool> e_istrm[4],
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > ostrms[4][8],
                      hls::stream<bool> e_ostrm[4]) {
    for (int j = 0; j < 4; j++) {
#pragma HLS UNROLL
        xf::common::utils_hw::streamShuffle<8, 8>(order_cfg[j], istrms[j], e_istrm[j], ostrms[j], e_ostrm[j]);
    }
}

void shuffle1_wrapper(hls::stream<bool>& join_on_strm,
                      hls::stream<ap_uint<8 * 8> > order_cfg[4],
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > istrms[4][8],
                      hls::stream<bool> e_istrm[4],
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > ostrms[4][8],
                      hls::stream<bool> e_ostrm[4]) {
    bool join_on = join_on_strm.read();
    if (join_on) {
        for (int j = 0; j < 4; j++) {
#pragma HLS UNROLL
            xf::common::utils_hw::streamShuffle<8, 8>(order_cfg[j], istrms[j], e_istrm[j], ostrms[j], e_ostrm[j]);
        }
#if 0
        for (int j = 0; j < 4; j++) {
#pragma HLS UNROLL
            xf::common::utils_hw::streamShuffle<8, 8>(order_cfg[j], istrms[j], e_istrm[j], ostrms[j], e_ostrm[j]);
        }
#endif
    } else {
        for (int j = 0; j < 4; j++) {
#pragma HLS UNROLL
            xf::common::utils_hw::streamShuffle<8, 8>(order_cfg[j], istrms[j], e_istrm[j], ostrms[j], e_ostrm[j]);
        }
    }
}

void shuffle2_wrapper(hls::stream<bool>& join_on_strm,
                      hls::stream<ap_uint<8 * 8> >& order_cfg,
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > istrms[14],
                      hls::stream<bool>& e_istrm,
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > ostrms[8],
                      hls::stream<bool>& e_ostrm) {
    bool join_on = join_on_strm.read();
    if (join_on) {
        xf::common::utils_hw::streamShuffle<14, 8>(order_cfg, istrms, e_istrm, ostrms, e_ostrm);
    }
}

template <int JN_NM, int COL_NM, int CH_NM>
void load_scan_wrapper(bool build_probe_flag,
                       ap_uint<512>* tin_meta,
                       ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_D[9],
                       hls::stream<bool> join_on_strm[JN_NM],
                       hls::stream<bool>& join_dual_key_on_strm,
                       hls::stream<bool>& agg_on_strm,
                       hls::stream<ap_uint<3> >& join_flag_strm,
                       hls::stream<ap_uint<32> >& write_out_cfg_strm,
                       hls::stream<ap_uint<289> >& alu1_cfg_strm,
                       hls::stream<ap_uint<289> >& alu2_cfg_strm,
                       hls::stream<ap_uint<32> >& filter_cfg_strm,
                       hls::stream<ap_uint<8 * 8> > shuffle1_cfg_strm[4],
                       hls::stream<ap_uint<8 * 8> >& shuffle2_cfg_strm,
                       hls::stream<ap_uint<8 * 8> >& shuffle3_cfg_strm,
                       hls::stream<ap_uint<8 * 8> >& shuffle4_cfg_strm,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A1,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A2,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A3,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A4,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A5,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A6,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A7,
                       ap_uint<8 * TPCH_INT_SZ * VEC_SCAN>* buf_A8,
                       hls::stream<ap_uint<8 * TPCH_INT_SZ> > out_strm[CH_NM][COL_NM],
                       hls::stream<bool> e_out_strm[CH_NM]) {
    hls::stream<int> nrow_strm;
#pragma HLS stream variable = nrow_strm depth = 2
    hls::stream<int8_t> cid_A_strm;
#pragma HLS stream variable = cid_A_strm depth = 32
    load_config<JN_NM>(build_probe_flag, tin_meta, buf_D, join_on_strm, join_dual_key_on_strm, agg_on_strm,
                       join_flag_strm, nrow_strm, cid_A_strm, write_out_cfg_strm, alu1_cfg_strm, alu2_cfg_strm,
                       filter_cfg_strm,
                       /*shuffle0_cfg,*/ shuffle1_cfg_strm, shuffle2_cfg_strm, shuffle3_cfg_strm, shuffle4_cfg_strm);

    scan_col_bufs<COL_NM, CH_NM>(buf_A1, buf_A2, buf_A3, buf_A4, buf_A5, buf_A6, buf_A7, buf_A8, nrow_strm, cid_A_strm,
                                 join_on_strm[0], out_strm, e_out_strm);
}
} // namespace gqe
} // namespace database
} // namespace xf

extern "C" void gqeJoin(ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A8[TEST_BUF_DEPTH],
                        size_t _build_probe_flag,
                        ap_uint<512> tin_meta[24],
                        ap_uint<512> tout_meta[24],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C8[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_D[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf0[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf0[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf7[TEST_BUF_DEPTH]) {
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_0 port = buf_A1
#pragma HLS INTERFACE s_axilite port = buf_A1 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_1 port = buf_A2
#pragma HLS INTERFACE s_axilite port = buf_A2 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_2 port = buf_A3
#pragma HLS INTERFACE s_axilite port = buf_A3 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_3 port = buf_A4
#pragma HLS INTERFACE s_axilite port = buf_A4 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_0 port = buf_A5
#pragma HLS INTERFACE s_axilite port = buf_A5 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_1 port = buf_A6
#pragma HLS INTERFACE s_axilite port = buf_A6 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_2 port = buf_A7
#pragma HLS INTERFACE s_axilite port = buf_A7 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_3 port = buf_A8
#pragma HLS INTERFACE s_axilite port = buf_A8 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_4 port = tin_meta
#pragma HLS INTERFACE s_axilite port = tin_meta bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = tout_meta
#pragma HLS INTERFACE s_axilite port = tout_meta bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C1
#pragma HLS INTERFACE s_axilite port = buf_C1 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C2
#pragma HLS INTERFACE s_axilite port = buf_C2 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C3
#pragma HLS INTERFACE s_axilite port = buf_C3 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C4
#pragma HLS INTERFACE s_axilite port = buf_C4 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C5
#pragma HLS INTERFACE s_axilite port = buf_C5 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C6
#pragma HLS INTERFACE s_axilite port = buf_C6 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C7
#pragma HLS INTERFACE s_axilite port = buf_C7 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem3_0 port = buf_C8
#pragma HLS INTERFACE s_axilite port = buf_C8 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_4 port = buf_D
#pragma HLS INTERFACE s_axilite port = buf_D bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_0 port = htb_buf0
#pragma HLS INTERFACE s_axilite port = htb_buf0 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_1 port = htb_buf1
#pragma HLS INTERFACE s_axilite port = htb_buf1 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_2 port = htb_buf2
#pragma HLS INTERFACE s_axilite port = htb_buf2 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_3 port = htb_buf3
#pragma HLS INTERFACE s_axilite port = htb_buf3 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_4 port = htb_buf4
#pragma HLS INTERFACE s_axilite port = htb_buf4 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_5 port = htb_buf5
#pragma HLS INTERFACE s_axilite port = htb_buf5 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_6 port = htb_buf6
#pragma HLS INTERFACE s_axilite port = htb_buf6 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem1_7 port = htb_buf7
#pragma HLS INTERFACE s_axilite port = htb_buf7 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_0 port = stb_buf0
#pragma HLS INTERFACE s_axilite port = stb_buf0 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_1 port = stb_buf1
#pragma HLS INTERFACE s_axilite port = stb_buf1 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_2 port = stb_buf2
#pragma HLS INTERFACE s_axilite port = stb_buf2 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_3 port = stb_buf3
#pragma HLS INTERFACE s_axilite port = stb_buf3 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_4 port = stb_buf4
#pragma HLS INTERFACE s_axilite port = stb_buf4 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_5 port = stb_buf5
#pragma HLS INTERFACE s_axilite port = stb_buf5 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_6 port = stb_buf6
#pragma HLS INTERFACE s_axilite port = stb_buf6 bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_7 port = stb_buf7
#pragma HLS INTERFACE s_axilite port = stb_buf7 bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS INTERFACE s_axilite port = _build_probe_flag bundle = control

    // clang-format on
    using namespace xf::database::gqe;

#pragma HLS dataflow
    const int jn_on_nm = 8;
    const int agg_on_nm = 3;

    hls::stream<bool> join_on_strm[jn_on_nm];
#pragma HLS stream variable = join_on_strm depth = 32
#pragma HLS array_partition variable = join_on_strm dim = 0

    hls::stream<bool> join_dual_key_on_strm;
#pragma HLS stream variable = join_dual_key_on_strm depth = 32

    hls::stream<ap_uint<32> > fcfg;
#pragma HLS stream variable = fcfg depth = 128
#pragma HLS resource variable = fcfg core = FIFO_LUTRAM

    hls::stream<ap_uint<289> > alu1_cfg_strm;
#pragma HLS stream variable = alu1_cfg_strm depth = 32
#pragma HLS resource variable = alu1_cfg_strm core = FIFO_LUTRAM

    hls::stream<ap_uint<289> > alu2_cfg_strm;
#pragma HLS stream variable = alu2_cfg_strm depth = 32
#pragma HLS resource variable = alu2_cfg_strm core = FIFO_LUTRAM

    hls::stream<bool> agg_on_strm;
#pragma HLS stream variable = agg_on_strm depth = 32

    hls::stream<ap_uint<32> > write_cfg_strm;
#pragma HLS stream variable = write_cfg_strm depth = 32

    hls::stream<ap_uint<8 * 8> > shuffle0_cfg[4];
#pragma HLS stream variable = shuffle0_cfg depth = 32
#pragma HLS array_partition variable = shuffle0_cfg dim = 0

    hls::stream<ap_uint<8 * 8> > shuffle1_cfg[4];
#pragma HLS stream variable = shuffle1_cfg depth = 32
#pragma HLS array_partition variable = shuffle1_cfg dim = 0

    hls::stream<ap_uint<8 * 8> > shuffle2_cfg;
#pragma HLS stream variable = shuffle2_cfg depth = 32

    hls::stream<ap_uint<8 * 8> > shuffle3_cfg;
#pragma HLS stream variable = shuffle3_cfg depth = 32

    hls::stream<ap_uint<8 * 8> > shuffle4_cfg;
#pragma HLS stream variable = shuffle4_cfg depth = 32

    const int nch = 4;
    const int scan_num = 1;

    hls::stream<ap_uint<32> > ch_strms[nch][8];
#pragma HLS stream variable = ch_strms depth = 32
#pragma HLS array_partition variable = ch_strms dim = 1
#pragma HLS resource variable = ch_strms core = FIFO_LUTRAM

    hls::stream<bool> e_ch_strms[nch];
#pragma HLS stream variable = e_ch_strms depth = 32

    hls::stream<ap_uint<32> > ch_strms_shuffle0[nch][8];
#pragma HLS stream variable = ch_strms_shuffle0 depth = 32
#pragma HLS array_partition variable = ch_strms_shuffle0 dim = 1
#pragma HLS resource variable = ch_strms_shuffle0 core = FIFO_LUTRAM

    hls::stream<bool> e_ch_strms_shuffle0[nch];
#pragma HLS stream variable = e_ch_strms_shuffle0 depth = 32

    hls::stream<ap_uint<3> > join_flag_strm;
#pragma HLS stream variable = join_flag_strm depth = 32
#pragma HLS resource variable = join_flag_strm core = FIFO_LUTRAM

#ifndef __SYNTHESIS__
    printf("************************************************************\n");
    printf("             General Query Egnine Kernel\n");
    printf("************************************************************\n");
#endif

#ifndef __SYNTHESIS__
    std::cout << "===================meta buffer ===================" << std::endl;
    std::cout << "tin_meta.vnum is " << tin_meta[0].range(7, 0) << std::endl;
    std::cout << "tin_meta.length is " << tin_meta[0].range(71, 8) << std::endl;
    std::cout << std::endl;
#endif

    bool build_probe_flag = (bool)(_build_probe_flag);
#ifndef __SYNTHESIS__
    std::cout << "_build_probe_flag= " << _build_probe_flag << " build_probe_flag= " << build_probe_flag << std::endl;
#endif
#if 0
    load_config<jn_on_nm>(build_probe_flag, tin_meta, buf_D, join_on_strm, join_dual_key_on_strm, agg_on_strm,
                          join_flag_strm, nrow_strm, cid_A_strm, write_cfg_strm, alu1_cfg_strm, alu2_cfg_strm, fcfg,
                          /*shuffle0_cfg,*/ shuffle1_cfg, shuffle2_cfg, shuffle3_cfg, shuffle4_cfg);

    scan_col_bufs<8, nch>((ap_uint<256>*)buf_A1, (ap_uint<256>*)buf_A2, (ap_uint<256>*)buf_A3, (ap_uint<256>*)buf_A4,
                          (ap_uint<256>*)buf_A5, (ap_uint<256>*)buf_A6, (ap_uint<256>*)buf_A7, (ap_uint<256>*)buf_A8,
                          nrow_strm, cid_A_strm, join_on_strm[0], ch_strms, e_ch_strms);
#endif

    load_scan_wrapper<jn_on_nm, 8, nch>(build_probe_flag, tin_meta, buf_D, join_on_strm, join_dual_key_on_strm,
                                        agg_on_strm, join_flag_strm, write_cfg_strm, alu1_cfg_strm, alu2_cfg_strm, fcfg,
                                        shuffle1_cfg, shuffle2_cfg, shuffle3_cfg, shuffle4_cfg, buf_A1, buf_A2, buf_A3,
                                        buf_A4, buf_A5, buf_A6, buf_A7, buf_A8, ch_strms, e_ch_strms);

#ifndef __SYNTHESIS__
    {
        printf("***** after Scan\n");
        size_t ss = 0;
        for (int ch = 0; ch < nch; ++ch) {
            size_t s = ch_strms[ch][0].size();
            printf("ch:%d nrow=%ld\n", ch, s);
            ss += s;
        }
        printf("total: nrow=%ld\n", ss);
    }
    for (int ch = 0; ch < nch; ++ch) {
        size_t s = ch_strms[ch][0].size();
        for (int c = 1; c < 5; ++c) {
            size_t ss = ch_strms[ch][c].size();
            if (s != ss) {
                printf(
                    "##### ch_strms[%d][0] has %ld row, ch_strms[%d][%d] has %ld "
                    "row.\n",
                    ch, s, ch, c, ss);
            }
        }
    }
#endif

    //    shuffle0_wrapper(shuffle0_cfg, ch_strms, e_ch_strms, ch_strms_shuffle0, e_ch_strms_shuffle0);

    // filter part
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > flt_strms[nch][8];
#pragma HLS stream variable = flt_strms depth = 32
#pragma HLS array_partition variable = flt_strms dim = 1
#pragma HLS resource variable = flt_strms core = FIFO_LUTRAM
    hls::stream<bool> e_flt_strms[4];
#pragma HLS stream variable = e_flt_strms depth = 32

    filter_ongoing<8, 8, 4>(join_on_strm[1].read(),
                            fcfg, //
                            ch_strms,
                            e_ch_strms, //
                            flt_strms, e_flt_strms);

#ifndef __SYNTHESIS__
    {
        printf("***** after Filter\n");
        size_t ss = 0;
        for (int ch = 0; ch < nch; ++ch) {
            size_t s = e_flt_strms[ch].size() - 1;
            printf("ch:%d nrow=%ld\n", ch, s);
            ss += s;
        }
        printf("total: nrow=%ld\n", ss);
    }
    for (int ch = 0; ch < nch; ++ch) {
        size_t s = flt_strms[ch][0].size();
        for (int c = 1; c < 4; ++c) {
            size_t ss = flt_strms[ch][c].size();
            if (s != ss) {
                printf(
                    "##### flt_strms[%d][0] has %ld row, flt_strms[%d][%d] has %ld "
                    "row.\n",
                    ch, s, ch, c, ss);
            }
        }
    }
    for (int ch = 0; ch < nch; ++ch) {
        for (int c = 0; c < 5; ++c) {
            if (ch_strms[ch][c].size() != 0) {
                printf("##### ch_strms[%d][%d] has data left after hash-join.\n", ch, c);
            }
        }
    }
    for (int ch = 0; ch < nch; ++ch) {
        printf("end flag nubmer %d and data number %d\n", e_flt_strms[ch].size(), flt_strms[ch][0].size());
    }
#endif

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > hj_in[4][8];
#pragma HLS stream variable = hj_in depth = 32
#pragma HLS array_partition variable = hj_in dim = 1
#pragma HLS resource variable = hj_in core = FIFO_LUTRAM
    hls::stream<bool> e_hj_in[4];
#pragma HLS stream variable = e_hj_in depth = 32

    shuffle1_wrapper(join_on_strm[6], shuffle1_cfg, flt_strms, e_flt_strms, hj_in, e_hj_in);
    // printf("shuffle done\n");
    // add demux 1-way data after filter to 2-way data
    // one for hash join, another one bypass
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > flt_dm_strms_0[nch][8];
#pragma HLS stream variable = flt_dm_strms_0 depth = 32
#pragma HLS array_partition variable = flt_dm_strms_0 dim = 1
#pragma HLS resource variable = flt_dm_strms_0 core = FIFO_LUTRAM
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > flt_dm_strms_1[nch][8];
#pragma HLS stream variable = flt_dm_strms_1 depth = 32
#pragma HLS array_partition variable = flt_dm_strms_1 dim = 1
#pragma HLS resource variable = flt_dm_strms_1 core = FIFO_LUTRAM

    hls::stream<bool> e_flt_dm_strms_0[4];
#pragma HLS stream variable = e_flt_dm_strms_0 depth = 32
    hls::stream<bool> e_flt_dm_strms_1[4];
#pragma HLS stream variable = e_flt_dm_strms_1 depth = 32

    demux_wrapper<8, nch, scan_num>(join_on_strm[2], hj_in, e_hj_in, flt_dm_strms_0, flt_dm_strms_1, e_flt_dm_strms_0,
                                    e_flt_dm_strms_1);
    // printf("Demux done\n");

    hls::stream<ap_uint<32> > jn_strm[14];
#pragma HLS stream variable = jn_strm depth = 32
#pragma HLS array_partition variable = jn_strm complete
#pragma HLS resource variable = jn_strm core = FIFO_LUTRAM
    hls::stream<bool> e_jn_strm;
#pragma HLS stream variable = e_jn_strm depth = 32 //

    hash_join_wrapper<8, nch, 14, 1>(build_probe_flag, join_flag_strm, join_on_strm[3], join_dual_key_on_strm,
                                     flt_dm_strms_1, e_flt_dm_strms_1, jn_strm,
                                     e_jn_strm, //
                                     htb_buf0, htb_buf1, htb_buf2, htb_buf3, htb_buf4, htb_buf5, htb_buf6, htb_buf7,
                                     stb_buf0, stb_buf1, stb_buf2, stb_buf3, stb_buf4, stb_buf5, stb_buf6, stb_buf7);
#if 1
    hls::stream<ap_uint<32> > hj_out[8];
#pragma HLS stream variable = hj_out depth = 32
#pragma HLS array_partition variable = hj_out complete
#pragma HLS resource variable = jn_strm core = FIFO_LUTRAM
    hls::stream<bool> e_hj_out;
#pragma HLS stream variable = e_hj_out depth = 32 //

    shuffle2_wrapper(join_on_strm[7], shuffle2_cfg, jn_strm, e_jn_strm, hj_out, e_hj_out);

#ifndef __SYNTHESIS__
    {
        printf("***** after Shuffle2\n");
        size_t s = e_hj_out.size() - 1;
        printf("nrow=%ld\n", s);
    }
#endif

    // 2nd data flow merge channel and bypass.
    hls::stream<ap_uint<32> > jn_bp_strm[8];
#pragma HLS stream variable = jn_bp_strm depth = 32
#pragma HLS array_partition variable = jn_bp_strm complete
#pragma HLS resource variable = jn_bp_strm core = FIFO_LUTRAM
    hls::stream<bool> e_jn_bp_strm;
#pragma HLS stream variable = e_jn_bp_strm depth = 32
    hash_join_bypass<8, nch>(join_on_strm[4], flt_dm_strms_0, e_flt_dm_strms_0, jn_bp_strm, e_jn_bp_strm);

    // mux the 2-way data, one from hash join, another one fron bypass
    hls::stream<ap_uint<32> > jn_mx_strm[8];
#pragma HLS stream variable = jn_mx_strm depth = 32
#pragma HLS array_partition variable = jn_mx_strm complete
#pragma HLS resource variable = jn_mx_strm core = FIFO_LUTRAM
    hls::stream<bool> e_jn_mx_strm;
#pragma HLS stream variable = e_jn_mx_strm depth = 32

#ifndef __SYNTHESIS__
    {
        printf("***** after Bypass \n");
        size_t s = e_jn_bp_strm.size() - 1;
        printf("nrow=%ld\n", s);
    }
#endif

    stream1D_mux2To1<8 * TPCH_INT_SZ, 8>(join_on_strm[5], jn_bp_strm, hj_out, e_jn_bp_strm, e_hj_out, jn_mx_strm,
                                         e_jn_mx_strm);
#ifndef __SYNTHESIS__
    {
        printf("***** after Join-Bypass MUX\n");
        size_t s = e_jn_mx_strm.size() - 1;
        printf("nrow=%ld\n", s);
    }
#endif

// write jn_mx_strm or go to eval-aggr pipeline

#if GQEJOIN_WITHOUT_AGGR

    writeTableMeta<BURST_LEN, 8 * TPCH_INT_SZ, VEC_LEN, 8>(build_probe_flag, jn_mx_strm, e_jn_mx_strm, buf_C1, buf_C2,
                                                           buf_C3, buf_C4, buf_C5, buf_C6, buf_C7, buf_C8, tout_meta,
                                                           write_cfg_strm);

#else
    // Evalaution 1
    hls::stream<ap_uint<32> > eval1_in[4];
#pragma HLS stream variable = eval1_in depth = 32
#pragma HLS array_partition variable = eval1_in complete
#pragma HLS resource variable = eval1_in core = FIFO_LUTRAM
    hls::stream<bool> e_eval1_in;
#pragma HLS stream variable = e_eval1_in depth = 32

    hls::stream<ap_uint<32> > eval1_bp[4];
#pragma HLS stream variable = eval1_bp depth = 32
#pragma HLS array_partition variable = eval1_bp complete
#pragma HLS resource variable = eval1_bp core = FIFO_LUTRAM
    hls::stream<bool> e_eval1_bp;
#pragma HLS stream variable = e_eval1_bp depth = 32
    split(jn_mx_strm, e_jn_mx_strm, eval1_in, e_eval1_in, eval1_bp, e_eval1_bp);

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > eval1_strm[5];
#pragma HLS array_partition variable = eval1_strm dim = 0
#pragma HLS stream variable = eval1_strm depth = 32
#pragma HLS resource variable = eval1_strm core = FIFO_LUTRAM

    hls::stream<bool> e_eval1_strm;
#pragma HLS stream variable = e_eval1_strm depth = 32
    dynamic_eval_stage(eval1_in, e_eval1_in, eval1_strm, e_eval1_strm, alu1_cfg_strm);

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > eval1[9];
#pragma HLS array_partition variable = eval1 dim = 0
#pragma HLS stream variable = eval1 depth = 32
#pragma HLS resource variable = eval1 core = FIFO_LUTRAM
    hls::stream<bool> e_eval1;
#pragma HLS stream variable = e_eval1 depth = 32
    combine(eval1_strm, e_eval1_strm, eval1_bp, e_eval1_bp, eval1, e_eval1);

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > eval1_res[8];
#pragma HLS array_partition variable = eval1_res dim = 0
#pragma HLS stream variable = eval1_res depth = 32
#pragma HLS resource variable = eval1_res core = FIFO_LUTRAM
    hls::stream<bool> e_eval1_res;
#pragma HLS stream variable = e_eval1_res depth = 32
    xf::common::utils_hw::streamShuffle<9, 8>(shuffle3_cfg, eval1, e_eval1, eval1_res, e_eval1_res);

    // Evalaution 2
    hls::stream<ap_uint<32> > eval2_in[4];
#pragma HLS stream variable = eval2_in depth = 32
#pragma HLS array_partition variable = eval2_in complete
#pragma HLS resource variable = eval2_in core = FIFO_LUTRAM
    hls::stream<bool> e_eval2_in;
#pragma HLS stream variable = e_eval2_in depth = 32

    hls::stream<ap_uint<32> > eval2_bp[4];
#pragma HLS stream variable = eval2_bp depth = 32
#pragma HLS array_partition variable = eval2_bp complete
#pragma HLS resource variable = eval2_bp core = FIFO_LUTRAM
    hls::stream<bool> e_eval2_bp;
#pragma HLS stream variable = e_eval2_bp depth = 32
    split(eval1_res, e_eval1_res, eval2_in, e_eval2_in, eval2_bp, e_eval2_bp);

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > eval2_strm[5];
#pragma HLS array_partition variable = eval2_strm dim = 0
#pragma HLS stream variable = eval2_strm depth = 32
#pragma HLS resource variable = eval2_strm core = FIFO_LUTRAM
    hls::stream<bool> e_eval2_strm;
#pragma HLS stream variable = e_eval2_strm depth = 32
    dynamic_eval_stage(eval2_in, e_eval2_in, eval2_strm, e_eval2_strm, alu2_cfg_strm);

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > eval2[9];
#pragma HLS array_partition variable = eval2 dim = 0
#pragma HLS stream variable = eval2 depth = 32
#pragma HLS resource variable = eval2 core = FIFO_LUTRAM
    hls::stream<bool> e_eval2;
#pragma HLS stream variable = e_eval2 depth = 32
    combine(eval2_strm, e_eval2_strm, eval2_bp, e_eval2_bp, eval2, e_eval2);

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > eval2_res[8];
#pragma HLS array_partition variable = eval2_res dim = 0
#pragma HLS stream variable = eval2_res depth = 32
#pragma HLS resource variable = eval2_res core = FIFO_LUTRAM
    hls::stream<bool> e_eval2_res;
#pragma HLS stream variable = e_eval2_res depth = 32
    xf::common::utils_hw::streamShuffle<9, 8>(shuffle4_cfg, eval2, e_eval2, eval2_res, e_eval2_res);

    // Aggregate
    hls::stream<ap_uint<32> > agg_strm[8];
#pragma HLS stream variable = agg_strm depth = 32
#pragma HLS resource variable = agg_strm core = FIFO_LUTRAM
    hls::stream<bool> e_agg_strm;
#pragma HLS stream variable = e_agg_strm depth = 32
    agg_wrapper<8>(agg_on_strm, eval2_res, e_eval2_res, agg_strm, e_agg_strm);

#ifndef __SYNTHESIS__
    {
        printf("***** after Eval & Aggr\n");
        printf("nrow=%ld\n", agg_strm[0].size());
    }
    {
        size_t s = agg_strm[0].size();
        for (int c = 1; c < 5; ++c) {
            size_t ss = agg_strm[c].size();
            if (s != ss) {
                printf("##### agg_strm[0] has %ld row, agg_strm[%d] has %ld row.\n", s, c, ss);
            }
        }
    }
    for (int c = 0; c < 4; ++c) {
        size_t s = jn_strm[c].size();
        if (s != 0) {
            printf("##### jn_strm[%d] has %ld data left after eval-aggr.\n", c, s);
        }
    }
#endif

    writeTableMeta<BURST_LEN, 8 * TPCH_INT_SZ, VEC_LEN, 8>(build_probe_flag, agg_strm, e_agg_strm, buf_C1, buf_C2,
                                                           buf_C3, buf_C4, buf_C5, buf_C6, buf_C7, buf_C8, tout_meta,
                                                           write_cfg_strm);

#ifndef __SYNTHESIS__
    for (int c = 0; c < 4; ++c) {
        size_t s = agg_strm[c].size();
        if (s != 0) {
            printf("##### agg_strm[%d] has %ld data left after write-out.\n", c, s);
        }
    }
#endif

#endif // GQEJOIN_WITHOUT_AGGR
#endif
}
