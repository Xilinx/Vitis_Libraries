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
#ifndef GQE_WRITE_OUT_HPP
#define GQE_WRITE_OUT_HPP

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#include <ap_int.h>
#include <hls_stream.h>

#include "xf_database/utils.hpp"

namespace xf {
namespace database {
namespace gqe {

template <int burst_len, int elem_size, int vec_len, int col_num>
void countForBurst(hls::stream<ap_uint<elem_size> > i_post_Agg[col_num],
                   hls::stream<bool>& i_e_strm,
                   hls::stream<ap_uint<elem_size * vec_len> > o_post_Agg[col_num],
                   hls::stream<ap_uint<8> >& o_nm_strm,
                   hls::stream<ap_uint<32> >& rnm_strm,
                   hls::stream<ap_uint<32> >& wr_cfg_istrm,
                   hls::stream<ap_uint<32> >& wr_cfg_ostrm) {
    const int sz = elem_size;
    bool e = i_e_strm.read();
    ap_uint<32> write_out_cfg = wr_cfg_istrm.read();
    wr_cfg_ostrm.write(write_out_cfg);
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
#endif

    ap_uint<sz * vec_len> vecs[col_num];
    int n = 0; // nrow count
    int r = 0; // element count in one 512b
    int b = 0; // burst lentg count
#pragma HLS array_partition variable = vecs complete
    while (!e) {
#pragma HLS pipeline II = 1
        e = i_e_strm.read();
        ++n;
        for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
            ap_uint<sz> t = i_post_Agg[c].read();
            vecs[c].range(sz * (vec_len - 1) - 1, 0) = vecs[c].range(sz * vec_len - 1, sz);
            vecs[c].range(sz * vec_len - 1, sz * (vec_len - 1)) = t;
            // vecs[c].range(sz * (r + 1) - 1, sz * r) = t;
        }
        if (r == vec_len - 1) {
            for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                if (write_out_cfg[c]) {
                    o_post_Agg[c].write(vecs[c]);
                }
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
                // vecs[c].range(sz * (r + 1) - 1, sz * r) = 0;
            }
        }
        for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
            if (write_out_cfg[c]) {
                o_post_Agg[c].write(vecs[c]);
            }
        }
        ++b;
    }
    if (b != 0) {
        XF_DATABASE_ASSERT(b <= burst_len);
        o_nm_strm.write(b);
        o_nm_strm.write(0);
    } else {
        o_nm_strm.write(0);
    }
    rnm_strm.write(n);
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    for (int c = 0; c < col_num; ++c) {
        printf("write out col %d size is %d\n", c, o_post_Agg[c].size());
    }
    printf("write out nm stream size is %d\n", o_nm_strm.size());
#endif
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void burstWrite(hls::stream<ap_uint<elem_size * vec_len> > i_strm[col_num],
                ap_uint<elem_size * vec_len>* ptr,
                hls::stream<ap_uint<8> >& nm_strm,
                hls::stream<ap_uint<32> >& rnm_strm,
                hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    // record the burst nubmer
    unsigned bnm = 0;
    // read out the block size
    ap_uint<elem_size* vec_len> first_r = ptr[0];
    int BLOCK_SIZE = first_r(elem_size * 2 - 1, elem_size).to_int();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << "block size for every col=" << BLOCK_SIZE << std::endl;
#endif
    // write_out_cfg, write out of bypass
    ap_uint<32> write_out_cfg = write_out_cfg_strm.read();
    int col_id[col_num];
    int col_offset[col_num];
#pragma HLS array_partition variable = col_offset complete
    int wcol = 0;
    for (int i = 0; i < col_num; ++i) {
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
    //
    int nm = nm_strm.read();
    while (nm) {
        // write each col one burst
        for (int k = 0; k < wcol; ++k) {
            const int id = col_id[k];
            const int offset = col_offset[k];
        BURST_WRITE_CORE_LOOP:
            for (int n = 0; n < nm; ++n) {
#pragma HLS pipeline II = 1
                ap_uint<elem_size* vec_len> out = i_strm[id].read();
                ptr[offset + bnm * burst_len + n] = out;
            }
        }
        bnm++;
        nm = nm_strm.read();
    }
    ap_uint<32> rnm = rnm_strm.read();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
    std::cout << std::dec << "write out row=" << rnm.to_int() << " col_nm=" << wcol << std::endl;
#endif
    first_r(elem_size - 1, 0) = rnm;
    ptr[0] = first_r;
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeTable(hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                hls::stream<bool>& e_post_Agg,
                ap_uint<elem_size * vec_len>* ptr,
                hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    const int k_fifo_buf = burst_len * 2;

    hls::stream<ap_uint<elem_size * vec_len> > m_post_Agg[col_num];
#pragma HLS array_partition variable = m_post_Agg dim = 0
#pragma HLS stream variable = m_post_Agg depth = k_fifo_buf
#pragma HLS resource variable = m_post_Agg core = FIFO_LUTRAM
    hls::stream<ap_uint<8> > nm_strm;
#pragma HLS stream variable = nm_strm depth = 2
    hls::stream<ap_uint<32> > rnm_strm;
#pragma HLS stream variable = rnm_strm depth = 4
    hls::stream<ap_uint<32> > mid_wr_cfg_strm;
#pragma HLS stream variable = mid_wr_cfg_strm depth = 1

#pragma HLS dataflow

    countForBurst<burst_len, elem_size, vec_len, col_num>(post_Agg, e_post_Agg, m_post_Agg, nm_strm, rnm_strm,
                                                          write_out_cfg_strm, mid_wr_cfg_strm);

#if !defined(__SYNTHESIS__) && XDEBUG == 1
    printf("\npost_Agg: %ld, e_post_Agg:%ld, m_post_Agg:%ld, nm_strm:%ld, rnm_strm:%ld\n\n", post_Agg[0].size(),
           e_post_Agg.size(), m_post_Agg[0].size(), nm_strm.size(), rnm_strm.size());
#endif

    burstWrite<burst_len, elem_size, vec_len, col_num>(m_post_Agg, ptr, nm_strm, rnm_strm, mid_wr_cfg_strm);
}

template <int elem_size, int vec_len, int col_num>
void align512(hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
              hls::stream<bool>& e_bfr_strm,
              hls::stream<ap_uint<elem_size> > post_Agg[col_num],
              hls::stream<bool>& e_post_Agg,
              hls::stream<ap_uint<elem_size> > agg_strm[col_num],
              hls::stream<bool>& agg_strm_e) {
    bool e = e_bfr_strm.read();
    while (!e) {
#pragma HLS pipeline II = 1
        for (int i = 0; i < col_num; i++) {
#pragma HLS UNROLL
            agg_strm[i].write(bfr_strm[i].read());
        }
        agg_strm_e.write(false);
        e = e_bfr_strm.read();
    }

    e = e_post_Agg.read();
    while (!e) {
#pragma HLS pipeline II = 1
        for (int i = 0; i < col_num; i++) {
#pragma HLS UNROLL
            agg_strm[i].write(post_Agg[i].read());
        }
        agg_strm_e.write(false);
        e = e_post_Agg.read();
    }
    agg_strm_e.write(true);
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void countForBurstV2(hls::stream<ap_uint<elem_size> > i_post_Agg[col_num],
                     hls::stream<bool>& i_e_strm,
                     hls::stream<ap_uint<elem_size * vec_len> > o_post_Agg[col_num],
                     hls::stream<ap_uint<8> >& o_nm_strm,
                     hls::stream<ap_uint<32> >& rnm_strm,
                     hls::stream<ap_uint<32> >& wr_cfg_istrm,
                     hls::stream<ap_uint<32> >& wr_cfg_ostrm) {
    const int sz = elem_size;
    bool e = i_e_strm.read();
    ap_uint<32> write_out_cfg = wr_cfg_istrm.read();
    wr_cfg_ostrm.write(write_out_cfg);
    wr_cfg_ostrm.write(wr_cfg_istrm.read());
    wr_cfg_ostrm.write(wr_cfg_istrm.read());
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
#endif

    ap_uint<sz * vec_len> vecs[col_num];
    int n = 0; // nrow count
    int r = 0; // element count in one 512b
    int b = 0; // burst lentg count
#pragma HLS array_partition variable = vecs complete
    while (!e) {
#pragma HLS pipeline II = 1
        e = i_e_strm.read();
        ++n;
        for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
            ap_uint<sz> t = i_post_Agg[c].read();
            vecs[c].range(sz * (vec_len - 1) - 1, 0) = vecs[c].range(sz * vec_len - 1, sz);
            vecs[c].range(sz * vec_len - 1, sz * (vec_len - 1)) = t;
            // vecs[c].range(sz * (r + 1) - 1, sz * r) = t;
        }
        if (r == vec_len - 1) {
            for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                if (write_out_cfg[c]) {
                    o_post_Agg[c].write(vecs[c]);
                }
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
                // vecs[c].range(sz * (r + 1) - 1, sz * r) = 0;
            }
        }
        for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
            if (write_out_cfg[c]) {
                o_post_Agg[c].write(vecs[c]);
            }
        }
        ++b;
    }
    if (b != 0) {
        XF_DATABASE_ASSERT(b <= burst_len);
        o_nm_strm.write(b);
        o_nm_strm.write(0);
    } else {
        o_nm_strm.write(0);
    }
    rnm_strm.write(n);
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    for (int c = 0; c < col_num; ++c) {
        printf("write out col %d size is %d\n", c, o_post_Agg[c].size());
    }
    printf("write out nm stream size is %d\n", o_nm_strm.size());
#endif
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void burstWriteV2(hls::stream<ap_uint<elem_size * vec_len> > i_strm[col_num],
                  ap_uint<elem_size * vec_len>* ptr,
                  hls::stream<ap_uint<8> >& nm_strm,
                  hls::stream<ap_uint<32> >& rnm_strm,
                  hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    // record the burst nubmer
    unsigned bnm = 0;
    // read out the block size
    //    ap_uint<elem_size* vec_len> first_r = ptr[0];
    //    int BLOCK_SIZE = first_r(63, 32).to_int();

    // write_out_cfg, write out of bypass
    ap_uint<32> write_out_cfg = write_out_cfg_strm.read();
    ap_uint<32> nrow = write_out_cfg_strm.read();
    int BLOCK_SIZE = write_out_cfg_strm.read();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << "block size for every col=" << BLOCK_SIZE << std::endl;
#endif
    int nrow_h;
    if (write_out_cfg[31] == 1)
        nrow_h = nrow(31, 4).to_int();
    else
        nrow_h = 0;

    int col_id[col_num];
    int col_offset[col_num];
#pragma HLS array_partition variable = col_offset complete
    int wcol = 0;
    for (int i = 0; i < col_num; ++i) {
        if (write_out_cfg[i]) {
            col_id[wcol] = i;
            col_offset[wcol++] = nrow_h + BLOCK_SIZE * i + 1;
        }
    }
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    for (int k = 0; k < wcol; ++k) {
        std::cout << "col_id=" << col_id[k] << ", col_offset=" << col_offset[k] << std::endl;
    }
#endif
    //
    int nm = nm_strm.read();
    while (nm) {
        // write each col one burst
        for (int k = 0; k < wcol; ++k) {
            const int id = col_id[k];
            const int offset = col_offset[k];
        BURST_WRITE_CORE_LOOP:
            for (int n = 0; n < nm; ++n) {
#pragma HLS pipeline II = 1
                ap_uint<elem_size* vec_len> out = i_strm[id].read();
                ptr[offset + bnm * burst_len + n] = out;
            }
        }
        bnm++;
        nm = nm_strm.read();
    }
    ap_uint<32> rnm = rnm_strm.read();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
    std::cout << std::dec << "write out rnm=" << rnm.to_int() << " nrow_h*16= " << nrow_h * 16 << " col_nm=" << wcol
              << std::endl;
#endif
    ap_uint<elem_size* vec_len> first_r = 0;
    if (write_out_cfg[31] == 1)
        first_r(31, 0) = rnm + nrow_h * 16;
    else
        first_r(31, 0) = rnm;
    first_r(63, 32) = BLOCK_SIZE;
    ptr[0] = first_r;
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeDataflow(hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
                   hls::stream<bool>& e_bfr_strm,
                   hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                   hls::stream<bool>& e_post_Agg,
                   ap_uint<elem_size * vec_len>* ptr,
                   hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    const int k_fifo_buf = burst_len * 2;

    hls::stream<ap_uint<elem_size> > agg_strm[col_num];
#pragma HLS array_partition variable = agg_strm dim = 0
#pragma HLS stream variable = agg_strm depth = 32
#pragma HLS resource variable = agg_strm core = FIFO_LUTRAM
    hls::stream<bool> agg_strm_e;
#pragma HLS stream variable = agg_strm_e depth = 32

    hls::stream<ap_uint<elem_size * vec_len> > m_post_Agg[col_num];
#pragma HLS array_partition variable = m_post_Agg dim = 0
#pragma HLS stream variable = m_post_Agg depth = k_fifo_buf
#pragma HLS resource variable = m_post_Agg core = FIFO_LUTRAM
    hls::stream<ap_uint<8> > nm_strm;
#pragma HLS stream variable = nm_strm depth = 2
    hls::stream<ap_uint<32> > rnm_strm;
#pragma HLS stream variable = rnm_strm depth = 4
    hls::stream<ap_uint<32> > mid_wr_cfg_strm;
#pragma HLS stream variable = mid_wr_cfg_strm depth = 1

#pragma HLS dataflow

    align512<elem_size, vec_len, col_num>(bfr_strm, e_bfr_strm, post_Agg, e_post_Agg, agg_strm, agg_strm_e);

    countForBurstV2<burst_len, elem_size, vec_len, col_num>(agg_strm, agg_strm_e, m_post_Agg, nm_strm, rnm_strm,
                                                            write_out_cfg_strm, mid_wr_cfg_strm);

#if !defined(__SYNTHESIS__) && XDEBUG == 1
    printf("\npost_Agg: %ld, e_post_Agg:%ld, m_post_Agg:%ld, nm_strm:%ld, rnm_strm:%ld\n\n", post_Agg[0].size(),
           e_post_Agg.size(), m_post_Agg[0].size(), nm_strm.size(), rnm_strm.size());
#endif

    burstWriteV2<burst_len, elem_size, vec_len, col_num>(m_post_Agg, ptr, nm_strm, rnm_strm, mid_wr_cfg_strm);
}

template <int elem_size, int vec_len, int col_num>
void writePrepare(ap_uint<elem_size * vec_len>* ptr,
                  hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
                  hls::stream<bool>& e_bfr_strm,
                  hls::stream<ap_uint<32> >& write_cfg_strm,
                  hls::stream<ap_uint<32> >& cfg_strm) {
    ap_uint<elem_size* vec_len> first_r = ptr[0];
    int BLOCK_SIZE = first_r(63, 32).to_int();
    int nrow_h = first_r(31, 4).to_int();
    ap_uint<4> nrow_l = first_r(3, 0).to_int();
    ap_uint<32> lft[vec_len][col_num];
#pragma HLS array_partition variable = lft dim = 1

    ap_uint<32> cfg_r = write_cfg_strm.read();
    cfg_strm.write(cfg_r);
    cfg_strm.write(first_r(31, 0));
    cfg_strm.write(first_r(63, 32));

    if (cfg_r[31] == 1) {
        for (int i = 0; i < col_num; i++) {
#pragma HLS pipeline II = 1
            ap_uint<512> tmp = ptr[nrow_h + BLOCK_SIZE * i + 1];
            for (int j = 0; j < vec_len; j++) {
#pragma HLS UNROLL
                lft[j][i] = tmp.range(32 * (j + 1) - 1, 32 * j);
            }
        }

        for (int j = 0; j < nrow_l; j++) {
#pragma HLS pipeline II = 1
            for (int i = 0; i < col_num; i++) {
#pragma HLS UNROLL
                bfr_strm[i].write(lft[j][i]);
            }
            e_bfr_strm.write(false);
        }
    }
    e_bfr_strm.write(true);
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeTableV2(hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                  hls::stream<bool>& e_post_Agg,
                  ap_uint<elem_size * vec_len>* ptr,
                  hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    hls::stream<ap_uint<elem_size> > bfr_strm[col_num];
#pragma HLS array_partition variable = bfr_strm dim = 0
#pragma HLS stream variable = bfr_strm depth = 32
#pragma HLS resource variable = bfr_strm core = FIFO_LUTRAM
    hls::stream<bool> e_bfr_strm;
#pragma HLS stream variable = e_bfr_strm depth = 32

    hls::stream<ap_uint<32> > cfg_strm;
#pragma HLS resource variable = cfg_strm core = FIFO_LUTRAM
#pragma HLS stream variable = cfg_strm depth = 32

    writePrepare<elem_size, vec_len, col_num>(ptr, bfr_strm, e_bfr_strm, write_out_cfg_strm, cfg_strm);

    writeDataflow<burst_len, elem_size, vec_len, col_num>(bfr_strm, e_bfr_strm, post_Agg, e_post_Agg, ptr, cfg_strm);
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif
