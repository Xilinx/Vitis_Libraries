/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "xf_data_analytics/text/re_engine.hpp"
#include "general_config.hpp"
#include "re_engine_acc.hpp"

void re_engine_acc::compute(ap_uint<64>* msg_buff, ap_uint<16>* len_buff, ap_uint<32>* out_buff) {
    hls_kernel(msg_buff, len_buff, out_buff);
}

void re_engine_acc::hls_kernel(ap_uint<64>* msg_buff, ap_uint<16>* len_buff, ap_uint<32>* out_buff) {
    ap_uint<64> instr_buff[PU_NM][INSTR_DEPTH];
#pragma HLS bind_storage variable = instr_buff type = ram_2p impl = uram
#pragma HLS array_partition variable = instr_buff dim = 1

    ap_uint<32> bitset_buff[PU_NM][CCLASS_NM * 8];
#pragma HLS bind_storage variable = bitset_buff type = ram_2p impl = bram
#pragma HLS array_partition variable = bitset_buff dim = 1

    unsigned int cpgp_nm = 0;

    xf::data_analytics::text::internal::readRECfg<PU_NM, INSTR_DEPTH, CCLASS_NM>(cpgp_nm, msg_buff, instr_buff,
                                                                                 bitset_buff);

    xf::data_analytics::text::internal::reExec<PU_NM, INSTR_DEPTH, CCLASS_NM, CPGP_NM, MSG_LEN, STACK_SIZE>(
        cpgp_nm, instr_buff, bitset_buff, len_buff, msg_buff + msg_buff[0], out_buff);
}
