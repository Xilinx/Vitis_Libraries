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
#ifndef _DSPLIB_FIR_SR_SYM_UTILS_HPP_
#define _DSPLIB_FIR_SR_SYM_UTILS_HPP_

#ifndef INLINE_DECL
#define INLINE_DECL inline __attribute__((always_inline))
#endif

/*
Single Rate Symmetrical FIR Utilities
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
namespace sr_sym {
// Constants used to specialize optimizations associated with the center-tap (CT) operation
#define K_CT_OP_WITH_1_SAMPLE 1
#define K_CT_OP_WITH_2_SAMPLES 2
#define K_CT_OP_WITH_3_SAMPLES 3
#define K_CT_OP_WITH_4_SAMPLES 4
#define K_CT_OP_WITH_5_SAMPLES 5
#define K_CT_OP_WITH_6_SAMPLES 6
#define K_CT_OP_WITH_7_SAMPLES 7

template <typename T_D, typename T_C>
struct T_accSym : T_acc384<T_D, T_C> {
    using T_acc384<T_D, T_C>::operator=;
};

template <typename T_D, typename T_C>
struct T_outValSym : T_outVal384<T_D, T_C> {
    using T_outVal384<T_D, T_C>::operator=;
};

INLINE_DECL constexpr unsigned int fnCTColumnsLeft(unsigned int firLen, unsigned int columns) {
    // Returns number of columns left to process in the centre tap operation.
    return (firLen % (2 * columns) + 1) / 2;
};

#ifdef __X86SIM__
// #define _DSPLIB_FIR_SR_SYM_DEBUG_ 1
#endif

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_accSym<TT_DATA, TT_COEFF> macSrSym(T_accSym<TT_DATA, TT_COEFF> acc,
                                                 T_buff_1024b<TT_DATA> xbuff,
                                                 unsigned int xstart,
                                                 unsigned int ystart,
                                                 T_buff_256b<TT_COEFF> zbuff,
                                                 unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = 1;
    T_accSym<TT_DATA, TT_COEFF> retVal;

    // floats datapath doesn't have a pre-adder, need to issue 2 x non-sym calls.
    if
        constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
            retVal.val = ::aie::sliding_mac<fnNumLanesSym<TT_DATA, TT_COEFF>(), fnNumColumnsSym<TT_DATA, TT_COEFF>()>(
                acc.val, zbuff.val, zstart, xbuff.val, xstart);
            retVal.val = ::aie::sliding_mac<fnNumLanesSym<TT_DATA, TT_COEFF>(), fnNumColumnsSym<TT_DATA, TT_COEFF>()>(
                retVal.val, zbuff.val, zstart, xbuff.val, ystart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                             xbuff.val, xstart, ystart);
    }

    return retVal; // CT call will unroll all required
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL T_accSym<TT_DATA, TT_COEFF> macSrSymCT(T_accSym<TT_DATA, TT_COEFF> acc,
                                                   T_buff_1024b<TT_DATA> xbuff,
                                                   unsigned int xstart,
                                                   T_buff_256b<TT_COEFF> zbuff,
                                                   unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points =
        TP_FIR_LEN %
        (2 *
         fnNumColumnsSym<TT_DATA, TT_COEFF>()); // cascaded kernels should all get 0 CT Points apart from last in chain.
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = 1;

    T_buff_256b<TT_COEFF> zbuffInt = zbuff;                        // init with zbuff to get col 0
    T_buff_256b<TT_COEFF> zbuffIntCt = null_buff_256b<TT_COEFF>(); // init with zbuff to get col 0
    const unsigned int zstep = 1;
    const unsigned int ctCoeffPos = Points == K_CT_OP_WITH_5_SAMPLES ? 2 : 1;
    const unsigned int ctDataPos = xstart + (Points + 1) / 2 - 1;
    const unsigned int ystart = xstart + (Points)-1;
    // MAC operation always use all columns in the intrinsic.
    // For 1 and 2 column data/coeff type combos all columns always have coeff to be operated on.
    // However, >=4 column intrinsic data/coeff type combos may need to get the coeff register rearraned.
    // Center Tap coeff needs to be moved to last column and unused columns cleared.
    // Currently, max column in use == 4.
    if
        constexpr(fnNumColumnsSym<TT_DATA, TT_COEFF>() == 4) {
            if
                constexpr(Points == K_CT_OP_WITH_3_SAMPLES) {
                    zbuffInt.val.set(zbuff.val.get(zstart + (ctCoeffPos * zstep)),
                                     zstart + (3 * zstep));                          // rewrite  ct coeff to col 3
                    zbuffInt.val.set(::aie::zero<TT_COEFF>(), zstart + (1 * zstep)); // clear col 1
                    zbuffInt.val.set(::aie::zero<TT_COEFF>(), zstart + (2 * zstep)); // clear col 2
                }
            else if
                constexpr(Points == K_CT_OP_WITH_5_SAMPLES) {
                    zbuffInt.val.set(zbuff.val.get(zstart + (ctCoeffPos * zstep)),
                                     zstart + (3 * zstep));                          // rewrite  ct coeff to col 3
                    zbuffInt.val.set(::aie::zero<TT_COEFF>(), zstart + (2 * zstep)); // clear col 2
                }

            if
                constexpr(Points == K_CT_OP_WITH_3_SAMPLES) {
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (0 * zstep)), 0);
                }
            else if
                constexpr(Points == K_CT_OP_WITH_5_SAMPLES) {
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (0 * zstep)), 0);
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (1 * zstep)), 1);
                }
            else if
                constexpr(Points == K_CT_OP_WITH_7_SAMPLES) {
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (0 * zstep)), 0);
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (1 * zstep)), 1);
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (2 * zstep)), 2);
                }
            zbuffIntCt.val.set(zbuff.val.get(zstart + (ctCoeffPos * zstep)), 4); // rewrite  ct coeff to col 0
        }

    T_accSym<TT_DATA, TT_COEFF> retVal;
    if
        constexpr(Points == 0) {
            // Do nothing. No center tap
            retVal = acc;
        }
    else if
        constexpr(Points == 1) {
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsSym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val,
                                                                                                   zbuffInt.val, zstart,
                                                                                                   xbuff.val, xstart);
        }
    else if
        constexpr(Points % 2 == 1 && (std::is_same_v<TT_COEFF, int16> && std::is_same_v<TT_DATA, int16>)) {
            // Int16 data & int16 coeff combo does not offer a CT call.
            // For an odd number of points, we need to spoof it with a non-ct symmetric call followed by asym call.

            retVal.val =
                ::aie::sliding_mul_sym_ops<Lanes, fnNumColumnsSym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                           TT_COEFF, TT_DATA,
                                           tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuffIntCt.val, 0,
                                                                                        xbuff.val, xstart, ystart);
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsSym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
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
}

template <typename TT_DATA, typename TT_COEFF>
INLINE_DECL T_accSym<TT_DATA, TT_COEFF> macSrSym(T_accSym<TT_DATA, TT_COEFF> acc,
                                                 T_buff_512b<TT_DATA> xbuff,
                                                 unsigned int xstart,
                                                 T_buff_512b<TT_DATA> ybuff,
                                                 unsigned int ystart,
                                                 T_buff_256b<TT_COEFF> zbuff,
                                                 unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points = 2 * fnNumColumnsSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = 1;
    T_accSym<TT_DATA, TT_COEFF> retVal;

    // floats datapath doesn't have a pre-adder, need to issue 2 x non-sym calls.
    if
        constexpr((std::is_same_v<TT_DATA, float> || std::is_same_v<TT_DATA, cfloat>)) {
            retVal.val = ::aie::sliding_mac<fnNumLanesSym<TT_DATA, TT_COEFF>(), fnNumColumnsSym<TT_DATA, TT_COEFF>()>(
                acc.val, zbuff.val, zstart, xbuff.val, xstart);
            retVal.val = ::aie::sliding_mac<fnNumLanesSym<TT_DATA, TT_COEFF>(), fnNumColumnsSym<TT_DATA, TT_COEFF>()>(
                retVal.val, zbuff.val, zstart, ybuff.val, ystart);
        }
    else {
        retVal.val = ::aie::sliding_mul_sym_ops<Lanes, Points, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuff.val, zstart,
                                                                                             xbuff.val, xstart,
                                                                                             ybuff.val, ystart);
    }

    return retVal;
}

template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL T_accSym<TT_DATA, TT_COEFF> macSrSymCT(T_accSym<TT_DATA, TT_COEFF> acc,
                                                   T_buff_512b<TT_DATA> xbuff,
                                                   unsigned int xstart,
                                                   T_buff_512b<TT_DATA> ybuff,
                                                   unsigned int ystart,
                                                   unsigned int ct,
                                                   T_buff_256b<TT_COEFF> zbuff,
                                                   unsigned int zstart) {
    constexpr unsigned int Lanes = fnNumLanesSym<TT_DATA, TT_COEFF>();
    constexpr unsigned int Points =
        TP_FIR_LEN %
        (2 *
         fnNumColumnsSym<TT_DATA, TT_COEFF>()); // cascaded kernels should all get 0 CT Points apart from last in chain.
    constexpr unsigned int CoeffStep = 1;
    constexpr unsigned int DataStepX = 1;
    constexpr unsigned int DataStepY = 1;

    T_buff_256b<TT_COEFF> zbuffInt = zbuff;                        // init with zbuff to get col 0
    T_buff_256b<TT_COEFF> zbuffIntCt = null_buff_256b<TT_COEFF>(); // init with zbuff to get col 0
    const unsigned int zstep = 1;
    const unsigned int ctCoeffPos = Points == K_CT_OP_WITH_5_SAMPLES ? 2 : 1;
    const unsigned int ctDataPos = xstart + (Points + 1) / 2 - 1;
    // MAC operation always use all columns in the intrinsic.
    // For 1 and 2 column data/coeff type combos all columns always have coeff to be operated on.
    // However, >=4 column intrinsic data/coeff type combos may need to get the coeff register rearraned.
    // Center Tap coeff needs to be moved to last column and unused columns cleared.
    // Currently, max column in use == 4.
    if
        constexpr(fnNumColumnsSym<TT_DATA, TT_COEFF>() == 4) {
            if
                constexpr(Points == K_CT_OP_WITH_3_SAMPLES) {
                    zbuffInt.val.set(zbuff.val.get(zstart + (ctCoeffPos * zstep)),
                                     zstart + (3 * zstep));                          // rewrite  ct coeff to col 3
                    zbuffInt.val.set(::aie::zero<TT_COEFF>(), zstart + (1 * zstep)); // clear col 1
                    zbuffInt.val.set(::aie::zero<TT_COEFF>(), zstart + (2 * zstep)); // clear col 2
                }
            else if
                constexpr(Points == K_CT_OP_WITH_5_SAMPLES) {
                    zbuffInt.val.set(zbuff.val.get(zstart + (ctCoeffPos * zstep)),
                                     zstart + (3 * zstep));                          // rewrite  ct coeff to col 3
                    zbuffInt.val.set(::aie::zero<TT_COEFF>(), zstart + (2 * zstep)); // clear col 2
                }

            if
                constexpr(Points == K_CT_OP_WITH_3_SAMPLES) {
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (0 * zstep)), 0);
                }
            else if
                constexpr(Points == K_CT_OP_WITH_5_SAMPLES) {
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (0 * zstep)), 0);
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (1 * zstep)), 1);
                }
            else if
                constexpr(Points == K_CT_OP_WITH_7_SAMPLES) {
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (0 * zstep)), 0);
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (1 * zstep)), 1);
                    zbuffIntCt.val.set(zbuff.val.get(zstart + (2 * zstep)), 2);
                }
            zbuffIntCt.val.set(zbuff.val.get(zstart + (ctCoeffPos * zstep)), 4); // rewrite  ct coeff to col 0
        }

    T_accSym<TT_DATA, TT_COEFF> retVal;
    if
        constexpr(Points == 0) {
            // Do nothing. No center tap
            retVal = acc;
        }
    else if
        constexpr(Points == 1) {
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsSym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                       TT_COEFF, TT_DATA, tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val,
                                                                                                   zbuffInt.val, zstart,
                                                                                                   xbuff.val, xstart);
        }
    else if
        constexpr(Points % 2 == 1 && (std::is_same_v<TT_COEFF, int16> && std::is_same_v<TT_DATA, int16>)) {
            // Int16 data & int16 coeff combo does not offer a CT call.
            // For an odd number of points, we need to spoof it with a non-ct symmetric call followed by asym call.

            retVal.val =
                ::aie::sliding_mul_sym_ops<Lanes, fnNumColumnsSym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
                                           TT_COEFF, TT_DATA,
                                           tAccBaseType_t<TT_DATA, TT_COEFF> >::mac_sym(acc.val, zbuffIntCt.val, 0,
                                                                                        xbuff.val, xstart, ybuff.val,
                                                                                        ystart);
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, fnNumColumnsSym<TT_DATA, TT_COEFF>(), CoeffStep, DataStepX, DataStepY,
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
}

// Initial MAC/MUL operation. 1Buff
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL T_accSym<TT_DATA, TT_COEFF> initMacSrSym(T_accSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_1024b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     unsigned int ystart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart) {
    return macSrSym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, ystart, zbuff, zstart);
};

// Initial MAC/MUL operation. 2Buff
template <typename TT_DATA, typename TT_COEFF, unsigned int TP_FIR_LEN>
INLINE_DECL T_accSym<TT_DATA, TT_COEFF> initMacSrSym(T_accSym<TT_DATA, TT_COEFF> acc,
                                                     T_buff_512b<TT_DATA> xbuff,
                                                     unsigned int xstart,
                                                     T_buff_512b<TT_DATA> ybuff,
                                                     unsigned int ystart,
                                                     T_buff_256b<TT_COEFF> zbuff,
                                                     unsigned int zstart) {
    return macSrSym<TT_DATA, TT_COEFF>(acc, xbuff, xstart, ybuff, ystart, zbuff, zstart);
};
}
}
}
}
}

#endif // _DSPLIB_FIR_SR_SYM_UTILS_HPP_
