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
 * @file bi_dm_control.hpp
 * @brief This file provides handshake between AXIM-to-URAM and URAM-to-AXIS.
 *
 * This file is part of Vitis Utility Library
 */

#ifndef _BI_4D_DM_CONTROL_IMPL_HPP_
#define _BI_4D_DM_CONTROL_IMPL_HPP_

#include <ap_int.h>
#include <ap_axi_sdata.h>
#include <hls_stream.h>
#include <hls_burst_maxi.h>

namespace xf {
namespace data_mover {
namespace bi_details {

enum CtrlOp { MOVE = 0, POP = 1, PUSH = 2, ADD = 3, JUMP = 4, EXIT = 5 };

/**
 * ALU logic inside controllers of bi-directional DM
 *
 * @param pm program memory for ALU
 * @param i_ack_strm ACK from local DM logic(axis_to_axim, etc)
 * @param i_intra_strm sync from local sync_dm
 * @param o_intra_strm sync to local sync_dm
 * @param o_pattern_strm output pattern id
 */
void bi_alu(ap_uint<32>* pm,
            hls::stream<ap_uint<8> >& i_ack_strm,
            hls::stream<ap_uint<8> >& i_intra_strm,
            hls::stream<ap_uint<8> >& o_intra_strm,
            hls::stream<ap_uint<8> >& o_pattern_strm) {
#pragma HLS dataflow
    bool last = false;
    ap_uint<8> R[8] = {0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS array_partition variable = R complete
    ap_uint<8> E = 0;
    ap_uint<16> pc = 0;

BI_PL_ALU_CORE_LOOP:
    while (!last) {
#pragma HLS pipeline II = 1
        ap_uint<32> op = pm[pc];
        ap_uint<5> op_type = op(4, 0);
        ap_uint<3> mode = op(7, 5);
#ifndef __SYNTHESIS__
        std::cout << std::hex << op << std::endl;
#endif
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
                        R[rd] = i_ack_strm.read(); // read ack from data mover unit
                        break;
                    case 1:
                        R[rd] = i_intra_strm.read(); // read sync from local sync_dm
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
                        o_pattern_strm.write(R[rs]); // output ptn_id
                        break;
                    case 1:
                        o_intra_strm.write(R[rs]); // send to local sync_dm
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

        E[4] = !o_intra_strm.full();
        E[3] = !o_pattern_strm.full();
        E[1] = !i_intra_strm.empty();
        E[0] = !i_ack_strm.empty();
    }
}

/**
 * Maintain handshakes among 4-controllers inside A bi-DM
 *
 * @param i_intra_strm sync from local(inside ctrl) ALU logic
 * @param i_inter_strm sync from other ctrl
 * @param o_intra_strm sync to local(inside ctrl) ALU logic
 * @param o_inter_strm sync to other ctrl
 */
void bi_sync_dm(hls::stream<ap_uint<8> >& i_intra_strm,
                hls::stream<ap_uint<8 + 8> >& i_inter_strm,
                hls::stream<ap_uint<8> >& o_intra_strm,
                hls::stream<ap_uint<8 + 8> >& o_inter_strm) {
    ap_uint<8> id;
    ap_uint<4> source_id;          // ID for current ctrl
    ap_uint<4> target_id;          // ID of target for current ctrl
    ap_uint<4> BROADCAST_ID = 0xF; // ID for broadcast

    // count for idles of other ctrls, starts from 0,
    // if idle>=3, it means it's ready to kill current alu(the other ctrls are ready to die).
    ap_uint<2> idle = 0;

    id = i_intra_strm.read();
    source_id = id.range(7, 4); // read ID from ALU(actually obtain from pm)
    target_id = id.range(3, 0);

#ifndef __SYNTHESIS__
    std::string src__0 = std::to_string(source_id) + "->" + std::to_string(target_id);
    std::ofstream fout("sync_dm_" + src__0 + ".txt");
    if (fout.is_open()) {
        fout << "## fout for ctrl_src_id=" << source_id << ", ctrl_target_id=" << target_id << " ##" << std::endl;
    } else {
        std::cerr << "f_Fail" << std::endl;
    }
#endif

    bool last = false;
SYNC_BI_DM_CTRL_0_LOOP:
    while (!last || idle < 3) {
#pragma HLS pipeline off
        ap_uint<8 + 8> d1;
        ap_uint<8> d2;
        if (i_inter_strm.read_nb(d1)) { // sync from other ctrls
            ap_uint<8> tmp;
            if (d1.range(11, 8) == BROADCAST_ID) { // target_id=broadcast(0xF)=>idle(EXIT) signal
                idle += 1;
                tmp = d1.range(7, 0); // the last node in ring(tmp==2) does not forward anymore
                if (tmp < 2) {
                    d1.range(7, 0) = tmp + 1;
                    o_inter_strm.write(d1); // bypass to other ctrls as well
                }
#ifndef __SYNTHESIS__
                fout << "[inter]rev BD_cast from: " << d1.range(15, 12) << std::endl;
                fout << "idle=" << idle << std::endl;
#endif
            } else if (d1.range(11, 8) != source_id) { // id not matched, bypass
#ifndef __SYNTHESIS__
                fout << "[inter]bypass to: " << d1.range(11, 8) << std::endl;
#endif
                o_inter_strm.write(d1);
            } else { // id matched, forward to inside ALU
#ifndef __SYNTHESIS__
                fout << "[inter]id match: " << d1.range(11, 8) << std::endl;
#endif
                o_intra_strm.write(d1.range(7, 0));
            }
        } else if (i_intra_strm.read_nb(d2)) { // sync from inside ALU
            if (d2 == 0xFE) {                  // 0xFE: update-targetID  flag
#ifndef __SYNTHESIS__
                fout << "updating target_id..." << std::endl;
#endif
                ap_uint<8> new_id;
                i_intra_strm.read(new_id);
                target_id = new_id.range(3, 0);
#ifndef __SYNTHESIS__
                fout << "new target_id: " << target_id << std::endl;
#endif
            } else {
                last = (d2 == 0xFF); // 0xFF: end flag
                ap_uint<8 + 8> tmp;
                if (last) { // receive EXIT from ALU(pm), forward this to the other ctrls
#ifndef __SYNTHESIS__
                    fout << "[intral]idle..., then only broadcast" << std::endl;
#endif
                    tmp.range(15, 12) = source_id;
                    tmp.range(11, 8) = BROADCAST_ID;
                    tmp.range(7, 0) = (ap_uint<8>)0;
                    o_inter_strm.write(tmp);
                } else {
#ifndef __SYNTHESIS__
                    fout << "[intral]forward to other ctrl: " << target_id << std::endl;
#endif
                    tmp = (source_id, target_id, d2);
                    tmp.range(15, 12) = source_id;
                    tmp.range(11, 8) = target_id;
                    tmp.range(7, 0) = d2;
                    o_inter_strm.write(tmp);
                }
            }
        }
    }
#ifndef __SYNTHESIS__
    fout << "## Ctrl exit OK ##" << std::endl;
    if (fout.is_open()) {
        fout.close();
    }
#endif
}

/**
 * DM controller for bi-directional data mover
 *
 * @param pm program memory for axim_burst_read control
 * @param i_ack_strm ACK from local axim/axis-uram
 * @param i_inter_strm sync from other ctrl(premier control)
 * @param o_inter_strm sync to other ctrl(next control)
 * @param o_pattern_strm output pattern id
 */
void bi_dm_ctrl(ap_uint<32>* pm,
                hls::stream<ap_uint<8> >& i_ack_strm,
                hls::stream<ap_uint<8 + 8> >& i_inter_strm,
                hls::stream<ap_uint<8 + 8> >& o_inter_strm,
                hls::stream<ap_uint<8> >& o_pattern_strm) {
#pragma HLS dataflow
    hls::stream<ap_uint<8>, 32> sync_i_intra_strm; // sync from inside alu
    hls::stream<ap_uint<8>, 32> sync_o_intra_strm; // sync to inside alu

    bi_details::bi_alu(pm, i_ack_strm, sync_o_intra_strm, sync_i_intra_strm, o_pattern_strm);
    bi_details::bi_sync_dm(sync_i_intra_strm, i_inter_strm, sync_o_intra_strm, o_inter_strm);
}

} // namespace bi_details
} // namespace data_mover
} // namespace xf
#endif
