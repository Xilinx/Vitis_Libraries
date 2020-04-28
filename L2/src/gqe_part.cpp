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

#include "gqe_part.hpp"
#include "gqe_blocks/scan_for_hp.hpp"
#include "gqe_blocks/filter_part.hpp"
#include "gqe_blocks/write_for_hp.hpp"
#include "xf_database/hash_partition.hpp"

namespace xf {
namespace database {
namespace gqe {

void load_config(ap_uint<8 * TPCH_INT_SZ * VEC_LEN> ptr[9],
                 const int col_index,
                 bool& mk_on,
                 hls::stream<int8_t>& col_id_strm,
                 hls::stream<ap_uint<32> >& wr_cfg_strm,
                 hls::stream<ap_uint<32> >& filter_cfg_strm) {
    const int filter_cfg_depth = 45;

    ap_uint<8 * TPCH_INT_SZ * VEC_LEN> config[9];
#pragma HLS resource variable = config core = RAM_1P_LUTRAM

    ap_uint<32> filter_cfg_a[filter_cfg_depth];
#pragma HLS resource variable = filter_cfg_a core = RAM_1P_LUTRAM

    for (int i = 0; i < 9; i++) {
#pragma HLS PIPELINE II = 1
        config[i] = ptr[i];
    }

    bool join_on = config[0][0] ? true : false;
    ap_uint<32> write_out_cfg;
    mk_on = config[0][2] ? true : false;
#ifndef __SYNTHESIS__
    if (mk_on)
        std::cout << "\nDual key is on\n";
    else
        std::cout << "\nDual key is off\n";
#endif

    for (int i = 0; i < COL_NUM; i++) {
#pragma HLS PIPELINE II = 1
        int8_t t = config[0].range(64 * col_index + 56 + 8 * i + 7, 64 * col_index + 56 + 8 * i);
        col_id_strm.write(t);
        write_out_cfg[i] = (t >= 0) ? 1 : 0;
    }

    wr_cfg_strm.write(write_out_cfg);

    for (int i = 0; i < filter_cfg_depth; i++) {
        filter_cfg_a[i] = config[3 * col_index + 3 + i / 16].range(32 * ((i % 16) + 1) - 1, 32 * (i % 16));
        filter_cfg_strm.write(filter_cfg_a[i]);
    }
}

/* XXX if dual key, shift the payload,
 * so that for the 3rd col of A table becomes 1st payload
 */
template <int COL_NM, int PLD_NM>
void hash_partition_channel_adapter(bool mk_on,
                                    hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[COL_NM],
                                    hls::stream<bool>& e_in_strm,
                                    hls::stream<ap_uint<8 * TPCH_INT_SZ * 2> >& key_strm,
                                    hls::stream<ap_uint<8 * TPCH_INT_SZ * PLD_NM> >& pld_strm,
                                    hls::stream<bool>& e_strm) {
    bool e = e_in_strm.read();
    while (!e) {
#pragma HLS pipeline II = 1

        ap_uint<8 * TPCH_INT_SZ * 2> key_tmp;
        ap_uint<8 * TPCH_INT_SZ * PLD_NM> pld_tmp;

        ap_uint<8 * TPCH_INT_SZ> d_tmp[COL_NM];
#pragma HLS array_partition variable = d_tmp complete
        for (int c = 0; c < COL_NM; ++c) {
#pragma HLS unroll
            d_tmp[c] = in_strm[c].read();
        }

        key_tmp.range(8 * TPCH_INT_SZ - 1, 0) = d_tmp[0];
        key_tmp.range(8 * TPCH_INT_SZ * 2 - 1, 8 * TPCH_INT_SZ) = mk_on ? d_tmp[1] : ap_uint<8 * TPCH_INT_SZ>(0);

        for (int c = 0; c < PLD_NM; ++c) {
#pragma HLS unroll
            pld_tmp.range(8 * TPCH_INT_SZ * (c + 1) - 1,
                          8 * TPCH_INT_SZ * c) = // check LUT usage
                mk_on ? d_tmp[2 + c] : d_tmp[1 + c];
        }
        // pld_tmp.range(8 * TPCH_INT_SZ * PLD_NM - 1, 8 * TPCH_INT_SZ * (PLD_NM - 1)) =
        // mk_on ? ap_uint<8 * TPCH_INT_SZ>(0) : d_tmp[PLD_NM];

        key_strm.write(key_tmp);
        pld_strm.write(pld_tmp);
        e_strm.write(false);
        e = e_in_strm.read();
    }
    e_strm.write(true);
}

template <int COL_IN_NM, int CH_NM, int COL_OUT_NM>
void hash_partition_wrapper(bool mk_on,
                            int k_depth,
                            hls::stream<int>& bit_num_strm,

                            hls::stream<ap_uint<8 * TPCH_INT_SZ> > in_strm[CH_NM][COL_IN_NM],
                            hls::stream<bool> e_in_strm[CH_NM],

                            hls::stream<ap_uint<16> >& o_bkpu_num_strm,
                            hls::stream<ap_uint<10> >& o_nm_strm,
                            hls::stream<ap_uint<8 * TPCH_INT_SZ> > out_strm[COL_OUT_NM]) {
#pragma HLS dataflow

    hls::stream<ap_uint<8 * TPCH_INT_SZ * 2> > key_strm[CH_NM];
#pragma HLS stream variable = key_strm depth = 16
    hls::stream<ap_uint<8 * TPCH_INT_SZ * 6> > pld_strm[CH_NM];
#pragma HLS stream variable = pld_strm depth = 16
    hls::stream<bool> e_strm[CH_NM];
#pragma HLS stream variable = e_strm depth = 16

    // let each channel adapt independently
    for (int ch = 0; ch < CH_NM; ++ch) {
#pragma HLS unroll
        hash_partition_channel_adapter<COL_IN_NM, 6>( // dual width 1 key, and
                                                      // 3 width payload
            mk_on, in_strm[ch], e_in_strm[ch], key_strm[ch], pld_strm[ch], e_strm[ch]);
    }

#ifndef __SYNTHESIS__
    printf("After adapt for hash partition\n");
    for (int ch = 0; ch < CH_NM; ++ch) {
        printf("ch:%d nrow=%ld,%ld\n", ch, key_strm[ch].size(), pld_strm[ch].size());
    }
#endif

    xf::database::hashPartition<1, 64, 192, 32, HASHWH, HASHWL, 18, CH_NM, COL_OUT_NM>(
        mk_on, k_depth, bit_num_strm, key_strm, pld_strm, e_strm, o_bkpu_num_strm, o_nm_strm, out_strm);
}

} // namespace gqe
} // namespace database
} // namespace xf

/**
 * @breif GQE partition kernel
 *
 * @param k_depth depth of each hash bucket in URAM
 * @param col_index index of input column
 * @param bit_num number of defined partition, log2(number of partition)
 *
 * @param buf_A input table buffer
 * @param buf_B output table buffer
 * @param buf_D configuration buffer
 *
 */
extern "C" void gqePart(const int k_depth,
                        const int col_index,
                        const int bit_num,
                        ap_uint<512> buf_A[TEST_BUF_DEPTH],
                        ap_uint<512> buf_B[TEST_BUF_DEPTH],
                        ap_uint<512> buf_D[TEST_BUF_DEPTH]) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_0 port = buf_A

#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_1 port = buf_B

#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_2 port = buf_D

#pragma HLS INTERFACE s_axilite port = k_depth bundle = control
#pragma HLS INTERFACE s_axilite port = col_index bundle = control
#pragma HLS INTERFACE s_axilite port = bit_num bundle = control
#pragma HLS INTERFACE s_axilite port = buf_A bundle = control
#pragma HLS INTERFACE s_axilite port = buf_B bundle = control
#pragma HLS INTERFACE s_axilite port = buf_D bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // clang-format on
    using namespace xf::database::gqe;

#pragma HLS DATAFLOW

    bool mk_on;
    hls::stream<int8_t> cid_strm;
#pragma HLS stream variable = cid_strm depth = 16
    hls::stream<ap_uint<32> > wr_cfg_strm;
#pragma HLS stream variable = wr_cfg_strm depth = 8

    // scan part
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > ch_strms[CH_NUM][COL_NUM];
#pragma HLS stream variable = ch_strms depth = 32
    hls::stream<bool> e_ch_strms[CH_NUM];
#pragma HLS stream variable = e_ch_strms depth = 32
    hls::stream<int> bit_num_strm;
#pragma HLS stream variable = bit_num_strm depth = 2
    hls::stream<int> bit_num_strm_copy;
#pragma HLS stream variable = bit_num_strm_copy depth = 2

    hls::stream<ap_uint<32> > fcfg;
#pragma HLS stream variable = fcfg depth = 64
#pragma HLS resource variable = fcfg core = FIFO_LUTRAM

    // filter part
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > flt_strms[CH_NUM][COL_NUM];
#pragma HLS stream variable = flt_strms depth = 32
#pragma HLS array_partition variable = flt_strms dim = 1
#pragma HLS resource variable = flt_strms core = FIFO_LUTRAM
    hls::stream<bool> e_flt_strms[CH_NUM];
#pragma HLS stream variable = e_flt_strms depth = 32

    // partition part
    hls::stream<ap_uint<16> > hp_bkpu_strm;
#pragma HLS stream variable = hp_bkpu_strm depth = 32
    hls::stream<ap_uint<10> > hp_nm_strm;
#pragma HLS stream variable = hp_nm_strm depth = 32
    hls::stream<ap_uint<8 * TPCH_INT_SZ> > hp_out_strms[COL_NUM];
#pragma HLS stream variable = hp_out_strms depth = 32

    load_config(buf_D, col_index, mk_on, cid_strm, wr_cfg_strm, fcfg);

    scan_to_channel<COL_NUM, CH_NUM>(bit_num, buf_A, cid_strm, ch_strms, e_ch_strms, bit_num_strm, bit_num_strm_copy);
#ifndef __SYNTHESIS__
    printf("***** after scan\n");
    for (int ch = 0; ch < CH_NUM; ++ch) {
        printf("ch:%d nrow=%ld\n", ch, e_ch_strms[ch].size());
    }
#endif

    filter_ongoing<COL_NUM, COL_NUM, CH_NUM>(fcfg, ch_strms, e_ch_strms, flt_strms, e_flt_strms);

#ifndef __SYNTHESIS__
    {
        printf("***** after Filter\n");
        size_t ss = 0;
        for (int ch = 0; ch < CH_NUM; ++ch) {
            size_t s = e_flt_strms[ch].size() - 1;
            printf("ch:%d nrow=%ld\n", ch, s);
            ss += s;
        }
        printf("total: nrow=%ld\n", ss);
    }
#endif

    hash_partition_wrapper<COL_NUM, CH_NUM, COL_NUM>(mk_on, k_depth, bit_num_strm, flt_strms, e_flt_strms, hp_bkpu_strm,
                                                     hp_nm_strm, hp_out_strms);

    writeTable<32, VEC_LEN, COL_NUM>(hp_out_strms, wr_cfg_strm, bit_num_strm_copy, hp_nm_strm, hp_bkpu_strm, buf_B);
}
