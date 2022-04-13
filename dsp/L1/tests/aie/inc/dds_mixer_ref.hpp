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
#ifndef _DSPLIB_dds_mixer_REF_HPP_
#define _DSPLIB_dds_mixer_REF_HPP_

/*
DDS reference model
*/

//#define _DSPLIB_DDS_MIXER_REF_DEBUG_

#include <adf.h>
#include <limits>
namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

// This mimics the uut function ddsMulVecScalarLanes.
template <typename T_DATA>
constexpr unsigned int fnDDSLanes() {
    return 0;
}; // default is error trap
template <>
constexpr unsigned int fnDDSLanes<cint16>() {
    return 8;
};
template <>
constexpr unsigned int fnDDSLanes<cint32>() {
    return 4;
};
template <>
constexpr unsigned int fnDDSLanes<cfloat>() {
    return 4;
};

//-----------------------------------------------------------------------------------------------------
// dds_mixer_ref class
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE>
class dds_mixer_ref {
   private:
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    T_DDS_TYPE phRotref[kNumLanes];

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);
    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_window<TT_DATA>* inWindowA, input_window<TT_DATA>* inWindowB, output_window<TT_DATA>* outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE>
class dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, 1> {
   private:
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    T_DDS_TYPE phRotref[kNumLanes];

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(input_window<TT_DATA>* inWindowA, output_window<TT_DATA>* outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0
//===========================================================
template <typename TT_DATA, // type of data input and output
          unsigned int TP_INPUT_WINDOW_VSIZE>
class dds_mixer_ref<TT_DATA, TP_INPUT_WINDOW_VSIZE, 0> {
   private:
    typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16, TT_DATA>::type T_DDS_TYPE;
    static constexpr unsigned int kNumLanes = fnDDSLanes<TT_DATA>();
    unsigned int m_samplePhaseInc;
    unsigned int m_phaseAccum = 0; // used to accumulate over multiple input windows
    T_DDS_TYPE phRotref[kNumLanes];

   public:
    // Constructor
    dds_mixer_ref(uint32_t phaseInc, uint32_t initialPhaseOffset = 0);

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer_ref::ddsMix); }
    // DDS
    void ddsMix(output_window<TT_DATA>* outWindow);
};
}
}
}
}
}

#endif // _DSPLIB_dds_mixer_REF_HPP_
