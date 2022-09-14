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
DDS Mixer code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

#include <adf.h>

#define __AIE_API_USE_NATIVE_1024B_VECTOR__
// Include for AIE API items
#include "aie_api/aie_adf.hpp"

#include "dds_mixer_traits.hpp"
#include "dds_mixer.hpp"
#include "dds_mixer_utils.hpp"
#include "kernel_api_utils.hpp"
#include "aie_api/utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

// aie_api is external to xf::dsp::aie namespace
namespace aie = ::aie;

//==============================================================================
// integer specializations

// Constructor to populate m_phRot array
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::kernelDdsMixerClass(uint32_t phaseInc) {
    m_phaseIndex = 0;
    // Calculate the phase increment for each clock cycle ( = per sample inc * input vector size)
    m_perCyclePhaseInc = phaseInc * m_kNumLanes;

    // calculate per-lane offset angle (as Cartesian)
    for (unsigned int i = 0; i < m_kNumLanes; i++) {
        m_phRot[i] = aie::sincos_complex(i * phaseInc);
    }
}

// Constructor to populate m_phRot array with an initial offset
// making this overload so that the default case doesn't get extra penalty of additions and sets with trivial value of
// 0.
// This enables SSR DDS
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::kernelDdsMixerClass(
    uint32_t phaseInc, uint32_t initialPhaseOffset)
    : kernelDdsMixerClass(phaseInc) {
    // initialise phase accumulator index to offset.
    // enhancement? if initialPhaseOffset was a template argument, then we could just set this at initialisation without
    // penalty
    m_phaseIndex = initialPhaseOffset;
}

// DDS_Mixer run-time function
// Overload for TP_MIXER_MODE=2
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
INLINE_DECL void kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_inIF_mm2 inInterface, T_outIF outInterface) {
    // pointer ww to m_phRot array
    // const aie::vector<TT_DATA,m_kNumLanes> *ww = (const aie::vector<TT_DATA,m_kNumLanes> *) m_phRot;
    const aie::vector<T_DDS_TYPE, m_kNumLanes>* ww = (const aie::vector<T_DDS_TYPE, m_kNumLanes>*)m_phRot;

    // AIE API declarations
    // only ever use index 0 dds_out, but phrasing this as a vector seems to help the compiler pipeline a lot better.
    using T_accint16Vect0 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(bm0);
    using T_accint32Vect0 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(aml0);
    using T_accVect0 =
        typename std::conditional<std::is_same<TT_DATA, cint16>::value, T_accint16Vect0, T_accint32Vect0>::type;
    using T_accint16Vect1 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(bm1);
    using T_accint32Vect1 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(aml1);
    using T_accVect1 =
        typename std::conditional<std::is_same<TT_DATA, cint16>::value, T_accint16Vect1, T_accint32Vect1>::type;
    using T_accint16Vect2 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(bm2);
    using T_accint32Vect2 = typename ::aie::accum<cacc80, m_kNumLanes> chess_storage(bm2);
    using T_accVect2 =
        typename std::conditional<std::is_same<TT_DATA, cint16>::value, T_accint16Vect2, T_accint32Vect2>::type;

    aie::vector<T_DDS_TYPE, m_kNumLanes> dds_out;
    aie::vector<T_DDS_TYPE, m_kNumLanes> rot_vec; // load rotation values in rot_vec
    aie::vector<TT_DATA, m_kNumLanes> chess_storage(wr0) mixer_vdata;
    T_accVect0 dds_acc;
    T_accVect1 dds_conj_acc;
    T_accVect2 mixer_acc;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);
    set_sat();
    set_rnd(kRoundMode);

    for (unsigned l = 0; l < m_kLoopCount; ++l) chess_prepare_for_pipelining chess_loop_count(m_kLoopCount) {
            dds_out[0] = aie::sincos_complex(m_phaseIndex);
            m_phaseIndex += m_perCyclePhaseInc;

            dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output
            mixer_vdata = T_inIF::template port_readincr<m_kNumLanes>(inInterface.inPortA);
            mixer_acc = aie::mul(mixer_vdata, dds_acc.template to_vector<TT_DATA>(this->m_kDdsShift));
            dds_conj_acc =
                aie::mul(aie::op_conj(rot_vec), aie::op_conj(dds_out[0])); // What?? Why not conjugate dds_acc?
            mixer_vdata = T_inIF::template port_readincr<m_kNumLanes>(inInterface.inPortB);
            mixer_acc = aie::mac(mixer_acc, mixer_vdata, dds_conj_acc.template to_vector<TT_DATA>(this->m_kDdsShift));

            // write to output buffer
            T_outIF::template port_writeincr(outInterface.outPort,
                                             mixer_acc.template to_vector<TT_DATA>(this->m_kMixerShift)); // mac output
        }
};

// Overload for TP_MIXER_MODE = 1  (DDS PLUS 1 data input MIXER MODE)
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
INLINE_DECL void kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_inIF_mm1 inInterface, T_outIF outInterface) {
    // pointer ww to m_phRot array as data type v8cint16
    const aie::vector<T_DDS_TYPE, m_kNumLanes>* ww = (const aie::vector<T_DDS_TYPE, m_kNumLanes>*)m_phRot;

    // dds_out is storage for sine/cos value generated from scalar function sincos, once per cycle
    using T_accint16Vect0 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(bm0);
    using T_accint32Vect0 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(aml0);
    using T_accVect0 =
        typename std::conditional<std::is_same<TT_DATA, cint16>::value, T_accint16Vect0, T_accint32Vect0>::type;
    using T_accint16Vect2 = typename ::aie::accum<cacc48, m_kNumLanes> chess_storage(bm2);
    using T_accint32Vect2 = typename ::aie::accum<cacc80, m_kNumLanes> chess_storage(bm2);
    using T_accVect2 =
        typename std::conditional<std::is_same<TT_DATA, cint16>::value, T_accint16Vect2, T_accint32Vect2>::type;

    aie::vector<T_DDS_TYPE, m_kNumLanes> dds_out;
    aie::vector<T_DDS_TYPE, m_kNumLanes> rot_vec;
    aie::vector<TT_DATA, m_kNumLanes> mixer_vdata;
    T_accVect0 dds_acc; // Note literal cacc48. This specialization is for int types and dds out for int is always
                        // cint16
    T_accVect2 mixer_acc;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);
    set_sat();
    set_rnd(kRoundMode);

    for (unsigned op = 0; op < m_kLoopCount; ++op) chess_prepare_for_pipelining chess_loop_count(m_kLoopCount) {
            dds_out[0] = aie::sincos_complex(m_phaseIndex);
            m_phaseIndex += m_perCyclePhaseInc;

            dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output
            mixer_vdata = T_inIF::template port_readincr<m_kNumLanes>(inInterface.inPort);
            mixer_acc = aie::mul(mixer_vdata, dds_acc.template to_vector<TT_DATA>(m_kDdsShift));

            // write to output buffer
            T_outIF::template port_writeincr(
                outInterface.outPort, mixer_acc.template to_vector<TT_DATA>(
                                          m_kDdsShift)); // shift 15 for one channel to compensate for dds binary point
        }
};

// Overload for TP_MIXER_MODE = 0  (DDS ONLY MODE)
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
INLINE_DECL void kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_outIF outInterface) {
    // m_phRot would ideally be constructor created and constant-persistant after that, hence stored in a register,
    // but kernels lose their registers between calls, so m_phRot has to be retrieved from memory.
    // This indirection of creating a pointer to m_phRot then loading that pointer into rot_vec appears to be
    // means to ensure that m_phRot is in memory (heap).
    const aie::vector<T_DDS_TYPE, m_kNumLanes>* ww = (const aie::vector<T_DDS_TYPE, m_kNumLanes>*)m_phRot;

    // dds_out is storage for sine/cos values generated from scalar function sincos
    aie::vector<T_DDS_TYPE, m_kNumLanes> dds_out;
    aie::vector<T_DDS_TYPE, m_kNumLanes> rot_vec;

    aie::accum<cacc48, m_kNumLanes> dds_acc;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);
    set_sat();
    set_rnd(kRoundMode);

    for (unsigned op = 0; op < m_kLoopCount; ++op) chess_prepare_for_pipelining chess_loop_count(m_kLoopCount) {
            dds_out[0] = aie::sincos_complex(m_phaseIndex);

            m_phaseIndex += m_perCyclePhaseInc;

            // create a per sample dds output using the per cycle dds value
            // and the per sample m_phRot array values
            dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output

            // write to output buffer
            T_outIF::template port_writeincr(outInterface.outPort, dds_acc.template to_vector<TT_DATA>(m_kDdsShift));
        }
};

//-----------------------------------
// cfloat specializations
// Constructor to populate m_phRot array
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
kernelDdsMixerClass<cfloat, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::kernelDdsMixerClass(uint32_t phaseInc) {
    m_phaseIndex = 0;
    cint16 phRotInt16;
    // Calculate the phase increment for each clock cycle ( = per sample inc * input vector size)
    m_perCyclePhaseInc = phaseInc * m_kNumLanes;

    // calculate per-lane offset angle (as Cartesian)
    for (unsigned int i = 0; i < m_kNumLanes; i++) {
        phRotInt16 = aie::sincos_complex(i * phaseInc);
        m_phRot[i].real = ((float)phRotInt16.real) / (float(1 << 15));
        m_phRot[i].imag = ((float)phRotInt16.imag) / (float(1 << 15));
    }
}

// Constructor to populate m_phRot array with an initial offset
// making this overload so that the default case doesn't get extra penalty of additions and sets with trivial value of
// 0.
// This enables SSR DDS
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
kernelDdsMixerClass<cfloat, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::kernelDdsMixerClass(
    uint32_t phaseInc, uint32_t initialPhaseOffset)
    : kernelDdsMixerClass(phaseInc) {
    // initialise phase accumulator index to offset.
    // enhancement? if initialPhaseOffset was a template argument, then we could just set this at initialisation without
    // penalty
    m_phaseIndex = initialPhaseOffset;
}

// DDS_Mixer run-time function
// Overload for TP_MIXER_MODE=2
//----------------------------------------------------------------------------------------------------------------------
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
INLINE_DECL void kernelDdsMixerClass<cfloat, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_inIF_mm2 inInterface, T_outIF outInterface) {
    using TT_DATA = cfloat;
    using T_DDS_TYPE = cfloat;
    // pointer ww to m_phRot array
    const aie::vector<TT_DATA, m_kNumLanes>* ww = (const aie::vector<TT_DATA, m_kNumLanes>*)m_phRot;
    constexpr int kUnrollFactor = 2; // optimizes microcode.
    // AIE API declarations
    // only ever use index 0 dds_out, but phrasing this as a vector seems to help the compiler pipeline a lot better.
    // cint16 ddsOutInt;
    cint16 ddsOutInt;
    cfloat ddsOutFloat;
    aie::vector<TT_DATA, m_kNumLanes>
        dds_out; // phrased as a vector even though it is scalar, since this optimizes far better
    aie::vector<TT_DATA, m_kNumLanes> mixer_vdata;
    aie::vector<TT_DATA, m_kNumLanes> dds_acc;
    aie::vector<TT_DATA, m_kNumLanes> dds_conj_acc;
    aie::vector<TT_DATA, m_kNumLanes> mixer_acc;
    aie::vector<TT_DATA, m_kNumLanes> mixer2;
    aie::vector<TT_DATA, m_kNumLanes> rot_vec; // load rotation values in rot_vec

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);
    set_sat();

    for (unsigned l = 0; l < m_kLoopCount / kUnrollFactor; ++l)
        chess_prepare_for_pipelining chess_loop_count(m_kLoopCount / kUnrollFactor) {
#pragma unroll(kUnrollFactor)
            for (int k = 0; k < kUnrollFactor; k++) {
                ddsOutInt = aie::sincos_complex(m_phaseIndex);
                ddsOutFloat.real = aie::to_float(ddsOutInt.real, 15);
                ddsOutFloat.imag = aie::to_float(ddsOutInt.imag, 15);
                dds_out[0] = ddsOutFloat;

                m_phaseIndex += m_perCyclePhaseInc;

                dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output
                mixer_vdata = T_inIF::template port_readincr<m_kNumLanes>(inInterface.inPortA);
                mixer_acc = aie::mul(mixer_vdata, dds_acc);
                dds_conj_acc = aie::mul(aie::op_conj(rot_vec), aie::op_conj(dds_out[0]));
                mixer_vdata = T_inIF::template port_readincr<m_kNumLanes>(inInterface.inPortB);
                mixer2 = aie::mul(mixer_vdata, dds_conj_acc);
                //    mixer_acc    = aie::mac(mixer_acc, mixer_vdata, dds_conj_acc) ;
                mixer_acc = aie::add(mixer_acc, mixer2);

                // write to output buffer
                T_outIF::template port_writeincr(outInterface.outPort, mixer_acc); // mac output
            }
        }
};

// Overload for TP_MIXER_MODE = 1  (DDS PLUS 1 data input MIXER MODE)
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
INLINE_DECL void kernelDdsMixerClass<cfloat, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_inIF_mm1 inInterface, T_outIF outInterface) {
    using TT_DATA = cfloat;
    using T_DDS_TYPE = cfloat;

    // pointer ww to m_phRot array as data type v8cint16
    const aie::vector<TT_DATA, m_kNumLanes>* ww = (const aie::vector<TT_DATA, m_kNumLanes>*)m_phRot;
    constexpr int kUnrollFactor = 2; // optimizes microcode.

    // dds_out is storage for sine/cos value generated from scalar function sincos, once per cycle
    cint16 ddsOutInt;
    cfloat ddsOutFloat;
    aie::vector<TT_DATA, m_kNumLanes> dds_out;
    aie::vector<TT_DATA, m_kNumLanes> dds_acc;
    aie::vector<TT_DATA, m_kNumLanes> mixer_vdata;
    aie::vector<TT_DATA, m_kNumLanes>
        mixer_acc; // force dds_acc and mixer acc to separate registers to avoid dependency?
    aie::vector<TT_DATA, m_kNumLanes> rot_vec;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);

    set_sat(); // Despite being a float specialization, the DDS output is cint16, so saturation and rounding are
               // relevant.
    set_rnd(kRoundMode);

    for (unsigned l = 0; l < m_kLoopCount / kUnrollFactor; ++l)
        chess_prepare_for_pipelining chess_loop_count(m_kLoopCount / kUnrollFactor) {
#pragma unroll(kUnrollFactor)
            for (int k = 0; k < kUnrollFactor; k++) {
                ddsOutInt = aie::sincos_complex(m_phaseIndex);
                ddsOutFloat.real = aie::to_float(ddsOutInt.real, 15);
                ddsOutFloat.imag = aie::to_float(ddsOutInt.imag, 15);
                dds_out[0] = ddsOutFloat;

                m_phaseIndex += m_perCyclePhaseInc;

                dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output
                mixer_vdata = T_inIF::template port_readincr<m_kNumLanes>(inInterface.inPort);
                mixer_acc = aie::mul(mixer_vdata, dds_acc);

                // write to output buffer
                T_outIF::template port_writeincr(outInterface.outPort, mixer_acc);
            }
        }
};

// Overload for TP_MIXER_MODE = 0  (DDS ONLY MODE)
template <unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
INLINE_DECL void kernelDdsMixerClass<cfloat, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_outIF outInterface) {
    using TT_DATA = cfloat;
    using T_DDS_TYPE = cfloat;
    // m_phRot would ideally be constructor created and constant-persistant after that, hence stored in a register,
    // but kernels lose their registers between calls, so m_phRot has to be retrieved from memory.
    // This indirection of creating a pointer to m_phRot then loading that pointer into rot_vec appears to be
    // means to ensure that m_phRot is in memory (heap).
    const aie::vector<TT_DATA, m_kNumLanes>* ww = (const aie::vector<TT_DATA, m_kNumLanes>*)m_phRot;
    constexpr int kUnrollFactor = 2; // optimizes microcode.
    unsigned int* phaseIndexPtr = (unsigned int*)&m_phaseIndex;

    // dds_out is storage for sine/cos values generated from scalar function sincos
    cint16 ddsOutInt;
    cfloat ddsOutFloat;
    aie::vector<TT_DATA, m_kNumLanes> dds_out;

    using T_accVect = typename ::aie::vector<cfloat, m_kNumLanes>;
    T_accVect dds_acc;

    aie::vector<TT_DATA, m_kNumLanes> rot_vec;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);
    m_phaseIndex = *phaseIndexPtr;
    set_sat(); // Despite being a float specialization, the DDS output is cint16, so saturation and rounding are
               // relevant.
    set_rnd(kRoundMode);

    for (unsigned op = 0; op < m_kLoopCount / kUnrollFactor; ++op)
        chess_prepare_for_pipelining chess_loop_count(m_kLoopCount / kUnrollFactor) {
#pragma unroll(kUnrollFactor)
            for (int k = 0; k < kUnrollFactor; k++) {
                ddsOutInt = aie::sincos_complex(m_phaseIndex);
                ddsOutFloat.real = aie::to_float(ddsOutInt.real, 15);
                ddsOutFloat.imag = aie::to_float(ddsOutInt.imag, 15);
                dds_out[0] = ddsOutFloat;

                m_phaseIndex += m_perCyclePhaseInc;

                // create a per sample dds output using the per cycle dds value
                // and the per sample m_phRot array values
                dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output

                // write to output buffer
                T_outIF::template port_writeincr(outInterface.outPort, dds_acc);
            }
        }
    *phaseIndexPtr = m_phaseIndex; // ensure persistence
};

// Entry level class

// DDS_MIXER function (MIXER_MODE_2)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
NOINLINE_DECL void dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsMix(T_inType* inWindowA,
                                                                                            T_inType* inWindowB,
                                                                                            T_outType* outWindow) {
    T_inIF inInterface(inWindowA, inWindowB);
    T_outIF outInterface(outWindow);
    this->ddsKernel(inInterface, outInterface);
};

// DDS_MIXER function (MIXER_MODE_1)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
NOINLINE_DECL void dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API>::ddsMix(T_inType* inWindowA,
                                                                                           T_outType* outWindow) {
    T_inIF inInterface(inWindowA);
    T_outIF outInterface(outWindow);
    this->ddsKernel(inInterface, outInterface);
};

// DDS_MIXER function (MIXER_MODE_0)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
NOINLINE_DECL void dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API>::ddsMix(T_outType* outWindow) {
    T_outIF outInterface(outWindow);
    this->ddsKernel(outInterface);
};
}
}
}
}
}
