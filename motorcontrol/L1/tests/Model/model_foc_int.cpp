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
