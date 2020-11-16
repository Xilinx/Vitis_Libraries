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

#ifndef _XF_DB_ISV_GQE_JOIN_H_
#define _XF_DB_ISV_GQE_JOIN_H_

#include "xf_utils_hw/types.hpp"
#include "xf_database/gqe_blocks/gqe_types.hpp"

#include <stddef.h>

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#define TEST_BUF_DEPTH 8000
#define VEC_SCAN 8
#define GQEJOIN_WITHOUT_AGGR 1

/**
 * @brief GQE join kernel
 *
 * @param _build_probe_flag kernel mode flag.
 *
 * @param buf_A input table buffer
 * @param buf_C output table buffer
 *
 * @param htb_buf hash table.
 * @param stb_buf overflow region of hash table.
 */
extern "C" void gqeJoin(ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_SCAN> buf_A8[TEST_BUF_DEPTH],
                        size_t _build_probe_flag,
                        ap_uint<512> tin_meta[24],
                        ap_uint<512> tout_meta[24],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_C8[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_D[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf0[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> htb_buf7[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf0[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf1[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf2[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf3[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf4[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf5[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf6[TEST_BUF_DEPTH],
                        ap_uint<8 * TPCH_INT_SZ * 8> stb_buf7[TEST_BUF_DEPTH]);

#endif
