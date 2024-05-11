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
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
struct T_buff_FirDecHb : T_buff_1024b<TT_DATA> {
    using T_buff_1024b<TT_DATA>::operator=;
};

// 2 buff arch aims to use 2 x 512-bit, but some data type combos require 2 x 1024-bit to store enough data for
// decimation.
// This degrades performance, but no alternative is available. See note above.
template <typename TT_DATA, typename TT_COEFF>
struct T_buff_FirDecHb<TT_DATA, TT_COEFF, kArch2Buff> : T_buff_512b<TT_DATA> {
    using T_buff_512b<TT_DATA>::operator=;
};
template <>
struct T_buff_FirDecHb<int32, int16, kArch2Buff> : T_buff_1024b<int32> {
    using T_buff_1024b<int32>::operator=;
}; // See note regarding Input Vector register size selection.
template <>
struct T_buff_FirDecHb<cint32, int16, kArch2Buff> : T_buff_1024b<cint32> {
    using T_buff_1024b<cint32>::operator=;
}; // See note regarding Input Vector register size selection.
#if __SUPPORTS_CFLOAT__ == 1
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
#endif
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
struct T_accFirDecHb : T_acc384<TT_DATA, TT_COEFF> {
    using T_acc384<TT_DATA, TT_COEFF>::operator=;
};

template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
struct T_outValFiRDecHb : T_outVal384<TT_DATA, TT_COEFF> {
    using T_outVal384<TT_DATA, TT_COEFF>::operator=;
};

//
//---------------------------------------------------------------------------------------------------
// Functions
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int T_SIZE>
INLINE_DECL void fnLoadXIpData(T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH>& buff, unsigned int splice, auto& inItr) {
    constexpr short kSpliceRange = T_SIZE == 256 ? 4 : 8;
    constexpr int kVectSize = T_SIZE / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVectSize>;
    t_vect* readPtr = (t_vect*)&*inItr;
    t_vect readVal = *readPtr;
    inItr += kVectSize;

    buff.val.insert(splice % kSpliceRange, readVal);
};

// Function to load reverse direction data
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH, unsigned int T_SIZE>
INLINE_DECL void fnLoadYIpData(T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH>& buff, unsigned int splice, auto& inItr) {
    const short kSpliceRange = T_SIZE == 256 ? 4 : 8;
    constexpr int kVectSize = T_SIZE / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVectSize>;
    t_vect* readPtr = (t_vect*)&*inItr;
    t_vect readVal = *readPtr;
    inItr -= kVectSize;
    buff.val.insert(splice % kSpliceRange, readVal);
};

// 2buff MAC
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> firDecHbMacSym(T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
                                                                     T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
                                                                     unsigned int xstart,
                                                                     T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
                                                                     unsigned int ystart,
                                                                     T_buff_256b<TT_COEFF> zbuff,
                                                                     unsigned int zstart) {
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> retVal;
    constexpr unsigned int Lanes = fnNumOpLanesDecHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsDecHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 2;
    constexpr unsigned int DataStepY = 2;

    // floats datapath doesn't have a pre-adder, need to issue 2 x non-sym calls.
    if
        constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val, zbuff.val,
                                                                                                   zstart, xbuff.val,
                                                                                                   xstart);
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(retVal.val,
                                                                                                   zbuff.val, zstart,
                                                                                                   ybuff.val, ystart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                             xbuff.val, xstart,
                                                                                             ybuff.val, ystart);
    }

    return retVal;
};

template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> firDecHbMacSym1buff(
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> retVal;
    constexpr unsigned int Lanes = fnNumOpLanesDecHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsDecHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 2;
    constexpr unsigned int DataStepY = 2;

    // floats datapath doesn't have a pre-adder, need to issue 2 x non-sym calls.
    if
        constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val, zbuff.val,
                                                                                                   zstart, xbuff.val,
                                                                                                   xstart);
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(retVal.val,
                                                                                                   zbuff.val, zstart,
                                                                                                   xbuff.val, ystart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                             xbuff.val, xstart, ystart);
    }

    return retVal;
};
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> firDecHbMacSymCt(
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> retVal;
    constexpr unsigned int Lanes = fnNumOpLanesDecHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = (2 * fnNumColumnsDecHb<TT_DATA, TT_COEFF>() -
                                     1); // cascaded kernels should all get 0 CT Points apart from last in chain.
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 2;
    constexpr unsigned int DataStepY = 2;

    T_buff_256b<TT_COEFF> zbuffInt = zbuff; // init with zbuff to get col 0
    T_buff_256b<TT_COEFF> zbuffIntCt = zbuff;
    const unsigned int zstep = 1;
    // const unsigned int ctCoeffPos = Points==K_CT_OP_WITH_5_SAMPLES?2:1;
    const unsigned int ctDataPos = xstart + (Points + 1) / 2 - 1;

    if
        constexpr(Points == 0) {
            // Do nothing. No center tap
            retVal = acc;
        }
    else if
        constexpr(Points == 1) {
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val,
                                                                                                   zbuffInt.val, zstart,
                                                                                                   xbuff.val, xstart);
        }
    else if
        constexpr(Points % 2 == 1 && (std::is_same_v<TT_COEFF, int16> && std::is_same_v<TT_DATA, int16>)) {
            // Int16 data & int16 coeff combo does not offer a CT call.
            // For an odd number of points, we need to spoof it with a non-ct symmetric call followed by asym call.

            retVal.val =
                ::aie::sliding_mul_sym_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX,
                                           DataStepY, TT_COEFF, TT_DATA,
                                           tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuffIntCt.val, 0,
                                                                                        xbuff.val, xstart, ybuff.val,
                                                                                        ystart);
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(retVal.val,
                                                                                                   zbuffIntCt.val, 4,
                                                                                                   xbuff.val,
                                                                                                   ctDataPos);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuffInt.val,
                                                                                             zstart, xbuff.val, xstart,
                                                                                             ybuff.val, ystart);
    }

    return retVal;
};

template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> firDecHbMacSymCt1buff(
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
    T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart) {
    T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> retVal;
    constexpr unsigned int Lanes = fnNumOpLanesDecHb<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = (2 * fnNumColumnsDecHb<TT_DATA, TT_COEFF>() -
                                     1); // cascaded kernels should all get 0 CT Points apart from last in chain.
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 2;
    constexpr unsigned int DataStepY = 2;

    T_buff_256b<TT_COEFF> zbuffInt = zbuff; // init with zbuff to get col 0
    T_buff_256b<TT_COEFF> zbuffIntCt = zbuff;
    const unsigned int zstep = 1;
    // const unsigned int ctCoeffPos = Points==K_CT_OP_WITH_5_SAMPLES?2:1;
    const unsigned int ctDataPos = xstart + (Points + 1) / 2 - 1;

    if
        constexpr(Points == 0) {
            // Do nothing. No center tap
            retVal = acc;
        }
    else if
        constexpr(Points == 1) {
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val,
                                                                                                   zbuffInt.val, zstart,
                                                                                                   xbuff.val, xstart);
        }
    else if
        constexpr(Points % 2 == 1 && (std::is_same_v<TT_COEFF, int16> && std::is_same_v<TT_DATA, int16>)) {
            // Int16 data & int16 coeff combo does not offer a CT call.
            // For an odd number of points, we need to spoof it with a non-ct symmetric call followed by asym call.

            retVal.val =
                ::aie::sliding_mul_sym_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX,
                                           DataStepY, TT_COEFF, TT_DATA,
                                           tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuffIntCt.val, 0,
                                                                                        xbuff.val, xstart, ystart);
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsDecHb<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(retVal.val,
                                                                                                   zbuffIntCt.val, 4,
                                                                                                   xbuff.val,
                                                                                                   ctDataPos);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuffInt.val,
                                                                                             zstart, xbuff.val, xstart);
    }

    return retVal;
};

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

// Initial MAC operation for 2buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHb(T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
                                                                   T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
                                                                   unsigned int xstart,
                                                                   T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> ybuff,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<TT_COEFF> zbuff,
                                                                   unsigned int zstart) {
    return firDecHbMacSym(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHbCt(T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
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
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHb(T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
                                                                   T_buff_FirDecHb<TT_DATA, TT_COEFF, TP_ARCH> xbuff,
                                                                   unsigned int xstart,
                                                                   unsigned int ystart,
                                                                   T_buff_256b<TT_COEFF> zbuff,
                                                                   unsigned int zstart) {
    return firDecHbMacSym1buff(acc, xbuff, xstart, ystart, zbuff, zstart);
};

// Initial MAC operation for 1buff arch. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, eArchType TP_ARCH>
INLINE_DECL T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> initMacDecHbCt(T_accFirDecHb<TT_DATA, TT_COEFF, TP_ARCH> acc,
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
