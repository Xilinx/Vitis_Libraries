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
 * @file bi_pl_4d_data_mover.hpp
 * @brief This file provides load data between axim/axis in both direction
 *
 * This file is part of Vitis Utility Library
 */

#ifndef XF_UTILS_HW_BI_PL_4D_DATAMOVER_HPP
#define XF_UTILS_HW_BI_PL_4D_DATAMOVER_HPP

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

/***** Disable pl_controller in SYNTHESIS *****/
#ifndef __SYNTHESIS__

#ifndef __SYNTHESIS__
template <typename T>
using maxi = hls::burst_maxi<T>;
#else
#include <stddef.h>
#include <stdint.h>

// Same to synthesis model, but allows to init from pointer
template <typename T>
class maxi {
   public:
    maxi(T* p) : Ptr(p) {
#pragma HLS inline
    }

    void read_request(size_t offset, unsigned len) {
#pragma HLS inline
        __fpga_maxi_read_req(&Ptr[offset], len);
    }

    T read() {
#pragma HLS inline
        T Tmp;
        __fpga_maxi_read(Ptr, &Tmp);
        return Tmp;
    }

    void write_request(size_t offset, unsigned len) {
#pragma HLS inline
        __fpga_maxi_write_req(&Ptr[offset], len);
    }

    void write(const T& val, ap_int<sizeof(T)> byte_enable_mask = -1) {
#pragma HLS inline
        __fpga_maxi_write(Ptr, &val, &byte_enable_mask);
    }

    void write_response() {
#pragma HLS inline
        __fpga_maxi_write_resp(Ptr);
    }

    // private:
    volatile T* Ptr;
};
#endif

#ifndef __SYNTHESIS__
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <unistd.h>
#define HLS_STREAM_THREAD_SAFE
std::vector<std::thread> csim_threads;
#define STD_REF(X) std::ref(X)
#define COMMA ,
#define RUN_IN_THREAD(F, ...)                                \
    do {                                                     \
        hls::stream_globals<false>::incr_task_counter();     \
        csim_threads.push_back(std::thread(F, __VA_ARGS__)); \
    } while (0)
#define JOIN_THREADS()                           \
    do {                                         \
        for (auto& th : csim_threads) th.join(); \
    } while (0)
#else
#define STD_REF(X) X
#define COMMA ,
#define RUN_IN_THREAD(F, ...) F(__VA_ARGS__)
#define JOIN_THREADS()
#endif

#endif
/***** Disable pl_controller in SYNTHESIS *****/

#include "xf_data_mover/bi_dm_4d_uram.hpp"
#include "xf_data_mover/bi_dm_control.hpp"

namespace xf {
namespace data_mover {
namespace bi_details {

/**
 * Load pattern and program memory for bi-DM
 *
 * @tparam W width of AXI-master/URAM/AXI-stream port
 *
 * @param cfg_port burst_maxi<> port of patterns and program memory
 * @param pattern_0 pattern for axis_to_uram
 * @param pattern_1 pattern for uram_to_axim
 * @param pattern_2 pattern for uram_to_axis
 * @param pattern_3 pattern for axim_to_uram
 * @param pm_0 program memory for ctrl of axis_to_uram
 * @param pm_1 program memory for ctrl of uram_to_axim
 * @param pm_2 program memory for ctrl of uram_to_axis
 * @param pm_3 program memory for ctrl of axim_to_uram
 */
template <int W>
void bi_load_cfg(hls::burst_maxi<ap_uint<W> > cfg_port,
                 ap_uint<32>* pattern_0,
                 ap_uint<32>* pattern_1,
                 ap_uint<32>* pattern_2,
                 ap_uint<32>* pattern_3,
                 ap_uint<32>* pm_0,
                 ap_uint<32>* pm_1,
                 ap_uint<32>* pm_2,
                 ap_uint<32>* pm_3) {
    const int elem_of_word = W / 32;

    ap_uint<32> rd_ptr = 0;
    for (ap_uint<4> t = 0; t < 8; t++) {
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
                target = pattern_2;
                break;
            }
            case 3: {
                target = pattern_3;
                break;
            }
            case 4: {
                target = pm_0;
                break;
            }
            case 5: {
                target = pm_1;
                break;
            }
            case 6: {
                target = pm_2;
                break;
            }
            case 7: {
                target = pm_3;
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
            // #pragma HLS unroll factor = 2
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
 * Universal wrapper of bi-directional data mover
 * @tparam WDATA
 * @tparam CACHE_DEPTH
 * @tparam LATENCY
 * @tparam BURST_LEN
 * @tparam OUTSTANDING
 *
 * @param i_axis_strm AXIS-Stream to read data from, and cache it to Cache
 * @param o_axis_strm AXIS-Stream to write data to, and forward it to AIE
 * @param i_maxi_port MAXI port for axim_to_uram unit
 * @param o_maxi_port MAXI port for uram_to_axim unit
 * @param pattern_0 pattern for axis_to_uram
 * @param pattern_1 pattern for uram_to_axim
 * @param pattern_2 pattern for uram_to_axis
 * @param pattern_3 pattern for axim_to_uram
 * @param pm_0 program memory for ctrl of axis_to_uram
 * @param pm_1 program memory for ctrl of uram_to_axim
 * @param pm_2 program memory for ctrl of uram_to_axis
 * @param pm_3 program memory for ctrl of axim_to_uram
 */
template <int WDATA, int CACHE_DEPTH, int LATENCY, int BURST_LEN, int OUTSTANDING>
void bi_dm(
#ifdef __SYNTHESIS__
    hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm,
    hls::stream<ap_axiu<WDATA, 0, 0, 0> >& o_axis_strm,
    hls::burst_maxi<ap_uint<WDATA> > i_maxi_port,
    hls::burst_maxi<ap_uint<WDATA> > o_maxi_port,
#endif
    ap_uint<32>* pattern_0,
    ap_uint<32>* pattern_1,
    ap_uint<32>* pattern_2,
    ap_uint<32>* pattern_3,
    ap_uint<32>* pm_0,
    ap_uint<32>* pm_1,
    ap_uint<32>* pm_2,
    ap_uint<32>* pm_3) {
#pragma HLS dataflow

    // sync between ctrls
    hls::stream<ap_uint<8 + 8>, 8> ctrl_sync_s2s_strm;
    hls::stream<ap_uint<8 + 8>, 8> ctrl_sync_s2m_strm;
    hls::stream<ap_uint<8 + 8>, 8> ctrl_sync_m2m_strm;
    hls::stream<ap_uint<8 + 8>, 8> ctrl_sync_m2s_strm;

    // ctrl-0
    hls::stream<ap_uint<8>, 8> c0_ack_strm;
    hls::stream<ap_uint<8>, 8> c0_pattern_strm;
    // ctrl-1
    hls::stream<ap_uint<8>, 8> c1_ack_strm;
    hls::stream<ap_uint<8>, 8> c1_pattern_strm;
    // ctrl-2
    hls::stream<ap_uint<8>, 8> c2_ack_strm;
    hls::stream<ap_uint<8>, 8> c2_pattern_strm;
    // ctrl-3
    hls::stream<ap_uint<8>, 8> c3_ack_strm;
    hls::stream<ap_uint<8>, 8> c3_pattern_strm;

/* for CSIM */
#ifndef __SYNTHESIS__
    std::cout << "## Test in CSIM_THREADS ##" << std::endl;
    hls::stream<ap_uint<8>, 32> c0_sync_i_intra_strm;
    hls::stream<ap_uint<8>, 32> c0_sync_o_intra_strm;
    hls::stream<ap_uint<8>, 32> c1_sync_i_intra_strm;
    hls::stream<ap_uint<8>, 32> c1_sync_o_intra_strm;
    hls::stream<ap_uint<8>, 32> c2_sync_i_intra_strm;
    hls::stream<ap_uint<8>, 32> c2_sync_o_intra_strm;
    hls::stream<ap_uint<8>, 32> c3_sync_i_intra_strm;
    hls::stream<ap_uint<8>, 32> c3_sync_o_intra_strm;

    // ctrl-0
    RUN_IN_THREAD(bi_details::bi_alu, STD_REF(pm_0), STD_REF(c0_ack_strm), STD_REF(c0_sync_o_intra_strm),
                  STD_REF(c0_sync_i_intra_strm), STD_REF(c0_pattern_strm));

    RUN_IN_THREAD(bi_details::bi_sync_dm, STD_REF(c0_sync_i_intra_strm), STD_REF(ctrl_sync_m2s_strm),
                  STD_REF(c0_sync_o_intra_strm), STD_REF(ctrl_sync_s2s_strm));

    // ctrl-1
    RUN_IN_THREAD(bi_details::bi_alu, STD_REF(pm_1), STD_REF(c1_ack_strm), STD_REF(c1_sync_o_intra_strm),
                  STD_REF(c1_sync_i_intra_strm), STD_REF(c1_pattern_strm));

    RUN_IN_THREAD(bi_details::bi_sync_dm, STD_REF(c1_sync_i_intra_strm), STD_REF(ctrl_sync_m2m_strm),
                  STD_REF(c1_sync_o_intra_strm), STD_REF(ctrl_sync_m2s_strm));

    // ctrl-2
    RUN_IN_THREAD(bi_details::bi_alu, STD_REF(pm_2), STD_REF(c2_ack_strm), STD_REF(c2_sync_o_intra_strm),
                  STD_REF(c2_sync_i_intra_strm), STD_REF(c2_pattern_strm));

    RUN_IN_THREAD(bi_details::bi_sync_dm, STD_REF(c2_sync_i_intra_strm), STD_REF(ctrl_sync_s2s_strm),
                  STD_REF(c2_sync_o_intra_strm), STD_REF(ctrl_sync_s2m_strm));

    // ctrl-3
    RUN_IN_THREAD(bi_details::bi_alu, STD_REF(pm_3), STD_REF(c3_ack_strm), STD_REF(c3_sync_o_intra_strm),
                  STD_REF(c3_sync_i_intra_strm), STD_REF(c3_pattern_strm));

    RUN_IN_THREAD(bi_details::bi_sync_dm, STD_REF(c3_sync_i_intra_strm), STD_REF(ctrl_sync_s2m_strm),
                  STD_REF(c3_sync_o_intra_strm), STD_REF(ctrl_sync_m2m_strm));

    JOIN_THREADS();

#else
    // bind storage
    hls::stream<ap_uint<32>, 32> s2u_waddr_strm("s2u_w_addr_strm");
    hls::stream<ap_uint<WDATA>, 64> s2u_wdata_strm("s2u_w_data_strm");

    hls::stream<ap_uint<32>, 32> u2m_raddr_strm("u2m_r_addr_strm");
    hls::stream<ap_uint<WDATA>, 4096> u2m_rdata_strm("u2m_r_data_strm");
#pragma HLS bind_storage variable = u2m_rdata_strm type = FIFO impl = URAM

    hls::stream<ap_uint<32>, 32> u2s_raddr_strm("u2s_r_addr_strm");
    hls::stream<ap_uint<WDATA>, 64> u2s_rdata_strm("u2s_r_data_strm");

    hls::stream<ap_uint<32>, 32> m2u_waddr_strm("m2u_w_addr_strm");
    hls::stream<ap_uint<WDATA>, 64> m2u_wdata_strm("m2u_w_data_strm");

#pragma HLS dataflow
    bi_details::bi_dm_ctrl(pm_0, c0_ack_strm, ctrl_sync_m2s_strm, ctrl_sync_s2s_strm, c0_pattern_strm);
    axis_to_uram<WDATA, CACHE_DEPTH>(i_axis_strm, pattern_0, c0_pattern_strm, s2u_waddr_strm, s2u_wdata_strm,
                                     c0_ack_strm);

    bi_details::bi_dm_ctrl(pm_1, c1_ack_strm, ctrl_sync_m2m_strm, ctrl_sync_m2s_strm, c1_pattern_strm);
    uram_to_axim<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(o_maxi_port, pattern_1, c1_pattern_strm,
                                                                      u2m_raddr_strm, u2m_rdata_strm, c1_ack_strm);

    bi_details::bi_dm_ctrl(pm_2, c2_ack_strm, ctrl_sync_s2s_strm, ctrl_sync_s2m_strm, c2_pattern_strm);
    uram_to_axis<WDATA, CACHE_DEPTH>(o_axis_strm, pattern_2, c2_pattern_strm, u2s_raddr_strm, u2s_rdata_strm,
                                     c2_ack_strm);

    bi_details::bi_dm_ctrl(pm_3, c3_ack_strm, ctrl_sync_s2m_strm, ctrl_sync_m2m_strm, c3_pattern_strm);
    axim_to_uram<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(i_maxi_port, pattern_3, c3_pattern_strm,
                                                                      m2u_waddr_strm, m2u_wdata_strm, c3_ack_strm);

    bi_uram_access<WDATA, CACHE_DEPTH>(s2u_waddr_strm, s2u_wdata_strm, m2u_waddr_strm, m2u_wdata_strm, u2s_raddr_strm,
                                       u2s_rdata_strm, u2m_raddr_strm, u2m_rdata_strm);
#endif
}

/**
 * Top wrapper of Bi-directional DM, read data from AXI-stream and offload to DDR/HBM
 *
 * @tparam WDATA width of AXI-master/URAM/AXI-stream port
 * @tparam CACHE_DEPTH depth of external URAM.
 * @tparam LATENCY latency of MAXI port.
 * @tparam BURST_LEN burst length of MAXI port.
 * @tparam OUTSTANDING write outstanding of MAXI port, should be less than 512.
 *
 * @param cfg_port AXI-master port for congfiguration
 * @param i_maxi_port AXI-master port for data movement, from DDR to on-chip cache
 * @param o_maxi_port AXI-master port for data movement, from on-chip cache to DDR
 * @param i_axis_strm AXI-Stream port to load data from AXI-Stream to on-chip cache
 * @param o_axis_strm AXI-Stream port to forward data from on-chip cache to AXI-Stream
 */
template <int WDATA, int CACHE_DEPTH, int LATENCY, int BURST_LEN, int OUTSTANDING>
void bi_data_mover(hls::burst_maxi<ap_uint<WDATA> > cfg_port,
                   hls::burst_maxi<ap_uint<WDATA> > i_maxi_port,
                   hls::burst_maxi<ap_uint<WDATA> > o_maxi_port,
                   hls::stream<ap_axiu<WDATA, 0, 0, 0> >& i_axis_strm,
                   hls::stream<ap_axiu<WDATA, 0, 0, 0> >& o_axis_strm) {
    ap_uint<32> pattern_0[1024];
    ap_uint<32> pattern_1[1024];
    ap_uint<32> pattern_2[1024];
    ap_uint<32> pattern_3[1024];
    ap_uint<32> pm_0[1024];
    ap_uint<32> pm_1[1024];
    ap_uint<32> pm_2[1024];
    ap_uint<32> pm_3[1024];
#pragma HLS bind_storage variable = pattern_0 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pattern_1 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pattern_2 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pattern_3 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_0 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_1 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_2 type = RAM_2P impl = bram
#pragma HLS bind_storage variable = pm_3 type = RAM_2P impl = bram
    // load program & pattern
    bi_details::bi_load_cfg<WDATA>(cfg_port, pattern_0, pattern_1, pattern_2, pattern_3, pm_0, pm_1, pm_2, pm_3);
    // Emit the pattern
    bi_details::bi_dm<WDATA, CACHE_DEPTH, LATENCY, BURST_LEN, OUTSTANDING>(i_axis_strm, o_axis_strm, i_maxi_port,
                                                                           o_maxi_port, pattern_0, pattern_1, pattern_2,
                                                                           pattern_3, pm_0, pm_1, pm_2, pm_3);
}

} // namespace bi_details
} // namespace data_mover
} // namespace xf
#endif
