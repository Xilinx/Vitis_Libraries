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
#ifndef _DSPLIB_FIR_DECIMATE_HB_UTILS_HPP_
#define _DSPLIB_FIR_DECIMATE_HB_UTILS_HPP_

/*
Half band decimation FIR Utilities.
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

/*
Note regarding Input Vector register size selection.
The type of the data register is not a constant 1024b or 512b (1buff or 2 buff architecture)
because for 2 integer types the number of samples required for an operation exceeds the capacity
of the 512b buffer used by the symmetrical mul/mac commands. The workaround is to use 2 standard mul/macs
using a set of 2 x 1024b.
For float types, there are no mul/macs with preadd (symmetrical) so the same approach as above is used.
*/

#include <stdio.h>
#include <adf.h>
#include "fir_decimate_hb.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_hb {
//---------------------------------------------------------------------------------------------------
// Type definitions

// Input Vector Register type

// 1 buff arch always use a 1024-bit
template <typename T_D, typename T_C, eArchType TP_ARCH>
struct T_buff_FirDecHb : T_buff_1024b<T_D> {
    using T_buff_1024b<T_D>::operator=;
};

// 2 buff arch aims to use 2 x 512-bit, but some data type combos require 2 x 1024-bit to store enough data for
// decimation.
// This degrades performance, but no alternative is available. See note above.
template <typename T_D, typename T_C>
struct T_buff_FirDecHb<T_D, T_C, kArch2Buff> : T_buff_512b<T_D> {
    using T_buff_512b<T_D>::operator=;
};
template <>
struct T_buff_FirDecHb<int32, int16, kArch2Buff> : T_buff_1024b<int32> {
    using T_buff_1024b<int32>::operator=;
}; // See note regarding Input Vector register size selection.
template <>
struct T_buff_FirDecHb<cint32, int16, kArch2Buff> : T_buff_1024b<cint32> {
    using T_buff_1024b<cint32>::operator=;
}; // See note regarding Input Vector register size selection.
template <>
struct T_buff_FirDecHb<float, float, kArch2Buff> : T_buff_1024b<float> {
    using T_buff_1024b<float>::operator=;
}; // See note regarding Input Vector register size selection.
template <>
struct T_buff_FirDecHb<cfloat, float, kArch2Buff> : T_buff_1024b<cfloat> {
    using T_buff_1024b<cfloat>::operator=;
}; // See note regarding Input Vector register size selection.
template <>
struct T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> : T_buff_1024b<cfloat> {
    using T_buff_1024b<cfloat>::operator=;
}; // See note regarding Input Vector register size selection.

template <typename T_D, typename T_C, eArchType TP_ARCH>
struct T_accFirDecHb : T_acc384<T_D, T_C> {
    using T_acc384<T_D, T_C>::operator=;
};

template <typename T_D, typename T_C, eArchType TP_ARCH>
struct T_outValFiRDecHb : T_outVal384<T_D, T_C> {
    using T_outVal384<T_D, T_C>::operator=;
};

//---------------------------------------------------------------------------------------------------
// Functions
template <typename T_D, typename T_C, eArchType TP_ARCH, unsigned int T_SIZE>
INLINE_DECL void fnLoadXIpData(T_buff_FirDecHb<T_D, T_C, TP_ARCH>& buff,
                               unsigned int splice,
                               input_window<T_D>* inWindow) {
    const short kSpliceRange = T_SIZE == 256 ? 4 : 8;
    buff.val.insert(splice % kSpliceRange, window_readincr_v<T_SIZE / 8 / sizeof(T_D)>(inWindow));
};

// Function to load reverse direction data
template <typename T_D, typename T_C, eArchType TP_ARCH, unsigned int T_SIZE>
INLINE_DECL void fnLoadYIpData(T_buff_FirDecHb<T_D, T_C, TP_ARCH>& buff,
                               unsigned int splice,
                               input_window<T_D>* inWindow) {
    const short kSpliceRange = T_SIZE == 256 ? 4 : 8;
    buff.val.insert(splice % kSpliceRange, window_readdecr_v<T_SIZE / 8 / sizeof(T_D)>(inWindow));
};

// Symmetric mul/mac for use in halfband decimator (fixed decimation factor of 2).
// Note that these functions are grouped by combination of type rather than by function as there
// is more commonality between functions for a given type then between same functions for different types.
//-----------------------------------------------------------------------------------------------------

template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMulSym(T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                            unsigned int xstart,
                                                            T_buff_FirDecHb<T_D, T_C, TP_ARCH> ybuff,
                                                            unsigned int ystart,
                                                            T_buff_256b<T_C> zbuff,
                                                            unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMulSym1buff(T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                                 unsigned int xstart,
                                                                 unsigned int ystart,
                                                                 T_buff_256b<T_C> zbuff,
                                                                 unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMulSymCt(T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                              unsigned int xstart,
                                                              T_buff_FirDecHb<T_D, T_C, TP_ARCH> ybuff,
                                                              unsigned int ystart,
                                                              unsigned int ct,
                                                              T_buff_256b<T_C> zbuff,
                                                              unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMulSymCt1buff(T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                                   unsigned int xstart,
                                                                   unsigned int ystart,
                                                                   unsigned int ct,
                                                                   T_buff_256b<T_C> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMacSym(T_accFirDecHb<T_D, T_C, TP_ARCH> acc,
                                                            T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                            unsigned int xstart,
                                                            T_buff_FirDecHb<T_D, T_C, TP_ARCH> ybuff,
                                                            unsigned int ystart,
                                                            T_buff_256b<T_C> zbuff,
                                                            unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMacSym1buff(T_accFirDecHb<T_D, T_C, TP_ARCH> acc,
                                                                 T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                                 unsigned int xstart,
                                                                 unsigned int ystart,
                                                                 T_buff_256b<T_C> zbuff,
                                                                 unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMacSymCt(T_accFirDecHb<T_D, T_C, TP_ARCH> acc,
                                                              T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                              unsigned int xstart,
                                                              T_buff_FirDecHb<T_D, T_C, TP_ARCH> ybuff,
                                                              unsigned int ystart,
                                                              unsigned int ct,
                                                              T_buff_256b<T_C> zbuff,
                                                              unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};
template <typename T_D, typename T_C, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<T_D, T_C, TP_ARCH> firDecHbMacSymCt1buff(T_accFirDecHb<T_D, T_C, TP_ARCH> acc,
                                                                   T_buff_FirDecHb<T_D, T_C, TP_ARCH> xbuff,
                                                                   unsigned int xstart,
                                                                   unsigned int ystart,
                                                                   unsigned int ct,
                                                                   T_buff_256b<T_C> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<T_D, T_C, TP_ARCH> retVal;
    return retVal; // mute warning
};

// DATA = cint16,  COEFF = int16>
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch2Buff> firDecHbMulSym<cint16, int16, kArch2Buff>(
    T_buff_FirDecHb<cint16, int16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint16, int16, kArch2Buff> ybuff,
    unsigned int ystart,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym(xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch1Buff> firDecHbMulSym1buff<cint16, int16, kArch1Buff>(
    T_buff_FirDecHb<cint16, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym(xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch2Buff> firDecHbMulSymCt<cint16, int16, kArch2Buff>(
    T_buff_FirDecHb<cint16, int16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint16, int16, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        mul4_sym_ct(xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch1Buff> firDecHbMulSymCt1buff<cint16, int16, kArch1Buff>(
    T_buff_FirDecHb<cint16, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym_ct(xbuff.val, xstart, xoffsets, xstep, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch2Buff> firDecHbMacSym<cint16, int16, kArch2Buff>(
    T_accFirDecHb<cint16, int16, kArch2Buff> acc,
    T_buff_FirDecHb<cint16, int16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint16, int16, kArch2Buff> ybuff,
    unsigned int ystart,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        mac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch1Buff> firDecHbMacSym1buff<cint16, int16, kArch1Buff>(
    T_accFirDecHb<cint16, int16, kArch1Buff> acc,
    T_buff_FirDecHb<cint16, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch2Buff> firDecHbMacSymCt<cint16, int16, kArch2Buff>(
    T_accFirDecHb<cint16, int16, kArch2Buff> acc,
    T_buff_FirDecHb<cint16, int16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint16, int16, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, ct, zbuff.val, zstart,
                             zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, int16, kArch1Buff> firDecHbMacSymCt1buff<cint16, int16, kArch1Buff>(
    T_accFirDecHb<cint16, int16, kArch1Buff> acc,
    T_buff_FirDecHb<cint16, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        mac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16,  COEFF = cint16>
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cint16, cint16, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cint16, cint16, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cint16> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym(xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cint16, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym(xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch2Buff> firDecHbMulSymCt(
    T_buff_FirDecHb<cint16, cint16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint16, cint16, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym_ct(xbuff.val, xstart, xoffsets, ybuff.val, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cint16, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mul4_sym_ct(xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cint16, cint16, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<cint16, cint16, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cint16, cint16, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cint16> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        mac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cint16, cint16, kArch1Buff> acc,
    T_buff_FirDecHb<cint16, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch2Buff> firDecHbMacSymCt(
    T_accFirDecHb<cint16, cint16, kArch2Buff> acc,
    T_buff_FirDecHb<cint16, cint16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint16, cint16, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        mac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint16, cint16, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cint16, cint16, kArch1Buff> acc,
    T_buff_FirDecHb<cint16, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint16, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = mac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = int32,  COEFF = int16>
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<int32, int16, kArch2Buff> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<int32, int16, kArch2Buff> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<int16> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    retVal.val = lmul8(xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac8(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, zbuff.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch1Buff> firDecHbMulSym1buff(T_buff_FirDecHb<int32, int16, kArch1Buff> xbuff,
                                                                        unsigned int xstart,
                                                                        unsigned int ystart,
                                                                        T_buff_256b<int16> zbuff,
                                                                        unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul8_sym(xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch2Buff> firDecHbMulSymCt(T_buff_FirDecHb<int32, int16, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<int32, int16, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     unsigned int ct,
                                                                     T_buff_256b<int16> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    T_buff_256b<int16> zbuff2; //= zbuff;
    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    zbuff2.val = upd_elem(zbuff.val, zstart - 1, 0); // zero the centre tap

    // These two commands spoof a lmul_sym_ct command which cannot be used directly because it uses 512b buffers
    // which cannot accommodate the range of data required. Hence it is split into an asym mul (including the centre
    // tap) and an asym mul which must not include the center tap. The center tap is zeroed directly.
    retVal.val = lmul8(xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac8(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, //-1? y has to work backwards.
                       zbuff.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<int32, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 1; // exception to decimation factor of 2 since this is the centre tap term
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul8_sym_ct(xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch2Buff> firDecHbMacSym(T_accFirDecHb<int32, int16, kArch2Buff> acc,
                                                                   T_buff_FirDecHb<int32, int16, kArch2Buff> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<int32, int16, kArch2Buff> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<int16> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2; // decimation factor
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2; // does not apply to z because coeffs are condensed in the constructor
    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac8(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, zbuff.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch1Buff> firDecHbMacSym1buff(T_accFirDecHb<int32, int16, kArch1Buff> acc,
                                                                        T_buff_FirDecHb<int32, int16, kArch1Buff> xbuff,
                                                                        unsigned int xstart,
                                                                        unsigned int ystart,
                                                                        T_buff_256b<int16> zbuff,
                                                                        unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac8_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch2Buff> firDecHbMacSymCt(T_accFirDecHb<int32, int16, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<int32, int16, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<int32, int16, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     unsigned int ct,
                                                                     T_buff_256b<int16> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 1; // exception to decimation factor, because the centre tap is in 2nd column
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    T_buff_256b<int16> zbuff2; //= zbuff;
    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    zbuff2.val = upd_elem(zbuff.val, zstartmod, 0); // zero the centre tap. 16 is z vsize

    // These two commands spoof a lmul_sym_ct command which cannot be used directly because it uses 512b buffers
    // which cannot accommodate the range of data required. Hence it is split into an asym mul (including the centre
    // tap) and an asym mul which must not include the center tap. The center tap is zeroed directly.
    retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac8(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, zbuff2.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int16, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<int32, int16, kArch1Buff> acc,
    T_buff_FirDecHb<int32, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<int32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac8_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = int32,  COEFF = int32>
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<int32, int32, kArch2Buff> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<int32, int32, kArch2Buff> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<int32> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch1Buff> firDecHbMulSym1buff(T_buff_FirDecHb<int32, int32, kArch1Buff> xbuff,
                                                                        unsigned int xstart,
                                                                        unsigned int ystart,
                                                                        T_buff_256b<int32> zbuff,
                                                                        unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch2Buff> firDecHbMulSymCt(T_buff_FirDecHb<int32, int32, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<int32, int32, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     unsigned int ct,
                                                                     T_buff_256b<int32> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym_ct(xbuff.val, xstart, xoffsets, ybuff.val, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<int32, int32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym_ct(xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch2Buff> firDecHbMacSym(T_accFirDecHb<int32, int32, kArch2Buff> acc,
                                                                   T_buff_FirDecHb<int32, int32, kArch2Buff> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<int32, int32, kArch2Buff> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<int32> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ybuff.val, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch1Buff> firDecHbMacSym1buff(T_accFirDecHb<int32, int32, kArch1Buff> acc,
                                                                        T_buff_FirDecHb<int32, int32, kArch1Buff> xbuff,
                                                                        unsigned int xstart,
                                                                        unsigned int ystart,
                                                                        T_buff_256b<int32> zbuff,
                                                                        unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch2Buff> firDecHbMacSymCt(T_accFirDecHb<int32, int32, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<int32, int32, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<int32, int32, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     unsigned int ct,
                                                                     T_buff_256b<int32> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val =
        lmac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<int32, int32, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<int32, int32, kArch1Buff> acc,
    T_buff_FirDecHb<int32, int32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<int32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32,  COEFF = int16>
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cint32, int16, kArch2Buff> xbuff,
                                                                    unsigned int xstart,
                                                                    T_buff_FirDecHb<cint32, int16, kArch2Buff> ybuff,
                                                                    unsigned int ystart,
                                                                    T_buff_256b<int16> zbuff,
                                                                    unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    retVal.val = lmul4(xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac4(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, zbuff.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cint32, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch2Buff> firDecHbMulSymCt(T_buff_FirDecHb<cint32, int16, kArch2Buff> xbuff,
                                                                      unsigned int xstart,
                                                                      T_buff_FirDecHb<cint32, int16, kArch2Buff> ybuff,
                                                                      unsigned int ystart,
                                                                      unsigned int ct,
                                                                      T_buff_256b<int16> zbuff,
                                                                      unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 1; // exception to decimation factor of 2 because this is the centre tap term
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    T_buff_256b<int16> zbuff2; //= zbuff;
    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    zbuff2.val = upd_elem(zbuff.val, zstart - 1, 0); // zero the centre tap

    // These two commands spoof a lmul_sym_ct command which cannot be used directly because it uses 512b buffers
    // which cannot accommodate the range of data required. Hence it is split into an asym mul (including the centre
    // tap) and an asym mul which must not include the center tap. The center tap is zeroed directly.
    retVal.val = lmul4(xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac4(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, //-1? y has to work backwards.
                       zbuff.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cint32, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym_ct(xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cint32, int16, kArch2Buff> acc,
                                                                    T_buff_FirDecHb<cint32, int16, kArch2Buff> xbuff,
                                                                    unsigned int xstart,
                                                                    T_buff_FirDecHb<cint32, int16, kArch2Buff> ybuff,
                                                                    unsigned int ystart,
                                                                    T_buff_256b<int16> zbuff,
                                                                    unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac4(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, zbuff.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cint32, int16, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, xstep, ystart, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch2Buff> firDecHbMacSymCt(T_accFirDecHb<cint32, int16, kArch2Buff> acc,
                                                                      T_buff_FirDecHb<cint32, int16, kArch2Buff> xbuff,
                                                                      unsigned int xstart,
                                                                      T_buff_FirDecHb<cint32, int16, kArch2Buff> ybuff,
                                                                      unsigned int ystart,
                                                                      unsigned int ct,
                                                                      T_buff_256b<int16> zbuff,
                                                                      unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kCols = 2;
    const unsigned int kDecFactor = 2;

    T_buff_256b<int16> zbuff2; //= zbuff;
    unsigned int ystartmod = ystart - (kCols - 1) * xstep;
    unsigned int zstartmod = zstart + (kCols - 1);
    int zstepmod = -(int)zstep;

    zbuff2.val = upd_elem(zbuff.val, zstartmod, 0); // zero the centre tap. 16 is z vsize

    // These two commands spoof a lmul_sym_ct command which cannot be used directly because it uses 512b buffers
    // which cannot accommodate the range of data required. Hence it is split into an asym mul (including the centre
    // tap) and an asym mul which must not include the center tap. The center tap is zeroed directly.
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    retVal.val = lmac4(retVal.val, ybuff.val, ystartmod, xoffsets, xstep, zbuff2.val, zstartmod, zoffsets, zstepmod);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int16, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cint32, int16, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, int16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym_ct(acc.val, xbuff.val, xstart, xoffsets, ystart, ct, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32,  COEFF = cint16>
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cint32, cint16, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cint32, cint16, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cint16> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cint32, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch2Buff> firDecHbMulSymCt(
    T_buff_FirDecHb<cint32, cint16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint32, cint16, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch2Buff> retVal;
    // This would only be called for a fir of length 1
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cint32, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch1Buff> retVal;
    // This would only be called for a fir of length 1
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cint32, cint16, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<cint32, cint16, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cint32, cint16, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cint16> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cint32, cint16, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch2Buff> firDecHbMacSymCt(
    T_accFirDecHb<cint32, cint16, kArch2Buff> acc,
    T_buff_FirDecHb<cint32, cint16, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint32, cint16, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets,
                       //         ybuff, ystart,                   ct,
                       zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint16, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cint32, cint16, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, cint16, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint16> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint16, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // with one column, the centre tap is just a mac
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets,
                       // ystart,                   ct,
                       zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32,  COEFF = int32>
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cint32, int32, kArch2Buff> xbuff,
                                                                    unsigned int xstart,
                                                                    T_buff_FirDecHb<cint32, int32, kArch2Buff> ybuff,
                                                                    unsigned int ystart,
                                                                    T_buff_256b<int32> zbuff,
                                                                    unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cint32, int32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<int32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul4_sym(xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch2Buff> firDecHbMulSymCt(T_buff_FirDecHb<cint32, int32, kArch2Buff> xbuff,
                                                                      unsigned int xstart,
                                                                      T_buff_FirDecHb<cint32, int32, kArch2Buff> ybuff,
                                                                      unsigned int ystart,
                                                                      unsigned int ct,
                                                                      T_buff_256b<int32> zbuff,
                                                                      unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch2Buff> retVal;
    // this would only be used for a fir of length 1!
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cint32, int32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch1Buff> retVal;
    // this would only be used for a fir of length 1!
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cint32, int32, kArch2Buff> acc,
                                                                    T_buff_FirDecHb<cint32, int32, kArch2Buff> xbuff,
                                                                    unsigned int xstart,
                                                                    T_buff_FirDecHb<cint32, int32, kArch2Buff> ybuff,
                                                                    unsigned int ystart,
                                                                    T_buff_256b<int32> zbuff,
                                                                    unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cint32, int32, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, int32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<int32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4_sym(acc.val, xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch2Buff> firDecHbMacSymCt(T_accFirDecHb<cint32, int32, kArch2Buff> acc,
                                                                      T_buff_FirDecHb<cint32, int32, kArch2Buff> xbuff,
                                                                      unsigned int xstart,
                                                                      T_buff_FirDecHb<cint32, int32, kArch2Buff> ybuff,
                                                                      unsigned int ystart,
                                                                      unsigned int ct,
                                                                      T_buff_256b<int32> zbuff,
                                                                      unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets,
                       // ybuff, ystart,
                       zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, int32, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cint32, int32, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, int32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<int32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, int32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x6420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x0000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets,
                       //       ystart,
                       zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32,  COEFF = cint32>
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cint32, cint32, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cint32, cint32, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cint32> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x20;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 2;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul2_sym(xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cint32, cint32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cint32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x20;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 2;
    const unsigned int kDecFactor = 2;

    retVal.val = lmul2_sym(xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch2Buff> firDecHbMulSymCt(
    T_buff_FirDecHb<cint32, cint32, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint32, cint32, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch2Buff> retVal;
    // This would only be called for a fir of length 1
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cint32, cint32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch1Buff> retVal;
    // This would only be called for a fir of length 1
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cint32, cint32, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<cint32, cint32, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cint32, cint32, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cint32> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x20;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 2;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac2_sym(acc.val, xbuff.val, xstart, xoffsets, ybuff.val, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cint32, cint32, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, cint32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cint32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x20;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 2;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac2_sym(acc.val, xbuff.val, xstart, xoffsets, ystart, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch2Buff> firDecHbMacSymCt(
    T_accFirDecHb<cint32, cint32, kArch2Buff> acc,
    T_buff_FirDecHb<cint32, cint32, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cint32, cint32, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch2Buff> retVal;
    const unsigned int xoffsets = 0x20;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 2;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac2(acc.val, xbuff.val, xstart, xoffsets,
                       // ybuff, ystart,                   ct,
                       zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cint32, cint32, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cint32, cint32, kArch1Buff> acc,
    T_buff_FirDecHb<cint32, cint32, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cint32> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cint32, cint32, kArch1Buff> retVal;
    const unsigned int xoffsets = 0x20;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 2;
    const unsigned int kDecFactor = 2;

    retVal.val = lmac2(acc.val, xbuff.val, xstart, xoffsets,
                       // ystart,                   ct,
                       zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = float,  COEFF = float>
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<float, float, kArch2Buff> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<float, float, kArch2Buff> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<float> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<float, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    // since there is no fpmul with preadd, simply perform the 2 sides as assymmetric muls.
    retVal.val = fpmul(xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, ybuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch1Buff> firDecHbMulSym1buff(T_buff_FirDecHb<float, float, kArch1Buff> xbuff,
                                                                        unsigned int xstart,
                                                                        unsigned int ystart,
                                                                        T_buff_256b<float> zbuff,
                                                                        unsigned int zstart) {
    T_accFirDecHb<float, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmul(xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, xbuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch2Buff> firDecHbMulSymCt(T_buff_FirDecHb<float, float, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<float, float, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     unsigned int ct,
                                                                     T_buff_256b<float> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<float, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    // This would be used only for a fir of length 1.

    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<float, float, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<float> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<float, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    // This would be used only for a FIR of length 1.

    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch2Buff> firDecHbMacSym(T_accFirDecHb<float, float, kArch2Buff> acc,
                                                                   T_buff_FirDecHb<float, float, kArch2Buff> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<float, float, kArch2Buff> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<float> zbuff,
                                                                   unsigned int zstart) {
    T_accFirDecHb<float, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, ybuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch1Buff> firDecHbMacSym1buff(T_accFirDecHb<float, float, kArch1Buff> acc,
                                                                        T_buff_FirDecHb<float, float, kArch1Buff> xbuff,
                                                                        unsigned int xstart,
                                                                        unsigned int ystart,
                                                                        T_buff_256b<float> zbuff,
                                                                        unsigned int zstart) {
    T_accFirDecHb<float, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, xbuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch2Buff> firDecHbMacSymCt(T_accFirDecHb<float, float, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<float, float, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<float, float, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     unsigned int ct,
                                                                     T_buff_256b<float> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<float, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    // the centre tap requires no symmetric pair.
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<float, float, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<float, float, kArch1Buff> acc,
    T_buff_FirDecHb<float, float, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<float> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<float, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 8;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    // The centre tap requires no symmetric pair term.
    return retVal;
}

// DATA = cfloat,  COEFF = float>
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cfloat, float, kArch2Buff> xbuff,
                                                                    unsigned int xstart,
                                                                    T_buff_FirDecHb<cfloat, float, kArch2Buff> ybuff,
                                                                    unsigned int ystart,
                                                                    T_buff_256b<float> zbuff,
                                                                    unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // since there is no fpmul with preadd, simply perform the 2 sides as assymmetric muls.
    retVal.val = fpmul(xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, ybuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cfloat, float, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<float> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmul(xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, xbuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch2Buff> firDecHbMulSymCt(T_buff_FirDecHb<cfloat, float, kArch2Buff> xbuff,
                                                                      unsigned int xstart,
                                                                      T_buff_FirDecHb<cfloat, float, kArch2Buff> ybuff,
                                                                      unsigned int ystart,
                                                                      unsigned int ct,
                                                                      T_buff_256b<float> zbuff,
                                                                      unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // This would be used only for a fir of length 1.

    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cfloat, float, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<float> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // This would be used only for a FIR of length 1.

    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cfloat, float, kArch2Buff> acc,
                                                                    T_buff_FirDecHb<cfloat, float, kArch2Buff> xbuff,
                                                                    unsigned int xstart,
                                                                    T_buff_FirDecHb<cfloat, float, kArch2Buff> ybuff,
                                                                    unsigned int ystart,
                                                                    T_buff_256b<float> zbuff,
                                                                    unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, ybuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cfloat, float, kArch1Buff> acc,
    T_buff_FirDecHb<cfloat, float, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<float> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, xbuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch2Buff> firDecHbMacSymCt(T_accFirDecHb<cfloat, float, kArch2Buff> acc,
                                                                      T_buff_FirDecHb<cfloat, float, kArch2Buff> xbuff,
                                                                      unsigned int xstart,
                                                                      T_buff_FirDecHb<cfloat, float, kArch2Buff> ybuff,
                                                                      unsigned int ystart,
                                                                      unsigned int ct,
                                                                      T_buff_256b<float> zbuff,
                                                                      unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    // the centre tap requires no symmetric pair.
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, float, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cfloat, float, kArch1Buff> acc,
    T_buff_FirDecHb<cfloat, float, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<float> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, float, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    // The centre tap requires no symmetric pair term.
    return retVal;
}

// DATA = cfloat,  COEFF = cfloat>
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch2Buff> firDecHbMulSym(T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cfloat> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // since there is no fpmul with preadd, simply perform the 2 sides as assymmetric muls.
    retVal.val = fpmul(xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, ybuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch1Buff> firDecHbMulSym1buff(
    T_buff_FirDecHb<cfloat, cfloat, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cfloat> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmul(xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, xbuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch2Buff> firDecHbMulSymCt(
    T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cfloat> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // This would be used only for a fir of length 1.

    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch1Buff> firDecHbMulSymCt1buff(
    T_buff_FirDecHb<cfloat, cfloat, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cfloat> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    // This would be used only for a FIR of length 1.

    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch2Buff> firDecHbMacSym(T_accFirDecHb<cfloat, cfloat, kArch2Buff> acc,
                                                                     T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<cfloat> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, ybuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch1Buff> firDecHbMacSym1buff(
    T_accFirDecHb<cfloat, cfloat, kArch1Buff> acc,
    T_buff_FirDecHb<cfloat, cfloat, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<cfloat> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    retVal.val = fpmac(retVal.val, xbuff.val, ystart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch2Buff> firDecHbMacSymCt(
    T_accFirDecHb<cfloat, cfloat, kArch2Buff> acc,
    T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<cfloat, cfloat, kArch2Buff> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cfloat> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch2Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    // the centre tap requires no symmetric pair.
    return retVal;
}
template <>
INLINE_DECL T_accFirDecHb<cfloat, cfloat, kArch1Buff> firDecHbMacSymCt1buff(
    T_accFirDecHb<cfloat, cfloat, kArch1Buff> acc,
    T_buff_FirDecHb<cfloat, cfloat, kArch1Buff> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<cfloat> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<cfloat, cfloat, kArch1Buff> retVal;
    const unsigned int xoffsets = 0xECA86420;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x00000000;
    const unsigned int zstep = 1;
    const unsigned int kLanes = 4;
    const unsigned int kDecFactor = 2;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    // The centre tap requires no symmetric pair term.
    return retVal;
}

// Initial MUL operation for 2buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHb(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMulSym(xbuff, xstart, ybuff, ystart, zbuff, zstart);
};

// Initial MAC operation for 2buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHb(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMacSym(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHbCt(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMulSymCt(xbuff, xstart, ybuff, ystart, ct, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHbCt(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMacSymCt(acc, xbuff, xstart, ybuff, ystart, ct, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHb(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMulSym1buff(xbuff, xstart, ystart, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHb(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMacSym1buff(acc, xbuff, xstart, ystart, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHbCt(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMulSymCt1buff(xbuff, xstart, ystart, ct, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int TP_DUAL_IP>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHbCt(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    return firDecHbMacSymCt1buff(acc, xbuff, xstart, ystart, ct, zbuff, zstart);
};
}
}
}
}
}
#endif // _DSPLIB_FIR_DECIMATE_HB_UTILS_HPP_
