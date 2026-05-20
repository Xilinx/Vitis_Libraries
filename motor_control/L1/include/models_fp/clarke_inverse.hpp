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
#ifndef _CLARKE_INVERSE_GOLDEN_HPP_
#define _CLARKE_INVERSE_GOLDEN_HPP_

namespace xf {
namespace motorcontrol {
namespace golden {

//--------------------------------------------------------------------------
// Clarke Inverse 2p to 3p Golden
// Va_out = Valpha
// Vb_out = (-Valpha + sqrt(3) * Vbeta) / 2
// Vc_out = (-Valpha - sqrt(3) * Vbeta) / 2
//--------------------------------------------------------------------------

/**
 * brief Clarke Inverse 2 phase to 3 phase golden reference model
 * tparam T         Floating-point type (default: float)
 * param va_out     Va as output of Clarke Inverse.
 * param vb_out     Vb as output of Clarke Inverse.
 * param vc_out     Vc as output of Clarke Inverse.
 * param valpha_in  Valpha as input of Clarke Inverse.
 * param vbeta_in   Vbeta as input of Clarke Inverse.
 */
template<typename T = float>
void clarke_inverse_golden(
    T& va_out,
    T& vb_out,
    T& vc_out,
    T valpha_in,
    T vbeta_in) {
    const T sqrt3 = static_cast<T>(1.732050);
    T Valpha = valpha_in;
    T Vbeta = vbeta_in;
    va_out = Valpha;
    vb_out = (static_cast<T>(0.0) - Valpha + Vbeta * sqrt3) / static_cast<T>(2.0);
    vc_out = (static_cast<T>(0.0) - Valpha - Vbeta * sqrt3) / static_cast<T>(2.0);
}

} // namespace golden
} // namespace motorcontrol
} // namespace xf

#endif
