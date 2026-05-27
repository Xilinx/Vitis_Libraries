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

/*
CONV_CORR kernel code.
This file captures the body of run-time code for the kernel class.
Coding conventions
TT_      template type suffix
TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <vector>
#include "conv_corr.hpp"
#include "conv_corr_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace conv_corr {

//-----------------------------------------------------------------------------------------------------
// Convolution/Correlation Kernel Class No RTP
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_USE_RTP_VECTOR_LENGTHS>
NOINLINE_DECL void
conv_corr<TT_DATA_F,
          TT_DATA_G,
          TT_DATA_OUT,
          TP_FUNCT_TYPE,
          TP_COMPUTE_MODE,
          TP_F_LEN,
          TP_G_LEN,
          TP_SHIFT,
          TP_API,
          TP_RND,
          TP_SAT,
          TP_NUM_FRAMES,
          TP_CASC_LEN,
          TP_PHASES,
          TP_KERNEL_POSITION,
          TP_PH_POSITION,
          TP_CASC_IN,
          TP_CASC_OUT,
          TP_USE_RTP_VECTOR_LENGTHS>::conv_corrNoRTP(input_buffer<TT_DATA_F>& __restrict inWindowF,
                                                     input_buffer<TT_DATA_G>& __restrict inWindowG,
                                                     output_buffer<TT_DATA_OUT>& __restrict outWindow) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    using T_accum = T_acc_ConCor<TT_DATA_F, TT_DATA_G>;
    using T_buff_Xbuff = T_InOut_Y_buff<TT_DATA_F>;            // Y_REG_BITS buffer for X Reg.
    using T_buff_Zbuff = T_InOut_W_buff<TT_DATA_G>;            // W_REG_BITS buffer for W Reg
    using T_buff_FSig = T_InOut_W_buff<TT_DATA_F>;             // W_REG_BITS buffer for reading F Data
    using T_buff_GSig = T_InOut_W_buff<TT_DATA_G>;             // W_REG_BITS buffer for reading G Data
    using dataVecOut_t = ::aie::vector<TT_DATA_OUT, m_kLanes>; // Output vector based on output data type and lanes
    using T_vecF = ::aie::vector<TT_DATA_F, m_kVecLoadF>;      // Vector type for F data load
    T_vecF zeroVec =
        ::aie::zeros<TT_DATA_F,
                     m_kVecLoadF>(); // Zero Vector of F data type and F load size to use for padding of zeros.

    T_accum acc;             // Declaration of Accumulator
    T_buff_Xbuff xbuff;      // Declaration of xbuff using X(1024b) Reg.
    T_buff_Zbuff zbuff;      // Declaration of zbuff using W(256b) Reg.
    dataVecOut_t dataVecOut; // Declaration of Output Vector to store results into Out buffer.
    dataVecOut_t dataOut =
        ::aie::zeros<TT_DATA_OUT, m_kLanes>(); // Declaration of Output Vector to store results after masking.

    constexpr unsigned int DataStepX = 1;                       // Initilization of slidingmul parameter i.e. DataStepX
    constexpr unsigned int DataStepZ = 1;                       // Initilization of slidingmul parameter i.e. DataStepZ
    constexpr unsigned int DataStepY = 1;                       // Initilization of slidingmul parameter i.e. DataStepY
    constexpr unsigned int Lanes = m_kLanes;                    // Num of Lanes
    constexpr unsigned int Points = m_kPoints;                  // Num of Points
    constexpr unsigned int kFLoadbits = getLog2<m_kVecLoadF>(); // No of bits for FLoad
    constexpr unsigned int kAccumLen = ROUND(TP_G_LEN, m_kVecLoadG);               // Len. of Accumulator
    constexpr unsigned int kInLoopLen = ROUND(m_kVecLoadF, m_kPoints);             // In loop Length
    constexpr unsigned int kGandFLoadRatio = FLOOR(m_kVecLoadG, m_kVecLoadF);      // Ratio of G load to F Load
    constexpr unsigned int kLoopCount = (CEIL(m_kLoopCount, m_kLanes) / m_kLanes); // Loopcount
    constexpr unsigned int kDataLoadofF = (kMaxBytesLoadOnAie / sizeof(TT_DATA_F));
    constexpr unsigned int kDataLoadofG = (kMaxBytesLoadOnAie / sizeof(TT_DATA_G));
    constexpr unsigned int kFrameoffsetF = (m_kPaddedLenData / kDataLoadofF);        // Offset of F ptr
    constexpr unsigned int kFrameoffsetG = (TP_G_LEN / kDataLoadofG);                // Offset of F ptr
    constexpr unsigned int kIncrOffsetGSig = ((TP_G_LEN / kDataLoadofG) - 1);        // Offset to increment G Sig
    constexpr unsigned int kPaddedVecLoopCnt = ROUND(m_kPaddedLenData, m_kVecLoadF); // Loop count for padded vector
    constexpr unsigned int kDataStartIndex =
        m_kFdataStartIndx; // Start index (in vectors) for F data to be copied into buffer
    constexpr unsigned int kDataEndIndex = m_kFdataEndIndx; // End index (in vectors) to stop F data copy
    constexpr unsigned int kXStartIdxAdj =
        (TP_COMPUTE_MODE == SAME_MODE)
            ? ((m_kFdataStartIndx * m_kVecLoadF) - (CEIL((TP_G_LEN >> 1), kSameModeRoundFactor) - 1))
            : (TP_COMPUTE_MODE == VALID_MODE) ? 0 : ((m_kFdataStartIndx * m_kVecLoadF) -
                                                     (TP_G_LEN - 1)); // Offset accounting for vector-aligned padding

    // Pointers to do Padding zeros to the F data based on compute mode
    T_buff_FSig* __restrict inPtrF = (T_buff_FSig*)inWindowF.data(); // Alias Input pointer for Sig_F
    T_buff_FSig* paddedFdataPtr = (T_buff_FSig*)paddedFdata;         // Vector pointer to padded F data

    // Pointers for Input F and G. Also pointer for Output Data.
    T_buff_FSig* inDataPtrF = (T_buff_FSig*)paddedFdata;                 // Input pointer for Sig_F
    T_buff_GSig* __restrict inDataPtrG = (T_buff_GSig*)inWindowG.data(); // Input pointer for Sig_G
    dataVecOut_t* __restrict outPtr = (dataVecOut_t*)outWindow.data();   // Output Pointer for conv/corr result

    // Alias Pointers for both F and G.
    T_buff_FSig* rdInDataFPtr = (T_buff_FSig*)paddedFdata;                 // Alias Input pointer for Sig_F
    T_buff_GSig* __restrict rdInDataGPtr = (T_buff_GSig*)inWindowG.data(); // Alias Input pointer for Sig_G

    // Alias for sliding_mul API
    using mul_ops = ::aie::sliding_mul_ops<Lanes, Points, DataStepX, DataStepZ, DataStepY, TT_DATA_G, TT_DATA_F,
                                           tConvCorrAccType_t<TT_DATA_F, TT_DATA_G> >;

    for (unsigned int frameIndx = 0; frameIndx < TP_NUM_FRAMES; frameIndx++)
        chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            // Initialize pointer to start of current frame
            T_buff_FSig* paddedFdataPtr = (T_buff_FSig*)paddedFdata + (frameIndx * kFrameoffsetF);

            // ========== Zero padding before F data(0 to kDataStartIndex) ==========
            for (unsigned int dataIndx = 0; dataIndx < kDataStartIndex; dataIndx++)
                chess_prepare_for_pipelining chess_loop_count(kDataStartIndex) {
                    *paddedFdataPtr++ = (T_buff_FSig)zeroVec; // Write zeroVec, increment paddedFdataPtr
                }

            // ========== Copy F data (kDataStartIndex to kDataEndIndex) ==========
            for (unsigned int dataIndx = kDataStartIndex; dataIndx < kDataEndIndex; dataIndx++)
                chess_prepare_for_pipelining chess_loop_count(kDataEndIndex - kDataStartIndex) {
                    *paddedFdataPtr++ = *inPtrF++; // Copy F data, increment both pointers
                }

            // ========== Zero padding after F data (kDataEndIndex to end) ==========
            constexpr unsigned int totalVecs = m_kPaddedLenData / m_kVecLoadF;
            constexpr unsigned int suffixCount = totalVecs - kDataEndIndex;

            for (unsigned int dataIndx = kDataEndIndex; dataIndx < totalVecs; dataIndx++)
                chess_prepare_for_pipelining chess_loop_count(suffixCount) {
                    *paddedFdataPtr++ = (T_buff_FSig)zeroVec; // Write zeroVec, increment paddedFdataPtr
                }

            // If the function type is conv (1), then we need to reverse the G data.
            if (TP_FUNCT_TYPE == CONV) {
                T_buff_GSig* __restrict inDataEndPtrG; // Local Pointer to fetch end address of G data
                constexpr int kVsize =
                    (kMaxBytesLoadOnAie /
                     sizeof(TT_DATA_G)); // Size of vector (256/8/sizeof(DATA) as per given data type.
                using t_vect = ::aie::vector<TT_DATA_G, kVsize>; // definition using the vec size.
                t_vect zbuffs, zbuffe; // Registers to hold Start and End data samples of G sig.

                inDataPtrG = (rdInDataGPtr + (frameIndx * kFrameoffsetG));
                inDataEndPtrG = (inDataPtrG + kIncrOffsetGSig); // Move the pointer to End address of G sig.

                // Vector pointers for efficient in-place data reversal
                t_vect* startPtr = (t_vect*)inDataPtrG;  // Points to beginning of G buffer
                t_vect* endPtr = (t_vect*)inDataEndPtrG; // Points to end of G buffer

                t_vect* outStartPtrG = (t_vect*)
                    inDataPtrG; // Output pointer pointing to Start of G buffer, used to write the swapped data of G sig
                t_vect* outEndPtrG = (t_vect*)inDataEndPtrG; // Output pointer pointing to End of G buffer, used to
                                                             // write the swapped data of G sig

                // Calculate number of full vector swaps and remaining elements
                constexpr unsigned int kFullVecSwaps = (TP_G_LEN / kVsize) >> 1;
                constexpr unsigned int kRemElements = TP_G_LEN % kVsize;
                constexpr bool kPartialVec = (kRemElements > 0);

                // reverse the G elements using inplace memory for Conv - full vectors
                for (unsigned int i = 0; i < kFullVecSwaps; i++)
                    chess_prepare_for_pipelining chess_loop_count(kFullVecSwaps) {
                        zbuffs = *startPtr++; // Load from starting position and increment
                        zbuffe = *endPtr--;   // Load from end position and decrement

                        // Reverse the data fetched and store in respective locations
                        zbuffs = (::aie::reverse(zbuffs));
                        zbuffe = (::aie::reverse(zbuffe));

                        *outStartPtrG++ = zbuffe; // Storing the data after swap to same G sig buffer.
                        *outEndPtrG-- = zbuffs;   // Storing the data after swap to same G sig buffer.
                    }

                // Handle the center vector if TP_G_LEN/kVsize is odd
                if
                    constexpr(kPartialVec || ((TP_G_LEN / kVsize) & 1)) {
                        zbuffs = *startPtr;

                        zbuffs = (::aie::reverse(zbuffs));
                        *outStartPtrG = zbuffs; // Storing the data after swap to same G sig buffer.
                    }
            }

            // Outer Loop to do Convolution/Correlation
            for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
                chess_prepare_for_pipelining chess_loop_count(kLoopCount) {
                    // Updation of F and G pointer to fetch respective data from both signal
                    inDataPtrF =
                        (rdInDataFPtr + (outIndx * (m_kLanes >> kFLoadbits)) +
                         (frameIndx * kPaddedVecLoopCnt)); // Reset F Sig Pointer - load from start including padding
                    inDataPtrG = (rdInDataGPtr + (frameIndx * kFrameoffsetG)); // Reset G Sig Pointer.

                    // Conservative bounds check - done ONCE per outer loop before any inner loops
                    // kInnerLoopFptrIncr: max F-ptr offset at the moment of 3rd vector load in the inner loop (4th load
                    // goes one further, but that one is also conditionally loaded)
                    constexpr unsigned int kInnerLoopFptrIncr = kAccumLen * kGandFLoadRatio + 1;
                    constexpr unsigned int kThirdVecLoadOffset =
                        (kInnerLoopFptrIncr < kPaddedVecLoopCnt) ? (kPaddedVecLoopCnt - kInnerLoopFptrIncr) : 0;
                    // The additional pre-increment advances one extra vector, so the threshold is reduced by one.
                    constexpr unsigned int kFourthVecLoadOffset =
                        (kInnerLoopFptrIncr + 1 < kPaddedVecLoopCnt) ? (kPaddedVecLoopCnt - kInnerLoopFptrIncr - 1) : 0;

                    unsigned int baseOffset = (outIndx * (m_kLanes >> kFLoadbits));
                    bool load3rdVec = (baseOffset < kThirdVecLoadOffset);
                    bool load4thVec = (baseOffset < kFourthVecLoadOffset);

                    // Initialization of Accumulator with Zeros to flush the previous data.
                    acc.val = ::aie::zeros<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>,
                                           acc.val.size()>(); // null all the elements of accumulator

// Create a "nested" loop, where inner 4 iteration loop will be unrolled, wrapped in a for (kAccumLen / 4 iteration)
// loop.
#pragma unroll(4)
                    for (unsigned int inLoopIndx = 0; inLoopIndx < kAccumLen;
                         inLoopIndx++) // Inner Loop to do conv/corr using F and G Data.
                    {
                        // Load Data of G Signal
                        upd_W_buff(zbuff.val, 0, inDataPtrG++); // Fetch the G data to Registrer zbuff.
                        if
                            constexpr((TP_FUNCT_TYPE == CORR) && (isComplex<TT_DATA_G>())) {
                                zbuff.val = ::aie::conj(
                                    zbuff.val); // zbuff data should be conjugate when function type is correlation
                            }
// Load Data of F Signal
#pragma unroll(kGandFLoadRatio)
                        for (unsigned int k = 0; k < kGandFLoadRatio; k++) {
                            // Pointer Manipulation to read No of "FLoad" elements into Y_REG_BITS buffer
                            // Always load first two vectors
                            upd_W_buff(xbuff.val, 0,
                                       inDataPtrF++); // update xbuff with F sig data based on vector load.
                            upd_W_buff(xbuff.val, kBuffIndx1,
                                       inDataPtrF); // update xbuff with F sig data based on vector load.

                            // Load 3rd and 4th vectors only when the precomputed bounds condition is true (evaluated
                            // once per outer loop); otherwise this unrolled path is a no-op and the pointer is not
                            // advanced.
                            if (load3rdVec) {
                                inDataPtrF++; // increment pointer to next vector boundary if 3rd vector load is valid
                                upd_W_buff(xbuff.val, kBuffIndx2,
                                           inDataPtrF); // update xbuff with F sig data based on 3rd vector load.

                                if (load4thVec) {
                                    inDataPtrF++; // increment pointer to next vector boundary if 4th vector load is
                                                  // valid
                                    upd_W_buff(xbuff.val, kBuffIndx3, inDataPtrF--); // update xbuff with F sig data
                                                                                     // based on 4th vector load, then
                                                                                     // decrement pointer
                                }
                                inDataPtrF--; // restore pointer to next iteration's first vector
                            }

#pragma unroll(kInLoopLen)
                            for (unsigned int l = 0; l < kInLoopLen; l++) {
                                // Sliding Multiplication of given signals
                                acc.val = mul_ops::mac(acc.val, zbuff.val, ((k * kInLoopLen + l) * m_kPoints),
                                                       xbuff.val, ((l * m_kPoints) + kXStartIdxAdj));

                            } // End of kInLoopLen
                        }     // End of kGandFLoadRatio
                    }         // End of InnerLoop

                    dataVecOut = acc.val.template to_vector<TT_DATA_OUT>(
                        TP_SHIFT); // Storing accumulator results into out data vector

                    // Handle partial output vector in last iteration for ALL compute modes
                    if
                        constexpr((m_kLoopCount % m_kLanes) != 0) {
                            constexpr unsigned int kValidSamplesLast = m_kLoopCount % m_kLanes;
                            constexpr uint32_t kLastVecMask = ~((1U << kValidSamplesLast) - 1U);

                            if (outIndx == (kLoopCount - 1)) {
                                dataVecOut = ::aie::select(dataVecOut, dataOut,
                                                           ::aie::mask<m_kLanes>::from_uint32(kLastVecMask));
                            }
                        }

                    *outPtr++ = dataVecOut; // write the output results to the iobuffer

                } // End of OuterLoop
        }         // End of Frames
};                // End of conv_corrNoRTP()

// Convolution/Correlation Kernel Class with RTP
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_COMPUTE_MODE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION,
          bool TP_CASC_IN,
          bool TP_CASC_OUT,
          unsigned int TP_USE_RTP_VECTOR_LENGTHS>
NOINLINE_DECL void conv_corr<TT_DATA_F,
                             TT_DATA_G,
                             TT_DATA_OUT,
                             TP_FUNCT_TYPE,
                             TP_COMPUTE_MODE,
                             TP_F_LEN,
                             TP_G_LEN,
                             TP_SHIFT,
                             TP_API,
                             TP_RND,
                             TP_SAT,
                             TP_NUM_FRAMES,
                             TP_CASC_LEN,
                             TP_PHASES,
                             TP_KERNEL_POSITION,
                             TP_PH_POSITION,
                             TP_CASC_IN,
                             TP_CASC_OUT,
                             TP_USE_RTP_VECTOR_LENGTHS>::conv_corrRTP(input_buffer<TT_DATA_F>& __restrict inWindowF,
                                                                      input_buffer<TT_DATA_G>& __restrict inWindowG,
                                                                      output_buffer<TT_DATA_OUT>& __restrict outWindow,
                                                                      const int32 (&inVecLen)[2]) {
    set_rnd_mode<TP_RND>(); // Set rounding mode
    set_sat_mode<TP_SAT>(); // Set saturation mode

    using T_accum = T_acc_ConCor<TT_DATA_F, TT_DATA_G>;
    using T_buff_Xbuff = T_InOut_Y_buff<TT_DATA_F>;            // 1024-bit X register buffer for F data
    using T_buff_Zbuff = T_InOut_W_buff<TT_DATA_G>;            // 256-bit W register buffer for G data
    using T_buff_FSig = T_InOut_W_buff<TT_DATA_F>;             // 256-bit W register buffer for F data
    using T_buff_GSig = T_InOut_W_buff<TT_DATA_G>;             // 256-bit W register buffer for G data
    using dataVecOut_t = ::aie::vector<TT_DATA_OUT, m_kLanes>; // Output vector type
    using T_vecF = ::aie::vector<TT_DATA_F, m_kVecLoadF>;      // F data load vector type
    T_vecF zeroVec = ::aie::zeros<TT_DATA_F, m_kVecLoadF>();   // Zero vector for F buffer padding

    T_accum acc;                                                  // Accumulator register
    T_buff_Xbuff xbuff;                                           // X(1024b) register buffer
    T_buff_Zbuff zbuff;                                           // W(256b) register buffer
    dataVecOut_t dataVecOut;                                      // Output vector
    dataVecOut_t dataOut = ::aie::zeros<TT_DATA_OUT, m_kLanes>(); // Zero vector for output lane masking
    unsigned int vecLenF = (unsigned int)inVecLen[0];             // RTP Based F_LEN
    unsigned int vecLenG = (unsigned int)inVecLen[1];             // RTP Based G_LEN

    constexpr unsigned int DataStepX = 1; // sliding_mul DataStepX
    constexpr unsigned int DataStepZ = 1; // sliding_mul DataStepZ
    constexpr unsigned int DataStepY = 1; // sliding_mul DataStepY

    // Runtime values derived from RTP vector lengths — computed in dependency order
    // so that getRuntimePaddedLength is called only once (start/end index reuse it).
    unsigned int kPaddedLenData = getRuntimePaddedLength<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE>(vecLenF, vecLenG);
    unsigned int kDataStartIndex =
        getRuntimeFdataStartIndex<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE>(vecLenF, vecLenG, kPaddedLenData);
    unsigned int kDataEndIndex = getRuntimeFdataEndIndex<TT_DATA_F, TT_DATA_G, TP_COMPUTE_MODE>(
        vecLenF, vecLenG, kPaddedLenData, kDataStartIndex);
    unsigned int kRuntimeLoopCount = getRuntimeLoopCount<TP_COMPUTE_MODE>(vecLenF, vecLenG);

    constexpr unsigned int Lanes = m_kLanes;
    constexpr unsigned int Points = m_kPoints;
    constexpr unsigned int kFLoadbits = getLog2<m_kVecLoadF>();               // Log2 of F vector load size
    constexpr unsigned int kInLoopLen = ROUND(m_kVecLoadF, m_kPoints);        // Inner loop unroll length
    constexpr unsigned int kGandFLoadRatio = FLOOR(m_kVecLoadG, m_kVecLoadF); // G-to-F load ratio
    const unsigned int kLoopCountDynamic =
        (CEIL(kRuntimeLoopCount, m_kLanes) / m_kLanes);           // Runtime output vector count
    constexpr unsigned int kLoopCountStatic = m_kMaxLoopCountVec; // Compile-time output vector count
    constexpr unsigned int kDataLoadofF = (kMaxBytesLoadOnAie / sizeof(TT_DATA_F));
    constexpr unsigned int kDataLoadofG = (kMaxBytesLoadOnAie / sizeof(TT_DATA_G));
    const unsigned int kFrameoffsetF = (kPaddedLenData / kDataLoadofF);        // Runtime F frame stride
    const unsigned int kAccumLen = ROUND(vecLenG, m_kVecLoadG);                // G accumulation length in vectors
    const unsigned int kFrameoffsetG = (vecLenG / kDataLoadofG);               // Runtime G frame stride
    const unsigned int kIncrOffsetGSig = ((vecLenG / kDataLoadofG) - 1);       // Index of last G vector
    const unsigned int kPaddedVecLoopCnt = ROUND(kPaddedLenData, m_kVecLoadF); // Padded F data length in vectors
    // MAC xbuff start offset accounting for runtime zero-padding
    const unsigned int kXStartIdxAdj =
        (TP_COMPUTE_MODE == SAME_MODE)
            ? ((kDataStartIndex * m_kVecLoadF) - (CEIL((vecLenG >> 1), kSameModeRoundFactor) - 1))
            : (TP_COMPUTE_MODE == VALID_MODE) ? 0u : ((kDataStartIndex * m_kVecLoadF) - (vecLenG - 1));
    // Output mask: precomputed once per call using runtime loop count
    const unsigned int kValidSamplesLast = kRuntimeLoopCount % m_kLanes;
    const uint32_t kLastVecMask = (kValidSamplesLast != 0) ? (~((1U << kValidSamplesLast) - 1U)) : 0U;

    // Signal I/O pointers
    T_buff_FSig* __restrict inPtrF = (T_buff_FSig*)inWindowF.data();     // F input read pointer
    T_buff_FSig* inDataPtrF = (T_buff_FSig*)paddedFdata;                 // Padded F data pointer
    T_buff_GSig* __restrict inDataPtrG = (T_buff_GSig*)inWindowG.data(); // G input read pointer
    dataVecOut_t* __restrict outPtr = (dataVecOut_t*)outWindow.data();   // Output write pointer

    // Base pointers for per-frame pointer resets
    T_buff_FSig* rdInDataFPtr = (T_buff_FSig*)paddedFdata;
    T_buff_GSig* __restrict rdInDataGPtr = (T_buff_GSig*)inWindowG.data();

    // Alias for sliding_mul API
    using mul_ops = ::aie::sliding_mul_ops<Lanes, Points, DataStepX, DataStepZ, DataStepY, TT_DATA_G, TT_DATA_F,
                                           tConvCorrAccType_t<TT_DATA_F, TT_DATA_G> >;

    for (unsigned int frameIndx = 0; frameIndx < TP_NUM_FRAMES; frameIndx++)
        chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            // Initialize pointer to start of current frame
            T_buff_FSig* paddedFdataPtr = (T_buff_FSig*)paddedFdata + (frameIndx * kFrameoffsetF);

            // ========== Zero padding before F data(0 to kDataStartIndex) ==========
            for (unsigned int dataIndx = 0; dataIndx < kDataStartIndex; dataIndx++) chess_prepare_for_pipelining {
                    *paddedFdataPtr++ = (T_buff_FSig)zeroVec; // Write zeroVec, increment paddedFdataPtr
                }

            // ========== Copy F data (kDataStartIndex to kDataEndIndex) ==========
            for (unsigned int dataIndx = kDataStartIndex; dataIndx < kDataEndIndex; dataIndx++)
                chess_prepare_for_pipelining {
                    *paddedFdataPtr++ = *inPtrF++; // Copy F data, increment both pointers
                }

            // ========== Zero padding after F data (kDataEndIndex to end) ==========
            for (unsigned int dataIndx = kDataEndIndex; dataIndx < kPaddedVecLoopCnt; dataIndx++)
                chess_prepare_for_pipelining {
                    *paddedFdataPtr++ = (T_buff_FSig)zeroVec; // Write zeroVec, increment paddedFdataPtr
                }

            // Reverse G data in-place for convolution
            if (TP_FUNCT_TYPE == CONV) {
                T_buff_GSig* __restrict inDataEndPtrG;                           // Pointer to last G vector
                constexpr int kVsize = (kMaxBytesLoadOnAie / sizeof(TT_DATA_G)); // Elements per vector
                using t_vect = ::aie::vector<TT_DATA_G, kVsize>;
                t_vect zbuffs, zbuffe; // Start and end vector registers for G reversal

                inDataPtrG = (rdInDataGPtr + (frameIndx * kFrameoffsetG));
                inDataEndPtrG = (inDataPtrG + kIncrOffsetGSig); // Point to last G vector

                t_vect* startPtr = (t_vect*)inDataPtrG;      // Read pointer at G start
                t_vect* endPtr = (t_vect*)inDataEndPtrG;     // Read pointer at G end
                t_vect* outStartPtrG = (t_vect*)inDataPtrG;  // Write pointer at G start
                t_vect* outEndPtrG = (t_vect*)inDataEndPtrG; // Write pointer at G end

                // Number of full vector swaps and partial vector flag
                const unsigned int kFullVecSwaps = (vecLenG / kVsize) >> 1;
                const unsigned int kRemElements = vecLenG % kVsize;
                const bool kPartialVec = (kRemElements > 0);

                // Swap and reverse full vector pairs from both ends toward center
                for (unsigned int i = 0; i < kFullVecSwaps; i++) chess_prepare_for_pipelining {
                        zbuffs = *startPtr++;
                        zbuffe = *endPtr--;
                        *outStartPtrG++ = ::aie::reverse(zbuffe);
                        *outEndPtrG-- = ::aie::reverse(zbuffs);
                    }

                // Handle center or partial vector (odd vector count or non-multiple vecLenG)
                if (kPartialVec || ((vecLenG / kVsize) & 1)) {
                    *outStartPtrG = ::aie::reverse(*startPtr);
                }
            }

            // Outer loop: one iteration per output vector
            for (unsigned int outIndx = 0; outIndx < kLoopCountDynamic; outIndx++) chess_prepare_for_pipelining {
                    // Reset F and G pointers for this output index
                    inDataPtrF =
                        (rdInDataFPtr + (outIndx * (m_kLanes >> kFLoadbits)) + (frameIndx * kPaddedVecLoopCnt));
                    inDataPtrG = (rdInDataGPtr + (frameIndx * kFrameoffsetG));

                    // Bounds check for 3rd/4th F vector loads, evaluated once per output vector
                    const unsigned int kInnerLoopFptrIncr = kAccumLen * kGandFLoadRatio + 1;
                    const unsigned int kThirdVecLoadOffset =
                        (kInnerLoopFptrIncr < kPaddedVecLoopCnt) ? (kPaddedVecLoopCnt - kInnerLoopFptrIncr) : 0;
                    // 4th-vector threshold is one less because the pre-increment advances one extra vector.
                    const unsigned int kFourthVecLoadOffset =
                        (kInnerLoopFptrIncr + 1 < kPaddedVecLoopCnt) ? (kPaddedVecLoopCnt - kInnerLoopFptrIncr - 1) : 0;

                    unsigned int baseOffset = (outIndx * (m_kLanes >> kFLoadbits));
                    bool load3rdVec = (baseOffset < kThirdVecLoadOffset);
                    bool load4thVec = (baseOffset < kFourthVecLoadOffset);

                    // Clear accumulator
                    acc.val = ::aie::zeros<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>, acc.val.size()>();

                    // kAccumLen is runtime-determined; unrolling here causes modulo-unroll bloat in Chess
                    for (unsigned int inLoopIndx = 0; inLoopIndx < kAccumLen; inLoopIndx++) {
                        // Load G vector
                        upd_W_buff(zbuff.val, 0, inDataPtrG++);
                        if
                            constexpr((TP_FUNCT_TYPE == CORR) && (isComplex<TT_DATA_G>())) {
                                zbuff.val = ::aie::conj(zbuff.val); // Conjugate G for correlation
                            }
// Load F vectors
#pragma unroll(kGandFLoadRatio)
                        for (unsigned int k = 0; k < kGandFLoadRatio; k++) {
                            // Load first two F vectors
                            upd_W_buff(xbuff.val, 0, inDataPtrF++);
                            upd_W_buff(xbuff.val, kBuffIndx1, inDataPtrF);

                            // Conditionally load 3rd and 4th F vectors based on precomputed bounds
                            if (load3rdVec) {
                                inDataPtrF++;
                                upd_W_buff(xbuff.val, kBuffIndx2, inDataPtrF);

                                if (load4thVec) {
                                    inDataPtrF++;
                                    upd_W_buff(xbuff.val, kBuffIndx3, inDataPtrF--);
                                }
                                inDataPtrF--; // Restore pointer for next iteration
                            }

#pragma unroll(kInLoopLen)
                            for (unsigned int l = 0; l < kInLoopLen; l++) {
                                acc.val = mul_ops::mac(acc.val, zbuff.val, ((k * kInLoopLen + l) * m_kPoints),
                                                       xbuff.val, ((l * m_kPoints) + kXStartIdxAdj));
                            } // End of kInLoopLen
                        }     // End of kGandFLoadRatio
                    }         // End of inner loop

                    dataVecOut = acc.val.template to_vector<TT_DATA_OUT>(TP_SHIFT);

                    // Mask partial lanes in the last output vector
                    if (kValidSamplesLast != 0) {
                        if (outIndx == (kLoopCountDynamic - 1)) {
                            dataVecOut =
                                ::aie::select(dataVecOut, dataOut, ::aie::mask<m_kLanes>::from_uint32(kLastVecMask));
                        }
                    }

                    *outPtr++ = dataVecOut;

                } // End of outer loop

            // Zero output vectors beyond the computed results to prevent stale data from previous iterations.
            for (unsigned int outIndx = kLoopCountDynamic; outIndx < kLoopCountStatic; outIndx++)
                chess_prepare_for_pipelining {
                    dataVecOut_t zeroOutVec = ::aie::zeros<TT_DATA_OUT, m_kLanes>();
                    *outPtr++ = zeroOutVec;
                }
        } // End of Frames
};        // End of conv_corrRTP()

#if (__HAS_ACCUM_PERMUTES__ == 1)
// Conv-Corr - stream specialization

// First Kernel of the cascade
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
NOINLINE_DECL void
conv_corr<TT_DATA_F,
          TT_DATA_G,
          TT_DATA_OUT,
          TP_FUNCT_TYPE,
          VALID_MODE /* stream supports only Valid Mode*/,
          TP_F_LEN,
          TP_G_LEN,
          TP_SHIFT,
          USE_STREAM_API /*For Stream TP_API is 1*/,
          TP_RND,
          TP_SAT,
          TP_NUM_FRAMES,
          TP_CASC_LEN,
          TP_PHASES,
          TP_KERNEL_POSITION,
          TP_PH_POSITION,
          CASC_IN_FALSE,
          CASC_OUT_TRUE,
          USE_RTP_VECTOR_LENGTHS_FALSE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                       input_stream<TT_DATA_F>* __restrict instream2F,
                                                       input_buffer<TT_DATA_G>& __restrict inWindowG,
                                                       output_cascade<cacc48>* __restrict outcascade) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kCoeffShuffle = ((kPoints > kNumPoints2) && (kStreamsPerCore > 1)) ? 1 : 0;
    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO_STREAMS) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = (kMuls * kCores);
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kLoopIncr =
        (kMacsPerCore == 1) ? kMaxSamplesInShuffleVec : (kMacsPerCore << MAC4ROTDELAY); // Loop increment
    constexpr unsigned int kDataBuffLen = ((kMacsPerCore * kPoints) < kDataBuffLenFactor)
                                              ? kMinDataBuffLen
                                              : ((kMacsPerCore * kPoints) << kShiftFactor2); // Data Buffer Length

    constexpr unsigned int kLoopCount = (CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen);
    constexpr unsigned int kPhaseIncr = (TP_PHASES >> (kStreamsPerCore - 1));
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kMuls * kMacsPerCore) >> (kStreamsPerCore - 1);
    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> kShiftFactor2) - 1);

    constexpr unsigned int kCoeffLoadShuffle = (((kPoints >> 1) - 1) * kCoeffShuffle);
    constexpr unsigned int kCoeffLoadShift = ((kStreamsPerCore - 1) + kCoeffLoadShuffle);

    constexpr unsigned int kBaseOffset = kStreamPhaseOffset;
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = ((kStreamsPerCore - 1) << kShiftFactor2);

    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << kShiftFactor2)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
    constexpr unsigned int kPhaseDelayComponent =
        (MAC4ROTDELAY * ((TP_KERNEL_POSITION >> (kPhaseIncr - 1)) - 1) * (kPhaseIncr - 1) +
         (MAC4ROTDELAY * (TP_KERNEL_POSITION & (kPhaseIncr - 1))));

    constexpr int kCascDelay = kCascDelayComponent + kPhaseDelayComponent;
    constexpr int kCascDelay1 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1))))
            ? (kCascDelay - 1)
            : kCascDelay;
    constexpr unsigned int kCascDelay2 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1)) - 1))
            ? (kCascDelay - 1)
            : kCascDelay;
    constexpr int kCascVecSizeOfCoeff = kMaxSamplesInShuffleVec / sizeof(TT_DATA_G);
    constexpr unsigned int kShuffleVecSize = (kMaxSamplesInShuffleVec / fnSizeOfDataType<cint16>());
    unsigned int xstart;
    unsigned int zstart;
    int xstep = kXstepOfMac4Rot;
    int zstep = kMacStepIncr;

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, (kLanes << 1)>;
    using T_accum_hi = ::aie::accum<cacc48, kLanes>;

    using T_buff_GSig_vect = ::aie::vector<TT_DATA_G, kCascVecSizeOfCoeff>; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;
    int rot = 1;

    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi acc4, vAcc;
    T_buff_GSig_vect tempBuff;
    T_buff_GSig_vect* __restrict vInDataPtrG = (T_buff_GSig_vect*)inWindowG.data(); // Input pointer for Sig_G
    T_buff_zbuff zbuff[kMacsPerCore];
    T_buff_GSig_vect* tempPtr = (T_buff_GSig_vect*)gReArrangeBuff;

    // consuming samples from stream to adjust delay
    if (doInit == 1) {
        for (unsigned int del = 0; del < -kCascDelay1; del++) chess_prepare_for_pipelining {
                readincr(instream1F);
            }
        for (unsigned int del = 0; del < -kCascDelay2; del++) chess_prepare_for_pipelining {
                readincr(instream2F);
            }
    }

    // Coefficients are stored in xbuffer
    gDataReArrange<TT_DATA_F, TT_DATA_G, TP_PHASES, TP_G_LEN, TP_FUNCT_TYPE, kStreamsPerCore>(vInDataPtrG, tempPtr);

    for (unsigned int i = 0;
         i < ((kLanes * kPoints) * (TP_CASC_LEN - TP_KERNEL_POSITION - 1) * kMacsPerCore) / kCascVecSizeOfCoeff; i++) {
        tempBuff = *tempPtr++;
        vAcc.from_vector(::aie::vector_cast<cint16>(tempBuff), 0);
        writeincr(outcascade, vAcc);
    }

    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / kCascVecSizeOfCoeff; j++) {
            tempBuff = *tempPtr++;
            zbuff[i].val.insert(j, tempBuff);
        }
    }

#define MAC_LOOP_FIRST_KERNEL(os)                                                                                \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                            \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                         \
        if (k == 0) {                                                                                            \
            acc = mac4_rot(acc, rot, xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, kZoffset, zstep);            \
        } else {                                                                                                 \
            acc4 = mac4(acc.template extract<kShuffleVecSize>(1), xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, \
                        kZoffset, zstep);                                                                        \
            acc.insert(1, acc4);                                                                                 \
        }                                                                                                        \
    }                                                                                                            \
    writeincr(outcascade, acc.template extract<kShuffleVecSize>(1));

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kLoopCount) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += kDataBuffLenFactor) {
                readStream(xbuff, ((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO_STREAMS) {
                    readStream(xbuff, (((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll,
                               1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + (MAC4ROTDELAY - 1));
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + MAC4ROTDELAY);
            }
        }
    doInit = 0;
    *vPtrDelayLineAcc = acc;
    *vPtrDelayLine = xbuff;

} // End of conv_corrMain() - First Kernel of cascade for stream

// Conv-Corr - stream specialization
// Intermediate Kernels of the cascade
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
NOINLINE_DECL void
conv_corr<TT_DATA_F,
          TT_DATA_G,
          TT_DATA_OUT,
          TP_FUNCT_TYPE,
          VALID_MODE /* stream supports only Valid Mode*/,
          TP_F_LEN,
          TP_G_LEN,
          TP_SHIFT,
          USE_STREAM_API /*For Stream TP_API is 1*/,
          TP_RND,
          TP_SAT,
          TP_NUM_FRAMES,
          TP_CASC_LEN,
          TP_PHASES,
          TP_KERNEL_POSITION,
          TP_PH_POSITION,
          CASC_IN_TRUE,
          CASC_OUT_TRUE,
          USE_RTP_VECTOR_LENGTHS_FALSE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                       input_stream<TT_DATA_F>* __restrict instream2F,
                                                       input_cascade<cacc48>* __restrict incascade,
                                                       output_cascade<cacc48>* __restrict outcascade) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = fnNumOfLanesForMac4Rot<TT_DATA_F>(); // Num of Lanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                   // Num of Points
    constexpr unsigned int kMuls = (kLanes * kPoints);                   // Num of Muls for MAC4_ROT()
    constexpr unsigned int kStreamPerCoreVar =
        ((m_kMuls * TP_PHASES) >> 1); // a factor to compute how many stream per Core.
    constexpr unsigned int kStreamsPerCore =
        (TP_G_LEN > kStreamPerCoreVar) ? 1 : kMaxNumOfStreams; // Selection of Streams needs to be processed.

    constexpr unsigned int kCoeffShuffle =
        (kPoints > kNumPoints2 && kStreamsPerCore > 1) ? 1 : 0; // factor to shuffle given coeff.
    constexpr unsigned int kMacOffsetIncr =
        (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // Offset to increase to get next MAC;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO_STREAMS) ? 1 : kLanes; // Increment step for each MAC
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;                             // Number of Cores

    constexpr unsigned int kMaxMuls = m_kMuls * kCores;                          // Maximum Number of MULS
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls; // number of MACS per Core
    constexpr unsigned int kDataBuffLen = ((kMacsPerCore * kPoints) < kDataBuffLenFactor)
                                              ? kMinDataBuffLen
                                              : ((kMacsPerCore * kPoints) << kShiftFactor2); // Data Buffer Length

    constexpr unsigned int kLoopCount = CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen; // Loopcount
    constexpr unsigned int kPhaseIncr = TP_PHASES >> (kStreamsPerCore - 1); // offset to increase Phase.
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kMuls * kMacsPerCore) >> (kStreamsPerCore - 1);

    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> kShiftFactor2) - 1);

    constexpr unsigned int kCoeffLoadShuffle = ((kPoints >> 1) - 1) * kCoeffShuffle;
    constexpr unsigned int kCoeffLoadShift = (kStreamsPerCore - 1) + kCoeffLoadShuffle;

    constexpr unsigned int kBaseOffset = kStreamPhaseOffset;
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = (kStreamsPerCore - 1) << kShiftFactor2;
    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << kShiftFactor2)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
    constexpr unsigned int kPhaseDelayComponent =
        MAC4ROTDELAY * ((TP_KERNEL_POSITION >> (kPhaseIncr - 1)) - 1) * (kPhaseIncr - 1) +
        MAC4ROTDELAY * (TP_KERNEL_POSITION & (kPhaseIncr - 1));
    constexpr unsigned int kCascDelay = kCascDelayComponent + kPhaseDelayComponent;
    constexpr unsigned int kCascDelay1 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1))))
            ? (kCascDelay - 1)
            : kCascDelay;
    constexpr unsigned int kCascDelay2 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1)) - 1))
            ? (kCascDelay - 1)
            : kCascDelay;

    constexpr unsigned int kShuffleVecSize = (kMaxSamplesInShuffleVec / fnSizeOfDataType<cint16>());
    unsigned int xstart;
    unsigned int zstart;
    int xstep = kXstepOfMac4Rot;
    int zstep = kMacStepIncr;

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, (kLanes << 1)>;
    using T_accum_hi = ::aie::accum<cacc48, kLanes>;

    using T_buff_GSig = TT_DATA_G; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;

    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi cascVect, acc4;
    ::aie::vector<cint16, kShuffleVecSize> zbuffLdcascVect;
    T_buff_zbuff zbuff[kMacsPerCore];
    int rot = 1;

    // consuming samples from stream to adjust delay
    if (doInit == 1) {
        for (unsigned int del = 0; del < -kCascDelay1; del++) chess_prepare_for_pipelining {
                readincr(instream1F);
            }
        for (unsigned int del = 0; del < -kCascDelay2; del++) chess_prepare_for_pipelining {
                readincr(instream2F);
            }
    }

    // Coefficients are stored in xbuffer
    for (unsigned int i = 0;
         i < (kLanes * kPoints) * (TP_CASC_LEN - TP_KERNEL_POSITION - 1) * kMacsPerCore / kCascVecSizeOfCoeff; i++) {
        cascVect = readincr_v<kShuffleVecSize>(incascade);
        writeincr(outcascade, cascVect);
    }

    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / kCascVecSizeOfCoeff; j++) {
            acc4 = readincr_v<kShuffleVecSize>(incascade);
            zbuffLdcascVect = acc4.template to_vector<cint16>(0);
            zbuff[i].val.insert(j, ::aie::vector_cast<TT_DATA_G>(zbuffLdcascVect));
        }
    }

#define MAC_LOOP_INTERMEDIATE_KERNEL(os)                                                                         \
    cascVect = readincr_v<kShuffleVecSize>(incascade);                                                           \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                            \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                         \
        if (k == 0) {                                                                                            \
            acc = mac4_rot(acc, cascVect, rot, xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, kZoffset, zstep);  \
        } else {                                                                                                 \
            acc4 = mac4(acc.template extract<kShuffleVecSize>(1), xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, \
                        kZoffset, zstep);                                                                        \
            acc.insert(1, acc4);                                                                                 \
        }                                                                                                        \
    }                                                                                                            \
    writeincr(outcascade, acc.template extract<kShuffleVecSize>(1));

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kLoopCount) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += kDataBuffLenFactor) {
                readStream(xbuff, ((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO_STREAMS) {
                    readStream(xbuff, (((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll,
                               1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + (MAC4ROTDELAY - 1));
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + MAC4ROTDELAY);
            }
        }
    doInit = 0;
    *vPtrDelayLineAcc = acc;
    *vPtrDelayLine = xbuff;
} // End of conv_corrMain() - Intermediate Kernels of cascade for stream

// Conv-Corr - stream specialization
// Last Kernel of the cascade
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
NOINLINE_DECL void
conv_corr<TT_DATA_F,
          TT_DATA_G,
          TT_DATA_OUT,
          TP_FUNCT_TYPE,
          VALID_MODE /* stream supports only Valid Mode*/,
          TP_F_LEN,
          TP_G_LEN,
          TP_SHIFT,
          USE_STREAM_API /*For Stream TP_API is 1*/,
          TP_RND,
          TP_SAT,
          TP_NUM_FRAMES,
          TP_CASC_LEN,
          TP_PHASES,
          TP_KERNEL_POSITION,
          TP_PH_POSITION,
          CASC_IN_TRUE,
          CASC_OUT_FALSE,
          USE_RTP_VECTOR_LENGTHS_FALSE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                       input_stream<TT_DATA_F>* __restrict instream2F,
                                                       input_cascade<cacc48>* __restrict incascade,
                                                       output_stream<TT_DATA_OUT>* __restrict outstream) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = fnNumOfLanesForMac4Rot<TT_DATA_F>(); // Num of kLanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                   // m_kPoints;   // Num of kPoints
    constexpr unsigned int kStreamPerCoreVar = ((kLanes * kPoints * TP_PHASES) >> 1);
    constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kStreamPerCoreVar) ? 1 : kMaxNumOfStreams;
    constexpr unsigned int kCoeffShuffle = ((kPoints > kNumPoints2) && (kStreamsPerCore > 1)) ? 1 : 0;

    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // TP_PHASES;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO_STREAMS) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = kLanes * kPoints * kCores;
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kDataBuffLen = ((kMacsPerCore * kPoints) < kDataBuffLenFactor)
                                              ? kMinDataBuffLen
                                              : ((kMacsPerCore * kPoints) << kShiftFactor2);       // Data Buffer Length
    constexpr unsigned int kLoopCount = CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen; // Loopcount
    constexpr unsigned int kPhaseIncr = TP_PHASES >> (kStreamsPerCore - 1);
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kLanes * kPoints * kMacsPerCore) >> (kStreamsPerCore - 1);

    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> kShiftFactor2) - 1);
    constexpr unsigned int kCoeffLoadShuffle = ((kPoints >> 1) - 1) * kCoeffShuffle;
    constexpr unsigned int kCoeffLoadShift = (kStreamsPerCore - 1) + kCoeffLoadShuffle;

    constexpr unsigned int kBaseOffset = kStreamPhaseOffset;
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = (kStreamsPerCore - 1) << kShiftFactor2;
    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << kShiftFactor2)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
    constexpr unsigned int kPhaseDelayComponent =
        MAC4ROTDELAY * ((TP_KERNEL_POSITION >> (kPhaseIncr - 1)) - 1) * (kPhaseIncr - 1) +
        MAC4ROTDELAY * (TP_KERNEL_POSITION & (kPhaseIncr - 1));
    constexpr unsigned int kCascDelay = kCascDelayComponent + kPhaseDelayComponent;
    constexpr unsigned int kCascDelay1 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1))))
            ? (kCascDelay - 1)
            : kCascDelay;
    constexpr unsigned int kCascDelay2 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1)) - 1))
            ? (kCascDelay - 1)
            : kCascDelay;
    unsigned int xstart;
    unsigned int zstart;
    int xstep = kXstepOfMac4Rot;
    int zstep = kMacStepIncr;

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, (kLanes << 1)>;
    using T_accum_hi = ::aie::accum<cacc48, kLanes>;

    constexpr int kCascVecSizeOfCoeff =
        kMaxSamplesInShuffleVec / sizeof(TT_DATA_G); // shuffle operates on v16, int16- v8-2 loads, v16
    constexpr unsigned int kShuffleVecSize = (kMaxSamplesInShuffleVec / fnSizeOfDataType<cint16>());
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;
    using dataoutVect_t = ::aie::vector<TT_DATA_OUT, kLanes>;
    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi cascVect, acc_lo, acc4;
    ::aie::vector<cint16, kShuffleVecSize> zbuffLdcascVect;
    T_buff_zbuff zbuff[kMacsPerCore];
    dataoutVect_t dataVecOut;
    int rot = 1;

    // consuming samples from stream to adjust delayBuff
    if (doInit == 1) {
        for (unsigned int del = 0; del < -kCascDelay1; del++) chess_prepare_for_pipelining {
                readincr(instream1F);
            }
        for (unsigned int del = 0; del < -kCascDelay2; del++) chess_prepare_for_pipelining {
                readincr(instream2F);
            }
    }

    // Coefficients are stored in xbuffer
    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / kCascVecSizeOfCoeff; j++) {
            acc4 = readincr_v<kShuffleVecSize>(incascade);
            zbuffLdcascVect = acc4.template to_vector<cint16>(0);
            zbuff[i].val.insert(j, ::aie::vector_cast<TT_DATA_G>(zbuffLdcascVect));
        }
    }

#define MAC_LOOP_LAST_KERNEL(os)                                                                                 \
    cascVect = readincr_v<kLanes>(incascade);                                                                    \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                            \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                         \
        if (k == 0) {                                                                                            \
            acc = mac4_rot(acc, cascVect, rot, xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, kZoffset, zstep);  \
        } else {                                                                                                 \
            acc4 = mac4(acc.template extract<kShuffleVecSize>(1), xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, \
                        kZoffset, zstep);                                                                        \
            acc.insert(1, acc4);                                                                                 \
        }                                                                                                        \
    }

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kLoopCount) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += kDataBuffLenFactor) {
                readStream(xbuff, ((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO_STREAMS) {
                    readStream(xbuff, (((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll,
                               1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + (MAC4ROTDELAY - 1));
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + MAC4ROTDELAY);

                acc_lo = acc.template extract<kShuffleVecSize>(0);
                dataVecOut = acc_lo.template to_vector<TT_DATA_OUT>(TP_SHIFT);
                writeincr(outstream, dataVecOut);
            }
        }
    doInit = 0;
    *vPtrDelayLineAcc = acc;
    *vPtrDelayLine = xbuff;
} // End of conv_corrMain() - Last Kernel of cascade for stream

// Single Kernel per phase
template <typename TT_DATA_F,
          typename TT_DATA_G,
          typename TT_DATA_OUT,
          unsigned int TP_FUNCT_TYPE,
          unsigned int TP_F_LEN,
          unsigned int TP_G_LEN,
          unsigned int TP_SHIFT,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_NUM_FRAMES,
          unsigned int TP_CASC_LEN,
          unsigned int TP_PHASES,
          unsigned int TP_KERNEL_POSITION,
          unsigned int TP_PH_POSITION>
NOINLINE_DECL void
conv_corr<TT_DATA_F,
          TT_DATA_G,
          TT_DATA_OUT,
          TP_FUNCT_TYPE,
          VALID_MODE /* stream supports only Valid Mode*/,
          TP_F_LEN,
          TP_G_LEN,
          TP_SHIFT,
          USE_STREAM_API /*For Stream TP_API is 1*/,
          TP_RND,
          TP_SAT,
          TP_NUM_FRAMES,
          TP_CASC_LEN,
          TP_PHASES,
          TP_KERNEL_POSITION,
          TP_PH_POSITION,
          CASC_IN_FALSE,
          CASC_OUT_FALSE,
          USE_RTP_VECTOR_LENGTHS_FALSE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                       input_stream<TT_DATA_F>* __restrict instream2F,
                                                       input_buffer<TT_DATA_G>& __restrict inWindowG,
                                                       output_stream<TT_DATA_OUT>* __restrict outstream) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = fnNumOfLanesForMac4Rot<TT_DATA_F>(); // Num of kLanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                   // m_kPoints;   // Num of kPoints
    constexpr unsigned int kStreamPerCoreVar = ((kLanes * kPoints * TP_PHASES) >> 1);
    constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kStreamPerCoreVar) ? 1 : kMaxNumOfStreams;
    constexpr unsigned int kCoeffShuffle = ((kPoints > kNumPoints2) && (kStreamsPerCore > 1)) ? 1 : 0;

    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // TP_PHASES;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO_STREAMS) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = kLanes * kPoints * kCores;
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kDataBuffLen = (kMacsPerCore * kPoints < kDataBuffLenFactor)
                                              ? kMinDataBuffLen
                                              : ((kMacsPerCore * kPoints) << kShiftFactor2);       // Data Buffer Length
    constexpr unsigned int kLoopCount = CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen; // Loopcount
    constexpr unsigned int kPhaseIncr = TP_PHASES >> (kStreamsPerCore - 1);
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kLanes * kPoints * kMacsPerCore) >> (kStreamsPerCore - 1);

    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> kShiftFactor2) - 1);
    constexpr unsigned int kCoeffLoadShuffle = ((kPoints >> 1) - 1) * kCoeffShuffle;
    constexpr unsigned int kCoeffLoadShift = (kStreamsPerCore - 1) + kCoeffLoadShuffle;

    constexpr unsigned int kBaseOffset = kStreamPhaseOffset;
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = (kStreamsPerCore - 1) << kShiftFactor2;
    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << kShiftFactor2)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
    constexpr unsigned int kPhaseDelayComponent =
        MAC4ROTDELAY * ((TP_KERNEL_POSITION >> (kPhaseIncr - 1)) - 1) * (kPhaseIncr - 1) +
        MAC4ROTDELAY * (TP_KERNEL_POSITION & (kPhaseIncr - 1));
    constexpr unsigned int kCascDelay = kCascDelayComponent + kPhaseDelayComponent;
    constexpr unsigned int kCascDelay1 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1))))
            ? (kCascDelay - 1)
            : kCascDelay;
    constexpr unsigned int kCascDelay2 =
        ((TP_PH_POSITION + 1) > (TP_PHASES - kStreamsPerCore * (TP_KERNEL_POSITION & (kPhaseIncr - 1)) - 1))
            ? (kCascDelay - 1)
            : kCascDelay;
    constexpr int kCascVecSizeOfCoeff = kMaxSamplesInShuffleVec / sizeof(TT_DATA_G);
    constexpr unsigned int kShuffleVecSize = (kMaxSamplesInShuffleVec / fnSizeOfDataType<cint16>());
    unsigned int xstart;
    unsigned int zstart;
    int xstep = kXstepOfMac4Rot;
    int zstep = kMacStepIncr;

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, (kLanes << 1)>;
    using T_accum_hi = ::aie::accum<cacc48, kLanes>;
    using T_buff_GSig_vect = ::aie::vector<TT_DATA_G, kCascVecSizeOfCoeff>; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;
    using dataoutVect_t = ::aie::vector<TT_DATA_OUT, kLanes>;
    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi acc_lo, acc4;

    T_buff_GSig_vect tempBuff;
    T_buff_GSig_vect* __restrict vInDataPtrG = (T_buff_GSig_vect*)inWindowG.data(); // Input pointer for Sig_G
    T_buff_zbuff zbuff[kMacsPerCore];
    T_buff_GSig_vect* tempPtr = (T_buff_GSig_vect*)gReArrangeBuff;

    dataoutVect_t dataVecOut;
    int rot = 1;

    // consuming samples from stream to adjust delayBuff
    if (doInit == 1) {
        for (unsigned int del = 0; del < -kCascDelay1; del++) chess_prepare_for_pipelining {
                readincr(instream1F);
            }
        for (unsigned int del = 0; del < -kCascDelay2; del++) chess_prepare_for_pipelining {
                readincr(instream2F);
            }
    }

    // Coefficients are stored in xbuffer
    gDataReArrange<TT_DATA_F, TT_DATA_G, TP_PHASES, TP_G_LEN, TP_FUNCT_TYPE, kStreamsPerCore>(vInDataPtrG, tempPtr);

    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / kCascVecSizeOfCoeff; j++) {
            tempBuff = *tempPtr++;
            zbuff[i].val.insert(j, tempBuff);
        }
    }

#define MAC_LOOP_SINGLE_KERNEL(os)                                                                               \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                            \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                         \
        if (k == 0) {                                                                                            \
            acc = mac4_rot(acc, rot, xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, kZoffset, zstep);            \
        } else {                                                                                                 \
            acc4 = mac4(acc.template extract<kShuffleVecSize>(1), xbuff, xstart, 0x0000, xstep, zbuff[k].val, 0, \
                        kZoffset, zstep);                                                                        \
            acc.insert(1, acc4);                                                                                 \
        }                                                                                                        \
    }

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kLoopCount) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += kDataBuffLenFactor) {
                readStream(xbuff, ((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO_STREAMS) {
                    readStream(xbuff, (((inLoopIndx >> kShiftFactor2) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll,
                               1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + (MAC4ROTDELAY - 1));
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + MAC4ROTDELAY);

                acc_lo = acc.template extract<kShuffleVecSize>(0);
                dataVecOut = acc_lo.template to_vector<TT_DATA_OUT>(TP_SHIFT);
                writeincr(outstream, dataVecOut);
            }
        }
    doInit = 0;
    *vPtrDelayLineAcc = acc;
    *vPtrDelayLine = xbuff;
} // End of conv_corrMain() - Single Kernel of cascade for stream

#endif

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {
