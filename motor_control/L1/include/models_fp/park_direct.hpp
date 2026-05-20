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
#ifndef _PARK_DIRECT_GOLDEN_HPP_
#define _PARK_DIRECT_GOLDEN_HPP_

#include <cmath>

namespace xf {
namespace motorcontrol {
namespace golden {

//--------------------------------------------------------------------------
// Park Direct Golden
// Id_out = Ialpha_in*cos(Theta_in) + Ibeta_in*sin(Theta_in)
// Iq_out = Ibeta_in*cos(Theta_in) - Ialpha_in*sin(Theta_in)
//--------------------------------------------------------------------------

/**
 * brief Park Direct golden reference model
 * tparam T           Floating-point type (default: float)
 * param id_out       Id as output of Park Direct.
 * param iq_out       Iq as output of Park Direct.
 * param ialpha_in    Ialpha as input of Park Direct.
 * param ibeta_in     Ibeta as input of Park Direct, orthogonal component of Ialpha.
 * param cos_theta_in Value of cos(theta) as input of Park Direct.
 * param sin_theta_in Value of sin(theta) as input of Park Direct.
 */
template<typename T = float>
void park_direct_golden(
    T& id_out,
    T& iq_out,
    T ialpha_in,
    T ibeta_in,
    T cos_theta_in,
    T sin_theta_in) {
    T Ialpha = ialpha_in;
    T Ibeta = ibeta_in;
    T cos_theta = cos_theta_in;
    T sin_theta = sin_theta_in;
    T Ia_cos, Ib_sin, Ib_cos, Ia_sin, Id, Iq;
    Ia_cos = Ialpha * cos_theta;
    Ib_sin = Ibeta * sin_theta;
    Ib_cos = Ibeta * cos_theta;
    Ia_sin = Ialpha * sin_theta;
    Id = Ia_cos + Ib_sin;
    Iq = Ib_cos - Ia_sin;
    id_out = Id;
    iq_out = Iq;
}

} // namespace golden
} // namespace motorcontrol
} // namespace xf

#endif
