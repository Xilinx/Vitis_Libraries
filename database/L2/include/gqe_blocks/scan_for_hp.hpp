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
#ifndef GQE_SCAN_FOR_HP_HPP
#define GQE_SCAN_FOR_HP_HPP

#ifndef __SYNTHESIS__
#include <stdio.h>
#include <iostream>
#endif

#include <ap_int.h>
#include <hls_stream.h>

#include "xf_database/utils.hpp"
#include "xf_database/types.hpp"

namespace xf {
namespace database {
namespace gqe {

template <int burst_len, int vec_len, int size0, int col_num>
void _read_to_colvec(const int bit_num,
                     ap_uint<8 * size0 * vec_len>* ptr,
                     hls::stream<int8_t>& col_id_strm,
                     hls::stream<ap_uint<8 * size0 * vec_len> > out_strm[col_num],
                     hls::stream<int>& nrow_strm,
                     hls::stream<int>& bit_num_strm,
                     hls::stream<int>& bit_num_strm_copy) {
    ap_uint<512> bw = ptr[0];

    // number of row in each col.
    int nrow = bw.range(31, 0);
    nrow_strm.write(nrow); // tells splitter

    // size of buffer space for 1 col.
    // int col_naxi = (bw.range(63,32) + 63) / 64;
    int col_naxi = bw.range(63, 32).to_int();

    bit_num_strm.write(bit_num);
    bit_num_strm_copy.write(bit_num);

    // offset of col data
    int col_offset[col_num];
#pragma HLS array_partition variable = col_offset complete
    for (int i = 0; i < col_num; ++i) {
        int cid = col_id_strm.read();
        if (cid == -1) {
            col_offset[i] = -1;
        } else if (cid == -2) {
            col_offset[i] = -2;
        } else {
            // +1 to skip col header to data offset
            col_offset[i] = col_naxi * cid + 1;
        }
    }

    // AXI read for each col
    int nread = (nrow + vec_len - 1) / vec_len;

    ap_uint<512> cnt;
    //#pragma HLS array_partition variable=cnt complete

    for (int i = 0; i < vec_len; i++) {
#pragma HLS UNROLL
        cnt.range((i + 1) * 32 - 1, i * 32) = i;
    }

    for (int i = 0; i < nread; i += burst_len) {
        const int len = ((i + burst_len) > nread) ? (nread - i) : burst_len;
        // do a burst read for one col
        for (int c = 0; c < col_num; ++c) {
            int offset = col_offset[c];
            if (offset < 0) {
                ap_uint<512> cnt_tmp = cnt;
                for (int j = 0; j < len; ++j) {
#pragma HLS pipeline II = 1
                    ap_uint<512> t;
                    // feed dummy data into col
                    if (offset == -1) t = 0;
                    // feed rowid into col
                    else if (offset == -2)
                        t = cnt_tmp;
                    out_strm[c].write(t);
                    for (int i = 0; i < vec_len; i++) {
#pragma HLS UNROLL
                        cnt_tmp.range((i + 1) * 32 - 1, i * 32) = cnt_tmp.range((i + 1) * 32 - 1, i * 32) + vec_len;
                    }
                }
            } else {
                // burst read for col
                for (int j = 0; j < len; ++j) {
#pragma HLS pipeline II = 1
                    ap_uint<512> t = ptr[offset + i + j];
                    out_strm[c].write(t);
                }
            }
        }
        for (int i = 0; i < vec_len; i++) {
#pragma HLS UNROLL
            cnt.range((i + 1) * 32 - 1, i * 32) = cnt.range((i + 1) * 32 - 1, i * 32) + len * vec_len;
        }
    }
}

template <int vec_len, int ch_num, int size0, int col_num>
void _split_colvec_to_channel(hls::stream<ap_uint<8 * size0 * vec_len> > colvec_strm[col_num],
                              hls::stream<int>& nrow_strm,
                              hls::stream<ap_uint<8 * size0> > col_strm[ch_num][col_num],
                              hls::stream<bool> e_strm[ch_num]) {
    enum { per_ch = vec_len / ch_num };
    int nrow = nrow_strm.read();
SPLIT_COL_VEC:
    for (int i = 0; i < nrow; i += vec_len) {
#pragma HLS pipeline II = per_ch
        ap_uint<8 * size0 * vec_len> colvec[col_num];
        for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
            colvec[c] = colvec_strm[c].read();
        }
        int n = (i + vec_len) > nrow ? (nrow - i) : vec_len;
        XF_DATABASE_ASSERT((vec_len % ch_num == 0) && (vec_len >= ch_num));
        // j for word in vec
        for (int j = 0; j < per_ch; ++j) {
            // ch for channel
            for (int ch = 0; ch < ch_num; ++ch) {
#pragma HLS unroll
                ap_uint<8 * size0> ct[col_num];
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    ct[c] = colvec[c].range(8 * size0 * (j * ch_num + ch + 1) - 1, 8 * size0 * (j * ch_num + ch));
                }
                if ((j * ch_num + ch) < n) {
                    for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                        col_strm[ch][c].write(ct[c]);
                    }
                    e_strm[ch].write(false);
                }
            }
        }
    }
    for (int ch = 0; ch < ch_num; ++ch) {
#pragma HLS unroll
        e_strm[ch].write(true);
    }
}

template <int COL_NM, int CH_NM>
void scan_to_channel(const int bit_num,
                     ap_uint<8 * TPCH_INT_SZ * VEC_LEN>* ptr,
                     hls::stream<int8_t>& col_id_strm,
                     hls::stream<ap_uint<8 * TPCH_INT_SZ> > out_strms[CH_NM][COL_NM],
                     hls::stream<bool> e_out_strms[CH_NM],
                     hls::stream<int>& bit_num_strm,
                     hls::stream<int>& bit_num_strm_copy) {
#pragma HLS dataflow
    enum { fifo_depth = BURST_LEN * 2 };

    hls::stream<int> nrow_strm;
#pragma HLS stream variable = nrow_strm depth = 8

    hls::stream<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > tmp_strms[COL_NM];
#pragma HLS stream variable = tmp_strms depth = fifo_depth
#pragma HLS resource variable = tmp_strms core = FIFO_LUTRAM

    _read_to_colvec<BURST_LEN, VEC_LEN, TPCH_INT_SZ, COL_NM>(bit_num, ptr, col_id_strm, //
                                                             tmp_strms, nrow_strm, bit_num_strm, bit_num_strm_copy);

    _split_colvec_to_channel<VEC_LEN, CH_NM, TPCH_INT_SZ, COL_NM>(tmp_strms, nrow_strm, //
                                                                  out_strms, e_out_strms);
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif // GQE_SCAN_TO_CHANNEL_H
