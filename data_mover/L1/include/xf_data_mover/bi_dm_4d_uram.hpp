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

#ifndef _BI_4D_DM_CORE_IMPL_HPP_
#define _BI_4D_DM_CORE_IMPL_HPP_

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

#include "xf_data_mover/dm_4d_uram.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace data_mover {
namespace bi_details {

/**
 * Parse the pre-defined 4D buffer descriptor with URAM address for each Bi-D data movement
 *
 * @param pattern_buf store each 4D pattern
 * @param pattern_id queue of pattern ID, the parser can be terminated by 0xFF
 * @param u_access_addr begin address in URAM of current pattern
 * @param tile_4d_strm tiling parameter for each dimension
 * @param bias_4d_strm offset of each dimension in each 4D cubic
 * @param buff_dim_4d_strm data length for each dimension in 1D layout
 * @param e_strm end flag of each pattern
 */
void bi_parse_pattern(ap_uint<32>* pattern_buf,
                      hls::stream<ap_uint<8> >& pattern_id,
                      // output
                      hls::stream<ap_uint<32> >& u_access_addr,
                      hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                      hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                      hls::stream<ap_uint<3 * 32> >& buff_dim_4d_strm,
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

                // load u_access_addr, buffer dim, offset dim, tiling dim, traversal dim
                // extra overhead per pattern
                ap_uint<32> cmd_s = cmd * 25;

// load start addr in URAM
#ifndef __SYNTHESIS__
                ap_uint<32> _u_access_addr = pattern_buf[cmd_s++];
#endif
                u_access_addr.write(pattern_buf[cmd_s++]);

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
                std::cout << "\nPattern's id: " << cmd;
                std::cout << "\nPattern's data in URAM: " << std::hex << _u_access_addr;
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
                                buff_dim_4d_strm.write((buff_dim[2], buff_dim[1], buff_dim[0]));
                                if ((w == wrap[dim_id[3]] - 1) && (z == wrap[dim_id[2]] - 1) &&
                                    (y == wrap[dim_id[1]] - 1) && (x == wrap[dim_id[0]] - 1))
                                    e_strm.write(0x1); // end one 4D pattern(including all tiles in wrap)
                                else
                                    e_strm.write(0x0);
                            }
                        }
                    }
                }
            } // end one 4D pattern
        }     // end if
    }         // end while
    // end all patterns in buf
    e_strm.write(0x2);
}

/**
 * Generate tiling address of each 4D pattern for URAM-read access, each pattern
 * has one URAM access-address.
 *
 * @tparam D depth of external URAM.
 *
 * @param u_access_addr_strm URAM access address for each pattern
 * @param tile_4d_strm tiling parameter for each dimension
 * @param bias_4d_strm offset of each dimension in each 4D cubic
 * @param buff_dim_4d_strm data length for each dimension in 1D layout
 * @param i_e_strm end flag of each pattern
 * @param raddr_strm read address for URAM-access
 * @param o_e_strm end flag of each pattern
 */
template <int D>
void read_uram_addr_gen(hls::stream<ap_uint<32> >& u_access_addr_strm,
                        hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                        hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                        hls::stream<ap_uint<3 * 32> >& buff_dim_4d_strm,
                        hls::stream<ap_uint<2> >& i_e_strm,
                        // output
                        hls::stream<ap_uint<32> >& raddr_strm,
                        hls::stream<ap_uint<2> >& o_e_strm) {
    ap_uint<2> last = 0;
    ap_uint<32> u_bais = 0;
    bool new_ptn = true;
    while (!last[1]) {
        if (i_e_strm.read_nb(last)) {
            if (last[1]) {
                break;
            } else if (new_ptn) {
                u_bais = u_access_addr_strm.read();
                new_ptn = false;
            }
            ap_uint<32> tiling[4], bias[4];
            (tiling[3], tiling[2], tiling[1], tiling[0]) = tile_4d_strm.read();
            (bias[3], bias[2], bias[1], bias[0]) = bias_4d_strm.read();
            ap_uint<3 * 32> dim_sz = buff_dim_4d_strm.read();
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
                            raddr_strm.write(u_bais + addr);
                            if (last[0] && (l == tiling[3] - 1) && (k == tiling[2] - 1) && (j == tiling[1] - 1) &&
                                (i == tiling[0] - 1)) {
                                o_e_strm.write(0x1);
                            } else
                                o_e_strm.write(0x0);
                        }
                    }
                }
            }
            if (last[0]) {
                new_ptn = true;
            }
        }
    }

    raddr_strm.write(0xFFFFFFFF);
    o_e_strm.write(0x2);
}

/**
 * Generate tiling address of each 4D pattern for URAM-write access, each pattern
 * has one URAM access-address.
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 *@param u_access_addr_strm URAM access address for each pattern
 *@param tile_4d_strm tiling parameter for each dimension
 *@param bias_4d_strm offset of each dimension in each 4D cubic
 *@param buff_dim_4d_strm data length for each dimension in 1D layout
 *@param i_e_strm end flag of each pattern
 *@param from_strm input AXI-stream to cache into URAM
 *@param waddr_strm write address for URAM-access
 *@param wdata_strm write data for URAM-access
 *@param o_ack acknowlege flag at each end of 4D pattern
 */
template <int W, int D>
void write_uram_addr_gen(hls::stream<ap_uint<32> >& u_access_addr_strm,
                         hls::stream<ap_uint<4 * 32> >& tile_4d_strm,
                         hls::stream<ap_uint<4 * 32> >& bias_4d_strm,
                         hls::stream<ap_uint<3 * 32> >& buff_dim_4d_strm,
                         hls::stream<ap_uint<2> >& i_e_strm,
                         hls::stream<ap_axiu<W, 0, 0, 0> >& from_strm,
                         // output
                         hls::stream<ap_uint<32> >& waddr_strm,
                         hls::stream<ap_uint<W> >& wdata_strm,
                         hls::stream<ap_uint<8> >& o_ack) {
    ap_uint<2> last = 0;
    ap_uint<32> u_bais = 0;
    bool new_ptn = true;
    while (!last[1]) {
        if (i_e_strm.read_nb(last)) {
            if (last[1]) {
                break;
            } else if (new_ptn) {
                u_bais = u_access_addr_strm.read();
                new_ptn = false;
            }
            ap_uint<32> tiling[4], bias[4];
            (tiling[3], tiling[2], tiling[1], tiling[0]) = tile_4d_strm.read();
            (bias[3], bias[2], bias[1], bias[0]) = bias_4d_strm.read();
            ap_uint<3 * 32> dim_sz = buff_dim_4d_strm.read();
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
                            waddr_strm.write(u_bais + addr);
                            wdata_strm.write(data_tmp.data);
                        }
                    }
                }
            }
            if (last[0]) {
                o_ack.write(0x0);
                new_ptn = true; // prev 4D pattern finished
            }
        }
    }
}

/**
 * Flatten each burst URAM-read access into elem-by-elem, and switch ping-pong at each end of pattern
 *
 * @tparam D depth of external URAM.
 *
 * @param blen_strm burst length of each URAM-read
 * @param i_e_strm end flag of each pattern
 * @param raddr_strm elem-by-elem address for each burst read
 */
template <int D>
void bi_flatten_uram_read(hls::stream<ap_uint<32> >& u_access_addr_strm,
                          hls::stream<ap_uint<16> >& blen_strm,
                          hls::stream<ap_uint<2> >& i_e_strm,
                          hls::stream<ap_uint<32> >& raddr_strm) {
    ap_uint<16> cnt = 0;
    ap_uint<32> rd_ptr = 0;
    ap_uint<2> last = 0;
    ap_uint<32> u_bias = 0;
    bool new_ptn = true;
FLATTERN_URAM_READ_REQUEST_LOOP:
    while (!last[1]) {
#pragma HLS pipeline II = 1
        if (cnt == 0) {
            if (i_e_strm.read_nb(last)) {
                if (!last[1]) { // if not last pattern
                    ap_uint<16> blen = blen_strm.read();
                    cnt = blen;
                    if (new_ptn) {
                        u_bias = u_access_addr_strm.read();
                        new_ptn = false;
                    }
                }
            }
        } else {
            cnt--;
            raddr_strm.write(u_bias + rd_ptr);
            if (last[0] && cnt == 0) { // finish one pattern
                rd_ptr = 0;
                new_ptn = true; // prev 4D pattern is flattened
            } else {
                rd_ptr++;
            }
        }
    }
    raddr_strm.write(0xFFFFFFFF);
}

/**
 * Wrapper of URAM for bi-directional access, both read and write can be issued with II = 1 simultaneously, data
 * congestion should be ensured by external requestor
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 * @param waddr_strm_0 input write address from AXI-Stream
 * @param wdata_strm_0 input write data from AXI-Stream
 * @param waddr_strm_1 input write address from MAXI
 * @param wdata_strm_1 input write address from MAXI
 * @param raddr_strm_0 output read address to AXI-Stream
 * @param rdata_strm_0 output read data to AXI-Stream
 * @param raddr_strm_1 output read address to MAXI
 * @param rdata_strm_1 output read data to MAXI
 */
template <int W, int D>
void bi_uram_access(hls::stream<ap_uint<32> >& waddr_strm_0,
                    hls::stream<ap_uint<W> >& wdata_strm_0,
                    hls::stream<ap_uint<32> >& waddr_strm_1,
                    hls::stream<ap_uint<W> >& wdata_strm_1,
                    hls::stream<ap_uint<32> >& raddr_strm_0,
                    hls::stream<ap_uint<W> >& rdata_strm_0,
                    hls::stream<ap_uint<32> >& raddr_strm_1,
                    hls::stream<ap_uint<W> >& rdata_strm_1) {
#ifndef __SYNTHESIS__
    ap_uint<W>* buff = (ap_uint<W>*)malloc(sizeof(ap_uint<W>) * D);
#else
    ap_uint<W> buff[D];
#pragma HLS bind_storage variable = buff type = RAM_2P impl = URAM
#endif
    bool last[2] = {false, false};
BI_URAM_ACCESS_CORE_LOOP:
    while (!last[0] || !last[1]) {
#pragma HLS pipeline II = 1
#pragma HLS dependence variable = buff inter false
#pragma HLS dependence variable = buff intra false
        ap_uint<32> waddr[2], raddr[2];

        // write into URAM
        if (!waddr_strm_0.empty()) {
            waddr[0] = waddr_strm_0.read();
            buff[waddr[0]] = wdata_strm_0.read();
        } else if (!waddr_strm_1.empty()) {
            waddr[1] = waddr_strm_1.read();
            buff[waddr[1]] = wdata_strm_1.read();
        }

        // read from URAM
        if (raddr_strm_0.read_nb(raddr[0]) && !(last[0] = (raddr[0] == 0xFFFFFFFF))) {
            ap_uint<W> rdata = buff[raddr[0]];
            rdata_strm_0.write(rdata);
        } else if (raddr_strm_1.read_nb(raddr[1]) && !(last[1] = (raddr[1] == 0xFFFFFFFF))) {
            ap_uint<W> rdata = buff[raddr[1]];
            rdata_strm_1.write(rdata);
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
 * @param maxi_port AXI-master port for reading
 * @param u_access_addr_strm URAM access address for data to write out
 * @param raddr_strm stream to get offset for burst read
 * @param blen_strm stream ot get burst length for each read
 * @param i_e_strm end flag of each pattern
 * @param fb_req_strm stream to get loopback info about burst read
 * @param ack acknowlege flag at each end of 4D pattern
 * @param req_info_strm stream to delay each valid burst read request
 * @param waddr_strm write address from DDR to URAM
 * @param wdata_strm write data from DDR to URAM
 *
 */
template <int W, int D, int LATENCY, int OUTSTANDING>
void axim_burst_read(hls::burst_maxi<ap_uint<W> > maxi_port,
                     hls::stream<ap_uint<32> >& u_access_addr_strm,
                     hls::stream<ap_uint<32> >& raddr_strm,
                     hls::stream<ap_uint<16> >& blen_strm,
                     hls::stream<ap_uint<2> >& i_e_strm,
                     hls::stream<ap_uint<17> >& fb_req_strm,
                     // output
                     hls::stream<ap_uint<8> >& ack,
                     hls::stream<ap_uint<17> >& req_info_strm,
                     hls::stream<ap_uint<32> >& waddr_strm,
                     hls::stream<ap_uint<W> >& wdata_strm) {
    ap_uint<10> rec_head = 0;      // record head
    ap_uint<10> rec_tail = 0;      // record tail
    ap_uint<16> read_left = 0;     // read left in 1 burst
    ap_uint<1> last_cubic = false; // if last cubic in one pattern, ack if yes
    ap_uint<32> wr_ptr = 0;
    ap_uint<32> u_bias = 0;
    ap_uint<2> last = 0;
    bool new_ptn = true;

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
                    if (new_ptn) {
                        u_bias = u_access_addr_strm.read();
                        new_ptn = false;
                    }
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
            waddr_strm.write(u_bias + wr_ptr);
            wdata_strm.write(raw_data);

            if (last_cubic && read_left == 0) {
                ack.write(0);
                new_ptn = true;
                wr_ptr = 0;
            } else
                wr_ptr++;
        }
    }

RES_ELEM_OF_LAST_CUBIC:
    for (int i = 0; i < read_left; i++) {
#pragma HLS pipeline II = 1
        ap_uint<W> raw_data = maxi_port.read();
        waddr_strm.write(u_bias + wr_ptr);
        wdata_strm.write(raw_data);
        wr_ptr++;
    }
    if (read_left != 0) ack.write(0);
    // end of delay
    req_info_strm.write((ap_uint<17>)-1);
}

/**
 * One of component of BiD-DM, read data from URAM and forward to AXI-Stream
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 * @param o_strm AXI-stream output
 * @param pattern_buf buffer to store 4D pattern against internal URAM cache
 * @param pattern_id identifier to enable which pattern
 * @param raddr_strm read address to access internal URAM
 * @param rdata_strm read data to access internal URAM
 * @param o_ack acknowlege flag at each end of 4D pattern
 */
template <int W, int D>
void uram_to_axis(hls::stream<ap_axiu<W, 0, 0, 0> >& o_strm,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  hls::stream<ap_uint<32> >& raddr_strm,
                  hls::stream<ap_uint<W> >& rdata_strm,
                  hls::stream<ap_uint<8> >& o_ack) {
#pragma HLS dataflow
    hls::stream<ap_uint<32>, 8> u_addr_strm("u2s_u_addr_strm");
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("u2s_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("u2s_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> buff_dim_4d_strm("u2s_buf_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("u2s_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = buff_dim_4d_strm type = FIFO impl = lutram

    bi_details::bi_parse_pattern(pattern_buf, pattern_id, u_addr_strm, tile_4d_strm, bias_4d_strm, buff_dim_4d_strm,
                                 parse_e_strm);

    hls::stream<ap_uint<2>, 32> gen_e_strm("u2s_gen_e_strm");

    read_uram_addr_gen<D>(u_addr_strm, tile_4d_strm, bias_4d_strm, buff_dim_4d_strm, parse_e_strm, raddr_strm,
                          gen_e_strm);

    details::write_to_axis<W>(gen_e_strm, rdata_strm, o_ack, o_strm);
}

/**
 * One of component of BiD-DM, read data from DDR/HBM and cache to URAM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 *
 * @param i_maxi_port AXI-master interface to access DDR/HBM
 * @param pattern_buf buffer to store 4D pattern against internal URAM cache
 * @param pattern_id identifier to enable which pattern
 * @param waddr_strm write address to access internal URAM, parsed in pattern
 * @param wdata_strm write data to access internal URAM
 * @param o_ack acknowlege flag at each end of 4D pattern
 */
template <int W, int D, int LATENCY, int BURST_LEN, int OUTSTANDING>
void axim_to_uram(hls::burst_maxi<ap_uint<W> > i_maxi_port,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  // output
                  hls::stream<ap_uint<32> >& waddr_strm,
                  hls::stream<ap_uint<W> >& wdata_strm,
                  hls::stream<ap_uint<8> >& o_ack) {
#pragma HLS dataflow
    hls::stream<ap_uint<32>, 8> u_addr_strm("u2s_u_addr_strm");
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("m2s_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("m2s_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> buff_dim_4d_strm("m2s_buf_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("m2s_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = buff_dim_4d_strm type = FIFO impl = lutram
    hls::stream<ap_uint<32>, 32> raddr_strm("m2s_raddr_strm");
    hls::stream<ap_uint<16>, 32> rlen_strm("m2s_rlen_strm");
    hls::stream<ap_uint<2>, 32> gen_e_strm("m2s_gen_e_strm");
#pragma HLS bind_storage variable = raddr_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = rlen_strm type = FIFO impl = lutram
    hls::stream<ap_uint<17>, OUTSTANDING / 2> br_info_strm;
    hls::stream<ap_uint<17>, OUTSTANDING / 2> br_fb_info_strm;
#pragma HLS bind_storage variable = br_info_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = br_fb_info_strm type = FIFO impl = lutram

    bi_details::bi_parse_pattern(pattern_buf, pattern_id, u_addr_strm, tile_4d_strm, bias_4d_strm, buff_dim_4d_strm,
                                 parse_e_strm);

    details::axim_read_addr_gen<BURST_LEN>(tile_4d_strm, bias_4d_strm, buff_dim_4d_strm, parse_e_strm, raddr_strm,
                                           rlen_strm, gen_e_strm);

    axim_burst_read<W, D, LATENCY, OUTSTANDING>(i_maxi_port, u_addr_strm, raddr_strm, rlen_strm, gen_e_strm,
                                                br_fb_info_strm, o_ack, br_info_strm, waddr_strm, wdata_strm);
    details::axim_br_loopback<LATENCY, OUTSTANDING>(br_info_strm, br_fb_info_strm);
}

/**
 * One of component of BiD-DM, read data from AXI-Stream and cache to DDR/HBM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 *
 * @param i_strm AXI-stream input
 * @param pattern_buf buffer to store 4D pattern against internal URAM cache
 * @param pattern_id identifier to enable which pattern
 * @param waddr_strm write address to access internal URAM
 * @param wdata_strm write data to access internal URAM
 * @param o_ack acknowlege flag at each end of 4D pattern
 */
template <int W, int D>
void axis_to_uram(hls::stream<ap_axiu<W, 0, 0, 0> >& i_strm,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  // output
                  hls::stream<ap_uint<32> >& waddr_strm,
                  hls::stream<ap_uint<W> >& wdata_strm,
                  hls::stream<ap_uint<8> >& o_ack) {
#pragma HLS dataflow
    hls::stream<ap_uint<32>, 32> uram_addr("s2u_uram_addr_strm");
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("s2u_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("s2u_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> buff_dim_4d_strm("s2u_buf_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("s2u_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = buff_dim_4d_strm type = FIFO impl = lutram

    bi_details::bi_parse_pattern(pattern_buf, pattern_id, uram_addr, tile_4d_strm, bias_4d_strm, buff_dim_4d_strm,
                                 parse_e_strm);

    write_uram_addr_gen<W, D>(uram_addr, tile_4d_strm, bias_4d_strm, buff_dim_4d_strm, parse_e_strm, i_strm, waddr_strm,
                              wdata_strm, o_ack);
}

/**
 * One of component of BiD-DM, read data from URAM and offload to DDR/HBM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 * @tparam D depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING write outstanding of MAXI port, should be less than 512.
 *
 * @param o_maxi_port AXI-master interface to access DDR/HBM
 * @param pattern_buf buffer to store 4D pattern against internal URAM cache
 * @param pattern_id identifier to enable which pattern
 * @param u_addr_strm address of data-to-write from URAM
 * @param u_data_strm write data to DDR/HBM from URAM
 * @param o_ack acknowlege flag at each end of 4D pattern
 */
template <int W, int D, int LATENCY, int BURST_LEN, int OUTSTANDING>
void uram_to_axim(hls::burst_maxi<ap_uint<W> > o_maxi_port,
                  ap_uint<32>* pattern_buf,
                  hls::stream<ap_uint<8> >& pattern_id,
                  hls::stream<ap_uint<32>, 32>& raddr_strm, // output
                  hls::stream<ap_uint<W> >& rdata_strm,     // input
                  hls::stream<ap_uint<8> >& o_ack) {
#pragma HLS dataflow
    hls::stream<ap_uint<32>, 8> u_addr_strm("m2s_u_addr_strm");
    hls::stream<ap_uint<4 * 32>, 8> tile_4d_strm("m2s_tile_4d_strm");
    hls::stream<ap_uint<4 * 32>, 8> bias_4d_strm("m2s_bias_4d_strm");
    hls::stream<ap_uint<3 * 32>, 8> buff_dim_4d_strm("m2s_buf_dim_4d_strm");
    hls::stream<ap_uint<2>, 8> parse_e_strm("m2s_parse_e_strm");
#pragma HLS bind_storage variable = tile_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = bias_4d_strm type = FIFO impl = lutram
#pragma HLS bind_storage variable = buff_dim_4d_strm type = FIFO impl = lutram
    hls::stream<ap_uint<32>, 32> u2m_waddr_strm("u2m_waddr_strm"); // write addr for maxi
#pragma HLS bind_storage variable = u2m_waddr_strm type = FIFO impl = lutram
    hls::stream<ap_uint<16>, 32> u2m_blen_strm[2];
#pragma HLS bind_storage variable = u2m_blen_strm type = FIFO impl = lutram
    hls::stream<ap_uint<2>, 32> gen_e_strm[2];

    bi_details::bi_parse_pattern(pattern_buf, pattern_id, u_addr_strm, tile_4d_strm, bias_4d_strm, buff_dim_4d_strm,
                                 parse_e_strm);

    details::axim_write_addr_gen<BURST_LEN>(tile_4d_strm, bias_4d_strm, buff_dim_4d_strm, parse_e_strm, u2m_waddr_strm,
                                            u2m_blen_strm, gen_e_strm);

    bi_flatten_uram_read<D>(u_addr_strm, u2m_blen_strm[0], gen_e_strm[0], raddr_strm);

    details::axim_burst_write<W, LATENCY, OUTSTANDING>(o_maxi_port, rdata_strm, u2m_waddr_strm, u2m_blen_strm[1],
                                                       gen_e_strm[1], o_ack);
}

} // namespace bi_details
} // namespace data_mover
} // namespace xf
#endif
