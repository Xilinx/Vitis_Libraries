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

#ifndef GQE_ISV_BUILD_FLOW_HPP
#define GQE_ISV_BUILD_FLOW_HPP

#include <hls_burst_maxi.h>

#include "xf_database/hash_multi_join_build_probe.hpp"
#include "xf_database/gqe_blocks_v3/uram_cache.hpp"

namespace xf {
namespace database {
namespace details {

// ************************************ Build ****************************************

/// @brief Truncates hash value for join build
template <int HASHW, int HASHWJ>
void trunc_hash(hls::stream<ap_uint<HASHW> >& i_hash_strm,
                hls::stream<bool>& i_e_strm,
                hls::stream<ap_uint<HASHWJ> >& o_hash_strm,
                hls::stream<bool>& o_e_strm) {
    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS PIPELINE II = 1
        ap_uint<HASHW> hash = i_hash_strm.read();
        last = i_e_strm.read();
        o_hash_strm.write(hash.range(HASHWJ - 1, 0));
        o_e_strm.write(false);
    }
    o_e_strm.write(true);
}

/// @brief Scan small table to count hash collision, build small table to its
/// hash addr
template <int HASHW, int KEYW, int PW, int S_PW, int ARW, int NUM_X, int NUM_Y>
void build_unit(ap_uint<32>& depth,
                ap_uint<32>& overflow_length,

                hls::stream<ap_uint<HASHW> >& i_hash_strm,
                hls::stream<ap_uint<KEYW> >& i_key_strm,
                hls::stream<ap_uint<PW> >& i_pld_strm,
                hls::stream<bool>& i_e_strm,

                hls::stream<ap_uint<ARW> >& o_addr_strm,
                hls::stream<ap_uint<KEYW + S_PW> >& o_row_strm,
                hls::stream<bool>& o_e_strm,

                ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
                ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

    const int HASH_DEPTH = (1 << HASHW) / 3 + 1;
    const int HASH_NUMBER = 1 << HASHW;
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst0(uram_buffer0);
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst1(uram_buffer1);

#ifndef __SYNTHESIS__
    unsigned int cnt = 0;
    unsigned int max_col = 0;
#endif

    ap_uint<72> elem = 0;
    ap_uint<72> base_elem = 0;
    ap_uint<72> overflow_elem = 0;

    ap_uint<32> overflow_length_temp = 0;
    ap_uint<ARW> overflow_base = HASH_NUMBER * depth;

    bool last = i_e_strm.read();
PRE_BUILD_LOOP:
    while (!last) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst0.blocks inter false
#pragma HLS dependence variable = uram_inst1.blocks inter false

        ap_uint<HASHW> hash_val = i_hash_strm.read();
        ap_uint<KEYW> key = i_key_strm.read();
        ap_uint<S_PW> pld = i_pld_strm.read(); // XXX trun
        last = i_e_strm.read();

        // mod 3 to calculate index for 24bit address
        ap_uint<HASHW> array_idx = hash_val / 3;
        ap_uint<HASHW> temp = array_idx * 3;
        ap_uint<2> bit_idx = hash_val - temp;

#ifndef __SYNTHESIS__
#ifdef DEBUG_MISS
        if (key == 8887)
            std::cout << std::hex << "build_stb debug: key=" << key << " hash=" << hash_val
                      << " error bitmap=" << uram_inst0.read(0xa7ed) << " array_idx=" << array_idx
                      << " bit_idx=" << bit_idx << std::endl;
#endif
#endif

        // read hash counter and ++, prevent duplicate key
        elem = uram_inst0.read(array_idx);

        // update && write new hash value
        ap_uint<24> v0 = elem(23, 0);
        ap_uint<24> v1 = elem(47, 24);
        ap_uint<24> v2 = elem(71, 48);

        ap_uint<24> v0a, v0b;
        ap_uint<24> v1a, v1b;
        ap_uint<24> v2a, v2b;

        ap_uint<ARW> hash_cnt;

        if (bit_idx == 0 || bit_idx == 3) {
            v0a = v0 + 1;
            v1a = v1;
            v2a = v2;

            v0b = ((v0 + 1) > depth) ? (v0 - depth + 1) : (ap_uint<24>*)0;
            v1b = (v1 > depth) ? (v1 - depth) : (ap_uint<24>*)0;
            v2b = (v2 > depth) ? (v2 - depth) : (ap_uint<24>*)0;

            hash_cnt = v0;
        } else if (bit_idx == 1) {
            v0a = v0;
            v1a = v1 + 1;
            v2a = v2;

            v0b = (v0 > depth) ? (v0 - depth) : (ap_uint<24>*)0;
            v1b = ((v1 + 1) > depth) ? (v1 - depth + 1) : (ap_uint<24>*)0;
            v2b = (v2 > depth) ? (v2 - depth) : (ap_uint<24>*)0;

            hash_cnt = v1;
        } else if (bit_idx == 2) {
            v0a = v0;
            v1a = v1;
            v2a = v2 + 1;

            v0b = (v0 > depth) ? (v0 - depth) : (ap_uint<24>*)0;
            v1b = (v1 > depth) ? (v1 - depth) : (ap_uint<24>*)0;
            v2b = ((v2 + 1) > depth) ? (v2 - depth + 1) : (ap_uint<24>*)0;

            hash_cnt = v2;
        }

        base_elem(23, 0) = v0a;
        base_elem(47, 24) = v1a;
        base_elem(71, 48) = v2a;

        overflow_elem(23, 0) = v0b;
        overflow_elem(47, 24) = v1b;
        overflow_elem(71, 48) = v2b;

        // connect key with payload
        ap_uint<KEYW + PW> srow = 0;
        srow = (key, pld);

        // update hash table
        uram_inst0.write(array_idx, base_elem);
        uram_inst1.write(array_idx, overflow_elem);

        // generate o_addr
        ap_uint<ARW> o_addr;

        if (hash_cnt >= depth) {
            // overflow
            o_addr = overflow_base + overflow_length_temp;
            overflow_length_temp++;
        } else {
            // underflow
            o_addr = depth * hash_val + hash_cnt;
        }

        o_addr_strm.write(o_addr);
        o_row_strm.write(srow);
        o_e_strm.write(false);
    }

    overflow_length = overflow_length_temp;
    o_e_strm.write(true);
}

// write row to HBM/DDR
template <int RW, int ARW, int OUTSTANDING>
void write_row(hls::burst_maxi<ap_uint<256> >& stb_buf,
               hls::stream<ap_uint<ARW> >& i_addr_strm,
               hls::stream<ap_uint<256> >& i_row_strm,
               hls::stream<bool>& i_e_strm) {
#pragma HLS INLINE off
    ap_uint<OUTSTANDING> check = 0;
    ap_uint<ARW> addr;
    ap_uint<256> row;
    int outstanding = OUTSTANDING;

    bool last = i_e_strm.read();
    while (!last || check != 0) {
#pragma HLS PIPELINE II = 1
        bool check_l = check[OUTSTANDING - 1];
        if (check_l) {
            stb_buf.write_response();
            outstanding++;
        }
        check = (check << 1);
        bool rd_e = !i_addr_strm.empty() && !i_e_strm.empty() && !i_row_strm.empty();
        if (rd_e && (outstanding > 0)) {
            {
#pragma HLS latency min = 1 max = 1
                i_addr_strm.read_nb(addr);
                i_e_strm.read_nb(last);
                i_row_strm.read_nb(row);
            }
            stb_buf.write_request(addr, 1);
            stb_buf.write(row);
            check[0] = 1;
            outstanding--;
        }
    }
}

/// @brief Write s-table to HBM/DDR
template <int ARW, int RW, int HBM_OUTSTANDING>
void write_stb(hls::burst_maxi<ap_uint<256> >& stb_buf,

               hls::stream<ap_uint<ARW> >& i_addr_strm,
               hls::stream<ap_uint<RW> >& i_row_strm,
               hls::stream<bool>& i_e_strm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    hls::stream<bool> e0_strm;
#pragma HLS STREAM variable = e0_strm depth = 8
#pragma HLS bind_storage variable = e0_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS STREAM variable = e1_strm depth = 8
#pragma HLS bind_storage variable = e1_strm type = fifo impl = srl

    hls::stream<ap_uint<ARW> > addr_strm;
#pragma HLS STREAM variable = addr_strm depth = 512
#pragma HLS bind_storage variable = addr_strm type = fifo impl = bram
    hls::stream<bool> e2_strm;
#pragma HLS STREAM variable = e2_strm depth = 512
#pragma HLS bind_storage variable = e2_strm type = fifo impl = srl

    hls::stream<ap_uint<256> > row_strm;
#pragma HLS STREAM variable = row_strm depth = 512
#pragma HLS bind_storage variable = row_strm type = fifo impl = bram
    hls::stream<bool> e3_strm;
#pragma HLS STREAM variable = e3_strm depth = 512
#pragma HLS bind_storage variable = e3_strm type = fifo impl = srl

    join_v3::sc::duplicate_strm_end<bool>(i_e_strm, e0_strm, e1_strm);

    join_v3::sc::stb_addr_gen<RW, ARW>(i_addr_strm, e0_strm, addr_strm, e2_strm);

    join_v3::sc::split_row<RW, ARW>(i_row_strm, e1_strm, row_strm, e3_strm);

    // XXX: hard-coded OUSTANDING to 8 for work-around the AXI adapter bug of HLS
    // as the internal depth of write-response FIFO is set to 8
    write_row<RW, ARW, 8>(stb_buf, addr_strm, row_strm, e3_strm);

    join_v3::sc::eliminate_strm_end<bool>(e2_strm);
}

/// @brief Top function of hash join build
template <int HASHW, int HASHWJ, int KEYW, int PW, int S_PW, int ARW, int HBM_OUTSTANDING, int NUM_X, int NUM_Y>
void build_wrapper(ap_uint<32>& depth,
                   ap_uint<32>& overflow_length,

                   hls::stream<ap_uint<HASHW> >& i_hash_strm,
                   hls::stream<ap_uint<KEYW> >& i_key_strm,
                   hls::stream<ap_uint<PW> >& i_pld_strm,
                   hls::stream<bool>& i_e_strm,

                   hls::burst_maxi<ap_uint<256> >& stb_buf,
                   ap_uint<72> (*uram_buffer0)[NUM_X][NUM_Y][4096],
                   ap_uint<72> (*uram_buffer1)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    hls::stream<ap_uint<HASHWJ> > trunc_hash_strm;
#pragma HLS stream variable = trunc_hash_strm depth = 8
#pragma HLS bind_storage variable = trunc_hash_strm type = fifo impl = srl
    hls::stream<bool> trunc_e_strm;
#pragma HLS stream variable = trunc_e_strm depth = 8

    hls::stream<ap_uint<ARW> > addr_strm;
#pragma HLS stream variable = addr_strm depth = 8
#pragma HLS bind_storage variable = addr_strm type = fifo impl = srl
    hls::stream<ap_uint<KEYW + S_PW> > row_strm;
#pragma HLS stream variable = row_strm depth = 8
#pragma HLS bind_storage variable = row_strm type = fifo impl = srl
    hls::stream<bool> e_strm;
#pragma HLS stream variable = e_strm depth = 8

    trunc_hash<HASHW, HASHWJ>(i_hash_strm, i_e_strm, trunc_hash_strm, trunc_e_strm);

    build_unit<HASHWJ, KEYW, PW, S_PW, ARW, NUM_X, NUM_Y>( //
        depth, overflow_length, trunc_hash_strm, i_key_strm, i_pld_strm, trunc_e_strm,

        addr_strm, row_strm, e_strm,

        uram_buffer0, uram_buffer1);

    write_stb<ARW, KEYW + S_PW, HBM_OUTSTANDING>(stb_buf, addr_strm, row_strm, e_strm);
}

// ************************************ Merge ****************************************

/// @brief Load overflow hash collision counter to genrate overflow hash bitmap
template <int HASHW, int ARW, int NUM_X, int NUM_Y>
void bitmap_addr_gen(ap_uint<32>& depth,
                     ap_uint<32>& overflow_length,

                     ap_uint<72> (*uram_buffer)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

    const int HASH_DEPTH = (1 << HASHW) / 3 + 1;
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst(uram_buffer);

    // base addr for overflow srow
    ap_uint<ARW> base_addr = 0;

BITMAP_ADDR_LOOP:
    for (int i = 0; i < HASH_DEPTH; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst.blocks inter false

        // genrate bitmap from hash counter
        ap_uint<72> elem = uram_inst.read(i);

        ap_uint<24> v0 = elem(23, 0);
        ap_uint<24> v1 = elem(47, 24);
        ap_uint<24> v2 = elem(71, 48);

        ap_uint<ARW> sum_0 = v0;
        ap_uint<ARW> sum_1 = v0 + v1;
        ap_uint<ARW> sum_2 = v0 + v1 + v2;

        ap_uint<72> head;
        head(23, 0) = base_addr;
        head(47, 24) = base_addr + sum_0;
        head(71, 48) = base_addr + sum_1;

        base_addr = base_addr + sum_2;

        // write bitmap addr to URAM for fully build
        uram_inst.write(i, head);
        ap_uint<128> bitmap = head;
    }
}

// merge overflow srow
template <int HASHWH, int HASHWJ, int KEYW, int PW, int ARW, int NUM_X, int NUM_Y>
void merge_unit(hls::stream<ap_uint<HASHWH + HASHWJ> >& i_hash_strm,
                hls::stream<ap_uint<KEYW> >& i_key_strm,
                hls::stream<ap_uint<PW> >& i_pld_strm,
                hls::stream<bool>& i_e_strm,

                hls::stream<ap_uint<ARW> >& o_addr_strm,
                hls::stream<ap_uint<KEYW + PW> >& o_row_strm,
                hls::stream<bool>& o_e_strm,

                ap_uint<72> (*uram_buffer)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

    const int number_of_stb_per_row = (KEYW + PW) / 256; // number of output based on 256 bit

    const int HASH_DEPTH = (1 << HASHWJ) / 3 + 1;
    UramCache<72, HASH_DEPTH, 8, NUM_X, NUM_Y> uram_inst(uram_buffer);

    ap_uint<72> elem = 0;
    ap_uint<72> new_elem = 0;
    ap_uint<72> elem_temp[4] = {0, 0, 0, 0};
    ap_uint<HASHWJ> array_idx_temp[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

    bool last = i_e_strm.read();
LOOP_BUILD_UNIT:
    while (!last) {
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = uram_inst.blocks inter false

        if (!i_e_strm.empty()) {
            // read
            ap_uint<KEYW> key = i_key_strm.read();
            ap_uint<PW> pld = i_pld_strm.read();
            ap_uint<HASHWH + HASHWJ> hash_val = i_hash_strm.read();
            last = i_e_strm.read();

            // connect key with payload
            ap_uint<KEYW + PW> stb_row = 0;
            stb_row = (key, pld);

            // mod 3 to calculate index for 24bit address
            ap_uint<HASHWJ> hash = hash_val(HASHWJ - 1, 0);
            ap_uint<HASHWJ> array_idx = hash / 3;
            ap_uint<HASHWJ> temp = array_idx * 3;
            ap_uint<2> bit_idx = hash - temp;

            // read hash counter and ++, prevent duplicate key
            elem = uram_inst.read(array_idx);

            ap_uint<24> v0 = elem(23, 0);
            ap_uint<24> v1 = elem(47, 24);
            ap_uint<24> v2 = elem(71, 48);

            ap_uint<24> v0a = (bit_idx == 0 || bit_idx == 3) ? ap_uint<24>(v0 + 1) : v0;
            ap_uint<24> v1a = (bit_idx == 1) ? ap_uint<24>(v1 + 1) : v1;
            ap_uint<24> v2a = (bit_idx == 2) ? ap_uint<24>(v2 + 1) : v2;

            new_elem(23, 0) = v0a;
            new_elem(47, 24) = v1a;
            new_elem(71, 48) = v2a;

            uram_inst.write(array_idx, new_elem);

            ap_uint<ARW> o_addr = (bit_idx == 0 || bit_idx == 3) ? v0 : ((bit_idx == 1) ? v1 : v2);

            // write stb
            o_addr_strm.write(o_addr);
            o_row_strm.write(stb_row);
            o_e_strm.write(false);
        }
    }

    o_e_strm.write(true);
}

// read row from HBM/DDR
template <int RW, int ARW, int OUTSTANDING>
void read_row(hls::burst_maxi<ap_uint<256> >& stb_buf,
              hls::stream<ap_uint<ARW> >& i_addr_strm,
              hls::stream<bool>& i_e_strm,

              hls::stream<ap_uint<256> >& o_row_strm,
              hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off

    ap_uint<OUTSTANDING> check = 0;
    ap_uint<ARW> addr;

    bool last = i_e_strm.read();
    while (!last || check != 0) {
#pragma HLS PIPELINE II = 1
        bool check_l = check[OUTSTANDING - 1];
        if (check_l) {
            o_row_strm.write(stb_buf.read());
            o_e_strm.write(false);
        }
        check = (check << 1);
        bool rd_e = !i_addr_strm.empty() && !i_e_strm.empty();
        if (rd_e) {
            {
#pragma HLS latency min = 1 max = 1
                i_addr_strm.read_nb(addr);
                i_e_strm.read_nb(last);
            }
            stb_buf.read_request(addr, 1);
            check[0] = 1;
        }
    }
    o_e_strm.write(true);
}

/// @brief Read s-table from HBM/DDR
template <int ARW, int RW, int HBM_OUTSTANDING>
void read_stb(hls::burst_maxi<ap_uint<256> >& stb_buf,

              hls::stream<ap_uint<ARW> >& i_addr_strm,
              hls::stream<bool>& i_e_strm,

              hls::stream<ap_uint<RW> >& o_row_strm,
              hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    hls::stream<ap_uint<ARW> > addr_strm;
#pragma HLS STREAM variable = addr_strm depth = 512
#pragma HLS bind_storage variable = addr_strm type = fifo impl = bram
    hls::stream<bool> e0_strm;
#pragma HLS STREAM variable = e0_strm depth = 512
#pragma HLS bind_storage variable = e0_strm type = fifo impl = srl

    hls::stream<ap_uint<256> > row_strm;
#pragma HLS STREAM variable = row_strm depth = 8
#pragma HLS bind_storage variable = row_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS STREAM variable = e1_strm depth = 8
#pragma HLS bind_storage variable = e1_strm type = fifo impl = srl

    join_v3::sc::stb_addr_gen<RW, ARW>(i_addr_strm, i_e_strm,

                                       addr_strm, e0_strm);

    read_row<RW, ARW, HBM_OUTSTANDING>(stb_buf, addr_strm, e0_strm,

                                       row_strm, e1_strm);

    join_v3::sc::combine_row<RW, ARW>(row_strm, e1_strm,

                                      o_row_strm, o_e_strm);
}

/// @brief Merge overflow srow from htb to stb
template <int HASH_MODE, int HASHWH, int HASHWJ, int KEYW, int PW, int ARW, int HBM_OUTSTANDING, int NUM_X, int NUM_Y>
void merge_stb(ap_uint<32>& depth,
               ap_uint<32>& overflow_length,
               hls::burst_maxi<ap_uint<256> >& htb_buf,
               hls::burst_maxi<ap_uint<256> >& stb_buf,
               ap_uint<72> (*uram_buffer)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    const int HASH_NUMBER = 1 << HASHWJ;

    hls::stream<ap_uint<ARW> > read_addr_strm;
#pragma HLS stream variable = read_addr_strm depth = 8
#pragma HLS bind_storage variable = read_addr_strm type = fifo impl = srl
    hls::stream<bool> e0_strm;
#pragma HLS stream variable = e0_strm depth = 8

    hls::stream<ap_uint<KEYW + PW> > row0_strm;
#pragma HLS stream variable = row0_strm depth = 8
#pragma HLS bind_storage variable = row0_strm type = fifo impl = srl
    hls::stream<bool> e1_strm;
#pragma HLS stream variable = e1_strm depth = 8

    hls::stream<ap_uint<KEYW> > key0_strm;
#pragma HLS stream variable = key0_strm depth = 8
#pragma HLS bind_storage variable = key0_strm type = fifo impl = srl
    hls::stream<ap_uint<PW> > pld_strm;
#pragma HLS stream variable = pld_strm depth = 8
#pragma HLS bind_storage variable = pld_strm type = fifo impl = srl
    hls::stream<bool> e2_strm;
#pragma HLS stream variable = e2_strm depth = 8

    hls::stream<ap_uint<KEYW> > key1_strm;
#pragma HLS stream variable = key1_strm depth = 8
#pragma HLS bind_storage variable = key1_strm type = fifo impl = srl
    hls::stream<ap_uint<HASHWH + HASHWJ> > hash_strm;
#pragma HLS stream variable = hash_strm depth = 8
#pragma HLS bind_storage variable = hash_strm type = fifo impl = srl
    hls::stream<bool> e3_strm;
#pragma HLS stream variable = e3_strm depth = 8

    hls::stream<ap_uint<KEYW + PW> > row1_strm;
#pragma HLS stream variable = row1_strm depth = 8
#pragma HLS bind_storage variable = row1_strm type = fifo impl = srl
    hls::stream<ap_uint<ARW> > write_addr_strm;
#pragma HLS stream variable = write_addr_strm depth = 8
#pragma HLS bind_storage variable = write_addr_strm type = fifo impl = srl
    hls::stream<bool> e4_strm;
#pragma HLS stream variable = e4_strm depth = 8

    // generate read addr
    join_v3::sc::read_addr_gen<ARW>(HASH_NUMBER * depth, overflow_length,

                                    read_addr_strm, e0_strm);

    // read overflow srow in stb_buf
    read_stb<ARW, KEYW + PW, HBM_OUTSTANDING>(stb_buf,

                                              read_addr_strm, e0_strm,

                                              row0_strm, e1_strm);

    // split row to hash and key
    splitCol<KEYW + PW, PW, KEYW>(row0_strm, e1_strm,

                                  pld_strm, key0_strm, e2_strm);

    // calculate hash
    join_v3::sc::hash_wrapper<HASH_MODE, KEYW, HASHWH + HASHWJ>(key0_strm, e2_strm,

                                                                hash_strm, key1_strm, e3_strm);

    // build overflow srow with its hash addr
    merge_unit<HASHWH, HASHWJ, KEYW, PW, ARW, NUM_X, NUM_Y>(hash_strm, key1_strm, pld_strm, e3_strm, write_addr_strm,
                                                            row1_strm, e4_strm,

                                                            uram_buffer);

    // write shuffled overflow srow to htb_buf
    write_stb<ARW, KEYW + PW, HBM_OUTSTANDING>(htb_buf, write_addr_strm, row1_strm, e4_strm);
}

/// @brief Top function of hash join merge
template <int HASH_MODE, int HASHWH, int HASHWJ, int KEYW, int PW, int ARW, int HBM_OUTSTANDING, int NUM_X, int NUM_Y>
void merge_wrapper(
    // input status
    ap_uint<32>& depth,
    ap_uint<32>& overflow_length,

    hls::burst_maxi<ap_uint<256> >& htb_buf,
    hls::burst_maxi<ap_uint<256> >& stb_buf,
    ap_uint<72> (*uram_buffer)[NUM_X][NUM_Y][4096]) {
#pragma HLS INLINE off

    // generate overflow bitmap by overflow hash counter
    bitmap_addr_gen<HASHWJ, ARW, NUM_X, NUM_Y>(depth, overflow_length, uram_buffer);

    // build overflow srow with its hash addr
    merge_stb<HASH_MODE, HASHWH, HASHWJ, KEYW, PW, ARW, HBM_OUTSTANDING, NUM_X, NUM_Y>(depth, overflow_length, htb_buf,
                                                                                       stb_buf, uram_buffer);
}

} // namespace details
} // namespace database
} // namespace xf

#endif // GQE_ISV_BUILD_FLOW_HPP
