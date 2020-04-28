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

#include "gqe_join.hpp"
#include "gqe_blocks/stream_helper.hpp"
#include "gqe_blocks/load_config.hpp"
#include "gqe_blocks/scan_to_channel.hpp"
#include "gqe_blocks/filter_part.hpp"
#include "gqe_blocks/hash_join_part.hpp"
#include "gqe_blocks/aggr_part.hpp"
#include "gqe_blocks/write_out.hpp"

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
        for (int j = 0; j < 4; j++) {
#pragma HLS UNROLL
            xf::common::utils_hw::streamShuffle<8, 8>(order_cfg[j], istrms[j], e_istrm[j], ostrms[j], e_ostrm[j]);
        }
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

void scan_wrapper_top(ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* ptr_A,
                      ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* ptr_B,
                      hls::stream<int8_t>& cid_A_strm, // may read once or twice
                      hls::stream<int8_t>& cid_B_strm, // may read none or once
                      hls::stream<bool>& join_on_strm,
                      hls::stream<ap_uint<8 * TPCH_INT_SZ> > out_strm[4][8],
                      hls::stream<bool> e_out_strm[4]) {
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_0 port = ptr_A
#pragma HLS INTERFACE s_axilite port = ptr_A bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_1 port = ptr_B
#pragma HLS INTERFACE s_axilite port = ptr_B bundle = control

    scan_wrapper<8, 4>(ptr_A, ptr_B, cid_A_strm, cid_B_strm, //
                       join_on_strm,                         //
                       out_strm, e_out_strm);
}

} // namespace gqe
} // namespace database
} // namespace xf

extern "C" void gqeJoin(ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_A[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_B[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_D[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf0[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf0[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf7[TEST_BUF_DEPTH]) {
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_0 port = buf_A
#pragma HLS INTERFACE s_axilite port = buf_A bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    16 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_1 port = buf_B
#pragma HLS INTERFACE s_axilite port = buf_B bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem0_2 port = buf_C
#pragma HLS INTERFACE s_axilite port = buf_C bundle = control

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    1 max_write_burst_length = 2 max_read_burst_length = 16 bundle = gmem0_3 port = buf_D
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

    // clang-format on
    using namespace xf::database::gqe;

#pragma HLS dataflow
    const int jn_on_nm = 8;
    const int agg_on_nm = 3;

    hls::stream<int8_t> cid_A_strm;
#pragma HLS stream variable = cid_A_strm depth = 32
    hls::stream<int8_t> cid_B_strm;
#pragma HLS stream variable = cid_B_strm depth = 32

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
    const int scan_num = 2;

    hls::stream<ap_uint<32> > ch_strms[nch][8];
#pragma HLS stream variable = ch_strms depth = 32
#pragma HLS array_partition variable = ch_strms dim = 1
#pragma HLS resource variable = ch_strms core = FIFO_LUTRAM

    hls::stream<bool> e_ch_strms[nch];
#pragma HLS stream variable = e_ch_strms depth = 32

    hls::stream<ap_uint<3> > join_flag_strm;
#pragma HLS stream variable = join_flag_strm depth = 32
#pragma HLS resource variable = join_flag_strm core = FIFO_LUTRAM

#ifndef __SYNTHESIS__
    printf("************************************************************\n");
    printf("             General Query Egnine Kernel\n");
    printf("************************************************************\n");
#endif

    load_config<jn_on_nm>(buf_D, join_on_strm, join_dual_key_on_strm, agg_on_strm, join_flag_strm, cid_A_strm,
                          cid_B_strm, write_cfg_strm, alu1_cfg_strm, alu2_cfg_strm, fcfg, shuffle1_cfg, shuffle2_cfg,
                          shuffle3_cfg, shuffle4_cfg);
    /*
        int size512=buf_B[0].range(63,32);
        int rowNum=buf_B[0].range(31,0);
        TPCH_INT* col0=(TPCH_INT*)(buf_B+size512*0);
        TPCH_INT* col1=(TPCH_INT*)(buf_B+size512*1);
        TPCH_INT* col2=(TPCH_INT*)(buf_B+size512*2);
        TPCH_INT* col3=(TPCH_INT*)(buf_B+size512*3);
        std::cout<<"size512: "<<size512<<std::endl;
        std::cout<<"rowNum:  "<<rowNum<<std::endl;
        std::cout<<"****"<<std::endl;
        for(int i=16;i<36;i++){
            std::cout<<"col0: "<<col0[i]<<" ";
    //        std::cout<<"col1: "<<col1[i]<<" ";
    //        std::cout<<"col2: "<<col2[i]<<" ";
    //        std::cout<<"col4: "<<col3[i]<<" ";
            std::cout<<std::endl;
        }
        std::cout<<std::endl;
    */

    scan_wrapper<8, nch>(buf_A, buf_B, cid_A_strm, cid_B_strm, //
                         join_on_strm[0],                      //
                         ch_strms, e_ch_strms);

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

    // filter part
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > flt_strms[nch][8];
#pragma HLS stream variable = flt_strms depth = 32
#pragma HLS array_partition variable = flt_strms dim = 1
#pragma HLS resource variable = flt_strms core = FIFO_LUTRAM
    hls::stream<bool> e_flt_strms[4];
#pragma HLS stream variable = e_flt_strms depth = 32

    filter_wrapper<8, 8, 4, scan_num>(fcfg, join_on_strm[1], //
                                      ch_strms, e_ch_strms,  //
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

    hash_join_wrapper<8, nch, 14, scan_num>(
        join_flag_strm, join_on_strm[3], join_dual_key_on_strm, flt_dm_strms_1, e_flt_dm_strms_1, jn_strm, e_jn_strm, //
        htb_buf0, htb_buf1, htb_buf2, htb_buf3, htb_buf4, htb_buf5, htb_buf6, htb_buf7, stb_buf0, stb_buf1, stb_buf2,
        stb_buf3, stb_buf4, stb_buf5, stb_buf6, stb_buf7);

    // printf("Hash join done\n");

    hls::stream<ap_uint<32> > hj_out[8];
#pragma HLS stream variable = hj_out depth = 32
#pragma HLS array_partition variable = hj_out complete
#pragma HLS resource variable = jn_strm core = FIFO_LUTRAM
    hls::stream<bool> e_hj_out;
#pragma HLS stream variable = e_hj_out depth = 32 //

    shuffle2_wrapper(join_on_strm[7], shuffle2_cfg, jn_strm, e_jn_strm, hj_out, e_hj_out);

    // 2nd data flow merge channel and bypass.
    hls::stream<ap_uint<32> > jn_bp_strm[8];
#pragma HLS stream variable = jn_bp_strm depth = 32
#pragma HLS array_partition variable = jn_bp_strm complete
#pragma HLS resource variable = jn_bp_strm core = FIFO_LUTRAM
    hls::stream<bool> e_jn_bp_strm;
#pragma HLS stream variable = e_jn_bp_strm depth = 32
    hash_join_bypass<8, nch>(join_on_strm[4], flt_dm_strms_0, e_flt_dm_strms_0, jn_bp_strm, e_jn_bp_strm);

    // printf("Bypass done\n");

    // mux the 2-way data, one from hash join, another one fron bypass
    hls::stream<ap_uint<32> > jn_mx_strm[8];
#pragma HLS stream variable = jn_mx_strm depth = 32
#pragma HLS array_partition variable = jn_mx_strm complete
#pragma HLS resource variable = jn_mx_strm core = FIFO_LUTRAM
    hls::stream<bool> e_jn_mx_strm;
#pragma HLS stream variable = e_jn_mx_strm depth = 32

    stream1D_mux2To1<8 * TPCH_INT_SZ, 8>(join_on_strm[5], jn_bp_strm, hj_out, e_jn_bp_strm, e_hj_out, jn_mx_strm,
                                         e_jn_mx_strm);

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

#ifndef __SYNTHESIS__
    {
        printf("***** after Join\n");
        printf("nrow=%ld\n", jn_mx_strm[0].size());
    }
    {
        size_t s = jn_strm[0].size();
        for (int c = 1; c < 4; ++c) {
            size_t ss = jn_strm[c].size();
            if (s != ss) {
                printf("##### jn_strm[0] has %ld row, jn_strm[%d] has %ld row.\n", s, c, ss);
            }
        }
    }
    for (int ch = 0; ch < nch; ++ch) {
        for (int c = 0; c < 4; ++c) {
            size_t s = flt_strms[ch][c].size();
            if (s != 0) {
                printf("##### flt_strms[%d][%d] has %ld data left after hash-join.\n", ch, c, s);
            }
        }
    }
#endif

    // Add combine after hash join

    // Flaten Eval and Aggregate
    // Evalaution
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
    // Demux the output of eval to 2 flow, one for agg and another by pass.

    // Aggregate
    hls::stream<ap_uint<32> > agg_strm[8];
#pragma HLS stream variable = agg_strm depth = 32
#pragma HLS resource variable = agg_strm core = FIFO_LUTRAM
    hls::stream<bool> e_agg_strm;
#pragma HLS stream variable = e_agg_strm depth = 32

    // one flow through aggr
    // Agg_Wrapper(eval_strm, e_eval_strm, agg_strm, e_agg_strm, agg_on_strm);
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

    writeTableV2<BURST_LEN, 8 * TPCH_INT_SZ, VEC_LEN, 8>(

        agg_strm, e_agg_strm, //
        buf_C, write_cfg_strm);

#ifndef __SYNTHESIS__
    for (int c = 0; c < 4; ++c) {
        size_t s = agg_strm[c].size();
        if (s != 0) {
            printf("##### agg_strm[%d] has %ld data left after write-out.\n", c, s);
        }
    }
#endif
}
