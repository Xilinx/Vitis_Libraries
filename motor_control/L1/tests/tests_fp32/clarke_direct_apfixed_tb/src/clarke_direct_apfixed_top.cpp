/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 *
 * Non-template HLS top wrapping Clarke_Direct_3p_ap_fixed<> for synthesis.
 */

#include "ap_fixed.h"
#include "hw/clarke_3p.hpp"

typedef ap_fixed<32, 16> T_IO;

void Clarke_Direct_3p_apfixed_top(
    T_IO& ialpha_out,
    T_IO& ibeta_out,
    T_IO& ihomop_out,
    T_IO ia_in,
    T_IO ib_in,
    T_IO ic_in) {
    Clarke_Direct_3p_ap_fixed<T_IO>(ialpha_out, ibeta_out, ihomop_out, ia_in, ib_in, ic_in);
}
