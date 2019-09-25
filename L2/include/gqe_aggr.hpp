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
#ifndef _XF_DB_GQE_AGGR_H_
#define _XF_DB_GQE_AGGR_H_

/**
 * @file gqe_aggr.hpp
 * @brief interface of GQE group-by aggregation kernel.
 */

#include "xf_utils_hw/types.hpp"
#include "gqe_blocks/gqe_types.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

#define TEST_BUF_DEPTH 4 << 20 //(512 bit x 4M = 256 MB)

/**
 * @brief GQE Aggr Kernel
 * \rst
 * For detailed document, see :ref:`gqe_kernel_design`.
 * \endrst
 * @param buf_in input table buffer.
 * @param buf_out output table buffer.
 * @param buf_cfg input configuration buffer.
 * @param buf_result_info output information buffer.
 *
 * @param ping_buf0 gqeAggr's temporal buffer for storing overflow.
 * @param ping_buf1 gqeAggr's temporal buffer for storing overflow.
 * @param ping_buf2 gqeAggr's temporal buffer for storing overflow.
 * @param ping_buf3 gqeAggr's temporal buffer for storing overflow.
 *
 * @param pong_buf0 gqeAggr's temporal buffer for storing overflow.
 * @param pong_buf1 gqeAggr's temporal buffer for storing overflow.
 * @param pong_buf2 gqeAggr's temporal buffer for storing overflow.
 * @param pong_buf3 gqeAggr's temporal buffer for storing overflow.
 *
 */
extern "C" void gqeAggr(ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_in[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> buf_out[],
                        ap_uint<8 * TPCH_INT_SZ> buf_cfg[],
                        ap_uint<8 * TPCH_INT_SZ> buf_result_info[],

                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> ping_buf0[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> ping_buf1[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> ping_buf2[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> ping_buf3[],

                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> pong_buf0[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> pong_buf1[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> pong_buf2[],
                        ap_uint<8 * TPCH_INT_SZ * VEC_LEN> pong_buf3[]);

#endif
