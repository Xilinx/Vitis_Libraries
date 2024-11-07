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
/*
MIXED_RADIX_FFT kernal code.
This file captures the body of run-time code for the kernal class.

Coding conventions
  TT_      template type suffix
  TP_      template parameter suffix
*/

//#include <adf.h>
#include <stdio.h>

using namespace std;

#include "device_defs.h"

// if we use 1kb registers -> aie api uses 2x512b registers for 1024b so we need this for QoR
#define __AIE_API_USE_NATIVE_1024B_VECTOR__
#include "aie_api/aie_adf.hpp"

//#define _DSPLIB_MIXED_RADIX_FFT_HPP_DEBUG_

#include "mixed_radix_fft.hpp"
#include "mixed_radix_fft_utils.hpp"
#include "mixed_radix_twGen.hpp"
#include "kernel_api_utils.hpp"

namespace xf {
namespace dsp {
namespace aie {
namespace fft {
namespace mixed_radix_fft {

// MIXED_RADIX_FFT single channel function - base of specialization .
//-----------------------------------------------------------------------------------------------------
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE>
INLINE_DECL void kernel_MixedRadixFFTClass<TT_IN_DATA,
                                           TT_OUT_DATA,
                                           TT_TWIDDLE,
                                           TP_POINT_SIZE,
                                           TP_FFT_NIFFT,
                                           TP_SHIFT,
                                           TP_RND,
                                           TP_SAT,
                                           TP_WINDOW_VSIZE,
                                           TP_START_RANK,
                                           TP_END_RANK,
                                           TP_DYN_PT_SIZE>::singleFrame(TT_IN_DATA* inbuff, TT_OUT_DATA* outbuff) {
    T_internalDataType* tmp_bufs[2] = {m_tmpBuff0, m_tmpBuff1}; // TODO tmp2 can sometimes be xbuff reused, similarly
                                                                // for odd stages, obuff can be used in place of tmp1.

    constexpr int kR5 = 5;
    constexpr int kR3 = 3;
    constexpr int kR2 = 2;
    constexpr int k6R2 = 64; // 2^6  this is just used to make the code below tabulate nicely.
    constexpr int kR4 = 4;
    constexpr int kR4twinc = // the radix 4 stage takes 2 twiddle tables for AIE1 and 3 for AIE2
#if __FFT_R4_IMPL__ == 0
        2;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
    3;
#endif //__FFT_R4_IMPL__ == 1

    constexpr int kR3twbase = m_kR5Stages * (kR5 - 1);
    constexpr int kR2twbase = m_kR5Stages * (kR5 - 1) + m_kR3Stages * (kR3 - 1);
    constexpr int kR4twbase = m_kR5Stages * (kR5 - 1) + m_kR3Stages * (kR3 - 1) + m_kR2Stages * (kR2 - 1);
    constexpr int ktwEnd =
        kR4twbase +
        m_kR4Stages * kR4twinc; // end of tw pointer table., i.e. total number of tw tables in the master table.

    bool inv = TP_FFT_NIFFT == 1 ? false : true;
    unsigned int pingPong = 0; // read from ping, write to pong, or vice versa.

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    int tw = 0; // indicates which table within the master to use, and we always use the next table, so this simply
                // increments.

    // TP_SHIFT is sent to each stage, but the stage will apply 0 unless it is the last stage
    //                                        TT_IN_DATA, TT_OUT_DATA, T_INTERNAL_DATA,    TT_TWIDDLE, TP_START_RANK,
    //                                        TP_END_RANK, stage,                                 TP_POINT_SIZE,
    //                                        rmodifier,                                           TP_SHIFT(see above)
    //                                        TW_BASE (which sub-table)
    // Radix 5 stages
    if
        constexpr(m_kR5Stages > 0) opt_r5_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, 0, TP_POINT_SIZE, kR5, 0, (kR5 - 1) * 0>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR5Stages > 1) opt_r5_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, 1, TP_POINT_SIZE, kR5 * kR5, 0, (kR5 - 1) * 1>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR5Stages > 2) opt_r5_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, 2, TP_POINT_SIZE, kR5 * kR5 * kR5, 0, (kR5 - 1) * 2>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);

    // Radix 3 stages
    // printf("\n m_kR5factor * kR3 = %d ; kR3twbase + (kR3 - 1) * 0 = %d", m_kR5factor * kR3, kR3twbase + (kR3 - 1) *
    // 0);
    if
        constexpr(m_kR3Stages > 0)
            opt_r3_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages, TP_POINT_SIZE, m_kR5factor * kR3, 0, kR3twbase + (kR3 - 1) * 0>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR3Stages > 1)
            opt_r3_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + 1, TP_POINT_SIZE, m_kR5factor * kR3 * kR3, 0, kR3twbase + (kR3 - 1) * 1>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR3Stages > 2)
            opt_r3_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + 2, TP_POINT_SIZE, m_kR5factor * kR3 * kR3 * kR3, 0, kR3twbase + (kR3 - 1) * 2>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR3Stages > 3) opt_r3_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + 3, TP_POINT_SIZE,
                                                m_kR5factor * kR3 * kR3 * kR3 * kR3, 0, kR3twbase + (kR3 - 1) * 3>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR3Stages > 4)
            opt_r3_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + 4, TP_POINT_SIZE, m_kR5factor * kR3 * kR3 * kR3 * kR3 * kR3, 0,
                         kR3twbase + (kR3 - 1) * 4>(inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable,
                                                    m_twiddlePtrPtr);

    // Radix 2 stage (s) - ideally use radix4 for speed, so only one r2 stage should ever exist, but radix2 can support
    // low powers of 2 where radix4 cannot.
    constexpr int kr2shift0 = m_kR5Stages + m_kR3Stages == m_kTotalStages - 1
                                  ? TP_SHIFT
                                  : 0; // TP_SHIFT should only be applied on the last stage of the FFT. In the context
                                       // of cascaded kernels TP_END_RANK cannot be compared to stage by opt_rX_stage
                                       // because TP_END_RANK is only the end of the kernel, not the end of the FFT
    constexpr int kr2shift1 = m_kR5Stages + m_kR3Stages + 1 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift2 = m_kR5Stages + m_kR3Stages + 2 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift3 = m_kR5Stages + m_kR3Stages + 3 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift4 = m_kR5Stages + m_kR3Stages + 4 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift5 = m_kR5Stages + m_kR3Stages + 5 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift6 = m_kR5Stages + m_kR3Stages + 6 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift7 = m_kR5Stages + m_kR3Stages + 7 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift8 = m_kR5Stages + m_kR3Stages + 8 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr2shift9 = m_kR5Stages + m_kR3Stages + 9 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    if
        constexpr(m_kR2Stages > 0) opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * kR2, kr2shift0, kR2twbase + (kR2 - 1) * 0>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 1)
            opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + 1, TP_POINT_SIZE, m_kR5factor * m_kR3factor * kR2 * kR2, kr2shift1,
                         kR2twbase + (kR2 - 1) * 1>(inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable,
                                                    m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 2)
            opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + 2, TP_POINT_SIZE, m_kR5factor * m_kR3factor * kR2 * kR2 * kR2,
                         kr2shift2, kR2twbase + (kR2 - 1) * 2>(inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable,
                                                               m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 3)
            opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + 3, TP_POINT_SIZE,
                         m_kR5factor * m_kR3factor * kR2 * kR2 * kR2 * kR2, kr2shift3, kR2twbase + (kR2 - 1) * 3>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 4)
            opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + 4, TP_POINT_SIZE,
                         m_kR5factor * m_kR3factor * kR2 * kR2 * kR2 * kR2 * kR2, kr2shift4, kR2twbase + (kR2 - 1) * 4>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 5) opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages + 5, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * kR2 * kR2 * kR2 * kR2 * kR2 * kR2,
                                                kr2shift5, kR2twbase + (kR2 - 1) * 5>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 6) opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages + 6, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * kR2 * kR2 * kR2 * kR2 * kR2 * kR2 * kR2,
                                                kr2shift6, kR2twbase + (kR2 - 1) * 6>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 7)
            opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + 7, TP_POINT_SIZE, m_kR5factor * m_kR3factor * k6R2 * kR2 * kR2,
                         kr2shift7, kR2twbase + (kR2 - 1) * 7>(inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable,
                                                               m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 8)
            opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + 8, TP_POINT_SIZE,
                         m_kR5factor * m_kR3factor * k6R2 * kR2 * kR2 * kR2, kr2shift8, kR2twbase + (kR2 - 1) * 8>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR2Stages > 9) opt_r2_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages + 9, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * k6R2 * kR2 * kR2 * kR2 * kR2, kr2shift9,
                                                kR2twbase + (kR2 - 1) * 9>(inbuff, outbuff, tmp_bufs, pingPong, inv,
                                                                           m_twTable, m_twiddlePtrPtr);

    // Radix 4 stages
    constexpr int kr4shift0 = m_kR5Stages + m_kR3Stages + m_kR2Stages == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr4shift1 = m_kR5Stages + m_kR3Stages + m_kR2Stages + 1 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr4shift2 = m_kR5Stages + m_kR3Stages + m_kR2Stages + 2 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr4shift3 = m_kR5Stages + m_kR3Stages + m_kR2Stages + 3 == m_kTotalStages - 1 ? TP_SHIFT : 0;
    constexpr int kr4shift4 = m_kR5Stages + m_kR3Stages + m_kR2Stages + 4 == m_kTotalStages - 1 ? TP_SHIFT : 0;

    if
        constexpr(m_kR4Stages > 0)
            opt_r4_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + m_kR2Stages, TP_POINT_SIZE,
                         m_kR5factor * m_kR3factor * m_kR2factor * kR4, kr4shift0, kR4twbase + kR4twinc * 0>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);

    if
        constexpr(m_kR4Stages > 1)
            opt_r4_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK, TP_END_RANK,
                         m_kR5Stages + m_kR3Stages + m_kR2Stages + 1, TP_POINT_SIZE,
                         m_kR5factor * m_kR3factor * m_kR2factor * kR4 * kR4, kr4shift1, kR4twbase + kR4twinc * 1>(
                inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR4Stages > 2) opt_r4_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages + m_kR2Stages + 2, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * m_kR2factor * kR4 * kR4 * kR4, kr4shift2,
                                                kR4twbase + kR4twinc * 2>(inbuff, outbuff, tmp_bufs, pingPong, inv,
                                                                          m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR4Stages > 3) opt_r4_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages + m_kR2Stages + 3, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * m_kR2factor * kR4 * kR4 * kR4 * kR4,
                                                kr4shift3, kR4twbase + kR4twinc * 3>(
            inbuff, outbuff, tmp_bufs, pingPong, inv, m_twTable, m_twiddlePtrPtr);
    if
        constexpr(m_kR4Stages > 4) opt_r4_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_START_RANK,
                                                TP_END_RANK, m_kR5Stages + m_kR3Stages + m_kR2Stages + 4, TP_POINT_SIZE,
                                                m_kR5factor * m_kR3factor * m_kR2factor * k6R2 * kR4 * kR4, kr4shift4,
                                                kR4twbase + kR4twinc * 4>(inbuff, outbuff, tmp_bufs, pingPong, inv,
                                                                          m_twTable, m_twiddlePtrPtr);
};

// Dynamic specialization of base
//-----------------------------------------------------------------------------------------------------
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK>
INLINE_DECL void kernel_MixedRadixFFTClass<TT_IN_DATA,
                                           TT_OUT_DATA,
                                           TT_TWIDDLE,
                                           TP_POINT_SIZE,
                                           TP_FFT_NIFFT,
                                           TP_SHIFT,
                                           TP_RND,
                                           TP_SAT,
                                           TP_WINDOW_VSIZE,
                                           TP_START_RANK,
                                           TP_END_RANK,
                                           1>::dynamicSingleFrame(TT_IN_DATA* inbuff,
                                                                  TT_TWIDDLE* twInBuff,
                                                                  TT_OUT_DATA* outbuff,
                                                                  std::array<T_ancillaryFields, kVectorSize>
                                                                      headerData) { // TODO aie::vector?
    T_internalDataType* tmp_bufs[2] = {m_tmpBuff0, m_tmpBuff1};

    using twVect_t = ::aie::vector<TT_TWIDDLE, kVectorSize>;
    using infoVect_t = ::aie::vector<T_ancillaryFields, kVectorSize>; // TODO T_info
    twVect_t* twPtr = (twVect_t*)twInBuff; // for twiddle fields output - NOTE: twInBuff is the pointers to the data
    infoVect_t* infoOutPtr = (infoVect_t*)twPtr;
    twPtr = &twPtr[twiddleIdxStart];

    infoVect_t* tw_table = infoOutPtr;
    TT_TWIDDLE* twbuff = twInBuff; //[twiddleIdxStart]; because indices are to the "row" in the whole tw buffer
    T_ancillaryFields* tw_ptrs = (T_ancillaryFields*)infoOutPtr +
                                 indexIdxStart * kVectorSize; // TODO can only be int32 because TT_TWIDDLE is cint16

    // Process headerData
    infoVect_t headerRead = infoOutPtr[0];

    int invInt = headerRead.get(0);
    bool inv = (invInt == 1) ? false : true;

    int nR2raw = headerRead.get(2);
    int stagesR3 = headerRead.get(3);
    int stagesR5 = headerRead.get(4);

    int stagesR2;
    int stagesR4;
#if __FFT_R4_IMPL__ == \
    0 // AIE1 - since spoofed radix 4 stages = two radix 2 stages, create twiddles as if it were radix 2 stages
#if __ONLY_R2_STAGES__ == 1 // Only use radix 2 stages. Do not perform any radix 4 stages ever.
    stagesR2 = nR2raw;
    stagesR4 = 0;
#elif __ONLY_R2_STAGES__ == 0
    // TODO OPTIMISATION Do operation instead of conditionals
    if (nR2raw >= 4) { // if pointsize is divisible by 16
        // Re-calculate number of R2s, and calculate number of R4s
        stagesR2 = (nR2raw & 1);
        stagesR4 = nR2raw >> 1;
    } else {
        stagesR2 = nR2raw;
        stagesR4 = 0;
    }
#endif // __ONLY_R2_STAGES__
#else  // AIE-ML
    stagesR2 = (nR2raw & 1);
    stagesR4 = nR2raw >> 1;
#endif // __FFT_R4_IMPL__

    set_rnd_mode<TP_RND>();
    set_sat_mode<TP_SAT>();

    int totalStages = stagesR5 + stagesR3 + stagesR2 + stagesR4;
    int pointsize = tw_table[factorsStagePtSizeIdxStart][0];
    unsigned int pingPong = 0; // read from ping, write to pong, or vice versa.

    int leg_counter = 0;
    int stage_number = 0;

    // Radix 5 stages
    for (int stage = 0; stage < stagesR5; stage++) {
        const unsigned int r = tw_table[factorsStepsizeIdxStart][stage_number];

        opt_r5_dyn_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_SHIFT>(
            inbuff, outbuff, tmp_bufs, pingPong, pointsize, r, inv, totalStages, stage_number, twbuff, tw_ptrs,
            leg_counter); // base_index is index of index?

        stage_number++;
        leg_counter = leg_counter + (kR5 - 1);
    }

    // Radix 3 stages
    for (int stage = 0; stage < stagesR3; stage++) {
        const unsigned int r = tw_table[factorsStepsizeIdxStart][stage_number];

        opt_r3_dyn_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_SHIFT>(
            inbuff, outbuff, tmp_bufs, pingPong, pointsize, r, inv, totalStages, stage_number, twbuff, tw_ptrs,
            leg_counter); // base_index is index of index?

        stage_number++;
        leg_counter = leg_counter + (kR3 - 1);
    }

    // Radix 2 stages
    for (int stage = 0; stage < stagesR2; stage++) {
        const unsigned int r = tw_table[factorsStepsizeIdxStart][stage_number];

        opt_r2_dyn_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_SHIFT>(
            inbuff, outbuff, tmp_bufs, pingPong, pointsize, r, inv, totalStages, stage_number, twbuff, tw_ptrs,
            leg_counter); // base_index is index of index?

        stage_number++;
        leg_counter = leg_counter + (kR2 - 1);
    }

#if __ONLY_R2_STAGES__ == 0 // Do not only use radix 2 stages. Perform radix 4 stages.

    constexpr int kR4twleginc = // the radix 4 stage takes 2 twiddle tables for AIE1 and 3 for AIE2
#if __FFT_R4_IMPL__ == 0
        2;
#endif //__FFT_R4_IMPL__ == 0
#if __FFT_R4_IMPL__ == 1
    3;
#endif //__FFT_R4_IMPL__ == 1

    // TODO need radix 4 shifts?
    // Radix 4 stages
    for (int stage = 0; stage < stagesR4; stage++) {
        const unsigned int r = tw_table[factorsStepsizeIdxStart][stage_number];

        opt_r4_dyn_stage<TT_IN_DATA, TT_OUT_DATA, T_internalDataType, TT_TWIDDLE, TP_SHIFT>(
            inbuff, outbuff, tmp_bufs, pingPong, pointsize, r, inv, totalStages, stage_number, twbuff, tw_ptrs,
            leg_counter);

        stage_number++;
        leg_counter = leg_counter + kR4twleginc;
    }
#endif // __ONLY_R2_STAGES__
};

template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE>
INLINE_DECL void
kernel_MixedRadixFFTClass<TT_IN_DATA,
                          TT_OUT_DATA,
                          TT_TWIDDLE,
                          TP_POINT_SIZE,
                          TP_FFT_NIFFT,
                          TP_SHIFT,
                          TP_RND,
                          TP_SAT,
                          TP_WINDOW_VSIZE,
                          TP_START_RANK,
                          TP_END_RANK,
                          TP_DYN_PT_SIZE>::kernelMixedRadixFFTmain(input_buffer<TT_IN_DATA>* inWindow,
                                                                   output_buffer<TT_OUT_DATA>* outWindow) {
    TT_IN_DATA* inbuff = (TT_IN_DATA*)inWindow->data();
    TT_OUT_DATA* outbuff = (TT_OUT_DATA*)outWindow->data();

    for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++) {
        singleFrame(inbuff, outbuff);
        inbuff += TP_POINT_SIZE;
        outbuff += TP_POINT_SIZE;
    }
};

template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK>
INLINE_DECL void kernel_MixedRadixFFTClass<TT_IN_DATA,
                                           TT_OUT_DATA,
                                           TT_TWIDDLE,
                                           TP_POINT_SIZE,
                                           TP_FFT_NIFFT,
                                           TP_SHIFT,
                                           TP_RND,
                                           TP_SAT,
                                           TP_WINDOW_VSIZE,
                                           TP_START_RANK,
                                           TP_END_RANK,
                                           1>::kernelMixedRadixFFTmain(input_buffer<TT_IN_DATA>* inWindow,
                                                                       input_buffer<TT_TWIDDLE>* twInWindow,
                                                                       output_buffer<TT_OUT_DATA>* outWindow) {
    TT_IN_DATA* inbuff = (TT_IN_DATA*)inWindow->data();
    TT_OUT_DATA* outbuff = (TT_OUT_DATA*)outWindow->data();

    TT_TWIDDLE* twbuff = (TT_TWIDDLE*)twInWindow->data(); // for ancillary fields output: header and factors and indices
    using infoVect_t = std::array<T_ancillaryFields, kVectorSize>;
    infoVect_t* infoOutPtr = (infoVect_t*)twbuff;

    infoVect_t headerRead = infoOutPtr[0];

    // Immediately read errorFlag and output zeros if error
    int errorFlag = headerRead[7];
    if (errorFlag == 1) {
        for (int w = 0; w < TP_WINDOW_VSIZE; w++) {
            *outbuff++ = {0, 0}; // TODO optimise
        }
    } else { // otherwise continue to read and process
        for (int frame = 0; frame < TP_WINDOW_VSIZE / TP_POINT_SIZE; frame++) {
            dynamicSingleFrame(inbuff, twbuff, outbuff, headerRead);
            inbuff += TP_POINT_SIZE;
            outbuff += TP_POINT_SIZE;
        }
    }
};

//-----------------------------------------------------------------------------------------------------
// For a single kernel - iobuffer in and out, no cascades
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK,
          unsigned int TP_DYN_PT_SIZE>
NOINLINE_DECL void
mixed_radix_fft<TT_IN_DATA,
                TT_OUT_DATA,
                TT_TWIDDLE,
                TP_POINT_SIZE,
                TP_FFT_NIFFT,
                TP_SHIFT,
                TP_RND,
                TP_SAT,
                TP_WINDOW_VSIZE,
                TP_START_RANK,
                TP_END_RANK,
                TP_DYN_PT_SIZE>::mixed_radix_fftMain(input_buffer<TT_IN_DATA>& __restrict inWindow,
                                                     output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    this->kernelMixedRadixFFTmain((input_buffer<TT_IN_DATA>*)&inWindow, (output_buffer<TT_OUT_DATA>*)&outWindow);
};

//-----------------------------------------------------------------------------------------------------
// For a single kernel - iobuffer in and out, no cascades DYNAMIC
template <typename TT_IN_DATA,
          typename TT_OUT_DATA,
          typename TT_TWIDDLE,
          unsigned int TP_POINT_SIZE,
          unsigned int TP_FFT_NIFFT,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_WINDOW_VSIZE,
          unsigned int TP_START_RANK,
          unsigned int TP_END_RANK>
NOINLINE_DECL void mixed_radix_fft<TT_IN_DATA,
                                   TT_OUT_DATA,
                                   TT_TWIDDLE,
                                   TP_POINT_SIZE,
                                   TP_FFT_NIFFT,
                                   TP_SHIFT,
                                   TP_RND,
                                   TP_SAT,
                                   TP_WINDOW_VSIZE,
                                   TP_START_RANK,
                                   TP_END_RANK,
                                   1>::mixed_radix_fftMain(input_buffer<TT_IN_DATA>& __restrict inWindow,
                                                           input_buffer<TT_TWIDDLE>& __restrict twInWindow,
                                                           output_buffer<TT_OUT_DATA>& __restrict outWindow) {
    this->kernelMixedRadixFFTmain((input_buffer<TT_IN_DATA>*)&inWindow, (input_buffer<TT_TWIDDLE>*)&twInWindow,
                                  (output_buffer<TT_OUT_DATA>*)&outWindow);
};
}
}
}
}
}
