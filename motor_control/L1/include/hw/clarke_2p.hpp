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
#ifndef _CLARKE_2P_HPP_
#define _CLARKE_2P_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// Clarke Inverse for 2 phases
// Va_out = Valpha
// Vb_out = [-Valpha + sqrt(3)*Vbeta]/2
// Vc_out = [-Valpha - sqrt(3)*Vbeta]/2
//--------------------------------------------------------------------------
/**
 * brief Clarke Inverse 2 phase convertion in the form of an inline HLS function
 * tparam T_IO	    Datatype of the input/output ap_fixed. ex. ap_fixed<32,16> is enough for Q16.16
 * param va_out    Va as output of Clarke Inverse.
 * param vb_out    Vb as output of Clarke Inverse.
 * param vc_out    Vc as output of Clarke Inverse.
 * param valpha_in Valpha as input of Clarke Inverse.
 * param vbeta_in  Vbeta as input of Clarke Inverse.
 */
template <class T_IO>
void Clarke_Inverse_2p_ap_fixed(T_IO& va_out,
                                T_IO& vb_out,
                                T_IO& vc_out,
                                T_IO valpha_in,
                                T_IO vbeta_in) //;
{
#pragma HLS INLINE off
    // typedef ap_fixed<32 + 3, 16 + 3>  T_MID;
    typedef T_IO T_MID;
    const T_MID sqrt3 = 1.732050;
    T_MID Valpha = valpha_in;
    T_MID Vbeta = vbeta_in;
    va_out = Valpha;
    vb_out = (0 - Valpha + Vbeta * sqrt3) / 2;
    vc_out = (0 - Valpha - Vbeta * sqrt3) / 2;
};

#endif
