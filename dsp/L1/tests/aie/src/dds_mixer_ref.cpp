/*
 * Copyright 2022 Xilinx, Inc.
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
/*
DDS reference model
*/

#include "dds_mixer_ref.hpp"
#include "fir_ref_utils.hpp"

// for base sin/cos lookup
#include "aie_api/aie_adf.hpp"

//#include "debug_utils.h"
#include <math.h>
#include <string>

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

// aie_api is external to xf::dsp::aie namespace
namespace aie = ::aie;

//-------------------------------------------------------------------
// Utility functions

// Templatised function for DDS function phase to cartesian function.
template <typename T_RET_TYPE, typename TT_DDS_DATA>
T_RET_TYPE phaseToCartesian(uint32 phaseAcc) {
    cint16 retValraw;
    cint64 retVal;
    TT_DDS_DATA validationVal;
    constexpr float rndConst = 0.5;
    // constexpr double pi = 3.14159265;
    double cos_out;
    double sin_out;
    double angle_rads;
    cint32 ddsint32;
    constexpr int scaleDds = 32768;              // multiplier to scale dds output
    uint32 phaseAccUsed = phaseAcc & 0xFFFFF000; // only the top 20 bits are used for sincos lookup.

    angle_rads = ((phaseAccUsed * M_PI) / pow(2, 31));
    cos_out = cos(angle_rads); // angle in radians
    sin_out = sin(angle_rads); // angle in radians

    ddsint32.real = floor(scaleDds * cos_out + rndConst); // TODO- floor? not rnd?
    ddsint32.imag = floor(scaleDds * sin_out + rndConst); // TODO- floor? not rnd?

    validationVal.real = ddsint32.real == scaleDds ? 32767 : ddsint32.real;
    validationVal.imag = ddsint32.imag == scaleDds ? 32767 : ddsint32.imag;
    retValraw = aie::sincos_complex(phaseAcc);
    retVal.real = retValraw.real;
    retVal.imag = retValraw.imag;

    try {
        if ((validationVal.real - retVal.real > 2) || (validationVal.imag - retVal.imag > 2) ||
            (validationVal.real - retVal.real < -2) || (validationVal.imag - retVal.imag < -2)) {
            throw 1;
        }
    } catch (int& i) {
        printf("Error: mismatch in DDS output versus validation model in phaseToCartesian\n");
        abort();
    }
    return retVal;
};

template <>
cfloat phaseToCartesian<cfloat, cfloat>(uint32 phaseAcc) {
    cint16 intrinsicVal;
    cfloat validationVal;
    cfloat retVal;
    constexpr float rndConst = 0.5;
    // constexpr double pi = 3.14159265;
    double cos_out;
    double sin_out;
    double angle_rads;
    double dds_real, dds_imag;
    cint32 ddsint32;
    constexpr int scaleDds = 32768;              // multiplier to scale dds output
    uint32 phaseAccUsed = phaseAcc & 0xFFFFF000; // only the top 20 bits are used for sincos lookup.

    angle_rads = ((phaseAccUsed * M_PI) / pow(2, 31));
    cos_out = cos(angle_rads); // angle in radians
    sin_out = sin(angle_rads); // angle in radians

    ddsint32.real = floor(scaleDds * cos_out + rndConst);                // TODO- floor? not rnd?
    ddsint32.imag = floor(scaleDds * sin_out + rndConst);                // TODO- floor? not rnd?
    ddsint32.real = (ddsint32.real == scaleDds) ? 32767 : ddsint32.real; // apply saturation.
    ddsint32.imag = (ddsint32.imag == scaleDds) ? 32767 : ddsint32.imag;

    intrinsicVal = aie::sincos_complex(phaseAcc);
    retVal.real = (float)intrinsicVal.real / (float)(1 << 15);
    retVal.imag = (float)intrinsicVal.imag / (float)(1 << 15);

    validationVal.real = (float)ddsint32.real / (float)(1 << 15);
    validationVal.imag = (float)ddsint32.imag / (float)(1 << 15);

    try {
        if ((ddsint32.real - intrinsicVal.real > 2) || (ddsint32.imag - intrinsicVal.imag > 2) ||
            (ddsint32.real - intrinsicVal.real < -2) || (ddsint32.imag - intrinsicVal.imag < -2)) {
            throw 1;
        }
    } catch (int& i) {
        printf("Error: mismatch in DDS output versus validation model in phaseToCartesian\n");
        abort();
    }
    return retVal;
};

template <typename T_D, typename T_IN>
inline T_D saturate(T_IN d_in){};
template <>
inline cint16 saturate<cint16, cint64>(cint64 d_in) {
    cint16 retVal;
    int64 temp;
    constexpr int64 maxpos = ((int64)1 << 15) - 1;
    constexpr int64 maxneg = -((int64)1 << 15);
    temp = d_in.real > maxpos ? maxpos : d_in.real;
    retVal.real = temp < maxneg ? maxneg : temp;
    temp = d_in.imag > maxpos ? maxpos : d_in.imag;
    retVal.imag = temp < maxneg ? maxneg : temp;
    return retVal;
};
template <>
inline cint32 saturate<cint32, cint64>(cint64 d_in) {
    cint32 retVal;
    int64 temp;
    constexpr int64 maxpos = ((int64)1 << 31) - 1;
    constexpr int64 maxneg = -((int64)1 << 31);
    temp = d_in.real > maxpos ? maxpos : d_in.real;
    retVal.real = temp < maxneg ? maxneg : temp;
    temp = d_in.imag > maxpos ? maxpos : d_in.imag;
    retVal.imag = temp < maxneg ? maxneg : temp;
    return retVal;
};
template <>
inline cfloat saturate<cfloat, cfloat>(cfloat d_in) {
    return d_in;
};

template <typename T_D>
inline T_D downshift(T_D d_in, int shift){};
template <>
inline cint64 downshift<cint64>(cint64 d_in, int shift) {
    cint64 retVal;
    retVal.real = d_in.real >> shift;
    retVal.imag = d_in.imag >> shift;
    return retVal;
}
template <>
inline cfloat downshift<cfloat>(cfloat d_in, int shift) {
    cfloat retVal;
    retVal.real = d_in.real / (float)(1 << shift);
    retVal.imag = d_in.imag / (float)(1 << shift);
    return retVal;
}

template <typename TT_RET_DATA, typename TT_DATA, int conj>
inline TT_RET_DATA cmplxMult(TT_RET_DATA d_in, TT_RET_DATA ddsOut, int ddsShift){};

template <>
inline cint64 cmplxMult<cint64, cint16, 0>(cint64 d_in, cint64 ddsOut, int ddsShift) {
    cint64 retVal;
    int64 temp;
    // constexpr int64 maxpos = ((int64)1<<15) -1;
    // constexpr int64 maxneg = -((int64)1<<15) ;
    constexpr int64 rndConst = 0; // rnd_floor
    // temp = (((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.real = temp < maxneg? maxneg : temp;
    retVal.real =
        (((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst) >> ddsShift;

    // temp = (((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.imag = temp < maxneg? maxneg : temp;
    retVal.imag =
        (((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real) + rndConst) >> ddsShift;

    return retVal;
};

template <>
inline cint64 cmplxMult<cint64, cint16, 1>(cint64 d_in, cint64 ddsOut, int ddsShift) {
    cint64 retVal;
    int64 temp;
    // constexpr int64 maxpos = ((int64)1<<15) -1;
    // constexpr int64 maxneg = -((int64)1<<15) ;
    constexpr int64 rndConst = 0; // rnd_floor
    // temp = (((int64)d_in.real * (int64)ddsOut.real) + ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.real = temp < maxneg? maxneg : temp;
    retVal.real =
        (((int64)d_in.real * (int64)ddsOut.real) + ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst) >> ddsShift;

    // temp = (((int64)d_in.imag * (int64)ddsOut.real) - ((int64)d_in.real * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.imag = temp < maxneg? maxneg : temp;
    retVal.imag =
        (((int64)d_in.imag * (int64)ddsOut.real) - ((int64)d_in.real * (int64)ddsOut.imag) + rndConst) >> ddsShift;

    return retVal;
};

template <>
inline cint64 cmplxMult<cint64, cint32, 0>(cint64 d_in, cint64 ddsOut, int ddsShift) {
    cint64 retVal;
    int64 temp;
    // constexpr int64 maxpos = ((int64)1<<31) -1;
    // constexpr int64 maxneg = -((int64)1<<31) ;
    constexpr int64 rndConst = 0; // rnd_floor
    // temp = (((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.real = temp < maxneg? maxneg : temp;
    retVal.real =
        (((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst) >> ddsShift;

    // temp = (((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.imag = temp < maxneg? maxneg : temp;
    retVal.imag =
        (((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real) + rndConst) >> ddsShift;

    return retVal;
};

template <>
inline cint64 cmplxMult<cint64, cint32, 1>(cint64 d_in, cint64 ddsOut, int ddsShift) {
    cint64 retVal;
    int64 temp;
    // constexpr int64 maxpos = ((int64)1<<31) -1;
    // constexpr int64 maxneg = -((int64)1<<31) ;
    constexpr int64 rndConst = 0; // rnd_floor
    // temp = (((int64)d_in.real * (int64)ddsOut.real) + ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.real = temp < maxneg? maxneg : temp;
    retVal.real =
        (((int64)d_in.real * (int64)ddsOut.real) + ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst) >> ddsShift;

    // temp = (((int64)d_in.imag * (int64)ddsOut.real) - ((int64)d_in.real * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // temp = temp > maxpos ? maxpos : temp;
    // retVal.imag = temp < maxneg? maxneg : temp;
    retVal.imag =
        (((int64)d_in.imag * (int64)ddsOut.real) - ((int64)d_in.real * (int64)ddsOut.imag) + rndConst) >> ddsShift;

    return retVal;
};

template <>
inline cfloat cmplxMult<cfloat, cfloat, 0>(cfloat d_in, cfloat ddsOut, int ddsShift) {
    cfloat retVal;
    retVal.real = ((d_in.real * ddsOut.real) - (d_in.imag * ddsOut.imag));
    retVal.imag = ((d_in.real * ddsOut.imag) + (d_in.imag * ddsOut.real));
    return retVal;
};

template <>
inline cfloat cmplxMult<cfloat, cfloat, 1>(cfloat d_in, cfloat ddsOut, int ddsShift) {
    cfloat retVal;
    retVal.real += ((d_in.real * ddsOut.real) + (d_in.imag * ddsOut.imag));
    retVal.imag += ((d_in.imag * ddsOut.real) - (d_in.real * ddsOut.imag));
    return retVal;
}

//-------------------------------------------------------------------
// End of Utility functions

//-------------------------------------------------------------------
// Class member functions
// Constructors
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE>
dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE>::dds_mixer_ref(uint32_t phaseInc,
                                                                            uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    // calculate phRotref values
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16_t, TT_DATA>::type T_DDS_TYPE;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    T_ACC_TYPE phRotRaw;
    for (int i = 0; i < kNumLanes; i++) {
        phRotRaw = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(phaseInc * i);
        this->phRotref[i].real = phRotRaw.real;
        this->phRotref[i].imag = phRotRaw.imag;
    }
}
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, 1>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    // calculate phRotref values
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16_t, TT_DATA>::type T_DDS_TYPE;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    T_ACC_TYPE phRotRaw;
    for (int i = 0; i < kNumLanes; i++) {
        phRotRaw = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(phaseInc * i);
        this->phRotref[i].real = phRotRaw.real;
        this->phRotref[i].imag = phRotRaw.imag;
    }
}
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, 0>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    // calculate phRotref values
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16_t, TT_DATA>::type T_DDS_TYPE;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    T_ACC_TYPE phRotRaw;
    for (int i = 0; i < kNumLanes; i++) {
        phRotRaw = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(phaseInc * i);
        this->phRotref[i].real = phRotRaw.real;
        this->phRotref[i].imag = phRotRaw.imag;
    }
}

// Non-constructors
// REF DDS function (default specialization for mixer mode 2)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE>
void dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE>::ddsMix(input_window<TT_DATA>* inWindowA,
                                                                          input_window<TT_DATA>* inWindowB,
                                                                          output_window<TT_DATA>* outWindow) {
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_DDS_TYPE;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    using T_INT_BASE = typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16, int32>::type;
    using T_BASE_DATA = typename std::conditional<std::is_same<TT_DATA, cfloat>::value, float, T_INT_BASE>::type;
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE ddsOutRaw;
    T_ACC_TYPE ddsOutRawConj;
    T_ACC_TYPE ddsOut;
    T_ACC_TYPE ddsOutConj;
    T_ACC_TYPE phRot;
    TT_DATA d_in;
    T_ACC_TYPE d_in64;
    TT_DATA d_in2;
    T_ACC_TYPE ddsMixerOut;
    T_ACC_TYPE ddsMixerOut2;
    T_ACC_TYPE ddsMixerOutAcc;
    T_ACC_TYPE mixerOutRaw;
    TT_DATA mixerOut;
    constexpr int ddsShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut +1  for bit growth
    constexpr int mixerShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 16; // similar to above, but with addition bit growth. Doesn't apply
                                                       // to float because that would require additional ops in UUT

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOutRaw = cmplxMult<T_ACC_TYPE, TT_DATA, 0>(ddsOutPrime, phRot, 0);
            ddsOutRawConj.real = ddsOutRaw.real;
            ddsOutRawConj.imag = -ddsOutRaw.imag;

            ddsOut = downshift(ddsOutRaw, ddsShift);
            ddsOutConj = downshift(ddsOutRawConj, ddsShift);
            // perform validation of ddsOut
            try {
                if (std::is_same<TT_DATA, cfloat>::value) {
                    if (((ddsOut.real - ddsOutValidation.real > 2) || (ddsOut.real - ddsOutValidation.real < -2) ||
                         (ddsOut.imag - ddsOutValidation.imag > 2) || (ddsOut.imag - ddsOutValidation.imag < -2))) {
                        throw 1;
                    }
                } else {
                    //          if (((ddsOut.real/(1<<ddsShift) - ddsOutValidation.real > 2) ||
                    //          (ddsOut.real/(1<<ddsShift) - ddsOutValidation.real < -2) ||
                    //               (ddsOut.imag/(1<<ddsShift) - ddsOutValidation.imag > 2) ||
                    //               (ddsOut.imag/(1<<ddsShift) - ddsOutValidation.imag < -2))) {
                    if (((ddsOut.real - ddsOutValidation.real > 2) || (ddsOut.real - ddsOutValidation.real < -2) ||
                         (ddsOut.imag - ddsOutValidation.imag > 2) || (ddsOut.imag - ddsOutValidation.imag < -2))) {
                        throw 1;
                    }
                }
            } catch (int& i) {
                printf("Error: mismatch in DDS output versus validation modelin ddsMix\n");
                abort();
            }

            d_in = window_readincr(inWindowA);
            d_in2 = window_readincr(inWindowB);

            // forward mixer (mix with dds output directly)
            d_in64.real = d_in.real;
            d_in64.imag = d_in.imag;
            ddsMixerOut = cmplxMult<T_ACC_TYPE, TT_DATA, 0>(d_in64, ddsOut, 0);

            // conjugate mixer (mix with conjugate of DDS)
            // ...and add to result of positive mixer.
            d_in64.real = d_in2.real;
            d_in64.imag = d_in2.imag;
            ddsMixerOut2 = cmplxMult<T_ACC_TYPE, TT_DATA, 0>(d_in64, ddsOutConj, 0);
            ddsMixerOutAcc.real = (ddsMixerOut.real + ddsMixerOut2.real);
            ddsMixerOutAcc.imag = (ddsMixerOut.imag + ddsMixerOut2.imag);

            mixerOutRaw = downshift(ddsMixerOutAcc, mixerShift);
            mixerOut = saturate<TT_DATA, T_ACC_TYPE>(mixerOutRaw);

            // write single dds raf sample to output window
            window_writeincr((output_window<TT_DATA>*)outWindow, mixerOut);

            // Accumulation
            // update phase_accum for next sample
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase over multiple input windows of data
        }
    }
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1
//===========================================================

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
void dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1>::ddsMix(input_window<TT_DATA>* inWindowA,
                                                                         output_window<TT_DATA>* outWindow) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE ddsOut;
    T_ACC_TYPE phRot;
    TT_DATA d_in;
    T_ACC_TYPE d_in64;
    T_ACC_TYPE ddsMixerOutraw;
    TT_DATA ddsMixerOut;
    constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOut = cmplxMult<T_ACC_TYPE, TT_DATA, 0>(ddsOutPrime, phRot, ddsShift);

            // validate dds output before continuing using bit-accurate output
            try {
                if (std::is_same<TT_DATA, cfloat>::value) {
                    if (((ddsOut.real - ddsOutValidation.real > 2) || (ddsOut.real - ddsOutValidation.real < -2) ||
                         (ddsOut.imag - ddsOutValidation.imag > 2) || (ddsOut.imag - ddsOutValidation.imag < -2))) {
                        throw 1;
                    }
                } else {
                    if (((ddsOut.real - ddsOutValidation.real > 2) || (ddsOut.real - ddsOutValidation.real < -2) ||
                         (ddsOut.imag - ddsOutValidation.imag > 2) || (ddsOut.imag - ddsOutValidation.imag < -2))) {
                        throw 1;
                    }
                }
            } catch (int& i) {
                printf("Error: mismatch in DDS output versus validation model\n");
                abort();
            }

            // update phase_accum for next sample
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase over multiple input windows of data

            // use input windows as needed for each mixer mode
            d_in = window_readincr(inWindowA);

            //  Mix
            d_in64.real = d_in.real;
            d_in64.imag = d_in.imag;
            ddsMixerOutraw = cmplxMult<T_ACC_TYPE, TT_DATA, 0>(d_in64, ddsOut, ddsShift);

            ddsMixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutraw);
            // write single dds raf sample to output window
            window_writeincr((output_window<TT_DATA>*)outWindow, ddsMixerOut);
        }
    }
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0
//===========================================================

template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE>
void dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0>::ddsMix(output_window<TT_DATA>* outWindow) {
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE ddsOutRaw;
    T_ACC_TYPE phRot;
    TT_DATA ddsOut;
    constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut
    using T_INT_BASE = typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16, int32>::type;
    using T_BASE_DATA = typename std::conditional<std::is_same<TT_DATA, cfloat>::value, float, T_INT_BASE>::type;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = phaseToCartesian<T_ACC_TYPE, T_DDS_TYPE>(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOutRaw = cmplxMult<T_ACC_TYPE, TT_DATA, 0>(ddsOutPrime, phRot, ddsShift);

            // validate dds output before continuing using bit-accurate output
            try {
                if (std::is_same<TT_DATA, cfloat>::value) {
                    if (((ddsOutRaw.real - ddsOutValidation.real > 2) ||
                         (ddsOutRaw.real - ddsOutValidation.real < -2) ||
                         (ddsOutRaw.imag - ddsOutValidation.imag > 2) ||
                         (ddsOutRaw.imag - ddsOutValidation.imag < -2))) {
                        throw 1;
                    }
                } else {
                    if (((ddsOutRaw.real - ddsOutValidation.real > 2) ||
                         (ddsOutRaw.real - ddsOutValidation.real < -2) ||
                         (ddsOutRaw.imag - ddsOutValidation.imag > 2) ||
                         (ddsOutRaw.imag - ddsOutValidation.imag < -2))) {
                        throw 1;
                    }
                }
            } catch (int& i) {
                printf("Error: mismatch in DDS output versus validation model\n");
                abort();
            }

            // update phase_accum for next sample
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase per sample

            ddsOut = saturate<TT_DATA, T_ACC_TYPE>(ddsOutRaw);

            // write single dds raf sample to output window
            window_writeincr((output_window<TT_DATA>*)outWindow, ddsOut);
        }
    }
};
}
}
}
}
}
