#ifndef _DSPLIB_fir_interpolate_fract_asym_UTILS_HPP_
#define _DSPLIB_fir_interpolate_fract_asym_UTILS_HPP_

/*
Single Rate Asymmetrical FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
// T_acc in here.
#include "kernel_utils.hpp"
namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace interpolate_fract_asym {
// based on me_sinple_calls.h, overloaded mul/mac calls
//-----------------------------------------------------------------------------------------------------

inline T_acc<int16, int16> macIntFract(T_acc<int16, int16> acc,
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
inline T_acc<cint16, int16> macIntFract(T_acc<cint16, int16> acc,
                                        T_buff_1024b<cint16> xbuff,
                                        unsigned int xstart,
                                        unsigned int xoffsets,
                                        T_buff_256b<int16> zbuff,
                                        unsigned int zstart,
                                        unsigned int zoffsets) {
    T_acc<cint16, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 8;

    retVal.val = mac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint16, COEFF = cint16
inline T_acc<cint16, cint16> macIntFract(T_acc<cint16, cint16> acc,
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

// DATA = int32, COEFF = int16
inline T_acc<int32, int16> macIntFract(T_acc<int32, int16> acc,
                                       T_buff_1024b<int32> xbuff,
                                       unsigned int xstart,
                                       unsigned int xoffsets,
                                       T_buff_256b<int16> zbuff,
                                       unsigned int zstart,
                                       unsigned int zoffsets) {
    T_acc<int32, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 8;

    retVal.val = lmac8(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = int32,  COEFF = int32>
inline T_acc<int32, int32> macIntFract(T_acc<int32, int32> acc,
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
inline T_acc<cint32, int16> macIntFract(T_acc<cint32, int16> acc,
                                        T_buff_1024b<cint32> xbuff,
                                        unsigned int xstart,
                                        unsigned int xoffsets,
                                        T_buff_256b<int16> zbuff,
                                        unsigned int zstart,
                                        unsigned int zoffsets) {
    T_acc<cint32, int16> retVal;
    const unsigned int xstep = 1;
    const unsigned int zstep = 4;

    retVal.val = lmac4(acc.val, xbuff.val, xstart, xoffsets, xstep, zbuff.val, zstart, zoffsets, zstep);
    return retVal;
}

// DATA = cint32, COEFF =  int32>
inline T_acc<cint32, int32> macIntFract(T_acc<cint32, int32> acc,
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
inline T_acc<cint32, cint16> macIntFract(T_acc<cint32, cint16> acc,
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
inline T_acc<cint32, cint32> macIntFract(T_acc<cint32, cint32> acc,
                                         T_buff_1024b<cint32> xbuff,
                                         unsigned int xstart,
                                         unsigned int xoffsets,
                                         T_buff_256b<cint32> zbuff,
                                         unsigned int zstart,
                                         unsigned int zoffsets) {
    T_acc<cint32, cint32> retVal;

    retVal.val = lmac2(acc.val, xbuff.val, xstart, xoffsets, zbuff.val, zstart,
                       zoffsets); // xoffsets && 0xFF <- only look at first 2 nibbles
    retVal.uval = lmac2(acc.uval, xbuff.val, xstart, xoffsets >> 8, zbuff.val, zstart, zoffsets >> 8);
    return retVal;
}

// DATA = float,  COEFF = float>
inline T_acc<float, float> macIntFract(T_acc<float, float> acc,
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
inline T_acc<cfloat, float> macIntFract(T_acc<cfloat, float> acc,
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
inline T_acc<cfloat, cfloat> macIntFract(T_acc<cfloat, cfloat> acc,
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

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF>
inline T_acc<TT_DATA, TT_COEFF> initMacIntFract(T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface,
                                                T_acc<TT_DATA, TT_COEFF> acc,
                                                T_buff_1024b<TT_DATA> xbuff,
                                                unsigned int xstart,
                                                unsigned int xoffsets,
                                                T_buff_256b<TT_COEFF> zbuff,
                                                unsigned int zstart,
                                                unsigned int zoffset) {
    return macIntFract(acc, xbuff, xstart, xoffsets, zbuff, zstart, zoffset);
};

template <typename TT_DATA, typename TT_COEFF>
inline T_acc<TT_DATA, TT_COEFF> initMacIntFract(T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface,
                                                T_acc<TT_DATA, TT_COEFF> acc,
                                                T_buff_1024b<TT_DATA> xbuff,
                                                unsigned int xstart,
                                                unsigned int xoffsets,
                                                T_buff_256b<TT_COEFF> zbuff,
                                                unsigned int zstart,
                                                unsigned int zoffset) {
    return macIntFract(acc, xbuff, xstart, xoffsets, zbuff, zstart, zoffset);
};
}
}
}
}
}
#endif // _DSPLIB_fir_interpolate_fract_asym_UTILS_HPP_

/*  (c) Copyright 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2) Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability) for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party) even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"). Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */
