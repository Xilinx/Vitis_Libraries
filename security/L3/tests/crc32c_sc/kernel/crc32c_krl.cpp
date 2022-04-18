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

#include "xf_security/crc32c_krl.hpp"

void crc32c_acc::compute(ap_uint<512>* in_buff, ap_uint<32>* len_buff, ap_uint<32>* out_buff) {
    hls_top(in_buff, len_buff, out_buff);
}

void crc32c_acc::hls_top(ap_uint<512>* in_buff, ap_uint<32>* len_buff, ap_uint<32>* out_buff) {
#pragma HLS INTERFACE m_axi offset = direct latency = 32 num_read_outstanding = 2 max_read_burst_length = 128 bundle = \
    gmem0 port = in_buff
#pragma HLS INTERFACE m_axi offset = direct latency = 32 num_read_outstanding = 2 max_read_burst_length = 128 bundle = \
    gmem1 port = len_buff
#pragma HLS INTERFACE m_axi offset = direct latency = 64 num_write_outstanding = 2 max_write_burst_length = \
    128 bundle = gmem2 port = out_buff

#pragma HLS dataflow

    hls::stream<ap_uint<32> > init_strm("init_strm");
#pragma HLS STREAM variable = init_strm depth = 32
#pragma HLS BIND_STORAGE variable = init_strm type = FIFO impl = LUTRAM
    hls::stream<ap_uint<512> > in_strm("in_strm");
#pragma HLS STREAM variable = in_strm depth = 256
#pragma HLS BIND_STORAGE variable = in_strm type = FIFO impl = BRAM
    hls::stream<ap_uint<32> > len_strm("len_strm");
#pragma HLS STREAM variable = len_strm depth = 32
#pragma HLS BIND_STORAGE variable = len_strm type = FIFO impl = LUTRAM
    hls::stream<bool> end_len_strm("end_len_strm");
#pragma HLS STREAM variable = end_len_strm depth = 32
#pragma HLS BIND_STORAGE variable = end_len_strm type = FIFO impl = SRL
    hls::stream<ap_uint<32> > out_strm("out_strm");
#pragma HLS STREAM variable = out_strm depth = 256
#pragma HLS BIND_STORAGE variable = out_strm type = FIFO impl = BRAM
    hls::stream<bool> end_out_strm("end_out_strm");
#pragma HLS STREAM variable = end_out_strm depth = 256
#pragma HLS BIND_STORAGE variable = end_out_strm type = FIFO impl = SRL

    scan(in_buff, len_buff, in_strm, init_strm, len_strm, end_len_strm);
#ifndef __SYNTHESIS__
    std::cout << "End scan...\n";
    std::cout << "in_strm.size() = " << in_strm.size() << std::endl;
    std::cout << "init_strm.size() = " << init_strm.size() << std::endl;
    std::cout << "len_strm.size() = " << len_strm.size() << std::endl;
    std::cout << "end_len_strm.size() = " << end_len_strm.size() << std::endl;
#endif
    xf::security::crc32c<sizeof(ap_uint<512>)>(init_strm, in_strm, len_strm, end_len_strm, out_strm, end_out_strm);
#ifndef __SYNTHESIS__
    std::cout << "End crc32c...\n";
    std::cout << "init_strm.size() = " << init_strm.size() << std::endl;
    std::cout << "in_strm.size() = " << in_strm.size() << std::endl;
    std::cout << "len_strm.size() = " << len_strm.size() << std::endl;
    std::cout << "end_len_strm.size() = " << end_len_strm.size() << std::endl;
    std::cout << "out_strm.size() = " << out_strm.size() << std::endl;
    std::cout << "end_out_strm.size() = " << end_out_strm.size() << std::endl;
#endif
    write(out_strm, end_out_strm, out_buff);
#ifndef __SYNTHESIS__
    std::cout << "End write...\n";
    std::cout << "init_strm.size() = " << init_strm.size() << std::endl;
    std::cout << "in_strm.size() = " << in_strm.size() << std::endl;
    std::cout << "len_strm.size() = " << len_strm.size() << std::endl;
    std::cout << "end_len_strm.size() = " << end_len_strm.size() << std::endl;
    std::cout << "out_strm.size() = " << out_strm.size() << std::endl;
    std::cout << "end_out_strm.size() = " << end_out_strm.size() << std::endl;
#endif
}
