/*
 * MIT License
 *
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of Advanced Micro Devices, Inc. shall not be used in advertising or
 * otherwise to promote the sale, use or other dealings in this Software without prior written authorization from
 * Advanced Micro Devices, Inc.
 */

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>
#include "vck190_test_harness.hpp"
//#include "pragma_macro.hpp"

const int W = 32;
const int D = 81920;
const int CN = 1;

extern "C" void vck190_test_harness(ap_uint<64>* cfg,
                                    ap_uint<64>* perf,
                                    ap_uint<32>* to_aie_data,
                                    ap_uint<32>* from_aie_data,
                                    hls::stream<ap_axiu<32, 0, 0, 0> >& to_aie_strm0,
                                    hls::stream<ap_axiu<32, 0, 0, 0> >& from_aie_strm0) {
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 port = cfg depth = 8192
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 port = perf depth = 8192
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 port = to_aie_data depth = 8192
#pragma HLS INTERFACE m_axi offset = slave latency = 64 num_write_outstanding = 16 num_read_outstanding = \
    16 max_write_burst_length = 64 max_read_burst_length = 64 bundle = gmem0 port = from_aie_data depth = 8192
#pragma HLS INTERFACE s_axilite port = cfg bundle = control
#pragma HLS INTERFACE s_axilite port = perf bundle = control
#pragma HLS INTERFACE s_axilite port = to_aie_data bundle = control
#pragma HLS INTERFACE s_axilite port = from_aie_data bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
#pragma HLS INTERFACE axis port = to_aie_strm0
#pragma HLS INTERFACE axis port = from_aie_strm0

    test_harness<W, D, CN> inst;
    inst.load_cfg(cfg);
    inst.load_buff(to_aie_data);

    inst.load_store_stream(to_aie_strm0, from_aie_strm0);
    // inst.load_store_stream(to_aie_strm0, from_aie_strm0);

    inst.store_buff(from_aie_data);
    inst.store_perf(perf);
}
