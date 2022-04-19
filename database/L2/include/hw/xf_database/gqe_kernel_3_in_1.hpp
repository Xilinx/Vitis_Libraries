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
#ifndef _XF_DB_GQE_3_IN_1_H_
#define _XF_DB_GQE_3_IN_1_H_

/**
 * @file gqe_kernel_3_in_1.hpp
 * @brief interface of 3-in-1 GQE kernel.
 */

#include <ap_int.h>
#include <hls_burst_maxi.h>
#include "xf_database/gqe_blocks_v3/gqe_types.hpp"

/**
 * @breif 3-in-1 GQE kernel (64-bit key version)
 *
 * @param din_col input table columns
 * @param din_val validation bits column
 *
 * @param din_krn_cfg input kernel configurations
 *
 * @param din_meta input meta info
 * @param dout_meta output meta info
 *
 * @param dout_col output table columns
 *
 * @param htb_buf HBM buffers used to save build table key/payload for JOIN flow and lower space of hash-table for BF
 * flow
 * @param stb_buf HBM buffers used to save overflowed build table key/payload for JOIN flow and higher space of
 * hash-table for BF flow
 *
 */
extern "C" void gqeKernel(
    // input data columns
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> > din_col0,
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> > din_col1,
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> > din_col2,

    // validation buffer
    hls::burst_maxi<ap_uint<64> > din_val,

    // kernel config
    ap_uint<64>* din_krn_cfg,

    // meta input buffer
    ap_uint<64>* din_meta,
    // meta output buffer
    ap_uint<256>* dout_meta,

    // output data columns
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > dout_col0,
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > dout_col1,
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > dout_col2,
    hls::burst_maxi<ap_uint<8 * TPCH_INT_SZ * VEC_LEN> > dout_col3,

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
    hls::burst_maxi<ap_uint<256> > stb_buf7);

#endif // _XF_DB_GQE_3_IN_1_H_
