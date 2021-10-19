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

/**
 * @file collect.hpp
 * @brief N-to-1 collector (merge), sub-sections divided by payload (row-id) = -1.
 *
 *
 * This file is part of Vitis Database Library.
 */

#ifndef XF_DATABASE_COLLECT_H
#define XF_DATABASE_COLLECT_H

#ifndef __cplusplus
#error "Vitis Database Library only works with C++."
#endif

// for using mul_ch_read & mux
#include "xf_database/hash_join_v2.hpp"

#include "ap_int.h"
#include "hls_stream.h"

namespace xf {
namespace database {
namespace details {

// @brief collect join result of mutiple PU (N-to-1 merge with all Fs as separator)
// MSB      -       LSB
// key | join_pld | pld
// XXX: caution, this implementation is really tricky for the HLS, currently only verified with 2020.2 released
// Vitis-HLS.
// Better don't touch this module if not necessary, as logic may be mis-synthesized (Csim/Cosim mismatch).
template <int PU, int KEYW, int S_PW, int B_PW>
void collect_unit(hls::stream<bool> bp_flag_strm[PU],
                  hls::stream<ap_uint<KEYW + S_PW + B_PW> > i_jrow_strm[PU],
                  hls::stream<bool> i_e_strm[PU],

                  ap_uint<32>& join_num,
                  hls::stream<ap_uint<KEYW + S_PW + B_PW> >& o_jrow_strm,
                  hls::stream<bool>& o_e_strm) {
#pragma HLS INLINE off

    const bool build_probe_flag = bp_flag_strm[0].read();
    for (int i = 1; i < PU; i++) {
        bp_flag_strm[i].read();
    }

    const int MAX = (1 << PU) - 1;
    ap_uint<KEYW + S_PW + B_PW> jrow_arr[PU];
#pragma HLS array_partition variable = jrow_arr dim = 1

    ap_uint<PU> empty_e = 0;
    ap_uint<PU> rd_e = 0;
    ap_uint<32> cnt = 0;
    ap_uint<PU> last = 0;
    ap_uint<PU> sec_end = 0;
#ifndef __SYNTHESIS__
    std::cout << std::dec << "PU=" << PU << std::endl;
#endif

    // do collect if it is probe
    if (build_probe_flag) {
        do {
#pragma HLS pipeline II = 1
            for (int i = 0; i < PU; i++) {
#pragma HLS unroll
                empty_e[i] = !i_e_strm[i].empty() && !last[i] && !sec_end[i];
            }

            rd_e = join_v2::mul_ch_read(empty_e);

            for (int i = 0; i < PU; i++) {
#pragma HLS unroll
                if (rd_e[i]) {
                    jrow_arr[i] = i_jrow_strm[i].read();
                    last[i] = i_e_strm[i].read();
                    if (jrow_arr[i].range(B_PW - 1, 0) == ~ap_uint<B_PW>(0)) {
                        sec_end[i] = 1;
                    }
                }
            }

            ap_uint<3> id = join_v2::mux<PU>(rd_e);
            ap_uint<KEYW + S_PW + B_PW> j = jrow_arr[id];
            bool valid_n = last[id];
            if (!valid_n && rd_e != 0 && !sec_end[id]) {
                o_jrow_strm.write(j);
                o_e_strm.write(false);
                cnt++;
            } else if (sec_end == MAX) {
                j.range(KEYW + S_PW + B_PW - 1, S_PW + B_PW) = ~ap_uint<KEYW>(0);
                j.range(B_PW - 1, 0) = ~ap_uint<B_PW>(0);
                o_jrow_strm.write(j);
                o_e_strm.write(false);
                for (int i = 0; i < PU; i++) {
#pragma HLS unroll
                    sec_end[i] = 0;
                }
            }

        } while (last != MAX);

#ifndef __SYNTHESIS__
        std::cout << std::dec << "Collect " << cnt << " rows" << std::endl;
#endif
    }
    join_num = cnt;
    o_e_strm.write(true);
}

// @brief for validation buffer of gqeFilter, split input rows with size of GRP_SZ, and dot the valid rows with true
template <int KEYW, int S_PW, int B_PW, int GRP_SZ>
void dot_hash(bool bf_on,
              hls::stream<ap_uint<KEYW + S_PW + B_PW> >& i_jrow_strm,
              hls::stream<bool>& i_e_strm,

              hls::stream<ap_uint<GRP_SZ> >& hash_table_strm,
              hls::stream<ap_uint<KEYW + S_PW + B_PW> >& o_jrow_strm,
              hls::stream<bool>& o_e_strm) {
    ap_uint<KEYW + S_PW + B_PW> j;
    ap_uint<GRP_SZ> hash_table = 0;

    bool last = i_e_strm.read();
    while (!last) {
#pragma HLS pipeline II = 1
        j = i_jrow_strm.read();
        last = i_e_strm.read();
        if (bf_on) {
            // dot hash table for valid keys
            ap_uint<B_PW> row_id = j.range(B_PW - 1, 0);

            // all Fs as end of section
            if (row_id == ~ap_uint<B_PW>(0)) {
                hash_table_strm.write(hash_table);
                o_e_strm.write(false);
                hash_table = 0;
            } else {
                row_id--;
                hash_table[row_id % GRP_SZ] = 1;
            }

            // bypass if not bloom-filtering
        } else {
            o_jrow_strm.write(j);
            o_e_strm.write(false);
        }
    }
    o_e_strm.write(true);
}

// @brief flush the dotted hash-table or joined rows out
template <int KEYW, int S_PW, int B_PW, int GRP_SZ>
void down_size(bool bf_on,
               hls::stream<ap_uint<GRP_SZ> >& hash_table_strm,
               hls::stream<ap_uint<KEYW + S_PW + B_PW> >& i_jrow_strm,
               hls::stream<bool>& i_e_strm,

               hls::stream<ap_uint<KEYW + S_PW + B_PW> >& o_jrow_strm,
               hls::stream<bool>& o_e_strm) {
    ap_uint<GRP_SZ> hash_table;
    int cnt = 0;
    int num_per_line = GRP_SZ / B_PW;

    bool last = i_e_strm.read();
    while (!last || (cnt != 0)) {
#pragma HLS pipeline II = 1
        if (bf_on) {
            if (cnt == 0) {
                hash_table = hash_table_strm.read();
                last = i_e_strm.read();
            }
            ap_uint<KEYW + S_PW + B_PW> j = 0;
            // as we use col0_out channel for emitting valid flags
            j.range(B_PW - 1, 0) = hash_table.range(B_PW - 1 + B_PW * cnt, B_PW * cnt);
            o_jrow_strm.write(j);
            o_e_strm.write(false);
            if (cnt == num_per_line - 1) {
                cnt = 0;
            } else {
                cnt++;
            }
            // bypass if not bloom-filtering
        } else {
            ap_uint<KEYW + S_PW + B_PW> j = i_jrow_strm.read();
            last = i_e_strm.read();
            o_jrow_strm.write(j);
            o_e_strm.write(false);
        }
    }
    o_e_strm.write(true);
}

// @brief top wrapper of collect module
template <int PU, int KEYW, int S_PW, int B_PW, int GRP_SZ>
void collect(bool bf_on,
             hls::stream<bool> bp_flag_strm[PU],
             hls::stream<ap_uint<KEYW + S_PW + B_PW> > i_jrow_strm[PU],
             hls::stream<bool> i_e_strm[PU],

             ap_uint<32>& join_num,
             hls::stream<ap_uint<KEYW + S_PW + B_PW> >& o_jrow_strm,
             hls::stream<bool>& o_e_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<GRP_SZ> > hash_table_strm("hash_table_strm");
#pragma HLS stream variable = hash_table_strm depth = 8
#pragma HLS resource variable = hash_table_strm core = FIFO_SRL
    hls::stream<ap_uint<KEYW + S_PW + B_PW> > col_jrow_strm("col_jrow_strm");
#pragma HLS stream variable = col_jrow_strm depth = 8
#pragma HLS resource variable = col_jrow_strm core = FIFO_SRL
    hls::stream<bool> col_e_strm("col_e_strm");
#pragma HLS stream variable = col_e_strm depth = 8
#pragma HLS resource variable = col_e_strm core = FIFO_SRL

    hls::stream<ap_uint<KEYW + S_PW + B_PW> > dot_jrow_strm("dot_jrow_strm");
#pragma HLS stream variable = dot_jrow_strm depth = 8
#pragma HLS resource variable = dot_jrow_strm core = FIFO_SRL
    hls::stream<bool> dot_e_strm("dot_e_strm");
#pragma HLS stream variable = dot_e_strm depth = 8
#pragma HLS resource variable = dot_e_strm core = FIFO_SRL

    collect_unit<PU, KEYW, S_PW, B_PW>(bp_flag_strm, i_jrow_strm, i_e_strm, join_num, col_jrow_strm, col_e_strm);
    dot_hash<KEYW, S_PW, B_PW, GRP_SZ>(bf_on, col_jrow_strm, col_e_strm, hash_table_strm, dot_jrow_strm, dot_e_strm);
    down_size<KEYW, S_PW, B_PW, GRP_SZ>(bf_on, hash_table_strm, dot_jrow_strm, dot_e_strm, o_jrow_strm, o_e_strm);
}

} // namespace details
} // namespace database
} // namespace xf

#endif // !defined(XF_DATABASE_COLLECT_H)
