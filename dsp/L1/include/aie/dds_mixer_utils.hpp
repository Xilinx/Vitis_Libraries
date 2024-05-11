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
#ifndef _DSPLIB_DDS_MIXER_UTILS_HPP_
#define _DSPLIB_DDS_MIXER_UTILS_HPP_

/*
DDS Mixer Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "dds_mixer.hpp"
#include "aie_api/aie_adf.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace mixer {
namespace dds_mixer {

template <typename TT_DATA, typename PortType>
template <unsigned int VECTOR_LEN>
auto INLINE_DECL T_IFbase<TT_DATA, PortType>::port_readincr(PortType* in) {
    // check which IO API we should use
    if
        constexpr(std::is_same_v<PortType, input_window<TT_DATA> >) { // todo - check if we should use is_convertable or
                                                                      // layout_compatible to handle inout_window or
                                                                      // something.
            return window_readincr_v<VECTOR_LEN>(in);
        }
    else if
        constexpr(std::is_same_v<PortType, input_stream<TT_DATA> >) {
            // stream
            return readincr_v<VECTOR_LEN>(in);
        }
}

// this should also work for cascade
template <typename TT_DATA, typename PortType>
template <typename OutDType>
void INLINE_DECL T_IFbase<TT_DATA, PortType>::port_writeincr(PortType* out, OutDType data) {
    // check which IO API we should use
    if
        constexpr(std::is_same_v<PortType, output_window<TT_DATA> >) { // todo - check if we should use is_convertable
                                                                       // or layout_compatible to handle inout_window or
                                                                       // something.
            return window_writeincr(out, data);
        }
    else if
        constexpr(std::is_same_v<PortType, output_stream<TT_DATA> >) {
            // stream
            return writeincr(out, data);
        }
}

template <unsigned int numTables, unsigned int tableRes, unsigned int lutMask, unsigned int numLanes, typename TT_DATA>
auto INLINE_DECL computeDDSOut(unsigned int m_phaseIndex, cint32** sincosLUT) {
    ::aie::accum<cacc64, 4> ddsOutLUT;
    ::aie::vector<cint32, 4> sincosVal[numTables];
#pragma unroll numTables
    for (int i = 0; i < numTables; i++) {
        sincosVal[i][0] = sincosLUT[i][(m_phaseIndex >> (32 - 10 * (i + 1))) & 0x000003FF];
    }
    if
        constexpr(numTables == 1) {
            ddsOutLUT.from_vector(sincosVal[0], 0);
            return ddsOutLUT;
        }
    else if
        constexpr(numTables == 2) { return ::aie::mul(sincosVal[0], sincosVal[1]); }
    else if
        constexpr(numTables == 3) {
            ::aie::accum<cacc64, 4> ddsAccInter;
            ddsAccInter = ::aie::mul(sincosVal[0], sincosVal[1]);
            ddsOutLUT = ::aie::mul(sincosVal[2], ddsAccInter.template to_vector<TT_DATA>(31));
            return ddsOutLUT;
        }
};

#
template <unsigned int numTables, unsigned int tableRes, unsigned int lutMask, unsigned int numLanes, typename TT_DATA>
auto INLINE_DECL computeDDSOutFloat(unsigned int m_phaseIndex, cfloat** sincosLUT) {
#if __SUPPORTS_CFLOAT__ == 1
    ::aie::vector<cfloat, 4> ddsOutLUT;
    ::aie::vector<cfloat, 4> sincosVal[numTables];
#pragma unroll numTables
    for (int i = 0; i < numTables; i++) {
        sincosVal[i][0] = sincosLUT[i][(m_phaseIndex >> (32 - 10 * (i + 1))) & 0x000003FF];
    }
    if
        constexpr(numTables == 1) { return sincosVal[0].get(0); }
    else if
        constexpr(numTables == 2) {
            ddsOutLUT = ::aie::mul(sincosVal[0], sincosVal[1]);
            return ddsOutLUT.get(0);
        }
    else if
        constexpr(numTables == 3) {
            ::aie::vector<cfloat, 4> ddsAccInter;
            ddsAccInter = ::aie::mul(sincosVal[0], sincosVal[1]);
            ddsOutLUT = ::aie::mul(sincosVal[2], ddsAccInter);
            return ddsOutLUT.get(0);
        }
#endif
};
}
}
}
}
}

#endif // _DSPLIB_DDS_MIXER_UTILS_HPP_
