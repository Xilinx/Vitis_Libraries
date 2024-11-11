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
 * @file dm_control.hpp
 * @brief This file provides handshake between AXIM-to-URAM and URAM-to-AXIS.
 *
 * This file is part of Vitis Utility Library
 */

#ifndef _4D_DM_CONTROL_IMPL_HPP_
#define _4D_DM_CONTROL_IMPL_HPP_

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

namespace xf {
namespace data_mover {
namespace details {

enum CtrlOp { MOVE = 0, POP = 1, PUSH = 2, ADD = 3, JUMP = 4, EXIT = 5 };

/**
 * One 8-bit ALU to control AXIM read/write with pre-configured program
 *
 * @param pm, program memory of 8-bit ALU, configured before ALU start-up
 * @param i_ack_strm, complete flag of each 4D pattern from AXIM
 * @param i_inter_strm, ingress sync state from internal AXIS
 * @param i_intra_strm, ingress sync state from external DM
 * @param o_pattern_strm, pattern ID for AXIM read/write
 * @param o_inter_strm, egress sync state to internal AXIS
 * @param o_intra_strm, egress sync state to external DM
 */
void alu_for_axim(ap_uint<32> pm[1024],
                  hls::stream<ap_uint<8> >& i_ack_strm,     // from m2s
                  hls::stream<ap_uint<8> >& i_inter_strm,   // from ctrl-1
                  hls::stream<ap_uint<8> >& i_intra_strm,   // from sync
                  hls::stream<ap_uint<8> >& o_pattern_strm, // to m2s
                  hls::stream<ap_uint<8> >& o_inter_strm,   // to ctrl-1
                  hls::stream<ap_uint<8> >& o_intra_strm    // to sync
                  ) {
#pragma HLS dataflow
    bool last = false;
    ap_uint<8> R[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS array_partition variable = R complete
    ap_uint<8> E = 0;
    ap_uint<16> pc = 0;

PL_ALU_CORE_LOOP:
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<32> op = pm[pc];
        ap_uint<5> op_type = op(4, 0);
        ap_uint<3> mode = op(7, 5);

        switch (op_type) {
            case MOVE: {
                ap_uint<4> rd = op(11, 8);
                ap_uint<8> rs = op(19, 12);
                if (mode == 0)
                    R[rd] = R[rs(3, 0)];
                else if (mode == 1)
                    R[rd] = rs;
                pc += 1;
                break;
            }
            case POP: {
                ap_uint<4> rd = op(11, 8);
                switch (mode) {
                    case 0:
                        R[rd] = i_ack_strm.read();
                        break;
                    case 1:
                        R[rd] = i_inter_strm.read();
                        break;
                    case 2:
                        R[rd] = i_intra_strm.read();
                        break;
                    default:
                        break;
                }
                pc += 1;
                break;
            }
            case PUSH: {
                ap_uint<4> rs = op(11, 8);
                switch (mode) {
                    case 0:
                        o_pattern_strm.write(R[rs]);
                        break;
                    case 1:
                        o_inter_strm.write(R[rs]);
                        break;
                    case 2:
                        o_intra_strm.write(R[rs]);
                        break;
                    default:
                        break;
                }
                pc += 1;
                break;
            }
            case ADD: {
                ap_uint<4> rd = op(11, 8);
                ap_uint<8> rs1 = op(19, 12);
                ap_uint<8> rs2 = op(27, 20);
                if (mode == 0)
                    R[rd] = R[rs1] + R[rs2];
                else if (mode == 1)
                    R[rd] = R[rs1] + rs2;
                pc += 1;
                break;
            }
            case JUMP: {
                ap_uint<4> rs1 = op(11, 8);
                ap_uint<4> rs2 = op(15, 12);
                ap_uint<16> imm = op(31, 16);
                if (mode == 0)
                    pc = imm;
                else if (mode == 1)
                    pc = (R[rs1] < R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 2)
                    pc = (R[rs1] >= R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 3)
                    pc = (R[rs1] == R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 4)
                    pc = (R[rs1] != R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 5)
                    pc = (E[rs1]) ? imm : (ap_uint<16>)(pc + 1);
                break;
            }
            case EXIT: {
                last = true;
                break;
            }
            default:
                break;
        }

        E[5] = !o_intra_strm.full();
        E[4] = !o_inter_strm.full();
        E[3] = !o_pattern_strm.full();
        E[2] = !i_intra_strm.empty();
        E[1] = !i_inter_strm.empty();
        E[0] = !i_ack_strm.empty();
    }
}

/**
 * One 8-bit ALU to control AXIS read/write with pre-configured program
 *
 * @param pm, program memory of 8-bit ALU, configured before ALU start-up
 * @param i_ack_strm, complete flag of each 4D pattern from AXIS
 * @param i_inter_strm, ingress sync state from internal AXIM
 * @param o_pattern_strm, pattern ID for AXIS read/write
 * @param o_inter_strm, egress sync state to internal AXIM
 */
void alu_for_axis(ap_uint<32> pm[1024],
                  hls::stream<ap_uint<8> >& i_ack_strm,
                  hls::stream<ap_uint<8> >& i_inter_strm,
                  hls::stream<ap_uint<8> >& o_pattern_strm,
                  hls::stream<ap_uint<8> >& o_inter_strm) {
#pragma HLS dataflow
    bool last = false;
    ap_uint<8> R[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS array_partition variable = R complete
    ap_uint<8> E = 0;
    ap_uint<16> pc = 0;

PL_ALU_CORE_LOOP:
    while (!last) {
#pragma HLS pipeline II = 1
        // #pragma HLS latency min = 1 max = 1
        ap_uint<32> op = pm[pc];
        ap_uint<5> op_type = op(4, 0);
        ap_uint<3> mode = op(7, 5);

        switch (op_type) {
            case MOVE: {
                ap_uint<4> rd = op(11, 8);
                ap_uint<8> rs = op(19, 12);
                if (mode == 0)
                    R[rd] = R[rs(3, 0)];
                else if (mode == 1)
                    R[rd] = rs;
                pc += 1;
                break;
            }
            case POP: {
                ap_uint<4> rd = op(11, 8);
                switch (mode) {
                    case 0:
                        R[rd] = i_ack_strm.read();
                        break;
                    case 1:
                        R[rd] = i_inter_strm.read();
                        break;
                    default:
                        break;
                }
                pc += 1;
                break;
            }
            case PUSH: {
                ap_uint<4> rs = op(11, 8);
                switch (mode) {
                    case 0:
                        o_pattern_strm.write(R[rs]);
                        break;
                    case 1:
                        o_inter_strm.write(R[rs]);
                        break;
                    default:
                        break;
                }
                pc += 1;
                break;
            }
            case ADD: {
                ap_uint<4> rd = op(11, 8);
                ap_uint<8> rs1 = op(19, 12);
                ap_uint<8> rs2 = op(27, 20);
                if (mode == 0)
                    R[rd] = R[rs1] + R[rs2];
                else if (mode == 1)
                    R[rd] = R[rs1] + rs2;
                pc += 1;
                break;
            }
            case JUMP: {
                ap_uint<4> rs1 = op(11, 8);
                ap_uint<4> rs2 = op(15, 12);
                ap_uint<16> imm = op(31, 16);
                if (mode == 0)
                    pc = imm;
                else if (mode == 1)
                    pc = (R[rs1] < R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 2)
                    pc = (R[rs1] >= R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 3)
                    pc = (R[rs1] == R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 4)
                    pc = (R[rs1] != R[rs2]) ? imm : (ap_uint<16>)(pc + 1);
                else if (mode == 5)
                    pc = (E[rs1]) ? imm : (ap_uint<16>)(pc + 1);
                break;
            }
            case EXIT: {
                last = true;
                break;
            }
            default:
                break;
        }

        E[4] = !o_inter_strm.full();
        E[3] = !o_pattern_strm.full();
        E[1] = !i_inter_strm.empty();
        E[0] = !i_ack_strm.empty();
    }
}

/**
 * Translate AXI-stream protocol to hls::stream
 *
 * @param i_cfg_strm, input configuaration (Work mode, monitor ID)
 * @param i_intra_strm, AXI-stream input
 * @param fifo_intra_strm, hls::stream output
 */
void axiu2fifo(hls::stream<ap_uint<8> >& i_cfg_strm,
               hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& i_intra_strm,
               hls::stream<ap_uint<8 + 8> >& fifo_intra_strm) {
    const ap_uint<8> STAND_MODE = 0;
    const ap_uint<8> CORP_MODE = 1;
    ap_uint<8> mode = i_cfg_strm.read();
    ap_uint<8> target_id = i_cfg_strm.read();

    bool last = false;
AXIU_TO_FIFO_LOOP:
    while (!last) {
#pragma HLS pipeline II = 1 style = flp
        ap_axiu<8 + 8, 0, 0, 0> in = i_intra_strm.read();
        last = (mode == STAND_MODE) ? (in.last == 1) : ((in.last == 1) && (in.data.range(15, 8) == target_id));
        if (!last) fifo_intra_strm.write(in.data);
    }
}

/**
 * Handshake interface amongs DMs to filter ingress sync message when ID matched, or just bypass if not matched
 * and be responsible for forward state appending its own ID.
 *
 * @param i_inter_strm, cfg or updated state from upstream ALU, temernated by 0xFF
 * @param i_intra_strm, translated hls::stream from axiu2fifo
 * @param o_inter_strm, external state goto upstream ALU
 * @param o_cfg_strm, configuration to axiu2fifo
 * @param o_intra_strm, egress AXI-stream to other DM
 */
void sync_dm(hls::stream<ap_uint<8> >& i_inter_strm,                // from internal alu, update state or cfg
             hls::stream<ap_uint<8 + 8> >& i_intra_strm,            // from other DMs, sink if matched
             hls::stream<ap_uint<8> >& o_inter_strm,                // to internal alu if target index matched
             hls::stream<ap_uint<8> >& o_cfg_strm,                  // to config th axi-stream to hls-stream
             hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& o_intra_strm) { // to other DMs, bypass or egress own's update)
    const ap_uint<8> STAND_MODE = 0;
    const ap_uint<8> CORP_MODE = 1;
    ap_uint<8> own_id, target_id;
    ap_uint<8> mode = i_inter_strm.read();
    if (mode == CORP_MODE) {
        own_id = i_inter_strm.read();
        target_id = i_inter_strm.read();
    } else {
        own_id = 0;
        target_id = 0;
    }
    o_cfg_strm.write(mode);
    o_cfg_strm.write(target_id);

    bool last = false;
SYNC_DM_CTRL_0_LOOP:
    while (!last) {
#pragma HLS pipeline off
        ap_uint<8 + 8> d1;
        ap_uint<8> d2;
        if (i_intra_strm.read_nb(d1)) {
            ap_axiu<8 + 8, 0, 0, 0> tmp;
            if (mode == STAND_MODE) { // bypass
                tmp.data = d1;
                tmp.keep = -1;
                tmp.last = 0;
                o_intra_strm.write(tmp);
            } else if (d1.range(15, 8) != target_id) { // not matched, bypass
                tmp.data = d1;
                tmp.keep = -1;
                tmp.last = 0;
                o_intra_strm.write(tmp);
            } else // matched, go to ctrl
                o_inter_strm.write(d1.range(7, 0));
        } else if (i_inter_strm.read_nb(d2)) {
            last = (d2 == 0xFF);
            ap_axiu<8 + 8, 0, 0, 0> tmp;
            if (last) {
                tmp.data = (own_id, (ap_uint<8>)0);
                tmp.keep = 0;
                tmp.last = 1;
                o_intra_strm.write(tmp);
            } else if (mode == CORP_MODE) {
                tmp.data = (own_id, d2);
                tmp.keep = -1;
                tmp.last = 0;
                o_intra_strm.write(tmp);
            }
        }
    }
}

/**
 * Wrapper of control logic for AXI-stream module
 *
 * @param pm, program memory of 8-bit ALU, configured before ALU start-up
 * @param i_ack_strm, complete flag of each 4D pattern from AXIS
 * @param i_inter_strm, ingress sync state from internal AXIM
 * @param o_pattern_strm, pattern ID for AXIS read/write
 * @param o_inter_strm, egress sync state to internal AXIM
 */
void dm_ctrl_axis(ap_uint<32> pm[1024],
                  hls::stream<ap_uint<8> >& i_ack_strm,
                  hls::stream<ap_uint<8> >& i_inter_strm,
                  hls::stream<ap_uint<8> >& o_pattern_strm,
                  hls::stream<ap_uint<8> >& o_inter_strm) {
    details::alu_for_axis(pm, i_ack_strm, i_inter_strm, o_pattern_strm, o_inter_strm);
}

/**
 * Wrapper of control logic for AXI-master module
 *
 * @param pm, program memory of 8-bit ALU, configured before ALU start-up
 * @param i_ack_strm, complete flag of each 4D pattern from AXIM
 * @param i_inter_strm, ingress sync state from internal AXIS
 * @param i_intra_strm, ingress sync state from external DM
 * @param o_pattern_strm, pattern ID for AXIM read/write
 * @param o_inter_strm, egress sync state to internal AXIS
 * @param o_intra_strm, egress sync state to external DM
 */
void dm_ctrl_axim(ap_uint<32> pm[1024],
                  hls::stream<ap_uint<8> >& i_ack_strm,
                  hls::stream<ap_uint<8> >& i_inter_strm,
                  hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& i_intra_strm,
                  hls::stream<ap_uint<8> >& o_pattern_strm,
                  hls::stream<ap_uint<8> >& o_inter_strm,
                  hls::stream<ap_axiu<8 + 8, 0, 0, 0> >& o_intra_strm) {
#pragma HLS dataflow

    hls::stream<ap_uint<8>, 32> sync_i_inter_strm;
    hls::stream<ap_uint<8>, 32> sync_o_intra_strm;

    details::alu_for_axim(pm, i_ack_strm, i_inter_strm, sync_o_intra_strm, o_pattern_strm, o_inter_strm,
                          sync_i_inter_strm);

    hls::stream<ap_uint<8>, 4> sync_cfg_strm;
    hls::stream<ap_uint<8 + 8>, 32> fifo_intra_strm;

    details::axiu2fifo(sync_cfg_strm, i_intra_strm, fifo_intra_strm);
    details::sync_dm(sync_i_inter_strm, fifo_intra_strm, sync_o_intra_strm, sync_cfg_strm, o_intra_strm);
}
} // namespace details

} // namespace data_mover
} // namespace xf
#endif
