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
#ifndef _DSPLIB_FIR_TDM_UTILS_HPP_
#define _DSPLIB_FIR_TDM_UTILS_HPP_

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "fir_tdm.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace tdm {

template <typename TT_DATA, typename TT_COEFF>
struct tTDMAccBaseType {
    using type = typename tAccBaseType<TT_DATA, TT_COEFF>::type;
};

#if __FIR_TDM_USE_48BIT_ACC__ == 1
template <>
struct tTDMAccBaseType<cint16, int32> {
    using type = cacc48;
};
template <>
struct tTDMAccBaseType<cint16, cint32> {
    using type = cacc48;
};
template <>
struct tTDMAccBaseType<int16, int32> {
    using type = acc48;
};
#endif

// Overloaded function to write to cascade output.
template <typename TT_DATA, typename TT_COEFF, unsigned int accSamples, typename TT_OUT_DATA = TT_DATA>
INLINE_DECL void writeCascade(T_outputIF<CASC_OUT_TRUE, TT_OUT_DATA> outInterface,
                              ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc) {
    using accTag = tTDMAccBaseType<TT_DATA, TT_COEFF>::type;
    writeincr<accTag, acc.size()>((output_cascade<accTag>*)outInterface.outCascade, acc);
}

// Overloaded function to read from cascade input.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> readCascade(
    T_inputIF<true, TT_DATA, TP_DUAL_IP> inInterface,
    ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc) {
    using accTag = tTDMAccBaseType<TT_DATA, TT_COEFF>::type;
    return readincr_v<acc.size(), accTag>((input_cascade<accTag>*)inInterface.inCascade);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int samples, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> mulTdm(
    ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc,
    ::aie::vector<TT_DATA, samples> dataVect,
    ::aie::vector<TT_COEFF, samples> coeffVect) {
    return ::aie::mul<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type>(dataVect, coeffVect);
}

template <typename TT_DATA, typename TT_COEFF, unsigned int samples, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> macTdm(
    ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc,
    ::aie::vector<TT_DATA, samples> dataVect,
    ::aie::vector<TT_COEFF, samples> coeffVect) {
    return ::aie::mac(acc, dataVect, coeffVect);
    // return acc;
}

#if __HAS_ACCUM_PERMUTES__ == 1

// AIE1 has funky permutes that allow for more efficient call
template <>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> mulTdm(
    ::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 16> dataVect,
    ::aie::vector<int16, 16> coeffVect) {
    ::aie::vector<cint16, 32> xbuff = ::aie::concat(dataVect, ::aie::zeros<cint16, 16>());
    ::aie::vector<int16, 16> zbuff = coeffVect;
    const unsigned int xstart = 0;
    const unsigned int ystart = 0;
    const unsigned int xoffsets = 0x76543210;
    const unsigned int xstep = 8;
    const unsigned int zstart = 0;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = 8;

    // return mul8(xbuff, xstart, xoffsets, ystart, xstep, zbuff, zstart, zoffsets, zstep);
    return mul8(xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
}

template <>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> macTdm(
    ::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 16> dataVect,
    ::aie::vector<int16, 16> coeffVect) {
    ::aie::vector<cint16, 32> xbuff = ::aie::concat(dataVect, ::aie::zeros<cint16, 16>());
    ::aie::vector<int16, 16> zbuff = coeffVect;
    const unsigned int xstart = 0;
    const unsigned int ystart = 0;
    const unsigned int xoffsets = 0x76543210;
    const unsigned int xstep = 8;
    const unsigned int zstart = 0;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = 8;

    // return mac8(acc, xbuff, xstart, xoffsets, ystart, xstep, zbuff, zstart, zoffsets, zstep);
    return mac8(acc, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
}
#endif

template <typename TT_DATA, typename TT_COEFF, unsigned int samples, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> mulTdm2(
    ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc,
    ::aie::vector<TT_DATA, samples> dataVect,
    ::aie::vector<TT_COEFF, samples> coeffVect) {
    return mulTdm(acc, dataVect, coeffVect);
}

template <typename TT_DATA, typename TT_COEFF, unsigned int samples, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> macTdm2(
    ::aie::accum<typename tTDMAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc,
    ::aie::vector<TT_DATA, samples> dataVect,
    ::aie::vector<TT_COEFF, samples> coeffVect) {
    return macTdm(acc, dataVect, coeffVect);
    // return acc;
}

#if __HAS_ACCUM_PERMUTES__ == 1

// AIE1 has funky permutes that allow for more efficient call
template <>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> mulTdm2(
    ::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 16> dataVect,
    ::aie::vector<int16, 16> coeffVect) {
    ::aie::vector<cint16, 32> xbuff = ::aie::concat(dataVect, ::aie::zeros<cint16, 16>());
    ::aie::vector<int16, 16> zbuff = coeffVect;
    const unsigned int xstart = 8;
    const unsigned int ystart = 0;
    const unsigned int xoffsets = 0x76543210;
    const int xstep = -8;
    const unsigned int zstart = 0;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = 8;

    // return mul8(xbuff, xstart, xoffsets, ystart, xstep, zbuff, zstart, zoffsets, zstep);
    return mul8(xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
}

template <>
INLINE_DECL::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> macTdm2(
    ::aie::accum<typename tTDMAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 16> dataVect,
    ::aie::vector<int16, 16> coeffVect) {
    ::aie::vector<cint16, 32> xbuff = ::aie::concat(dataVect, ::aie::zeros<cint16, 16>());
    ::aie::vector<int16, 16> zbuff = coeffVect;
    const unsigned int xstart = 8;
    const unsigned int ystart = 0;
    const unsigned int xoffsets = 0x76543210;
    const int xstep = -8;
    const unsigned int zstart = 0;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = 8;

    // return mac8(acc, xbuff, xstart, xoffsets, ystart, xstep, zbuff, zstart, zoffsets, zstep);
    return mac8(acc, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
}
#endif
}
}
}
}
}

#endif // _DSPLIB_FIR_TDM_UTILS_HPP_
