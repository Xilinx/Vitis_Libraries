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
#ifndef _DSPLIB_fir_resampler_UTILS_HPP_
#define _DSPLIB_fir_resampler_UTILS_HPP_

#include "device_defs.h"

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace resampler {

#if (__HAS_ACCUM_PERMUTES__ == 1)

//-----------------------------------------------------------------------------------------------------
//------------------------- 768-bit MAC calls
//-----------------------------------------------------------------------------------------------------

INLINE_DECL T_acc<int16, int16> macResampler(T_acc<int16, int16> acc,
                                             T_buff_1024b<int16> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<int16> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc<int16, int16> retVal;
    // static_assert(false, "Error: Unsupported MAC call.");
    return retVal;
}

// DATA = cint16, COEFF = int16
INLINE_DECL T_acc<cint16, int16> macResampler(T_acc<cint16, int16> acc,
                                              T_buff_1024b<cint16> xbuff,
                                              unsigned int xstart,
                                              unsigned int xoffsets,
                                              T_buff_256b<int16> zbuff,
                                              unsigned int zstart,
                                              unsigned int zoffsets) {
    T_acc<cint16, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 8; // lanes

    retVal.val = mac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint16
INLINE_DECL T_acc<cint16, cint16> macResampler(T_acc<cint16, cint16> acc,
                                               T_buff_1024b<cint16> xbuff,
                                               unsigned int xstart,
                                               unsigned int xoffsets,
                                               T_buff_256b<cint16> zbuff,
                                               unsigned int zstart,
                                               unsigned int zoffsets) {
    T_acc<cint16, cint16> retVal;

    retVal.val = mac8(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint16, COEFF = int32
INLINE_DECL T_acc<cint16, int32> macResampler(T_acc<cint16, int32> acc,
                                              T_buff_1024b<cint16> xbuff,
                                              unsigned int xstart,
                                              unsigned int xoffsets,
                                              T_buff_256b<int32> zbuff,
                                              unsigned int zstart,
                                              unsigned int zoffsets) {
    T_acc<cint16, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 4; // lanes

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint32
INLINE_DECL T_acc<cint16, cint32> macResampler(T_acc<cint16, cint32> acc,
                                               T_buff_1024b<cint16> xbuff,
                                               unsigned int xstart,
                                               unsigned int xoffsets,
                                               T_buff_256b<cint32> zbuff,
                                               unsigned int zstart,
                                               unsigned int zoffsets) {
    T_acc<cint16, cint32> retVal;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = int16, COEFF = int32
INLINE_DECL T_acc<int16, int32> macResampler(T_acc<int16, int32> acc,
                                             T_buff_1024b<int16> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<int32> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc<int16, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int mtap = 0;
    const unsigned int zstep = 0;
    unsigned int ystart = xstart;

    // retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    // workaround for exceeding 256-bit zbuff.
    // Do: acc0 = z00*(x00 - y00) + z01*x01, where z00 = z01, x00=y00=x01
    retVal.val =
        lmac8_antisym_ct(acc.val, xbuff.val, xstart, xoffsets, ystart, mtap, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = int32, COEFF = int16
INLINE_DECL T_acc<int32, int16> macResampler(T_acc<int32, int16> acc,
                                             T_buff_1024b<int32> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<int16> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc<int32, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 8; // lanes

    retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = int32,  COEFF = int32>
INLINE_DECL T_acc<int32, int32> macResampler(T_acc<int32, int32> acc,
                                             T_buff_1024b<int32> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<int32> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc<int32, int32> retVal;

    retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  int16>
INLINE_DECL T_acc<cint32, int16> macResampler(T_acc<cint32, int16> acc,
                                              T_buff_1024b<cint32> xbuff,
                                              unsigned int xstart,
                                              unsigned int xoffsets,
                                              T_buff_256b<int16> zbuff,
                                              unsigned int zstart,
                                              unsigned int zoffsets) {
    T_acc<cint32, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 4; // lanes

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32, COEFF =  int32>
INLINE_DECL T_acc<cint32, int32> macResampler(T_acc<cint32, int32> acc,
                                              T_buff_1024b<cint32> xbuff,
                                              unsigned int xstart,
                                              unsigned int xoffsets,
                                              T_buff_256b<int32> zbuff,
                                              unsigned int zstart,
                                              unsigned int zoffsets) {
    T_acc<cint32, int32> retVal;
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  cint16>
INLINE_DECL T_acc<cint32, cint16> macResampler(T_acc<cint32, cint16> acc,
                                               T_buff_1024b<cint32> xbuff,
                                               unsigned int xstart,
                                               unsigned int xoffsets,
                                               T_buff_256b<cint16> zbuff,
                                               unsigned int zstart,
                                               unsigned int zoffsets) {
    T_acc<cint32, cint16> retVal;
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  cint32>
INLINE_DECL T_acc<cint32, cint32> macResampler(T_acc<cint32, cint32> acc,
                                               T_buff_1024b<cint32> xbuff,
                                               unsigned int xstart,
                                               unsigned int xoffsets,
                                               T_buff_256b<cint32> zbuff,
                                               unsigned int zstart,
                                               unsigned int zoffsets) {
    T_acc<cint32, cint32> retVal;

    retVal.val = lmac2(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = float,  COEFF = float>
INLINE_DECL T_acc<float, float> macResampler(T_acc<float, float> acc,
                                             T_buff_1024b<float> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<float> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc<float, float> retVal;
    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cfloat, COEFF =  float>
INLINE_DECL T_acc<cfloat, float> macResampler(T_acc<cfloat, float> acc,
                                              T_buff_1024b<cfloat> xbuff,
                                              unsigned int xstart,
                                              unsigned int xoffsets,
                                              T_buff_256b<float> zbuff,
                                              unsigned int zstart,
                                              unsigned int zoffsets) {
    T_acc<cfloat, float> retVal;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cfloat, COEFF =  cfloat>
INLINE_DECL T_acc<cfloat, cfloat> macResampler(T_acc<cfloat, cfloat> acc,
                                               T_buff_1024b<cfloat> xbuff,
                                               unsigned int xstart,
                                               unsigned int xoffsets,
                                               T_buff_256b<cfloat> zbuff,
                                               unsigned int zstart,
                                               unsigned int zoffsets) {
    T_acc<cfloat, cfloat> retVal;
    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

//-----------------------------------------------------------------------------------------------------
//------------------------- 384-bit MAC calls
//-----------------------------------------------------------------------------------------------------

inline T_acc384<int16, int16> macResampler(T_acc384<int16, int16> acc,
                                           T_buff_1024b<int16> xbuff,
                                           unsigned int xstart,
                                           unsigned int xoffsets,
                                           T_buff_256b<int16> zbuff,
                                           unsigned int zstart,
                                           unsigned int zoffsets) {
    T_acc384<int16, int16> retVal;
    // static_assert(false, "Error: Unsupported MAC call.");
    return retVal;
}

// DATA = cint16, COEFF = int16
inline T_acc384<cint16, int16> macResampler(T_acc384<cint16, int16> acc,
                                            T_buff_1024b<cint16> xbuff,
                                            unsigned int xstart,
                                            unsigned int xoffsets,
                                            T_buff_256b<int16> zbuff,
                                            unsigned int zstart,
                                            unsigned int zoffsets) {
    T_acc384<cint16, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = acc.getLanes(); // lanes

    retVal.val = mac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint16
inline T_acc384<cint16, cint16> macResampler(T_acc384<cint16, cint16> acc,
                                             T_buff_1024b<cint16> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<cint16> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc384<cint16, cint16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = acc.getLanes(); // lanes

    retVal.val = mac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = int32
INLINE_DECL T_acc384<cint16, int32> macResampler(T_acc384<cint16, int32> acc,
                                                 T_buff_1024b<cint16> xbuff,
                                                 unsigned int xstart,
                                                 unsigned int xoffsets,
                                                 T_buff_256b<int32> zbuff,
                                                 unsigned int zstart,
                                                 unsigned int zoffsets) {
    T_acc384<cint16, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 4; // lanes

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint32
INLINE_DECL T_acc384<cint16, cint32> macResampler(T_acc384<cint16, cint32> acc,
                                                  T_buff_1024b<cint16> xbuff,
                                                  unsigned int xstart,
                                                  unsigned int xoffsets,
                                                  T_buff_256b<cint32> zbuff,
                                                  unsigned int zstart,
                                                  unsigned int zoffsets) {
    T_acc384<cint16, cint32> retVal;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = int16, COEFF = int32
INLINE_DECL T_acc384<int16, int32> macResampler(T_acc384<int16, int32> acc,
                                                T_buff_1024b<int16> xbuff,
                                                unsigned int xstart,
                                                unsigned int xoffsets,
                                                T_buff_256b<int32> zbuff,
                                                unsigned int zstart,
                                                unsigned int zoffsets) {
    T_acc384<int16, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int mtap = 0;
    const unsigned int zstep = 0;
    unsigned int ystart = xstart;

    // retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    // workaround for exceeding 256-bit zbuff.
    // Do: acc0 = z00*(x00 - y00) + z01*x01, where z00 = z01, x00=y00=x01
    retVal.val =
        lmac8_antisym_ct(acc.val, xbuff.val, xstart, xoffsets, ystart, mtap, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}
// DATA = int32, COEFF = int16
inline T_acc384<int32, int16> macResampler(T_acc384<int32, int16> acc,
                                           T_buff_1024b<int32> xbuff,
                                           unsigned int xstart,
                                           unsigned int xoffsets,
                                           T_buff_256b<int16> zbuff,
                                           unsigned int zstart,
                                           unsigned int zoffsets) {
    T_acc384<int32, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = acc.getLanes(); // lanes

    retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = int32,  COEFF = int32>
inline T_acc384<int32, int32> macResampler(T_acc384<int32, int32> acc,
                                           T_buff_1024b<int32> xbuff,
                                           unsigned int xstart,
                                           unsigned int xoffsets,
                                           T_buff_256b<int32> zbuff,
                                           unsigned int zstart,
                                           unsigned int zoffsets) {
    T_acc384<int32, int32> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = acc.getLanes(); // lanes

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32, COEFF =  int16>
inline T_acc384<cint32, int16> macResampler(T_acc384<cint32, int16> acc,
                                            T_buff_1024b<cint32> xbuff,
                                            unsigned int xstart,
                                            unsigned int xoffsets,
                                            T_buff_256b<int16> zbuff,
                                            unsigned int zstart,
                                            unsigned int zoffsets) {
    T_acc384<cint32, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = acc.getLanes(); // lanes

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32, COEFF =  int32>
inline T_acc384<cint32, int32> macResampler(T_acc384<cint32, int32> acc,
                                            T_buff_1024b<cint32> xbuff,
                                            unsigned int xstart,
                                            unsigned int xoffsets,
                                            T_buff_256b<int32> zbuff,
                                            unsigned int zstart,
                                            unsigned int zoffsets) {
    T_acc384<cint32, int32> retVal;
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  cint16>
inline T_acc384<cint32, cint16> macResampler(T_acc384<cint32, cint16> acc,
                                             T_buff_1024b<cint32> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<cint16> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc384<cint32, cint16> retVal;
    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cint32, COEFF =  cint32>
inline T_acc384<cint32, cint32> macResampler(T_acc384<cint32, cint32> acc,
                                             T_buff_1024b<cint32> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<cint32> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc384<cint32, cint32> retVal;

    retVal.val = lmac2(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = float,  COEFF = float>
inline T_acc384<float, float> macResampler(T_acc384<float, float> acc,
                                           T_buff_1024b<float> xbuff,
                                           unsigned int xstart,
                                           unsigned int xoffsets,
                                           T_buff_256b<float> zbuff,
                                           unsigned int zstart,
                                           unsigned int zoffsets) {
    T_acc384<float, float> retVal;
    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cfloat, COEFF =  float>
inline T_acc384<cfloat, float> macResampler(T_acc384<cfloat, float> acc,
                                            T_buff_1024b<cfloat> xbuff,
                                            unsigned int xstart,
                                            unsigned int xoffsets,
                                            T_buff_256b<float> zbuff,
                                            unsigned int zstart,
                                            unsigned int zoffsets) {
    T_acc384<cfloat, float> retVal;

    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

// DATA = cfloat, COEFF =  cfloat>
inline T_acc384<cfloat, cfloat> macResampler(T_acc384<cfloat, cfloat> acc,
                                             T_buff_1024b<cfloat> xbuff,
                                             unsigned int xstart,
                                             unsigned int xoffsets,
                                             T_buff_256b<cfloat> zbuff,
                                             unsigned int zstart,
                                             unsigned int zoffsets) {
    T_acc384<cfloat, cfloat> retVal;
    retVal.val = fpmac(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart, zoffsets);
    return retVal;
}

#else  // __HAS_ACCUM_PERMUTES__ = 0

// Do nothing when permute is not supported
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_acc<TT_DATA, TT_COEFF> macResampler(T_acc<TT_DATA, TT_COEFF> acc,
                                                  T_buff_1024b<TT_DATA> xbuff,
                                                  unsigned int xstart,
                                                  unsigned int xoffsets,
                                                  T_buff_256b<TT_COEFF> zbuff,
                                                  unsigned int zstart,
                                                  unsigned int zoffsets) {
    return acc;
};
#endif //__HAS_ACCUM_PERMUTES__

// Initial MAC/MUL operation. Long, 768-bit accs
template <typename TT_DATA, typename TT_COEFF, bool TP_CASC_IN, unsigned int TP_DUAL_IP>
inline T_acc<TT_DATA, TT_COEFF> initMacResampler(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                 T_acc<TT_DATA, TT_COEFF> acc,
                                                 T_buff_1024b<TT_DATA> xbuff,
                                                 unsigned int xstart,
                                                 unsigned int xoffsets,
                                                 T_buff_256b<TT_COEFF> zbuff,
                                                 unsigned int zstart,
                                                 unsigned int zoffset) {
    return macResampler(acc, xbuff, xstart, xoffsets, zbuff, zstart, zoffset);
};

// Initial MAC/MUL operation. Short, 384-bit accs
template <typename TT_DATA, typename TT_COEFF, bool TP_CASC_IN, unsigned int TP_DUAL_IP>
inline T_acc384<TT_DATA, TT_COEFF> initMacResampler(T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                                    T_acc384<TT_DATA, TT_COEFF> acc,
                                                    T_buff_1024b<TT_DATA> xbuff,
                                                    unsigned int xstart,
                                                    unsigned int xoffsets,
                                                    T_buff_256b<TT_COEFF> zbuff,
                                                    unsigned int zstart,
                                                    unsigned int zoffset) {
    return macResampler(acc, xbuff, xstart, xoffsets, zbuff, zstart, zoffset);
};

template <unsigned int TP_INTERPOLATE_FACTOR>
inline constexpr int intDataNeeded(const int x) {
    return (1 + (int)(((x)-1) / TP_INTERPOLATE_FACTOR));
}
}
}
}
}
}
#endif // _DSPLIB_fir_resampler_UTILS_HPP_
