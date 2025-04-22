/*
Copyright (C) 2022-2022, Xilinx, Inc.
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
SPDX-License-Identifier: X11

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Except as contained in this notice, the name of Advanced Micro Devices
shall not be used in advertising or otherwise to promote the sale,
use or other dealings in this Software without prior written authorization
from Advanced Micro Devices, Inc.
*/
#include "model_foc_int.hpp"

template <class t_int>
void FOC_Simple_2<t_int>::updating(double dt) {
    t_cur += dt;
    int args[FOC_ARGS_SIZE];
    // pid_w.setPara(19115, 256, 0, 0);
    // pid_id.setPara(0, 256, 0, 0);
    // pid_Te.setPara(0, 256, 0, 0);
    args[ARGES_IDX::CONTROL_MODE_ARGS] = FOC_Mode::MOD_SPEED_WITH_TORQUE;
    args[ARGES_IDX::CONTROL_FIXPERIOD_ARGS] = 0;
    args[ARGES_IDX::FLUX_SP_ARGS] = pid_id.m_sp;
    args[ARGES_IDX::FLUX_KP_ARGS] = pid_id.m_kp;
    args[ARGES_IDX::FLUX_KI_ARGS] = pid_id.m_ki;
    args[ARGES_IDX::FLUX_KD_ARGS] = pid_id.m_kd;
    args[ARGES_IDX::TORQUE_SP_ARGS] = pid_Te.m_sp;
    args[ARGES_IDX::TORQUE_KP_ARGS] = pid_Te.m_kp;
    args[ARGES_IDX::TORQUE_KI_ARGS] = pid_Te.m_ki;
    args[ARGES_IDX::TORQUE_KD_ARGS] = pid_Te.m_kd;
    args[ARGES_IDX::RPM_SP_ARGS] = pid_w.m_sp; // 19115; // rpm sp
    args[ARGES_IDX::RPM_KP_ARGS] = pid_w.m_kp;
    args[ARGES_IDX::RPM_KI_ARGS] = pid_w.m_ki;
    args[ARGES_IDX::RPM_KD_ARGS] = pid_w.m_kd;
    args[ARGES_IDX::ANGLE_SH_ARGS] = 0; // ANGLE_SH_REG
    args[ARGES_IDX::VD_ARGS] = 0;       // vd
    args[ARGES_IDX::VQ_ARGS] = 15000;   // vq
    args[ARGES_IDX::CONTROL2_ARGS] = 0; // reg2
    args[ARGES_IDX::FW_KP_ARGS] = 5;    // fw kp
    args[ARGES_IDX::FW_KI_ARGS] = 2;    // fw ki

    short rpm = w;
    short theta_m = (theta_e + 1) / 2;
    short Va, Vb, Vc;
    // clang-format off
        foc_demo2(
            // Input for GPIO
            Ia,             // Phase A current
            Ib,             // Phase B current
            Ic,             // Phase B current
            rpm,            // RPM
            theta_m, // Encoder count
            // Output for GPIO
            Va,
            Vb,
            Vc,
            args);
    // clang-format on

    pid_id.m_din = args[ARGES_IDX::FLUX_DIN_ARGS];
    pid_id.m_acc = args[ARGES_IDX::FLUX_ACC_ARGS];
    pid_id.m_err_pre = args[ARGES_IDX::FLUX_ERR_ARGS];
    pid_id.m_out = args[ARGES_IDX::FLUX_OUT_ARGS];
    pid_Te.m_din = args[ARGES_IDX::TORQUE_DIN_ARGS];
    pid_Te.m_acc = args[ARGES_IDX::TORQUE_ACC_ARGS];
    pid_Te.m_err_pre = args[ARGES_IDX::TORQUE_ERR_ARGS];
    pid_Te.m_out = args[ARGES_IDX::TORQUE_OUT_ARGS];
    pid_w.m_din = args[ARGES_IDX::RPM_DIN_ARGS];
    pid_w.m_acc = args[ARGES_IDX::RPM_ACC_ARGS];
    pid_w.m_err_pre = args[ARGES_IDX::RPM_ERR_ARGS];
    pid_w.m_out = args[ARGES_IDX::RPM_OUT_ARGS];

    out_va = Va;
    out_vb = Vb;
    out_vc = Vc;
}
