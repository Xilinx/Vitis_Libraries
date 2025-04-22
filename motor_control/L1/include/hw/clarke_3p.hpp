/*
Copyright (C) 2022-2022, Xilinx, Inc.
Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
#ifndef _CLARKE_PARK_3P_HPP_
#define _CLARKE_PARK_3P_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// Clarke Direct 3p
// Ialpha_out = 2/3*Ia- 1/3*(Ib -Ic)
// Ibeta_out = 2*(Ib-Ic)/sqrt(3)
// Ihomop_out = 2/3*(Ia+Ib+Ic)
// Where Ia+Ib+Ic != 0 ?
// iα and iβ components in an orthogonal reference frame and
// io the homopolar component of the system
//--------------------------------------------------------------------------

/**
 * brief Clarke Direct 3 phase convertion in the form of an inline HLS function
 * tparam T_IO	    Datatype of the input/output ap_fixed. ex. ap_fixed<32,16> is enough for Q16.16
 * param ialpha_out    Ialpha as output of Clarke Direct.
 * param ibeta_out     Ibeta as output of Clarke Direct, orthogonal component of Ialpha.
 * param ihomop_out    Ihomopolar as output of Clarke Direct, homopolar component of the system.
 * param ia_in     Ia as input of Clarke Direct.
 * param ib_in     Ib as input of Clarke Direct.
 * param ib_in     Ib as input of Clarke Direct.
 */
template <class T_IO>
void Clarke_Direct_3p_ap_fixed(
    T_IO& ialpha_out, T_IO& ibeta_out, T_IO& ihomop_out, T_IO ia_in, T_IO ib_in, T_IO ic_in) {
#pragma HLS INLINE off
    // typedef ap_fixed<32 + 4, 16 + 4>  T_MID;
    typedef T_IO T_MID;
    const T_MID sqrt3a = 0.577350; // * 1/SQRT(3)
    T_MID Ia = ia_in;
    T_MID Ib = ib_in;
    T_MID Ic = ic_in;
    T_MID Ialpha, Ibeta, Ihomop;
#pragma HLS BIND_OP variable = Ialpha op = mul impl = dsp
#pragma HLS BIND_OP variable = Ibeta op = mul impl = dsp
#pragma HLS BIND_OP variable = Ihomop op = mul impl = dsp
    Ialpha = (2 * Ia - (Ib + Ic)) / 3;
    Ibeta = (Ib - Ic) * sqrt3a;
    Ihomop = 2 * (Ia + Ib + Ic) / 3;
    ialpha_out = Ialpha;
    ibeta_out = Ibeta;
    ihomop_out = Ihomop;
};

#endif
