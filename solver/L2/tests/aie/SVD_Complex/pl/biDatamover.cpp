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
#include "config.h"
#include "xf_data_mover/bi_pl_4d_data_mover.hpp"

extern "C" void biDatamover(hls::burst_maxi<ap_uint<WDATA> > datamover_cfg,
                            hls::burst_maxi<ap_uint<WDATA> > i_maxi_strm,
                            hls::burst_maxi<ap_uint<WDATA> > o_maxi_strm,
                            hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm,
                            hls::stream<ap_axiu<WDATA, 0, 0, 0> >& o_axis_strm) {
#pragma HLS interface m_axi offset = slave bundle = gmem0_0 port = datamover_cfg max_read_burst_length = \
    32 num_read_outstanding = 16 latency = 32
#pragma HLS interface m_axi offset = slave bundle = gmem0_1 port = i_maxi_strm max_read_burst_length = \
    32 num_read_outstanding = 16 latency = 32
#pragma HLS interface m_axi offset = slave bundle = gmem0_2 port = o_maxi_strm max_write_burst_length = \
    32 num_write_outstanding = 16 latency = 32
#pragma HLS interface s_axilite bundle = control port = datamover_cfg
#pragma HLS interface s_axilite bundle = control port = i_maxi_strm
#pragma HLS interface s_axilite bundle = control port = o_maxi_strm
#pragma HLS interface axis port = i_axis_strm
#pragma HLS interface axis port = o_axis_strm

#pragma HLS interface s_axilite bundle = control port = return
#pragma HLS dataflow
    xf::data_mover::bi_details::bi_data_mover<WDATA, 65536, 16, 32, 16>(datamover_cfg, i_maxi_strm, o_maxi_strm,
                                                                        i_axis_strm, o_axis_strm);
}