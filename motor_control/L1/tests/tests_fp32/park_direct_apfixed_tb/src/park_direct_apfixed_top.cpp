/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 *
 * Non-template HLS top wrapping Park_Direct_ap_fixed<> for synthesis.
 */

#include "ap_fixed.h"
#include "hw/park.hpp"

typedef ap_fixed<32, 16> T_IO;
typedef ap_fixed<32, 2> T_SINCOS;

void Park_Direct_apfixed_top(
    T_IO& id_out,
    T_IO& iq_out,
    T_IO ialpha_in,
    T_IO ibeta_in,
    T_SINCOS cos_theta_in,
    T_SINCOS sin_theta_in) {
    Park_Direct_ap_fixed<T_IO, T_SINCOS>(
        id_out, iq_out, ialpha_in, ibeta_in, cos_theta_in, sin_theta_in);
}
