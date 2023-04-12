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
#ifndef _CLARKE_2P_HPP_
#define _CLARKE_2P_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// Clarke Direct 2 phases
// Ialpha = Ia
// Ibeta = (Ia + 2Ib)/sqrt(3)
// Where Ia+Ib+Ic = 0
//--------------------------------------------------------------------------
#ifdef __SYNTHESIS__
template <class T_NUM>
void Clarke_Direct_2p_T_numeral(T_NUM& ialpha_out, T_NUM& ibeta_out, T_NUM ia_in, T_NUM ib_in) {
#pragma HLS INLINE off
    T_NUM Ia = ia_in;
    T_NUM Ib = ib_in;
    T_NUM Ialpha = Ia;
    T_NUM Ibeta = (Ia + 2.0 * Ib) / sqrt(3.0);
    ialpha_out = Ialpha;
    ibeta_out = Ibeta;
}
#endif

/**
 * @brief Clarke Direct 2 phase convertion in the form of an inline HLS function
 * @tparam T_IN	    Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_OUT	Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam I_SQRT3A The number \f$\frac{1}{\sqrt{3}}\f$ for Q16.16
 * @tparam SQRT3A_SCALE  Number significant bits of I_SQRT3A. ex. SQRT3A_SCALE is 16 for Q16.16
 * @param ialpha_out    Ialpha as output of Clarke Direct.
 * @param ibeta_out    Ibeta as output of Clarke Direct, orthogonal component of Ialpha.
 * @param ia_in     Ia as input of Clarke Direct.
 * @param ib_in     Ib as input of Clarke Direct.
 */
template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int I_SQRT3A, int SQRT3A_SCALE>
void Clarke_Direct_2p_T(T_OUT& ialpha_out, T_OUT& ibeta_out, T_IN ia_in, T_IN ib_in) //;
{
#pragma HLS INLINE off
    T_MID Ia = ia_in;
    T_MID Ib = ib_in;
    T_MID Ialpha = Ia;
    T_MID Ibd = Ialpha + (Ib << 1);                 // calculate Ia+2*Ib
    T_MID Ibeta = (Ibd * I_SQRT3A) >> SQRT3A_SCALE; // * 1/SQRT(3)
    Ibeta = Clip_AP<T_MID>(Ibeta, (T_MID)MIN_OUT, (T_MID)MAX_OUT);
    ialpha_out = Ialpha;
    ibeta_out = Ibeta;
};

/**
 * @brief Clarke Direct 2 phase convertion in the form of an inline HLS function
 * @tparam IN_W	    Bit width of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MID_W	Bit width of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for
 * Q16.16
 * @tparam OUT_W	Bit width of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam I_SQRT3A The number \f$\frac{1}{\sqrt{3}}\f$ for Q16.16
 * @tparam SQRT3A_SCALE  Number significant bits of I_SQRT3A. ex. SQRT3A_SCALE is 16 for Q16.16
 * @param ialpha_out    Ialpha as output of Clarke Direct.
 * @param ibeta_out    Ibeta as output of Clarke Direct, orthogonal component of Ialpha.
 * @param ia_in     Ia as input of Clarke Direct.
 * @param ib_in     Ib as input of Clarke Direct.
 */
template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int I_SQRT3A, int SQRT3A_SCALE>
void Clarke_Direct_2p_T_AP(ap_int<OUT_W>& ialpha_out,
                           ap_int<OUT_W>& ibeta_out,
                           ap_int<IN_W> ia_in,
                           ap_int<IN_W> ib_in) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + SQRT3A_SCALE); // +1 ?
    Clarke_Direct_2p_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, I_SQRT3A, SQRT3A_SCALE>(
        ialpha_out, ibeta_out, ia_in, ib_in);
}

//--------------------------------------------------------------------------
// Clarke Inverse for 2 phases
// Va = Valpha
// Vb = [-Valpha + sqrt(3)*Vbeta]/2
// Vc = [-Valpha - sqrt(3)*Vbeta]/2
// Va, Vb, Vc;							// Clarke Inverse -> SVPWM
//--------------------------------------------------------------------------
#ifdef __SYNTHESIS__
template <class T_NUM>
void Clarke_Inverse_2p_T_numeral(T_NUM& va_out, T_NUM& vb_out, T_NUM& vc_out, T_NUM valpha_in, T_NUM vbeta_in) {
#pragma HLS INLINE off
    T_NUM Valpha = valpha_in;
    T_NUM Vbeta = vbeta_in;
    va_out = Valpha;
    vb_out = (-Valpha + Vbeta * sqrt(3.0)) / 2.0;
    vc_out = (-Valpha - Vbeta * sqrt(3.0)) / 2.0;
}
#endif

/**
 * @brief Clarke Inverse 2 phase convertion in the form of an inline HLS function
 * @tparam T_IN	    Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for Q16.16
 * @tparam T_OUT	Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam V_SQRT3C The number \f$\sqrt{3}\f$ for Q16.16
 * @tparam V_SQRT3C_SCALE  Number significant bits of V_SQRT3C. ex. SQRT3C_SCALE is 16 for Q16.16
 * @param va_out    Va as output of Clarke Inverse.
 * @param vb_out    Vb as output of Clarke Inverse.
 * @param vc_out    Vc as output of Clarke Inverse.
 * @param valpha_in Valpha as input of Clarke Inverse.
 * @param vbeta_in  Vbeta as input of Clarke Inverse.
 */
template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, int V_SQRT3C, int V_SQRT3C_SCALE>
void Clarke_Inverse_2p_T(T_OUT& va_out, T_OUT& vb_out, T_OUT& vc_out, T_IN valpha_in, T_IN vbeta_in) //;
{
#pragma HLS INLINE off
    T_MID Valpha = valpha_in;
    T_MID Vbeta = vbeta_in;
    T_MID s3vb = Vbeta * V_SQRT3C; // (sqrt(3)*(2^15))*Vbeta
    va_out = Valpha;
    vb_out = Clip_AP<T_MID>(((s3vb >> V_SQRT3C_SCALE) - Valpha) >> 1, (T_MID)MIN_OUT,
                            (T_MID)MAX_OUT); // (-Valpha + sqrt(3)*Vbeta)/2
    vc_out = Clip_AP<T_MID>((0 - Valpha - (s3vb >> V_SQRT3C_SCALE)) >> 1, (T_MID)MIN_OUT,
                            (T_MID)MAX_OUT); // (-Valpha - sqrt(3)*Vbeta)/2
};

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
#pragma HLS BIND_OP variable = vb_out op = mul impl = dsp
#pragma HLS BIND_OP variable = vc_out op = mul impl = dsp
    vb_out = (0 - Valpha + Vbeta * sqrt3) / 2;
    vc_out = (0 - Valpha - Vbeta * sqrt3) / 2;
};

/**
 * @brief Clarke Inverse 2 phase convertion in the form of an inline HLS function
 * @tparam IN_W	    Bit width of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MID_W	Bit width of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for
 * Q16.16
 * @tparam OUT_W	Bit width of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam V_SQRT3C The number \f$\sqrt{3}\f$ for Q16.16
 * @tparam V_SQRT3C_SCALE  Number significant bits of V_SQRT3C. ex. SQRT3C_SCALE is 16 for Q16.16
 * @param va_out    Va as output of Clarke Inverse.
 * @param vb_out    Vb as output of Clarke Inverse.
 * @param vc_out    Vc as output of Clarke Inverse.
 * @param valpha_in Valpha as input of Clarke Inverse.
 * @param vbeta_in  Vbeta as input of Clarke Inverse.
 */
template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int V_SQRT3C, int V_SQRT3C_SCALE>
void Clarke_Inverse_2p_T_AP(ap_int<OUT_W>& va_out,
                            ap_int<OUT_W>& vb_out,
                            ap_int<OUT_W>& vc_out,
                            ap_int<IN_W> valpha_in,
                            ap_int<IN_W> vbeta_in) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + V_SQRT3C_SCALE + 1); // +1 ?
    Clarke_Inverse_2p_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, V_SQRT3C, V_SQRT3C_SCALE>(
        va_out, vb_out, vc_out, valpha_in, vbeta_in);
};
#endif