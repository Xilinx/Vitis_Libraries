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
#ifndef GQE_WRITE_TABLE_HPP
#define GQE_WRITE_TABLE_HPP

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#include <ap_int.h>
#include <hls_stream.h>
#include "gqe_part.hpp"

namespace xf {
namespace database {
namespace gqe {

template <int elem_size, int vec_len, int col_num>
void countForBurst(hls::stream<ap_uint<elem_size> > i_post_Agg[col_num],
                   hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                   hls::stream<int>& bit_num_strm,
                   hls::stream<ap_uint<10> >& i_nm_strm,
                   hls::stream<ap_uint<16> >& i_bkpu_num_strm,

                   hls::stream<ap_uint<elem_size * vec_len> > o_post_Agg[col_num],
                   hls::stream<ap_uint<16> >& o_loc_strm,
                   hls::stream<ap_uint<8> >& o_nm_strm,
                   hls::stream<int>& rnm_strm,
                   hls::stream<ap_uint<32> >& o_wr_cfg_strm,
                   hls::stream<int>& o_bit_num_strm) {
    const int sz = elem_size;
    ap_uint<6> pu_idx;
    ap_uint<10> bk_idx;
    int n = 0; // nrow count
    int r = 0; // element count in one 512b
    int b = 0; // burst lentg count
    ap_uint<sz * vec_len> vecs[col_num];
#pragma HLS array_partition variable = vecs complete
    int nrow_cnt[512];
#pragma HLS resource variable = nrow_cnt core = RAM_S2P_BRAM
    for (int i = 0; i < 512; i++) {
#pragma HLS PIPELINE II = 1
        nrow_cnt[i] = 0;
    }

    ap_uint<32> write_out_cfg = i_wr_cfg_strm.read();
    o_wr_cfg_strm.write(write_out_cfg);
    const int bit_num = bit_num_strm.read();
    o_bit_num_strm.write(bit_num);
    const int BK = 1 << bit_num;

    ap_uint<10> nm = i_nm_strm.read();
    while (nm != 0) {
        (pu_idx, bk_idx) = i_bkpu_num_strm.read();
        ap_uint<16> location = pu_idx * BK + bk_idx;
        o_loc_strm.write(location);
        nrow_cnt[location] += nm;
        const ap_uint<8> burst_len = (nm + vec_len - 1) / vec_len;
        for (int i = 0; i < nm; i++) {
#pragma HLS pipeline II = 1
            for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                ap_uint<sz> t = i_post_Agg[c].read();
                vecs[c].range(sz * (vec_len - 1) - 1, 0) = vecs[c].range(sz * vec_len - 1, sz);
                vecs[c].range(sz * vec_len - 1, sz * (vec_len - 1)) = t;
            }

            if (r == vec_len - 1) {
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    if (write_out_cfg[c]) o_post_Agg[c].write(vecs[c]);
                }
                if (b == burst_len - 1) {
                    o_nm_strm.write(burst_len);
                    b = 0;
                } else {
                    ++b;
                }
                r = 0;
            } else {
                ++r;
            }
        }

        if (r != 0) {
            // handle incomplete vecs
            for (; r < vec_len; ++r) {
#pragma HLS unroll
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    vecs[c].range(sz * (vec_len - 1) - 1, 0) = vecs[c].range(sz * vec_len - 1, sz);
                    vecs[c].range(sz * vec_len - 1, sz * (vec_len - 1)) = 0;
                }
            }
            for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                if (write_out_cfg[c]) o_post_Agg[c].write(vecs[c]);
            }
            ++b;
        }

        if (b != 0) o_nm_strm.write(b);

        b = 0;
        r = 0;
        nm = i_nm_strm.read();
    }

    o_nm_strm.write(0);
    for (int i = 0; i < BK; i++) {
#pragma HLS PIPELINE II = 1
        rnm_strm.write(nrow_cnt[i]);
    }
}

template <int elem_size, int vec_len, int col_num>
void burstWrite(hls::stream<ap_uint<elem_size * vec_len> > i_strm[col_num],
                hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                hls::stream<int>& bit_num_strm,
                ap_uint<elem_size * vec_len>* ptr,
                hls::stream<ap_uint<16> >& i_loc_strm,
                hls::stream<ap_uint<8> >& nm_strm,
                hls::stream<int>& rnm_strm) {
    // read out the block size
    ap_uint<elem_size* vec_len> first_r = ptr[0];
    int BLOCK_SIZE = first_r(elem_size * 2 - 1, elem_size).to_int();
    int PARTITION_SIZE = first_r(elem_size * 3 - 1, elem_size * 2).to_int();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << "Partition size:" << PARTITION_SIZE << std::endl;
    std::cout << "In one partition, block size for every col is: " << BLOCK_SIZE << std::endl;
    long long cnt = 0;
#endif

    ap_uint<32> write_out_cfg = i_wr_cfg_strm.read();
    const int bit_num = bit_num_strm.read();
    const int BK = 1 << bit_num;

    ap_uint<32> burst_step_cnt[512];
#pragma HLS resource variable = burst_step_cnt core = RAM_S2P_BRAM
    for (int i = 0; i < 512; i++) {
#pragma HLS PIPELINE II = 1
        burst_step_cnt[i] = 0;
    }

    int col_id[col_num];
    int col_offset[col_num];
#pragma HLS array_partition variable = col_offset complete
    int wcol = 0;
    for (int i = 0; i < col_num; ++i) {
#pragma HLS unroll
        if (write_out_cfg[i]) {
            col_id[wcol] = i;
            col_offset[wcol++] = BLOCK_SIZE * i + 1;
        }
    }
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    for (int k = 0; k < wcol; ++k) {
        std::cout << "col_id=" << col_id[k] << ", col_offset=" << col_offset[k] << std::endl;
    }
#endif

    ap_uint<8> nm = nm_strm.read();
    while (nm != 0) {
        // write each col one burst
        ap_uint<16> loc_t = i_loc_strm.read();
        const int p_base_addr = PARTITION_SIZE * loc_t;
        ap_uint<32> burst_step_cnt_reg = burst_step_cnt[loc_t];
        for (int k = 0; k < wcol; ++k) {
            const int id = col_id[k];
            const int offset = col_offset[k];
        BURST_WRITE_CORE_LOOP:
            for (int n = 0; n < nm; ++n) {
#pragma HLS pipeline II = 1
                ap_uint<elem_size* vec_len> out = i_strm[id].read();
                ptr[p_base_addr + offset + burst_step_cnt_reg + n] = out;
            }
        }

        burst_step_cnt_reg += nm;
        burst_step_cnt[loc_t] = burst_step_cnt_reg;
        nm = nm_strm.read();
    }

FINAL_WRITE_HEAD_LOOP:
    for (int i = 0; i < BK; i++) {
        const int base_addr = PARTITION_SIZE * i;
        int rnm = rnm_strm.read();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
        std::cout << "P" << i << "\twrite out row number = " << rnm << std::endl;
        cnt += rnm;
#endif
        first_r(elem_size - 1, 0) = rnm;
        ptr[base_addr] = first_r;
    }
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << "Total number of write-out row: " << cnt << std::endl;
#endif
}

template <int elem_size, int vec_len, int col_num>
void writeTable(hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                hls::stream<int>& bit_num_strm,
                hls::stream<ap_uint<10> >& i_nm_strm,
                hls::stream<ap_uint<16> >& i_bkpu_num_strm,

                ap_uint<elem_size * vec_len>* ptr) {
    const int burst_len = BURST_LEN;
    const int k_fifo_buf = burst_len * 2;
    hls::stream<ap_uint<elem_size * vec_len> > counter_post_Agg[col_num];
#pragma HLS stream variable = counter_post_Agg depth = k_fifo_buf
#pragma HLS resource variable = counter_post_Agg core = FIFO_LUTRAM

    hls::stream<ap_uint<16> > counter_loc_strm;
#pragma HLS stream variable = counter_loc_strm depth = 8
    hls::stream<ap_uint<8> > counter_nm_strm;
#pragma HLS stream variable = counter_nm_strm depth = 8
    hls::stream<int> counter_rnm_strm;
#pragma HLS stream variable = counter_rnm_strm depth = 8
    hls::stream<ap_uint<32> > mid_write_cfg_strm;
#pragma HLS stream variable = mid_write_cfg_strm depth = 4
    hls::stream<int> mid_bit_num_strm;
#pragma HLS stream variable = mid_bit_num_strm depth = 4

#pragma HLS dataflow

    countForBurst<elem_size, vec_len, col_num>(post_Agg, i_wr_cfg_strm, bit_num_strm, i_nm_strm, i_bkpu_num_strm,
                                               counter_post_Agg, counter_loc_strm, counter_nm_strm, counter_rnm_strm,
                                               mid_write_cfg_strm, mid_bit_num_strm);

    burstWrite<elem_size, vec_len, col_num>(counter_post_Agg, mid_write_cfg_strm, mid_bit_num_strm, ptr,
                                            counter_loc_strm, counter_nm_strm, counter_rnm_strm);
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif
