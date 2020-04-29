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
#ifndef GQE_LOAD_CONFIG_HPP
#define GQE_LOAD_CONFIG_HPP

#ifndef __SYNTHESIS__
#include <stdio.h>
#include <iostream>
#endif

#include <ap_int.h>
#include <hls_stream.h>

#include "gqe_blocks/gqe_types.hpp"

namespace xf {
namespace database {
namespace gqe {

template <int width = 64>
ap_uint<width> ext_64(ap_uint<512> data, int bias) {
#pragma HLS inline
    ap_uint<width> result = data(64 * bias + 63, 64 * bias);
    return result;
}

template <int width = 8>
ap_uint<width> ext_8(ap_uint<64> data, int bias) {
#pragma HLS inline
    ap_uint<width> result = data(8 * bias + 7, 8 * bias);
    return result;
}

// for gqe join
template <int JN_NM>
void load_config(ap_uint<8 * TPCH_INT_SZ * VEC_LEN> ptr[9],
                 hls::stream<bool> join_on_strm[JN_NM],
                 hls::stream<bool>& join_dual_key_on_strm,
                 hls::stream<bool>& agg_on_strm,
                 hls::stream<ap_uint<3> >& join_flag_strm,
                 hls::stream<int8_t>& col_id_A_strm,
                 hls::stream<int8_t>& col_id_B_strm,
                 hls::stream<ap_uint<32> >& write_out_cfg_strm,
                 hls::stream<ap_uint<289> >& alu1_cfg_strm,
                 hls::stream<ap_uint<289> >& alu2_cfg_strm,
                 hls::stream<ap_uint<32> >& filter_cfg_strm,
                 hls::stream<ap_uint<8 * 8> > shuffle1_cfg_strm[4],
                 hls::stream<ap_uint<8 * 8> >& shuffle2_cfg_strm,
                 hls::stream<ap_uint<8 * 8> >& shuffle3_cfg_strm,
                 hls::stream<ap_uint<8 * 8> >& shuffle4_cfg_strm) {
    const int filter_cfg_depth = 45;

    ap_uint<8 * TPCH_INT_SZ * VEC_LEN> config[9];
#pragma HLS resource variable = config core = RAM_1P_LUTRAM

    bool join_on;
    bool join_dual_key_on;
    bool agg_on;
    ap_uint<3> join_flag;
    int8_t col_id_A[8];
    int8_t col_id_B[8];
    ap_uint<32> write_out_cfg;
    ap_uint<289> alu_cfg1;
    ap_uint<289> alu_cfg2;
    ap_uint<32> filter_cfg_a[filter_cfg_depth];
#pragma HLS resource variable = filter_cfg_a core = RAM_1P_LUTRAM
    ap_uint<32> filter_cfg_b[filter_cfg_depth];
#pragma HLS resource variable = filter_cfg_b core = RAM_1P_LUTRAM

    for (int i = 0; i < 9; i++) {
        config[i] = ptr[i];
    }

    if (config[0][0] == 0) {
        join_on = false;
    } else {
        join_on = true;
    }

    if (config[0][1] == 0) {
        agg_on = false;
    } else {
        agg_on = true;
    }

    if (config[0][2] == 0) {
        join_dual_key_on = false;
    } else {
        join_dual_key_on = true;
    }

    join_flag = config[0].range(5, 3);

    for (int i = 0; i < 8; i++) {
        col_id_A[i] = config[0].range(56 + 8 * i + 7, 56 + 8 * i);
    }
    for (int i = 0; i < 8; i++) {
        col_id_B[i] = config[0].range(120 + 8 * i + 7, 120 + 8 * i);
    }

    write_out_cfg.range(7, 0) = config[0].range(191, 184);
    write_out_cfg.set_bit(31, config[0].get_bit(6));

    ap_uint<64> shuffle_cfg1a = config[0].range(255, 192);
    ap_uint<64> shuffle_cfg1b = config[0].range(319, 256);
    ap_uint<64> shuffle_cfg2 = config[0].range(383, 320);
    ap_uint<64> shuffle_cfg3 = config[0].range(447, 384);
    ap_uint<64> shuffle_cfg4 = config[0].range(511, 448);

    alu_cfg1.range(288, 0) = config[1].range(288, 0);
    alu_cfg2.range(288, 0) = config[2].range(288, 0);

    for (int i = 0; i < filter_cfg_depth; i++) {
        filter_cfg_a[i] = config[3 + i / 16].range(32 * ((i % 16) + 1) - 1, 32 * (i % 16));
    }

    for (int i = 0; i < filter_cfg_depth; i++) {
        filter_cfg_b[i] = config[6 + i / 16].range(32 * ((i % 16) + 1) - 1, 32 * (i % 16));
    }
    /*
        ap_uint<64> shuffle_cfg1=config[8].range(63,0);

        ap_uint<8*8> shuffle_cfg2=config[8].range(127,64);

        ap_uint<8*8> shuffle_cfg3=config[8].range(191,128);*/
    shuffle3_cfg_strm.write(shuffle_cfg3);
    /*
        ap_uint<8*8> shuffle_cfg4=config[8].range(256,192);*/
    shuffle4_cfg_strm.write(shuffle_cfg4);

    for (int i = 0; i < JN_NM; ++i) {
#pragma HLS unroll
        join_on_strm[i].write(join_on);
    }
    join_dual_key_on_strm.write(join_dual_key_on);
    agg_on_strm.write(agg_on);

    alu1_cfg_strm.write(alu_cfg1);
    alu2_cfg_strm.write(alu_cfg2);
    write_out_cfg_strm.write(write_out_cfg);

    if (join_on) {
        shuffle1_cfg_strm[0].write(shuffle_cfg1a);
        shuffle1_cfg_strm[1].write(shuffle_cfg1a);
        shuffle1_cfg_strm[2].write(shuffle_cfg1a);
        shuffle1_cfg_strm[3].write(shuffle_cfg1a);

        shuffle1_cfg_strm[0].write(shuffle_cfg1b);
        shuffle1_cfg_strm[1].write(shuffle_cfg1b);
        shuffle1_cfg_strm[2].write(shuffle_cfg1b);
        shuffle1_cfg_strm[3].write(shuffle_cfg1b);

        shuffle2_cfg_strm.write(shuffle_cfg2);

        for (int i = 0; i < 8; i++) {
            col_id_A_strm.write(col_id_A[i]);
        }
        // for(int i = 0; i < 5; i++) {
        //    col_id_A_strm.write(col_id_A[i]);
        //}
        for (int i = 0; i < 8; i++) {
            col_id_B_strm.write(col_id_B[i]);
        }

        for (int i = 0; i < filter_cfg_depth; i++) {
            filter_cfg_strm.write(filter_cfg_a[i]);
        }
        // for(int i = 0; i < filter_cfg_depth; i++) {
        //    filter_cfg_strm.write(filter_cfg_a[i]);
        //}
        for (int i = 0; i < filter_cfg_depth; i++) {
            filter_cfg_strm.write(filter_cfg_b[i]);
        }

        join_flag_strm.write(join_flag);

    } else {
        shuffle1_cfg_strm[0].write(shuffle_cfg1a);
        shuffle1_cfg_strm[1].write(shuffle_cfg1a);
        shuffle1_cfg_strm[2].write(shuffle_cfg1a);
        shuffle1_cfg_strm[3].write(shuffle_cfg1a);

        for (int i = 0; i < 8; i++) {
            col_id_A_strm.write(col_id_A[i]);
        }

        for (int i = 0; i < filter_cfg_depth; i++) {
            filter_cfg_strm.write(filter_cfg_a[i]);
        }
    }
}

// for gqe aggregate
template <int CHNM, int ColNM>
void load_config(ap_uint<8 * TPCH_INT_SZ>* ptr,
                 hls::stream<int8_t>& col_id_strm,
                 hls::stream<ap_uint<32> >& alu1_cfg_strm,
                 hls::stream<ap_uint<32> >& alu2_cfg_strm,
                 hls::stream<ap_uint<32> >& filter_cfg_strm,
                 hls::stream<ap_uint<ColNM * ColNM> > shuffle1_cfg_strm[CHNM],
                 hls::stream<ap_uint<ColNM * ColNM> > shuffle2_cfg_strm[CHNM],
                 hls::stream<ap_uint<ColNM * ColNM> > shuffle3_cfg_strm[CHNM],
                 hls::stream<ap_uint<ColNM * ColNM> > shuffle4_cfg_strm[CHNM],
                 hls::stream<ap_uint<32> >& merge_column_cfg_strm,
                 hls::stream<ap_uint<32> >& group_aggr_cfg_strm,
                 hls::stream<bool>& direct_aggr_cfg_strm,
                 hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    ap_uint<8 * TPCH_INT_SZ> config[128];
#pragma HLS resource variable = config core = RAM_1P_BRAM

    int8_t col_id[8];
    ap_uint<32> alu1_cfg[10];
    ap_uint<32> alu2_cfg[10];
    ap_uint<32> filter_cfg[45];
#pragma HLS resource variable = filter_cfg core = RAM_1P_LUTRAM
    ap_uint<ColNM * ColNM> shuffle1_cfg;
    ap_uint<ColNM * ColNM> shuffle2_cfg;
    ap_uint<ColNM * ColNM> shuffle3_cfg;
    ap_uint<ColNM * ColNM> shuffle4_cfg;
    ap_uint<32> merge_column_cfg[2];
    ap_uint<32> group_aggr_cfg[4];
    bool direct_aggr_cfg;
    ap_uint<32> write_out_cfg;

    // store
    for (int i = 0; i < 128; i++) {
        config[i] = ptr[i];

#if !defined __SYNTHESIS__ && XDEBUG == 1
        std::cout << std::dec << "config: id=" << i << std::hex << " value=" << config[i] << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1
    }

    // scan
    for (int i = 0; i < 4; i++) {
        col_id[i] = config[0].range(8 * (i + 1) - 1, 8 * i);
        col_id[i + 4] = config[1].range(8 * (i + 1) - 1, 8 * i);
    }

    // alu
    alu1_cfg[0] = config[11];
    alu1_cfg[1] = config[10];
    alu1_cfg[2] = config[9];
    alu1_cfg[3] = config[8];
    alu1_cfg[4] = config[7];
    alu1_cfg[5] = config[6];
    alu1_cfg[6] = config[5];
    alu1_cfg[7] = config[4];
    alu1_cfg[8] = config[3];
    alu1_cfg[9] = config[2];

    alu2_cfg[0] = config[21];
    alu2_cfg[1] = config[20];
    alu2_cfg[2] = config[19];
    alu2_cfg[3] = config[18];
    alu2_cfg[4] = config[17];
    alu2_cfg[5] = config[16];
    alu2_cfg[6] = config[15];
    alu2_cfg[7] = config[14];
    alu2_cfg[8] = config[13];
    alu2_cfg[9] = config[12];

    // filter
    for (int i = 0; i < 45; i++) {
        filter_cfg[i] = config[22 + i];
    }

    // shuffle
    shuffle1_cfg(31, 0) = config[67];
    shuffle1_cfg(63, 32) = config[68];

    shuffle2_cfg(31, 0) = config[69];
    shuffle2_cfg(63, 32) = config[70];

    shuffle3_cfg(31, 0) = config[71];
    shuffle3_cfg(63, 32) = config[72];

    shuffle4_cfg(31, 0) = config[73];
    shuffle4_cfg(63, 32) = config[74];

    // group aggr
    for (int i = 0; i < 4; i++) {
        group_aggr_cfg[i] = config[i + 75];
    }

    // merge column
    merge_column_cfg[0] = config[79];
    merge_column_cfg[1] = config[80];

    // direct aggr
    direct_aggr_cfg = config[81][0] == 1 ? true : false;

    // write out
    write_out_cfg = config[82];

#if !defined __SYNTHESIS__ && XDEBUG == 1
    std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
#endif // !defined __SYNTHESIS__ && XDEBUG == 1

    // output
    for (int i = 0; i < 8; i++) {
        col_id_strm.write(col_id[i]);
    }

    for (int i = 0; i < 4; i++) {
        shuffle1_cfg_strm[i].write(shuffle1_cfg);
        shuffle2_cfg_strm[i].write(shuffle2_cfg);
        shuffle3_cfg_strm[i].write(shuffle3_cfg);
        shuffle4_cfg_strm[i].write(shuffle4_cfg);

        group_aggr_cfg_strm.write(group_aggr_cfg[i]);
    }

    for (int i = 0; i < 10; i++) {
        alu1_cfg_strm.write(alu1_cfg[i]);
        alu2_cfg_strm.write(alu2_cfg[i]);
    }

    for (int i = 0; i < 45; i++) {
        filter_cfg_strm.write(filter_cfg[i]);
    }

    merge_column_cfg_strm.write(merge_column_cfg[0]);
    merge_column_cfg_strm.write(merge_column_cfg[1]);
    direct_aggr_cfg_strm.write(direct_aggr_cfg);
    write_out_cfg_strm.write(write_out_cfg);
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif
