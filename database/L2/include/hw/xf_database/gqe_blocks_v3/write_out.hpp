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
#ifndef GQE_ISV_WRITE_OUT_HPP
#define GQE_ISV_WRITE_OUT_HPP

#include <ap_int.h>
#include <hls_stream.h>
#include "hls_burst_maxi.h"

#ifdef __SYNTHESIS__
#include "etc/autopilot_ssdm_op.h"
#endif

#include "xf_database/utils.hpp"
#include "xf_utils_hw/stream_shuffle.hpp"
#include "xf_database/gqe_blocks_v3/gqe_enums.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#define USER_DEBUG true
#define XDEBUG true
#endif

namespace xf {
namespace database {
namespace gqe {

template <int burst_len, int elem_size, int vec_len, int col_num, int hash_wh, int grp_sz>
void count_for_burst(hls::stream<ap_uint<8> >& i_general_cfg_strm,
                     hls::stream<ap_uint<elem_size * vec_len> > in_strm[col_num], // 512-bit in
                     hls::stream<ap_uint<10> >& i_nm_strm,
                     hls::stream<int>& i_partition_size_strm,
                     hls::stream<ap_uint<8> >& i_wr_cfg_strm,
                     hls::stream<int>& i_bit_num_strm,
                     hls::stream<ap_uint<10 + hash_wh> >& i_bkpu_strm,

                     hls::stream<ap_uint<8> > o_general_cfg_strm[col_num + 1],
                     hls::stream<ap_uint<elem_size * vec_len> > out_strm[col_num + 1], // 512-bit out
                     hls::stream<ap_uint<8> > o_nm_strm[col_num + 1],
                     hls::stream<ap_uint<8> > o_wr_cfg_strm[col_num + 1],
                     hls::stream<int> o_p_base_addr_strm[col_num + 1],
                     hls::stream<int> o_burst_step_cnt_strm[col_num + 1],
                     hls::stream<int>& o_per_part_nm_strm) {
    enum { PU = 1 << hash_wh, hash_tb_sz = grp_sz / elem_size };
#ifdef USER_DEBUG
    std::cout << "i_nm_strm.size(): " << i_nm_strm.size() << std::endl;
    for (int c = 0; c < col_num; c++) {
        std::cout << "in_strm[" << c << "].size(): " << in_strm[c].size() << std::endl;
    }
#endif
    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
        o_general_cfg_strm[c].write(general_cfg);
    }

    bool join_on = general_cfg[1];
    bool bf_on = general_cfg[2];
    bool part_on = general_cfg[3];
    bool build_probe_flag = general_cfg[5];

    // read out the partition info
    int PARTITION_SIZE = i_partition_size_strm.read();
    const int bit_num_org = i_bit_num_strm.read();

    ap_uint<hash_wh> pu_idx;
    ap_uint<10> bk_idx;
    int r = 0; // element count in one 512b
    int b = 0; // burst length count

    // input registers
    ap_uint<elem_size * vec_len> t[col_num];
#pragma HLS array_partition variable = t complete dim = 1
    // output registers
    ap_uint<elem_size * vec_len> vecs[col_num + 1];
#pragma HLS array_partition variable = vecs complete dim = 1
    int nrow_step_cnt[256];
//#pragma HLS array_partition variable = nrow_step_cnt complete dim = 1
#pragma HLS bind_storage variable = nrow_step_cnt type = ram_s2p impl = bram
    int nrow_cnt[256];
#pragma HLS bind_storage variable = nrow_cnt type = ram_s2p impl = bram

    ap_uint<8> write_out_cfg = i_wr_cfg_strm.read();
    for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
        o_wr_cfg_strm[c].write(write_out_cfg);
    }
#ifndef __SYNTHESIS__
    std::cout << "wr_en: " << write_out_cfg << std::endl;
#endif

    ap_uint<16> location;
    int partition_num, bit_num, part_num_per_PU;
    int p_base_addr, burst_step_cnt_reg;

    // read in number of elemnts
    ap_uint<10> nm = i_nm_strm.read();

    if (part_on) {
    LOOP_INIT_NROW_CNT:
        for (int i = 0; i < 256; i++) {
#pragma HLS PIPELINE II = 1
            nrow_step_cnt[i] = 0;
            nrow_cnt[i] = 0;
        }
        burst_step_cnt_reg = 0;
        // get the partition num
        partition_num = 1 << bit_num_org;
        // get the bit num after minus PU idx 2 bits
        bit_num = bit_num_org - hash_wh;
        // get the part num in each PU
        part_num_per_PU = 1 << bit_num;

        while (nm != 0) {
            (pu_idx, bk_idx) = i_bkpu_strm.read();

            // the partition idx among all PUs
            location = pu_idx * part_num_per_PU + bk_idx;
            p_base_addr = PARTITION_SIZE * location;
            int nrow_step_cnt_reg = nrow_step_cnt[location];

        // convert dat number from nm * 32bit to nm/16 * 512bit
        CACHE_BURST_LEN_LOOP:
            for (int i = 0; i < (nm + vec_len - 1) / vec_len; i++) {
#pragma HLS pipeline II = 1
                burst_step_cnt_reg += vec_len;
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    t[c] = in_strm[c].read();
                    vecs[c].range(elem_size * vec_len - 1, 0) = t[c];
                }

                for (int c = 0; c < col_num + 1; ++c) {
#pragma HLS unroll
                    if (write_out_cfg[c]) out_strm[c].write(vecs[c]);
                }
                if (b == burst_len - 1) {
                    {
#pragma HLS latency min = 1 max = 1
                        for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
                            if (write_out_cfg[c]) {
                                o_nm_strm[c].write(burst_len);
                                // get write ddr base addr
                                o_p_base_addr_strm[c].write(p_base_addr);
                                // get the offset after nm times of write
                                o_burst_step_cnt_strm[c].write((nrow_step_cnt_reg + vec_len - 1) / vec_len);
                            }
                        }
                    }
                    b = 0;

                    // update offset
                    nrow_step_cnt_reg += burst_step_cnt_reg;
                    burst_step_cnt_reg = 0;
                } else {
                    ++b;
                }
            }

            nrow_step_cnt[location] = nrow_step_cnt_reg;
            if (b != 0) {
                {
#pragma HLS latency min = 1 max = 1
                    for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
                        if (write_out_cfg[c]) {
                            o_nm_strm[c].write(b);
                            o_p_base_addr_strm[c].write(p_base_addr);
                            // get the offset after nm times of write
                            o_burst_step_cnt_strm[c].write((nrow_step_cnt_reg + vec_len - 1) / vec_len);
                        }
                    }
                }

                // update offset
                nrow_step_cnt[location] = nrow_step_cnt_reg + burst_step_cnt_reg;
                burst_step_cnt_reg = 0;
            }

            b = 0;

            nrow_cnt[location] += nm;

            nm = i_nm_strm.read();
        }

        // send end flag
        for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
            if (write_out_cfg[c]) {
                o_nm_strm[c].write(0);
                o_p_base_addr_strm[c].write(p_base_addr);
                o_burst_step_cnt_strm[c].write((nrow_step_cnt[location] + vec_len - 1) / vec_len);
            }
        }

        // output nrow of each partition
        for (int i = 0; i < partition_num; i++) {
#pragma HLS PIPELINE II = 1
            o_per_part_nm_strm.write(nrow_cnt[i]);
        }
    } else {
        if (build_probe_flag) {
            while (nm > 0) {
#pragma HLS pipeline II = 1
                // read in
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    t[c] = in_strm[c].read();
                }
                switch (general_cfg.range(4, 0)) {
                    case gqe::JoinOn:
                        // reshape for output join
                        for (int c = 0; c < col_num + 1; ++c) {
#pragma HLS unroll
                            vecs[c].range(elem_size * (vec_len - 1) - 1, 0) =
                                vecs[c].range(elem_size * vec_len - 1, elem_size);
                            // join uses column 0 to output result key & pld
                            vecs[c].range(elem_size * vec_len - 1, elem_size * (vec_len - 1)) =
                                t[0].range(elem_size * (c + 1) - 1, elem_size * c);
                        }
                        if (r == vec_len - 1) { // 256bit, write 1 data
                            for (int c = 0; c < col_num + 1; ++c) {
#pragma HLS unroll
                                if (write_out_cfg[c]) {
                                    out_strm[c].write(vecs[c]);
                                }
                            }
                            if (b == burst_len - 1) {
                                for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
                                    if (write_out_cfg[c]) {
                                        o_nm_strm[c].write(burst_len);
                                    }
                                }
                                b = 0;
                            } else {
                                ++b;
                            }
                            r = 0;
                        } else {
                            ++r;
                        }
                        nm = i_nm_strm.read();
                        break;
                    case gqe::BloomFilterOn:
                        // bloom filter uses column 0 to output validation bits
                        if (vec_len > hash_tb_sz) {
                            vecs[0].range(elem_size * hash_tb_sz * (vec_len / hash_tb_sz - 1) - 1, 0) =
                                vecs[0].range(elem_size * vec_len - 1, elem_size * hash_tb_sz);
                        }
                        vecs[0].range(elem_size * vec_len - 1, elem_size * (vec_len - hash_tb_sz)) =
                            t[0].range(elem_size * hash_tb_sz - 1, 0);
                        if (r == vec_len - hash_tb_sz) {
                            for (int c = 0; c < col_num + 1; ++c) {
#pragma HLS unroll
                                if (write_out_cfg[c]) {
                                    out_strm[c].write(vecs[c]);
                                }
                            }
                            if (b == burst_len - 1) {
                                for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
                                    if (write_out_cfg[c]) {
                                        o_nm_strm[c].write(burst_len);
                                    }
                                }
                                b = 0;
                            } else {
                                ++b;
                            }
                            r = 0;
                        } else {
                            r += hash_tb_sz;
                        }
                        nm = i_nm_strm.read();
                        break;
#ifndef __SYNTHESIS__
                    default:
                        std::cerr << "Error: illegal kernel switching combination\n";
                        exit(1);
#endif
                }
            }

            // algin to 512bit with 0
            if (r != 0) {
            LOOP_FOR:
                if (join_on) {
                    for (; r < vec_len; ++r) {
#pragma HLS pipeline II = 1
                        for (int c = 0; c < col_num + 1; ++c) {
#pragma HLS unroll
                            vecs[c].range(elem_size * (vec_len - 1) - 1, 0) =
                                vecs[c].range(elem_size * vec_len - 1, elem_size);
                            vecs[c].range(elem_size * vec_len - 1, elem_size * (vec_len - 1)) = 0;
                        }
                    }
                } else if (bf_on) {
                    for (; r < vec_len; r += hash_tb_sz) {
#pragma HLS pipeline II = 1
                        vecs[0].range(elem_size * hash_tb_sz * (vec_len / hash_tb_sz - 1) - 1, 0) =
                            vecs[0].range(elem_size * vec_len - 1, elem_size * hash_tb_sz);
                        vecs[0].range(elem_size * vec_len - 1, elem_size * (vec_len - hash_tb_sz)) = 0;
                    }
                }
                for (int c = 0; c < col_num + 1; ++c) {
#pragma HLS unroll
                    if (write_out_cfg[c]) {
                        out_strm[c].write(vecs[c]);
                    }
                }
                ++b;
            }

            // last burst
            if (b != 0) {
                XF_DATABASE_ASSERT(b <= burst_len);
                for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
                    if (write_out_cfg[c]) {
                        o_nm_strm[c].write(b);
                    }
                }
            }
            // send end flag
            for (int c = 0; c < col_num + 1; c++) {
#pragma HLS unroll
                if (write_out_cfg[c]) {
                    o_nm_strm[c].write(0);
                }
            }
#ifndef __SYNTHESIS__
            std::cout << "o_nm_strm.size(): " << o_nm_strm[0].size() << std::endl;
#endif

        } // build_probe_flag
    }
}

// burst write for 1 col
template <int elem_size, int vec_len, int burst_len>
void burst_write(hls::stream<ap_uint<8> >& general_cfg_strm,
                 hls::stream<ap_uint<elem_size * vec_len> >& i_strm,
                 hls::stream<ap_uint<8> >& i_nm_strm,
                 hls::stream<ap_uint<8> >& i_wr_cfg_strm,
                 hls::stream<int>& i_part_base_addr_strm,
                 hls::stream<int>& i_burst_step_cnt_strm,
                 hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr,
                 const int idx) {
    ap_uint<8> general_cfg = general_cfg_strm.read();
    bool bp_flag = general_cfg[5];
    bool part_on = general_cfg[3];
    ap_uint<8> wr_en = i_wr_cfg_strm.read();

    int part_base_addr = 0;
    int burst_step_cnt = 0;
    int offset_addr = 0;

    const int CHECK_LATENCY = 64;
    ap_uint<CHECK_LATENCY> check = 0;
    bool end = false;
    ap_uint<8> nm = 0;
    int bnm = 0;
    ap_uint<elem_size * vec_len> out;

    if (wr_en[idx]) {
        if (bp_flag) {
            while (!end) {
#pragma HLS pipeline II = 1
                bool check_l = check[CHECK_LATENCY - 1];
                if (nm == 0) {
                    if (part_on) {
                        bool rd_e =
                            !i_nm_strm.empty() && !i_part_base_addr_strm.empty() && !i_burst_step_cnt_strm.empty();
                        if (rd_e) {
                            {
#pragma HLS latency min = 1 max = 1
                                i_nm_strm.read_nb(nm);
                                i_part_base_addr_strm.read_nb(part_base_addr);
                                i_burst_step_cnt_strm.read_nb(burst_step_cnt);
                            }
                            end = (nm == 0);
                            offset_addr = part_base_addr + burst_step_cnt;
                            if (nm) {
                                ptr.write_request(offset_addr, nm);
                            }
                        }
                    } else {
                        if (i_nm_strm.read_nb(nm)) {
                            offset_addr = bnm * burst_len;
                            bnm++;
                            end = (nm == 0);
                            if (nm) {
                                ptr.write_request(offset_addr, nm);
                            }
                        }
                    }
                } else {
                    if (i_strm.read_nb(out)) {
                        check[0] = (nm == 1);
                        nm--;
                        ptr.write(out);
                    }
                }
                check = (check << 1);
                if (check_l) {
                    ptr.write_response();
#ifdef __SYNTHESIS__
                    ap_wait();
#endif
                }
            }
        }
    }
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void burst_write_wrapper(hls::stream<ap_uint<8> > general_cfg_strm[col_num + 1],
                         hls::stream<ap_uint<elem_size * vec_len> > in_strm[col_num + 1], // 256-bit out
                         hls::stream<ap_uint<8> > i_nm_strm[col_num + 1],
                         hls::stream<ap_uint<8> > i_wr_cfg_strm[col_num + 1],
                         hls::stream<int> i_part_base_addr_strm[col_num + 1],
                         hls::stream<int> i_burst_step_cnt_strm[col_num + 1],
                         hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr0,
                         hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr1,
                         hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr2,
                         hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr3) {
#pragma HLS dataflow
    burst_write<elem_size, vec_len, burst_len>(general_cfg_strm[0], in_strm[0], i_nm_strm[0], i_wr_cfg_strm[0],
                                               i_part_base_addr_strm[0], i_burst_step_cnt_strm[0], ptr0, 0);
    burst_write<elem_size, vec_len, burst_len>(general_cfg_strm[1], in_strm[1], i_nm_strm[1], i_wr_cfg_strm[1],
                                               i_part_base_addr_strm[1], i_burst_step_cnt_strm[1], ptr1, 1);
    burst_write<elem_size, vec_len, burst_len>(general_cfg_strm[2], in_strm[2], i_nm_strm[2], i_wr_cfg_strm[2],
                                               i_part_base_addr_strm[2], i_burst_step_cnt_strm[2], ptr2, 2);
    burst_write<elem_size, vec_len, burst_len>(general_cfg_strm[3], in_strm[3], i_nm_strm[3], i_wr_cfg_strm[3],
                                               i_part_base_addr_strm[3], i_burst_step_cnt_strm[3], ptr3, 3);
}

template <int burst_len, int elem_size, int vec_len, int col_num, int hash_wh, int grp_sz>
void write_table_out_core(hls::stream<ap_uint<8> >& general_cfg_strm,
                          hls::stream<ap_uint<elem_size * vec_len> > in_strm[col_num],
                          hls::stream<ap_uint<10> >& i_nm_strm,
                          hls::stream<int>& i_partition_size_strm,
                          hls::stream<int>& i_bit_num_strm,
                          hls::stream<ap_uint<8> >& i_wr_cfg_strm,
                          hls::stream<ap_uint<10 + hash_wh> >& i_bkpu_strm,

                          hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr0,
                          hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr1,
                          hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr2,
                          hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr3,
                          hls::stream<int>& o_per_part_nm_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<elem_size * vec_len> > mid_vecs_strm[col_num + 1];
#pragma HLS stream variable = mid_vecs_strm depth = 64
#pragma HLS bind_storage variable = mid_vecs_strm type = fifo impl = lutram
    hls::stream<ap_uint<8> > mid_wr_cfg_strm[col_num + 1];
#pragma HLS stream variable = mid_wr_cfg_strm depth = 2
    hls::stream<ap_uint<8> > mid_nm_strm[col_num + 1];
#pragma HLS stream variable = mid_nm_strm depth = 4
    hls::stream<ap_uint<8> > mid_general_cfg_strm[col_num + 1];
#pragma HLS stream variable = mid_general_cfg_strm depth = 2
    hls::stream<int> mid_part_base_addr_strm[col_num + 1];
#pragma HLS stream variable = mid_part_base_addr_strm depth = 4
    hls::stream<int> mid_burst_step_cnt_strm[col_num + 1];
#pragma HLS stream variable = mid_burst_step_cnt_strm depth = 4

    count_for_burst<burst_len, elem_size, vec_len, col_num, hash_wh, grp_sz>(
        general_cfg_strm, in_strm, i_nm_strm, i_partition_size_strm, i_wr_cfg_strm, i_bit_num_strm, i_bkpu_strm,

        mid_general_cfg_strm, mid_vecs_strm, mid_nm_strm, mid_wr_cfg_strm, mid_part_base_addr_strm,
        mid_burst_step_cnt_strm, o_per_part_nm_strm);
#ifdef USER_DEBUG
    std::cout << "mid_nm_strm.size() = " << mid_nm_strm[0].size() << std::endl;
    for (int c = 0; c < col_num + 1; c++) {
        std::cout << "mid_vecs_strm[" << c << "].size() = " << mid_vecs_strm[c].size() << std::endl;
    }
#endif
    burst_write_wrapper<burst_len, elem_size, vec_len, col_num>(mid_general_cfg_strm, mid_vecs_strm, mid_nm_strm,
                                                                mid_wr_cfg_strm, mid_part_base_addr_strm,
                                                                mid_burst_step_cnt_strm, ptr0, ptr1, ptr2, ptr3);
}

// gqeJoin used write data out
template <int burst_len, int elem_size, int vec_len, int col_num, int hash_wh, int grp_sz>
void write_table_out(hls::stream<ap_uint<8> >& general_cfg_strm,
                     hls::stream<ap_uint<8> >& write_out_cfg_strm,
                     hls::stream<ap_uint<elem_size * vec_len> > in_strm[col_num],
                     hls::stream<ap_uint<10> >& i_nm_strm,
                     hls::stream<ap_uint<10 + hash_wh> >& i_bkpu_strm,
                     hls::stream<int>& i_bit_num_strm,
                     hls::stream<ap_uint<32> >& nr_row_strm,

                     hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr0,
                     hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr1,
                     hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr2,
                     hls::burst_maxi<ap_uint<elem_size * vec_len> >& ptr3,
                     hls::burst_maxi<ap_uint<elem_size * vec_len> >& dout_meta) {
    hls::stream<ap_uint<8> > mid_general_cfg_strm;
#pragma HLS stream variable = mid_general_cfg_strm depth = 2
    hls::stream<int> mid_partition_size_strm;
#pragma HLS stream variable = mid_partition_size_strm depth = 2
    hls::stream<int> mid_bit_num_strm;
#pragma HLS stream variable = mid_bit_num_strm depth = 2
    hls::stream<int> mid_per_part_nm_strm;
#pragma HLS stream variable = mid_per_part_nm_strm depth = 256
#pragma HLS bind_storage variable = mid_per_part_nm_strm type = fifo impl = bram

    ap_uint<8> general_cfg = general_cfg_strm.read();
    mid_general_cfg_strm.write(general_cfg);
    bool part_on = general_cfg[3];
    bool bp_flag = general_cfg[5];
    // read out the partition size
    dout_meta.read_request(0, 1);
    ap_uint<256> om0 = dout_meta.read();
    int PARTITION_SIZE = om0.range(135, 104);
    // read in the bit num
    const int bit_num = i_bit_num_strm.read();
    // get the partition num
    const int partition_num = 1 << bit_num;

    mid_partition_size_strm.write(PARTITION_SIZE);
    mid_bit_num_strm.write(bit_num);

    // write-out core
    write_table_out_core<burst_len, elem_size, vec_len, col_num, hash_wh, grp_sz>(
        mid_general_cfg_strm, in_strm, i_nm_strm, mid_partition_size_strm, mid_bit_num_strm, write_out_cfg_strm,
        i_bkpu_strm, ptr0, ptr1, ptr2, ptr3, mid_per_part_nm_strm);

    // write nrow out for JOIN
    if (bp_flag) {
        ap_uint<32> rnm = nr_row_strm.read();
        om0.range(71, 8) = rnm;
#ifdef USER_DEBUG
        std::cout << "total hj result row number: " << rnm << std::endl;
#endif
    }
    dout_meta.write_request(0, 1);
    dout_meta.write(om0);
    dout_meta.write_response();

//----------update meta------------
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    int cnt = 0;
#endif
    // define the buffer used to save nrow of each partition
    ap_uint<elem_size * vec_len> metaout[32];
#pragma HLS bind_storage variable = metaout type = ram_s2p impl = lutram
    if (part_on) {
    // update tout meta
    FINAL_WRITE_HEAD_LOOP:
        for (int i = 0; i < partition_num; i++) {
#pragma HLS PIPELINE II = 1
            int rnm = mid_per_part_nm_strm.read();
            ap_uint<8> idx = i / 8;
            ap_uint<8> idx_ex = i % 8;
            metaout[idx].range(32 * (idx_ex + 1) - 1, 32 * idx_ex) = rnm;

#if !defined(__SYNTHESIS__) && XDEBUG == 1
            std::cout << "P" << i << "\twrite out row number = " << rnm << std::endl;
            cnt += rnm;
#endif
        }
        // write nrow of each partition to dout_meta, ddr
        dout_meta.write_request(16, ((partition_num + 7) / 8));
        for (int i = 0; i < (partition_num + 7) / 8; i++) {
#pragma HLS PIPELINE II = 1
            dout_meta.write(metaout[i]); /* [16 + i] */
        }
        dout_meta.write_response();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
        std::cout << "Total number of write-out row: " << cnt << std::endl;
#endif
    }
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif // GQE_ISV_WRITE_OUT_HPP
