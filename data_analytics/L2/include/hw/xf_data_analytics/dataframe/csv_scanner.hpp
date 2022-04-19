/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_SCAN_KERNEL_HPP_
#define _XF_SCAN_KERNEL_HPP_

#include <ap_int.h>
#include <hls_math.h>
#include <hls_stream.h>
#include "internals/gzip_csv_block.hpp"
#include "internals/filter.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#include <memory>
#endif

#define BN 12    // pipeline number
#define NM 4     // parse block number in csv parser
#define FN 3     // filter number
#define FW 128   // file input bit-width
#define OUTW 256 // output bit-width

namespace xf {
namespace data_analytics {
namespace dataframe {

/**
 * @brief csv scanner kernel
 *
 * @param csvBuf input CSV files
 * @param firValue result buffer
 * @param szBuf size of input block and pre-allocated size of output block
 * @param cfgBuf input configuration
 */
void csv_scanner(ap_uint<128>* csvBuf, ap_uint<256>* firValue, ap_uint<64>* szBuf, ap_uint<64>* cfgBuf) {
#ifndef __SYNTHESIS__
    std::cout << "Start Kernel" << std::endl;
#endif
    ap_uint<32> in_begin_buf[BN];
    ap_uint<32> in_size_buf[BN];
    ap_uint<32> in_size_buf2[BN];
    ap_uint<8> type_buf[BN][NM][16];
    ap_uint<4> type_valid_buf[BN][16];
    ap_uint<3> type_num[BN][7]; // = {0, 0, 0, 0, 1, 2, 4};
    ap_uint<9> num_of_column = 0;
#pragma HLS array_partition variable = in_size_buf2 dim = 1 complete
#pragma HLS array_partition variable = type_buf dim = 1 complete
#pragma HLS array_partition variable = type_buf dim = 2 complete
#pragma HLS bind_storage variable = type_buf type = ram_1p impl = lutram
#pragma HLS array_partition variable = type_valid_buf dim = 1 complete
#pragma HLS array_partition variable = type_num dim = 0
#pragma HLS bind_storage variable = type_valid_buf type = ram_1p impl = lutram

    // ap_uint<FW> cond_field_flag = 0b1000000;
    // ap_uint<FW> out_field_flag = 0b0111111;
    hls::stream<ap_uint<64> > fir_cfg_strm("fir_cfg_strm");
#pragma HLS stream variable = fir_cfg_strm depth = 32
#ifndef __SYNTHESIS__
    std::cout << "Read CFG..." << std::endl;
#endif
    ap_uint<4> bn = szBuf[0];
    ap_uint<32> fir_sz[BN];
    ap_uint<32> cond_field_flag;
    ap_uint<32> out_field_flag;
    ap_uint<32> crc_check_field;
    internals::read_cfg<BN, NM, FW>(szBuf, cfgBuf, in_begin_buf, in_size_buf, in_size_buf2, fir_sz, type_buf,
                                    type_valid_buf, type_num, num_of_column, cond_field_flag, out_field_flag,
                                    crc_check_field, fir_cfg_strm);
#ifndef __SYNTHESIS__
    std::cout << "Core Kernel Start" << std::endl;
#endif
    internals::coreWrapp<BN, NM, FW, FN, OUTW>(cfgBuf[0][0], in_begin_buf, in_size_buf, in_size_buf2, cond_field_flag,
                                               out_field_flag, crc_check_field, csvBuf, type_buf, type_valid_buf,
                                               type_num, num_of_column, fir_cfg_strm, fir_sz, firValue);

    for (int i = 0; i < BN; i++) {
        szBuf[BN * 2 + 1 + i] = fir_sz[i];
    }
#ifndef __SYNTHESIS__
    std::cout << "End Kernel" << std::endl;
#endif
}

} // end namespace dataframe
} // end namespace data_analytics
} // end namespace xf

#endif
