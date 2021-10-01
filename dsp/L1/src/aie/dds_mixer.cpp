/*
 * Copyright 2021 Xilinx, Inc.
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

#define __NEW_WINDOW_H__ 1
#define __AIEARCH__ 1
#define __AIENGINE__ 1
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

// Constructor to populate phRot array
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
// __attribute__((noinline))

kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::kernelDdsMixerClass(uint32_t phaseInc) {
    // Calculate the phase increment for each clock cycle ( = per sample inc * input vector size)
    perCyclePhaseInc = phaseInc * numLanes;

    // Add phRot constructor

    for (unsigned int i = 0; i < numLanes; i++) {
        phRot[i] = aie::sincos_complex(i * phaseInc);
    }
}

// DDS_Mixer run-time function
// The input is a window.
// The output interface is a window
// dds_mixer function
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
inline void kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_inIF_mm2 inInterface, T_outIF outInterface) {
    // ddsKernel_<TT_DATA>(inInterface.inPortA, inInterface.inPortB, outInterface.outPort);

    // pointer ww to phRot array
    const aie::vector<TT_DATA, numLanes>* ww = (const aie::vector<TT_DATA, numLanes>*)phRot;

    // AIE API declarations
    // only ever use index 0 dds_out, but phrasing this as a vector seems to help the compiler pipeline a lot better.
    aie::vector<TT_DATA, numLanes> dds_out;
    aie::vector<TT_DATA, numLanes> chess_storage(wr0) mixer_vdata;
    aie::accum<T_acc, numLanes> chess_storage(bm0) dds_acc;
    aie::accum<T_acc, numLanes> chess_storage(bm1) dds_conj_acc;
    aie::accum<T_acc, numLanes> chess_storage(bm2) mixer_acc;
    aie::vector<TT_DATA, numLanes> rot_vec; // load rotation values in rot_vec

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);

    set_sat();

    for (unsigned l = 0; l < loopCount; ++l) chess_prepare_for_pipelining chess_loop_count(loopCount) {
            dds_out[0] = aie::sincos_complex(phaseIndex);
            phaseIndex += perCyclePhaseInc;

            dds_acc = aie::mul(rot_vec, dds_out[0]); // per sample dds output

            // multiply sine/cos vector to input channel 1
            mixer_vdata = T_inIF::template port_readincr<numLanes>(inInterface.inPortA);

            mixer_acc = aie::mul(mixer_vdata, dds_acc.template to_vector<TT_DATA>(this->dds_shift));

            // Add 2nd data input functionality
            dds_conj_acc = aie::mul(aie::op_conj(rot_vec), aie::op_conj(dds_out[0]));

            mixer_vdata = T_inIF::template port_readincr<numLanes>(inInterface.inPortB);

            mixer_acc = aie::mac(mixer_acc, mixer_vdata, dds_conj_acc.template to_vector<TT_DATA>(this->dds_shift));

            // write to output buffer
            T_outIF::template port_writeincr(outInterface.outPort,
                                             mixer_acc.template to_vector<TT_DATA>(this->mixer_shift)); // mac output
        }
};

//===========================================================
// OVERLOAD for mixer_mode = 1  (DDS PLUS 1 data input MIXER MODE)
//===============

// NOTE:  The variables specified in the .hpp constructor are not accessible using there name
// and need to be prefixed with this-> (for pointers) or this. (for others)

// DDS_Mixer run-time function
// The input is a window.
// The output interface is a window
// dds_mixer function
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
inline void kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_inIF_mm1 inInterface, T_outIF outInterface)

{
    // pointer ww to phRot array as data type v8cint16
    const aie::vector<TT_DATA, numLanes>* ww = (const aie::vector<TT_DATA, numLanes>*)phRot;

    // dds_out is storage for sine/cos value generated from scalar function sincos, once per cycle
    TT_DATA dds_out;

    aie::vector<TT_DATA, numLanes> mixer_vdata;
    aie::accum<T_acc, numLanes> chess_storage(bm0) dds_acc;
    aie::accum<T_acc, numLanes> chess_storage(bm2) mixer_acc;

    aie::vector<TT_DATA, numLanes> rot_vec;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);

    set_sat();

    for (unsigned l = 0; l < loopCount; ++l) chess_prepare_for_pipelining chess_loop_count(loopCount) {
            dds_out = aie::sincos_complex(phaseIndex);
            phaseIndex += perCyclePhaseInc;

            dds_acc = aie::mul(rot_vec, dds_out); // per sample dds output

            // multiply sine/cos vector to input channel 1
            // increment input window for next sample
            mixer_vdata = T_inIF::template port_readincr<numLanes>(inInterface.inPort);

            mixer_acc = aie::mul(mixer_vdata, dds_acc.template to_vector<TT_DATA>(dds_shift));

            // write to output buffer
            T_outIF::template port_writeincr(outInterface.outPort, mixer_acc.template to_vector<TT_DATA>(mixer_shift));
        }
};

//===========================================================
// OVERLOAD for mixer_mode = 0  (DDS ONLY MODE)
//===============

// DDS_Mixer run-time function
// dds_mixer function
//----------------------------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
inline void kernelDdsMixerClass<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsKernel(
    T_outIF outInterface) {
    // pointer ww to phRot array as data type v8cint16
    const aie::vector<TT_DATA, numLanes>* ww = (const aie::vector<TT_DATA, numLanes>*)phRot;

    // dds_out is storage for sine/cos values generated from scalar function sincos
    TT_DATA dds_out;

    aie::accum<T_acc, numLanes> chess_storage(bm0) dds_acc;

    aie::vector<TT_DATA, numLanes> rot_vec;

    // load rotation values in rot_vec
    rot_vec = *(ww + 0);

    set_sat();

    for (unsigned l = 0; l < loopCount; ++l) chess_prepare_for_pipelining chess_loop_count(loopCount) {
            dds_out = aie::sincos_complex(phaseIndex);
            phaseIndex += perCyclePhaseInc;

            // create a per sample dds output using the per cycle dds value
            // and the per sample phRot array values
            dds_acc = aie::mul(rot_vec, dds_out); // per sample dds output

            // write to output buffer
            T_outIF::template port_writeincr(outInterface.outPort, dds_acc.template to_vector<TT_DATA>(dds_shift));
        }
};

// DDS_MIXER function (MIXER_MODE_2)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_MIXER_MODE, unsigned int TP_API>
__attribute__((noinline)) void dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, TP_MIXER_MODE, TP_API>::ddsMix(
    T_inType* inWindowA, T_inType* inWindowB, T_outType* outWindow) {
    T_inIF inInterface(inWindowA, inWindowB);
    T_outIF outInterface(outWindow);
    this->ddsKernel(inInterface, outInterface);
};

// DDS_MIXER function (MIXER_MODE_1)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
__attribute__((noinline)) void dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_1, TP_API>::ddsMix(
    T_inType* inWindowA, T_outType* outWindow) {
    T_inIF inInterface(inWindowA);
    T_outIF outInterface(outWindow);
    this->ddsKernel(inInterface, outInterface);
};
// DDS_MIXER function (MIXER_MODE_0)
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int TP_INPUT_WINDOW_VSIZE, unsigned int TP_API>
__attribute__((noinline)) void dds_mixer<TT_DATA, TP_INPUT_WINDOW_VSIZE, MIXER_MODE_0, TP_API>::ddsMix(
    T_outType* outWindow) {
    T_outIF outInterface(outWindow);
    this->ddsKernel(outInterface);
};
}
}
}
}
}
