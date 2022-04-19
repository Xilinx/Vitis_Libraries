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

#include "xf_database/gqe_blocks_v3/gqe_types.hpp"
#include "xf_database/gqe_blocks_v3/gqe_traits.hpp"

#include "xf_database/gqe_blocks_v3/load_config.hpp"
#include "xf_database/gqe_blocks_v3/scan_cols.hpp"

#include "xf_database/gqe_blocks_v3/gqe_filter.hpp"

#include "xf_database/gqe_blocks_v3/demux_and_mux.hpp"

#include "xf_database/gqe_blocks_v3/multi_func_pu.hpp"

#include "xf_database/gqe_blocks_v3/write_out.hpp"

#include <ap_int.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

namespace xf {
namespace database {
namespace gqe {

// load kernel config and scan cols in
template <int CH_NM, int COL_NM, int BLEN, int GRP_SZ>
void load_cfg_and_scan(const int bucket_depth,
                       hls::burst_maxi<ap_uint<64> >& din_krn_cfg,
                       hls::burst_maxi<ap_uint<64> >& din_meta,
                       hls::burst_maxi<ap_uint<256> >& din_col0,
                       hls::burst_maxi<ap_uint<256> >& din_col1,
                       hls::burst_maxi<ap_uint<256> >& din_col2,
                       hls::burst_maxi<ap_uint<64> >& din_val,
                       hls::stream<ap_uint<8> >& general_cfg_strm,
                       hls::stream<ap_uint<3> >& join_cfg_strm,
                       hls::stream<ap_uint<36> >& bf_cfg_strm,
                       hls::stream<ap_uint<15> >& part_cfg_strm,
                       hls::stream<ap_uint<32> >& filter_cfg_strm,
                       hls::stream<int>& bit_num_strm_copy,
                       hls::stream<ap_uint<8 * TPCH_INT_SZ> > ch_strms[CH_NM][COL_NM],
                       hls::stream<bool> e_ch_strms[CH_NM],
                       hls::stream<ap_uint<8> >& write_out_cfg_strm) {
    hls::stream<int64_t> nrow_strm;
#pragma HLS stream variable = nrow_strm depth = 2
#pragma HLS bind_storage variable = nrow_strm type = fifo impl = lutram
    hls::stream<ap_uint<8> > dup_general_cfg_strm;
#pragma HLS stream variable = dup_general_cfg_strm depth = 2
#pragma HLS bind_storage variable = dup_general_cfg_strm type = fifo impl = lutram

    int secID;
    ap_uint<3> din_col_en;
    ap_uint<2> rowID_flags;

    load_config(bucket_depth, din_krn_cfg, din_meta, nrow_strm, secID, general_cfg_strm, dup_general_cfg_strm,
                join_cfg_strm, bf_cfg_strm, part_cfg_strm, bit_num_strm_copy, filter_cfg_strm, din_col_en, rowID_flags,
                write_out_cfg_strm);

    scan_cols<CH_NM, COL_NM, BLEN, GRP_SZ>(rowID_flags, nrow_strm, secID, dup_general_cfg_strm, din_col_en, din_col0,
                                           din_col1, din_col2, din_val, ch_strms, e_ch_strms);
}

} // namespace gqe
} // namespace database
} // namespace xf

extern "C" void gqeKernel(
    // input data columns
    hls::burst_maxi<ap_uint<256> > din_col0,
    hls::burst_maxi<ap_uint<256> > din_col1,
    hls::burst_maxi<ap_uint<256> > din_col2,

    // validation buffer
    hls::burst_maxi<ap_uint<64> > din_val,

    // kernel config
    hls::burst_maxi<ap_uint<64> > din_krn_cfg, // 14*512b

    // input meta buffer
    hls::burst_maxi<ap_uint<64> > din_meta, // 1*512b
    // output meta buffer
    hls::burst_maxi<ap_uint<256> > dout_meta, // 24*256b

    //  output data columns
    hls::burst_maxi<ap_uint<256> > dout_col0,
    hls::burst_maxi<ap_uint<256> > dout_col1,
    hls::burst_maxi<ap_uint<256> > dout_col2,
    hls::burst_maxi<ap_uint<256> > dout_col3,

    // hbm buffers used to save build table key/payload for JOIN,
    // lower space of hash-table for BF
    hls::burst_maxi<ap_uint<256> > htb_buf0,
    hls::burst_maxi<ap_uint<256> > htb_buf1,
    hls::burst_maxi<ap_uint<256> > htb_buf2,
    hls::burst_maxi<ap_uint<256> > htb_buf3,
    hls::burst_maxi<ap_uint<256> > htb_buf4,
    hls::burst_maxi<ap_uint<256> > htb_buf5,
    hls::burst_maxi<ap_uint<256> > htb_buf6,
    hls::burst_maxi<ap_uint<256> > htb_buf7,

    // hbm buffers used to save overflowed build table key/payload for JOIN,
    // higher space of hash-table for BF
    hls::burst_maxi<ap_uint<256> > stb_buf0,
    hls::burst_maxi<ap_uint<256> > stb_buf1,
    hls::burst_maxi<ap_uint<256> > stb_buf2,
    hls::burst_maxi<ap_uint<256> > stb_buf3,
    hls::burst_maxi<ap_uint<256> > stb_buf4,
    hls::burst_maxi<ap_uint<256> > stb_buf5,
    hls::burst_maxi<ap_uint<256> > stb_buf6,
    hls::burst_maxi<ap_uint<256> > stb_buf7) {
    enum { HBM_OUTSTANDING = 64 };
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_0 port = din_col0

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_1 port = din_col1

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_2 port = din_col2

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_3 port = din_val

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_3 port = din_krn_cfg

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = gmem0_3 port = din_meta

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem1_0 port = dout_meta

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem1_1 port = dout_col0

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem1_2 port = dout_col1

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem1_3 port = dout_col2

#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = gmem1_0 port = dout_col3

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_0 port =                      \
        htb_buf0 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_1 port =                      \
        htb_buf1 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_2 port =                      \
        htb_buf2 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_3 port =                      \
        htb_buf3 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_4 port =                      \
        htb_buf4 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_5 port =                      \
        htb_buf5 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_6 port =                      \
        htb_buf6 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem2_7 port =                      \
        htb_buf7 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_0 port =                      \
        stb_buf0 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_1 port =                      \
        stb_buf1 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_2 port =                      \
        stb_buf2 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_3 port =                      \
        stb_buf3 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_4 port =                      \
        stb_buf4 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_5 port =                      \
        stb_buf5 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_6 port =                      \
        stb_buf6 max_widen_bitwidth = 256

#pragma HLS INTERFACE m_axi offset = slave latency = 2 num_write_outstanding = HBM_OUTSTANDING num_read_outstanding = \
    HBM_OUTSTANDING max_write_burst_length = 8 max_read_burst_length = 8 bundle = gmem3_7 port =                      \
        stb_buf7 max_widen_bitwidth = 256

#pragma HLS INTERFACE s_axilite port = din_col0 bundle = control
#pragma HLS INTERFACE s_axilite port = din_col1 bundle = control
#pragma HLS INTERFACE s_axilite port = din_col2 bundle = control
#pragma HLS INTERFACE s_axilite port = din_val bundle = control

#pragma HLS INTERFACE s_axilite port = din_krn_cfg bundle = control
#pragma HLS INTERFACE s_axilite port = din_meta bundle = control
#pragma HLS INTERFACE s_axilite port = dout_meta bundle = control

#pragma HLS INTERFACE s_axilite port = dout_col0 bundle = control
#pragma HLS INTERFACE s_axilite port = dout_col1 bundle = control
#pragma HLS INTERFACE s_axilite port = dout_col2 bundle = control
#pragma HLS INTERFACE s_axilite port = dout_col3 bundle = control

#pragma HLS INTERFACE s_axilite port = htb_buf0 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf1 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf2 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf3 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf4 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf5 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf6 bundle = control
#pragma HLS INTERFACE s_axilite port = htb_buf7 bundle = control

#pragma HLS INTERFACE s_axilite port = stb_buf0 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf1 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf2 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf3 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf4 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf5 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf6 bundle = control
#pragma HLS INTERFACE s_axilite port = stb_buf7 bundle = control

#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifndef __SYNTHESIS__
    std::cout << "in gqeKernel ..............." << std::endl;
#endif

    using namespace xf::database::gqe;

#pragma HLS dataflow

    const int nch = 4;
    const int ncol = 3;
    const int nPU = 8;
    const int burstlen = 32;
    const int group_sz = 256;
    const int bucket_depth = 512;

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > ch_strms[nch][ncol];
#pragma HLS stream variable = ch_strms depth = 32
#pragma HLS bind_storage variable = ch_strms type = fifo impl = lutram
    hls::stream<bool> e_ch_strms[nch];
#pragma HLS stream variable = e_ch_strms depth = 32

    hls::stream<ap_uint<32> > filter_cfg_strm;
#pragma HLS stream variable = filter_cfg_strm depth = 128
#pragma HLS bind_storage variable = filter_cfg_strm type = fifo impl = lutram
    hls::stream<ap_uint<8> > write_out_cfg_strm;
#pragma HLS stream variable = write_out_cfg_strm depth = 2
    hls::stream<int> bit_num_strm_copy;
#pragma HLS stream variable = bit_num_strm_copy depth = 2

#ifndef __SYNTHESIS__
    printf("************************************************************\n");
    printf("             General Query Egnine Kernel\n");
    printf("************************************************************\n");
#endif

    hls::stream<ap_uint<8> > general_cfg_strm;
#pragma HLS stream variable = general_cfg_strm depth = 2
    hls::stream<ap_uint<3> > join_cfg_strm;
#pragma HLS stream variable = join_cfg_strm depth = 2
    hls::stream<ap_uint<36> > bf_cfg_strm;
#pragma HLS stream variable = bf_cfg_strm depth = 2
    hls::stream<ap_uint<15> > part_cfg_strm;
#pragma HLS stream variable = part_cfg_strm depth = 2

    // load configurations & scan columns
    load_cfg_and_scan<nch, ncol, burstlen, group_sz>(
        bucket_depth, din_krn_cfg, din_meta, din_col0, din_col1, din_col2, din_val, general_cfg_strm, join_cfg_strm,
        bf_cfg_strm, part_cfg_strm, filter_cfg_strm, bit_num_strm_copy, ch_strms, e_ch_strms, write_out_cfg_strm);

#ifndef __SYNTHESIS__
    std::cout << "after scan" << std::endl;
    for (int ch = 0; ch < nch; ch++) {
        std::cout << "e_ch_strms[" << ch << "].size(): " << e_ch_strms[ch].size() << std::endl;
        for (int c = 0; c < ncol; c++) {
            std::cout << "ch_strms[" << ch << "][" << c << "].size(): " << ch_strms[ch][c].size() << std::endl;
        }
    }
#endif

    hls::stream<ap_uint<8 * TPCH_INT_SZ> > flt_strms[nch][ncol];
#pragma HLS stream variable = flt_strms depth = 32
#pragma HLS bind_storage variable = flt_strms type = fifo impl = lutram
    hls::stream<bool> e_flt_strms[nch];
#pragma HLS stream variable = e_flt_strms depth = 32
#pragma HLS bind_storage variable = e_flt_strms type = fifo impl = srl

    // dynamic filtering
    filter_ongoing<ncol, ncol, nch>(filter_cfg_strm, ch_strms, e_ch_strms, flt_strms, e_flt_strms);

#ifndef __SYNTHESIS__
    std::cout << "after filtering" << std::endl;
    for (int ch = 0; ch < nch; ch++) {
        std::cout << "e_flt_strms[" << ch << "].size(): " << e_flt_strms[ch].size() << std::endl;
        for (int c = 0; c < ncol; c++) {
            std::cout << "flt_strms[" << ch << "][" << c << "].size(): " << flt_strms[ch][c].size() << std::endl;
        }
    }
#endif

    hls::stream<ap_uint<8> > general_cfg_pu_strm;
#pragma HLS stream variable = general_cfg_pu_strm depth = 2
#pragma HLS bind_storage variable = general_cfg_pu_strm type = fifo impl = lutram
    hls::stream<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > pu_strm[ncol];
#pragma HLS stream variable = pu_strm depth = 256
#pragma HLS bind_storage variable = pu_strm type = fifo impl = bram
    hls::stream<ap_uint<10 + Log2<nPU>::value> > hp_bkpu_strm;
#pragma HLS stream variable = hp_bkpu_strm depth = 32
    hls::stream<ap_uint<10> > pu_nm_strm;
#pragma HLS stream variable = pu_nm_strm depth = 32
    hls::stream<ap_uint<32> > nr_row_strm("nr_row_strm");
#pragma HLS stream variable = nr_row_strm depth = 2
#pragma HLS bind_storage variable = nr_row_strm type = fifo impl = lutram

    // multi-functional-PU
    multi_func_pu_wrapper<ncol, nch, nPU, 1, group_sz, HBM_OUTSTANDING>(
        general_cfg_strm, join_cfg_strm, part_cfg_strm, bf_cfg_strm, flt_strms, e_flt_strms, general_cfg_pu_strm,
        pu_strm, pu_nm_strm, nr_row_strm, hp_bkpu_strm, htb_buf0, htb_buf1, htb_buf2, htb_buf3, htb_buf4, htb_buf5,
        htb_buf6, htb_buf7, stb_buf0, stb_buf1, stb_buf2, stb_buf3, stb_buf4, stb_buf5, stb_buf6, stb_buf7);

#ifndef __SYNTHESIS__
    std::cout << "passed join" << std::endl;
    std::cout << "pu_nm_strm.size: " << pu_nm_strm.size() << std::endl;
    for (int c = 0; c < ncol; c++) {
        std::cout << "pu_strm[" << c << "].size(): " << pu_strm[c].size() << std::endl;
    }
#endif

    // write out
    write_table_out<burstlen, 8 * TPCH_INT_SZ, VEC_LEN, ncol, Log2<nPU>::value, group_sz>(
        general_cfg_pu_strm, write_out_cfg_strm, pu_strm, pu_nm_strm, hp_bkpu_strm, bit_num_strm_copy, nr_row_strm,

        dout_col0, dout_col1, dout_col2, dout_col3, dout_meta);
}
