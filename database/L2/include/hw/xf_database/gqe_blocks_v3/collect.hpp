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

#ifndef GQE_ISV_COLLECT_HPP
#define GQE_ISV_COLLECT_HPP

#ifndef __cplusplus
#error "Vitis Database Library only works with C++."
#endif

// for using mul_ch_read & mux
#include "xf_database/hash_join_v2.hpp"

#include "xf_database/gqe_blocks_v3/gqe_enums.hpp"

#include "ap_int.h"
#include "hls_stream.h"

namespace xf {
namespace database {
namespace details {

// @brief collect join result of multiple PU (N-to-1 merge) 1 by 1.
// collect part result of multiple PU segment by segment.
// collect bloom filter dotted hash table group by group (containing synchronization)
// MSB      -       LSB
// key | join_pld | pld
// XXX: caution, this implementation is really tricky for the HLS, currently only verified with 2020.2 released
// Vitis-HLS.
// Better don't touch this module if not necessary, as logic may be mis-synthesized (Csim/Cosim mismatch).
// template <int PU, int EW, int VEC_LEN, int COL_NM, int GRP_SZ>
template <int PU, int EW, int COL_NM, int GRP_SZ>
void collect(hls::stream<ap_uint<8> >& i_general_cfg_strm,
             hls::stream<ap_uint<EW * VEC_LEN> > i_strm[PU][COL_NM],
             hls::stream<ap_uint<10> > i_nm_strm[PU],
             hls::stream<ap_uint<10> > i_hp_bk_strm[PU],

             ap_uint<32>& join_num,
             hls::stream<ap_uint<EW * VEC_LEN> > o_row_strm[COL_NM],
             hls::stream<ap_uint<10> >& o_nm_strm,
             hls::stream<ap_uint<10 + Log2<PU>::value> >& o_hp_bkpu_strm) {
#pragma HLS INLINE off

    ap_uint<8> general_cfg = i_general_cfg_strm.read();
    bool build_probe_flag = general_cfg[5];
    bool part_on = general_cfg[3];

    const int MAX = (1 << PU) - 1;
    ap_uint<GRP_SZ> hash_table = 0;
    ap_uint<EW * VEC_LEN * COL_NM> row_out_tmp;
    ap_uint<EW * VEC_LEN> row_out[COL_NM];
#pragma HLS array_partition variable = row_out dim = 0
    ap_uint<PU> priority = 1;

    ap_uint<PU> empty_e = 0;
    ap_uint<PU> rd_e = 0;
    ap_uint<32> cnt = 0;
    ap_uint<10> nm[PU];
#pragma HLS array_partition variable = nm dim = 1
    ap_uint<10> nm_r = 0;
    ap_uint<PU> last = 0;
    ap_uint<PU> sec_end = 0;
#ifndef __SYNTHESIS__
    std::cout << std::dec << "PU=" << PU << std::endl;
#endif

    // do collect if it is probe
    if (build_probe_flag) {
        // collect data segment by segment for hash partition
        if (part_on) {
            do {
                for (int i = 0; i < PU; i++) {
#pragma HLS unroll
                    empty_e[i] = !i_nm_strm[i].empty() && !last[i] && !i_hp_bk_strm[i].empty();
                }
                if (priority & empty_e) {
                    rd_e = priority;
                    priority = (priority << 1) | (priority >> (PU - 1));
                } else {
                    rd_e = join_v2::mul_ch_read(empty_e);
                }
                ap_uint<Log2<PU>::value> idx = join_v2::mux<PU>(rd_e);

                ap_uint<10> bk;
                if (rd_e != 0) {
                    {
#pragma HLS latency min = 1 max = 1
                        i_nm_strm[idx].read_nb(nm[idx]);
                        i_hp_bk_strm[idx].read_nb(bk);
                    }
                    nm_r = (nm[idx] + VEC_LEN - 1) / VEC_LEN;
                    if (nm[idx] == 0) {
                        last[idx] = 1;
                    } else {
                        o_hp_bkpu_strm.write((idx, bk));
                        o_nm_strm.write(nm[idx]);
                    FORWARD_CORE_LOOP:
                        while (nm_r > 0) {
#pragma HLS pipeline II = 1
                            if (!i_strm[idx][0].empty() && !i_strm[idx][1].empty() && !i_strm[idx][2].empty()) {
                                for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                                    ap_uint<EW * VEC_LEN> tmp_r;
                                    i_strm[idx][c].read_nb(tmp_r);
                                    o_row_strm[c].write(tmp_r);
                                }
                                nm_r--;
                            }
                        }
                    }
                }
            } while (last != MAX);
        } else {
            do {
#pragma HLS pipeline II = 1 style = flp
                for (int i = 0; i < PU; i++) {
#pragma HLS unroll
                    empty_e[i] = !i_nm_strm[i].empty() && !last[i] && !sec_end[i] && !i_strm[i][0].empty() &&
                                 !i_strm[i][1].empty() && !i_strm[i][2].empty();
                }
                rd_e = join_v2::mul_ch_read(empty_e);
                ap_uint<Log2<PU>::value> idx = join_v2::mux<PU>(rd_e);
                switch (general_cfg.range(4, 0)) {
                    case gqe::JoinOn:
                        if (rd_e != 0) {
                            {
#pragma HLS latency min = 1 max = 1
                                i_nm_strm[idx].read_nb(nm[idx]);
                                for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                                    ap_uint<EW * VEC_LEN> tmp_r;
                                    i_strm[idx][c].read_nb(tmp_r);
                                    row_out_tmp((c + 1) * EW * VEC_LEN - 1, EW * VEC_LEN * c) = tmp_r;
                                }
                            }
                            if (nm[idx] > 0) {
                                for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                                    o_row_strm[c].write(
                                        row_out_tmp.range((c + 1) * EW * VEC_LEN - 1, EW * VEC_LEN * c));
                                }
                                o_nm_strm.write(1);
                                cnt++;
                            } else {
                                last[idx] = 1;
                            }
                        }
                        break;
                    case gqe::BloomFilterOn:
                        if (rd_e != 0) {
                            {
#pragma HLS latency min = 1 max = 1
                                i_nm_strm[idx].read_nb(nm[idx]);
                                for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                                    ap_uint<EW * VEC_LEN> tmp_r;
                                    i_strm[idx][c].read_nb(tmp_r);
                                    row_out_tmp((c + 1) * EW * VEC_LEN - 1, EW * VEC_LEN * c) = tmp_r;
                                }
                            }
                            if (nm[idx] > 0) {
                                sec_end[idx] = 1;
                                hash_table |= (ap_uint<GRP_SZ>)row_out_tmp.range(GRP_SZ - 1, 0);
                                if (sec_end == MAX) {
                                    for (int c = 0; c < COL_NM; c++) {
#pragma HLS unroll
                                        row_out_tmp.range(EW * VEC_LEN * c + GRP_SZ - 1, EW * VEC_LEN * c) = hash_table;
                                        o_row_strm[c].write(row_out_tmp((c + 1) * EW * VEC_LEN - 1, EW * VEC_LEN * c));
                                    }
                                    o_nm_strm.write(1);
                                    sec_end = 0;
                                    hash_table = 0;
                                }
                            } else {
                                last[idx] = 1;
                            }
                        }
                        break;
                    default:
#ifndef __SYNTHESIS__
                        std::cerr << "Error: illegal kernel switching combination\n";
                        exit(1);
#endif
                        break;
                }
            } while (last != MAX);
        }

#ifndef __SYNTHESIS__
        std::cout << std::dec << "Collect " << cnt << " rows" << std::endl;
#endif
    }
    join_num = cnt;
    o_nm_strm.write(0);
}

} // namespace details
} // namespace database
} // namespace xf

#endif // GQE_ISV_COLLECT_HPP
