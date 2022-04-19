/*
 * Copyright 2021 Xilinx, Inc.
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
#ifndef GQE_ISV_LOAD_CONFIG_HPP
#define GQE_ISV_LOAD_CONFIG_HPP

#ifndef __SYNTHESIS__
#include <stdio.h>
#include <iostream>
//#define USER_DEBUG true
#endif

#include <ap_int.h>
#include <hls_stream.h>
#include "hls_burst_maxi.h"

#include "xf_database/gqe_blocks_v3/gqe_types.hpp"

namespace xf {
namespace database {
namespace gqe {

// load kernel config for gqeKernel
static void load_config(const int bucket_depth,
                        hls::burst_maxi<ap_uint<64> >& din_krn_cfg,
                        hls::burst_maxi<ap_uint<64> >& din_meta,
                        hls::stream<int64_t>& nrow_strm,
                        int32_t& secID,
                        hls::stream<ap_uint<8> >& general_cfg_strm,
                        hls::stream<ap_uint<8> >& dup_general_cfg_strm,
                        hls::stream<ap_uint<3> >& join_cfg_strm,
                        hls::stream<ap_uint<36> >& bf_cfg_strm,
                        hls::stream<ap_uint<15> >& part_cfg_strm,
                        hls::stream<int>& bit_num_strm_copy,
                        hls::stream<ap_uint<32> >& filter_cfg_strm,
                        ap_uint<3>& din_col_en,
                        ap_uint<2>& rowID_flags,
                        hls::stream<ap_uint<8> >& write_out_cfg_strm) {
#ifdef USER_DEBUG
    std::cout << "-------- load kernel config --------" << std::endl;
#endif
    const int filter_cfg_depth = 53;

    din_meta.read_request(0, 512 / 64);
    ap_uint<512> meta;
    for (int j = 0; j < 512 / 64; j++) {
#pragma HLS pipeline II = 1
        meta.range(j * 32 + 31, j * 32) = din_meta.read();
    }
    // read in the number of rows
    int64_t nrow = meta.range(71, 8);
    nrow_strm.write(nrow);
    // read in the secID and pass to scan_cols module
    secID = meta.range(167, 136);
#ifndef __SYNTHESIS__
    std::cout << "===================meta buffer ===================" << std::endl;
    std::cout << "din_meta.vnum = " << meta.range(7, 0) << std::endl;
    std::cout << "din_meta.nrow = " << nrow << std::endl;
    std::cout << "din_meta.secID = " << secID << std::endl;
#endif

    // read in krn_cfg from off-chip memory
    din_krn_cfg.read_request(0, 14 * 512 / 64);
    ap_uint<512> config[14];
    for (int i = 0; i < 14; ++i) {
        for (int j = 0; j < 512 / 64; j++) {
#pragma HLS pipeline II = 1
            config[i].range(j * 64 + 63, j * 64) = din_krn_cfg.read() /* [i*(512/64)+j] */;
        }
    }

    bool tab_index = config[0][5];

    // define general_cfg to represent the general cfg for each module:
    // bit6: dual key on or off; bit5: build/probe flag; bit4 aggr on or off; bit3: part on or off; bit2: bf on or off;
    // bit1: join on or off; bit0: bypass on or off
    ap_uint<8> general_cfg = config[0].range(7, 0);
    general_cfg_strm.write(general_cfg);
    dup_general_cfg_strm.write(general_cfg);

    // define join_cfg to represent the join cfg for each module:
    // bit2: append mode; bit1-0: join type
    ap_uint<3> join_cfg;
    join_cfg.range(1, 0) = config[1].range(51, 50);
    join_cfg[2] = config[1][52];
    join_cfg_strm.write(join_cfg);

    // define part_cfg to represent the part cfg for each module:
    // bit14-4: bucket_depth; bit3-0: log_part;
    ap_uint<15> part_cfg;
    // log_part
    part_cfg.range(3, 0) = config[2].range(53, 50);
    part_cfg.range(14, 4) = bucket_depth;
    part_cfg_strm.write(part_cfg);
    // a copy of log_part to write-out module
    bit_num_strm_copy.write(config[2].range(53, 50));

    // define bf_cfg to represent the bloom filter cfg for each module:
    // bit 35-0: bloom filter size in bits
    ap_uint<36> bf_cfg;
    // bloom filter size
    bf_cfg.range(35, 0) = config[3].range(85, 50);
    bf_cfg_strm.write(bf_cfg);

    // read in input col enable flag for table A
    if (!tab_index) {
        din_col_en = config[0].range(12, 10);
        // read in input col enable flag for table B
    } else {
        din_col_en = config[0].range(15, 13);
    }

    // gen_rowid and valid_en flag
    if (!tab_index) {
        rowID_flags = config[0].range(31, 30);
    } else {
        rowID_flags = config[0].range(33, 32);
    }

    // write out col cfg
    ap_uint<8> write_out_en;
    // append mode
    write_out_en[7] = config[1][52];
    if (!tab_index) {
        // build
        write_out_en.range(3, 0) = config[0].range(19, 16);
    } else {
        // probe
        write_out_en.range(3, 0) = config[0].range(23, 20);
    }
    write_out_cfg_strm.write(write_out_en);
#ifdef USER_DEBUG
    std::cout << "write_out_en: " << (int)write_out_en << std::endl;
#endif

    // filter cfg
    ap_uint<32> filter_cfg_a[filter_cfg_depth];
#pragma HLS bind_storage variable = filter_cfg_a type = ram_1p impl = lutram

    for (int i = 0; i < filter_cfg_depth; i++) {
#pragma HLS pipeline II = 1
        filter_cfg_a[i] = config[4 * tab_index + 6 + i / 16].range(32 * ((i % 16) + 1) - 1, 32 * (i % 16));
        filter_cfg_strm.write(filter_cfg_a[i]);
    }
}

} // namespace gqe
} // namespace database
} // namespace xf

#endif // GQE_ISV_LOAD_CONFIG_HPP
