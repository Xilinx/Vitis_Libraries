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
#ifndef _PARK_INVERSE_FP32_HPP_
#define _PARK_INVERSE_FP32_HPP_

namespace xf {
namespace motorcontrol {
namespace hls {

//--------------------------------------------------------------------------
// Park Inverse FP32
// Valpha_out = Vd_in*cos(Theta_in) - Vq_in*sin(Theta_in)
// Vbeta_out = Vq_in*cos(Theta_in) + Vd_in*sin(Theta_in)
//--------------------------------------------------------------------------

/**
 * brief Park Inverse convertion in the form of an inline HLS function
 * param valpha_out     Valpha as output of Park Inverse.
 * param vbeta_out      Vbeta as output of Park Inverse.
 * param vd_in          Vd as input of Park Inverse.
 * param vq_in          Vq as input of Park Inverse.
 * param cos_theta_in   Value of cos(theta) as input of Park Inverse.
 * param sin_theta_in   Value of sin(theta) as input of Park Inverse.
 */
inline void Park_Inverse_fp32(
    float& valpha_out, float& vbeta_out, float vd_in, float vq_in, float cos_theta_in, float sin_theta_in) {
#pragma HLS INLINE
#pragma HLS PIPELINE II = 1
    float Vd = vd_in;
    float Vq = vq_in;
    float cos_theta = cos_theta_in;
    float sin_theta = sin_theta_in;
    float Vd_cos, Vq_sin, Vq_cos, Vd_sin, va_inv, vb_inv;
    Vd_cos = Vd * cos_theta;
    Vq_sin = Vq * sin_theta;
    Vq_cos = Vq * cos_theta;
    Vd_sin = Vd * sin_theta;
    va_inv = Vd_cos - Vq_sin;
    vb_inv = Vd_sin + Vq_cos;
    valpha_out = va_inv;
    vbeta_out = vb_inv;
}

} // namespace hls
} // namespace motorcontrol
} // namespace xf

#endif
