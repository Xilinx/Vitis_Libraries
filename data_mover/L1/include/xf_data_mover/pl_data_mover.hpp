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

#ifndef XF_UTILS_HW_PL_DATAMOVER_HPP
#define XF_UTILS_HW_PL_DATAMOVER_HPP

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

namespace xf {
namespace data_mover {
namespace details {

/**
 * A bit more about manual burst / write design
 * Both take "request params + data" and operate on burst_maxi
 * Both will act like "request at first, close request later"
 * This means there'll be requests on the fly, and they need buffer.
 *
 * One important limit to be noticed:
 * burst maxi port will first "cut" request into sub request, if data request ran across 4KB border.
 * Outstanding are upperbound of such sub request, in pragma for burst maxi port
 * To avoid deadlock caused by sending too much request, request from HLS side need to be less than outstanding.
 *
 * In actual design, burst maxi width is no bigger than 64 Bytes (512 bits).
 * If BURSTLEN <= 64, then total size of one quest is less than 64 * 64 = 4096 Byte
 * So actual sub request is at most twice as much as request from HLS side.
 * So we could at least create (outstanding / 2) HLS requests is safe to be handled.
 *
 * But BURSTLEN should not be too small, for best bandwidth, at least 16.
 */

/**
 * Read from stream and create burst write request to DDR/HBM
 *
 * @tparam WDATA width of MAXI port.
 * @tparam LATENCY latency of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 * @tparam BURSTLEN read burst length of MAXI port, should be less than 64.
 *
 * @param r_offset, stream to get offset for burst write
 * @param r_burst, stream ot get length for burst write, should be no bigger BURSTLEN
 * @param e_r, end of write request
 * @param w_data, stream to read data for writing
 * @param data MAXI port for writing
 */

template <int WDATA, int LATENCY, int OUTSTANDING, int BURSTLEN>
void manualBurstWrite( // input
    hls::stream<ap_uint<64> >& r_offset,
    hls::stream<ap_uint<10> >& r_burst,
    hls::stream<bool>& e_r,
    hls::stream<ap_axiu<WDATA, 0, 0, 0> >& w_data,
    // output
    hls::burst_maxi<ap_uint<WDATA> >& data) {
    ap_uint<LATENCY> check = 0;
    ap_uint<10> req_left = OUTSTANDING / 2;
    ap_uint<10> write_left = 0;
    ap_uint<10> burst_record[1024];
    ap_uint<10> rec_head = 0;
    ap_uint<10> rec_tail = 0;
    bool last = e_r.read();

ACC_BURST_REQ_LOOP:
    // TODO: accumulate so many request at first might cause deadlock, to be fixed.
    while (!last && rec_tail != OUTSTANDING / 2) {
#pragma HLS pipeline II = 1
        ap_uint<64> tmp_offset = r_offset.read();
        ap_uint<64> tmp_burst = r_burst.read();
        last = e_r.read();
        burst_record[rec_tail++] = tmp_burst;
        data.write_request(tmp_offset, tmp_burst);
        req_left--;
    }
    if (rec_tail != 0) {
        write_left = burst_record[rec_head++];
    }

BURST_WRITE_LOOP:
    while (!last || write_left != 0 || rec_head != rec_tail) {
#pragma HLS pipeline II = 1
        bool check_l = check[LATENCY - 1];
        check <<= 1;

        if (write_left != 0) { // load data and write if possible
            ap_axiu<WDATA, 0, 0, 0> tmp_data = w_data.read();
            data.write(tmp_data.data); // haha, too many data

            if (--write_left == 0) { // if all data of current write request has been sent
                check[0] = 1;
                if (rec_head != rec_tail) {
                    write_left = burst_record[rec_head++];
                }
            }
        }

        if (check_l) { // if a write request has become old enough
            data.write_response();

            if (!last) {
                ap_uint<64> tmp_offset = r_offset.read();
                ap_uint<64> tmp_burst = r_burst.read();
                last = e_r.read();
                burst_record[rec_tail++] = tmp_burst;
                data.write_request(tmp_offset, tmp_burst);
            } else {
                req_left++;
            }
        }
    }

    while (check != 0) {
        bool check_l = check[LATENCY - 1];
        check <<= 1;
        if (check_l) {
            data.write_response();
            req_left--;
        }
    }
}

/**
 * Create burst read request to DDR/HBM and write to stream
 *
 * @tparam WDATA width of MAXI port.
 * @tparam LATENCY latency of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 * @tparam BURSTLEN read burst length of MAXI port, should be less than 64.
 *
 * @param data MAXI port for reading
 * @param r_offset, stream to get offset for burst read
 * @param r_burst, stream ot get length for burst read, should be no bigger BURSTLEN
 * @param e_r, end of read request
 * @param w_data, stream to write read result
 */

template <int WDATA, int LATENCY, int OUTSTANDING, int BURSTLEN>
void manualBurstRead( // input
    hls::burst_maxi<ap_uint<WDATA> >& data,
    hls::stream<ap_uint<64> >& r_offset,
    hls::stream<ap_uint<10> >& r_burst,
    hls::stream<bool>& e_r,
    // output
    hls::stream<ap_axiu<WDATA, 0, 0, 0> >& w_data) {
    ap_uint<LATENCY> check = 0;             // delay check
    ap_uint<10> req_left = OUTSTANDING / 2; // how many more request could be issued, "/2" is to avoid hang
    ap_uint<10> req_ready = 0;              // how many request should be ready, according to delay check
    ap_uint<10> burst_record[1024];         // burstlen record
    ap_uint<10> rec_head = 0;               // record head
    ap_uint<10> rec_tail = 0;               // record tail
    ap_uint<10> read_left = 0;              // read left in 1 burst
    bool last = e_r.read();

BURST_READ_LOOP:
    while (!last || req_ready != 0 || check != 0) {
#pragma HLS pipeline II = 1
        //#pragma HLS pipeline II = 1

        bool check_l = check[LATENCY - 1];
        check <<= 1;

        if (req_left != 0 && !last) { // if read outstanding is not exhausted, issue more request
            ap_uint<64> tmp_offset = r_offset.read();
            ap_uint<10> tmp_burst = r_burst.read();
            last = e_r.read();

            data.read_request(tmp_offset, tmp_burst);
            check[0] = 1;
            req_left--;
            burst_record[rec_tail++] = tmp_burst;
        }

        if (req_ready != 0 || read_left != 0) { // if there's mature req
            if (read_left == 0) {
                read_left = burst_record[rec_head++] - 1;
                req_ready--;
                req_left++;
            } else {
                read_left--;
            }

            ap_axiu<WDATA, 0, 0, 0> tmp_data;
            tmp_data.data = data.read();
            tmp_data.keep = -1;
            tmp_data.last = 0;
            w_data.write(tmp_data);
        }

        if (check_l) { // if a new request has become old enough
            req_ready++;
        }
    }

    while (read_left != 0) {
#pragma HLS pipeline II = 1
        ap_axiu<WDATA, 0, 0, 0> tmp_data;
        tmp_data.data = data.read();
        tmp_data.keep = -1;
        tmp_data.last = 0;
        read_left--;
        w_data.write(tmp_data);
    }
}

template <int BURSTLEN>
void cmdParser(hls::burst_maxi<ap_uint<64> > descriptor,
               hls::stream<ap_uint<64> >& r_offset,
               hls::stream<ap_uint<10> >& r_burst,
               hls::stream<bool>& e_r) {
    descriptor.read_request(0, 1);
    ap_uint<64> cmd_nums = descriptor.read();
    ap_uint<64> cmd_buf_ptr = 1;

    if (cmd_nums != 0) {
        descriptor.read_request(cmd_buf_ptr, 9);
    }

    for (int cmd_idx = 0; cmd_idx < cmd_nums; cmd_idx++) {
        ap_uint<64> cfg[9];
        for (int i = 0; i < 9; i++) {
            cfg[i] = descriptor.read();
        }
        cmd_buf_ptr += 9;
        descriptor.read_request(cmd_buf_ptr, 9);

        ap_uint<64>& offset = cfg[0];
        ap_uint<64>& i1 = cfg[1];
        ap_uint<64>& d1 = cfg[2];
        ap_uint<64>& i2 = cfg[3];
        ap_uint<64>& d2 = cfg[4];
        ap_uint<64>& i3 = cfg[5];
        ap_uint<64>& d3 = cfg[6];
        ap_uint<64>& i4 = cfg[7];
        ap_uint<64>& d4 = cfg[8];

        ap_uint<64> x_inc;
        if (i1 == 1) {
            x_inc = BURSTLEN;
        } else {
            x_inc = 1;
        }

        for (ap_uint<64> w = 0; w < d4; w++) {
            ap_uint<64> s4 = offset + w * i4;
            for (ap_uint<64> z = 0; z < d3; z++) {
                ap_uint<64> s3 = s4 + z * i3;
                for (ap_uint<64> y = 0; y < d2; y++) {
                    ap_uint<64> s2 = s3 + y * i2;
                    for (ap_uint<64> x = 0; x < d1; x += x_inc) {
#pragma HLS pipeline II = 1
                        ap_uint<64> s1 = s2 + x * i1;

                        ap_uint<10> burst;
                        if (i1 == 1) {
                            if ((x + BURSTLEN) <= d1) {
                                burst = BURSTLEN;
                            } else {
                                burst = d1 - x;
                            }
                        } else {
                            burst = 1;
                        }

                        r_offset.write(s1);
                        r_burst.write(burst);
                        e_r.write(false);
                    }
                }
            }
        }
    }
    e_r.write(true);
}
} // namespace details

/**
 * @brief data_mover for reading multiple 4D cuboids.
 * It will read the descriptors, access data as descriptors demand and feed data to AXI stream.
 * Descriptors are stored in descriptor buffer, which starts with 64 bit integer which represent number of descriptors
 * inside the buffer. It's followed by one or multiple descritpors, each of which consists 9 x 64bits interger, cfg[0],
 * cfg[1] .. cfg[8].
 * All descriptors will be processed one by one, from the first to the last.
 * Each descriptor represent the access pattern of 4D cuboid, which could be treated like a 4-layer nested loop.
 * Please take reference from design internal doc page for details of descriptor format.
 *
 * @tparam WDATA Bit width of data element
 * @tparam LATENCY MAXI port latency, should be the same with pragma setup
 * @tparam OUTSTANDING MAXI port read/write outstanding, should be the same with pragma setup
 * @tparam BURSTLEN MAXI port read/write burst length, should be the same with pragma setup
 *
 * @param descriptor_buffer Buffer that stores one or multiple descriptors.
 * @param data Buffer that contains data to be accessed
 * @param w_data AXI Stream which data will be written to.
 */

template <int WDATA, int LATENCY, int OUTSTANDING, int BURSTLEN>
void read4D(
    // input
    hls::burst_maxi<ap_uint<64> >& descriptor_buffer,
    hls::burst_maxi<ap_uint<WDATA> >& data,
    // ouput
    hls::stream<ap_axiu<WDATA, 0, 0, 0> >& w_data) {
#pragma HLS dataflow
    hls::stream<ap_uint<64> > r_offset("r_offset");
#pragma HLS stream variable = r_offset depth = OUTSTANDING
    hls::stream<ap_uint<10> > r_burst("r_burst");
#pragma HLS stream variable = r_burst depth = OUTSTANDING
    hls::stream<bool> e_r("e_r");
#pragma HLS stream variable = e_r depth = OUTSTANDING

    details::cmdParser<BURSTLEN>(descriptor_buffer, r_offset, r_burst, e_r);
    details::manualBurstRead<WDATA, LATENCY, OUTSTANDING, BURSTLEN>(data, r_offset, r_burst, e_r, w_data);
}

/**
 * @brief data_mover for write multiple 4D cuboids.
 * It will read data from AXI stream, write access data to the address as descriptors demand.
 * Descriptors are stored in descriptor buffer, which starts with 64 bit integer which represent number of descriptors
 * inside the buffer. It's followed by one or multiple descritpors, each of which consists 9 x 64bits interger, cfg[0],
 * cfg[1] .. cfg[8].
 * All descriptors will be processed one by one, from the first to the last.
 * Each descriptor represent the access pattern of 4D cuboid, which could be treated like a 4-layer nested loop.
 * Please take reference from design internal doc page for details of descriptor format.
 *
 * @tparam WDATA Bit width of data element
 * @tparam LATENCY MAXI port latency, should be the same with pragma setup
 * @tparam OUTSTANDING MAXI port read/write outstanding, should be the same with pragma setup
 * @tparam BURSTLEN MAXI port read/write burst length, should be the same with pragma setup
 *
 * @param descriptor_buffer Buffer that stores one or multiple descriptors.
 * @param w_data AXI Stream which data will be written to.
 * @param data Buffer that contains data to be accessed
 */

template <int WDATA, int LATENCY, int OUTSTANDING, int BURSTLEN>
void write4D(
    // input
    hls::burst_maxi<ap_uint<64> >& descriptor_buffer,
    hls::stream<ap_axiu<WDATA, 0, 0, 0> >& w_data,
    // ouput
    hls::burst_maxi<ap_uint<WDATA> >& data) {
#pragma HLS dataflow
    hls::stream<ap_uint<64> > r_offset("r_offset");
#pragma HLS stream variable = r_offset depth = OUTSTANDING
    hls::stream<ap_uint<10> > r_burst("r_burst");
#pragma HLS stream variable = r_burst depth = OUTSTANDING
    hls::stream<bool> e_r("e_r");
#pragma HLS stream variable = e_r depth = OUTSTANDING

    details::cmdParser<BURSTLEN>(descriptor_buffer, r_offset, r_burst, e_r);
    details::manualBurstWrite<WDATA, LATENCY, OUTSTANDING, BURSTLEN>(r_offset, r_burst, e_r, w_data, data);
}

} // namespace data_mover
} // namespace xf

#endif
