/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
#ifndef _DSPLIB_dds_mixer_REF_HPP_
#define _DSPLIB_dds_mixer_REF_HPP_

/*
DDS reference model
*/

//#define _DSPLIB_DDS_MIXER_REF_DEBUG_

#include <adf.h>
#include <limits>
#include "device_defs.h"
//#include "coarse_sincos.h"
//#include "fine_sincos.h"
#include "dds_luts.h"
#include "dds_luts_floats.h"
using namespace adf;
#define USE_INBUILT_SINCOS 0
#define USE_LUT_SINCOS 1

#define USE_PHASE_RELOAD_TRUE 1
#define USE_PHASE_RELOAD_FALSE 0

#define MIXER_MODE_0 0
#define MIXER_MODE_1 1
#define MIXER_MODE_2 2

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

template <typename T_DATA, unsigned int TP_SC_MODE>
constexpr unsigned int fnDDSLanes() {
    return 0;
}; // default is error trap
template <>
constexpr unsigned int fnDDSLanes<cint16, USE_INBUILT_SINCOS>() {
    return 8;
};
template <>
constexpr unsigned int fnDDSLanes<cint32, USE_INBUILT_SINCOS>() {
    return 4;
};
template <>
constexpr unsigned int fnDDSLanes<cfloat, USE_INBUILT_SINCOS>() {
    return 4;
};

#if __SUPPORTS_CFLOAT__ == 1
template <>
constexpr unsigned int fnDDSLanes<cint16, USE_LUT_SINCOS>() {
    return 8;
};
template <>
constexpr unsigned int fnDDSLanes<cint32, USE_LUT_SINCOS>() {
    return 4;
};
template <>
constexpr unsigned int fnDDSLanes<cfloat, USE_LUT_SINCOS>() {
    return 2;
};
#else
template <>
constexpr unsigned int fnDDSLanes<cint16, USE_LUT_SINCOS>() {
    return 8;
};
template <>
constexpr unsigned int fnDDSLanes<cint32, USE_LUT_SINCOS>() {
    return 8;
};
#endif

template <typename T_ACC_TYPE,
          typename T_DDS_TYPE,
          unsigned int TP_NUM_LANES,
          unsigned int TP_SC_MODE = USE_INBUILT_SINCOS,
          unsigned int TP_NUM_LUTS = 1,
          typename T_LUT_DTYPE = cint32_t,
          unsigned int TP_RND = 0>
class ddsMixerHelper {
   private:
    static constexpr unsigned int lookupBits = 20;
    static constexpr unsigned int phAngMask = ((1 << lookupBits) - 1) << (32 - lookupBits);

   public:
    T_ACC_TYPE phaseToCartesian(uint32 phaseAcc);
    void populateRotVecInbuilt(unsigned int phaseInc, T_DDS_TYPE (&phRotref)[TP_NUM_LANES]);

    ddsMixerHelper(){};
};

template <typename T_ACC_TYPE,
          typename T_DDS_TYPE,
          unsigned int TP_NUM_LANES,
          unsigned int TP_NUM_LUTS,
          typename T_LUT_DTYPE,
          unsigned int TP_RND>
class ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, TP_NUM_LANES, USE_LUT_SINCOS, TP_NUM_LUTS, T_LUT_DTYPE, TP_RND> {
   private:
    static constexpr unsigned int kNumLUTBits = 10;
    static constexpr unsigned int kLUTSize = 1 << kNumLUTBits;
    static constexpr unsigned int lookupBits = TP_NUM_LUTS * kNumLUTBits;
    static constexpr unsigned int phAngMask = ((1 << lookupBits) - 1) << (32 - lookupBits);

   public:
    T_ACC_TYPE phaseToCartesian(uint32 phaseAcc);
    void populateRotVecLUT(unsigned int phaseInc,
                           T_LUT_DTYPE (&phRotSml)[TP_NUM_LANES],
                           T_LUT_DTYPE (&phRotBig)[TP_NUM_LANES]);
    alignas(32) T_LUT_DTYPE* sincosLUT[TP_NUM_LUTS];
    ddsMixerHelper() {
#if __SUPPORTS_CFLOAT__ == 1
        using T_DUMMY_TYPE = cfloat;
#else
        using T_DUMMY_TYPE = cint16;
#endif
        if
            constexpr(std::is_same<T_ACC_TYPE, T_DUMMY_TYPE>::value) {
#if __SUPPORTS_CFLOAT__ == 1
                sincosLUT[0] = (T_ACC_TYPE*)sincosLUTFloat1;
                if
                    constexpr(TP_NUM_LUTS > 1) { sincosLUT[1] = (T_ACC_TYPE*)sincosLUTFloat2; }
                if
                    constexpr(TP_NUM_LUTS > 2) { sincosLUT[2] = (T_ACC_TYPE*)sincosLUTFloat3; }
#endif
            }
        else {
            sincosLUT[0] = (cint32*)sincosLUTCoarse32;
            if
                constexpr(TP_NUM_LUTS > 1) { sincosLUT[1] = (cint32*)sincosLUTFine32; }
            if
                constexpr(TP_NUM_LUTS > 2) { sincosLUT[2] = (cint32*)sincosLUTFiner32; }
        }
    };
};
//-----------------------------------------------------------------------------------------------------
// dds_mixer_ref class MIXER-MODE 2 : USE_INBUILT_SINCOS
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_SC_MODE = USE_INBUILT_SINCOS,
          unsigned int TP_NUM_LUTS = 1,
          unsigned int TP_RND = 0,
          unsigned int TP_SAT = 1>
class dds_mixer_ref {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_INBUILT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    using T_INT_BASE = typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16, int32>::type;
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16_t, TT_DATA>::type T_DDS_TYPE;
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut
    static constexpr int mixerShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 16; // similar to above, but with addition bit growth. Doesn't apply
                                                       // to float because that would require additional ops in UUT
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    using T_BASE_DATA = typename std::conditional<std::is_same<TT_DATA, cfloat>::value, float, T_INT_BASE>::type;
#else
    static constexpr int ddsShift = 15;
    static constexpr int mixerShift = 16;
    typedef cint64 T_ACC_TYPE;
    using T_BASE_DATA = T_INT_BASE;
#endif
    T_DDS_TYPE phRotref[kNumLanes];

   public:
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, TP_SC_MODE, TP_NUM_LUTS> ddsFuncs;
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA, input_buffer<TT_DATA>& inWindowB, output_buffer<TT_DATA>& outWindow);
};

// dds_mixer_ref class MIXER-MODE 2 : USE_LUT_SINCOS
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    TP_MIXER_MODE,
                    TP_USE_PHASE_RELOAD,
                    USE_LUT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_LUT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    using T_INT_BASE = typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16, int32>::type;
    typedef TT_DATA T_DDS_TYPE;
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : sizeof(TT_DATA) / 2 * 8 - 1;
    static constexpr int mixerShift = std::is_same<TT_DATA, cfloat>::value ? 0 : ddsShift + 1;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    using T_BASE_DATA = typename std::conditional<std::is_same<TT_DATA, cfloat>::value, float, T_INT_BASE>::type;
#else
    static constexpr int ddsShift = sizeof(TT_DATA) / 2 * 8 - 1;
    static constexpr int mixerShift = ddsShift + 1;
    typedef cint64 T_ACC_TYPE;
    using T_BASE_DATA = T_INT_BASE;
#endif
    using t_lutDataType =
        typename std::conditional<(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value),
                                  cint32,
                                  TT_DATA>::type;
    t_lutDataType phRotBig[kNumLanes];
    t_lutDataType phRotSml[kNumLanes];

   public:
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_LUT_SINCOS, TP_NUM_LUTS, t_lutDataType> ddsFuncs;
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA, input_buffer<TT_DATA>& inWindowB, output_buffer<TT_DATA>& outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1:  USE_INBUILT_SINCOS
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_1,
                    TP_USE_PHASE_RELOAD,
                    USE_INBUILT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_INBUILT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    T_DDS_TYPE phRotref[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 15;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = 15;
    typedef cint64 T_ACC_TYPE;
#endif

    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_INBUILT_SINCOS, TP_NUM_LUTS> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA, output_buffer<TT_DATA>& outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1 :  USE_LUT_SINCOS
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_1,
                    TP_USE_PHASE_RELOAD,
                    USE_LUT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_LUT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    typedef TT_DATA T_DDS_TYPE;
    using t_lutDataType =
        typename std::conditional<(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value),
                                  cint32,
                                  TT_DATA>::type;
    t_lutDataType phRotBig[kNumLanes];
    t_lutDataType phRotSml[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : sizeof(TT_DATA) / 2 * 8 - 1;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = sizeof(TT_DATA) / 2 * 8 - 1;
    typedef cint64 T_ACC_TYPE;
#endif
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_LUT_SINCOS, TP_NUM_LUTS, t_lutDataType> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA, output_buffer<TT_DATA>& outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0: USE_INBUILT_SINCOS
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_0,
                    TP_USE_PHASE_RELOAD,
                    USE_INBUILT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_INBUILT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    T_DDS_TYPE phRotref[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = 15;
    typedef cint64 T_ACC_TYPE;
#endif
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_INBUILT_SINCOS, TP_NUM_LUTS> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(output_buffer<TT_DATA>& outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0 : USE_LUT_SINCOS
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_USE_PHASE_RELOAD,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_0,
                    TP_USE_PHASE_RELOAD,
                    USE_LUT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_LUT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows

    typedef TT_DATA T_DDS_TYPE;
    using t_lutDataType =
        typename std::conditional<(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value),
                                  cint32,
                                  TT_DATA>::type;
    t_lutDataType phRotBig[kNumLanes];
    t_lutDataType phRotSml[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : sizeof(TT_DATA) / 2 * 8 - 1;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = sizeof(TT_DATA) / 2 * 8 - 1;
    typedef cint64 T_ACC_TYPE;
#endif
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_LUT_SINCOS, TP_NUM_LUTS, t_lutDataType> ddsFuncs;

   public:
    //     // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    //     // DDS
    void ddsMix(output_buffer<TT_DATA>& outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 2 : USE_INBUILT_SINCOS : RTP Enabled
//===========================================================

template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_2,
                    USE_PHASE_RELOAD_TRUE,
                    USE_INBUILT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_INBUILT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    unsigned int m_phaseValpre = 0;

    using T_INT_BASE = typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16, int32>::type;
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16_t, TT_DATA>::type T_DDS_TYPE;
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut
    static constexpr int mixerShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 16; // similar to above, but with addition bit growth. Doesn't apply
                                                       // to float because that would require additional ops in UUT
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    using T_BASE_DATA = typename std::conditional<std::is_same<TT_DATA, cfloat>::value, float, T_INT_BASE>::type;
#else
    static constexpr int ddsShift = 15;
    static constexpr int mixerShift = 16;
    typedef cint64 T_ACC_TYPE;
    using T_BASE_DATA = T_INT_BASE;
#endif
    T_DDS_TYPE phRotref[kNumLanes];

   public:
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_INBUILT_SINCOS, TP_NUM_LUTS> ddsFuncs;
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA,
                input_buffer<TT_DATA>& inWindowB,
                output_buffer<TT_DATA>& outWindow,
                const unsigned int PhaseRTP);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 2 : USE_LUT_SINCOS : RTP Enabled
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_2,
                    USE_PHASE_RELOAD_TRUE,
                    USE_LUT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_LUT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    unsigned int m_phaseValpre = 0;

    using T_INT_BASE = typename std::conditional<std::is_same<TT_DATA, cint16>::value, int16, int32>::type;
    typedef TT_DATA T_DDS_TYPE;
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : sizeof(TT_DATA) / 2 * 8 - 1;
    static constexpr int mixerShift = std::is_same<TT_DATA, cfloat>::value ? 0 : ddsShift + 1;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
    using T_BASE_DATA = typename std::conditional<std::is_same<TT_DATA, cfloat>::value, float, T_INT_BASE>::type;
#else
    static constexpr int ddsShift = sizeof(TT_DATA) / 2 * 8 - 1;
    static constexpr int mixerShift = ddsShift + 1;
    typedef cint64 T_ACC_TYPE;
    using T_BASE_DATA = T_INT_BASE;
#endif
    using t_lutDataType =
        typename std::conditional<(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value),
                                  cint32,
                                  TT_DATA>::type;
    t_lutDataType phRotBig[kNumLanes];
    t_lutDataType phRotSml[kNumLanes];

   public:
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_LUT_SINCOS, TP_NUM_LUTS, t_lutDataType> ddsFuncs;
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA,
                input_buffer<TT_DATA>& inWindowB,
                output_buffer<TT_DATA>& outWindow,
                const unsigned int PhaseRTP);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1:  USE_INBUILT_SINCOS : RTP Enabled
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_1,
                    USE_PHASE_RELOAD_TRUE,
                    USE_INBUILT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_INBUILT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    unsigned int m_phaseValpre = 0;

    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    T_DDS_TYPE phRotref[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 15;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = 15;
    typedef cint64 T_ACC_TYPE;
#endif

    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_INBUILT_SINCOS, TP_NUM_LUTS> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA, output_buffer<TT_DATA>& outWindow, const unsigned int PhaseRTP);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1 :  USE_LUT_SINCOS  : RTP Enabled
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_1,
                    USE_PHASE_RELOAD_TRUE,
                    USE_LUT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_LUT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    unsigned int m_phaseValpre = 0;

    typedef TT_DATA T_DDS_TYPE;
    using t_lutDataType =
        typename std::conditional<(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value),
                                  cint32,
                                  TT_DATA>::type;
    t_lutDataType phRotBig[kNumLanes];
    t_lutDataType phRotSml[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : sizeof(TT_DATA) / 2 * 8 - 1;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = sizeof(TT_DATA) / 2 * 8 - 1;
    typedef cint64 T_ACC_TYPE;
#endif
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_LUT_SINCOS, TP_NUM_LUTS, t_lutDataType> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_buffer<TT_DATA>& inWindowA, output_buffer<TT_DATA>& outWindow, const unsigned int PhaseRTP);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0: USE_INBUILT_SINCOS : RTP Enabled
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>

class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_0,
                    USE_PHASE_RELOAD_TRUE,
                    USE_INBUILT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_INBUILT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    unsigned int m_phaseValpre = 0;

    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    T_DDS_TYPE phRotref[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift =
        std::is_same<TT_DATA, cfloat>::value ? 0 : 15; // compensation for fixed precision ddsOut
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = 15;
    typedef cint64 T_ACC_TYPE;
#endif
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_INBUILT_SINCOS, TP_NUM_LUTS> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(output_buffer<TT_DATA>& outWindow, const unsigned int PhaseRTP);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0 : USE_LUT_SINCOS : RTP Enabled
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_NUM_LUTS,
          unsigned int TP_RND,
          unsigned int TP_SAT>
class dds_mixer_ref<TT_DATA,
                    TP_INPUT_WINDOW_VSIZE,
                    MIXER_MODE_0,
                    USE_PHASE_RELOAD_TRUE,
                    USE_LUT_SINCOS,
                    TP_NUM_LUTS,
                    TP_RND,
                    TP_SAT> {
   private:
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA, USE_LUT_SINCOS>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    unsigned int m_phaseValpre = 0;
    typedef TT_DATA T_DDS_TYPE;
    using t_lutDataType =
        typename std::conditional<(std::is_same<TT_DATA, cint16>::value || std::is_same<TT_DATA, cint32>::value),
                                  cint32,
                                  TT_DATA>::type;
    t_lutDataType phRotBig[kNumLanes];
    t_lutDataType phRotSml[kNumLanes];
#if __SUPPORTS_CFLOAT__ == 1
    static constexpr int ddsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : sizeof(TT_DATA) / 2 * 8 - 1;
    typedef typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cint64>::type T_ACC_TYPE;
#else
    static constexpr int ddsShift = sizeof(TT_DATA) / 2 * 8 - 1;
    typedef cint64 T_ACC_TYPE;
#endif
    ddsMixerHelper<T_ACC_TYPE, T_DDS_TYPE, kNumLanes, USE_LUT_SINCOS, TP_NUM_LUTS, t_lutDataType> ddsFuncs;

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(output_buffer<TT_DATA>& outWindow, const unsigned int PhaseRTP);
};
}
}
}
}
}

#endif // _DSPLIB_dds_mixer_REF_HPP_
