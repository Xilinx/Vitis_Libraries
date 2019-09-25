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
#ifndef _XF_DB_GQE_JOIN_H_
#define _XF_DB_GQE_JOIN_H_

/**
 * @file gqe_join.hpp
 * @brief interface of GQE join kernel.
 */

#include "xf_utils_hw/types.hpp"
#include "gqe_blocks/gqe_types.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#define TEST_BUF_DEPTH 8000

/**
 * @brief GQE Join Kernel
 * \rst
 * For detailed document, see :ref:`gqe_kernel_design`.
 * \endrst
 * @param buf_A input table A buffer.
 * @param buf_B input table B buffer.
 * @param buf_C output table C buffer.
 * @param buf_D configuration buffer.
 *
 * @param htb_buf0 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf1 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf2 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf3 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf4 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf5 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf6 gqeJoin's temporal buffer for storing small table.
 * @param htb_buf7 gqeJoin's temporal buffer for storing small table.
 *
 * @param stb_buf0 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf1 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf2 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf3 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf4 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf5 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf6 gqeJoin's temporal buffer for storing small table.
 * @param stb_buf7 gqeJoin's temporal buffer for storing small table.
 *
 */
extern "C" void gqeJoin(ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_A[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_B[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_D[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf0[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf1[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf2[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf3[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf4[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf5[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf6[],
                        ap_uint<8 * TPCH_INT_SZ * 2> htb_buf7[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf0[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf1[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf2[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf3[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf4[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf5[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf6[],
                        ap_uint<8 * TPCH_INT_SZ * 2> stb_buf7[]);

extern "C" void gqeJoinHJ(bool join_on,
                          bool join_dual_key_on,
                          ap_uint<64> shuffle2_cfg,
                          ap_uint<3> join_flag,

                          hls::stream<ap_uint<8 * TPCH_INT_SZ> > hj_in[4][8],
                          hls::stream<bool> e_hj_in[4],

                          hls::stream<ap_uint<32> > jn_mx_strm[8],
                          hls::stream<bool>& e_jn_mx_strm,

                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf0[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf1[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf2[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf3[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf4[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf5[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf6[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> htb_buf7[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf0[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf1[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf2[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf3[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf4[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf5[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf6[TEST_BUF_DEPTH],
                          ap_uint<8 * TPCH_INT_SZ * 2> stb_buf7[TEST_BUF_DEPTH]);

#endif
