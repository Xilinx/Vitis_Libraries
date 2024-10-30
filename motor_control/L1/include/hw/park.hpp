/*
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
#ifndef _PARK_HPP_
#define _PARK_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// Park Direct
// Id_out = Ialpha_in*cos(Theta_in) + Ibeta_in*sin(Theta_in)
// Iq_out = Ibeta_in*cos(Theta_in) - Ialpha_in*sin(Theta_in)
//--------------------------------------------------------------------------
/**
 * brief Park Direct convertion in the form of an inline HLS function
 * tparam T_IO	        Datatype of the input/output ap_fixed. ex. ap_fixed<32,16> is enough for Q16.16
 * tparam T_SINCOS	    Datatype of the sin/cos ap_fixed. ex. ap_fixed<W_sin, 1> is enough for Q16.16
 * param id_out        Id as output of Park Direct.
 * param iq_out        Iq as output of Park Direct.
 * param ialpha_in     Ialpha as input of Park Direct.
 * param ibeta_in      Ibeta as input of Park Direct, orthogonal component of Ialpha.
 * param cos_theta_in  Value of cos(theta) as input of Park Direct.
 * param sin_theta_in  Value of sin(theta) as input of Park Direct.
 */
template <class T_IO, class T_SINCOS>
void Park_Direct_ap_fixed(
    T_IO& id_out, T_IO& iq_out, T_IO ialpha_in, T_IO ibeta_in, T_SINCOS cos_theta_in, T_SINCOS sin_theta_in) {
#pragma HLS INLINE off
    // typedef ap_fixed<32 + 4, 16 + 4>  T_MID;
    typedef T_IO T_MID;
    T_MID Ialpha = ialpha_in;
    T_MID Ibeta = ibeta_in;
    T_MID cos_theta = (T_MID)cos_theta_in;
    T_MID sin_theta = (T_MID)sin_theta_in;
    T_MID Ia_cos, Ib_sin, Ib_cos, Ia_sin, Id, Iq;

    Ia_cos = (T_MID)Ialpha * cos_theta;
    Ib_sin = (T_MID)Ibeta * sin_theta;
    Ib_cos = (T_MID)Ibeta * cos_theta;
    Ia_sin = (T_MID)Ialpha * sin_theta;
    Id = Ia_cos + Ib_sin;
    Iq = Ib_cos - Ia_sin;
    id_out = Id;
    iq_out = Iq;
};

//--------------------------------------------------------------------------
// Park Inverse
// Valpha_out = Vd_in*cos(Theta_in) - Vq_in*sin(Theta_in)
// Vbeta_out = Vq_in*cos(Theta_in) + Vd_in*sin(Theta_in)
//--------------------------------------------------------------------------
template <class T_IO, class T_SINCOS>
void Park_Inverse_ap_fixed(
    T_IO& valpha_out, T_IO& vbeta_out, T_IO vd_in, T_IO vq_in, T_SINCOS cos_theta_in, T_SINCOS sin_theta_in) {
#pragma HLS INLINE off
    // typedef ap_fixed<32 + 4, 16 + 4>  T_MID;
    typedef T_IO T_MID;
    // Decode Input
    T_MID Vd = vd_in;
    T_MID Vq = vq_in;
    T_MID cos_theta = (T_MID)cos_theta_in;
    T_MID sin_theta = (T_MID)sin_theta_in;
    T_MID Vd_cos, Vq_sin, Vq_cos, Vd_sin, va_inv, vb_inv;
    Vd_cos = (T_MID)Vd * cos_theta;
    Vq_sin = (T_MID)Vq * sin_theta;
    Vq_cos = (T_MID)Vq * cos_theta;
    Vd_sin = (T_MID)Vd * sin_theta;
    va_inv = Vd_cos - Vq_sin;
    vb_inv = Vd_sin + Vq_cos;
    valpha_out = va_inv;
    vbeta_out = vb_inv;
}

#endif
