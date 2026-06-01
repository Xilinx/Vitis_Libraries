/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 *
 * Non-template HLS top wrapping PID_Control_ap_fixed<> for synthesis.
 */

#include "ap_fixed.h"
#include "hw/pid_control.hpp"

typedef ap_fixed<32, 16> T_IO;
typedef ap_fixed<48, 24> T_ACC;
typedef ap_fixed<32, 16> T_ERR;
typedef ap_fixed<32, 16> T_PID;

void PID_Control_apfixed_top(T_IO& Res_out,
                             T_ACC& I_err_prev,
                             T_ERR& Error_prev,
                             T_IO in_Measured,
                             T_IO Sp,
                             T_PID Kp,
                             T_PID Ki,
                             T_PID Kd,
                             bool mode_change) {
    PID_Control_ap_fixed<T_IO, T_ACC, T_ERR, T_PID>(Res_out, I_err_prev, Error_prev, in_Measured, Sp, Kp, Ki, Kd,
                                                    mode_change);
}
