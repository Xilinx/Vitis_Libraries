/*
Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
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
#ifndef _SVPWM_FP32_HPP_
#define _SVPWM_FP32_HPP_

#include <cmath>

namespace xf {
namespace motorcontrol {
namespace hls {

//--------------------------------------------------------------------------
// SVPWM FP32
// Voff = (Vmin + Vmax) / 2
// Va_saddle = Va - Voff
// Vb_saddle = Vb - Voff
// Vc_saddle = Vc - Voff
// duty_ratio = (V_saddle + dc_link) / (2 * dc_link)
//--------------------------------------------------------------------------

struct SVPWMOutput_fp32 {
    float duty_a;
    float duty_b;
    float duty_c;
    float voff;
    int sector;
    float va_saddle;
    float vb_saddle;
    float vc_saddle;

    SVPWMOutput_fp32()
        : duty_a(0), duty_b(0), duty_c(0), voff(0), sector(0), va_saddle(0), vb_saddle(0), vc_saddle(0) {}
};

inline float get_voff_fp32(float va, float vb, float vc) {
#pragma HLS INLINE
    float vmin = va;
    if (vb < vmin) vmin = vb;
    if (vc < vmin) vmin = vc;
    float vmax = va;
    if (vb > vmax) vmax = vb;
    if (vc > vmax) vmax = vc;
    float voff = (vmin + vmax) / 2.0f;
    return voff;
}

inline float get_duty_ratio_fp32(float v_saddle, float dc_link) {
#pragma HLS INLINE
    float max_val = dc_link;
    float min_val = -dc_link;
    float v_clamped = v_saddle;
    if (v_clamped > max_val) v_clamped = max_val;
    if (v_clamped < min_val) v_clamped = min_val;
    float vp_saddle = v_clamped + max_val;
    float ratio = (vp_saddle / 2.0f) / max_val;
    if (ratio >= 1.0f) ratio = 0.99999f;
    if (ratio < 0.0f) ratio = 0.0f;
    return ratio;
}

inline int determine_sector_fp32(float valpha, float vbeta) {
#pragma HLS INLINE
    const float PI = 3.14159265359f;
    float angle = std::atan2(vbeta, valpha);
    if (angle < 0.0f) angle += 2.0f * PI;
    int sector = static_cast<int>(angle / (PI / 3.0f)) + 1;
    if (sector > 6) sector = 6;
    if (sector < 1) sector = 1;
    return sector;
}

/**
 * brief SVPWM duty cycle calculation in the form of an inline HLS function
 * param output     Output structure with duty cycles
 * param va_cmd     Phase A voltage command
 * param vb_cmd     Phase B voltage command
 * param vc_cmd     Phase C voltage command
 * param dc_link    DC link voltage
 */
inline void SVPWM_fp32(SVPWMOutput_fp32& output, float va_cmd, float vb_cmd, float vc_cmd, float dc_link) {
#pragma HLS INLINE
#pragma HLS PIPELINE II = 1
    float voff = get_voff_fp32(va_cmd, vb_cmd, vc_cmd);
    float va_saddle = va_cmd - voff;
    float vb_saddle = vb_cmd - voff;
    float vc_saddle = vc_cmd - voff;
    float duty_a = get_duty_ratio_fp32(va_saddle, dc_link);
    float duty_b = get_duty_ratio_fp32(vb_saddle, dc_link);
    float duty_c = get_duty_ratio_fp32(vc_saddle, dc_link);
    const float SQRT3_INV = 0.57735026919f;
    float valpha = va_cmd;
    float vbeta = SQRT3_INV * (vb_cmd - vc_cmd);
    int sector = determine_sector_fp32(valpha, vbeta);
    output.duty_a = duty_a;
    output.duty_b = duty_b;
    output.duty_c = duty_c;
    output.voff = voff;
    output.sector = sector;
    output.va_saddle = va_saddle;
    output.vb_saddle = vb_saddle;
    output.vc_saddle = vc_saddle;
}

} // namespace hls
} // namespace motorcontrol
} // namespace xf

#endif
