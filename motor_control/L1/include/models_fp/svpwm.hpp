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
#ifndef _SVPWM_GOLDEN_HPP_
#define _SVPWM_GOLDEN_HPP_

#include <cmath>
#include <algorithm>

namespace xf {
namespace motorcontrol {
namespace golden {

//--------------------------------------------------------------------------
// SVPWM Golden
// Voff = (Vmin + Vmax) / 2
// Va_saddle = Va - Voff
// Vb_saddle = Vb - Voff
// Vc_saddle = Vc - Voff
// duty_ratio = (V_saddle + dc_link) / (2 * dc_link)
//--------------------------------------------------------------------------

template <typename T = float>
struct SVPWMOutput {
    T duty_a;
    T duty_b;
    T duty_c;
    T voff;
    int sector;
    T va_saddle;
    T vb_saddle;
    T vc_saddle;

    SVPWMOutput() : duty_a(0), duty_b(0), duty_c(0), voff(0), sector(0), va_saddle(0), vb_saddle(0), vc_saddle(0) {}
};

template <typename T>
inline T get_voff(T va, T vb, T vc) {
    T vmin = std::min({va, vb, vc});
    T vmax = std::max({va, vb, vc});
    return (vmin + vmax) / static_cast<T>(2.0);
}

template <typename T>
inline T get_duty_ratio(T v_saddle, T dc_link) {
    T max_val = dc_link;
    T min_val = -dc_link;
    if (v_saddle > max_val) v_saddle = max_val;
    if (v_saddle < min_val) v_saddle = min_val;
    T vp_saddle = v_saddle + max_val;
    T ratio = (vp_saddle / static_cast<T>(2.0)) / max_val;
    if (ratio >= static_cast<T>(1.0)) ratio = static_cast<T>(0.99999);
    if (ratio < static_cast<T>(0.0)) ratio = static_cast<T>(0.0);
    return ratio;
}

template <typename T>
inline int determine_sector(T valpha, T vbeta) {
    T angle = std::atan2(vbeta, valpha);
    if (angle < 0) angle += static_cast<T>(2.0 * M_PI);
    int sector = static_cast<int>(angle / (M_PI / static_cast<T>(3.0))) + 1;
    if (sector > 6) sector = 6;
    if (sector < 1) sector = 1;
    return sector;
}

/**
 * brief SVPWM duty cycle calculation golden reference model
 * tparam T        Floating-point type (default: float)
 * param output    Output structure with duty cycles
 * param va_cmd    Phase A voltage command
 * param vb_cmd    Phase B voltage command
 * param vc_cmd    Phase C voltage command
 * param dc_link   DC link voltage
 */
template <typename T = float>
void svpwm_golden(SVPWMOutput<T>& output, T va_cmd, T vb_cmd, T vc_cmd, T dc_link) {
    T voff = get_voff(va_cmd, vb_cmd, vc_cmd);
    T va_saddle = va_cmd - voff;
    T vb_saddle = vb_cmd - voff;
    T vc_saddle = vc_cmd - voff;
    T duty_a = get_duty_ratio(va_saddle, dc_link);
    T duty_b = get_duty_ratio(vb_saddle, dc_link);
    T duty_c = get_duty_ratio(vc_saddle, dc_link);
    const T SQRT3_INV = static_cast<T>(0.57735026919);
    T valpha = va_cmd;
    T vbeta = SQRT3_INV * (vb_cmd - vc_cmd);
    int sector = determine_sector(valpha, vbeta);
    output.duty_a = duty_a;
    output.duty_b = duty_b;
    output.duty_c = duty_c;
    output.voff = voff;
    output.sector = sector;
    output.va_saddle = va_saddle;
    output.vb_saddle = vb_saddle;
    output.vc_saddle = vc_saddle;
}

} // namespace golden
} // namespace motorcontrol
} // namespace xf

#endif
