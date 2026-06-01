/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 *
 * Non-template HLS top wrapping Clarke_Inverse_2p_ap_fixed<> for synthesis.
 */

#include "ap_fixed.h"
#include "hw/clarke_2p.hpp"

typedef ap_fixed<32, 16> T_IO;

void Clarke_Inverse_2p_apfixed_top(T_IO& va_out, T_IO& vb_out, T_IO& vc_out, T_IO valpha_in, T_IO vbeta_in) {
    Clarke_Inverse_2p_ap_fixed<T_IO>(va_out, vb_out, vc_out, valpha_in, vbeta_in);
}
