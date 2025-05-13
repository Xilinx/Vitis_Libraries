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
#ifndef _DSPLIB_FIR_DECIMATE_SYM_UTILS_HPP_
#define _DSPLIB_FIR_DECIMATE_SYM_UTILS_HPP_

/*
Symmetrical Decimation FIR Utilities
This file contains sets of overloaded, templatized and specialized templatized functions for use
by the main kernel class and run-time function. These functions are separate from the traits file
because they are purely for kernel use, not graph level compilation.
*/

#include <stdio.h>
#include <adf.h>
#include "fir_decimate_sym.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fir {
namespace decimate_sym {

template <typename TT_DATA, typename TT_COEFF>
struct T_accDecSym : T_acc384<TT_DATA, TT_COEFF> {
    using T_acc384<TT_DATA, TT_COEFF>::operator=;
};

template <typename TT_DATA, typename TT_COEFF>
struct T_outValDecSym : T_outVal384<TT_DATA, TT_COEFF> {
    using T_outVal384<TT_DATA, TT_COEFF>::operator=;
};

#ifndef L_BUFFER
#define L_BUFFER xa
#endif
#ifndef R_BUFFER
#define R_BUFFER xb
#endif
#ifndef Y_BUFFER
#define Y_BUFFER ya
#endif
#ifndef Z_BUFFER
#define Z_BUFFER wc0
#endif

//---------------------------------------------------------------------------------------------------
// Functions

template <typename TT_DATA, typename TT_COEFF, unsigned int T_SIZE>
INLINE_DECL void fnLoadXIpData(T_buff_1024b<TT_DATA>& buff, const unsigned int splice, auto& inItr) {
    constexpr short kSpliceRange = T_SIZE == 256 ? 4 : 8;
    constexpr int kVectSize = T_SIZE / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVectSize>;
    t_vect* readPtr = (t_vect*)&*inItr;
    t_vect readVal = *readPtr;
    inItr += kVectSize;
    buff.val.insert(splice % kSpliceRange, readVal);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int T_SIZE>
INLINE_DECL void fnLoadXIpData(T_buff_512b<TT_DATA>& buff, unsigned int splice, auto& inItr) {
    constexpr short kSpliceRange = T_SIZE == 256 ? 2 : 4;
    constexpr int kVectSize = T_SIZE / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVectSize>;
    t_vect* readPtr = (t_vect*)&*inItr;
    t_vect readVal = *readPtr;
    inItr += kVectSize;
    buff.val.insert(splice % kSpliceRange, readVal);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int T_SIZE>
INLINE_DECL void fnLoadYIpData(T_buff_512b<TT_DATA>& buff, unsigned int splice, auto& inItr) {
    constexpr short kSpliceRange = T_SIZE == 256 ? 2 : 4;
    constexpr int kVectSize = T_SIZE / 8 / sizeof(TT_DATA);
    using t_vect = ::aie::vector<TT_DATA, kVectSize>;
    t_vect* readPtr = (t_vect*)&*inItr;
    t_vect readVal = *readPtr;
    inItr -= kVectSize;
    buff.val.insert(splice % kSpliceRange, readVal);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                          T_buff_1024b<TT_DATA> xbuff,
                                                          unsigned int xstart,
                                                          unsigned int ystart,
                                                          T_buff_256b<TT_COEFF> zbuff,
                                                          unsigned int zstart,
                                                          const unsigned int decimateOffsets) {
    // API Sliding_mul_sym unrolls all the required MAC calls for the 1 buff arch.
    // There's nothing left to do when this function is called.
    // Not used, retained for ompatibilty with non-API low level intrinsic mode.

    // Do nothing here
    return acc;
    // constexpr unsigned int Lanes             = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    // constexpr unsigned int Points            = 2*fnNumColumnsDecSym<TT_DATA, TT_COEFF>();
    // constexpr unsigned int CoeffStep         = 1;
    // constexpr unsigned int DataStepX         = 1;
    // constexpr unsigned int DataStepY         = TP_DECIMATE_FACTOR;
    // T_accDecSym<TT_DATA, TT_COEFF> retVal;
    // retVal.val = ::aie::sliding_mul_sym_ops<Lanes,
    //                                 Points,
    //                                 CoeffStep, DataStepX, DataStepY,
    //                                 TT_COEFF, TT_DATA,
    //                                 tAccBaseType_t<TT_DATA,TT_COEFF>
    //                                 >::mac_sym
    //                 (acc.val, zbuff.val, zstart, xbuff.val, xstart, ystart);
    // return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                            T_buff_1024b<TT_DATA> xbuff,
                                                            unsigned int xstart,
                                                            unsigned int ystart,
                                                            unsigned int ct,
                                                            T_buff_256b<TT_COEFF> zbuff,
                                                            unsigned int zstart,
                                                            const unsigned int decimateOffsets) {
    // API Sliding_mul_sym unrolls all the required MAC calls for the 1 buff arch.
    // There's nothing left to do when this function is called.
    // Not used, retained for ompatibilty with non-API low level intrinsic mode.

    // Do nothing here
    return acc;

    //     constexpr unsigned int Lanes             = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    //     constexpr unsigned int Points            = 2*fnNumColumnsDecSym<TT_DATA, TT_COEFF>()-1;
    //     constexpr unsigned int CoeffStep         = 1;
    //     constexpr unsigned int DataStepX         = 1;
    //     constexpr unsigned int DataStepY         = TP_DECIMATE_FACTOR;
    //     // constexpr unsigned int DataStepY         = 1;

    //     T_accDecSym<TT_DATA, TT_COEFF> retVal;
    //     if constexpr (Points == 1){
    //         retVal.val = ::aie::sliding_mul_ops<Lanes,
    //                                     Points,
    //                                     CoeffStep, DataStepX, DataStepY,
    //                                     TT_COEFF, TT_DATA,
    //                                     tAccBaseType_t<TT_DATA,TT_COEFF>
    //                                     >::mac
    //                     (acc.val, zbuff.val, zstart, xbuff.val, xstart);
    //     } else {
    //         retVal.val = ::aie::sliding_mul_sym_ops<Lanes,
    //                                     Points,
    //                                     CoeffStep, DataStepX, DataStepY,
    //                                     TT_COEFF, TT_DATA,
    //                                     tAccBaseType_t<TT_DATA,TT_COEFF>
    //                                     >::mac_sym
    //                     (acc.val, zbuff.val, zstart, xbuff.val, xstart);
    //     }
    //     return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                          T_buff_1024b<TT_DATA> xbuff,
                                                          unsigned int xstart,
                                                          T_buff_256b<TT_COEFF> zbuff,
                                                          unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = TP_FIR_LEN;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
    T_accDecSym<TT_DATA, TT_COEFF> retVal;

    // #define _DSPLIB_FIR_DEC_SYM_DEBUG_ 1

    retVal.val =
        ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                   tAccBaseType_t<TT_DATA, TT_COEFF> >::mul_sym(zbuff.val, zstart, xbuff.val, xstart);
    // (acc.val, zbuff.val, zstart, xbuff.val, xstart);

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> macDecSym1Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                            T_buff_1024b<TT_DATA> xbuff,
                                                            unsigned int xstart,
                                                            T_buff_256b<TT_COEFF> zbuff,
                                                            unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = TP_FIR_LEN;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
    T_accDecSym<TT_DATA, TT_COEFF> retVal;

    // #define _DSPLIB_FIR_DEC_SYM_DEBUG_ 1

    if
        constexpr(Points == 1) {
            retVal.val = ::aie::sliding_mul_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val, zbuff.val, zstart,
                                                                                         xbuff.val, xstart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                             xbuff.val, xstart);
    }

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> macDecSym2Buff(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                          T_buff_512b<TT_DATA> xbuff,
                                                          unsigned int xstart,
                                                          T_buff_512b<TT_DATA> ybuff,
                                                          unsigned int ystart,
                                                          T_buff_256b<TT_COEFF> zbuff,
                                                          unsigned int zstart,
                                                          const unsigned int decimateOffsets) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;
    T_accDecSym<TT_DATA, TT_COEFF> retVal;

    retVal.val =
        ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                   tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart, xbuff.val,
                                                                                xstart, ybuff.val, ystart);

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> macDecSym2Buffct(T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                            T_buff_512b<TT_DATA> xbuff,
                                                            unsigned int xstart,
                                                            T_buff_512b<TT_DATA> ybuff,
                                                            unsigned int ystart,
                                                            unsigned int ct,
                                                            T_buff_256b<TT_COEFF> zbuff,
                                                            unsigned int zstart,
                                                            const unsigned int decimateOffsets) {
    constexpr unsigned int Lanes = fnNumLanesDecSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsDecSym<TT_DATA, TT_COEFF>() - 1;
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = TP_DECIMATE_FACTOR;

    T_accDecSym<TT_DATA, TT_COEFF> retVal;
    if
        constexpr(Points == 1) {
            retVal.val = ::aie::sliding_mul_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val, zbuff.val, zstart,
                                                                                         xbuff.val, xstart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                             xbuff.val, xstart,
                                                                                             ybuff.val, ystart);
    }
    return retVal;
}

// Initial MAC/MUL operation. Take inputIF as an argument to ease overloading.
template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buff(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                              T_buff_1024b<TT_DATA> xbuff,
                                                              unsigned int xstart,
                                                              unsigned int ystart,
                                                              T_buff_256b<TT_COEFF> zbuff,
                                                              unsigned int zstart,
                                                              const unsigned int decimateOffsets) {
    return macDecSym1Buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
    // return macDecSym1Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, zbuff, zstart,
    // decimateOffsets);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buff(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                              T_buff_1024b<TT_DATA> xbuff,
                                                              unsigned int xstart,
                                                              unsigned int ystart,
                                                              T_buff_256b<TT_COEFF> zbuff,
                                                              unsigned int zstart,
                                                              const unsigned int decimateOffsets) {
    return macDecSym1Buff<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
    // return macDecSym1Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, zbuff, zstart,
    // decimateOffsets);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buffct(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accDecSym<TT_DATA, TT_COEFF> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    const unsigned int decimateOffsets) {
    return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
    // return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, ct, zbuff, zstart,
    // decimateOffsets);
};

template <typename TT_DATA,
          typename TT_COEFF,
          unsigned int TP_FIR_LEN,
          unsigned int TP_DECIMATE_FACTOR,
          unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym1Buffct(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accDecSym<TT_DATA, TT_COEFF> acc,
    T_buff_1024b<TT_DATA> xbuff,
    unsigned int xstart,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    const unsigned int decimateOffsets) {
    return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_FIR_LEN, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, zbuff, zstart);
    // return macDecSym1Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ystart, ct, zbuff, zstart,
    // decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buff(T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                              T_buff_512b<TT_DATA> xbuff,
                                                              unsigned int xstart,
                                                              T_buff_512b<TT_DATA> ybuff,
                                                              unsigned int ystart,
                                                              T_buff_256b<TT_COEFF> zbuff,
                                                              unsigned int zstart,
                                                              const unsigned int decimateOffsets) {
    return macDecSym2Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart,
                                                                 decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buff(T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
                                                              T_accDecSym<TT_DATA, TT_COEFF> acc,
                                                              T_buff_512b<TT_DATA> xbuff,
                                                              unsigned int xstart,
                                                              T_buff_512b<TT_DATA> ybuff,
                                                              unsigned int ystart,
                                                              T_buff_256b<TT_COEFF> zbuff,
                                                              unsigned int zstart,
                                                              const unsigned int decimateOffsets) {
    return macDecSym2Buff<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart,
                                                                 decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buffct(
    T_inputIF<CASC_IN_FALSE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accDecSym<TT_DATA, TT_COEFF> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    const unsigned int decimateOffsets) {
    return macDecSym2Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, ct, zbuff, zstart,
                                                                   decimateOffsets);
};

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_DECIMATE_FACTOR, unsigned int TP_DUAL_IP>
INLINE_DECL T_accDecSym<TT_DATA, TT_COEFF> initMacDecSym2Buffct(
    T_inputIF<CASC_IN_TRUE, TT_DATA, TP_DUAL_IP> inInterface,
    T_accDecSym<TT_DATA, TT_COEFF> acc,
    T_buff_512b<TT_DATA> xbuff,
    unsigned int xstart,
    T_buff_512b<TT_DATA> ybuff,
    unsigned int ystart,
    unsigned int ct,
    T_buff_256b<TT_COEFF> zbuff,
    unsigned int zstart,
    const unsigned int decimateOffsets) {
    return macDecSym2Buffct<TT_DATA, TT_COEFF, TP_DECIMATE_FACTOR>(acc, xbuff, xstart, ybuff, ystart, ct, zbuff, zstart,
                                                                   decimateOffsets);
};

// Shift and Saturate Decimation Sym
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal384<TT_DATA, TT_COEFF> shiftAndSaturateDecSym(T_acc384<TT_DATA, TT_COEFF> acc, const int shift) {
    return shiftAndSaturate(acc, shift);
};

// Shift and Saturate Decimation Sym
template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_outVal<TT_DATA, TT_COEFF> shiftAndSaturateDecSym(T_acc<TT_DATA, TT_COEFF> acc, const int shift) {
    return shiftAndSaturate(acc, shift);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_DECIMATE_SYM_UTILS_HPP_
