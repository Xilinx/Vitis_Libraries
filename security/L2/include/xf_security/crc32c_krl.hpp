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

/**
 * @file crc32_krl.hpp
 * @brief header file for crc32_krl.
 * This file part of Vitis Security Library.
 *
 */

#ifndef _XF_SECURITY_CRC32C_KERNEL_HPP_
#define _XF_SECURITY_CRC32C_KERNEL_HPP_

#pragma once

#include "vpp_acc.hpp"
#include <ap_int.h>
#include <hls_stream.h>
#include "xf_security/crc32c.hpp"

#if !defined(__SYNTHESIS__)
#include <iostream>
#endif

class crc32c_acc : public VPP_ACC<crc32c_acc, 1> {
    // port bindings
    ZERO_COPY(in_buff);
    ZERO_COPY(len_buff);
    ZERO_COPY(out_buff);

    // for both U.2 and U200
    SYS_PORT(in_buff, bank0);
    SYS_PORT(len_buff, bank0);
    SYS_PORT(out_buff, bank0);
    // for U50
    SYS_PORT_PFM(u50, in_buff, HBM[0]);
    SYS_PORT_PFM(u50, len_buff, HBM[1]);
    SYS_PORT_PFM(u50, out_buff, HBM[2]);

   public:
    /**
     * @brief top of the compute.
     *
     * @param in_buff input buffer providing the data that needs to be calculated.
     * @param len_buff length of each data in bytes from address 1 to N, address 0 used to save the number of input
     * data.
     * @param out_buff output result buffer.
     *
     */
    static void compute(ap_uint<512>* in_buff, ap_uint<32>* len_buff, ap_uint<32>* out_buff);
    /**
     * @brief top of the HLS kernel.
     *
     * @param in_buff input buffer providing the data that needs to be calculated.
     * @param len_buff length of each data in bytes from address 1 to N, address 0 used to save the number of input
     * data.
     * @param out_buff output result buffer.
     *
     */
    static void hls_top(ap_uint<512>* in_buff, ap_uint<32>* len_buff, ap_uint<32>* out_buff);
};

/**
 * @brief scan module for reading the input data.
 *
 * @param in_buff input buffer providing the data that needs to be calculated.
 * @param len_buff length of each data in bytes from address 1 to N, address 0 used to save the number of input
 * data.
 * @param in_strm input stream to feed to crc32c primitive.
 * @param init_strm initialization stream to feed to crc32c primitive.
 * @param len_strm length stream to feed to crc32c primitive.
 * @param end_len_strm end flag of len_strm.
 *
 */
static void scan(ap_uint<512>* in_buff,
                 ap_uint<32>* len_buff,
                 hls::stream<ap_uint<512> >& in_strm,
                 hls::stream<ap_uint<32> >& init_strm,
                 hls::stream<ap_uint<32> >& len_strm,
                 hls::stream<bool>& end_len_strm) {
    ap_uint<32> idx = 0;
    ap_uint<32> num = len_buff[0];

    for (int i = 0; i < num; i++) {
        ap_uint<32> len = len_buff[i + 1];
        init_strm.write(~0);
        len_strm.write(len);
        end_len_strm.write(false);
        ap_uint<32> len_blk = len / sizeof(ap_uint<512>) + ((len % sizeof(ap_uint<512>)) > 0);
        for (int j = 0; j < len_blk; j++) {
#pragma HLS pipeline II = 1
            in_strm.write(in_buff[idx++]);
        }
    }
    end_len_strm.write(true);
}

/**
 * @brief scan module for reading the input data.
 *
 * @param out_strm output stream to get from crc32c primitive.
 * @param end_out_strm end flag of out_strm.
 * @param out_buff output result buffer.
 *
 */
static void write(hls::stream<ap_uint<32> >& out_strm, hls::stream<bool>& end_out_strm, ap_uint<32>* out_buff) {
    bool end = end_out_strm.read();
    ap_uint<32> idx = 0;
    while (!end) {
#pragma HLS pipeline II = 1
        out_buff[idx++] = out_strm.read();
        end = end_out_strm.read();
    }
}

#endif // _XF_SECURITY_CRC32C_KERNEL_HPP_
