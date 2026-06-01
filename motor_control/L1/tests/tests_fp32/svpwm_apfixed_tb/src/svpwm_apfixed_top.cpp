/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 *
 * Non-template HLS top wrapping details::calculate_ratios_core<> for synthesis.
 */

#include "ap_fixed.h"
#include "hw/svpwm.hpp"

using namespace xf::motorcontrol;

typedef ap_fixed<32, 16> T_FOC_COM;

void SVPWM_ratios_apfixed_top(ap_ufixed<17, 1>& duty_a,
                              ap_ufixed<17, 1>& duty_b,
                              ap_ufixed<17, 1>& duty_c,
                              T_FOC_COM va_cmd,
                              T_FOC_COM vb_cmd,
                              T_FOC_COM vc_cmd,
                              T_FOC_COM dc_link_adc,
                              int args_dc_link_ref,
                              int args_dc_src_mode) {
    T_FOC_COM Vcmd[3];
    Vcmd[0] = va_cmd;
    Vcmd[1] = vb_cmd;
    Vcmd[2] = vc_cmd;
    ap_ufixed<17, 1> duty_ratio[3];
    details::calculate_ratios_core<T_FOC_COM>(duty_ratio, Vcmd, dc_link_adc, args_dc_link_ref, args_dc_src_mode);
    duty_a = duty_ratio[0];
    duty_b = duty_ratio[1];
    duty_c = duty_ratio[2];
}
