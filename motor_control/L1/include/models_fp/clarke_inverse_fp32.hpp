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
#ifndef _CLARKE_INVERSE_FP32_HPP_
#define _CLARKE_INVERSE_FP32_HPP_

namespace xf {
namespace motorcontrol {
namespace hls {

//--------------------------------------------------------------------------
// Clarke Inverse 2p to 3p FP32
// Va_out = Valpha
// Vb_out = (-Valpha + sqrt(3) * Vbeta) / 2
// Vc_out = (-Valpha - sqrt(3) * Vbeta) / 2
//--------------------------------------------------------------------------

/**
 * brief Clarke Inverse 2 phase to 3 phase convertion in the form of an inline HLS function
 * param va_out       Va as output of Clarke Inverse.
 * param vb_out       Vb as output of Clarke Inverse.
 * param vc_out       Vc as output of Clarke Inverse.
 * param valpha_in    Valpha as input of Clarke Inverse.
 * param vbeta_in     Vbeta as input of Clarke Inverse.
 */
inline void Clarke_Inverse_2p_fp32(
    float& va_out,
    float& vb_out,
    float& vc_out,
    float valpha_in,
    float vbeta_in) {
#pragma HLS INLINE
#pragma HLS PIPELINE II=1
    const float sqrt3 = 1.732050f;
    const float one_half = 0.5f;
    float Valpha = valpha_in;
    float Vbeta = vbeta_in;
    float Va = Valpha;
    float Vb_temp;
    Vb_temp = Vbeta * sqrt3;
    float Vb = (0.0f - Valpha + Vb_temp) * one_half;
    float Vc_temp;
    Vc_temp = Vbeta * sqrt3;
    float Vc = (0.0f - Valpha - Vc_temp) * one_half;
    va_out = Va;
    vb_out = Vb;
    vc_out = Vc;
}

} // namespace hls
} // namespace motorcontrol
} // namespace xf

#endif
