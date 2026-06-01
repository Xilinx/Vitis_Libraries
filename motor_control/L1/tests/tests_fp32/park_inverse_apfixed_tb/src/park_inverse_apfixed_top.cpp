/*
 * Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: X11
 *
 * Non-template HLS top wrapping Park_Inverse_ap_fixed<> for synthesis.
 */

#include "ap_fixed.h"
#include "hw/park.hpp"

typedef ap_fixed<32, 16> T_IO;
typedef ap_fixed<32, 2> T_SINCOS;

void Park_Inverse_apfixed_top(
    T_IO& valpha_out, T_IO& vbeta_out, T_IO vd_in, T_IO vq_in, T_SINCOS cos_theta_in, T_SINCOS sin_theta_in) {
    Park_Inverse_ap_fixed<T_IO, T_SINCOS>(valpha_out, vbeta_out, vd_in, vq_in, cos_theta_in, sin_theta_in);
}
