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
 * @file pl_4d_data_mover.hpp
 * @brief This file provides load data from AXI master to AXI stream and vice versa.
 *
 * This file is part of Vitis Utility Library
 */

#ifndef XF_UTILS_HW_PL_4D_DATAMOVER_HPP
#define XF_UTILS_HW_PL_4D_DATAMOVER_HPP

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

#include "xf_data_mover/dm_4d_uram.hpp"
#include "xf_data_mover/dm_control.hpp"

namespace xf {
namespace data_mover {
namespace details {

/**
 * Load 4D pattern and program memory
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 *
 * @param pattern_0, 4D pattern for AXIM-side mover
 * @param pattern_1, 4D pattern for AXIS-side mover
 * @param pm_0, program memory for AXIM-side mover
 * @param pm_1, program memory for AXIS-side mover
 */
template <int W>
void load_cfg(hls::burst_maxi<ap_uint<W> > cfg_port,
              ap_uint<32>* pattern_0,
              ap_uint<32>* pattern_1,
              ap_uint<32>* pm_0,
              ap_uint<32>* pm_1) {
    const int elem_of_word = W / 32;

    ap_uint<32> rd_ptr = 0;
    for (ap_uint<3> t = 0; t < 4; t++) {
        ap_uint<32>* target;
        switch (t) {
            case 0: {
                target = pattern_0;
                break;
            }
            case 1: {
                target = pattern_1;
                break;
            }
            case 2: {
                target = pm_0;
                break;
            }
            case 3: {
                target = pm_1;
                break;
            }
        }

        // load header
        cfg_port.read_request(rd_ptr++, 1);
        ap_uint<W> num_of_cmd = cfg_port.read();
        ap_uint<32> nread = (num_of_cmd + elem_of_word - 1) / elem_of_word;
        cfg_port.read_request(rd_ptr, nread);
        rd_ptr += nread;

        ap_uint<8> idx = 0;
        ap_uint<16> wr_ptr = 0;
        ap_uint<W> word;
    LOAD_CFG_CORE_LOOP:
        for (int i = 0; i < num_of_cmd; i++) {
#pragma HLS pipeline II = 1
            //#pragma HLS unroll factor = 2
            if (idx == 0) word = cfg_port.read();
            target[wr_ptr++] = word(idx * 32 + 31, idx * 32);

            if (idx == elem_of_word - 1)
                idx = 0;
            else
                idx++;
        }
    }
}

/**
 * The core of Read DM, read data from DDR/HBM and forward to AXI-stream
 *
 * @tparam WDATA width of AXI-master/URAM/AXI-stream port
 * @tparam CACHE_DEPTH depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 *
 * @param maxi_port, AXI-master port
 * @param pattern_0, 4D pattern for AXIM-side mover
 * @param pattern_1, 4D pattern for AXIS-side mover
 * @param pm_0, program memory for AXIM-side mover
 * @param pm_1, program memory for AXIS-side mover
 * @param i_sync_strm, input sync singal from other DM
 * @param o_sync_strm, output sync singal to other DM
 * @param o_axis_strm, AXI-stream output
 */
template <int WDATA, int CACHE_DEPTH, int LATENCY, int BURST_LEN, int OUTSTANDING>
void read_4D(hls::burst_maxi<ap_uint<WDATA> > maxi_port,
             ap_uint<32>* pattern_0,
             ap_uint<32>* pattern_1,
             ap_uint<32>* pm_0,
             ap_uint<32>* pm_1,
             hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& i_sync_strm,
             hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& o_sync_strm,
             hls::stream<ap_axiu<WDATA, 0, 0, 0> >& o_axis_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<8>, 8> ctrl0_ack_strm;
    hls::stream<ap_uint<8>, 8> ctrl0_pattern_strm;
    hls::stream<ap_uint<8>, 8> ctrl_sync_s2m_strm;
    hls::stream<ap_uint<8>, 8> ctrl_sync_m2s_strm;
    hls::stream<ap_uint<32>, 32> uram_waddr_strm;
#pragma HLS bind_storage variable = uram_waddr_strm type = FIFO impl = lutram
    hls::stream<ap_uint<WDATA>, 32> uram_wdata_strm;
#pragma HLS bind_storage variable = uram_wdata_strm type = FIFO impl = lutram

    dm_ctrl_axim(pm_0, ctrl0_ack_strm, ctrl_sync_s2m_strm, i_sync_strm, ctrl0_pattern_strm, ctrl_sync_m2s_strm,
                 o_sync_strm);
    axim_to_uram<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(maxi_port, pattern_0, ctrl0_pattern_strm,
                                                                      ctrl0_ack_strm, uram_waddr_strm, uram_wdata_strm);

    hls::stream<ap_uint<8>, 8> ctrl1_ack_strm;
    hls::stream<ap_uint<8>, 8> ctrl1_pattern_strm;

    dm_ctrl_axis(pm_1, ctrl1_ack_strm, ctrl_sync_m2s_strm, ctrl1_pattern_strm, ctrl_sync_s2m_strm);
    uram_to_axis<WDATA, CACHE_DEPTH>(uram_waddr_strm, uram_wdata_strm, pattern_1, ctrl1_pattern_strm, ctrl1_ack_strm,
                                     o_axis_strm);
}

/**
 * The core of Write DM, read data from AXI-stream and offload to DDR/HBM
 *
 * @tparam WDATA width of AXI-master/URAM/AXI-stream port
 * @tparam CACHE_DEPTH depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING write outstanding of MAXI port, should be less than 512.
 *
 * @param maxi_port, AXI-master port
 * @param pattern_0, 4D pattern for AXIM-side mover
 * @param pattern_1, 4D pattern for AXIS-side mover
 * @param pm_0, program memory for AXIM-side mover
 * @param pm_1, program memory for AXIS-side mover
 * @param i_sync_strm, input sync singal from other DM
 * @param o_sync_strm, output sync singal to other DM
 * @param i_axis_strm, AXI-stream input
 */
template <int WDATA, int CACHE_DEPTH, int LATENCY, int BURST_LEN, int OUTSTANDING>
void write_4D(hls::burst_maxi<ap_uint<WDATA> > maxi_port,
              ap_uint<32>* pattern_0,
              ap_uint<32>* pattern_1,
              ap_uint<32>* pm_0,
              ap_uint<32>* pm_1,
              hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& i_sync_strm,
              hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& o_sync_strm,
              hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<8>, 8> ctrl0_ack_strm;
    hls::stream<ap_uint<8>, 8> ctrl0_pattern_strm;
    hls::stream<ap_uint<8>, 8> ctrl_sync_s2m_strm;
    hls::stream<ap_uint<8>, 8> ctrl_sync_m2s_strm;
    hls::stream<ap_uint<32>, 32> uram_raddr_strm;
#pragma HLS bind_storage variable = uram_raddr_strm type = FIFO impl = lutram
    hls::stream<ap_uint<WDATA>, 4096> uram_rdata_strm;
#pragma HLS bind_storage variable = uram_rdata_strm type = FIFO impl = URAM

    dm_ctrl_axim(pm_0, ctrl0_ack_strm, ctrl_sync_s2m_strm, i_sync_strm, ctrl0_pattern_strm, ctrl_sync_m2s_strm,
                 o_sync_strm);
    uram_to_axim<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(maxi_port, pattern_0, ctrl0_pattern_strm,
                                                                      uram_rdata_strm, uram_raddr_strm, ctrl0_ack_strm);

    hls::stream<ap_uint<8>, 8> ctrl1_ack_strm;
    hls::stream<ap_uint<8>, 8> ctrl1_pattern_strm;

    dm_ctrl_axis(pm_1, ctrl1_ack_strm, ctrl_sync_m2s_strm, ctrl1_pattern_strm, ctrl_sync_s2m_strm);
    axis_to_uram<WDATA, CACHE_DEPTH>(uram_raddr_strm, i_axis_strm, pattern_1, ctrl1_pattern_strm, uram_rdata_strm,
                                     ctrl1_ack_strm);
}

} // namespace details

/**
 * The top wrapper of Read DM, read data from DDR/HBM and forward to AXI-stream
 *
 * @tparam WDATA width of AXI-master/URAM/AXI-stream port
 * @tparam CACHE_DEPTH depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING read outstanding of MAXI port, should be less than 512.
 *
 * @param cfg_port, AXI-master port for congfiguration
 * @param data_port, AXI-master port for data movement
 * @param i_sync_strm, input sync singal from other DM
 * @param o_sync_strm, output sync singal to other DM
 * @param o_axis_strm, AXI-stream output
 */
template <int WDATA, int CACHE_DEPTH, int LATENCY, int BURST_LEN, int OUTSTANDING>
void ddr_to_stream(hls::burst_maxi<ap_uint<WDATA> > cfg_port,
                   hls::burst_maxi<ap_uint<WDATA> > data_port,
                   hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& i_sync_strm,
                   hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& o_sync_strm,
                   hls::stream<ap_axiu<WDATA, 0, 0, 0> >& o_axis_strm) {
    ap_uint<32> pattern_m2s[1024];
    ap_uint<32> pattern_s2s[1024];
    ap_uint<32> pm_m2s[1024];
    ap_uint<32> pm_s2s[1024];
#pragma HLS bind_storage variable = pattern_m2s type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pattern_s2s type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_m2s type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_s2s type = RAM_2P impl = bram
    // load program & pattern
    details::load_cfg<WDATA>(cfg_port, pattern_m2s, pattern_s2s, pm_m2s, pm_s2s);
    // Emit the pattern
    details::read_4D<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(
        data_port, pattern_m2s, pattern_s2s, pm_m2s, pm_s2s, i_sync_strm, o_sync_strm, o_axis_strm);
}

/**
 * The top wrapper of Write DM, read data from AXI-stream and offload to DDR/HBM
 *
 * @tparam WDATA width of AXI-master/URAM/AXI-stream port
 * @tparam CACHE_DEPTH depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING write outstanding of MAXI port, should be less than 512.
 *
 * @param cfg_port, AXI-master port for congfiguration
 * @param data_port, AXI-master port for data movement
 * @param i_sync_strm, input sync singal from other DM
 * @param o_sync_strm, output sync singal to other DM
 * @param i_axis_strm, AXI-stream input
 */
template <int WDATA, int CACHE_DEPTH, int LATENCY, int BURST_LEN, int OUTSTANDING>
void stream_to_ddr(hls::burst_maxi<ap_uint<WDATA> > cfg_port,
                   hls::burst_maxi<ap_uint<WDATA> > data_port,
                   hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& i_sync_strm,
                   hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& o_sync_strm,
                   hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm) {
    ap_uint<32> pattern_s2m[1024];
    ap_uint<32> pattern_s2s[1024];
    ap_uint<32> pm_s2m[1024];
    ap_uint<32> pm_s2s[1024];
#pragma HLS bind_storage variable = pattern_s2m type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pattern_s2s type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_s2m type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_s2s type = RAM_2P impl = bram
    // load program & pattern
    details::load_cfg<WDATA>(cfg_port, pattern_s2m, pattern_s2s, pm_s2m, pm_s2s);
    // Emit the pattern
    details::write_4D<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(
        data_port, pattern_s2m, pattern_s2s, pm_s2m, pm_s2s, i_sync_strm, o_sync_strm, i_axis_strm);
}

} // namespace data_mover
} // namespace xf
#endif
