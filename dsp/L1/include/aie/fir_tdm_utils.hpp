/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

// LONG ACCs

template <typename TT_DATA, typename TT_COEFF, unsigned int samples, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> mulTdm(
    ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc,
    ::aie::vector<TT_DATA, samples> dataVect,
    ::aie::vector<TT_COEFF, samples> coeffVect) {
    return ::aie::mul(dataVect, coeffVect);
}

template <typename TT_DATA, typename TT_COEFF, unsigned int samples, unsigned int accSamples>
INLINE_DECL::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> macTdm(
    ::aie::accum<typename tAccBaseType<TT_DATA, TT_COEFF>::type, accSamples> acc,
    ::aie::vector<TT_DATA, samples> dataVect,
    ::aie::vector<TT_COEFF, samples> coeffVect) {
    return ::aie::mac(acc, dataVect, coeffVect);
    // return acc;
}

template <>
INLINE_DECL::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> mulTdm(
    ::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 8> dataVect,
    ::aie::vector<int16, 8> coeffVect) {
    ::aie::vector<cint16, 16> xbuff = ::aie::concat(dataVect, ::aie::zeros<cint16, 8>());
    ::aie::vector<int16, 16> zbuff = ::aie::concat(coeffVect, ::aie::zeros<int16, 8>());
    int xstart, ystart = 0;
    unsigned int xoffsets = 0x76543210;
    int mtap = 0;
    unsigned int zstart = 0;
    unsigned int zoffsets = 0x76543210;
    unsigned int zstep = 0;

    return mul8_antisym_ct(xbuff, xstart, xoffsets, ystart, mtap, zbuff, zstart, zoffsets, zstep);
    // return ::aie::mul(dataVect, coeffVect);
}

template <>
INLINE_DECL::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> macTdm(
    ::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 8> dataVect,
    ::aie::vector<int16, 8> coeffVect) {
    // return ::aie::mac(acc, dataVect, coeffVect);
    ::aie::vector<cint16, 16> xbuff = ::aie::concat(dataVect, ::aie::zeros<cint16, 8>());
    ::aie::vector<int16, 16> zbuff = ::aie::concat(coeffVect, ::aie::zeros<int16, 8>());
    int xstart, ystart = 0;
    unsigned int xoffsets = 0x76543210;
    int mtap = 0;
    unsigned int zstart = 0;
    unsigned int zoffsets = 0x76543210;
    unsigned int zstep = 0;

    return mac8_antisym_ct(acc, xbuff, xstart, xoffsets, ystart, mtap, zbuff, zstart, zoffsets, zstep);
}

template <>
INLINE_DECL::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> mulTdm(
    ::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 16> dataVect,
    ::aie::vector<int16, 16> coeffVect) {
    ::aie::vector<cint16, 16> xbuff = dataVect;
    ::aie::vector<int16, 16> zbuff = coeffVect;
    unsigned int xstart, ystart = 0;
    unsigned int xoffsets = 0x76543210;
    unsigned int xstep = 8;
    unsigned int zstart = 0;
    unsigned int zoffsets = 0x76543210;
    unsigned int zstep = 8;

    return mul8(xbuff, xstart, xoffsets, ystart, xstep, zbuff, zstart, zoffsets, zstep);
    // return ::aie::mul(dataVect, coeffVect);
}

template <>
INLINE_DECL::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> macTdm(
    ::aie::accum<typename tAccBaseType<cint16, int16>::type, 8> acc,
    ::aie::vector<cint16, 16> dataVect,
    ::aie::vector<int16, 16> coeffVect) {
    ::aie::vector<cint16, 16> xbuff = dataVect;
    ::aie::vector<int16, 16> zbuff = coeffVect;
    unsigned int xstart, ystart = 0;
    unsigned int xoffsets = 0x76543210;
    unsigned int xstep = 8;
    unsigned int zstart = 0;
    unsigned int zoffsets = 0x76543210;
    unsigned int zstep = 8;

    return mac8(acc, xbuff, xstart, xoffsets, ystart, xstep, zbuff, zstart, zoffsets, zstep);
}

// // Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
// template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
// INLINE_DECL T_acc<TT_DATA, TT_COEFF> initMacTdm(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
//                                                    T_acc<TT_DATA, TT_COEFF> acc,
//                                                    T_buff_1024b<TT_DATA> xbuff,
//                                                    unsigned int xstart,
//                                                    T_buff_256b<TT_COEFF> zbuff,
//                                                    unsigned int zstart) {
//     return mulTdm<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
// };
// template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
// INLINE_DECL T_acc<TT_DATA, TT_COEFF> initMacTdm(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
//                                                    T_acc<TT_DATA, TT_COEFF> acc,
//                                                    T_buff_1024b<TT_DATA> xbuff,
//                                                    unsigned int xstart,
//                                                    T_buff_256b<TT_COEFF> zbuff,
//                                                    unsigned int zstart) {
//     return macTdm<TT_DATA, TT_COEFF>(acc, xbuff, xstart, zbuff, zstart);
// };
}
}
}
}
}

#endif // _DSPLIB_FIR_TDM_UTILS_HPP_
