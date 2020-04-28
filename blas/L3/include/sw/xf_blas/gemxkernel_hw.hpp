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

#ifndef GEMXKERNEL_HW_HPP
#define GEMXKERNEL_HW_HPP

// control
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read)
//        bit 7  - auto_restart (Read/Write)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x10 : Data signal of p_DdrRd_m_Val
//        bit 31~0 - p_DdrRd_m_Val[31:0] (Read/Write)
// 0x14 : Data signal of p_DdrRd_m_Val
//        bit 31~0 - p_DdrRd_m_Val[63:32] (Read/Write)
// 0x18 : reserved
// 0x1c : Data signal of p_DdrWr_m_Val
//        bit 31~0 - p_DdrWr_m_Val[31:0] (Read/Write)
// 0x20 : Data signal of p_DdrWr_m_Val
//        bit 31~0 - p_DdrWr_m_Val[63:32] (Read/Write)
// 0x24 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_AP_CTRL 0x00
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_GIE 0x04
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_IER 0x08
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_ISR 0x0c
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_P_DDRRD_M_VAL_DATA 0x10
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_BITS_P_DDRRD_M_VAL_DATA 64
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_ADDR_P_DDRWR_M_VAL_DATA 0x1c
#define XGEMXKERNEL_0_GEMXKERNEL_0_CONTROL_BITS_P_DDRWR_M_VAL_DATA 64

#endif