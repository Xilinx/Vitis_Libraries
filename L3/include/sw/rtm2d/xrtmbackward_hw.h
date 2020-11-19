/*
 * Copyright 2019 Xilinx, Inc.
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
// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2020.1 (64-bit)
// Copyright 1986-2020 Xilinx, Inc. All Rights Reserved.
// ==============================================================
// control
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read)
//        bit 4  - ap_continue (Read/Write/SC)
//        bit 7  - auto_restart (Read/Write)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0  - enable ap_done interrupt (Read/Write)
//        bit 1  - enable ap_ready interrupt (Read/Write)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0  - ap_done (COR/TOW)
//        bit 1  - ap_ready (COR/TOW)
//        others - reserved
// 0x10 : Data signal of p_z
//        bit 31~0 - p_z[31:0] (Read/Write)
// 0x14 : reserved
// 0x18 : Data signal of p_x
//        bit 31~0 - p_x[31:0] (Read/Write)
// 0x1c : reserved
// 0x20 : Data signal of p_t
//        bit 31~0 - p_t[31:0] (Read/Write)
// 0x24 : reserved
// 0x28 : Data signal of p_recz
//        bit 31~0 - p_recz[31:0] (Read/Write)
// 0x2c : reserved
// 0x30 : Data signal of p_rec
//        bit 31~0 - p_rec[31:0] (Read/Write)
// 0x34 : Data signal of p_rec
//        bit 31~0 - p_rec[63:32] (Read/Write)
// 0x38 : reserved
// 0x3c : Data signal of p_coefz
//        bit 31~0 - p_coefz[31:0] (Read/Write)
// 0x40 : Data signal of p_coefz
//        bit 31~0 - p_coefz[63:32] (Read/Write)
// 0x44 : reserved
// 0x48 : Data signal of p_coefx
//        bit 31~0 - p_coefx[31:0] (Read/Write)
// 0x4c : Data signal of p_coefx
//        bit 31~0 - p_coefx[63:32] (Read/Write)
// 0x50 : reserved
// 0x54 : Data signal of p_taperz
//        bit 31~0 - p_taperz[31:0] (Read/Write)
// 0x58 : Data signal of p_taperz
//        bit 31~0 - p_taperz[63:32] (Read/Write)
// 0x5c : reserved
// 0x60 : Data signal of p_taperx
//        bit 31~0 - p_taperx[31:0] (Read/Write)
// 0x64 : Data signal of p_taperx
//        bit 31~0 - p_taperx[63:32] (Read/Write)
// 0x68 : reserved
// 0x6c : Data signal of p_v2dt2
//        bit 31~0 - p_v2dt2[31:0] (Read/Write)
// 0x70 : Data signal of p_v2dt2
//        bit 31~0 - p_v2dt2[63:32] (Read/Write)
// 0x74 : reserved
// 0x78 : Data signal of p_p0
//        bit 31~0 - p_p0[31:0] (Read/Write)
// 0x7c : Data signal of p_p0
//        bit 31~0 - p_p0[63:32] (Read/Write)
// 0x80 : reserved
// 0x84 : Data signal of p_p1
//        bit 31~0 - p_p1[31:0] (Read/Write)
// 0x88 : Data signal of p_p1
//        bit 31~0 - p_p1[63:32] (Read/Write)
// 0x8c : reserved
// 0x90 : Data signal of p_r0
//        bit 31~0 - p_r0[31:0] (Read/Write)
// 0x94 : Data signal of p_r0
//        bit 31~0 - p_r0[63:32] (Read/Write)
// 0x98 : reserved
// 0x9c : Data signal of p_r1
//        bit 31~0 - p_r1[31:0] (Read/Write)
// 0xa0 : Data signal of p_r1
//        bit 31~0 - p_r1[63:32] (Read/Write)
// 0xa4 : reserved
// 0xa8 : Data signal of p_i0
//        bit 31~0 - p_i0[31:0] (Read/Write)
// 0xac : Data signal of p_i0
//        bit 31~0 - p_i0[63:32] (Read/Write)
// 0xb0 : reserved
// 0xb4 : Data signal of p_i1
//        bit 31~0 - p_i1[31:0] (Read/Write)
// 0xb8 : Data signal of p_i1
//        bit 31~0 - p_i1[63:32] (Read/Write)
// 0xbc : reserved
// 0xc0 : Data signal of p_upb
//        bit 31~0 - p_upb[31:0] (Read/Write)
// 0xc4 : Data signal of p_upb
//        bit 31~0 - p_upb[63:32] (Read/Write)
// 0xc8 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XRTMBACKWARD_CONTROL_ADDR_AP_CTRL 0x00
#define XRTMBACKWARD_CONTROL_ADDR_GIE 0x04
#define XRTMBACKWARD_CONTROL_ADDR_IER 0x08
#define XRTMBACKWARD_CONTROL_ADDR_ISR 0x0c
#define XRTMBACKWARD_CONTROL_ADDR_P_Z_DATA 0x10
#define XRTMBACKWARD_CONTROL_BITS_P_Z_DATA 32
#define XRTMBACKWARD_CONTROL_ADDR_P_X_DATA 0x18
#define XRTMBACKWARD_CONTROL_BITS_P_X_DATA 32
#define XRTMBACKWARD_CONTROL_ADDR_P_T_DATA 0x20
#define XRTMBACKWARD_CONTROL_BITS_P_T_DATA 32
#define XRTMBACKWARD_CONTROL_ADDR_P_RECZ_DATA 0x28
#define XRTMBACKWARD_CONTROL_BITS_P_RECZ_DATA 32
#define XRTMBACKWARD_CONTROL_ADDR_P_REC_DATA 0x30
#define XRTMBACKWARD_CONTROL_BITS_P_REC_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_COEFZ_DATA 0x3c
#define XRTMBACKWARD_CONTROL_BITS_P_COEFZ_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_COEFX_DATA 0x48
#define XRTMBACKWARD_CONTROL_BITS_P_COEFX_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_TAPERZ_DATA 0x54
#define XRTMBACKWARD_CONTROL_BITS_P_TAPERZ_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_TAPERX_DATA 0x60
#define XRTMBACKWARD_CONTROL_BITS_P_TAPERX_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_V2DT2_DATA 0x6c
#define XRTMBACKWARD_CONTROL_BITS_P_V2DT2_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_P0_DATA 0x78
#define XRTMBACKWARD_CONTROL_BITS_P_P0_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_P1_DATA 0x84
#define XRTMBACKWARD_CONTROL_BITS_P_P1_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_R0_DATA 0x90
#define XRTMBACKWARD_CONTROL_BITS_P_R0_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_R1_DATA 0x9c
#define XRTMBACKWARD_CONTROL_BITS_P_R1_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_I0_DATA 0xa8
#define XRTMBACKWARD_CONTROL_BITS_P_I0_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_I1_DATA 0xb4
#define XRTMBACKWARD_CONTROL_BITS_P_I1_DATA 64
#define XRTMBACKWARD_CONTROL_ADDR_P_UPB_DATA 0xc0
#define XRTMBACKWARD_CONTROL_BITS_P_UPB_DATA 64
