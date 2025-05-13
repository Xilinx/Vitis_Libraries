/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
/*
DDS reference model
*/
#include "device_defs.h"
#include "dds_mixer_ref.hpp"
#include "fir_ref_utils.hpp"
// for base sin/cos lookup
#include "aie_api/aie_adf.hpp"

#include <math.h>
#include <string>

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

// aie_api is external to xf::dsp::aie namespace
namespace aie = ::aie;
template <typename T_D, typename T_IN>
inline T_D saturate(T_IN d_in, unsigned int satMode = 1){};
template <>
inline cint16 saturate<cint16, cint64>(cint64 d_in, unsigned int satMode) {
    cint16 retVal;
    int64 temp;
    constexpr int64 maxpos = ((int64)1 << 15) - 1;
    constexpr int64 maxneg = -((int64)1 << 15);
    if (satMode == 0 || satMode == 2) { // Mode: none or invalid
        retVal.real = d_in.real;
        retVal.imag = d_in.imag;
    } else if (satMode == 1) { // Mode: saturate
        temp = d_in.real > maxpos ? maxpos : d_in.real;
        retVal.real = temp < maxneg ? maxneg : temp;
        temp = d_in.imag > maxpos ? maxpos : d_in.imag;
        retVal.imag = temp < maxneg ? maxneg : temp;
    } else if (satMode == 3) { // Mode: symmetric
        temp = d_in.real > maxpos ? maxpos : d_in.real;
        retVal.real = temp < maxneg ? maxneg + 1 : temp; // Cap at int maxneg +1
        temp = d_in.imag > maxpos ? maxpos : d_in.imag;
        retVal.imag = temp < maxneg ? maxneg + 1 : temp; // Cap at int maxneg +1
    }
    return retVal;
};
template <>
inline cint32 saturate<cint32, cint64>(cint64 d_in, unsigned int satMode) {
    cint32 retVal;
    int64 temp;
    constexpr int64 maxpos = ((int64)1 << 31) - 1;
    constexpr int64 maxneg = -((int64)1 << 31);
    if (satMode == 0 || satMode == 2) { // Mode: none or invalid
        retVal.real = d_in.real;
        retVal.imag = d_in.imag;
    } else if (satMode == 1) { // Mode: saturate
        temp = d_in.real > maxpos ? maxpos : d_in.real;
        retVal.real = temp < maxneg ? maxneg : temp;
        temp = d_in.imag > maxpos ? maxpos : d_in.imag;
        retVal.imag = temp < maxneg ? maxneg : temp;
    } else if (satMode == 3) { // Mode: symmetric
        temp = d_in.real > maxpos ? maxpos : d_in.real;
        retVal.real = temp < maxneg ? maxneg + 1 : temp; // Cap at int maxneg +1
        temp = d_in.imag > maxpos ? maxpos : d_in.imag;
        retVal.imag = temp < maxneg ? maxneg + 1 : temp; // Cap at int maxneg +1
    }
    return retVal;
};

template <typename T_A>
inline void roundAccDDSLuts(int shift, T_A& accum) {
    if
        constexpr(std::is_same<T_A, cfloat>::value) {
            // no rounding applied to float types
        }
    else {
        accum.real = rounding(rnd_sym_inf, shift, accum.real);
        accum.imag = rounding(rnd_sym_inf, shift, accum.imag);
    }
};

template <typename T_A>
inline void roundAccDDS(int shift, T_A& accum, unsigned int rndMode) {
    if
        constexpr(std::is_same<T_A, cfloat>::value) {
            // no rounding applied to float types
        }
    else {
        accum.real = rounding(rndMode, shift, accum.real);
        accum.imag = rounding(rndMode, shift, accum.imag);
    }
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
inline cfloat saturate<cfloat, cfloat>(cfloat d_in, unsigned int satMode) {
    return d_in;
};
template <>
inline cfloat downshift<cfloat>(cfloat d_in, int shift) {
    cfloat retVal;
    retVal.real = d_in.real / (float)(1 << shift);
    retVal.imag = d_in.imag / (float)(1 << shift);
    return retVal;
}

template <typename TT_RET_DATA, int conj, typename TT_DATA1, typename TT_DATA2>
inline TT_RET_DATA cmplxMult(TT_DATA1 d_in, TT_DATA2 d_in2, int ddsShift, unsigned int rndMode = 0) {
    TT_RET_DATA retVal;
    // constexpr int64 rndConst = 0; //floor
    // retVal.real = (((int64)d_in.real * (int64)d_in2.real) - ((int64)d_in.imag * (int64)d_in2.imag) + rndConst)  >>
    // ddsShift;
    // retVal.imag = (((int64)d_in.real * (int64)d_in2.imag) + ((int64)d_in.imag * (int64)d_in2.real) + rndConst)  >>
    // ddsShift;
    retVal.real = ((int64)d_in.real * (int64)d_in2.real) - ((int64)d_in.imag * (int64)d_in2.imag);
    retVal.imag = ((int64)d_in.real * (int64)d_in2.imag) + ((int64)d_in.imag * (int64)d_in2.real);
    roundAccDDS(ddsShift, retVal, rndMode);

    return retVal;
};

template <typename TT_RET_DATA, int conj>
inline TT_RET_DATA cmplxMult(TT_RET_DATA d_in, TT_RET_DATA ddsOut, int ddsShift, unsigned int rndMode = 0) {
    TT_RET_DATA retVal;
    // constexpr int64 rndConst = 0; //floor
    // retVal.real = (((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // retVal.imag = (((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real) + rndConst)  >>
    // ddsShift;

    retVal.real = ((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag);
    retVal.imag = ((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real);
    roundAccDDS(ddsShift, retVal, rndMode);

    return retVal;
};
template <>
inline cint64 cmplxMult<cint64, 0>(cint64 d_in, cint64 ddsOut, int ddsShift, unsigned int rndMode) {
    cint64 retVal;
    // constexpr int64 rndConst = 0; //floor
    // retVal.real = (((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // retVal.imag = (((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real) + rndConst)  >>
    // ddsShift;
    retVal.real = ((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag);
    retVal.imag = ((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real);
    roundAccDDS(ddsShift, retVal, rndMode);

    return retVal;
};

template <>
inline cint64 cmplxMult<cint64, 1>(cint64 d_in, cint64 ddsOut, int ddsShift, unsigned int rndMode) {
    cint64 retVal;
    // constexpr int64 rndConst = 0; //rnd_floor
    // retVal.real = (((int64)d_in.real * (int64)ddsOut.real) + ((int64)d_in.imag * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    // retVal.imag = (((int64)d_in.imag * (int64)ddsOut.real) - ((int64)d_in.real * (int64)ddsOut.imag) + rndConst)  >>
    // ddsShift;
    retVal.real = ((int64)d_in.real * (int64)ddsOut.real) - ((int64)d_in.imag * (int64)ddsOut.imag);
    retVal.imag = ((int64)d_in.real * (int64)ddsOut.imag) + ((int64)d_in.imag * (int64)ddsOut.real);
    roundAccDDS(ddsShift, retVal, rndMode);

    return retVal;
};

template <typename TT_VAL_DATA, typename TT_ACT_DATA>
void validate_ref_data(TT_VAL_DATA validationVal, TT_ACT_DATA actVal, float tol, char* funcName) {
    try {
        if ((validationVal.real - actVal.real > tol) || (validationVal.imag - actVal.imag > tol) ||
            (validationVal.real - actVal.real < -tol) || (validationVal.imag - actVal.imag < -tol)) {
            throw 1;
        }
    } catch (int& i) {
        printf(
            "Error: mismatch in DDS output versus validation model in %s - actVal = {%d, %d} validationVal = {%d, %d} "
            "error {%d, %d}\n",
            funcName, actVal.real, actVal.imag, validationVal.real, validationVal.imag,
            (validationVal.real - actVal.real), (validationVal.imag - actVal.imag));
        // abort();
    }
}

template <>
inline cfloat cmplxMult<cfloat, 0, cfloat, cfloat>(cfloat d_in, cfloat ddsOut, int ddsShift, unsigned int rndMode) {
    cfloat retVal;
    retVal.real = ((d_in.real * ddsOut.real) - (d_in.imag * ddsOut.imag));
    retVal.imag = ((d_in.real * ddsOut.imag) + (d_in.imag * ddsOut.real));
    return retVal;
};

template <>
inline cfloat cmplxMult<cfloat, 1, cfloat, cfloat>(cfloat d_in, cfloat ddsOut, int ddsShift, unsigned int rndMode) {
    cfloat retVal;
    retVal.real += ((d_in.real * ddsOut.real) + (d_in.imag * ddsOut.imag));
    retVal.imag += ((d_in.imag * ddsOut.real) - (d_in.real * ddsOut.imag));
    return retVal;
}
//-------------------------------------------------------------------
// Utility functions

// Templatised function for DDS function phase to cartesian function.
template <typename T_ACC_TYPE,
          typename T_DDS_TYPE,
          unsigned int TP_NUM_LANES,
          unsigned int TP_SC_MODE,
          unsigned int TP_NUM_LUTS,
          typename T_LUT_DTYPE,
          unsigned int TP_RND>
T_ACC_TYPE
ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, TP_NUM_LANES, TP_SC_MODE, TP_NUM_LUTS, T_LUT_DTYPE, TP_RND>::phaseToCartesian(
    uint32 phaseAcc) {
    cint16 retValraw;
    T_ACC_TYPE retVal;
    T_DDS_TYPE validationVal;
    constexpr float rndConst = 0.5;
    double cos_out;
    double sin_out;
    double angle_rads;
    cint32 ddsint32;
    uint32 phaseAccUsed = phaseAcc & phAngMask;

    angle_rads = ((phaseAccUsed * M_PI) / pow(2, 31));
    cos_out = cos(angle_rads); // angle in radians
    sin_out = sin(angle_rads); // angle in radians

    constexpr long int scaleDds = 32767; // multiplier to scale dds output
#if __SINCOS_IN_HW__ == 1
    retValraw = aie::sincos_complex(phaseAcc);
#endif
    retVal.real = retValraw.real;
    retVal.imag = retValraw.imag;
    ddsint32.real = floor(scaleDds * cos_out + rndConst); // TODO- floor? not rnd?
    ddsint32.imag = floor(scaleDds * sin_out + rndConst); // TODO- floor? not rnd?
    validationVal.real = ddsint32.real == scaleDds ? (scaleDds - 1) : ddsint32.real;
    validationVal.imag = ddsint32.imag == scaleDds ? (scaleDds - 1) : ddsint32.imag;
    if
        constexpr(std::is_same<T_ACC_TYPE, cfloat>()) {
            retVal.real = (float)retValraw.real / (float)(1 << 15);
            retVal.imag = (float)retValraw.imag / (float)(1 << 15);
            validationVal.real = (float)ddsint32.real / (float)(1 << 15);
            validationVal.imag = (float)ddsint32.imag / (float)(1 << 15);
        }

    validate_ref_data(validationVal, retVal, 3.0, "phaseToCartesian");

    return retVal;
};

template <typename T_ACC_TYPE,
          typename T_DDS_TYPE,
          unsigned int TP_NUM_LANES,
          unsigned int TP_NUM_LUTS,
          typename T_LUT_DTYPE,
          unsigned int TP_RND>
T_ACC_TYPE ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, TP_NUM_LANES, USE_LUT_SINCOS, TP_NUM_LUTS, T_LUT_DTYPE, TP_RND>::
    phaseToCartesian(uint32 phaseAcc) {
    if
        constexpr(std::is_same<T_ACC_TYPE, cfloat>::value) {
            T_ACC_TYPE retVal;
            T_DDS_TYPE validationVal;
            constexpr float rndConst = 0.5;
            double cos_out;
            double sin_out;
            double angle_rads;
            uint32 phaseAccUsed = phaseAcc & phAngMask;

            angle_rads = ((phaseAccUsed * M_PI) / pow(2, 31));
            cos_out = cos(angle_rads); // angle in radians
            sin_out = sin(angle_rads); // angle in radians

            constexpr long int scaleDds =
                ((int64)1 << ((sizeof(T_DDS_TYPE) / 2) * 8 - 1)) - 1; // multiplier to scale dds output
            T_ACC_TYPE sincosVal[TP_NUM_LUTS];
            for (int i = 0; i < TP_NUM_LUTS; i++) {
                sincosVal[i] = sincosLUT[i][(phaseAcc >> (32 - kNumLUTBits * (i + 1))) & 0x000003FF];
            }
            if
                constexpr(TP_NUM_LUTS == 1) {
                    retVal = sincosVal[0];
                    roundAccDDSLuts(0, retVal);
                }
            else if
                constexpr(TP_NUM_LUTS == 2) {
                    retVal = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(sincosVal[0], sincosVal[1], 0);
                    roundAccDDSLuts(0, retVal);
                }
            else if
                constexpr(TP_NUM_LUTS == 3) {
                    retVal = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(sincosVal[0], sincosVal[1], 0);
                    retVal = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(retVal, sincosVal[2], 0);
                    roundAccDDSLuts(0, retVal);
                }
            validationVal.real = (cos_out);
            validationVal.imag = (sin_out);
            validate_ref_data(validationVal, retVal, 0.0001, "phaseToCartesian");

            return retVal;
        }
    else {
        T_ACC_TYPE retVal;
        T_DDS_TYPE validationVal;
        constexpr float rndConst = 0.5;
        double cos_out;
        double sin_out;
        double angle_rads;
        cint32 ddsint32;
        uint32 phaseAccUsed = phaseAcc & phAngMask;

        angle_rads = ((phaseAccUsed * M_PI) / pow(2, 31));
        cos_out = cos(angle_rads); // angle in radians
        sin_out = sin(angle_rads); // angle in radians

        constexpr long int scaleDds =
            ((int64)1 << ((sizeof(T_DDS_TYPE) / 2) * 8 - 1)) - 1; // multiplier to scale dds output
        T_ACC_TYPE sincosVal[TP_NUM_LUTS];
        cint32 sincosValInter[TP_NUM_LUTS];
        T_ACC_TYPE tempVal;
        for (int i = 0; i < TP_NUM_LUTS; i++) {
            sincosValInter[i] = sincosLUT[i][(phaseAcc >> (32 - kNumLUTBits * (i + 1))) & 0x000003FF];
            sincosVal[i].real = sincosValInter[i].real;
            sincosVal[i].imag = sincosValInter[i].imag;
        }
        const int maxProdBits = 63;
        constexpr int shiftAmt = maxProdBits - (sizeof(T_DDS_TYPE) / 2 * 8);
        constexpr int shiftAmt1LUT = 32 - (sizeof(T_DDS_TYPE) / 2 * 8);
        if
            constexpr(TP_NUM_LUTS == 1) {
                tempVal = sincosVal[0];
                roundAccDDSLuts(shiftAmt1LUT, tempVal);
            }
        else if
            constexpr(TP_NUM_LUTS == 2) {
                tempVal = cmplxMult<cint64_t, 0>(sincosVal[0], sincosVal[1], 0);
                roundAccDDSLuts(shiftAmt, tempVal);
            }
        else if
            constexpr(TP_NUM_LUTS == 3) {
                tempVal = cmplxMult<cint64_t, 0>(sincosVal[0], sincosVal[1], 0);
                roundAccDDSLuts(31, tempVal);
                tempVal = cmplxMult<cint64_t, 0>(tempVal, sincosVal[2], 0);
                roundAccDDSLuts(shiftAmt, tempVal);
            }
        retVal.real = tempVal.real;
        retVal.imag = tempVal.imag;
        ddsint32.real = floor(scaleDds * cos_out * pow(((double)32767 / (double)32768), TP_NUM_LUTS) +
                              rndConst); // TODO- floor? not rnd?
        ddsint32.imag = floor(scaleDds * sin_out * pow(((double)32767 / (double)32768), TP_NUM_LUTS) +
                              rndConst); // TODO- floor? not rnd?
        validationVal.real = ddsint32.real == scaleDds ? (scaleDds - 1) : ddsint32.real;
        validationVal.imag = ddsint32.imag == scaleDds ? (scaleDds - 1) : ddsint32.imag;
        validate_ref_data(validationVal, retVal, 3.0, "phaseToCartesian");

        return retVal;
    }
};

template <typename T_ACC_TYPE,
          typename T_DDS_TYPE,
          unsigned int TP_NUM_LANES,
          unsigned int TP_SC_MODE,
          unsigned int TP_NUM_LUTS,
          typename T_LUT_DTYPE,
          unsigned int TP_RND>
void ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, TP_NUM_LANES, TP_SC_MODE, TP_NUM_LUTS, T_LUT_DTYPE, TP_RND>::
    populateRotVecInbuilt(unsigned int phaseInc, T_DDS_TYPE (&phRotref)[TP_NUM_LANES]) {
    for (int i = 0; i < TP_NUM_LANES; i++) {
        T_ACC_TYPE phRotRaw;
        phRotRaw = phaseToCartesian(phaseInc * i);
        phRotref[i].real = phRotRaw.real;
        phRotref[i].imag = phRotRaw.imag;
    }
}

template <typename T_ACC_TYPE,
          typename T_DDS_TYPE,
          unsigned int TP_NUM_LANES,
          unsigned int TP_NUM_LUTS,
          typename T_LUT_DTYPE,
          unsigned int TP_RND>
void ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, TP_NUM_LANES, USE_LUT_SINCOS, TP_NUM_LUTS, T_LUT_DTYPE, TP_RND>::
    populateRotVecLUT(unsigned int phaseInc,
                      T_LUT_DTYPE (&phRotSml)[TP_NUM_LANES],
                      T_LUT_DTYPE (&phRotBig)[TP_NUM_LANES]) {
    for (int i = 0; i < TP_NUM_LANES; i++) {
        T_ACC_TYPE phRotRaw;
        phRotRaw = phaseToCartesian(phaseInc * i);
        phRotSml[i].real = phRotRaw.real;
        phRotSml[i].imag = phRotRaw.imag;
        phRotRaw = phaseToCartesian(phaseInc * TP_NUM_LANES * i);
        phRotBig[i].real = phRotRaw.real;
        phRotBig[i].imag = phRotRaw.imag;
    }
}
//-------------------------------------------------------------------
// End of Utility functions

//-------------------------------------------------------------------
// Class member functions
// Constructors
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_SC_MODE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              TP_MIXER_MODE,
              TP_USE_PHASE_RELOAD,
              TP_SC_MODE,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecInbuilt(phaseInc, this->phRotref);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              TP_MIXER_MODE,
              TP_USE_PHASE_RELOAD,
              USE_LUT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecLUT(phaseInc, phRotSml, phRotBig);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_1,
              TP_USE_PHASE_RELOAD,
              USE_INBUILT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecInbuilt(phaseInc, phRotref);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_1,
              TP_USE_PHASE_RELOAD,
              USE_LUT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecLUT(phaseInc, phRotSml, phRotBig);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_0,
              TP_USE_PHASE_RELOAD,
              USE_INBUILT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecInbuilt(phaseInc, phRotref);
}
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_0,
              TP_USE_PHASE_RELOAD,
              USE_LUT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecLUT(phaseInc, phRotSml, phRotBig);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_2,
              USE_PHASE_RELOAD_TRUE,
              USE_INBUILT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_phaseValpre = initialPhaseOffset;
    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecInbuilt(phaseInc, phRotref);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_2,
              USE_PHASE_RELOAD_TRUE,
              USE_LUT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_phaseValpre = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecLUT(phaseInc, phRotSml, phRotBig);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_1,
              USE_PHASE_RELOAD_TRUE,
              USE_INBUILT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_phaseValpre = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecInbuilt(phaseInc, phRotref);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_1,
              USE_PHASE_RELOAD_TRUE,
              USE_LUT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_phaseValpre = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecLUT(phaseInc, phRotSml, phRotBig);
}

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_0,
              USE_PHASE_RELOAD_TRUE,
              USE_INBUILT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_phaseValpre = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecInbuilt(phaseInc, phRotref);
}
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
dds_mixer_ref<TT_DATA,
              TP_INPUT_WINDOW_VSIZE,
              MIXER_MODE_0,
              USE_PHASE_RELOAD_TRUE,
              USE_LUT_SINCOS,
              TP_NUM_LUTS,
              TP_RND,
              TP_SAT,
              TP_PHASE_RELOAD_API,
              TP_USE_PHASE_INC_RELOAD>::dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset) {
    this->m_phaseAccum = initialPhaseOffset;
    this->m_phaseValpre = initialPhaseOffset;

    this->m_samplePhaseInc = phaseInc;
    ddsFuncs.populateRotVecLUT(phaseInc, phRotSml, phRotBig);
}

// Non-constructors
// REF DDS function (default specialization for mixer mode 2)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_SC_MODE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   TP_MIXER_MODE,
                   TP_USE_PHASE_RELOAD,
                   TP_SC_MODE,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    input_buffer<TT_DATA>& __restrict inWindowB,
                                                    output_buffer<TT_DATA>& __restrict outWindow) {
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
    TT_DATA* in0Ptr = inWindowA.data();
    TT_DATA* in1Ptr = inWindowB.data();
    TT_DATA* outPtr = outWindow.data();
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(ddsOutPrime, phRot, ddsShift, TP_RND);
            ddsOutConj.real = ddsOut.real;
            ddsOutConj.imag = -ddsOut.imag;
            validate_ref_data(ddsOutValidation, ddsOut, 2.0, "mode 2");
            d_in = *in0Ptr++;
            d_in2 = *in1Ptr++;
            d_in64.real = d_in.real;
            d_in64.imag = d_in.imag;
            ddsMixerOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOut, 0, TP_RND);
            d_in64.real = d_in2.real;
            d_in64.imag = d_in2.imag;
            ddsMixerOut2 = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOutConj, 0, TP_RND);
            ddsMixerOutAcc.real = (ddsMixerOut.real + ddsMixerOut2.real);
            ddsMixerOutAcc.imag = (ddsMixerOut.imag + ddsMixerOut2.imag);
            roundAccDDS(mixerShift, ddsMixerOutAcc, TP_RND);
            mixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutAcc, TP_SAT);
            *outPtr++ = mixerOut;
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase over multiple input windows of data
        }
    }
};

// mode 2, phaseInc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_SC_MODE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   TP_MIXER_MODE,
                   TP_USE_PHASE_RELOAD,
                   TP_SC_MODE,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMix(inWindowA, inWindowB, outWindow);
};
//===========================================================
// SPECIALIZATION for mixer_mode = 2, LUT Based Implementation
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   TP_MIXER_MODE,
                   TP_USE_PHASE_RELOAD,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    input_buffer<TT_DATA>& __restrict inWindowB,
                                                    output_buffer<TT_DATA>& __restrict outWindow) {
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
    TT_DATA* in0Ptr = inWindowA.data();
    TT_DATA* in1Ptr = inWindowB.data();
    TT_DATA* outPtr = outWindow.data();
    int max = 1;
    if
        constexpr(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value) {
            max = (1 << (((sizeof(TT_DATA) / 2) * 8) - 1)) - 1;
        }
    double errTol = (double)(0.00023) * (double)(max);
    t_lutDataType phRotS;
    t_lutDataType phRotB;
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / (kNumLanes * kNumLanes); i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int cyc = 0; cyc < kNumLanes; cyc++) {
            phRotB = phRotBig[cyc];
            for (int l = 0; l < kNumLanes; l++) {
                phRotS = phRotSml[l];
                ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutPrime, phRotB, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOut, phRotS, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                ddsOutConj.real = ddsOut.real;
                ddsOutConj.imag = -ddsOut.imag;
                validate_ref_data(ddsOutValidation, ddsOut, errTol, "mode 2");
                d_in = *in0Ptr++;
                d_in2 = *in1Ptr++;
                d_in64.real = d_in.real;
                d_in64.imag = d_in.imag;
                ddsMixerOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOut, 0);
                d_in64.real = d_in2.real;
                d_in64.imag = d_in2.imag;
                ddsMixerOut2 = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOutConj, 0);
                ddsMixerOutAcc.real = (ddsMixerOut.real + ddsMixerOut2.real);
                ddsMixerOutAcc.imag = (ddsMixerOut.imag + ddsMixerOut2.imag);
                roundAccDDSLuts(mixerShift, ddsMixerOutAcc);
                mixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutAcc, TP_SAT);
                *outPtr++ = mixerOut;
                m_phaseAccum = m_phaseAccum + (m_samplePhaseInc);
            }
        }
    }
};
// SPECIALIZATION for mixer_mode = 2, LUT Based Implementation
// phase Int RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   TP_MIXER_MODE,
                   TP_USE_PHASE_RELOAD,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMix(inWindowA, inWindowB, outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   TP_USE_PHASE_RELOAD,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    output_buffer<TT_DATA>& __restrict outWindow) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    TT_DATA ddsOut;
    T_ACC_TYPE ddsOutraw;
    T_ACC_TYPE phRot;
    TT_DATA d_in;
    T_ACC_TYPE d_in64;
    TT_DATA d_in32;
    T_ACC_TYPE ddsMixerOutraw;
    TT_DATA ddsMixerOut;
    TT_DATA* inPtr = inWindowA.data();
    TT_DATA* outPtr = outWindow.data();
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOutraw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(ddsOutPrime, phRot, ddsShift, TP_RND);
            ddsOut = saturate<TT_DATA, T_ACC_TYPE>(ddsOutraw, TP_SAT);
            validate_ref_data(ddsOutValidation, ddsOut, 2.0, "mode 1");
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase over multiple input windows of data
            d_in = *inPtr++;
            d_in64.real = d_in.real;
            d_in64.imag = d_in.imag;
            d_in32 = saturate<TT_DATA, T_ACC_TYPE>(d_in64, TP_SAT);
            ddsMixerOutraw = cmplxMult<T_ACC_TYPE, 0, TT_DATA, TT_DATA>(d_in32, ddsOut, ddsShift, TP_RND);
            ddsMixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutraw, TP_SAT);
            // write single dds raf sample to output window
            *outPtr++ = ddsMixerOut;
        }
    }
};

// SPECIALIZATION for mixer_mode = 1
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   TP_USE_PHASE_RELOAD,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMix(inWindowA, outWindow);
};
//===========================================================
// SPECIALIZATION for mixer_mode = 1, LUT Based Implementation
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   TP_USE_PHASE_RELOAD,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    output_buffer<TT_DATA>& __restrict outWindow) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE ddsOut;
    T_ACC_TYPE phRot;
    TT_DATA d_in;
    T_ACC_TYPE d_in64;
    T_ACC_TYPE ddsMixerOutraw;
    TT_DATA ddsMixerOut;
    TT_DATA* inPtr = inWindowA.data();
    TT_DATA* outPtr = outWindow.data();
    int max = 1;
    if
        constexpr(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value) {
            max = (1 << (((sizeof(TT_DATA) / 2) * 8) - 1)) - 1;
        }
    double errTol = (double)(0.00023) * (double)(max);
    typedef TT_DATA T_DDS_TYPE;
    t_lutDataType phRotS;
    t_lutDataType phRotB;
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / (kNumLanes * kNumLanes); i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int cyc = 0; cyc < kNumLanes; cyc++) { // the big phase rotator stores rotations for every lane-th sample.
            phRotB = phRotBig[cyc];
            for (int l = 0; l < kNumLanes; l++) {
                phRotS = phRotSml[l];
                ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutPrime, phRotB, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOut, phRotS, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                validate_ref_data(ddsOutValidation, ddsOut, errTol, "mode 1");
                d_in = *inPtr++;
                d_in64.real = d_in.real;
                d_in64.imag = d_in.imag;
                ddsMixerOutraw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOut, 0);
                roundAccDDSLuts(ddsShift, ddsMixerOutraw);
                ddsMixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutraw, TP_SAT);
                m_phaseAccum = m_phaseAccum + (m_samplePhaseInc);
                *outPtr++ = ddsMixerOut;
            }
        }
    }
};

// SPECIALIZATION for mixer_mode = 1, LUT Based Implementation
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   TP_USE_PHASE_RELOAD,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMix(inWindowA, outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   TP_USE_PHASE_RELOAD,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(output_buffer<TT_DATA>& outWindow) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutRaw;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE phRot;
    TT_DATA ddsOut;
    TT_DATA* outPtr = outWindow.data();
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOutRaw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(ddsOutPrime, phRot, ddsShift, TP_RND);
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase per sample
            validate_ref_data(ddsOutValidation, ddsOutRaw, 3.0, "mode 0");
            ddsOut = saturate<TT_DATA, T_ACC_TYPE>(ddsOutRaw, TP_SAT);
            *outPtr++ = ddsOut;
        }
    }
};

// SPECIALIZATION for mixer_mode = 0
// phase Int RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   TP_USE_PHASE_RELOAD,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMix(outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0, LUT Based Implementation
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   TP_USE_PHASE_RELOAD,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(output_buffer<TT_DATA>& outWindow) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutRaw;
    int max = 1;
    if
        constexpr(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value) {
            max = (1 << (((sizeof(TT_DATA) / 2) * 8) - 1)) - 1;
        }
    double errTolPercent = TP_NUM_LUTS == 1 ? 0.013 : 0.00023;
    double errTol = (double)(errTolPercent) * (double)(max);
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE phRot;
    t_lutDataType phRotS;
    t_lutDataType phRotB;
    TT_DATA ddsOut;
    TT_DATA* outPtr = outWindow.data();
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / (kNumLanes * kNumLanes); i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int cyc = 0; cyc < kNumLanes; cyc++) { // the big phase rotator stores rotations for every lane-th sample.
            phRotB = phRotBig[cyc];
            for (int l = 0; l < kNumLanes; l++) {
                phRotS = phRotSml[l];
                ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
                ddsOutRaw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutPrime, phRotB, 0);
                roundAccDDSLuts(ddsShift, ddsOutRaw);
                ddsOutRaw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutRaw, phRotS, 0);
                roundAccDDSLuts(ddsShift, ddsOutRaw);
                // validate dds output before continuing using bit-accurate output
                validate_ref_data(ddsOutValidation, ddsOutRaw, errTol, "mode 0");
                // update phase_accum for next sample
                ddsOut.real = ddsOutRaw.real;
                ddsOut.imag = ddsOutRaw.imag;
                // write single dds raf sample to output window
                *outPtr++ = ddsOut;
                m_phaseAccum = m_phaseAccum + (m_samplePhaseInc);
            }
        }
    }
};

// SPECIALIZATION for mixer_mode = 0, LUT Based Implementation
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   TP_USE_PHASE_RELOAD,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMix(outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 2, phase offset input
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    input_buffer<TT_DATA>& __restrict inWindowB,
                                                    output_buffer<TT_DATA>& __restrict outWindow,
                                                    const unsigned int PhaseRTP) {
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
    TT_DATA* in0Ptr = inWindowA.data();
    TT_DATA* in1Ptr = inWindowB.data();
    TT_DATA* outPtr = outWindow.data();

    m_phaseAccum += (PhaseRTP - m_phaseValpre);
    m_phaseValpre = PhaseRTP;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(ddsOutPrime, phRot, ddsShift, TP_RND);
            ddsOutConj.real = ddsOut.real;
            ddsOutConj.imag = -ddsOut.imag;
            validate_ref_data(ddsOutValidation, ddsOut, 2.0, "mode 2");
            d_in = *in0Ptr++;
            d_in2 = *in1Ptr++;
            d_in64.real = d_in.real;
            d_in64.imag = d_in.imag;
            ddsMixerOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOut, 0, TP_RND);
            d_in64.real = d_in2.real;
            d_in64.imag = d_in2.imag;
            ddsMixerOut2 = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOutConj, 0, TP_RND);
            ddsMixerOutAcc.real = (ddsMixerOut.real + ddsMixerOut2.real);
            ddsMixerOutAcc.imag = (ddsMixerOut.imag + ddsMixerOut2.imag);
            roundAccDDS(mixerShift, ddsMixerOutAcc, TP_RND);
            mixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutAcc, TP_SAT);
            *outPtr++ = mixerOut;
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase over multiple input windows of data
        }
    }
};
// SPECIALIZATION for mixer_mode = 2, phase offset input
// phase int RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     output_buffer<TT_DATA>& __restrict outWindow,
                                                     const unsigned int PhaseRTP,
                                                     const unsigned int PhaseIncRTP) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMix(inWindowA, inWindowB, outWindow, PhaseRTP);
};

// overload for iobuffer input
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixB(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    // This overload simply calls the base overload since performance of ref model matters little.
    uint32* inPhaseOffsetPtr = inPhaseOffset.data();
    unsigned int PhaseRTP = *inPhaseOffsetPtr;
    this->ddsMix(inWindowA, inWindowB, outWindow, PhaseRTP);
};

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixD(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMixB(inWindowA, inWindowB, inPhaseOffset, outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 2, LUT Based Implementation, phase offset input
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    input_buffer<TT_DATA>& __restrict inWindowB,
                                                    output_buffer<TT_DATA>& __restrict outWindow,
                                                    const unsigned int PhaseRTP) {
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
    TT_DATA* in0Ptr = inWindowA.data();
    TT_DATA* in1Ptr = inWindowB.data();
    TT_DATA* outPtr = outWindow.data();

    m_phaseAccum += (PhaseRTP - m_phaseValpre);
    m_phaseValpre = PhaseRTP;

    int max = 1;
    if
        constexpr(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value) {
            max = (1 << (((sizeof(TT_DATA) / 2) * 8) - 1)) - 1;
        }
    double errTol = (double)(0.00023) * (double)(max);
    t_lutDataType phRotS;
    t_lutDataType phRotB;
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / (kNumLanes * kNumLanes); i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int cyc = 0; cyc < kNumLanes; cyc++) {
            phRotB = phRotBig[cyc];
            for (int l = 0; l < kNumLanes; l++) {
                phRotS = phRotSml[l];
                ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutPrime, phRotB, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOut, phRotS, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                ddsOutConj.real = ddsOut.real;
                ddsOutConj.imag = -ddsOut.imag;
                validate_ref_data(ddsOutValidation, ddsOut, errTol, "mode 2");
                d_in = *in0Ptr++;
                d_in2 = *in1Ptr++;
                d_in64.real = d_in.real;
                d_in64.imag = d_in.imag;
                ddsMixerOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOut, 0);
                d_in64.real = d_in2.real;
                d_in64.imag = d_in2.imag;
                ddsMixerOut2 = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOutConj, 0);
                ddsMixerOutAcc.real = (ddsMixerOut.real + ddsMixerOut2.real);
                ddsMixerOutAcc.imag = (ddsMixerOut.imag + ddsMixerOut2.imag);
                roundAccDDSLuts(mixerShift, ddsMixerOutAcc);
                mixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutAcc, TP_SAT);
                *outPtr++ = mixerOut;
                m_phaseAccum = m_phaseAccum + (m_samplePhaseInc);
            }
        }
    }
};

// SPECIALIZATION for mixer_mode = 2, LUT Based Implementation, phase offset input
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     output_buffer<TT_DATA>& __restrict outWindow,
                                                     const unsigned int PhaseRTP,
                                                     const unsigned int PhaseIncRTP) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMix(inWindowA, inWindowB, outWindow, PhaseRTP);
};

// overload for iobuffer input
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixB(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    // This overload simply calls the base overload since performance of ref model matters little.
    uint32* inPhaseOffsetPtr = inPhaseOffset.data();
    unsigned int PhaseRTP = *inPhaseOffsetPtr;
    this->ddsMix(inWindowA, inWindowB, outWindow, PhaseRTP);
};

// overload for iobuffer input
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_2,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixD(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<TT_DATA>& __restrict inWindowB,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMixB(inWindowA, inWindowB, inPhaseOffset, outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1 , phase offset input
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    output_buffer<TT_DATA>& __restrict outWindow,
                                                    const unsigned int PhaseRTP) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    TT_DATA ddsOut;
    T_ACC_TYPE ddsOutraw;
    T_ACC_TYPE phRot;
    TT_DATA d_in;
    T_ACC_TYPE d_in64;
    TT_DATA d_in32;
    T_ACC_TYPE ddsMixerOutraw;
    TT_DATA ddsMixerOut;
    TT_DATA* inPtr = inWindowA.data();
    TT_DATA* outPtr = outWindow.data();

    m_phaseAccum += (PhaseRTP - m_phaseValpre);
    m_phaseValpre = PhaseRTP;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOutraw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(ddsOutPrime, phRot, ddsShift, TP_RND);
            ddsOut = saturate<TT_DATA, T_ACC_TYPE>(ddsOutraw, TP_SAT);
            validate_ref_data(ddsOutValidation, ddsOut, 2.0, "mode 1");
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase over multiple input windows of data
            d_in = *inPtr++;
            d_in64.real = d_in.real;
            d_in64.imag = d_in.imag;
            d_in32 = saturate<TT_DATA, T_ACC_TYPE>(d_in64, TP_SAT);
            ddsMixerOutraw = cmplxMult<T_ACC_TYPE, 0, TT_DATA, TT_DATA>(d_in32, ddsOut, ddsShift, TP_RND);
            ddsMixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutraw, TP_SAT);
            // write single dds raf sample to output window
            *outPtr++ = ddsMixerOut;
        }
    }
};
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     output_buffer<TT_DATA>& __restrict outWindow,
                                                     const unsigned int PhaseRTP,
                                                     const unsigned int PhaseIncRTP) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMix(inWindowA, outWindow, PhaseRTP);
};

// overload for iobuffer phase offset input
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixB(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    // This overload simply calls the base overload since performance of ref model matters little.
    uint32* inPhaseOffsetPtr = inPhaseOffset.data();
    unsigned int PhaseRTP = *inPhaseOffsetPtr;
    this->ddsMix(inWindowA, outWindow, PhaseRTP);
};

// overload for iobuffer phase offset input
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixD(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMixB(inWindowA, inPhaseOffset, outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1, LUT Based Implementation, phase offset input
//===========================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(input_buffer<TT_DATA>& __restrict inWindowA,
                                                    output_buffer<TT_DATA>& __restrict outWindow,
                                                    const unsigned int PhaseRTP) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE ddsOut;
    T_ACC_TYPE phRot;
    TT_DATA d_in;
    T_ACC_TYPE d_in64;
    T_ACC_TYPE ddsMixerOutraw;
    TT_DATA ddsMixerOut;
    TT_DATA* inPtr = inWindowA.data();
    TT_DATA* outPtr = outWindow.data();

    m_phaseAccum += (PhaseRTP - m_phaseValpre);
    m_phaseValpre = PhaseRTP;

    int max = 1;
    if
        constexpr(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value) {
            max = (1 << (((sizeof(TT_DATA) / 2) * 8) - 1)) - 1;
        }
    double errTol = (double)(0.00023) * (double)(max);
    typedef TT_DATA T_DDS_TYPE;
    t_lutDataType phRotS;
    t_lutDataType phRotB;
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / (kNumLanes * kNumLanes); i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int cyc = 0; cyc < kNumLanes; cyc++) { // the big phase rotator stores rotations for every lane-th sample.
            phRotB = phRotBig[cyc];
            for (int l = 0; l < kNumLanes; l++) {
                phRotS = phRotSml[l];
                ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutPrime, phRotB, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                ddsOut = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOut, phRotS, 0);
                roundAccDDSLuts(ddsShift, ddsOut);
                validate_ref_data(ddsOutValidation, ddsOut, errTol, "mode 1");
                d_in = *inPtr++;
                d_in64.real = d_in.real;
                d_in64.imag = d_in.imag;
                ddsMixerOutraw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(d_in64, ddsOut, 0);
                roundAccDDSLuts(ddsShift, ddsMixerOutraw);
                ddsMixerOut = saturate<TT_DATA, T_ACC_TYPE>(ddsMixerOutraw, TP_SAT);
                m_phaseAccum = m_phaseAccum + (m_samplePhaseInc);
                *outPtr++ = ddsMixerOut;
            }
        }
    }
};

// SPECIALIZATION for mixer_mode = 1, LUT Based Implementation, phase offset input
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     output_buffer<TT_DATA>& __restrict outWindow,
                                                     const unsigned int PhaseRTP,
                                                     const unsigned int PhaseIncRTP) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMix(inWindowA, outWindow, PhaseRTP);
};

// overload for phase offset iobuffer input
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixB(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    // This overload simply calls the base overload since performance of ref model matters little.
    uint32* inPhaseOffsetPtr = inPhaseOffset.data();
    unsigned int PhaseRTP = *inPhaseOffsetPtr;
    this->ddsMix(inWindowA, outWindow, PhaseRTP);
};

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_1,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixD(input_buffer<TT_DATA>& __restrict inWindowA,
                                                     input_buffer<uint32>& __restrict inPhaseOffset,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& __restrict outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMixB(inWindowA, inPhaseOffset, outWindow);
};

// ========================================================================
// SPECIALIZATION for mixer_mode = 0, LUT Based Implementation, phase offset input
// ========================================================================

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(output_buffer<TT_DATA>& outWindow, const unsigned int PhaseRTP) {
    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutRaw;
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE phRot;
    TT_DATA ddsOut;
    TT_DATA* outPtr = outWindow.data();

    m_phaseAccum += (PhaseRTP - m_phaseValpre);
    m_phaseValpre = PhaseRTP;

    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / kNumLanes; i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int k = 0; k < kNumLanes; k++) {
            ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
            phRot.real = phRotref[k].real;
            phRot.imag = phRotref[k].imag;
            ddsOutRaw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, T_ACC_TYPE>(ddsOutPrime, phRot, ddsShift, TP_RND);
            m_phaseAccum = m_phaseAccum + (m_samplePhaseInc); // accumulate phase per sample
            validate_ref_data(ddsOutValidation, ddsOutRaw, 3.0, "mode 0");
            ddsOut = saturate<TT_DATA, T_ACC_TYPE>(ddsOutRaw, TP_SAT);
            *outPtr++ = ddsOut;
        }
    }
};

// SPECIALIZATION for mixer_mode = 0, LUT Based Implementation, phase offset input
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(output_buffer<TT_DATA>& outWindow,
                                                     const unsigned int PhaseRTP,
                                                     const unsigned int PhaseIncRTP) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMix(outWindow, PhaseRTP);
};

// phase offset iobuffer overload
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixB(input_buffer<uint32>& inPhaseOffset,
                                                     output_buffer<TT_DATA>& outWindow) {
    // This overload simply calls the base overload since performance of ref model matters little.
    uint32* inPhaseOffsetPtr = inPhaseOffset.data();
    unsigned int PhaseRTP = *inPhaseOffsetPtr;
    this->ddsMix(outWindow, PhaseRTP);
};

// phase offset iobuffer overload
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_INBUILT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixD(input_buffer<uint32>& inPhaseOffset,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecInbuilt(PhaseIncRTP, phRotref);
    }
    this->ddsMixB(inPhaseOffset, outWindow);
};

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          //   unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMix(output_buffer<TT_DATA>& outWindow, const unsigned int PhaseRTP) {
    m_phaseAccum += (PhaseRTP - m_phaseValpre);
    m_phaseValpre = PhaseRTP;

    T_ACC_TYPE ddsOutPrime;
    T_ACC_TYPE ddsOutRaw;
    int max = 1;
    if
        constexpr(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value) {
            max = (1 << (((sizeof(TT_DATA) / 2) * 8) - 1)) - 1;
        }
    double errTolPercent = TP_NUM_LUTS == 1 ? 0.013 : 0.00023;
    double errTol = (double)(errTolPercent) * (double)(max);
    T_ACC_TYPE ddsOutValidation;
    T_ACC_TYPE phRot;
    t_lutDataType phRotS;
    t_lutDataType phRotB;
    TT_DATA ddsOut;
    TT_DATA* outPtr = outWindow.data();
    for (unsigned int i = 0; i < TP_INPUT_WINDOW_VSIZE / (kNumLanes * kNumLanes); i++) {
        ddsOutPrime = ddsFuncs.phaseToCartesian(m_phaseAccum);
        for (int cyc = 0; cyc < kNumLanes; cyc++) { // the big phase rotator stores rotations for every lane-th sample.
            phRotB = phRotBig[cyc];
            for (int l = 0; l < kNumLanes; l++) {
                phRotS = phRotSml[l];
                ddsOutValidation = ddsFuncs.phaseToCartesian(m_phaseAccum);
                ddsOutRaw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutPrime, phRotB, 0);
                roundAccDDSLuts(ddsShift, ddsOutRaw);
                ddsOutRaw = cmplxMult<T_ACC_TYPE, 0, T_ACC_TYPE, t_lutDataType>(ddsOutRaw, phRotS, 0);
                roundAccDDSLuts(ddsShift, ddsOutRaw);
                // validate dds output before continuing using bit-accurate output
                validate_ref_data(ddsOutValidation, ddsOutRaw, errTol, "mode 0");
                // update phase_accum for next sample
                ddsOut.real = ddsOutRaw.real;
                ddsOut.imag = ddsOutRaw.imag;
                // write single dds raf sample to output window
                *outPtr++ = ddsOut;
                m_phaseAccum = m_phaseAccum + (m_samplePhaseInc);
            }
        }
    }
};

template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          //   unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixC(output_buffer<TT_DATA>& outWindow,
                                                     const unsigned int PhaseRTP,
                                                     const unsigned int PhaseIncRTP) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMix(outWindow, PhaseRTP);
};

// phase offset input by iobuffer overload
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          //   unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixB(input_buffer<uint32>& inPhaseOffset,
                                                     output_buffer<TT_DATA>& outWindow) {
    // This overload simply calls the base overload since performance of ref model matters little.
    uint32* inPhaseOffsetPtr = inPhaseOffset.data();
    unsigned int PhaseRTP = *inPhaseOffsetPtr;
    this->ddsMix(outWindow, PhaseRTP);
};

// phase offset input by iobuffer overload
// phase inc RTP
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          //   unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_PHASE_RELOAD_API,
          unsigned int TP_USE_PHASE_INC_RELOAD>
void dds_mixer_ref<TT_DATA,
                   TP_INPUT_WINDOW_VSIZE,
                   MIXER_MODE_0,
                   USE_PHASE_RELOAD_TRUE,
                   USE_LUT_SINCOS,
                   TP_NUM_LUTS,
                   TP_RND,
                   TP_SAT,
                   TP_PHASE_RELOAD_API,
                   TP_USE_PHASE_INC_RELOAD>::ddsMixD(input_buffer<uint32>& inPhaseOffset,
                                                     const unsigned int PhaseIncRTP,
                                                     output_buffer<TT_DATA>& outWindow) {
    if (PhaseIncRTP != this->m_samplePhaseInc) {
        m_samplePhaseInc = PhaseIncRTP;
        ddsFuncs.populateRotVecLUT(PhaseIncRTP, phRotSml, phRotBig);
    }
    this->ddsMixB(inPhaseOffset, outWindow);
};
}
}
}
}
}
