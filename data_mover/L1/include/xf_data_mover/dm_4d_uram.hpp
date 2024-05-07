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

/**
 * @file pl_data_mover.hpp
 * @brief This file provides load data from AXI master to AXI stream and vice versa.
 *
 * This file is part of Vitis Utility Library
 */

#ifndef _4D_DM_CORE_IMPL_HPP_
#define _4D_DM_CORE_IMPL_HPP_

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace data_mover {
namespace details {

/**
 * Parse the pre-defined 4D buffer descriptor for each data movement
 *
 * @param pattern_buf, store each 4D pattern
 * @param pattern_id, queue of pattern ID, the parser can be terminated by 0xFF
 * @param tile_4d_strm, tiling parameter for each dimension
 * @param bias_4d_strm, offset of each dimension in each 4D cubic
 * @param dim_4d_strm, data length for each dimension in 1D layout
 * @param e_strm, end flag of each pattern
 */
void parse_pattern(ap_uint<32>* pattern_buf,
                   hls::stream<ap_uint<8> >& pattern_id,
                   // output
                   hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                   hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                   hls::stream<ap_uint<3 * 32> >& dim_4d_strm,
                   hls::stream<ap_uint<2> >& e_strm) {
    ap_uint<1> last = false;
    while (!last) {
        ap_uint<8> cmd_t;
        if (pattern_id.read_nb(cmd_t)) {
            last = (cmd_t == 0xFF);
            ap_uint<32> cmd = cmd_t(7, 0);
            if (!last) {
                ap_uint<32> buff_dim[4];
                ap_uint<32> offset[4];
                ap_uint<32> tiling[4];
                ap_uint<32> dim_id[4];
                ap_uint<32> stride[4];
                ap_uint<32> wrap[4];
#pragma HLS array_partition variable = buff_dim complete
#pragma HLS array_partition variable = offset complete
#pragma HLS array_partition variable = tiling complete
#pragma HLS array_partition variable = dim_id complete
#pragma HLS array_partition variable = stride complete
#pragma HLS array_partition variable = wrap complete

                // load buffer dim, offset dim, tiling dim, traversal dim
                // extra overhead per pattern
                ap_uint<32> cmd_s = cmd * 24;
            LOAD_PATTERN_CFG_LOOP:
                for (ap_uint<5> i = 0; i < 24; i++) {
#pragma HLS pipeline II = 1
                    ap_uint<32> load_cfg = pattern_buf[cmd_s++];
                    if (i < 4)
                        buff_dim[i(1, 0)] = load_cfg;
                    else if (i < 8)
                        offset[i(1, 0)] = load_cfg;
                    else if (i < 12)
                        tiling[i(1, 0)] = load_cfg;
                    else if (i < 16)
                        dim_id[i(1, 0)] = load_cfg;
                    else if (i < 20)
                        stride[i(1, 0)] = load_cfg;
                    else
                        wrap[i(1, 0)] = load_cfg;
                }
#ifndef __SYNTHESIS__
                std::cout << "\nbuff_dim[4]:";
                for (int i = 0; i < 4; i++) std::cout << buff_dim[i] << " ";
                std::cout << "\noffset[4]:";
                for (int i = 0; i < 4; i++) std::cout << offset[i] << " ";
                std::cout << "\ntiling[4]:";
                for (int i = 0; i < 4; i++) std::cout << tiling[i] << " ";
                std::cout << "\ndim_id[4]:";
                for (int i = 0; i < 4; i++) std::cout << dim_id[i] << " ";
                std::cout << "\nstride[4]:";
                for (int i = 0; i < 4; i++) std::cout << stride[i] << " ";
                std::cout << "\nwrap[4]:";
                for (int i = 0; i < 4; i++) std::cout << wrap[i] << " ";
                std::cout << "\n" << std::endl;
#endif
            PRE_CAL_DIM_LOOP:
                for (int i = 1; i < 3; i++) {
#pragma HLS pipeline II = 2
                    buff_dim[i] *= buff_dim[i - 1];
                }

                // iterative to calcuate offset for each cubic

                ap_uint<32> bias[4];
#pragma HLS array_partition variable = bias complete

            CAL_DIM_OFFSET_TOP_LOOP:
                for (int w = 0; w < wrap[dim_id[3]]; w++) { // the wrap count of pre-defined 4th dimension
#pragma HLS loop_flatten off
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
                    bias[dim_id[3]] = offset[dim_id[3]] + stride[dim_id[3]] * w;
                    for (int z = 0; z < wrap[dim_id[2]]; z++) { // then 3rd dim
#pragma HLS loop_flatten off
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
                        bias[dim_id[2]] = offset[dim_id[2]] + stride[dim_id[2]] * z;
                        for (int y = 0; y < wrap[dim_id[1]]; y++) { // then 2nd dim
#pragma HLS loop_flatten off
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
                            bias[dim_id[1]] = offset[dim_id[1]] + stride[dim_id[1]] * y;
                        CAL_DIM_OFFSET_INNER_0_LOOP:
                            for (int x = 0; x < wrap[dim_id[0]]; x++) { // 1st dim at last
#pragma HLS pipeline off
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
                                bias[dim_id[0]] = offset[dim_id[0]] + stride[dim_id[0]] * x;

                                tile_4d_strm.write((tiling[3], tiling[2], tiling[1], tiling[0]));
                                bias_4d_strm.write((bias[3], bias[2], bias[1], bias[0]));
                                dim_4d_strm.write((buff_dim[2], buff_dim[1], buff_dim[0]));
                                if ((w == wrap[dim_id[3]] - 1) && (z == wrap[dim_id[2]] - 1) &&
                                    (y == wrap[dim_id[1]] - 1) && (x == wrap[dim_id[0]] - 1))
                                    e_strm.write(0x1);
                                else
                                    e_strm.write(0x0);
                            }
                        }
                    }
                }
            } // end one 4D pattern
        }     // end if
    }         // end while

    e_strm.write(0x2);
}

/**
 * Generate tiling address of each 4D pattern for URAM-read access, each end of
 * pattern will switch the ping-pong base address
 *
 * @tparam D depth of external URAM.
 *
 * @param tile_4d_strm, tiling parameter for each dimension
 * @param bias_4d_strm, offset of each dimension in each 4D cubic
 * @param dim_4d_strm, data length for each dimension in 1D layout
 * @param i_e_strm, end flag of each pattern
 * @param raddr_strm, read address for URAM-access
 * @param o_e_strm, end flag of each pattern
 */
template <int D>
void read_uram_addr_gen(hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                        hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                        hls::stream<ap_uint<3 * 32> >& dim_4d_strm,
                        hls::stream<ap_uint<2> >& i_e_strm,
                        // output
                        hls::stream<ap_uint<32> >& raddr_strm,
                        hls::stream<ap_uint<2> >& o_e_strm) {
    const int PING_PONG_OFFSET = (D >> 1);
    int pp_offset = 0; // start from ping

    ap_uint<2> last = 0;
    while (!last[1]) {
        if (i_e_strm.read_nb(last)) {
            if (!last[1]) {
                ap_uint<32> tiling[4], bias[4];
                (tiling[3], tiling[2], tiling[1], tiling[0]) = tile_4d_strm.read();
                (bias[3], bias[2], bias[1], bias[0]) = bias_4d_strm.read();
                ap_uint<3 * 32> dim_sz = dim_4d_strm.read();
                for (int l = 0; l < tiling[3]; l++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                    for (int k = 0; k < tiling[2]; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        for (int j = 0; j < tiling[1]; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        INNER_ADDR_GEN_LOOP:
                            for (int i = 0; i < tiling[0]; i++) {
#pragma HLS pipeline II = 1
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                                ap_uint<32> addr = (bias[3] + l) * dim_sz(95, 64) + (bias[2] + k) * dim_sz(63, 32) +
                                                   (bias[1] + j) * dim_sz(31, 0) + (bias[0] + i);
                                raddr_strm.write(pp_offset + addr);
                                if (last[0] && (l == tiling[3] - 1) && (k == tiling[2] - 1) && (j == tiling[1] - 1) &&
                                    (i == tiling[0] - 1))
                                    o_e_strm.write(0x1);
                                else
                                    o_e_strm.write(0x0);
                            }
                        }
                    }
                }

                // switch ping-pong at end of each pattern
                if (last[0]) {
                    pp_offset = (pp_offset == 0) ? PING_PONG_OFFSET : 0;
                }
            } // end one 4D cubic
        }     // end if
    }         // end while

    raddr_strm.write(0xFFFFFFFF);
    o_e_strm.write(0x2);
}

/**
 * Generate tiling address of each 4D pattern for URAM-write access, each end of
 * pattern will switch the ping-pong base address
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 *@param tile_4d_strm, tiling parameter for each dimension
 *@param bias_4d_strm, offset of each dimension in each 4D cubic
 *@param dim_4d_strm, data length for each dimension in 1D layout
 *@param i_e_strm, end flag of each pattern
 *@param from_strm, input AXI-strema to cache into URAM
 *@param waddr_strm, write address for URAM-access
 *@param wdata_strm, write data for URAM-access
 *@param o_ack, acknowlege flag at each end of 4D pattern
 */
template <int W, int D>
void write_uram_addr_gen(hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                         hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                         hls::stream<ap_uint<3 * 32> >& dim_4d_strm,
                         hls::stream<ap_uint<2> >& i_e_strm,
                         hls::stream<ap_axiu<W, 0, 0, 0> >& from_strm,
                         // output
                         hls::stream<ap_uint<32> >& waddr_strm,
                         hls::stream<ap_uint<W> >& wdata_strm,
                         hls::stream<ap_uint<8> >& o_ack) {
    const int PING_PONG_OFFSET = (D >> 1);
    int pp_offset = 0; // start from ping

    ap_uint<2> last = 0;
    while (!last[1]) {
        if (i_e_strm.read_nb(last)) {
            if (!last[1]) {
                ap_uint<32> tiling[4], bias[4];
                (tiling[3], tiling[2], tiling[1], tiling[0]) = tile_4d_strm.read();
                (bias[3], bias[2], bias[1], bias[0]) = bias_4d_strm.read();
                ap_uint<3 * 32> dim_sz = dim_4d_strm.read();
                for (int l = 0; l < tiling[3]; l++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                    for (int k = 0; k < tiling[2]; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        for (int j = 0; j < tiling[1]; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        INNER_ADDR_GEN_LOOP:
                            for (int i = 0; i < tiling[0]; i++) {
#pragma HLS pipeline II = 1
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                                ap_uint<32> addr = (bias[3] + l) * dim_sz(95, 64) + (bias[2] + k) * dim_sz(63, 32) +
                                                   (bias[1] + j) * dim_sz(31, 0) + (bias[0] + i);
                                ap_axiu<W, 0, 0, 0> data_tmp = from_strm.read();
                                waddr_strm.write(pp_offset + addr);
                                wdata_strm.write(data_tmp.data);
                            }
                        }
                    }
                }

                // switch ping-pong at end of each pattern
                if (last[0]) {
                    o_ack.write(0x0);
                    pp_offset = (pp_offset == 0) ? PING_PONG_OFFSET : 0;
                }
            } // end one 4D cubic
        }     // end if
    }         // end while
}

/**
 * Generate tiling address of each 4D pattern for DDR-read access
 *
 * @tparam BURST_LEN burst length of AXI read
 *
 * @param tile_4d_strm, tiling parameter for each dimension
 * @param bias_4d_strm, offset of each dimension in each 4D cubic
 * @param dim_4d_strm, data length for each dimension in 1D layout
 * @param i_e_strm, end flag of each pattern
 * @param raddr_strm, read address for DDR-access
 * @param blen_strm, read burst length for DDR-access
 * @param o_e_strm, end flag of each pattern
 */
template <int BURST_LEN>
void axim_read_addr_gen(hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                        hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                        hls::stream<ap_uint<3 * 32> >& dim_4d_strm,
                        hls::stream<ap_uint<2> >& i_e_strm,
                        // output
                        hls::stream<ap_uint<32> >& raddr_strm,
                        hls::stream<ap_uint<16> >& blen_strm,
                        hls::stream<ap_uint<2> >& o_e_strm) {
    ap_uint<2> last = 0;
    while (!last[1]) {
        if (i_e_strm.read_nb(last)) {
            if (!last[1]) {
                ap_uint<32> tiling[4], bias[4];
                (tiling[3], tiling[2], tiling[1], tiling[0]) = tile_4d_strm.read();
                (bias[3], bias[2], bias[1], bias[0]) = bias_4d_strm.read();
                ap_uint<3 * 32> dim_sz = dim_4d_strm.read();
                for (int l = 0; l < tiling[3]; l++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                    for (int k = 0; k < tiling[2]; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        for (int j = 0; j < tiling[1]; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        INNER_ADDR_GEN_LOOP:
                            for (int i = 0; i < tiling[0]; i += BURST_LEN) {
#pragma HLS pipeline II = 1
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                                ap_uint<32> addr = (bias[3] + l) * dim_sz(95, 64) + (bias[2] + k) * dim_sz(63, 32) +
                                                   (bias[1] + j) * dim_sz(31, 0) + (bias[0] + i);
                                ap_uint<16> burst_len;
                                if ((i + BURST_LEN) <= tiling[0])
                                    burst_len = BURST_LEN;
                                else
                                    burst_len = tiling[0] - i;

                                raddr_strm.write(addr);
                                blen_strm.write(burst_len);
                                if (last[0] && (l == tiling[3] - 1) && (k == tiling[2] - 1) && (j == tiling[1] - 1) &&
                                    (tiling[0] <= (i + BURST_LEN)))
                                    o_e_strm.write(0x1);
                                else
                                    o_e_strm.write(0x0);
                            }
                        }
                    }
                }
            } // end one 4D cubic
        }     // end if
    }         // end while

    o_e_strm.write(0x2);
}

/**
 * Generate tiling address of each 4D pattern for DDR-write access
 *
 * @tparam BURST_LEN burst length of AXI write
 *
 * @param tile_4d_strm, tiling parameter for each dimension
 * @param bias_4d_strm, offset of each dimension in each 4D cubic
 * @param dim_4d_strm, data length for each dimension in 1D layout
 * @param i_e_strm, end flag of each pattern
 * @param waddr_strm, write address for DDR-access
 * @param blen_strm, write burst len for DDR-access
 * @param o_e_strm, end flag of each 4D pattern
 */
template <int BURST_LEN>
void axim_write_addr_gen(hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                         hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                         hls::stream<ap_uint<3 * 32> >& dim_4d_strm,
                         hls::stream<ap_uint<2> >& i_e_strm,
                         // output
                         hls::stream<ap_uint<32> >& waddr_strm,
                         hls::stream<ap_uint<16> > blen_strm[2],
                         hls::stream<ap_uint<2> > o_e_strm[2]) {
    ap_uint<2> last = 0;
    while (!last[1]) {
        if (i_e_strm.read_nb(last)) {
            if (!last[1]) {
                ap_uint<32> tiling[4], bias[4];
                (tiling[3], tiling[2], tiling[1], tiling[0]) = tile_4d_strm.read();
                (bias[3], bias[2], bias[1], bias[0]) = bias_4d_strm.read();
                ap_uint<3 * 32> dim_sz = dim_4d_strm.read();
                for (int l = 0; l < tiling[3]; l++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                    for (int k = 0; k < tiling[2]; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        for (int j = 0; j < tiling[1]; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                        INNER_ADDR_GEN_LOOP:
                            for (int i = 0; i < tiling[0]; i += BURST_LEN) {
#pragma HLS pipeline II = 1
#pragma HLS LOOP_TRIPCOUNT min = 2 max = 2
                                ap_uint<32> addr = (bias[3] + l) * dim_sz(95, 64) + (bias[2] + k) * dim_sz(63, 32) +
                                                   (bias[1] + j) * dim_sz(31, 0) + (bias[0] + i);
                                ap_uint<16> burst_len;
                                if ((i + BURST_LEN) <= tiling[0])
                                    burst_len = BURST_LEN;
                                else
                                    burst_len = tiling[0] - i;

                                waddr_strm.write(addr);
                                blen_strm[0].write(burst_len);
                                blen_strm[1].write(burst_len);
                                if (last[0] && (l == tiling[3] - 1) && (k == tiling[2] - 1) && (j == tiling[1] - 1) &&
                                    (tiling[0] <= (i + BURST_LEN))) {
                                    o_e_strm[0].write(0x1);
                                    o_e_strm[1].write(0x1);
                                } else {
                                    o_e_strm[0].write(0x0);
                                    o_e_strm[1].write(0x0);
                                }
                            }
                        }
                    }
                }
            } // end one 4D cubic
        }     // end if
    }         // end while

    o_e_strm[0].write(0x2);
    o_e_strm[1].write(0x2);
}

/**
 * Flatten each burst URAM-read access into elem-by-elem, and switch ping-pong at each end of pattern
 *
 * @tparam D depth of external URAM.
 *
 * @param blen_strm, burst length of each URAM-read
 * @param i_e_strm, end flag of each pattern
 * @param raddr_strm, elem-by-elem address for each burst read
 */
template <int D>
void flatten_uram_read(hls::stream<ap_uint<16> >& blen_strm,
                       hls::stream<ap_uint<2> >& i_e_strm,
                       hls::stream<ap_uint<32> >& raddr_strm) {
    const int PING_PONG_OFFSET = (D >> 1);
    int pp_offset = 0; // start from ping
    ap_uint<16> cnt = 0;
    ap_uint<32> rd_ptr = 0;
    ap_uint<2> last = 0;
FLATTERN_URAM_READ_REQUEST_LOOP:
    while (!last[1]) {
#pragma HLS pipeline II = 1
        if (cnt == 0) {
            if (i_e_strm.read_nb(last)) {
                if (!last[1]) {
                    ap_uint<16> blen = blen_strm.read();
                    cnt = blen;
                }
            }
        } else {
            cnt--;
            raddr_strm.write(pp_offset + rd_ptr);
            if (last[0] && cnt == 0) { // finish one pattern
                pp_offset = (pp_offset == 0) ? PING_PONG_OFFSET : 0;
                rd_ptr = 0;
            } else
                rd_ptr++;
        }
    }

    raddr_strm.write(0xFFFFFFFF);
}

/**
 * Wrapper of URAM access, both read and write can be issued with II = 1 simultaneously, data congestion
 * should be ensured by external requestor
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 * @param waddr_strm input write address
 * @param wdata_strm input write data
 * @param raddr_strm input read address
 * @param rdata_strm output read data
 */
template <int W, int D>
void uram_access(hls::stream<ap_uint<32> >& waddr_strm,
                 hls::stream<ap_uint<W> >& wdata_strm,
                 hls::stream<ap_uint<32> >& raddr_strm,
                 hls::stream<ap_uint<W> >& rdata_strm) {
#ifndef __SYNTHESIS__
    ap_uint<W>* buff = (ap_uint<W>*)malloc(sizeof(ap_uint<W>) * D);
#else
    ap_uint<W> buff[D];
#pragma HLS bind_storage variable = buff type = RAM_2P impl = URAM
#endif
    bool last = false;
URAM_ACCESS_CORE_LOOP:
    while (!last) {
#pragma HLS pipeline II = 1
#pragma HLS dependence variable = buff inter false
#pragma HLS dependence variable = buff intra false
        ap_uint<32> waddr, raddr;
        if (!waddr_strm.empty() && !wdata_strm.empty()) {
            waddr = waddr_strm.read();
            buff[waddr] = wdata_strm.read();
        }

        if (raddr_strm.read_nb(raddr)) {
            last = (raddr == 0xFFFFFFFF);
            if (!last) {
                ap_uint<W> rdata = buff[raddr];
                rdata_strm.write(rdata);
            }
        }
    }

#ifndef __SYNTHESIS__
    free(buff);
#endif
}

/**
 * Create burst read request from DDR/HBM and forward to URAM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 *
 * @param maxi_port, AXI-master port for reading
 * @param raddr_strm, stream to get offset for burst read
 * @param blen_strm, stream ot get burst length for each read
 * @param i_e_strm, end flag of each pattern
 * @param fb_req_strm, stream to get loopback info about burst read
 * @param ack, acknowlege flag at each end of 4D pattern
 * @param req_info_strm, stream to delay each valid burst read request
 * @param waddr_strm, write address to cache the read from DDR to URAM
 * @param wdata_strm, write data from DDR to URAM
 *
 */
template <int W, int D, int LATENCY, int OUTSTANDING>
void axim_burst_read(hls::burst_maxi<ap_uint<W> > maxi_port,
                     hls::stream<ap_uint<32> >& raddr_strm,
                     hls::stream<ap_uint<16> >& blen_strm,
                     hls::stream<ap_uint<2> >& i_e_strm,
                     hls::stream<ap_uint<17> >& fb_req_strm,
                     // output
                     hls::stream<ap_uint<8> >& ack,
                     hls::stream<ap_uint<17> >& req_info_strm,
                     hls::stream<ap_uint<32> >& waddr_strm,
                     hls::stream<ap_uint<W> >& wdata_strm) {
    const int PING_PONG_OFFSET = (D >> 1);
    int pp_offset = 0;             // start from ping
    ap_uint<10> rec_head = 0;      // record head
    ap_uint<10> rec_tail = 0;      // record tail
    ap_uint<16> read_left = 0;     // read left in 1 burst
    ap_uint<1> last_cubic = false; // if last cubic in one pattern, ack if yes
    ap_uint<32> wr_ptr = 0;
    ap_uint<2> last = 0;
BURST_READ_LOOP:
    while (!last[1] || (rec_head != rec_tail)) {
#pragma HLS pipeline II = 1
        ap_uint<10> req_left;
        if (rec_tail == rec_head)
            req_left = OUTSTANDING / 2;
        else if (rec_tail < rec_head)
            req_left = OUTSTANDING / 2 - (rec_head - rec_tail);
        else
            req_left = (rec_tail - rec_head) - (1024 - OUTSTANDING / 2);

        if (req_left != 0 && !last[1]) { // if read outstanding is not exhausted, issue more request
            if (i_e_strm.read_nb(last)) {
                if (!last[1]) {
                    ap_uint<32> tmp_offset = raddr_strm.read();
                    ap_uint<16> tmp_burst = blen_strm.read();

                    maxi_port.read_request(tmp_offset, tmp_burst);
                    req_info_strm.write((last[0], tmp_burst));
                    rec_head++;
                }
            }
        }

        if (!fb_req_strm.empty() || read_left != 0) { // if there's mature request
            if (read_left == 0) {                     // next burst to uram
                rec_tail++;
                ap_uint<17> rec_tmp = fb_req_strm.read();
                last_cubic = rec_tmp[16];
                read_left = rec_tmp(15, 0) - 1;
            } else {
                read_left--;
            }

            ap_uint<W> raw_data = maxi_port.read();
            waddr_strm.write(pp_offset + wr_ptr);
            wdata_strm.write(raw_data);

            if (last_cubic && read_left == 0) {
                ack.write(0);
                pp_offset = (pp_offset == 0) ? PING_PONG_OFFSET : 0;
                wr_ptr = 0;
            } else
                wr_ptr++;
        }
    }

RES_ELEM_OF_LAST_CUBIC:
    for (int i = 0; i < read_left; i++) {
#pragma HLS pipeline II = 1
        ap_uint<W> raw_data = maxi_port.read();
        waddr_strm.write(pp_offset + wr_ptr);
        wdata_strm.write(raw_data);
        wr_ptr++;
    }
    if (read_left != 0) ack.write(0);
    // end of delay
    req_info_strm.write((ap_uint<17>)-1);
}

/**
 * Delay each burst read request for better outstanding read
 *
 * @tparam LATENCY latency of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 *
 * @param i_info_strm, input burst read request
 * @param o_info_strm, delayed burst read request
 */
template <int LATENCY, int OUTSTANDING>
void axim_br_loopback(hls::stream<ap_uint<17> >& i_info_strm, hls::stream<ap_uint<17> >& o_info_strm) {
    ap_uint<LATENCY> check = 0; // delay check
    hls::stream<ap_uint<17>, LATENCY + OUTSTANDING> fb_fifo;
    bool last = false;
LOOPBACK_BURST_REST_REQ_LOOP:
    while (!last) {
#pragma HLS pipeline II = 1
        bool check_l = check[LATENCY - 1];
        check <<= 1;
        ap_uint<17> info_tmp;
        if (i_info_strm.read_nb(info_tmp)) {
            last = (info_tmp == (ap_uint<17>)-1);
            if (!last) {
                fb_fifo.write(info_tmp);
                check[0] = 1;
            }
        }

        if (check_l) {
            o_info_strm.write(fb_fifo.read());
        }
    }
}

/**
 * Create burst write request to DDR/HBM, data from URAM
 *
 * @tparam W width of MAXI port.
 * @tparam LATENCY latency of MAXI port.
 * @tparam OUTSTANDING write outstanding of MAXI port, should be less than 512.
 *
 * @param maxi_port, AXI-master port for writing
 * @param rdata_strm, stream to get data for burst write
 * @param offset_strm, stream ot get address for each write
 * @param blen_strm, stream ot get burst length for each write
 * @param i_e_strm, end flag of each pattern
 * @param ack, acknowlege flag at each end of 4D pattern
 */
template <int W, int LATENCY, int OUTSTANDING>
void axim_burst_write(hls::burst_maxi<ap_uint<W> > maxi_port,
                      hls::stream<ap_uint<W> >& rdata_strm,
                      hls::stream<ap_uint<32> >& offset_strm,
                      hls::stream<ap_uint<16> >& blen_strm,
                      hls::stream<ap_uint<2> >& i_e_strm,
                      // output
                      hls::stream<ap_uint<8> >& ack) {
    ap_uint<LATENCY> check = 0;             // delay check
    ap_uint<10> req_left = OUTSTANDING / 2; // how many more request could be issued, "/2" is to avoid hang
    ap_uint<17> burst_record[1024];         // burstlen record
    ap_uint<10> rec_head = 0;               // record head
    ap_uint<10> rec_tail = 0;               // record tail
    ap_uint<10> rec_tail_last = 0;          // record tail to track each new burst write
    ap_uint<16> write_left = 0;             // write left in 1 burst
    ap_uint<1> last_cubic = false;          // if last cubic in one pattern, ack if yes
    ap_uint<2> last = 0;
BURST_WRITE_LOOP:
    while (!last[1] || req_left != OUTSTANDING / 2) {
#pragma HLS pipeline II = 1
        bool check_l = check[LATENCY - 1];
        check <<= 1;

        if (req_left != 0 && !last[1]) {
            if (i_e_strm.read_nb(last)) {
                if (!last[1]) {
                    ap_uint<32> tmp_offset = offset_strm.read();
                    ap_uint<16> tmp_burst = blen_strm.read();

                    maxi_port.write_request(tmp_offset, tmp_burst);
                    burst_record[rec_tail++] = (last[0], tmp_burst);
                }
            }
        }

        if (rec_head != rec_tail || write_left != 0) {
            ap_uint<W> wdata;
            if (rdata_strm.read_nb(wdata)) {
                if (write_left == 0) { // if there is write request left
                    ap_uint<17> rec_tmp = burst_record[rec_head++];
                    last_cubic = rec_tmp[16];
                    write_left = rec_tmp(15, 0) - 1;
                } else // if there is unwritten data for a burst
                    write_left--;

                maxi_port.write(wdata);
                if (write_left == 0) {
                    if (last_cubic) ack.write(0);
                    check[0] = 1;
                }
            }
        }

        if (check_l) {
            maxi_port.write_response();
        }

        bool new_burst = (rec_tail_last != rec_tail);
        if (check_l && !new_burst) {
            req_left++;
        } else if (!check_l && new_burst) {
            req_left--;
        }

        rec_tail_last = rec_tail;
    }

    bool resdu_burst = (write_left != 0);
RESDU_BURST_WRITE_LEFT_LOOP:
    while (write_left != 0) {
#pragma HLS pipeline II = 1
        ap_uint<W> wdata;
        if (rdata_strm.read_nb(wdata)) {
            maxi_port.write(wdata);
            write_left--;
        }
    }
    if (resdu_burst) {
        ack.write(0);
        maxi_port.write_response();
    }
}

/**
 * Forward data from URAM and package it to AXI-stream
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 *
 * @param e_strm, end flag of each 4D pattern
 * @param rdata_strm, data form URAM
 * @param o_ack, acknowlege flag at each end of 4D pattern
 * @param to_strm, AXI-stream output to other kernel
 */
template <int W>
void write_to_axis(hls::stream<ap_uint<2> >& e_strm,
                   hls::stream<ap_uint<W> >& rdata_strm,
                   // output
                   hls::stream<ap_uint<8> >& o_ack,
                   hls::stream<ap_axiu<W, 0, 0, 0> >& to_strm) {
    ap_uint<2> last = 0;
    while (!last[1]) {
#pragma HLS pipeline II = 1
        if (e_strm.read_nb(last)) {
            if (!last[1]) {
                ap_axiu<W, 0, 0, 0> tmp_data;
                tmp_data.data = rdata_strm.read();
                tmp_data.keep = -1;
                tmp_data.last = 0;
                to_strm.write(tmp_data);
                // end of one pattern
                if (last[0]) o_ack.write(0);
            }
        }
    }
}

/**
 * One of component of Read DM, read data from URAM and forward to AXI-stream
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 * @param waddr_strm, write address to access internal URAM
 * @param wdata_strm, write data to access internal URAM
 * @param pattern_buf, buffer to store 4D pattern against internal URAM cache
 * @param pattern_id, identifier to enable which pattern
 * @param o_ack, acknowlege flag at each end of 4D pattern
 * @param to_strm, AXI-stream output
 */
template <int W, int D>
void uram_to_axis(hls::stream<ap_uint<32> >& waddr_strm,
                  hls::stream<ap_uint<W> >& wdata_strm,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  // output
                  hls::stream<ap_uint<8> >& o_ack,
                  hls::stream<ap_axiu<W, 0, 0, 0> >& to_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("u2s_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("u2s_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> dim_4d_strm("u2s_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("u2s_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = dim_4d_strm type = FIFO impl = lutram

    parse_pattern(pattern_buf, pattern_id, tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm);

    hls::stream<ap_uint<32>, 32> raddr_strm("u2s_raddr_strm");
    hls::stream<ap_uint<W>, 32> rdata_strm("u2s_rdata_strm");
    hls::stream<ap_uint<2>, 32> gen_e_strm("u2s_gen_e_strm");
#pragma HLS bind_storage variable = raddr_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = rdata_strm type = FIFO impl = lutram
    read_uram_addr_gen<D>(tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm, raddr_strm, gen_e_strm);

    uram_access<W, D>(waddr_strm, wdata_strm, raddr_strm, rdata_strm);

    write_to_axis<W>(gen_e_strm, rdata_strm, o_ack, to_strm);

#ifndef __SYNTHESIS__
    std::cout << "uram_to_axi_stream finished." << std::endl;
#endif
}

/**
 * One of component of Read DM, read data from DDR/HBM and cache to URAM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 *
 * @param maxi_port, AXI-master interface to access DDR/HBM
 * @param pattern_buf, buffer to store 4D pattern against internal URAM cache
 * @param pattern_id, identifier to enable which pattern
 * @param o_ack, acknowlege flag at each end of 4D pattern
 * @param waddr_strm, write address to access internal URAM
 * @param wdata_strm, write data to access internal URAM
 */
template <int W, int D, int LATENCY, int BURST_LEN, int OUTSTANDING>
void axim_to_uram(hls::burst_maxi<ap_uint<W> > maxi_port,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  // output
                  hls::stream<ap_uint<8> >& o_ack,
                  hls::stream<ap_uint<32> >& waddr_strm,
                  hls::stream<ap_uint<W> >& wdata_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("m2s_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("m2s_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> dim_4d_strm("m2s_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("m2s_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = dim_4d_strm type = FIFO impl = lutram

    parse_pattern(pattern_buf, pattern_id, tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm);

    hls::stream<ap_uint<32>, 32> raddr_strm("m2s_raddr_strm");
    hls::stream<ap_uint<16>, 32> rlen_strm("m2s_rlen_strm");
    hls::stream<ap_uint<2>, 32> gen_e_strm("m2s_gen_e_strm");
#pragma HLS bind_storage variable = raddr_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = rlen_strm type = FIFO impl = lutram
    axim_read_addr_gen<BURST_LEN>(tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm, raddr_strm, rlen_strm,
                                  gen_e_strm);

    hls::stream<ap_uint<17>, OUTSTANDING / 2> br_info_strm;
    hls::stream<ap_uint<17>, OUTSTANDING / 2> br_fb_info_strm;
#pragma HLS bind_storage variable = br_info_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = br_fb_info_strm type = FIFO impl = lutram
    axim_burst_read<W, D, LATENCY, OUTSTANDING>(maxi_port, raddr_strm, rlen_strm, gen_e_strm, br_fb_info_strm, o_ack,
                                                br_info_strm, waddr_strm, wdata_strm);
    axim_br_loopback<LATENCY, OUTSTANDING>(br_info_strm, br_fb_info_strm);
}

/**
 * One of component of Write DM, read data from AXI-stream and cache to URAM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 * @param raddr_strm, read address to access internal URAM
 * @param from_strm, AXI-stream input
 * @param pattern_buf, buffer to store 4D pattern against internal URAM cache
 * @param pattern_id, identifier to enable which pattern
 * @param rdata_strm, read data to access internal URAM
 * @param o_ack, acknowlege flag at each end of 4D pattern
 */
template <int W, int D>
void axis_to_uram(hls::stream<ap_uint<32> >& raddr_strm,
                  hls::stream<ap_axiu<W, 0, 0, 0> >& from_strm,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  // output
                  hls::stream<ap_uint<W> >& rdata_strm,
                  hls::stream<ap_uint<8> >& o_ack) {
#pragma HLS dataflow
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("s2u_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("s2u_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> dim_4d_strm("s2u_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("s2u_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = dim_4d_strm type = FIFO impl = lutram

    parse_pattern(pattern_buf, pattern_id, tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm);

    hls::stream<ap_uint<32>, 32> waddr_strm("s2u_waddr_strm");
    hls::stream<ap_uint<W>, 32> wdata_strm("s2u_wdata_strm");
#pragma HLS bind_storage variable = waddr_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = wdata_strm type = FIFO impl = lutram
    write_uram_addr_gen<W, D>(tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm, from_strm, waddr_strm, wdata_strm,
                              o_ack);

    uram_access<W, D>(waddr_strm, wdata_strm, raddr_strm, rdata_strm);
}

/**
 * One of component of Write DM, read data from URAM and offload to DDR/HBM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING write outstanding of MAXI port, should be less than 512.
 *
 * @param maxi_port, AXI-master interface to access DDR/HBM
 * @param pattern_buf, buffer to store 4D pattern against internal URAM cache
 * @param pattern_id, identifier to enable which pattern
 * @param rdata_strm, write data to DDR/HBM from URAM
 * @param raddr_strm, write address to access URAM
 * @param o_ack, acknowlege flag at each end of 4D pattern
 */
template <int W, int D, int LATENCY, int BURST_LEN, int OUTSTANDING>
void uram_to_axim(hls::burst_maxi<ap_uint<W> > maxi_port,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  hls::stream<ap_uint<W> >& rdata_strm,
                  // output
                  hls::stream<ap_uint<32> >& raddr_strm,
                  hls::stream<ap_uint<8> >& o_ack) {
#pragma HLS dataflow
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("m2s_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("m2s_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> dim_4d_strm("m2s_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("m2s_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = dim_4d_strm type = FIFO impl = lutram

    parse_pattern(pattern_buf, pattern_id, tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm);

    hls::stream<ap_uint<32>, 32> u2m_waddr_strm("u2m_waddr_strm");
#pragma HLS bind_storage variable = u2m_waddr_strm type = FIFO impl = lutram
    hls::stream<ap_uint<16>, 32> u2m_blen_strm[2];
#pragma HLS bind_storage variable = u2m_blen_strm type = FIFO impl = lutram
    hls::stream<ap_uint<2>, 32> gen_e_strm[2];
    axim_write_addr_gen<BURST_LEN>(tile_4d_strm, bias_4d_strm, dim_4d_strm, parse_e_strm, u2m_waddr_strm, u2m_blen_strm,
                                   gen_e_strm);

    flatten_uram_read<D>(u2m_blen_strm[0], gen_e_strm[0], raddr_strm);

    axim_burst_write<W, LATENCY, OUTSTANDING>(maxi_port, rdata_strm, u2m_waddr_strm, u2m_blen_strm[1], gen_e_strm[1],
                                              o_ack);
}

} // namespace details

} // namespace data_mover
} // namespace xf
#endif
