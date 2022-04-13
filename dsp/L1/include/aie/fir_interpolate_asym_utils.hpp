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
#ifndef _DSPLIB_FIR_INTERPOLATE_ASYM_UTILS_HPP_
#define _DSPLIB_FIR_INTERPOLATE_ASYM_UTILS_HPP_

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif
#ifndef NOINLINE_DECL
#define NOINLINE_DECL inline __attribute__((noinline))
#endif

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
    const unsigned int kDRegLen = 8;
    const unsigned int kBitsInNibble = 4;
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
    const unsigned int kDRegLen = 8;
    const unsigned int kBitsInNibble = 4;
    const unsigned int zoffsets = 0x76543210;
    const unsigned int xstep = 1;
    const unsigned int zstep = lanes;
    unsigned int xstart = xstartlut;
    unsigned int xoffsets = (int32)xoffsetslut;
    unsigned int zstart = 0;

    retVal.val = lmac8(acc.val, xbuff, xstart, xoffsets, xstep, zbuff, zstart, zoffsets, zstep);
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
    const unsigned int kDRegLen = 8;
    const unsigned int kBitsInNibble = 4;
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
    const unsigned int kDRegLen = 8;
    const unsigned int kBitsInNibble = 4;
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

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal<TT_DATA, TT_COEFF> shiftAndSaturateIntAsym(const T_acc<TT_DATA, TT_COEFF> acc, const int shift) {
    return shiftAndSaturate(acc, shift);
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
