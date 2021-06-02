#ifndef _DSPLIB_FIR_DECIMATE_ASYM_UTILS_HPP_
#define _DSPLIB_FIR_DECIMATE_ASYM_UTILS_HPP_

/*
Asymmetrical Decimation FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "fir_decimate_asym.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_asym {

#ifndef Y_BUFFER
#define Y_BUFFER ya
#endif
#ifndef X_BUFFER
#define X_BUFFER xd
#endif
#ifndef Z_BUFFER
#define Z_BUFFER wc0
#endif

template <typename T_D, typename T_C>
struct T_accDecAsym {};
template <>
struct T_accDecAsym<int16, int16> : T_acc384<int16, int16> {
    using T_acc384<int16, int16>::operator=;
};
template <>
struct T_accDecAsym<cint16, int16> : T_acc384<cint16, int16> {
    using T_acc384<cint16, int16>::operator=;
};
template <>
struct T_accDecAsym<cint16, cint16> : T_acc384<cint16, cint16> {
    using T_acc384<cint16, cint16>::operator=;
};
template <>
struct T_accDecAsym<int32, int16> : T_acc384<int32, int16> {
    using T_acc384<int32, int16>::operator=;
};
template <>
struct T_accDecAsym<int32, int32> : T_acc384<int32, int32> {
    using T_acc384<int32, int32>::operator=;
};
template <>
struct T_accDecAsym<cint32, int16> : T_acc384<cint32, int16> {
    using T_acc384<cint32, int16>::operator=;
};
template <>
struct T_accDecAsym<cint32, int32> : T_acc384<cint32, int32> {
    using T_acc384<cint32, int32>::operator=;
};
template <>
struct T_accDecAsym<cint32, cint16> : T_acc384<cint32, cint16> {
    using T_acc384<cint32, cint16>::operator=;
};
template <>
struct T_accDecAsym<cint32, cint32> : T_acc384<cint32, cint32> {
    using T_acc384<cint32, cint32>::operator=;
};
template <>
struct T_accDecAsym<float, float> : T_acc384<float, float> {
    using T_acc384<float, float>::operator=;
};
template <>
struct T_accDecAsym<cfloat, float> : T_acc384<cfloat, float> {
    using T_acc384<cfloat, float>::operator=;
};
template <>
struct T_accDecAsym<cfloat, cfloat> : T_acc384<cfloat, cfloat> {
    using T_acc384<cfloat, cfloat>::operator=;
};

template <typename T_D, typename T_C>
struct T_outValDecAsym {};
template <>
struct T_outValDecAsym<int16, int16> : T_outVal384<int16, int16> {
    using T_outVal384<int16, int16>::operator=;
};
template <>
struct T_outValDecAsym<cint16, int16> : T_outVal384<cint16, int16> {
    using T_outVal384<cint16, int16>::operator=;
};
template <>
struct T_outValDecAsym<cint16, cint16> : T_outVal384<cint16, cint16> {
    using T_outVal384<cint16, cint16>::operator=;
};
template <>
struct T_outValDecAsym<int32, int16> : T_outVal384<int32, int16> {
    using T_outVal384<int32, int16>::operator=;
};
template <>
struct T_outValDecAsym<int32, int32> : T_outVal384<int32, int32> {
    using T_outVal384<int32, int32>::operator=;
};
template <>
struct T_outValDecAsym<cint32, int16> : T_outVal384<cint32, int16> {
    using T_outVal384<cint32, int16>::operator=;
};
template <>
struct T_outValDecAsym<cint32, int32> : T_outVal384<cint32, int32> {
    using T_outVal384<cint32, int32>::operator=;
};
template <>
struct T_outValDecAsym<cint32, cint16> : T_outVal384<cint32, cint16> {
    using T_outVal384<cint32, cint16>::operator=;
};
template <>
struct T_outValDecAsym<cint32, cint32> : T_outVal384<cint32, cint32> {
    using T_outVal384<cint32, cint32>::operator=;
};
template <>
struct T_outValDecAsym<float, float> : T_outVal384<float, float> {
    using T_outVal384<float, float>::operator=;
};
template <>
struct T_outValDecAsym<cfloat, float> : T_outVal384<cfloat, float> {
    using T_outVal384<cfloat, float>::operator=;
};
template <>
struct T_outValDecAsym<cfloat, cfloat> : T_outVal384<cfloat, cfloat> {
    using T_outVal384<cfloat, cfloat>::operator=;
};

//---------------------------------------------------------------------------------------------------
// Functions

// Overloaded shift and saturate calls to allow null operation for float types
template <typename TT_DATA, typename TT_COEFF>
inline T_outVal384<TT_DATA, TT_COEFF> shiftAndSaturateDecAsym(const T_acc384<TT_DATA, TT_COEFF> acc, const int shift) {
    // generic 384-bit wide acc shift and saturate
    return shiftAndSaturate(acc, shift);
};

//
template <typename TT_DATA, unsigned int T_SIZE>
inline void fnLoadXIpData(T_buff_1024b<TT_DATA>& buff, const unsigned int splice, input_window<TT_DATA>* inWindow) {
    using buf_type = typename T_buff_1024b<TT_DATA>::v_type;
    if
        constexpr(T_SIZE == 256) {
            T_buff_256b<TT_DATA> readData;
            const short kSpliceRange = 4;
            readData = window_readincr_256b<TT_DATA>(inWindow); // Read 256b from input window
            buf_type chess_storage(Y_BUFFER) sb = upd_w(buff.val, splice % kSpliceRange, readData.val);
            buff.val = sb;
        }
    else {
        T_buff_128b<TT_DATA> readData;
        const short kSpliceRange = 8;
        readData = window_readincr_128b<TT_DATA>(inWindow);
        buf_type chess_storage(Y_BUFFER) sb = upd_v(buff.val, splice % kSpliceRange, readData.val);
        buff.val = sb;
    }
};

#ifndef _DSPLIB_FIR_AIE_LLI_API_DEBUG_

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA>
inline T_buff_512b<TT_DATA> select(T_buff_1024b<TT_DATA> xbuff,
                                   unsigned int xstart,
                                   const unsigned int xOffsets,
                                   const unsigned int xstartUpper) {
    using buf_type = typename T_buff_512b<TT_DATA>::v_type;
    buf_type chess_storage(X_BUFFER) tmp;
    T_buff_512b<TT_DATA> retVal;
    const unsigned int sel = 0xFF00;
    const unsigned int xoffsets = xOffsets;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = 0;
    const unsigned int yoffsets_hi = xOffsets;
    retVal.val = select16(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xstartUpper, yoffsets, yoffsets_hi);
    return retVal;
}

//-----------------------------------------------------------------------------------------------------
template <>
inline T_buff_512b<int32> select(T_buff_1024b<int32> xbuff,
                                 unsigned int xstart,
                                 const unsigned int xOffsets,
                                 const unsigned int xstartUpper) {
    // xstartUpper = xstart + m_kLanes/2 * TP_DECIMATE_FACTOR; //upper half of lanes
    const unsigned int lanes = 2 * (xstartUpper - xstart) / ((0xF00 & xOffsets) >> 8); // extract lanes
    T_buff_512b<int32> retVal;
    const unsigned int sel =
        lanes == 8 ? 0xFF00 : 0xFFF0; // int32 data and int32 coefs requres selection switch after 4 lanes
    const unsigned int xoffsets = xOffsets;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = xOffsets;
    const unsigned int yoffsets_hi = xOffsets;
    retVal.val = select16(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xstartUpper, yoffsets, yoffsets_hi);
    return retVal;
}

template <>
inline T_buff_512b<int16> select(T_buff_1024b<int16> xbuff,
                                 unsigned int xstart,
                                 const unsigned int xOffsets,
                                 const unsigned int xstartUpper) {
    T_buff_512b<int16> retVal;
    // select32 if int16/int16 was supported
    return retVal;
}
template <>
inline T_buff_512b<cint32> select(T_buff_1024b<cint32> xbuff,
                                  unsigned int xstart,
                                  const unsigned int xOffsets,
                                  const unsigned int xstartUpper) {
    T_buff_512b<cint32> retVal;
    const unsigned int sel = 0xF0;
    const unsigned int xoffsets = xOffsets;
    const unsigned int yoffsets = xOffsets;
    retVal.val = select8(sel, xbuff.val, xstart, xOffsets, xstartUpper, xOffsets);
    return retVal;
}
template <>
inline T_buff_512b<float> select(T_buff_1024b<float> xbuff,
                                 unsigned int xstart,
                                 const unsigned int xOffsets,
                                 const unsigned int xstartUpper) {
    T_buff_512b<float> retVal;
    const unsigned int sel = 0xFF00;
    const unsigned int xoffsets = xOffsets;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = 0;
    const unsigned int yoffsets_hi = xOffsets;
    retVal.val = fpselect16(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xstartUpper, yoffsets, yoffsets_hi);
    return retVal;
}
template <>
inline T_buff_512b<cfloat> select(T_buff_1024b<cfloat> xbuff,
                                  unsigned int xstart,
                                  const unsigned int xOffsets,
                                  const unsigned int xstartUpper) {
    T_buff_512b<cfloat> retVal;
    const unsigned int sel = 0xC;
    const unsigned int xoffsets = xOffsets;
    const unsigned int yoffsets = xOffsets;
    retVal.val = fpselect8(sel, xbuff.val, xstart, xOffsets, xstartUpper, xOffsets);
    return retVal;
}

// overloaded mul/mac calls
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DFX, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecAsym<TT_DATA, TT_COEFF> mulDecAsym1Buff(T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets,
                                                       const unsigned int xstartUpper) {
    if
        constexpr(TP_DFX == kLowDF) {
            constexpr unsigned int Lanes = fnNumLanesDecAsym<TT_DATA, TT_COEFF>();
            constexpr unsigned int Cols = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>();
            constexpr unsigned int CoeffStep = 1;
            constexpr unsigned int DataStepX = 1;
            constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            retVal.val = ::aie::sliding_mul_ops<
                Lanes, Cols, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mul(zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
            return retVal;
        }
    else if
        constexpr(TP_DFX == kHighDF) {
            using buf_type = typename T_buff_512b<TT_DATA>::v_type;
            buf_type chess_storage(X_BUFFER) tmp;
            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            const unsigned int xoffsets = decimateOffsets;
            const unsigned int xmulstart = 0;
            constexpr unsigned int CoeffStep = 1;
            constexpr unsigned int DataStepX = 1;
            constexpr unsigned int DataStepY = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>();
            T_buff_512b<TT_DATA> buff;

            buff = select(xbuff, xstart, xoffsets, xstartUpper);
            tmp = buff.val;
            retVal.val = ::aie::sliding_mul_ops<
                fnNumLanesDecAsym<TT_DATA, TT_COEFF>(), fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX,
                DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mul(zbuff.val, zstart, tmp,
                                                                                            xmulstart);
            return retVal;
        }
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DFX, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecAsym<TT_DATA, TT_COEFF> macDecAsym1Buff(T_accDecAsym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets,
                                                       const unsigned int xstartUpper) {
    if
        constexpr(TP_DFX == kLowDF) {
            const unsigned int CoeffStep = 1;
            const unsigned int DataStepX = 1;
            constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;

            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            retVal.val = ::aie::sliding_mul_ops<
                fnNumLanesDecAsym<TT_DATA, TT_COEFF>(), fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX,
                DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac(acc.val, zbuff.val, zstart,
                                                                                            xbuff.val, xstart);
            return retVal;
        }
    else if
        constexpr(TP_DFX == kHighDF) {
            using buf_type = typename T_buff_512b<TT_DATA>::v_type;
            buf_type chess_storage(X_BUFFER) tmp;
            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            const unsigned int xoffsets = decimateOffsets;
            const unsigned int xmulstart = 0;
            constexpr unsigned int CoeffStep = 1;
            constexpr unsigned int DataStepX = 1;
            constexpr unsigned int DataStepY = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(); // 4 for 4 column intrinsics

            tmp = select(xbuff, xstart, xoffsets, xstartUpper).val;
            retVal.val = ::aie::sliding_mul_ops<
                fnNumLanesDecAsym<TT_DATA, TT_COEFF>(), fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX,
                DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac(acc.val, zbuff.val, zstart,
                                                                                            tmp, xmulstart);
            return retVal;
        }
}

#endif // _DSPLIB_FIR_AIE_LLI_API_DEBUG_

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DFX, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecAsym<TT_DATA, TT_COEFF> initMacDecAsym1Buff(T_inputIF<CASC_IN_FALSE, TT_DATA> inInterface,
                                                           T_accDecAsym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets,
                                                           const unsigned int xstartUpper) {
    return mulDecAsym1Buff<TT_DATA, TT_COEFF, TP_DFX, TP_DECIMATE_FACTOR>(xbuff, xstart, zbuff, zstart, decimateOffsets,
                                                                          xstartUpper);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DFX, unsigned int TP_DECIMATE_FACTOR>
inline T_accDecAsym<TT_DATA, TT_COEFF> initMacDecAsym1Buff(T_inputIF<CASC_IN_TRUE, TT_DATA> inInterface,
                                                           T_accDecAsym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets,
                                                           const unsigned int xstartUpper) {
    return macDecAsym1Buff<TT_DATA, TT_COEFF, TP_DFX, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart,
                                                                          decimateOffsets, xstartUpper);
};
}
}
}
}
} // namespaces
#endif // _DSPLIB_FIR_DECIMATE_ASYM_UTILS_HPP_

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
