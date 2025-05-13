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
INLINE_DECL void fnLoadXIpData(T_buff_1024b<TT_DATA>& buff, const unsigned int splice, auto& inItr) {
    if
        constexpr(T_SIZE == 256) { upd_win_incr_256b<TT_DATA>(buff, splice, inItr); }
    else {
        upd_win_incr_128b<TT_DATA>(buff, splice, inItr);
    }
};

#if (__HAS_ACCUM_PERMUTES__ == 1)

#define CHESS_STORAGE_X_BUFF chess_storage(X_BUFFER)

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
    const unsigned int xoffsets = xOffsets << 1;
    const unsigned int xoffsets_hi = 0;
    const unsigned int yoffsets = xOffsets << 1;
    const unsigned int yoffsets_hi = xOffsets << 1;
    const unsigned int xsquare = 0x3210;
    const unsigned int ysquare = 0x3210;
    // Due to xsqure limitations (only able to offset 0x3 samples), int16 data is not able to use pre-select.

    retVal.val =
        select32(sel, xbuff.val, xstart, xoffsets, xoffsets_hi, xsquare, xstartUpper, yoffsets, yoffsets_hi, ysquare);
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
    const unsigned int sel = selAddr == 2 ? 0xC : selAddr == 4 ? 0xFFF0 : 0xFF00; // lanes 4 to 8.

    retVal = select(sel, xbuff, xstart, xOffsets, xstartUpper);
    return retVal;
}

#else
// clear out constraint
#define CHESS_STORAGE_X_BUFF

//-----------------------------------------------------------------------------------------------------
template <typename TT_DATA, unsigned int Lanes, unsigned int Cols>
INLINE_DECL T_buff_512b<TT_DATA> select(T_buff_1024b<TT_DATA> xbuff,
                                        unsigned int xstart,
                                        const unsigned int xOffsets,
                                        const unsigned int xstartUpper) {
    T_buff_512b<TT_DATA> retVal;
    return retVal;
}
#endif // (__HAS_ACCUM_PERMUTES__ == 1)

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
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, Cols, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                       tAccBaseType_t<TT_DATA, TT_COEFF> >::mul(zbuff.val, zstart, xbuff.val, xstart);
            return retVal;
        }
    else if
        constexpr(TP_DFX == kHighDF) {
            using buf_type = typename T_buff_512b<TT_DATA>::v_type;
            // buf_type tmp;
            buf_type CHESS_STORAGE_X_BUFF tmp;
            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            const unsigned int xoffsets = decimateOffsets;
            const unsigned int xmulstart = 0;
            constexpr unsigned int DataStepY = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>();
            T_buff_512b<TT_DATA> buff;
            buff = select<TT_DATA, Lanes, Cols>(xbuff, xstart, xoffsets, xstartUpper);
            tmp = buff.val;
            retVal.val =
                ::aie::sliding_mul_ops<Lanes, Cols, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                       tAccBaseType_t<TT_DATA, TT_COEFF> >::mul(zbuff.val, zstart, tmp, xmulstart);
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
            retVal.val = ::aie::sliding_mul_ops<Lanes, Cols, CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                                tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val, zbuff.val, zstart,
                                                                                         xbuff.val, xstart);
            return retVal;
        }
    else if
        constexpr(TP_DFX == kHighDF) {
            using buf_type = typename T_buff_512b<TT_DATA>::v_type;
            // buf_type tmp;
            buf_type CHESS_STORAGE_X_BUFF tmp;
            T_accDecAsym<TT_DATA, TT_COEFF> retVal;
            const unsigned int xoffsets = decimateOffsets;
            const unsigned int xmulstart = 0;
            constexpr unsigned int DataStepY = fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(); // 4 for 4 column intrinsics
            T_buff_512b<TT_DATA> buff;

            buff = select<TT_DATA, Lanes, Cols>(xbuff, xstart, xoffsets, xstartUpper);
            tmp = buff.val;
            retVal.val =
                ::aie::sliding_mul_ops<fnNumLanesDecAsym<TT_DATA, TT_COEFF>(), fnNumColumnsDecAsym<TT_DATA, TT_COEFF>(),
                                       CoeffStep, DataStepX, DataStepY, TT_COEFF, TT_DATA,
                                       tAccBaseType_t<TT_DATA, TT_COEFF> >::mac(acc.val, zbuff.val, zstart, tmp,
                                                                                xmulstart);
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

template <unsigned int offset, unsigned int kParallelPhases>
INLINE_DECL constexpr std::array<unsigned int, kParallelPhases> fnPhaseStartOffsets() {
    std::array<unsigned int, kParallelPhases> ret = {};
    for (int phase = 0; phase < kParallelPhases; ++phase) {
        ret[phase] = offset == 0 ? 0 : phase >= offset ? 1 : 0;
    }
    return ret;
};
// template<unsigned int phase, unsigned int kParallelPhases>
INLINE_DECL constexpr unsigned int fnCoeffPhase(unsigned int phase, unsigned int kParallelPhases) {
    return (kParallelPhases - phase) % kParallelPhases;
};

// Deinterleave
// Read 256-bits per phase.
template <bool TP_CASC_IN, typename TT_DATA, unsigned int TP_DUAL_IP, unsigned int TP_NUM_INPUTS>
INLINE_DECL void streamLoadAndDeinterleave(std::array<T_buff_1024b<TT_DATA>, TP_NUM_INPUTS>& sbuffArray,
                                           T_inputIF<TP_CASC_IN, TT_DATA, TP_DUAL_IP> inInterface,
                                           unsigned int splice,
                                           unsigned int phaseOffset) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr int kSamplesIn256b = kSamplesIn128b * 2;
    constexpr int kSamplesIn512b = kSamplesIn128b * 4;
    constexpr int kSamplesIn1024b = kSamplesIn128b * 8;
    if
        constexpr(TP_NUM_INPUTS == 1) { readStream256(sbuffArray[0], splice, inInterface); }
    else if
        constexpr(TP_NUM_INPUTS == 2 || TP_NUM_INPUTS == 3 || TP_NUM_INPUTS == 4 || TP_NUM_INPUTS == 5 ||
                  TP_NUM_INPUTS == 6 || TP_NUM_INPUTS == 7) {
            constexpr int kSamplesInVec = T_buff_256b<TT_DATA>::getLanes();

            if
                constexpr(std::is_same<TT_DATA, int16>::value) {
                    std::array<T_buff_1024b<int32>, TP_NUM_INPUTS> sbuffArrayInt32;
                    std::array<T_buff_256b<TT_DATA>, TP_NUM_INPUTS> streamReadData;
                    constexpr int int32Toint16Ratio = 2;

#pragma unroll(TP_NUM_INPUTS)
                    for (int j = 0; j < (TP_NUM_INPUTS); j++) {
                        // initialize
                        sbuffArrayInt32[j].val = ::aie::vector_cast<int32>(sbuffArray[j].val);
                    }
#pragma unroll(TP_NUM_INPUTS)
                    for (int j = 0; j < (TP_NUM_INPUTS); j++) {
                        readStream256(streamReadData[j], 0, inInterface);
                    }
#pragma unroll(TP_NUM_INPUTS* kSamplesInVec / int32Toint16Ratio)
                    for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec / int32Toint16Ratio); j++) {
                        // Read scalar, as there's no efficient vector deinterleve instruction to operate on odd number
                        // of output vectors
                        int buffIndex0 =
                            ((2 * j / kSamplesInVec + (2 * j) * TP_NUM_INPUTS) / kSamplesInVec) % TP_NUM_INPUTS;
                        int sampleIndex0 = (2 * j / kSamplesInVec + 2 * j * TP_NUM_INPUTS) % kSamplesInVec;
                        int16 readSample0 = streamReadData[buffIndex0].val.get(sampleIndex0);

                        int buffIndex1 = (((2 * j + 1) / kSamplesInVec + (2 * j + 1) * TP_NUM_INPUTS) / kSamplesInVec) %
                                         TP_NUM_INPUTS;
                        int sampleIndex1 = ((2 * j + 1) / kSamplesInVec + (2 * j + 1) * TP_NUM_INPUTS) % kSamplesInVec;
                        int16 readSample1 = streamReadData[buffIndex1].val.get(sampleIndex1);

                        unsigned int writeSampleIdx =
                            (splice * kSamplesIn256b / int32Toint16Ratio + j % (kSamplesInVec / int32Toint16Ratio)) %
                            (kSamplesIn1024b / int32Toint16Ratio);
                        unsigned int writeBuffIdx = (phaseOffset + ((2 * j + 0)) / kSamplesInVec) % TP_NUM_INPUTS;
                        int32 writeInt32Sample = (readSample1 << 16) | (0xFFFF & readSample0);

                        sbuffArrayInt32[writeBuffIdx].val.set(writeInt32Sample, (writeSampleIdx));
                    }

#pragma unroll(TP_NUM_INPUTS)
                    for (int j = 0; j < (TP_NUM_INPUTS); j++) {
                        // initialize
                        sbuffArray[j].val = ::aie::vector_cast<int16>(sbuffArrayInt32[j].val);
                    }
                }
            else {
// 32-bit or 64-but types

#pragma unroll(TP_NUM_INPUTS* kSamplesInVec)
                for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec); j++) {
                    // Read scalar, as there's no efficient vector deinterleve instruction to operate on odd number of
                    // output vectors
                    TT_DATA readSample = readincr(inInterface.inStream);

                    unsigned int sampleIdx = (splice * kSamplesIn256b + j / TP_NUM_INPUTS) % kSamplesIn1024b;

                    sbuffArray[(phaseOffset + j) % TP_NUM_INPUTS].val.set(readSample, sampleIdx);
                }
            }
        }
};

// Deinterleave
// Read 256-bits per phase.
template <typename TT_DATA, unsigned int TP_NUM_INPUTS>
INLINE_DECL void bufferLoadAndDeinterleave(std::array<T_buff_1024b<TT_DATA>, TP_NUM_INPUTS>& sbuffArray,
                                           auto& inItr,
                                           unsigned int splice,
                                           unsigned int phaseOffset) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr int kSamplesIn256b = kSamplesIn128b * 2;
    constexpr int kSamplesIn512b = kSamplesIn128b * 4;
    using t512v = ::aie::vector<TT_DATA, kSamplesIn512b>;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t128v = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using vec = typename T_buff_512b<TT_DATA>::v_type;
    if
        constexpr(TP_NUM_INPUTS == 1) { upd_win_incr_256b<TT_DATA>(sbuffArray[0], splice, inItr); }
    else if
        constexpr(TP_NUM_INPUTS == 2) {
            constexpr int kSamplesInVec = T_buff_512b<TT_DATA>::getLanes();
            vec writeValLoc;
            upd_win_incr_256b<TT_DATA>(writeValLoc, 0, inItr);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 1, inItr);
            ::std::pair<t256v, t256v> inIntlv =
                ::aie::interleave_unzip(writeValLoc.template extract<kSamplesInVec / 2>(0),
                                        writeValLoc.template extract<kSamplesInVec / 2>(1), 1);
            sbuffArray[0].val.insert(splice % 4, inIntlv.first);
            sbuffArray[1].val.insert(splice % 4, inIntlv.second);
        }
    else if
        constexpr(TP_NUM_INPUTS == 3 || TP_NUM_INPUTS == 5 || TP_NUM_INPUTS == 6 || TP_NUM_INPUTS == 7) {
            constexpr int kSamplesInVec = T_buff_256b<TT_DATA>::getLanes();

#pragma unroll(TP_NUM_INPUTS* kSamplesInVec)
            for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec); j++) {
                // Read scalar, as there's no efficient vector deinterleve instruction to operate on odd number of
                // output vectors
                TT_DATA readSample;
                upd_win_incr_sample(readSample, inItr);

                sbuffArray[(phaseOffset + j) % TP_NUM_INPUTS].val.set(
                    readSample, (splice * kSamplesIn256b + j / TP_NUM_INPUTS) % T_buff_1024b<TT_DATA>::getLanes());
            }

            // Interleave 4 to 1
        }
    else if
        constexpr(TP_NUM_INPUTS == 4) {
            constexpr int kSamplesInVec = T_buff_512b<TT_DATA>::getLanes();
            vec writeValLoc;
            upd_win_incr_256b<TT_DATA>(writeValLoc, 0, inItr);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 1, inItr);
            ::std::pair<t256v, t256v> inIntlv0 =
                ::aie::interleave_unzip(writeValLoc.template extract<kSamplesInVec / 2>(0),
                                        writeValLoc.template extract<kSamplesInVec / 2>(1), 1);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 0, inItr);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 1, inItr);
            ::std::pair<t256v, t256v> inIntlv1 =
                ::aie::interleave_unzip(writeValLoc.template extract<kSamplesInVec / 2>(0),
                                        writeValLoc.template extract<kSamplesInVec / 2>(1), 1);
            ::std::pair<t256v, t256v> inIntlv2 = ::aie::interleave_unzip(inIntlv0.first, inIntlv1.first, 1);
            sbuffArray[0].val.insert(splice % 4, inIntlv2.first);
            sbuffArray[2].val.insert(splice % 4, inIntlv2.second);
            ::std::pair<t256v, t256v> inIntlv3 = ::aie::interleave_unzip(inIntlv0.second, inIntlv1.second, 1);
            sbuffArray[1].val.insert(splice % 4, inIntlv3.first);
            sbuffArray[3].val.insert(splice % 4, inIntlv3.second);
        }
};

// Deinterleave
// Read 256-bits per phase.
template <typename TT_DATA, unsigned int TP_NUM_INPUTS>
INLINE_DECL void bufferLoadAndDeinterleave(std::array<T_buff_1024b<TT_DATA>, TP_NUM_INPUTS>& sbuffArray,
                                           TT_DATA*& inPtr,
                                           unsigned int splice,
                                           unsigned int phaseOffset) {
    constexpr int kSamplesIn128b = 16 / sizeof(TT_DATA);
    constexpr int kSamplesIn256b = kSamplesIn128b * 2;
    constexpr int kSamplesIn512b = kSamplesIn128b * 4;
    using t512v = ::aie::vector<TT_DATA, kSamplesIn512b>;
    using t256v = ::aie::vector<TT_DATA, kSamplesIn256b>;
    using t128v = ::aie::vector<TT_DATA, kSamplesIn128b>;
    using vec = typename T_buff_512b<TT_DATA>::v_type;
    if
        constexpr(TP_NUM_INPUTS == 1) { upd_win_incr_256b<TT_DATA>(sbuffArray[0], splice, inPtr); }
    else if
        constexpr(TP_NUM_INPUTS == 2) {
            constexpr int kSamplesInVec = T_buff_512b<TT_DATA>::getLanes();
            vec writeValLoc;
            upd_win_incr_256b<TT_DATA>(writeValLoc, 0, inPtr);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 1, inPtr);
            ::std::pair<t256v, t256v> inIntlv =
                ::aie::interleave_unzip(writeValLoc.template extract<kSamplesInVec / 2>(0),
                                        writeValLoc.template extract<kSamplesInVec / 2>(1), 1);
            sbuffArray[0].val.insert(splice % 4, inIntlv.first);
            sbuffArray[1].val.insert(splice % 4, inIntlv.second);
        }
    else if
        constexpr(TP_NUM_INPUTS == 3 || TP_NUM_INPUTS == 5 || TP_NUM_INPUTS == 6 || TP_NUM_INPUTS == 7) {
            constexpr int kSamplesInVec = T_buff_256b<TT_DATA>::getLanes();

#pragma unroll(TP_NUM_INPUTS* kSamplesInVec)
            for (int j = 0; j < (TP_NUM_INPUTS * kSamplesInVec); j++) {
                // Read scalar, as there's no efficient vector deinterleve instruction to operate on odd number of
                // output vectors
                TT_DATA readSample = *inPtr++;

                sbuffArray[(phaseOffset + j) % TP_NUM_INPUTS].val.set(
                    readSample, (splice * kSamplesIn256b + j / TP_NUM_INPUTS) % T_buff_1024b<TT_DATA>::getLanes());
            }

            // Interleave 4 to 1
        }
    else if
        constexpr(TP_NUM_INPUTS == 4) {
            constexpr int kSamplesInVec = T_buff_512b<TT_DATA>::getLanes();
            vec writeValLoc;
            upd_win_incr_256b<TT_DATA>(writeValLoc, 0, inPtr);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 1, inPtr);
            ::std::pair<t256v, t256v> inIntlv0 =
                ::aie::interleave_unzip(writeValLoc.template extract<kSamplesInVec / 2>(0),
                                        writeValLoc.template extract<kSamplesInVec / 2>(1), 1);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 0, inPtr);
            upd_win_incr_256b<TT_DATA>(writeValLoc, 1, inPtr);
            ::std::pair<t256v, t256v> inIntlv1 =
                ::aie::interleave_unzip(writeValLoc.template extract<kSamplesInVec / 2>(0),
                                        writeValLoc.template extract<kSamplesInVec / 2>(1), 1);
            ::std::pair<t256v, t256v> inIntlv2 = ::aie::interleave_unzip(inIntlv0.first, inIntlv1.first, 1);
            sbuffArray[0].val.insert(splice % 4, inIntlv2.first);
            sbuffArray[2].val.insert(splice % 4, inIntlv2.second);
            ::std::pair<t256v, t256v> inIntlv3 = ::aie::interleave_unzip(inIntlv0.second, inIntlv1.second, 1);
            sbuffArray[1].val.insert(splice % 4, inIntlv3.first);
            sbuffArray[3].val.insert(splice % 4, inIntlv3.second);
        }
};
}
}
}
}
} // namespaces
#endif // _DSPLIB_FIR_DECIMATE_ASYM_UTILS_HPP_
