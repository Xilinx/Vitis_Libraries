/*
 * Copyright 2022 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _CLARKE_PARK_3P_HPP_
#define _CLARKE_PARK_3P_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// Clarke Direct 3p
// Ialpha = 2/3*Ia- 1/3*(Ib -Ic)
// Ibeta = 2*(Ib-Ic)/sqrt(3)
// Ihomop = 2/3*(Ia+Ib+Ic)
// Where Ia+Ib+Ic != 0 ?
// iα and iβ components in an orthogonal reference frame and
// io the homopolar component of the system
//--------------------------------------------------------------------------
#ifdef __SYNTHESIS__
template <class T_NUM>
void Clarke_Direct_3p_T_numeral(
    T_NUM& ialpha_out, T_NUM& ibeta_out, T_NUM& ihomop_out, T_NUM ia_in, T_NUM ib_in, T_NUM ic_in) {
#pragma HLS INLINE off
    T_NUM Ialpha = (2.0 * ia_in - 1.0 * (ib_in - ic_in)) / 3.0;
    T_NUM Ibeta = 2.0 * (ib_in - ic_in) / sqrt(3.0);
    T_NUM Ihomop = 2.0 / 3.0 * (ia_in + ib_in + ic_in);
    ialpha_out = Ialpha;
    ibeta_out = Ibeta;
    ihomop_out = Ihomop;
}
#endif

/**
 * @brief Clarke Direct 3 phase convertion in the form of an inline HLS function
 * @tparam T_IN	    Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_OUT	Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam I_SQRT3A The number \f$\frac{1}{\sqrt{3}}\f$ for Q16.16
 * @tparam SQRT3A_SCALE  Number significant bits of I_SQRT3A. ex. SQRT3A_SCALE is 16 for Q16.16
 * @param ialpha_out    Ialpha as output of Clarke Direct.
 * @param ibeta_out     Ibeta as output of Clarke Direct, orthogonal component of Ialpha.
 * @param ihomop_out    Ihomopolar as output of Clarke Direct, homopolar component of the system.
 * @param ia_in     Ia as input of Clarke Direct.
 * @param ib_in     Ib as input of Clarke Direct.
 * @param ib_in     Ib as input of Clarke Direct.
 */
template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int I_SQRT3A, int SQRT3A_SCALE>
void Clarke_Direct_3p_T(T_OUT& ialpha_out, T_OUT& ibeta_out, T_OUT& ihomop_out, T_IN ia_in, T_IN ib_in, T_IN ic_in) {
#pragma HLS INLINE off
    T_MID Ia = ia_in;
    T_MID Ib = ib_in;
    T_MID Ic = ic_in;
    T_MID Ialpha = (2 * Ia - (Ib + Ic)) / 3;
    T_MID Ibd = (Ib - Ic);                                // calculate Ia+2*Ib
    T_MID Ibeta = ((Ib - Ic) * I_SQRT3A) >> SQRT3A_SCALE; // * 1/SQRT(3)
    T_MID Ihomop = 2 * (Ia + Ib + Ic) / 3;
    Ibeta = Clip_AP<T_MID>(Ibeta, (T_MID)MIN_OUT, (T_MID)MAX_OUT);
    ialpha_out = Ialpha;
    ibeta_out = Ibeta;
    ihomop_out = Ihomop;
};

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

/**
 * @brief Clarke Direct 3 phase convertion in the form of an inline HLS function
 * @tparam IN_W	    bit width of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MID_W	bit width of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for
 * Q16.16
 * @tparam OUT_W	bit width of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam I_SQRT3A The number \f$\frac{1}{\sqrt{3}}\f$ for Q16.16
 * @tparam SQRT3A_SCALE  Number significant bits of I_SQRT3A. ex. SQRT3A_SCALE is 16 for Q16.16
 * @param ialpha_out    Ialpha as output of Clarke Direct.
 * @param ibeta_out     Ibeta as output of Clarke Direct, orthogonal component of Ialpha.
 * @param ihomop_out    Ihomopolar as output of Clarke Direct, homopolar component of the system.
 * @param ia_in     Ia as input of Clarke Direct.
 * @param ib_in     Ib as input of Clarke Direct.
 * @param ib_in     Ib as input of Clarke Direct.
 */
template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int I_SQRT3A, int SQRT3A_SCALE>
void Clarke_Direct_3p_T_AP(ap_int<OUT_W>& ialpha_out,
                           ap_int<OUT_W>& ibeta_out,
                           ap_int<OUT_W>& ihomop_out,
                           ap_int<IN_W> ia_in,
                           ap_int<IN_W> ib_in,
                           ap_int<IN_W> ic_in) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + SQRT3A_SCALE); // +1 ?
    Clarke_Direct_3p_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, I_SQRT3A, SQRT3A_SCALE>(
        ialpha_out, ibeta_out, ihomop_out, ia_in, ib_in, ic_in);
};
#endif
