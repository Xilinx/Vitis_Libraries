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
#ifndef _PARK_HPP_
#define _PARK_HPP_

#include "common.hpp"
#include "utils.hpp"

//--------------------------------------------------------------------------
// Park Direct
// Id = Ialpha*cos(Theta) + Ibeta*sin(Theta)
// Iq = Ibeta*cos(Theta) - Ialpha*sin(Theta)
//--------------------------------------------------------------------------
#ifdef __SYNTHESIS__
template <class T_NUM>
void Park_Direct_T_numeral(T_NUM& ia_out, T_NUM& ib_out, T_NUM ialpha_in, T_NUM ibeta_in, T_NUM Theta_in) {
#pragma HLS INLINE off
    T_NUM Theta = ((2 * M_PI * 2) / 1000.0) * Theta_in;
    ia_out = T_NUM(ialpha_in) * cos(Theta) + T_NUM(ibeta_in) * sin(Theta);
    ib_out = T_NUM(ibeta_in) * cos(Theta) - T_NUM(ialpha_in) * sin(Theta);
}
#endif

/**
 * @brief Park Direct convertion in the form of an inline HLS function
 * @tparam T_IN	        Type of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam T_MID	    Type of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for
 * Q16.16
 * @tparam T_OUT	    Type of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	    Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	    Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam T_SINCOS     Type of the input Trigonometric function tables. ex. short or ap_int<16> is enough for Q16.16
 * @tparam SINCOS_SCALE Number significant bits of Trigonometric function table. ex. SINCOS_SCALE is 16 for Q16.16
 * @param id_out        Id as output of Park Direct.
 * @param iq_out        Iq as output of Park Direct.
 * @param ialpha_in     Ialpha as input of Park Direct.
 * @param ibeta_in      Ibeta as input of Park Direct, orthogonal component of Ialpha.
 * @param cos_theta_in  Value of cos(theta) as input of Park Direct.
 * @param sin_theta_in  Value of sin(theta) as input of Park Direct.
 */
template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, class T_SINCOS, int SINCOS_SCALE>
void Park_Direct_T(
    T_OUT& id_out, T_OUT& iq_out, T_IN ialpha_in, T_IN ibeta_in, T_SINCOS cos_theta_in, T_SINCOS sin_theta_in) {
#pragma HLS INLINE off
    // Decode Input
    T_MID Ialpha = ialpha_in;
    T_MID Ibeta = ibeta_in;

    // Process data
    T_MID cos_theta = (T_MID)cos_theta_in;
    T_MID sin_theta = (T_MID)sin_theta_in;
    T_MID Ia_cos = (T_MID)Ialpha * cos_theta;
    T_MID Ib_sin = (T_MID)Ibeta * sin_theta;
    T_MID Ib_cos = (T_MID)Ibeta * cos_theta;
    T_MID Ia_sin = (T_MID)Ialpha * sin_theta;
    T_MID Id_dir = Ia_cos + Ib_sin;
    T_MID Iq_dir = Ib_cos - Ia_sin;
    T_MID Id = Clip_AP<T_MID>(Id_dir >> SINCOS_SCALE, (T_MID)MIN_OUT, (T_MID)MAX_OUT); // Remove fractional part
    T_MID Iq = Clip_AP<T_MID>(Iq_dir >> SINCOS_SCALE, (T_MID)MIN_OUT, (T_MID)MAX_OUT); // Remove fractional part
    id_out = Id;
    iq_out = Iq;
};

/**
 * @brief Park Direct convertion in the form of an inline HLS function
 * @tparam Qm_W	    Integer part width of the Qm.Qn
 * @tparam Qn_W	    Factional part width of the Qm.Qn
 * @tparam T_SINCOS     Type of the input Trigonometric function tables. ex. short or ap_int<16> is enough for Q15.16
 * @tparam SINCOS_SCALE Number significant bits of Trigonometric function table. ex. SINCOS_SCALE is 16 for Q15.16
 * @param id_out        Id as output of Park Direct.
 * @param iq_out        Iq as output of Park Direct.
 * @param ialpha_in     Ialpha as input of Park Direct.
 * @param ibeta_in      Ibeta as input of Park Direct, orthogonal component of Ialpha.
 * @param cos_theta_in  Value of cos(theta) as input of Park Direct.
 * @param sin_theta_in  Value of sin(theta) as input of Park Direct.
 */
template <int Qm_W, int Qn_W, int MAX_OUT, int MIN_OUT, class T_SINCOS, int SINCOS_SCALE>
void Park_Direct_Qmn(ap_fixed<(Qm_W + Qn_W + 1), (Qm_W + 1)>& id_out,
                     ap_fixed<(Qm_W + Qn_W + 1), (Qm_W + 1)>& iq_out,
                     ap_fixed<(Qm_W + Qn_W + 1), (Qm_W + 1)> ialpha_in,
                     ap_fixed<(Qm_W + Qn_W + 1), (Qm_W + 1)> ibeta_in,
                     T_SINCOS cos_theta_in,
                     T_SINCOS sin_theta_in) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MAX_OUT <= (1 << (Qm_W + 1)));
    XF_MOTORCONTROL_HW_ASSERT(MIN_OUT >= 0 - (1 << (Qm_W + 1)));
    typedef ap_fixed<(Qm_W + Qn_W + 1 + SINCOS_SCALE), (Qm_W + SINCOS_SCALE + 1)> T_MID;
    typedef ap_fixed<(Qm_W + Qn_W + 1), (Qm_W + 1)> T_IO;

    // Decode Input
    T_MID Ialpha = ialpha_in;
    T_MID Ibeta = ibeta_in;

    // Process data
    T_MID cos_theta = (T_MID)cos_theta_in;
    T_MID sin_theta = (T_MID)sin_theta_in;
    T_MID Ia_cos = (T_MID)Ialpha * cos_theta;
    T_MID Ib_sin = (T_MID)Ibeta * sin_theta;
    T_MID Ib_cos = (T_MID)Ibeta * cos_theta;
    T_MID Ia_sin = (T_MID)Ialpha * sin_theta;
    T_MID Id_dir = Ia_cos + Ib_sin;
    T_MID Iq_dir = Ib_cos - Ia_sin;
    T_IO Id = (T_IO)(Id_dir >> SINCOS_SCALE);
    T_IO Iq = (T_IO)(Iq_dir >> SINCOS_SCALE);
    Id = Clip_AP<T_IO>(Id, MIN_OUT, MAX_OUT); // Clip
    Iq = Clip_AP<T_IO>(Iq, MIN_OUT, MAX_OUT); // Clip
    id_out = Id;
    iq_out = Iq;

#ifndef __SYNTHESIS__
#ifdef DEBUG_PARK
    printf("after format to ap_fixed, Ialpha, Ibeta, Ia_cos, Ib_sin, Ib_cos, Ia_sin :\n %f\t %f\t %f\t %f\t %f\t %f\n ",
           Ialpha.to_float(), Ibeta.to_float(), Ia_cos.to_float(), Ib_sin.to_float(), Ib_cos.to_float(),
           Ia_sin.to_float());
#endif
#endif
};

/**
 * @brief Park Direct convertion in the form of an inline HLS function
 * @tparam IN_W	        bit width of the input data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MID_W	    bit width of the data in the middle of calculation. ex. int_32t or ap_int<32> is enough for
 * Q16.16
 * @tparam OUT_W	    bit width of the output data. ex. short or ap_int<16> is enough for Q16.16
 * @tparam MAX_OUT	    Limit value for the maximum output. ex. MAX_OUT is 32767 for Q16.16
 * @tparam MIN_OUT	    Limit value for the minimum output. ex. MIN_OUT is -32767 for Q16.16
 * @tparam SINCOS_W     bit width of the input Trigonometric function tables. ex. short or ap_int<16> is enough for
 * Q16.16
 * @tparam SINCOS_SCALE Number significant bits of Trigonometric function table. ex. SINCOS_SCALE is 16 for Q16.16
 * @param id_out        Ialpha as output of Park Direct.
 * @param iq_out        Ibeta as output of Park Direct, orthogonal component of Ialpha.
 * @param ialpha_in     Ialpha as input of Park Direct.
 * @param ibeta_in      Ibeta as input of Park Direct, orthogonal component of Ialpha.
 * @param cos_theta_in  Value of cos(theta) as input of Park Direct.
 * @param sin_theta_in  Value of sin(theta) as input of Park Direct.
 */
template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int SINCOS_W, int SINCOS_SCALE>
void Park_Direct_T_AP(ap_int<OUT_W>& ia_out,
                      ap_int<OUT_W>& ib_out,
                      ap_int<IN_W> ialpha_in,
                      ap_int<IN_W> ibeta_in,
                      ap_int<IN_W> Theta_in,
                      ap_int<SINCOS_W>* cos_table,
                      ap_int<SINCOS_W>* sin_table) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + SINCOS_SCALE + 1);
    ap_int<SINCOS_W> cos_theta_in = cos_table[Theta_in];
    ap_int<SINCOS_W> sin_theta_in = sin_table[Theta_in];
    Park_Direct_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, ap_int<SINCOS_W>, SINCOS_SCALE>(
        ia_out, ib_out, ialpha_in, ibeta_in, cos_theta_in, sin_theta_in);
}

//--------------------------------------------------------------------------
// Park Inverse
// Valpha = Vd*cos(Theta) - Vq*sin(Theta)
// Vbeta = Vq*cos(Theta) + Vd*sin(Theta)
//--------------------------------------------------------------------------
//#ifdef __SYNTHESIS__
template <class T_NUM>
void Park_Inverse_T_numeral(T_NUM& va_out, T_NUM& vb_out, T_NUM vd_in, T_NUM vq_in, T_NUM Theta_in) {
#pragma HLS INLINE off
    va_out = T_NUM(vd_in) * cos(Theta_in) - T_NUM(vq_in) * sin(Theta_in);
    vb_out = T_NUM(vq_in) * cos(Theta_in) + T_NUM(vd_in) * sin(Theta_in);
}
//#endif

template <class T_IN, class T_MID, class T_OUT, int MAX_OUT, int MIN_OUT, class T_SINCOS, int SINCOS_SCALE>
void Park_Inverse_T(
    T_OUT& valpha_out, T_OUT& vb_out, T_IN vd_in, T_IN vq_in, T_SINCOS cos_theta_in, T_SINCOS sin_theta_in) {
#pragma HLS INLINE off
    // Decode Input
    T_MID Vd = vd_in;
    T_MID Vq = vq_in;

    // Process data
    T_MID cos_theta = (T_MID)cos_theta_in;
    T_MID sin_theta = (T_MID)sin_theta_in;
    T_MID Vd_cos = (T_MID)Vd * cos_theta;
    T_MID Vq_sin = (T_MID)Vq * sin_theta;
    T_MID Vq_cos = (T_MID)Vq * cos_theta;
    T_MID Vd_sin = (T_MID)Vd * sin_theta;
    T_MID va_inv = Vd_cos - Vq_sin;
    T_MID vb_inv = Vd_sin + Vq_cos;
    T_MID Va = Clip_AP<T_MID>(va_inv >> SINCOS_SCALE, (T_MID)MIN_OUT, (T_MID)MAX_OUT); // Remove fractional part
    T_MID Vb = Clip_AP<T_MID>(vb_inv >> SINCOS_SCALE, (T_MID)MIN_OUT, (T_MID)MAX_OUT); // Remove fractional part
    valpha_out = Va;
    vb_out = Vb;
}

template <int IN_W, int MID_W, int OUT_W, int MAX_OUT, int MIN_OUT, int SINCOS_W, int SINCOS_SCALE>
void Park_Inverse_T_AP(ap_int<OUT_W>& valpha_out,
                       ap_int<OUT_W>& vb_out,
                       ap_int<IN_W> vd_in,
                       ap_int<IN_W> vq_in,
                       ap_int<SINCOS_W> Theta_in,
                       ap_int<SINCOS_W>* cos_table,
                       ap_int<SINCOS_W>* sin_table) {
#pragma HLS INLINE off
    XF_MOTORCONTROL_HW_ASSERT(MID_W >= IN_W + SINCOS_SCALE + 1);
    ap_int<SINCOS_W> cos_theta_in = cos_table[Theta_in];
    ap_int<SINCOS_W> sin_theta_in = sin_table[Theta_in];
    Park_Inverse_T<ap_int<IN_W>, ap_int<MID_W>, ap_int<OUT_W>, MAX_OUT, MIN_OUT, ap_int<SINCOS_W>, SINCOS_SCALE>(
        valpha_out, vb_out, vd_in, vq_in, cos_theta_in, sin_theta_in);
};

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
#pragma HLS BIND_OP variable = Ia_cos op = mul impl = dsp
#pragma HLS BIND_OP variable = Ib_sin op = mul impl = dsp
#pragma HLS BIND_OP variable = Ib_cos op = mul impl = dsp
#pragma HLS BIND_OP variable = Ia_sin op = mul impl = dsp
#pragma HLS BIND_OP variable = Id op = add impl = dsp
#pragma HLS BIND_OP variable = Iq op = sub impl = dsp

    Ia_cos = (T_MID)Ialpha * cos_theta;
    Ib_sin = (T_MID)Ibeta * sin_theta;
    Ib_cos = (T_MID)Ibeta * cos_theta;
    Ia_sin = (T_MID)Ialpha * sin_theta;
    Id = Ia_cos + Ib_sin;
    Iq = Ib_cos - Ia_sin;
    id_out = Id;
    iq_out = Iq;
};

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
#pragma HLS BIND_OP variable = Vd_cos op = mul impl = dsp
#pragma HLS BIND_OP variable = Vq_sin op = mul impl = dsp
#pragma HLS BIND_OP variable = Vq_cos op = mul impl = dsp
#pragma HLS BIND_OP variable = Vd_sin op = mul impl = dsp
#pragma HLS BIND_OP variable = va_inv op = sub impl = dsp
#pragma HLS BIND_OP variable = vb_inv op = add impl = dsp
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
