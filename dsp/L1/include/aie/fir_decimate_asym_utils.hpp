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
struct T_accDecAsym : T_acc384<T_D, T_C> {
    using T_acc384<T_D, T_C>::operator=;
};

template <typename T_D, typename T_C>
struct T_outValDecAsym : T_outVal384<T_D, T_C> {
    using T_outVal384<T_D, T_C>::operator=;
};

//---------------------------------------------------------------------------------------------------
// Functions

// Overloaded shift and saturate calls to allow null operation for float types
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal384<TT_DATA, TT_COEFF> shiftAndSaturateDecAsym(const T_acc384<TT_DATA, TT_COEFF> acc,
                                                                   const int shift) {
    // generic 384-bit wide acc shift and saturate
    return shiftAndSaturate(acc, shift);
};

//
template <typename TT_DATA, unsigned int T_SIZE>
INLINE_DECL void fnLoadXIpData(T_buff_1024b<TT_DATA>& buff,
                               const unsigned int splice,
                               input_window<TT_DATA>* inWindow) {
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

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA>
INLINE_DECL T_buff_512b<TT_DATA> select(const unsigned int sel,
                                        T_buff_1024b<TT_DATA> xbuff,
                                        unsigned int xstart,
                                        const unsigned int xOffsets,
                                        const unsigned int xstartUpper) {
    T_buff_512b<TT_DATA> retVal;
    const unsigned int xoffsets = xOffsets;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = xOffsets;
    const unsigned int yoffsets_hi = xOffsets;
    retVal.val = select16(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xstartUpper, yoffsets, yoffsets_hi);
    return retVal;
}

//-----------------------------------------------------------------------------------------------------
template <>
INLINE_DECL T_buff_512b<int32> select(const unsigned int sel,
                                      T_buff_1024b<int32> xbuff,
                                      unsigned int xstart,
                                      const unsigned int xOffsets,
                                      const unsigned int xstartUpper) {
    T_buff_512b<int32> retVal;
    const unsigned int xoffsets = xOffsets;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = xOffsets;
    const unsigned int yoffsets_hi = xOffsets;
    retVal.val = select16(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xstartUpper, yoffsets, yoffsets_hi);
    return retVal;
}

template <>
INLINE_DECL T_buff_512b<int16> select(const unsigned int sel,
                                      T_buff_1024b<int16> xbuff,
                                      unsigned int xstart,
                                      const unsigned int xOffsets,
                                      const unsigned int xstartUpper) {
    T_buff_512b<int16> retVal;
    // select32 if int16/int16 was supported
    return retVal;
}
template <>
INLINE_DECL T_buff_512b<cint32> select(const unsigned int sel,
                                       T_buff_1024b<cint32> xbuff,
                                       unsigned int xstart,
                                       const unsigned int xOffsets,
                                       const unsigned int xstartUpper) {
    T_buff_512b<cint32> retVal;
    const unsigned int xoffsets = xOffsets;
    const unsigned int yoffsets = xOffsets;
    retVal.val = select8(sel, xbuff.val, xstart, xOffsets, xstartUpper, xOffsets);
    return retVal;
}
template <>
INLINE_DECL T_buff_512b<float> select(const unsigned int sel,
                                      T_buff_1024b<float> xbuff,
                                      unsigned int xstart,
                                      const unsigned int xOffsets,
                                      const unsigned int xstartUpper) {
    T_buff_512b<float> retVal;
    const unsigned int xoffsets = xOffsets;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = 0;
    const unsigned int yoffsets_hi = xOffsets;
    retVal.val = fpselect16(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xstartUpper, yoffsets, yoffsets_hi);
    return retVal;
}
template <>
INLINE_DECL T_buff_512b<cfloat> select(const unsigned int sel,
                                       T_buff_1024b<cfloat> xbuff,
                                       unsigned int xstart,
                                       const unsigned int xOffsets,
                                       const unsigned int xstartUpper) {
    T_buff_512b<cfloat> retVal;
    const unsigned int selInt = 0xC;
    const unsigned int xoffsets = xOffsets;
    const unsigned int yoffsets = xOffsets;
    retVal.val = fpselect8(selInt, xbuff.val, xstart, xOffsets, xstartUpper, xOffsets);
    return retVal;
}

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int Lanes, unsigned int Cols>
INLINE_DECL T_buff_512b<TT_DATA> select(T_buff_1024b<TT_DATA> xbuff,
                                        unsigned int xstart,
                                        const unsigned int xOffsets,
                                        const unsigned int xstartUpper) {
    T_buff_512b<TT_DATA> retVal;
    // Preselect data buffer in such way that xoffset limits are avoided.
    // Data buffer can hold 1024-bit, which maybe up to 32 samples, but xoffsets are only 4-bit (3-bit for floats).
    // Samples from 1-24-bit buffer that are offset by decimate_factor are placed in adjacent position in the temp
    // 512-bit buffer and can be indexed nicely.
    const unsigned int selAddr = Cols * Lanes / 2;                                // lanes 2 to 8.
    const unsigned int sel = selAddr == 2 ? 0xC : selAddr == 4 ? 0xFFF0 : 0xFF00; // lanes 2 to 8.

    retVal = select(sel, xbuff, xstart, xOffsets, xstartUpper);
    return retVal;
}

// overloaded mul/mac calls
//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DFX, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecAsym<TT_DATA, TT_COEFF> mulDecAsym(T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets,
                                                       const unsigned int xstartUpper) {
    constexpr unsigned int Lanes = fnNumLanesDecAsym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Cols = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    if
        constexpr(TP_DFX == kLowDF) {
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
            constexpr unsigned int DataStepY = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>();
            T_buff_512b<TT_DATA> buff;
            buff = select<TT_DATA, Lanes, Cols>(xbuff, xstart, xoffsets, xstartUpper);
            tmp = buff.val;
            retVal.val = ::aie::sliding_mul_ops<
                Lanes, Cols, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mul(zbuff.val, zstart, tmp,
                                                                                            xmulstart);
            return retVal;
        }
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DFX, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecAsym<TT_DATA, TT_COEFF> macDecAsym(T_accDecAsym<TT_DATA, TT_COEFF> acc,
                                                       T_buff_1024b<TT_DATA> xbuff,
                                                       unsigned int xstart,
                                                       T_buff_256b<TT_COEFF> zbuff,
                                                       unsigned int zstart,
                                                       const unsigned int decimateOffsets,
                                                       const unsigned int xstartUpper) {
    constexpr unsigned int Lanes = fnNumLanesDecAsym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Cols = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    if
        constexpr(TP_DFX == kLowDF) {
            constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;

            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            retVal.val = ::aie::sliding_mul_ops<
                Lanes, Cols, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
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
            constexpr unsigned int DataStepY = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(); // 4 for 4 column intrinsics
            T_buff_512b<TT_DATA> buff;

            buff = select<TT_DATA, Lanes, Cols>(xbuff, xstart, xoffsets, xstartUpper);
            tmp = buff.val;
            retVal.val = ::aie::sliding_mul_ops<
                fnNumLanesDecAsym<TT_DATA, TT_COEFF>(), fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX,
                DataStepY, TT_COEFF, TT_DATA,
                accClassTag_t<fnAccClass<TT_DATA>(), fnAccSize<TT_DATA, TT_COEFF>()> >::mac(acc.val, zbuff.val, zstart,
                                                                                            tmp, xmulstart);
            return retVal;
        }
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_DFX,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecAsym<TT_DATA, TT_COEFF> initMacDecAsym(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecAsym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets,
                                                           const unsigned int xstartUpper) {
    return mulDecAsym<TT_DATA, TT_COEFF, TP_DFX, TP_DECIMATE_FACTOR>(xbuff, xstart, zbuff, zstart, decimateOffsets,
                                                                     xstartUpper);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_DFX,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecAsym<TT_DATA, TT_COEFF> initMacDecAsym(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                           T_accDecAsym<TT_DATA, TT_COEFF> acc,
                                                           T_buff_1024b<TT_DATA> xbuff,
                                                           unsigned int xstart,
                                                           T_buff_256b<TT_COEFF> zbuff,
                                                           unsigned int zstart,
                                                           const unsigned int decimateOffsets,
                                                           const unsigned int xstartUpper) {
    return macDecAsym<TT_DATA, TT_COEFF, TP_DFX, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart, decimateOffsets,
                                                                     xstartUpper);
};
}
}
}
}
} // namespaces
#endif // _DSPLIB_FIR_DECIMATE_ASYM_UTILS_HPP_
