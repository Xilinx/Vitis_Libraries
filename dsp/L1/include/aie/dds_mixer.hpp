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
#ifndef _DSPLIB_DDS_MIXER_HPP_
#define _DSPLIB_DDS_MIXER_HPP_

/*
DDS Mixer.
This file exists to capture the definition of the dds_mixer kernel class.
The class definition holds defensive checks on parameter range and other
legality.
The constructor definition is held in this class because this class must be
accessible to graph level aie compilation.
The main runtime ddsMix function is captured elsewhere as it contains aie
intrinsics which are not included in aie graph level
compilation.
*/

/* Coding conventions
   TT_      template type suffix
   TP_      template parameter suffix
*/

/* Design Notes
*/

#include <adf.h>
#include "fir_utils.hpp"
#include "dds_mixer_traits.hpp"
#include <vector>

//#define _DSPLIB_DDS_MIXER_HPP_DEBUG_

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_API = IO_API::WINDOW>
class kernelDdsMixerClass {
   public:
    // 0 = rnd_floor, 1 = rnd_ceil, 2 = rnd_pos_inf, 3 = rnd_neg_inf, 4 = rnd_sym_inf, 5 = rnd_sym_zero, 6 =
    // rnd_conv_even, 7 = rnd_conv_odd
    static constexpr unsigned int kRoundMode = 0;                                // mode used in point designs.
    static constexpr unsigned int m_kNumLanes = ddsMulVecScalarLanes<TT_DATA>(); // todo - calculate for other types
    // todo, make these parameters in future release.
    static constexpr unsigned int m_kDdsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 15;
    static constexpr unsigned int m_kMixerShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 16;
    // Keeps track of where we are in sincos curve, incremented by phaseIncr; initial value set in constructor
    static constexpr unsigned int m_kLoopCount = (TP_INPUT_WINDOW_VSIZE / m_kNumLanes);
    static constexpr unsigned int m_kNumMixerInputs =
        (TP_MIXER_MODE == MIXER_MODE_2) ? 2 : (TP_MIXER_MODE == MIXER_MODE_1) ? 1 : 0;

    // typedef typename std::conditional<std::is_same<TT_DATA, cint32>::value, cint16_t, TT_DATA>::type T_DDS_TYPE;
    using T_DDS_TYPE = cint16; // true for TT_DATA=cint16 or cint32. Cfloat is handled by a specialization of this
                               // class.

    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
                  "ERROR: DDS Mixer TT_DATA is not a supported type (Must be cint16).");
    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
                  "ERROR: DDS Mixer TP_INPUT_WINDOW_VSIZE must be a multiple of m_kNumLanes ");
    static_assert(fnEnumType<TT_DATA>() != enumCint32 || TP_MIXER_MODE != MIXER_MODE_0,
                  "ERROR: cint32 is not support for DDS output.");

    using T_inType = typename std::conditional<(TP_API == WINDOW), input_window<TT_DATA>, input_stream<TT_DATA> >::type;
    using T_outType =
        typename std::conditional<(TP_API == WINDOW), output_window<TT_DATA>, output_stream<TT_DATA> >::type;
    using T_inIF = T_inputIF<TT_DATA, T_inType, m_kNumMixerInputs>;
    using T_outIF = T_outputIF<TT_DATA, T_outType>;

    using T_acc = typename std::conditional<std::is_same<TT_DATA, cint16>::value, cacc48, cacc80>::type;

    unsigned int m_phaseIndex = 0;
    unsigned int m_perCyclePhaseInc;
    alignas(32) T_DDS_TYPE m_phRot[m_kNumLanes];

    // Constructor - use aie_api so definition within kernel scope
    kernelDdsMixerClass(unsigned int phaseInc);
    // Constructor with initialOffset
    kernelDdsMixerClass(unsigned int phaseInc, unsigned int initialPhaseOffset);

    // for ddsKernel overloads
    using T_inIF_mm2 = T_inputIF<TT_DATA, T_inType, 2>;
    using T_inIF_mm1 = T_inputIF<TT_DATA, T_inType, 1>;

    // DDS Kernel
    // Use overoads rather than class specialisations to keep hiearchy a bit more simple
    // and avoid requirement of dependant names with this->variable.
    // mixer mode 2
    void ddsKernel(T_inIF_mm2 inInterface, T_outIF outInterface);
    // mixer mode 1
    void ddsKernel(T_inIF_mm1 inInterface, T_outIF outInterface);
    // mixer mode 0
    void ddsKernel(T_outIF outInterface);
};

template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
class kernelDdsMixerClass<cfloat, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API> {
   public:
    using TT_DATA = cfloat;
    static constexpr unsigned int kRoundMode = 0;                               // mode used in point designs.
    static constexpr unsigned int m_kNumLanes = ddsMulVecScalarLanes<cfloat>(); // todo - calculate for other types
    static constexpr unsigned int m_kSamplesInReg = 1024 / sizeof(cfloat);      // 16, but this phrasing explains more.
    // todo, make these parameters in future release.
    static constexpr unsigned int m_kDdsShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 15;
    static constexpr unsigned int m_kMixerShift = std::is_same<TT_DATA, cfloat>::value ? 0 : 16;
    static constexpr unsigned int m_kLoopCount = (TP_INPUT_WINDOW_VSIZE / m_kNumLanes);
    static constexpr unsigned int m_kNumMixerInputs =
        (TP_MIXER_MODE == MIXER_MODE_2) ? 2 : (TP_MIXER_MODE == MIXER_MODE_1) ? 1 : 0;
    static constexpr float m_kInt2floatScale = (float)1.0 / (float)(1 << 15);

    static_assert(TP_MIXER_MODE <= 2, "ERROR: DDS Mixer Mode must be 0, 1 or 2. ");
    static_assert(fnEnumType<TT_DATA>() != enumUnknownType,
                  "ERROR: DDS Mixer TT_DATA is not a supported type (Must be cint16).");
    static_assert((TP_INPUT_WINDOW_VSIZE % m_kNumLanes) == 0,
                  "ERROR: DDS Mixer TP_INPUT_WINDOW_VSIZE must be a multiple of m_kNumLanes ");

    using T_inType = typename std::conditional<(TP_API == WINDOW), input_window<TT_DATA>, input_stream<TT_DATA> >::type;
    using T_outType =
        typename std::conditional<(TP_API == WINDOW), output_window<TT_DATA>, output_stream<TT_DATA> >::type;
    using T_inIF = T_inputIF<TT_DATA, T_inType, m_kNumMixerInputs>;
    using T_outIF = T_outputIF<TT_DATA, T_outType>;
    using T_acc = cfloat; // typename std::conditional<std::is_same<TT_DATA, cfloat>::value, cfloat, cacc48>::type;
                          // //cacc48; // todo for other types

    // Keeps track of where we are in sincos curve, incremented by phaseIncr; initial value set in constructor
    unsigned int m_phaseIndex = 0;
    unsigned int m_perCyclePhaseInc;
    alignas(32) TT_DATA m_phRot[m_kNumLanes];

    // Constructor - use aie_api so definition within kernel scope
    kernelDdsMixerClass(unsigned int phaseInc);
    // Constructor with initialOffset
    kernelDdsMixerClass(unsigned int phaseInc, unsigned int initialPhaseOffset);

    // for ddsKernel overloads
    using T_inIF_mm2 = T_inputIF<TT_DATA, T_inType, 2>;
    using T_inIF_mm1 = T_inputIF<TT_DATA, T_inType, 1>;

    // DDS Kernel
    // Use overoads rather than class specialisations to keep hiearchy a bit more simple
    // and avoid requirement of dependant names with this->variable.
    // mixer mode 2
    void ddsKernel(T_inIF_mm2 inInterface, T_outIF outInterface);
    // mixer mode 1
    void ddsKernel(T_inIF_mm1 inInterface, T_outIF outInterface);
    // mixer mode 0
    void ddsKernel(T_outIF outInterface);
};

//-----------------------------------------------------------------------------------------------------
// Default specialization of kernel entry class, also for MIXER_MODE=2
template <typename TT_DATA,
          unsigned int TP_INPUT_WINDOW_VSIZE,
          unsigned int TP_MIXER_MODE,
          unsigned int TP_API = IO_API::WINDOW>
class dds_mixer : public kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API> {
   public:
    // Help the compiler deal with dependant names
    using baseClass = kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>;
    using thisClass = dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>;
    using T_inType = typename thisClass::T_inType;
    using T_outType = typename thisClass::T_outType;
    using T_inIF = typename thisClass::T_inIF;
    using T_outIF = typename thisClass::T_outIF;
    using T_acc = typename thisClass::T_acc;

    // Constructor
    dds_mixer(unsigned int phaseInc) : baseClass(phaseInc) {}

    // Constructor with phaseOffset - for SSR
    dds_mixer(unsigned int phaseInc, unsigned int initialPhaseOffset) : baseClass(phaseInc, initialPhaseOffset) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer::ddsMix); }

    // dds
    void ddsMix(T_inType* __restrict inWindowA, T_inType* __restrict inWindowB, T_outType* __restrict outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 1
//===============
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
class dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API>
    : public kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API> {
   private:
   public:
    // Help the compiler deal with dependant names
    using baseClass = kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API>;
    using thisClass = dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API>;
    using T_inType = typename thisClass::T_inType;
    using T_outType = typename thisClass::T_outType;
    using T_inIF = typename thisClass::T_inIF;
    using T_outIF = typename thisClass::T_outIF;
    using T_acc = typename thisClass::T_acc;

    // Constructor
    dds_mixer(unsigned int phaseInc) : baseClass(phaseInc) {}

    // Constructor with phaseOffset - for SSR
    dds_mixer(unsigned int phaseInc, unsigned int initialPhaseOffset) : baseClass(phaseInc, initialPhaseOffset) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer::ddsMix); }

    // dds
    void ddsMix(T_inType* __restrict inWindowA, T_outType* restrict outWindow);
};

//===========================================================
// SPECIALIZATION for mixer_mode = 0
//===============
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
class dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API>
    : public kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API> {
   private:
   public:
    // Help the compiler deal with dependant names
    using baseClass = kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API>;
    using thisClass = dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API>;
    using T_inType = typename thisClass::T_inType;
    using T_outType = typename thisClass::T_outType;
    using T_inIF = typename thisClass::T_inIF;
    using T_outIF = typename thisClass::T_outIF;
    using T_acc = typename thisClass::T_acc;

    // Constructor
    dds_mixer(unsigned int phaseInc) : baseClass(phaseInc) {}

    // Constructor with phaseOffset - for SSR
    dds_mixer(unsigned int phaseInc, unsigned int initialPhaseOffset) : baseClass(phaseInc, initialPhaseOffset) {}

    // Register Kernel Class
    static void registerKernelClass() { REGISTER_FUNCTION(dds_mixer::ddsMix); }

    // dds
    void ddsMix(T_outType* __restrict outWindow);
};
}
}
}
}
}

#endif // _DSPLIB_DDS_MIXER_HPP_
