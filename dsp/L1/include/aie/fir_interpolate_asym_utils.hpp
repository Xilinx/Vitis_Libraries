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
#ifndef _DSPLIB_FIR_INTERPOLATE_ASYM_UTILS_HPP_
#define _DSPLIB_FIR_INTERPOLATE_ASYM_UTILS_HPP_

#include "device_defs.h"
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_asym {
/*
Asymmetrical Interpolation FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

// Templated struct to hold the accumulator type appropriate
// to the selected data and coefficient types;
// derived from base T_ACC struct - which requires bringing operator= into scope.
template <typename T_D, typename T_C>
struct T_accIntAsym : T_acc<T_D, T_C> {
    using T_acc<T_D, T_C>::operator=;
};

// templated struct to hold the appropriate vector type for write to the class output window.
template <typename T_D, typename T_C>
struct T_outValIntAsym : T_outVal<T_D, T_C> {
    using T_outVal<T_D, T_C>::operator=;
};

#if (__HAS_ACCUM_PERMUTES__ == 1)

// Overloaded mul/mac calls for asymmetric interpolation
//-----------------------------------------------------------------------------------------------------
// DATA = int16, COEFF = int16
INLINE_DECL T_accIntAsym<int16, int16> mulIntAsym(v64int16 xbuff,
                                                  v16int16 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int16, int16> retVal;
    const unsigned int xsquare = 0x1010;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x03020100;
    const unsigned int zoffsets_hi = 0x07060504;
    const unsigned int zstep = 1;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int xoffsets_hi = (int32)(xoffsetslut >> 32);
    unsigned int zstart = 0;

    retVal.val = mul16(xbuff, xstart, xoffsets, xoffsets_hi, xsquare, zbuff, zstart, zoffsets, zoffsets_hi, zstep);
    return retVal;
}

INLINE_DECL T_accIntAsym<int16, int16> macIntAsym(T_accIntAsym<int16, int16> acc,
                                                  v64int16 xbuff,
                                                  v16int16 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int16, int16> retVal;
    const unsigned int xsquare = 0x1010;
    const unsigned int xstep = 2;
    const unsigned int zoffsets = 0x03020100;
    ;
    const unsigned int zoffsets_hi = 0x07060504;
    const unsigned int zstep = 1;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int xoffsets_hi = (int32)(xoffsetslut >> 32);
    unsigned int zstart = 0;

    retVal.val =
        mac16(acc.val, xbuff, xstart, xoffsets, xoffsets_hi, xsquare, zbuff, zstart, zoffsets, zoffsets_hi, zstep);
    return retVal;
}

// DATA = cint16, COEFF = int16
INLINE_DECL T_accIntAsym<cint16, int16> mulIntAsym(v32cint16 xbuff,
                                                   v16int16 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint16, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = mul8(xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint16, int16> macIntAsym(T_accIntAsym<cint16, int16> acc,
                                                   v32cint16 xbuff,
                                                   v16int16 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint16, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = mac8(acc.val, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint16
INLINE_DECL T_accIntAsym<cint16, cint16> mulIntAsym(v32cint16 xbuff,
                                                    v8cint16 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint16, cint16> retVal;
    const unsigned int zoffsets = 0x76543210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = mul8(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint16, cint16> macIntAsym(T_accIntAsym<cint16, cint16> acc,
                                                    v32cint16 xbuff,
                                                    v8cint16 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint16, cint16> retVal;
    const unsigned int zoffsets = 0x76543210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = mac8(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = int32, COEFF = int16
INLINE_DECL T_accIntAsym<int32, int16> mulIntAsym(v32int32 xbuff,
                                                  v16int16 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int32, int16> retVal;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int xstep = 1;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul8(xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

INLINE_DECL T_accIntAsym<int32, int16> macIntAsym(T_accIntAsym<int32, int16> acc,
                                                  v32int32 xbuff,
                                                  v16int16 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int32, int16> retVal;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int xstep = 1;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac8(acc.val, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = int32
INLINE_DECL T_accIntAsym<cint16, int32> mulIntAsym(v32cint16 xbuff,
                                                   v8int32 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint16, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul4(xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint16, int32> macIntAsym(T_accIntAsym<cint16, int32> acc,
                                                   v32cint16 xbuff,
                                                   v8int32 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint16, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac4(acc.val, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint32
INLINE_DECL T_accIntAsym<cint16, cint32> mulIntAsym(v32cint16 xbuff,
                                                    v4cint32 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint16, cint32> retVal;
    const unsigned int zoffsets = 0x76543210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul4(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint16, cint32> macIntAsym(T_accIntAsym<cint16, cint32> acc,
                                                    v32cint16 xbuff,
                                                    v4cint32 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint16, cint32> retVal;
    const unsigned int zoffsets = 0x76543210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac4(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = int16, COEFF = int32
INLINE_DECL T_accIntAsym<int16, int32> mulIntAsym(v64int16 xbuff,
                                                  v8int32 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int16, int32> retVal;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int xstep = 1;
    const unsigned int mtap = 0;
    const unsigned int zstep = 0;
    unsigned int xstart = xstartlut;
    unsigned int ystart = xstart;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    // retVal.val = lmul8(     xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    // workaround for exceeding 256-bit zbuff.
    // Do: acc0 = z00*(x00 - y00) + z01*x01, where z00 = z01, x00=y00=x01
    retVal.val = lmul8_antisym_ct(xbuff, xstart, xoffsets, ystart, mtap, zbuff, zstart, zoffsets, zstep);

    return retVal;
}

INLINE_DECL T_accIntAsym<int16, int32> macIntAsym(T_accIntAsym<int16, int32> acc,
                                                  v64int16 xbuff,
                                                  v8int32 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int16, int32> retVal;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int xstep = 1;
    const unsigned int mtap = 0;
    const unsigned int zstep = 0;
    unsigned int xstart = xstartlut;
    unsigned int ystart = xstart;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    // retVal.val = lmac8(acc.val, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    // workaround for exceeding 256-bit zbuff.
    // Do: acc0 = z00*(x00 - y00) + z01*x01, where z00 = z01, x00=y00=x01
    retVal.val = lmac8_antisym_ct(acc.val, xbuff, xstart, xoffsets, ystart, mtap, zbuff, zstart, zoffsets, zstep);

    return retVal;
}

// DATA = int32,  COEFF = int32>
INLINE_DECL T_accIntAsym<int32, int32> mulIntAsym(v32int32 xbuff,
                                                  v8int32 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int32, int32> retVal;
    const unsigned int zoffsets = 0x76543210; // was = 0x00000000;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul8(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<int32, int32> macIntAsym(T_accIntAsym<int32, int32> acc,
                                                  v32int32 xbuff,
                                                  v8int32 zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<int32, int32> retVal;
    const unsigned int zoffsets = 0x76543210; // 0x00000000;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac8(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  int16>
INLINE_DECL T_accIntAsym<cint32, int16> mulIntAsym(v16cint32 xbuff,
                                                   v16int16 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint32, int16> retVal;
    const unsigned int zoffsets = 0x3210;
    const unsigned int xstep = 1;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul4(xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint32, int16> macIntAsym(T_accIntAsym<cint32, int16> acc,
                                                   v16cint32 xbuff,
                                                   v16int16 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint32, int16> retVal;
    const unsigned int zoffsets = 0x3210;
    const unsigned int xstep = 1;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac4(acc.val, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32, COEFF =  int32>
INLINE_DECL T_accIntAsym<cint32, int32> mulIntAsym(v16cint32 xbuff,
                                                   v8int32 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint32, int32> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul4(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint32, int32> macIntAsym(T_accIntAsym<cint32, int32> acc,
                                                   v16cint32 xbuff,
                                                   v8int32 zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cint32, int32> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac4(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  cint16>
INLINE_DECL T_accIntAsym<cint32, cint16> mulIntAsym(v16cint32 xbuff,
                                                    v8cint16 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint32, cint16> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmul4(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint32, cint16> macIntAsym(T_accIntAsym<cint32, cint16> acc,
                                                    v16cint32 xbuff,
                                                    v8cint16 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint32, cint16> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac4(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  cint32>
INLINE_DECL T_accIntAsym<cint32, cint32> mulIntAsym(v16cint32 xbuff,
                                                    const v4cint32 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint32, cint32> retVal;
    const unsigned int zoffsets = 0x10;
    const unsigned int zoffsets2 = 0x32;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int xoffsets2 = (int32)xoffsetslut >> 8;
    unsigned int zstart = 0;

    retVal.val = lmul2(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    retVal.uval = lmul2(xbuff, xstart, xoffsets2, zbuff, zstart, zoffsets2);
    return retVal;
}

INLINE_DECL T_accIntAsym<cint32, cint32> macIntAsym(T_accIntAsym<cint32, cint32> acc,
                                                    v16cint32 xbuff,
                                                    v4cint32 zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cint32, cint32> retVal;
    const unsigned int zoffsets = 0x10;
    const unsigned int zoffsets2 = 0x32;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int xoffsets2 = (int32)xoffsetslut >> 8;
    unsigned int zstart = 0;

    retVal.val = lmac2(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    retVal.uval = lmac2(acc.uval, xbuff, xstart, xoffsets2, zbuff, zstart, zoffsets2);
    return retVal;
}

// DATA = float,  COEFF = float>
INLINE_DECL T_accIntAsym<float, float> mulIntAsym(v32float xbuff,
                                                  v8float zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<float, float> retVal;
    const unsigned int zoffsets = 0x76543210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = fpmul(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<float, float> macIntAsym(T_accIntAsym<float, float> acc,
                                                  v32float xbuff,
                                                  v8float zbuff,
                                                  unsigned int interpolateFactor,
                                                  unsigned int lanes,
                                                  int64 xoffsetslut,
                                                  int xstartlut) {
    T_accIntAsym<float, float> retVal;
    const unsigned int zoffsets = 0x76543210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = fpmac(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = cfloat, COEFF =  float>
INLINE_DECL T_accIntAsym<cfloat, float> mulIntAsym(v16cfloat xbuff,
                                                   v8float zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cfloat, float> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = fpmul(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<cfloat, float> macIntAsym(T_accIntAsym<cfloat, float> acc,
                                                   v16cfloat xbuff,
                                                   v8float zbuff,
                                                   unsigned int interpolateFactor,
                                                   unsigned int lanes,
                                                   int64 xoffsetslut,
                                                   int xstartlut) {
    T_accIntAsym<cfloat, float> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = fpmac(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// DATA = cfloat, COEFF =  cfloat>
INLINE_DECL T_accIntAsym<cfloat, cfloat> mulIntAsym(v16cfloat xbuff,
                                                    v4cfloat zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cfloat, cfloat> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = fpmul(xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

INLINE_DECL T_accIntAsym<cfloat, cfloat> macIntAsym(T_accIntAsym<cfloat, cfloat> acc,
                                                    v16cfloat xbuff,
                                                    v4cfloat zbuff,
                                                    unsigned int interpolateFactor,
                                                    unsigned int lanes,
                                                    int64 xoffsetslut,
                                                    int xstartlut) {
    T_accIntAsym<cfloat, cfloat> retVal;
    const unsigned int zoffsets = 0x3210;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = fpmac(acc.val, xbuff, xstart, xoffsets, zbuff, zstart, zoffsets);
    return retVal;
}

// Overloaded mul call for asymmetric interpolation
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_accIntAsym<TT_DATA, TT_COEFF> mulIntAsym(T_buff_1024b<TT_DATA> xbuff,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int interpolateFactor,
                                                       unsigned int lanes,
                                                       int64 xoffsetslut,
                                                       int xstart) {
    return mulIntAsym(xbuff.val, zbuff.val, interpolateFactor, lanes, xoffsetslut, xstart);
};
// (T_accIntAsym<int16, int16> acc, v64int16 xbuff, v16int16 zbuff,
// unsigned int interpolateFactor, unsigned int lanes, int64 xoffsetslut, int xstartlut)
// Overloaded mac call for asymmetric interpolation
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_accIntAsym<TT_DATA, TT_COEFF> macIntAsym(T_accIntAsym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int interpolateFactor,
                                                       unsigned int lanes,
                                                       int64 xoffsetslut,
                                                       int xstart) {
    return macIntAsym(acc, xbuff.val, zbuff.val, interpolateFactor, lanes, xoffsetslut, xstart);
};

#else  // __HAS_ACCUM_PERMUTES__ = 0

// Do nothing when permute is not supported
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_accIntAsym<TT_DATA, TT_COEFF> mulIntAsym(T_buff_1024b<TT_DATA> xbuff,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int interpolateFactor,
                                                       unsigned int lanes,
                                                       int64 xoffsetslut,
                                                       int xstart) {
    T_accIntAsym<TT_DATA, TT_COEFF> empty_acc;
    return empty_acc;
};

// Do nothing when permute is not supported
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_accIntAsym<TT_DATA, TT_COEFF> macIntAsym(T_accIntAsym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int interpolateFactor,
                                                       unsigned int lanes,
                                                       int64 xoffsetslut,
                                                       int xstart) {
    return acc;
};
#endif //__HAS_ACCUM_PERMUTES__

// Interleave
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_INPUTS, unsigned int TP_NUM_OUTPUTS>
INLINE_DECL void bufferInterleaveIntAsym(std::array<T_outVal<TT_DATA, TT_COEFF>, TP_NUM_INPUTS> outArray,
                                         auto& outItr,
                                         auto& outItr2) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr int kSamplesIn256b = kSamplesIn128b * 2;
    constexpr int kSamplesIn512b = kSamplesIn128b * 4;
    using t512v = ::aie::vector<TT_DATA, kSamplesIn512b>;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t128v = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using vec = typename T_outVal<TT_DATA, TT_COEFF>::v_type;
    constexpr int kSamplesInVec = T_outVal<TT_DATA, TT_COEFF>::getLanes();
    if
        constexpr(TP_NUM_INPUTS == 1) { *outItr++ = outArray[0].val; }
    else if
        constexpr(TP_NUM_INPUTS == 2) {
            vec writeVal;
            ::std::pair<vec, vec> inIntlv = ::aie::interleave_zip(outArray[0].val, outArray[1].val, 1);
            writeVal = inIntlv.first;
            *outItr++ = writeVal;
            writeVal = inIntlv.second;
            *outItr++ = writeVal;
            if
                constexpr(TP_NUM_OUTPUTS == 2) {
                    writeVal = inIntlv.first;
                    *outItr2++ = writeVal;
                    writeVal = inIntlv.second;
                    *outItr2++ = writeVal;
                }
        }
    else if
        constexpr(TP_NUM_INPUTS == 3 || TP_NUM_INPUTS == 5 || TP_NUM_INPUTS == 6 || TP_NUM_INPUTS == 7 ||
                  TP_NUM_INPUTS == 9) {
            vec writeValLoc;
#pragma unroll(TP_NUM_INPUTS* kSamplesInVec)
            for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec); j++) {
                writeValLoc[j % kSamplesInVec] = outArray[j % TP_NUM_INPUTS].val.get(j / TP_NUM_INPUTS);

                if ((j - kSamplesInVec + 1) % kSamplesInVec == 0) {
                    *outItr++ = writeValLoc;
                }
            }
            // Interleave 4 to 1
        }
    else if
        constexpr(TP_NUM_INPUTS == 4) {
            vec writeValLoc;
            ::std::pair<vec, vec> inIntlv0 =
                ::aie::interleave_zip(outArray[0].val, outArray[2].val, 1); // Interleave 4 to 2
            ::std::pair<vec, vec> inIntlv1 =
                ::aie::interleave_zip(outArray[1].val, outArray[3].val, 1); // Interleave 2 to 1
            ::std::pair<vec, vec> inIntlv2 =
                ::aie::interleave_zip(inIntlv0.first, inIntlv1.first, 1); // Interleave 2 to 1
            writeValLoc = inIntlv2.first;
            *outItr++ = writeValLoc;
            writeValLoc = inIntlv2.second;
            *outItr++ = writeValLoc;
            inIntlv2 = ::aie::interleave_zip(inIntlv0.second, inIntlv1.second, 1); // Interleave 2 to 1
            writeValLoc = inIntlv2.first;
            *outItr++ = writeValLoc;
            writeValLoc = inIntlv2.second;
            *outItr++ = writeValLoc;
        }
    else if
        constexpr(TP_NUM_INPUTS == 8) {
            vec writeValLoc;
            ::std::pair<vec, vec> inIntlv0 = ::aie::interleave_zip(outArray[0].val, outArray[4].val, 1);
            ::std::pair<vec, vec> inIntlv1 = ::aie::interleave_zip(outArray[1].val, outArray[5].val, 1);
            ::std::pair<vec, vec> inIntlv2 = ::aie::interleave_zip(outArray[2].val, outArray[6].val, 1);
            ::std::pair<vec, vec> inIntlv3 = ::aie::interleave_zip(outArray[3].val, outArray[7].val, 1);

            ::std::pair<vec, vec> inIntlv4 = ::aie::interleave_zip(inIntlv0.first, inIntlv2.first, 1);
            ::std::pair<vec, vec> inIntlv5 = ::aie::interleave_zip(inIntlv1.first, inIntlv3.first, 1);

            ::std::pair<vec, vec> inIntlv6 = ::aie::interleave_zip(inIntlv4.first, inIntlv5.first, 1);
            writeValLoc = inIntlv6.first;
            *outItr++ = writeValLoc;
            writeValLoc = inIntlv6.second;
            *outItr++ = writeValLoc;

            inIntlv6 = ::aie::interleave_zip(inIntlv4.second, inIntlv5.second, 1);
            writeValLoc = inIntlv6.first;
            *outItr++ = writeValLoc;
            writeValLoc = inIntlv6.second;
            *outItr++ = writeValLoc;

            inIntlv4 = ::aie::interleave_zip(inIntlv0.second, inIntlv2.second, 1);
            inIntlv5 = ::aie::interleave_zip(inIntlv1.second, inIntlv3.second, 1);

            inIntlv6 = ::aie::interleave_zip(inIntlv4.first, inIntlv5.first, 1);
            writeValLoc = inIntlv6.first;
            *outItr++ = writeValLoc;
            writeValLoc = inIntlv6.second;
            *outItr++ = writeValLoc;

            inIntlv6 = ::aie::interleave_zip(inIntlv4.second, inIntlv5.second, 1);
            writeValLoc = inIntlv6.first;
            *outItr++ = writeValLoc;
            writeValLoc = inIntlv6.second;
            *outItr++ = writeValLoc;
        }
};

// Interleave
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_NUM_INPUTS, unsigned int TP_NUM_OUTPUTS>
INLINE_DECL void streamInterleaveIntAsym(std::array<T_outVal384<TT_DATA, TT_COEFF>, TP_NUM_INPUTS> outArray,
                                         T_outputIF<CASC_OUT_FALSE, TT_DATA> outInterface) {
    using vec = typename T_outVal384<TT_DATA, TT_COEFF>::v_type;
    constexpr int kSamplesInVec = T_outVal384<TT_DATA, TT_COEFF>::getLanes();

    if
        constexpr(std::is_same<TT_DATA, int16>::value) {
// Stream IO is 32-bit wide. In order to utilize full BW, stick 2 int16 together for a stream access.

#pragma unroll(TP_NUM_INPUTS* kSamplesInVec / 2)
            for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec); j += 2) {
                int16 outSample1 = outArray[j % TP_NUM_INPUTS].val.get(j / TP_NUM_INPUTS);
                int16 outSample2 = outArray[(j + 1) % TP_NUM_INPUTS].val.get((j + 1) / TP_NUM_INPUTS);
                int32 outSample32 = (outSample2 << 16) | (0xFFFF & outSample1);
                writeincr((output_stream<int32>*)outInterface.outStream, outSample32);
            }
        }
    else {
#pragma unroll(TP_NUM_INPUTS* kSamplesInVec)
        for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec); j++) {
            TT_DATA outSample = outArray[j % TP_NUM_INPUTS].val.get(j / TP_NUM_INPUTS);
            writeincr(outInterface.outStream, outSample);
        }
    }
};

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal<TT_DATA, TT_COEFF> shiftAndSaturateIntAsym(const T_acc<TT_DATA, TT_COEFF> acc, const int shift) {
    return shiftAndSaturate(acc, shift);
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
INLINE_DECL T_accIntAsym<TT_DATA, TT_COEFF> initMacIntAsym(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accIntAsym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int interpolateFactor,
                                                           unsigned int lanes,
                                                           int64 xoffsetslut,
                                                           int xstart) {
    return mulIntAsym(xbuff, zbuff, interpolateFactor, lanes, xoffsetslut, xstart);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DUAL_IP>
INLINE_DECL T_accIntAsym<TT_DATA, TT_COEFF> initMacIntAsym(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accIntAsym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int interpolateFactor,
                                                           unsigned int lanes,
                                                           int64 xoffsetslut,
                                                           int xstart) {
    return macIntAsym(acc, xbuff, zbuff, interpolateFactor, lanes, xoffsetslut, xstart);
};
}
}
}
}
} // namespaces

#endif // _DSPLIB_FIR_INTERPOLATE_HB_UTILS_HPP_
