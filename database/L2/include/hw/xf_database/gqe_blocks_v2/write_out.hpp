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
#ifndef GQE_ISV_WRITE_OUT_HPP
#define GQE_ISV_WRITE_OUT_HPP

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#include <ap_int.h>
#include <hls_stream.h>

#include "xf_database/utils.hpp"
#include "xf_utils_hw/stream_shuffle.hpp"

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
            col_offset[wcol++] = BLOCK_SIZE * i;
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
    ptr[16 * BLOCK_SIZE - 1] = first_r;
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

// one time burst write for 1 col
template <int elem_size, int vec_len>
void bwrite(const int nm,
            const int bnm,
            const int burst_len,
            hls::stream<ap_uint<elem_size * vec_len> >& i_strm,
            ap_uint<elem_size * vec_len>* ptr) {
#pragma HLS inline off
    for (int n = 0; n < nm; ++n) {
#pragma HLS pipeline II = 1
        ap_uint<elem_size* vec_len> out = i_strm.read();
        ptr[bnm * burst_len + n] = out;
    }
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void burstWriteAggr(hls::stream<ap_uint<elem_size * vec_len> > i_strm[col_num],
                    ap_uint<elem_size * vec_len>* ptr0,
                    ap_uint<elem_size * vec_len>* ptr1,
                    ap_uint<elem_size * vec_len>* ptr2,
                    ap_uint<elem_size * vec_len>* ptr3,
                    ap_uint<elem_size * vec_len>* ptr4,
                    ap_uint<elem_size * vec_len>* ptr5,
                    ap_uint<elem_size * vec_len>* ptr6,
                    ap_uint<elem_size * vec_len>* ptr7,
                    ap_uint<elem_size * vec_len>* ptr8,
                    ap_uint<elem_size * vec_len>* ptr9,
                    ap_uint<elem_size * vec_len>* ptr10,
                    ap_uint<elem_size * vec_len>* ptr11,
                    ap_uint<elem_size * vec_len>* ptr12,
                    ap_uint<elem_size * vec_len>* ptr13,
                    ap_uint<elem_size * vec_len>* ptr14,
                    ap_uint<elem_size * vec_len>* ptr15,
                    ap_uint<512>* tout_meta,
                    hls::stream<ap_uint<8> >& nm_strm,
                    hls::stream<ap_uint<32> >& rnm_strm,
                    hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    // record the burst nubmer
    unsigned bnm = 0;
    // write_out_cfg, write out of bypass
    ap_uint<32> write_out_cfg = write_out_cfg_strm.read();

    int nm = nm_strm.read();
    while (nm) {
        // write each col one burst
        if (write_out_cfg[0]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[0], ptr0);
        if (write_out_cfg[1]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[1], ptr1);
        if (write_out_cfg[2]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[2], ptr2);
        if (write_out_cfg[3]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[3], ptr3);
        if (write_out_cfg[4]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[4], ptr4);
        if (write_out_cfg[5]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[5], ptr5);
        if (write_out_cfg[6]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[6], ptr6);
        if (write_out_cfg[7]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[7], ptr7);
        if (write_out_cfg[8]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[8], ptr8);
        if (write_out_cfg[9]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[9], ptr9);
        if (write_out_cfg[10]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[10], ptr10);
        if (write_out_cfg[11]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[11], ptr11);
        if (write_out_cfg[12]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[12], ptr12);
        if (write_out_cfg[13]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[13], ptr13);
        if (write_out_cfg[14]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[14], ptr14);
        if (write_out_cfg[15]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[15], ptr15);
        bnm++;
        nm = nm_strm.read();
    }

    ap_uint<32> rnm = rnm_strm.read();
#ifndef __SYNTHESIS__
    std::cout << std::dec << "write out row=" << rnm.to_int() << std::endl;
#endif
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
    std::cout << std::dec << "write out row=" << rnm.to_int() << " col_nm=" << wcol << std::endl;
#endif
    // write number of resulting rows to metaout
    tout_meta[0].range(71, 8) = rnm;
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeTableAggr(hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                    hls::stream<bool>& e_post_Agg,
                    ap_uint<elem_size * vec_len>* ptr0,
                    ap_uint<elem_size * vec_len>* ptr1,
                    ap_uint<elem_size * vec_len>* ptr2,
                    ap_uint<elem_size * vec_len>* ptr3,
                    ap_uint<elem_size * vec_len>* ptr4,
                    ap_uint<elem_size * vec_len>* ptr5,
                    ap_uint<elem_size * vec_len>* ptr6,
                    ap_uint<elem_size * vec_len>* ptr7,
                    ap_uint<elem_size * vec_len>* ptr8,
                    ap_uint<elem_size * vec_len>* ptr9,
                    ap_uint<elem_size * vec_len>* ptr10,
                    ap_uint<elem_size * vec_len>* ptr11,
                    ap_uint<elem_size * vec_len>* ptr12,
                    ap_uint<elem_size * vec_len>* ptr13,
                    ap_uint<elem_size * vec_len>* ptr14,
                    ap_uint<elem_size * vec_len>* ptr15,
                    ap_uint<512>* tout_meta,
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

    burstWriteAggr<burst_len, elem_size, vec_len, col_num>(m_post_Agg, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7,
                                                           ptr8, ptr9, ptr10, ptr11, ptr12, ptr13, ptr14, ptr15,
                                                           tout_meta, nm_strm, rnm_strm, mid_wr_cfg_strm);
}

template <int elem_size, int vec_len, int col_num>
void align512(bool& build_probe_flag,
              hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
              hls::stream<bool>& e_bfr_strm,
              hls::stream<ap_uint<elem_size> > post_Agg[col_num],
              hls::stream<bool>& e_post_Agg,
              hls::stream<ap_uint<elem_size> > agg_strm[col_num],
              hls::stream<bool>& agg_strm_e) {
    if (build_probe_flag) {
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
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void countForBurstV2(bool& build_probe_flag,
                     hls::stream<ap_uint<elem_size> > i_post_Agg[col_num],
                     hls::stream<bool>& i_e_strm,
                     hls::stream<ap_uint<elem_size * vec_len> > o_post_Agg[col_num],
                     hls::stream<ap_uint<8> >& o_nm_strm,
                     hls::stream<ap_uint<32> >& rnm_strm,
                     hls::stream<ap_uint<32> >& wr_cfg_istrm,
                     hls::stream<ap_uint<32> >& wr_cfg_ostrm) {
    if (build_probe_flag) {
        const int sz = elem_size;
        bool e = i_e_strm.read();
        ap_uint<32> write_out_cfg = wr_cfg_istrm.read();
        wr_cfg_ostrm.write(write_out_cfg);
        wr_cfg_ostrm.write(wr_cfg_istrm.read());
//        wr_cfg_ostrm.write(wr_cfg_istrm.read());
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

        // write number of resulting rows to metaout
        // tout_meta[0].range(71, 8) = n;
        rnm_strm.write(n);

#if !defined(__SYNTHESIS__) && XDEBUG == 1
        for (int c = 0; c < col_num; ++c) {
            printf("write out col %d size is %d\n", c, o_post_Agg[c].size());
        }
        printf("write out nm stream size is %d\n", o_nm_strm.size());
#endif
    }
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void burstWriteV2(bool& build_probe_flag,
                  hls::stream<ap_uint<elem_size * vec_len> > i_strm[col_num],
                  ap_uint<elem_size * vec_len>* ptr,
                  hls::stream<ap_uint<8> >& nm_strm,
                  hls::stream<ap_uint<32> >& rnm_strm,
                  hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    if (build_probe_flag) {
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
                col_offset[wcol++] = nrow_h + BLOCK_SIZE * i;
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
        ptr[BLOCK_SIZE * 8 - 1] = first_r;
    }
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeDataflow(bool& build_probe_flag,
                   hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
                   hls::stream<bool>& e_bfr_strm,
                   hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                   hls::stream<bool>& e_post_Agg,
                   ap_uint<elem_size * vec_len>* ptr,
                   ap_uint<512>* tout_meta,
                   hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    const int k_fifo_buf = burst_len * 2;

    hls::stream<ap_uint<elem_size> > agg_strm[col_num];
#pragma HLS stream variable = agg_strm depth = 32
#pragma HLS resource variable = agg_strm core = FIFO_LUTRAM
    hls::stream<bool> agg_strm_e;
#pragma HLS stream variable = agg_strm_e depth = 32

    hls::stream<ap_uint<elem_size * vec_len> > m_post_Agg[col_num];
#pragma HLS stream variable = m_post_Agg depth = k_fifo_buf
#pragma HLS resource variable = m_post_Agg core = FIFO_LUTRAM
    hls::stream<ap_uint<8> > nm_strm;
#pragma HLS stream variable = nm_strm depth = 2
    hls::stream<ap_uint<32> > mid_wr_cfg_strm;
#pragma HLS stream variable = mid_wr_cfg_strm depth = 1

#pragma HLS dataflow

    align512<elem_size, vec_len, col_num>(build_probe_flag, bfr_strm, e_bfr_strm, post_Agg, e_post_Agg, agg_strm,
                                          agg_strm_e);

    countForBurstV2<burst_len, elem_size, vec_len, col_num>(build_probe_flag, agg_strm, agg_strm_e, m_post_Agg, nm_strm,
                                                            write_out_cfg_strm, mid_wr_cfg_strm, tout_meta);

#if !defined(__SYNTHESIS__) && XDEBUG == 1
    printf("\npost_Agg: %ld, e_post_Agg:%ld, m_post_Agg:%ld, nm_strm:%ld, rnm_strm:%ld\n\n", post_Agg[0].size(),
           e_post_Agg.size(), m_post_Agg[0].size(), nm_strm.size(), rnm_strm.size());
#endif

    burstWriteV2<burst_len, elem_size, vec_len, col_num>(build_probe_flag, m_post_Agg, ptr, nm_strm, mid_wr_cfg_strm);
}

template <int elem_size, int vec_len, int col_num>
void writePrepare(bool& build_probe_flag,
                  hls::stream<bool>& e_post_Agg,
                  ap_uint<elem_size * vec_len>* ptr,
                  ap_uint<512>* tout_meta,
                  hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
                  hls::stream<bool>& e_bfr_strm,
                  hls::stream<ap_uint<32> >& write_cfg_strm,
                  hls::stream<ap_uint<32> >& cfg_strm) {
    //  ap_uint<elem_size* vec_len> first_r = ptr[0];
    // int BLOCK_SIZE = first_r(63, 32).to_int();
    // int nrow_h = first_r(31, 4).to_int();
    // ap_uint<4> nrow_l = first_r(3, 0).to_int();
    ap_uint<32> nrow = tout_meta[0].range(71, 8); // length is casted from 64bits to 32bits
    int BLOCK_SIZE = nrow.range(31, 4);
    int nrow_h = BLOCK_SIZE;
    ap_uint<4> nrow_l = nrow.range(3, 0);

    ap_uint<32> lft[vec_len][col_num];
#pragma HLS array_partition variable = lft dim = 1

    ap_uint<32> cfg_r = write_cfg_strm.read();
    if (build_probe_flag) {
        cfg_strm.write(cfg_r);
        // cfg_strm.write(first_r(31, 0));
        // cfg_strm.write(first_r(63, 32));
        cfg_strm.write(nrow);
        cfg_strm.write(BLOCK_SIZE);

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
    } else {
        e_post_Agg.read();
    }
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeTableV2(bool& build_probe_flag,
                  hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                  hls::stream<bool>& e_post_Agg,
                  ap_uint<elem_size * vec_len>* ptr,
                  ap_uint<512>* tout_meta,
                  hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    hls::stream<ap_uint<elem_size> > bfr_strm[col_num];
#pragma HLS stream variable = bfr_strm depth = 32
#pragma HLS resource variable = bfr_strm core = FIFO_LUTRAM
    hls::stream<bool> e_bfr_strm;
#pragma HLS stream variable = e_bfr_strm depth = 32

    hls::stream<ap_uint<32> > cfg_strm;
#pragma HLS resource variable = cfg_strm core = FIFO_LUTRAM
#pragma HLS stream variable = cfg_strm depth = 32

    writePrepare<elem_size, vec_len, col_num>(build_probe_flag, e_post_Agg, ptr, tout_meta, bfr_strm, e_bfr_strm,
                                              write_out_cfg_strm, cfg_strm);

    writeDataflow<burst_len, elem_size, vec_len, col_num>(build_probe_flag, bfr_strm, e_bfr_strm, post_Agg, e_post_Agg,
                                                          ptr, tout_meta, cfg_strm);
}

// get the last row(512 aligned) data from each column buffer
template <int vec_len>
void get_left_dat(const int nrow_h, ap_uint<512>* ptr, ap_uint<32>* left_data) {
#pragma HLS inline off
    ap_uint<512> tmp = ptr[nrow_h + 1];
    for (int i = 0; i < vec_len; ++i) {
#pragma HLS pipeline II = 1
        // left_data[i] = tmp.range(32 * (i + 1) - 1, 32 * i);
        left_data[i] = tmp.range(31, 0);
        tmp.range(479, 0) = tmp.range(511, 32);
    }
}

template <int elem_size, int vec_len, int col_num>
void writePrepareMeta(bool build_probe_flag,
                      hls::stream<bool>& e_post_Agg,
                      ap_uint<elem_size * vec_len>* ptr0,
                      ap_uint<elem_size * vec_len>* ptr1,
                      ap_uint<elem_size * vec_len>* ptr2,
                      ap_uint<elem_size * vec_len>* ptr3,
                      ap_uint<elem_size * vec_len>* ptr4,
                      ap_uint<elem_size * vec_len>* ptr5,
                      ap_uint<elem_size * vec_len>* ptr6,
                      ap_uint<elem_size * vec_len>* ptr7,
                      ap_uint<512>* tout_meta,
                      hls::stream<bool>& o_bp_flag_strm,
                      hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
                      hls::stream<bool>& e_bfr_strm,
                      hls::stream<ap_uint<32> >& write_cfg_strm,
                      hls::stream<ap_uint<32> >& cfg_strm,
                      hls::stream<ap_uint<8 * 8> >& shuffle5_cfg_strm) {
    //  ap_uint<elem_size* vec_len> first_r = ptr[0];
    // int BLOCK_SIZE = first_r(63, 32).to_int();
    // int nrow_h = first_r(31, 4).to_int();
    // ap_uint<4> nrow_l = first_r(3, 0).to_int();
    o_bp_flag_strm.write(build_probe_flag);
    ap_uint<32> nrow = tout_meta[0].range(71, 8); // length is casted from 64bits to 32bits
    int BLOCK_SIZE = nrow.range(31, 4);
    int nrow_h = BLOCK_SIZE;
    ap_uint<4> nrow_l = nrow.range(3, 0);

    ap_uint<32> lft[col_num][vec_len];

    ap_uint<32> cfg_r = write_cfg_strm.read();
    if (build_probe_flag) {
        // the real output buffer idx is shuffled by shuffle5_cfg_strm
        ap_uint<64> shuffle_out = 0;
        for (int i = 0; i < 8; i++) {
            shuffle_out.range(i * 8 + 7, i * 8) = -1;
        }
        for (int i = 0; i < 8; i++) {
            ap_int<8> meta_idx = tout_meta[i].range(79, 72);

            // std::cout << "meta_idx == " << meta_idx << std::endl;
            if (meta_idx >= 0) {
                shuffle_out.range(meta_idx * 8 + 7, meta_idx * 8) = i;
            }
            // std::cout << "shuffle_out[" << i << "] = " << (int)shuffle_out.range(meta_idx * 8 + 7, meta_idx * 8) <<
            // std::endl;
        }
        shuffle5_cfg_strm.write(shuffle_out);

        cfg_strm.write(cfg_r);
        // cfg_strm.write(first_r(31, 0));
        // cfg_strm.write(first_r(63, 32));
        cfg_strm.write(nrow);
        // cfg_strm.write(BLOCK_SIZE);

        if (cfg_r[31] == 1) {
#pragma HLS allocation instances = get_left_dat limit = 1 function
            get_left_dat<vec_len>(nrow_h, ptr0, lft[0]);
            get_left_dat<vec_len>(nrow_h, ptr1, lft[1]);
            get_left_dat<vec_len>(nrow_h, ptr2, lft[2]);
            get_left_dat<vec_len>(nrow_h, ptr3, lft[3]);
            get_left_dat<vec_len>(nrow_h, ptr4, lft[4]);
            get_left_dat<vec_len>(nrow_h, ptr5, lft[5]);
            get_left_dat<vec_len>(nrow_h, ptr6, lft[6]);
            get_left_dat<vec_len>(nrow_h, ptr7, lft[7]);

            for (int j = 0; j < nrow_l; j++) {
                for (int i = 0; i < col_num; i++) {
#pragma HLS pipeline II = 1
                    bfr_strm[i].write(lft[i][j]);
                }
                e_bfr_strm.write(false);
            }
        }
        e_bfr_strm.write(true);
    } else {
        e_post_Agg.read();
    }
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void burstWriteV2Meta(bool& build_probe_flag,
                      hls::stream<ap_uint<elem_size * vec_len> > i_strm[col_num],
                      ap_uint<elem_size * vec_len>* ptr0,
                      ap_uint<elem_size * vec_len>* ptr1,
                      ap_uint<elem_size * vec_len>* ptr2,
                      ap_uint<elem_size * vec_len>* ptr3,
                      ap_uint<elem_size * vec_len>* ptr4,
                      ap_uint<elem_size * vec_len>* ptr5,
                      ap_uint<elem_size * vec_len>* ptr6,
                      ap_uint<elem_size * vec_len>* ptr7,
                      ap_uint<512>* tout_meta,
                      hls::stream<ap_uint<8> >& nm_strm,
                      hls::stream<ap_uint<32> >& rnm_strm,
                      hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    if (build_probe_flag) {
        // record the burst nubmer
        unsigned bnm = 0;
        // read out the block size
        //    ap_uint<elem_size* vec_len> first_r = ptr[0];
        //    int BLOCK_SIZE = first_r(63, 32).to_int();

        // write_out_cfg, write out of bypass
        ap_uint<32> write_out_cfg = write_out_cfg_strm.read();
        ap_uint<32> nrow = write_out_cfg_strm.read();
#if !defined(__SYNTHESIS__) && XDEBUG == 1
        std::cout << "block size for every col=" << BLOCK_SIZE << std::endl;
#endif
        int nrow_h;
        if (write_out_cfg[31] == 1)
            nrow_h = nrow(31, 4).to_int();
        else
            nrow_h = 0;

        int nm = nm_strm.read();
        while (nm) {
            // write each col one burst
            if (write_out_cfg[0]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[0], ptr0);
            if (write_out_cfg[1]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[1], ptr1);
            if (write_out_cfg[2]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[2], ptr2);
            if (write_out_cfg[3]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[3], ptr3);
            if (write_out_cfg[4]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[4], ptr4);
            if (write_out_cfg[5]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[5], ptr5);
            if (write_out_cfg[6]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[6], ptr6);
            if (write_out_cfg[7]) bwrite<elem_size, vec_len>(nm, bnm, burst_len, i_strm[7], ptr7);
            bnm++;
            nm = nm_strm.read();
        }

        ap_uint<32> rnm = rnm_strm.read();
        // write number of resulting rows to metaout
        tout_meta[0].range(71, 8) = rnm;

#if !defined(__SYNTHESIS__) && XDEBUG == 1
        std::cout << std::hex << "write out config" << write_out_cfg << std::endl;
        std::cout << std::dec << "write out nrow_h*16= " << nrow_h * 16 << std::endl;
#endif
    }
}

template <int elem_size, int col_num>
void shuffle5_wrapper(bool& build_probe_flag,
                      hls::stream<ap_uint<8 * 8> >& shuffle5_cfg_strm,
                      hls::stream<ap_uint<elem_size> > i_strm[col_num],
                      hls::stream<ap_uint<elem_size> > o_strm[col_num],
                      hls::stream<bool>& i_e_strm,
                      hls::stream<bool>& o_e_strm) {
    if (build_probe_flag) {
        xf::common::utils_hw::streamShuffle<8, 8>(shuffle5_cfg_strm, i_strm, i_e_strm, o_strm, o_e_strm);
    }
}
template <int burst_len, int elem_size, int vec_len, int col_num>
void writeDataflowMeta(hls::stream<bool>& bp_flag_strm,
                       hls::stream<ap_uint<elem_size> > bfr_strm[col_num],
                       hls::stream<bool>& e_bfr_strm,
                       hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                       hls::stream<bool>& e_post_Agg,
                       ap_uint<elem_size * vec_len>* ptr0,
                       ap_uint<elem_size * vec_len>* ptr1,
                       ap_uint<elem_size * vec_len>* ptr2,
                       ap_uint<elem_size * vec_len>* ptr3,
                       ap_uint<elem_size * vec_len>* ptr4,
                       ap_uint<elem_size * vec_len>* ptr5,
                       ap_uint<elem_size * vec_len>* ptr6,
                       ap_uint<elem_size * vec_len>* ptr7,
                       ap_uint<512>* tout_meta,
                       hls::stream<ap_uint<32> >& write_out_cfg_strm,
                       hls::stream<ap_uint<64> >& shuffle5_cfg_strm) {
    const int k_fifo_buf = burst_len * 2;

    hls::stream<ap_uint<elem_size> > agg_strm[col_num];
#pragma HLS stream variable = agg_strm depth = 32
#pragma HLS resource variable = agg_strm core = FIFO_LUTRAM
    hls::stream<bool> agg_strm_e;
#pragma HLS stream variable = agg_strm_e depth = 32

    hls::stream<ap_uint<elem_size> > agg_strm_shuffle[col_num];
#pragma HLS stream variable = agg_strm_shuffle depth = 32
#pragma HLS resource variable = agg_strm_shuffle core = FIFO_LUTRAM
    hls::stream<bool> agg_strm_shuffle_e;
#pragma HLS stream variable = agg_strm_shuffle_e depth = 32

    hls::stream<ap_uint<elem_size * vec_len> > m_post_Agg[col_num];
#pragma HLS stream variable = m_post_Agg depth = k_fifo_buf
#pragma HLS resource variable = m_post_Agg core = FIFO_LUTRAM

    hls::stream<ap_uint<8> > nm_strm;
#pragma HLS stream variable = nm_strm depth = 2

    hls::stream<ap_uint<32> > rnm_strm;
#pragma HLS stream variable = rnm_strm depth = 2

    hls::stream<ap_uint<32> > mid_wr_cfg_strm;
#pragma HLS stream variable = mid_wr_cfg_strm depth = 2

#pragma HLS dataflow

    bool build_probe_flag = bp_flag_strm.read();

    align512<elem_size, vec_len, col_num>(build_probe_flag, bfr_strm, e_bfr_strm, post_Agg, e_post_Agg, agg_strm,
                                          agg_strm_e);

    shuffle5_wrapper<elem_size, col_num>(build_probe_flag, shuffle5_cfg_strm, agg_strm, agg_strm_shuffle, agg_strm_e,
                                         agg_strm_shuffle_e);

    countForBurstV2<burst_len, elem_size, vec_len, col_num>(build_probe_flag, agg_strm_shuffle, agg_strm_shuffle_e,
                                                            m_post_Agg, nm_strm, rnm_strm, write_out_cfg_strm,
                                                            mid_wr_cfg_strm);

#if !defined(__SYNTHESIS__) && XDEBUG == 1
    printf("\npost_Agg: %ld, e_post_Agg:%ld, m_post_Agg:%ld, nm_strm:%ld, rnm_strm:%ld\n\n", post_Agg[0].size(),
           e_post_Agg.size(), m_post_Agg[0].size(), nm_strm.size(), rnm_strm.size());
#endif
    burstWriteV2Meta<burst_len, elem_size, vec_len, col_num>(build_probe_flag, m_post_Agg, ptr0, ptr1, ptr2, ptr3, ptr4,
                                                             ptr5, ptr6, ptr7, tout_meta, nm_strm, rnm_strm,
                                                             mid_wr_cfg_strm);
}

template <int burst_len, int elem_size, int vec_len, int col_num>
void writeTableMeta(bool build_probe_flag,
                    hls::stream<ap_uint<elem_size> > post_Agg[col_num],
                    hls::stream<bool>& e_post_Agg,
                    ap_uint<elem_size * vec_len>* ptr0,
                    ap_uint<elem_size * vec_len>* ptr1,
                    ap_uint<elem_size * vec_len>* ptr2,
                    ap_uint<elem_size * vec_len>* ptr3,
                    ap_uint<elem_size * vec_len>* ptr4,
                    ap_uint<elem_size * vec_len>* ptr5,
                    ap_uint<elem_size * vec_len>* ptr6,
                    ap_uint<elem_size * vec_len>* ptr7,
                    ap_uint<512>* tout_meta,
                    hls::stream<ap_uint<32> >& write_out_cfg_strm) {
    hls::stream<ap_uint<elem_size> > bfr_strm[col_num];
#pragma HLS stream variable = bfr_strm depth = 32
#pragma HLS resource variable = bfr_strm core = FIFO_LUTRAM
    hls::stream<bool> e_bfr_strm;
#pragma HLS stream variable = e_bfr_strm depth = 32

    hls::stream<ap_uint<32> > cfg_strm;
#pragma HLS resource variable = cfg_strm core = FIFO_LUTRAM
#pragma HLS stream variable = cfg_strm depth = 32

    hls::stream<ap_uint<64> > shuffle5_cfg_strm;
#pragma HLS stream variable = shuffle5_cfg_strm depth = 2

    hls::stream<bool> bp_flag_strm;
#pragma HLS stream variable = bp_flag_out_strm depth = 2

    writePrepareMeta<elem_size, vec_len, col_num>(build_probe_flag, e_post_Agg, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5,
                                                  ptr6, ptr7, tout_meta, bp_flag_strm, bfr_strm, e_bfr_strm,
                                                  write_out_cfg_strm, cfg_strm, shuffle5_cfg_strm);

    writeDataflowMeta<burst_len, elem_size, vec_len, col_num>(bp_flag_strm, bfr_strm, e_bfr_strm, post_Agg, e_post_Agg,
                                                              ptr0, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7, tout_meta,
                                                              cfg_strm, shuffle5_cfg_strm);
}

// for part
// count and prepare data for burst write
template <int elem_size, int vec_len, int col_num, int hash_wh>
void countForBurstPart(hls::stream<ap_uint<elem_size*(1 << hash_wh)> > i_post_Agg[col_num],
                       hls::stream<int>& i_partition_size_strm,
                       hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                       hls::stream<int>& i_bit_num_strm,
                       hls::stream<ap_uint<10> >& i_nm_strm,
                       hls::stream<ap_uint<12> >& i_bkpu_num_strm,

                       hls::stream<ap_uint<elem_size * vec_len> > o_post_Agg[col_num],
                       hls::stream<ap_uint<8> > o_nm_512_strm[3],
                       hls::stream<ap_uint<32> > o_wr_cfg_strm[3],
                       hls::stream<int> o_p_base_addr_strm[3],
                       hls::stream<int> o_burst_strm_cnt_reg_strm[3],
                       hls::stream<int>& o_per_part_nm_strm) {
    enum { PU = 1 << hash_wh };
    // read out the partition size
    int PARTITION_SIZE = i_partition_size_strm.read();

#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << "Partition size:" << PARTITION_SIZE << std::endl;
    long long cnt = 0;
#endif

    const int sz = elem_size;
    ap_uint<2> pu_idx;
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
    o_wr_cfg_strm[0].write(write_out_cfg);
    o_wr_cfg_strm[1].write(write_out_cfg);
    o_wr_cfg_strm[2].write(write_out_cfg);
    const int bit_num_org = i_bit_num_strm.read();
    // get the partition num
    const int partition_num = 1 << bit_num_org;
    // get the bit num after minus PU idx 2 bits
    const int bit_num = bit_num_org - hash_wh;
    // get the part num in each PU
    const int part_num_per_PU = 1 << bit_num;

    ap_uint<10> nm = i_nm_strm.read();
    while (nm != 0) {
        (pu_idx, bk_idx) = i_bkpu_num_strm.read();

        // the partition idx among all PUs
        ap_uint<16> location = pu_idx * part_num_per_PU + bk_idx;

        // get write ddr base addr
        int p_base_addr = PARTITION_SIZE * location;
        o_p_base_addr_strm[0].write(p_base_addr);
        o_p_base_addr_strm[1].write(p_base_addr);
        o_p_base_addr_strm[2].write(p_base_addr);

        // get the offset after nm times of write
        // int burst_step_cnt_reg = nrow_cnt[location] / vec_len;
        int burst_step_cnt_reg = (nrow_cnt[location] + vec_len - 1) / vec_len;
        o_burst_strm_cnt_reg_strm[0].write(burst_step_cnt_reg);
        o_burst_strm_cnt_reg_strm[1].write(burst_step_cnt_reg);
        o_burst_strm_cnt_reg_strm[2].write(burst_step_cnt_reg);

        // convert dat number from nm * 32bit to nm/16 * 512bit
        const ap_uint<8> burst_len = (nm + vec_len - 1) / vec_len;
        for (int i = 0; i < (nm + PU - 1) / PU; i++) {
#pragma HLS pipeline II = 1
            for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                ap_uint<sz* PU> t = i_post_Agg[c].read();

                vecs[c].range(sz * PU * (vec_len / PU - 1) - 1, 0) = vecs[c].range(sz * vec_len - 1, sz * PU);
                vecs[c].range(sz * vec_len - 1, sz * PU * (vec_len / PU - 1)) = t;
            }

            if (r == vec_len - PU) {
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    if (write_out_cfg[c]) o_post_Agg[c].write(vecs[c]);
                }
                if (b == burst_len - 1) {
                    o_nm_512_strm[0].write(burst_len);
                    o_nm_512_strm[1].write(burst_len);
                    o_nm_512_strm[2].write(burst_len);
                    b = 0;
                } else {
                    ++b;
                }
                r = 0;
            } else {
                r = r + PU;
            }
        }

        if (r != 0) {
            // handle incomplete vecs
            for (; r < vec_len; r = r + PU) {
                for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                    vecs[c].range(sz * PU * (vec_len / PU - 1) - 1, 0) = vecs[c].range(sz * vec_len - 1, sz * PU);
                    vecs[c].range(sz * vec_len - 1, sz * PU * (vec_len / PU - 1)) = 0;
                }
            }
            for (int c = 0; c < col_num; ++c) {
#pragma HLS unroll
                if (write_out_cfg[c]) o_post_Agg[c].write(vecs[c]);
            }
            ++b;
        }

        if (b != 0) {
            o_nm_512_strm[0].write(b);
            o_nm_512_strm[1].write(b);
            o_nm_512_strm[2].write(b);
        }

        b = 0;
        r = 0;

        nrow_cnt[location] += nm;

        nm = i_nm_strm.read();
    }

    o_nm_512_strm[0].write(0);
    o_nm_512_strm[1].write(0);
    o_nm_512_strm[2].write(0);
    // output nrow of each partition
    for (int i = 0; i < partition_num; i++) {
#pragma HLS PIPELINE II = 1
        o_per_part_nm_strm.write(nrow_cnt[i]);
    }
}

// one time burst write for 1 col
template <int elem_size, int vec_len>
void bwrite_part(ap_uint<8> nm,
                 const int part_base_addr,
                 const int burst_step_cnt_reg,
                 hls::stream<ap_uint<elem_size * vec_len> >& i_strm,
                 ap_uint<elem_size * vec_len>* ptr) {
    for (int n = 0; n < nm; ++n) {
#pragma HLS PIPELINE II = 1
        ap_uint<elem_size* vec_len> out = i_strm.read();
        ptr[part_base_addr + burst_step_cnt_reg + n] = out;
    }
}
// for part, burst write to col 0,3,6, using axi0
template <int elem_size, int vec_len, int col_num>
void burstwrite_col036(hls::stream<ap_uint<elem_size * vec_len> >& i_strm0,
                       hls::stream<ap_uint<elem_size * vec_len> >& i_strm1,
                       hls::stream<ap_uint<elem_size * vec_len> >& i_strm2,
                       hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                       hls::stream<ap_uint<8> >& i_nm_512_strm,
                       hls::stream<int>& i_p_base_addr_strm,
                       hls::stream<int>& i_burst_step_cnt_reg_strm,

                       ap_uint<elem_size * vec_len>* ptr0,
                       ap_uint<elem_size * vec_len>* ptr1,
                       ap_uint<elem_size * vec_len>* ptr2) {
    ap_uint<32> write_out_cfg = i_wr_cfg_strm.read();
    ap_uint<8> nm = i_nm_512_strm.read();
    while (nm != 0) {
        int p_base_addr = i_p_base_addr_strm.read();
        int burst_step_cnt_reg = i_burst_step_cnt_reg_strm.read();

        if (write_out_cfg[0]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm0, ptr0);
        if (write_out_cfg[3]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm1, ptr1);
        if (write_out_cfg[6]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm2, ptr2);

        nm = i_nm_512_strm.read();
    }
}
// for part, burst write to col 1,4,7, using axi1
template <int elem_size, int vec_len, int col_num>
void burstwrite_col147(hls::stream<ap_uint<elem_size * vec_len> >& i_strm0,
                       hls::stream<ap_uint<elem_size * vec_len> >& i_strm1,
                       hls::stream<ap_uint<elem_size * vec_len> >& i_strm2,
                       hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                       hls::stream<ap_uint<8> >& i_nm_512_strm,
                       hls::stream<int>& i_p_base_addr_strm,
                       hls::stream<int>& i_burst_step_cnt_reg_strm,

                       ap_uint<elem_size * vec_len>* ptr0,
                       ap_uint<elem_size * vec_len>* ptr1,
                       ap_uint<elem_size * vec_len>* ptr2) {
    ap_uint<32> write_out_cfg = i_wr_cfg_strm.read();
    ap_uint<8> nm = i_nm_512_strm.read();
    while (nm != 0) {
        int p_base_addr = i_p_base_addr_strm.read();
        int burst_step_cnt_reg = i_burst_step_cnt_reg_strm.read();

        if (write_out_cfg[1]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm0, ptr0);
        if (write_out_cfg[4]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm1, ptr1);
        if (write_out_cfg[7]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm2, ptr2);

        nm = i_nm_512_strm.read();
    }
}
// for part, burst write to col 2,5, using axi2
template <int elem_size, int vec_len, int col_num>
void burstwrite_col25(hls::stream<ap_uint<elem_size * vec_len> >& i_strm0,
                      hls::stream<ap_uint<elem_size * vec_len> >& i_strm1,
                      hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                      hls::stream<ap_uint<8> >& i_nm_512_strm,
                      hls::stream<int>& i_p_base_addr_strm,
                      hls::stream<int>& i_burst_step_cnt_reg_strm,

                      ap_uint<elem_size * vec_len>* ptr0,
                      ap_uint<elem_size * vec_len>* ptr1) {
    ap_uint<32> write_out_cfg = i_wr_cfg_strm.read();
    ap_uint<8> nm = i_nm_512_strm.read();
    while (nm != 0) {
        int p_base_addr = i_p_base_addr_strm.read();
        int burst_step_cnt_reg = i_burst_step_cnt_reg_strm.read();

        if (write_out_cfg[2]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm0, ptr0);
        if (write_out_cfg[5]) bwrite_part<elem_size, vec_len>(nm, p_base_addr, burst_step_cnt_reg, i_strm1, ptr1);

        nm = i_nm_512_strm.read();
    }
}

// write out for partition
template <int elem_size, int vec_len, int col_num, int hash_wh>
void writeTablePart(hls::stream<ap_uint<elem_size*(1 << hash_wh)> > post_Agg[col_num],
                    hls::stream<int>& i_partition_size_strm,
                    hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                    hls::stream<int>& i_bit_num_strm,
                    hls::stream<ap_uint<10> >& i_nm_strm,
                    hls::stream<ap_uint<12> >& i_bkpu_num_strm,

                    hls::stream<int>& o_per_part_nm_strm,

                    ap_uint<elem_size * vec_len>* ptr0,
                    ap_uint<elem_size * vec_len>* ptr1,
                    ap_uint<elem_size * vec_len>* ptr2,
                    ap_uint<elem_size * vec_len>* ptr3,
                    ap_uint<elem_size * vec_len>* ptr4,
                    ap_uint<elem_size * vec_len>* ptr5,
                    ap_uint<elem_size * vec_len>* ptr6,
                    ap_uint<elem_size * vec_len>* ptr7) {
    const int burst_len = BURST_LEN;
    const int k_fifo_buf = burst_len * 2;

    hls::stream<ap_uint<elem_size * vec_len> > mid_post_Agg[col_num];
#pragma HLS stream variable = mid_post_Agg depth = k_fifo_buf
#pragma HLS resource variable = mid_post_Agg core = FIFO_LUTRAM

    hls::stream<ap_uint<8> > mid_nm_512_strm[3];
#pragma HLS array_partition variable = mid_nm_512_strm complete
#pragma HLS stream variable = mid_nm_512_strm depth = 8

    hls::stream<ap_uint<32> > mid_write_cfg_strm[3];
#pragma HLS array_partition variable = mid_write_cfg_strm complete
#pragma HLS stream variable = mid_write_cfg_strm depth = 4

    hls::stream<int> mid_p_base_addr_strm[3];
#pragma HLS array_partition variable = mid_p_base_addr_strm complete
#pragma HLS stream variable = mid_p_base_addr_strm depth = 4

    hls::stream<int> mid_burst_step_cnt_reg_strm[3];
#pragma HLS array_partition variable = mid_burst_step_cnt_reg_strm complete
#pragma HLS stream variable = mid_burst_strm_cnt_reg_strm depth = 4

#pragma HLS dataflow

    countForBurstPart<elem_size, vec_len, col_num, hash_wh>(
        post_Agg, i_partition_size_strm, i_wr_cfg_strm, i_bit_num_strm, i_nm_strm, i_bkpu_num_strm, mid_post_Agg,
        mid_nm_512_strm, mid_write_cfg_strm, mid_p_base_addr_strm, mid_burst_step_cnt_reg_strm, o_per_part_nm_strm);

    burstwrite_col036<elem_size, vec_len, col_num>(mid_post_Agg[0], mid_post_Agg[3], mid_post_Agg[6],
                                                   mid_write_cfg_strm[0], mid_nm_512_strm[0], mid_p_base_addr_strm[0],
                                                   mid_burst_step_cnt_reg_strm[0], ptr0, ptr3, ptr6);
    burstwrite_col147<elem_size, vec_len, col_num>(mid_post_Agg[1], mid_post_Agg[4], mid_post_Agg[7],
                                                   mid_write_cfg_strm[1], mid_nm_512_strm[1], mid_p_base_addr_strm[1],
                                                   mid_burst_step_cnt_reg_strm[1], ptr1, ptr4, ptr7);

    burstwrite_col25<elem_size, vec_len, col_num>(mid_post_Agg[2], mid_post_Agg[5], mid_write_cfg_strm[2],
                                                  mid_nm_512_strm[2], mid_p_base_addr_strm[2],
                                                  mid_burst_step_cnt_reg_strm[2], ptr2, ptr5);
}

// write out for partition
template <int elem_size, int vec_len, int col_num, int hash_wh>
void writeTablePartWrapper(hls::stream<ap_uint<elem_size*(1 << hash_wh)> > post_Agg[col_num],
                           hls::stream<ap_uint<32> >& i_wr_cfg_strm,
                           hls::stream<int>& i_bit_num_strm,
                           hls::stream<ap_uint<10> >& i_nm_strm,
                           hls::stream<ap_uint<12> >& i_bkpu_num_strm,

                           ap_uint<elem_size * vec_len>* ptr0,
                           ap_uint<elem_size * vec_len>* ptr1,
                           ap_uint<elem_size * vec_len>* ptr2,
                           ap_uint<elem_size * vec_len>* ptr3,
                           ap_uint<elem_size * vec_len>* ptr4,
                           ap_uint<elem_size * vec_len>* ptr5,
                           ap_uint<elem_size * vec_len>* ptr6,
                           ap_uint<elem_size * vec_len>* ptr7,
                           ap_uint<512>* tout_meta) {
    hls::stream<int> mid_partition_size_strm;
#pragma HLS stream variable = mid_partition_size_strm depth = 2
    hls::stream<int> mid_bit_num_strm;
#pragma HLS stream variable = mid_bit_num_strm depth = 2
    hls::stream<int> mid_per_part_nm_strm;
#pragma HLS stream variable = mid_per_part_nm_strm depth = 256
#pragma HLS resource variable = mid_per_part_nm_strm core = FIFO_BRAM
    // read out the partition size
    int PARTITION_SIZE = tout_meta[0].range(135, 104);
    mid_partition_size_strm.write(PARTITION_SIZE);

    // read in the bit num
    const int bit_num = i_bit_num_strm.read();
    mid_bit_num_strm.write(bit_num);
    // get the partition num
    const int partition_num = 1 << bit_num;

    writeTablePart<elem_size, vec_len, col_num, hash_wh>(
        post_Agg, mid_partition_size_strm, i_wr_cfg_strm, mid_bit_num_strm, i_nm_strm, i_bkpu_num_strm,
        mid_per_part_nm_strm, ptr0, ptr1, ptr2, ptr3, ptr4, ptr5, ptr6, ptr7);
//----------update meta------------
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    int cnt = 0;
#endif
    // define the buffer used to save nrow of each partition
    ap_uint<512> metaout[16];
#pragma HLS resource variable = metaout core = RAM_S2P_BRAM
// update tout meta
FINAL_WRITE_HEAD_LOOP:
    for (int i = 0; i < partition_num; i++) {
#pragma HLS PIPELINE II = 1
        int rnm = mid_per_part_nm_strm.read();
        ap_uint<8> idx = i / 16;
        ap_uint<8> idx_ex = i % 16;
        metaout[idx].range(32 * (idx_ex + 1) - 1, 32 * idx_ex) = rnm;

// tout_meta[8 + idx].range(32 * (idx_ex + 1) - 1, 32 * idx_ex) = rnm;

#if !defined(__SYNTHESIS__) && XDEBUG == 1
        std::cout << "P" << i << "\twrite out row number = " << rnm << std::endl;
        cnt += rnm;
#endif
    }
    // write nrow of each partition to tout_meta, ddr
    for (int i = 0; i < (partition_num + 15) / 16; i++) {
#pragma HLS PIPELINE II = 1
        tout_meta[8 + i] = metaout[i];
    }
#if !defined(__SYNTHESIS__) && XDEBUG == 1
    std::cout << "Total number of write-out row: " << cnt << std::endl;
#endif
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif
