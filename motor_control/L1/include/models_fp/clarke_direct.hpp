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
#ifndef _CLARKE_DIRECT_GOLDEN_HPP_
#define _CLARKE_DIRECT_GOLDEN_HPP_

namespace xf {
namespace motorcontrol {
namespace golden {

//--------------------------------------------------------------------------
// Clarke Direct 3p Golden
// Ialpha_out = (2*Ia - (Ib + Ic)) / 3
// Ibeta_out = (Ib - Ic) * (1/sqrt(3))
// Ihomop_out = 2*(Ia + Ib + Ic) / 3
//--------------------------------------------------------------------------

/**
 * brief Clarke Direct 3 phase golden reference model
 * tparam T          Floating-point type (default: float)
 * param ialpha_out  Ialpha as output of Clarke Direct.
 * param ibeta_out   Ibeta as output of Clarke Direct, orthogonal component of Ialpha.
 * param ihomop_out  Ihomopolar as output of Clarke Direct, homopolar component of the system.
 * param ia_in       Ia as input of Clarke Direct.
 * param ib_in       Ib as input of Clarke Direct.
 * param ic_in       Ic as input of Clarke Direct.
 */
template<typename T = float>
void clarke_direct_golden(
    T& ialpha_out,
    T& ibeta_out,
    T& ihomop_out,
    T ia_in,
    T ib_in,
    T ic_in) {
    const T sqrt3a = static_cast<T>(0.577350);
    T Ia = ia_in;
    T Ib = ib_in;
    T Ic = ic_in;
    T Ialpha, Ibeta, Ihomop;
    Ialpha = (static_cast<T>(2.0) * Ia - (Ib + Ic)) / static_cast<T>(3.0);
    Ibeta = (Ib - Ic) * sqrt3a;
    Ihomop = static_cast<T>(2.0) * (Ia + Ib + Ic) / static_cast<T>(3.0);
    ialpha_out = Ialpha;
    ibeta_out = Ibeta;
    ihomop_out = Ihomop;
}

} // namespace golden
} // namespace motorcontrol
} // namespace xf

#endif
