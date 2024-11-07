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
CONV_CORR kernal code.
This file captures the body of run-time code for the kernal class.
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
// Convolution/Correlation Kernel Class
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
          bool TP_CASC_OUT>
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
                             TP_CASC_OUT>::conv_corrMain(input_buffer<TT_DATA_F>& __restrict inWindowF,
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

    T_buff_FSig sigF;        // Declaration of Sig F
    T_buff_GSig sigG;        // Declaration of Sig G
    T_accum acc;             // Declaration of Accumulator
    T_buff_Xbuff xbuff;      // Declaration of xbuff using X(1024b) Reg.
    T_buff_Zbuff zbuff;      // Declaration of zbuff using W(256b) Reg.
    dataVecOut_t dataVecOut; // Declaration of Output Vector to store results into Out buffer.

    constexpr unsigned int DataStepX = ONE;                     // Initilization of slidingmul parameter i.e. DataStepX
    constexpr unsigned int DataStepZ = ONE;                     // Initilization of slidingmul parameter i.e. DataStepZ
    constexpr unsigned int DataStepY = ONE;                     // Initilization of slidingmul parameter i.e. DataStepY
    constexpr unsigned int Lanes = m_kLanes;                    // Num of Lanes
    constexpr unsigned int Points = m_kPoints;                  // Num of Points
    constexpr unsigned int kFLoadbits = getLog2<m_kVecLoadF>(); // No of bits for FLoad
    constexpr unsigned int kAccumLen = ROUND(TP_G_LEN, m_kVecLoadG);                 // Len. of Accumulator
    constexpr unsigned int kInLoopLen = ROUND(m_kVecLoadF, m_kPoints);               // In loop Length
    constexpr unsigned int kGandFLoadRatio = FLOOR(m_kVecLoadG, m_kVecLoadF);        // Ratio of G load to F Load
    constexpr unsigned int kLoopCount = (CEIL(m_kLoopCount, m_kLanes) / m_kLanes);   // Loopcount
    constexpr unsigned int kFrameoffsetF = (m_kPaddedLenData / sigF.getSizeOfVec()); // Offset of F ptr
    constexpr unsigned int kFrameoffsetG = (TP_G_LEN / sigG.getSizeOfVec());         // Offset of F ptr
    constexpr unsigned int kIncrOffsetGSig = getOffsetGsig<TT_DATA_G, TP_G_LEN>();   // Offset to increment G Sig
    unsigned int SwapCount = (ROUND(TP_G_LEN, m_kVecLoadG) >> 1); // Count of elements in G-signal to reverse them

    T_buff_FSig* __restrict inDataPtrF = (T_buff_FSig*)inWindowF.data(); // Input pointer for Sig_F
    T_buff_GSig* __restrict inDataPtrG = (T_buff_GSig*)inWindowG.data(); // Input pointer for Sig_G
    dataVecOut_t* __restrict outPtr = (dataVecOut_t*)outWindow.data();   // Output Pointer for conv/corr result

    T_buff_FSig* __restrict rdInDataFPtr = (T_buff_FSig*)inWindowF.data(); // Alias Input pointer for Sig_F
    T_buff_GSig* __restrict rdInDataGPtr = (T_buff_GSig*)inWindowG.data(); // Alias Input pointer for Sig_G

    T_buff_FSig* __restrict inFPtrPerFrame = (T_buff_FSig*)inWindowF.data(); // Alias Input pointer for Sig_F
    T_buff_GSig* __restrict inGPtrPerFrame = (T_buff_GSig*)inWindowG.data(); // Alias Input pointer for Sig_G

    // Alias for sliding_mul API
    using mul_ops = ::aie::sliding_mul_ops<Lanes, Points, DataStepX, DataStepZ, DataStepY, TT_DATA_G, TT_DATA_F,
                                           tConvCorrAccType_t<TT_DATA_F, TT_DATA_G> >;

    for (int frameIndx = 0; frameIndx < TP_NUM_FRAMES; frameIndx++)
        chess_prepare_for_pipelining chess_loop_count(TP_NUM_FRAMES) {
            if (TP_FUNCT_TYPE == 1) // function type is "CONV"
            {
                T_buff_GSig* __restrict inDataEndPtrG; // Local Pointer to fetch end address of G data
                T_buff_Zbuff zbuffs, zbuffe;           // Registers to hold Start and End data samples of G sig.
                constexpr int kVsize = maxBytesLoadOnAie() /
                                       sizeof(TT_DATA_G); // Size of vector (256/8/sizeof(DATA) as per given data type.
                using t_vect = ::aie::vector<TT_DATA_G, kVsize>; // definition using the vec size.

                inDataPtrG = (rdInDataGPtr + (frameIndx * kFrameoffsetG));
                inDataEndPtrG = (inDataPtrG + kIncrOffsetGSig); // Move the pointer to End address of G sig.

                t_vect* outStartPtrG = (t_vect*)
                    inDataPtrG; // Output pointer pointing to Start of G buffer, used to write the swapped data of G sig
                t_vect* outEndPtrG = (t_vect*)inDataEndPtrG; // Output pointer pointing to End of G buffer, used to
                                                             // write the swapped data of G sig

                // reverse the G elements using inplace memory for Conv.
                for (unsigned int i = 0; i < SwapCount; i++) chess_prepare_for_pipelining chess_loop_count(EIGHT) {
                        upd_W_buff(zbuffs.val, ZERO, inDataPtrG++);    // Load from starting postion and increment
                        upd_W_buff(zbuffe.val, ZERO, inDataEndPtrG--); // Load from End position and decrement

                        // Reverse the data fetched and store in respective locations
                        zbuffs.val = (::aie::reverse(zbuffs.val));
                        zbuffe.val = (::aie::reverse(zbuffe.val));

                        *outStartPtrG++ = zbuffe.val; // Storing the data after swap to same G sig buffer.
                        *outEndPtrG-- = zbuffs.val;   // Storing the data after swap to same G sig buffer.
                    }
            }

            // Outer Loop to do Convolution/Correlation
            for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
                chess_prepare_for_pipelining chess_loop_count(UNROLL_8) {
                    // Updation of F and G pointer to fetch respective data from both signal
                    inDataPtrF = (rdInDataFPtr + (outIndx * (m_kLanes >> kFLoadbits)) +
                                  (frameIndx * kFrameoffsetF));                // Reset F Sig Pointer.
                    inDataPtrG = (rdInDataGPtr + (frameIndx * kFrameoffsetG)); // Reset G Sig Pointer.

                    // Initialization of Accumulator with Zeros to flush the previous data.
                    acc.val = ::aie::zeros<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>,
                                           acc.val.size()>(); // null all the elements of accumulator

#pragma unroll(UNROLL_16)
                    for (unsigned int inLoopIndx = 0; inLoopIndx < kAccumLen;
                         inLoopIndx++) // Inner Loop to do conv/corr using F and G Data.
                    {
                        // Load Data of G Signal
                        upd_W_buff(zbuff.val, ZERO, inDataPtrG++); // Fetch the G data to Registrer zbuff.
                        if
                            constexpr((TP_FUNCT_TYPE == 0) && (isComplex<TT_DATA_G>())) {
                                zbuff.val = ::aie::conj(zbuff.val);
                            }
// Load Data of F Signal
#pragma unroll(kGandFLoadRatio)
                        for (unsigned int k = 0; k < kGandFLoadRatio; k++) {
                            // Pointer Manipulation to read No of "FLoad" elements into Y_REG_BITS buffer outIndx.e.
                            // xbuff
                            upd_W_buff(xbuff.val, ZERO,
                                       inDataPtrF++); // update xbuff with F sig data based on vector load.
                            upd_W_buff(xbuff.val, ONE,
                                       inDataPtrF);     // update xbuff with F sig data based on vector load.
                            if (m_kLanes > m_kVecLoadF) // m_kVecLoad_Len_F is nothing but FLoad
                            {
                                inDataPtrF++;
                                upd_W_buff(xbuff.val, TWO,
                                           inDataPtrF--); // Fetch the data when kLanes greater than F Vector load
                            }

#pragma unroll(kInLoopLen)
                            for (unsigned int l = 0; l < kInLoopLen; l++) {
                                // Sliding Multiplication of given signals
                                acc.val = mul_ops::mac(acc.val, zbuff.val, (k * kInLoopLen + l) * m_kPoints, xbuff.val,
                                                       l * m_kPoints); // Sliding MAC function Call

                            } // End of kInLoopLen
                        }     // End of kGandFLoadRatio
                    }         // End of InnerLoop

                    dataVecOut = acc.val.template to_vector<TT_DATA_OUT>(
                        TP_SHIFT);          // Storing accumulator results into out data vector
                    *outPtr++ = dataVecOut; // write the output results to the iobuffer

                } // End of OuterLoop
        }         // End of Frames
};                // End of conv_corrMain()

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
NOINLINE_DECL void conv_corr<TT_DATA_F,
                             TT_DATA_G,
                             TT_DATA_OUT,
                             TP_FUNCT_TYPE,
                             TP_COMPUTE_MODE_IS_VALID_MODE(),
                             TP_F_LEN,
                             TP_G_LEN,
                             TP_SHIFT,
                             TP_API_IS_ONE(),
                             TP_RND,
                             TP_SAT,
                             TP_NUM_FRAMES,
                             TP_CASC_LEN,
                             TP_PHASES,
                             TP_KERNEL_POSITION,
                             TP_PH_POSITION,
                             CASC_IN_FALSE,
                             CASC_OUT_TRUE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                           input_stream<TT_DATA_F>* __restrict instream2F,
                                                           input_buffer<TT_DATA_G>& __restrict inWindowG,
                                                           output_cascade<cacc48>* __restrict outcascade) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of kLanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                       // Num of kPoints
    constexpr unsigned int kstreampercore_var = ((kLanes * kPoints * TP_PHASES) >> 1);
    constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kstreampercore_var) ? 1 : maxNumOfStreams();

    constexpr unsigned int kCoeffShuffle = (kPoints > TWO && kStreamsPerCore > 1) ? 1 : 0;
    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // TP_PHASES;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = (kLanes * kPoints * kCores);
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kLoopIncr = (kMacsPerCore == 1) ? SIXTEEN : (kMacsPerCore << MAC4ROTDELAY); // Loop increment
    constexpr unsigned int kDataBuffLen = ((kMacsPerCore * kPoints) < dataBuffLenFactor())
                                              ? minDataBuffLen()
                                              : ((kMacsPerCore * kPoints) << mulFactor2()); // Data Buffer Length

    constexpr unsigned int kLoopCount = (CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen); // Loopcount
    constexpr unsigned int kPhaseIncr = (TP_PHASES >> (kStreamsPerCore - 1));
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kLanes * kPoints * kMacsPerCore) >> (kStreamsPerCore - 1);
    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> TWO) - 1);

    constexpr unsigned int kCoeffLoadShuffle = (((kPoints >> 1) - 1) * kCoeffShuffle);
    constexpr unsigned int kCoeffLoadShift = ((kStreamsPerCore - 1) + kCoeffLoadShuffle);

    constexpr unsigned int kBaseOffset = baseOffsetOfStream();
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift =
        ((kStreamsPerCore - 1) << TWO); //  loopidx+FOUR position for data buffer start when 'streams = TWO'

    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << TWO)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
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
    unsigned int xstart;
    unsigned int zstart;

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, V8SIZE>;
    using T_accum_hi = ::aie::accum<cacc48, V4SIZE>;
    using T_buff_GSig = TT_DATA_G;                           // W_REG_BITS buffer for reading G Data
    using T_buff_GSig_vect = ::aie::vector<TT_DATA_G, FOUR>; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;
    int rot = 1;

    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi acc4;
    T_buff_GSig_vect tempBuff;
    T_buff_GSig_vect* __restrict vInDataPtrG = (T_buff_GSig_vect*)inWindowG.data(); // Input pointer for Sig_G
    T_buff_zbuff zbuff[kMacsPerCore];
    T_buff_GSig_vect chess_storage(% chess_alignof(v32cint16)) g_rearrange_buff[TP_G_LEN];
    T_buff_GSig_vect* tempPtr = (T_buff_GSig_vect*)g_rearrange_buff;

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
    gdata_rearrange<TT_DATA_F, TT_DATA_G>(vInDataPtrG, tempPtr, kStreamsPerCore, TP_PHASES, TP_G_LEN, TP_FUNCT_TYPE);

    for (unsigned int i = 0; i < ((kLanes * kPoints) * (TP_CASC_LEN - TP_KERNEL_POSITION - 1) * kMacsPerCore) / FOUR;
         i++) {
        tempBuff = *tempPtr++;
        acc4 = ups(tempBuff, 0);
        writeincr(outcascade, acc4);
    }

    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / FOUR; j++) {
            tempBuff = *tempPtr++;
            zbuff[i].val = upd_v(zbuff[i].val, j, tempBuff);
        }
    }

#define MAC_LOOP_FIRST_KERNEL(os)                                                                            \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                        \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                     \
        if (k == 0) {                                                                                        \
            acc = mac4_rot(acc, rot, xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr); \
        } else {                                                                                             \
            acc4 = mac4(ext_hi(acc), xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr); \
            acc = upd_hi(acc, acc4);                                                                         \
        }                                                                                                    \
    }                                                                                                        \
    writeincr(outcascade, ext_hi(acc));

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(UNROLL_4) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += FOUR) {
                //  xbuff = upd_v(xbuff, (inLoopIndx>>TWO), getc_wss(0));
                readStream(xbuff, ((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO) {
                    readStream(xbuff, (((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll, 1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_FIRST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + TWO);
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
NOINLINE_DECL void conv_corr<TT_DATA_F,
                             TT_DATA_G,
                             TT_DATA_OUT,
                             TP_FUNCT_TYPE,
                             TP_COMPUTE_MODE_IS_VALID_MODE(),
                             TP_F_LEN,
                             TP_G_LEN,
                             TP_SHIFT,
                             TP_API_IS_ONE(),
                             TP_RND,
                             TP_SAT,
                             TP_NUM_FRAMES,
                             TP_CASC_LEN,
                             TP_PHASES,
                             TP_KERNEL_POSITION,
                             TP_PH_POSITION,
                             CASC_IN_TRUE,
                             CASC_OUT_TRUE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                           input_stream<TT_DATA_F>* __restrict instream2F,
                                                           input_cascade<cacc48>* __restrict incascade,
                                                           output_cascade<cacc48>* __restrict outcascade) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of kLanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                       // m_kPoints;   // Num of kPoints
    constexpr unsigned int kstreampercore_var = ((kLanes * kPoints * TP_PHASES) >> 1);
    constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kstreampercore_var) ? 1 : maxNumOfStreams();

    constexpr unsigned int kCoeffShuffle = (kPoints > TWO && kStreamsPerCore > 1) ? 1 : 0;
    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // TP_PHASES;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = kLanes * kPoints * kCores;
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kDataBuffLen =
        (kMacsPerCore * kPoints < FOUR) ? minDataBuffLen() : ((kMacsPerCore * kPoints) << TWO); // Data Buffer Length

    constexpr unsigned int kLoopCount = CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen; // Loopcount
    constexpr unsigned int kPhaseIncr = TP_PHASES >> (kStreamsPerCore - 1);
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kLanes * kPoints * kMacsPerCore) >> (kStreamsPerCore - 1);

    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> TWO) - 1);

    constexpr unsigned int kCoeffLoadShuffle = ((kPoints >> 1) - 1) * kCoeffShuffle;
    constexpr unsigned int kCoeffLoadShift = (kStreamsPerCore - 1) + kCoeffLoadShuffle;

    constexpr unsigned int kBaseOffset = baseOffsetOfStream();
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = (kStreamsPerCore - 1)
                                          << TWO; //  loopidx+FOUR position for data buffer start when 'streams = TWO'
    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << TWO)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
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

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, V8SIZE>;
    using T_accum_hi = ::aie::accum<cacc48, V4SIZE>;
    using T_buff_GSig = TT_DATA_G; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;

    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi cascVect, acc4;
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
    for (unsigned int i = 0; i < (kLanes * kPoints) * (TP_CASC_LEN - TP_KERNEL_POSITION - 1) * kMacsPerCore / FOUR;
         i++) {
        cascVect = readincr_v<V4SIZE>(incascade);
        writeincr(outcascade, cascVect);
    }

    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / FOUR; j++) {
            cascVect = readincr_v<V4SIZE>(incascade);
            zbuff[i].val = upd_v(zbuff[i].val, j, srs(cascVect, 0));
        }
    }

#define MAC_LOOP_INTERMEDIATE_KERNEL(os)                                                                               \
    cascVect = readincr_v<V4SIZE>(incascade);                                                                          \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                                  \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                               \
        if (k == 0) {                                                                                                  \
            acc = mac4_rot(acc, cascVect, rot, xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr); \
        } else {                                                                                                       \
            acc4 = mac4(ext_hi(acc), xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr);           \
            acc = upd_hi(acc, acc4);                                                                                   \
        }                                                                                                              \
    }                                                                                                                  \
    writeincr(outcascade, ext_hi(acc));

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(UNROLL_4) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += FOUR) {
                //  xbuff = upd_v(xbuff, (inLoopIndx>>TWO), getc_wss(0));
                readStream(xbuff, ((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);

                if (kStreamsPerCore == TWO) {
                    readStream(xbuff, (((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll, 1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + TWO);
                MAC_LOOP_INTERMEDIATE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + THREE);
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
NOINLINE_DECL void conv_corr<TT_DATA_F,
                             TT_DATA_G,
                             TT_DATA_OUT,
                             TP_FUNCT_TYPE,
                             TP_COMPUTE_MODE_IS_VALID_MODE(),
                             TP_F_LEN,
                             TP_G_LEN,
                             TP_SHIFT,
                             TP_API_IS_ONE(),
                             TP_RND,
                             TP_SAT,
                             TP_NUM_FRAMES,
                             TP_CASC_LEN,
                             TP_PHASES,
                             TP_KERNEL_POSITION,
                             TP_PH_POSITION,
                             CASC_IN_TRUE,
                             CASC_OUT_FALSE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                            input_stream<TT_DATA_F>* __restrict instream2F,
                                                            input_cascade<cacc48>* __restrict incascade,
                                                            output_stream<TT_DATA_OUT>* __restrict outstream) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of kLanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                       // m_kPoints;   // Num of kPoints
    constexpr unsigned int kstreampercore_var = ((kLanes * kPoints * TP_PHASES) >> 1);
    constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kstreampercore_var) ? 1 : maxNumOfStreams();
    constexpr unsigned int kCoeffShuffle = (kPoints > TWO && kStreamsPerCore > 1) ? 1 : 0;

    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // TP_PHASES;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = kLanes * kPoints * kCores;
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kDataBuffLen =
        (kMacsPerCore * kPoints < FOUR) ? 16 : ((kMacsPerCore * kPoints) << TWO);                  // Data Buffer Length
    constexpr unsigned int kLoopCount = CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen; // Loopcount
    constexpr unsigned int kPhaseIncr = TP_PHASES >> (kStreamsPerCore - 1);
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kLanes * kPoints * kMacsPerCore) >> (kStreamsPerCore - 1);

    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> TWO) - 1);
    constexpr unsigned int kCoeffLoadShuffle = ((kPoints >> 1) - 1) * kCoeffShuffle;
    constexpr unsigned int kCoeffLoadShift = (kStreamsPerCore - 1) + kCoeffLoadShuffle;

    constexpr unsigned int kBaseOffset = baseOffsetOfStream();
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = (kStreamsPerCore - 1)
                                          << TWO; //  loopidx+FOUR position for data buffer start when 'streams = TWO'
    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << TWO)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
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

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, V8SIZE>;
    using T_accum_hi = ::aie::accum<cacc48, V4SIZE>;
    using T_buff_GSig = TT_DATA_G; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;
    using dataoutVect_t = ::aie::vector<TT_DATA_OUT, V4SIZE>;
    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi cascVect, acc_lo, acc4;
    //    T_buff_GSig* __restrict inDataPtrG = (T_buff_GSig*)inWindowG.data();      // Input pointer for Sig_G
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
        for (unsigned int j = 0; j < (kLanes * kPoints) / FOUR; j++) {
            cascVect = readincr_v<V4SIZE>(incascade);
            // upd_W_buff(zbuff[i].val,j,(T_accum_hi *)cascVect);
            // zbuff[i].val.insert(j,cascVect);
            zbuff[i].val = upd_v(zbuff[i].val, j, srs(cascVect, 0));
        }
    }

#define MAC_LOOP_LAST_KERNEL(os)                                                                                       \
    cascVect = readincr_v<V4SIZE>(incascade);                                                                          \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                                  \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                               \
        if (k == 0) {                                                                                                  \
            acc = mac4_rot(acc, cascVect, rot, xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr); \
        } else {                                                                                                       \
            acc4 = mac4(ext_hi(acc), xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr);           \
            acc = upd_hi(acc, acc4);                                                                                   \
        }                                                                                                              \
    }

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(UNROLL_4) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += FOUR) {
                //   xbuff = upd_v(xbuff, (inLoopIndx>>TWO), getc_wss(0));
                readStream(xbuff, ((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO) {
                    readStream(xbuff, (((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll, 1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + TWO);
                MAC_LOOP_LAST_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + MAC4ROTDELAY);

                acc_lo = ext_lo(acc);
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
NOINLINE_DECL void conv_corr<TT_DATA_F,
                             TT_DATA_G,
                             TT_DATA_OUT,
                             TP_FUNCT_TYPE,
                             TP_COMPUTE_MODE_IS_VALID_MODE(),
                             TP_F_LEN,
                             TP_G_LEN,
                             TP_SHIFT,
                             TP_API_IS_ONE(),
                             TP_RND,
                             TP_SAT,
                             TP_NUM_FRAMES,
                             TP_CASC_LEN,
                             TP_PHASES,
                             TP_KERNEL_POSITION,
                             TP_PH_POSITION,
                             CASC_IN_FALSE,
                             CASC_OUT_FALSE>::conv_corrMain(input_stream<TT_DATA_F>* __restrict instream1F,
                                                            input_stream<TT_DATA_F>* __restrict instream2F,
                                                            input_buffer<TT_DATA_G>& __restrict inWindowG,
                                                            output_stream<TT_DATA_OUT>* __restrict outstream) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    constexpr unsigned int kLanes = getLanesOfMac4RotIntrinsic<TT_DATA_F>(); // Num of kLanes
    constexpr unsigned int kPoints = m_kMuls / kLanes;                       // m_kPoints;   // Num of kPoints
    constexpr unsigned int kstreampercore_var = ((kLanes * kPoints * TP_PHASES) >> 1);
    constexpr unsigned int kStreamsPerCore = (TP_G_LEN > kstreampercore_var) ? 1 : maxNumOfStreams();
    constexpr unsigned int kCoeffShuffle = (kPoints > TWO && kStreamsPerCore > 1) ? 1 : 0;

    constexpr unsigned int kMacOffsetIncr = (kCoeffShuffle == 1) ? kPoints : kStreamsPerCore; // TP_PHASES;
    constexpr unsigned int kMacStepIncr = (kStreamsPerCore == TWO) ? 1 : kLanes;
    constexpr unsigned int kCores = TP_CASC_LEN * TP_PHASES;

    constexpr unsigned int kMaxMuls = kLanes * kPoints * kCores;
    constexpr unsigned int kMacsPerCore = (CEIL(TP_G_LEN, kMaxMuls)) / kMaxMuls;
    constexpr unsigned int kDataBuffLen =
        (kMacsPerCore * kPoints < FOUR) ? 16 : ((kMacsPerCore * kPoints) << TWO);                  // Data Buffer Length
    constexpr unsigned int kLoopCount = CEIL((TP_F_LEN / TP_PHASES), kDataBuffLen) / kDataBuffLen; // Loopcount
    constexpr unsigned int kPhaseIncr = TP_PHASES >> (kStreamsPerCore - 1);
    constexpr unsigned int kCoeffPerCore = (TP_PHASES * kLanes * kPoints * kMacsPerCore) >> (kStreamsPerCore - 1);

    constexpr unsigned int kPtrStart =
        (TP_PHASES - ((1 + (TP_KERNEL_POSITION & (kPhaseIncr - 1))) << (kStreamsPerCore - 1)));
    constexpr unsigned int kPtrIncr =
        (TP_G_LEN - kCoeffPerCore * (CEIL((TP_KERNEL_POSITION + 1), kPhaseIncr) / kPhaseIncr) + kPtrStart);
    constexpr unsigned int kBuffIndxRoll = ((kDataBuffLen >> TWO) - 1);
    constexpr unsigned int kCoeffLoadShuffle = ((kPoints >> 1) - 1) * kCoeffShuffle;
    constexpr unsigned int kCoeffLoadShift = (kStreamsPerCore - 1) + kCoeffLoadShuffle;

    constexpr unsigned int kBaseOffset = baseOffsetOfStream();
    constexpr unsigned int kZoffset = kMacOffsetIncr * kBaseOffset; // Offset
    constexpr unsigned int kXstartShift = (kStreamsPerCore - 1)
                                          << TWO; //  loopidx+FOUR position for data buffer start when 'streams = TWO'
    constexpr unsigned int kCascDelayComponent =
        (MAC4ROTDELAY - ((kMacsPerCore * kPoints) << TWO)) * (TP_KERNEL_POSITION >> (kPhaseIncr - 1));
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

    using dataVectA_t = ::aie::vector<TT_DATA_F, kDataBuffLen>;
    using T_accum = ::aie::accum<cacc48, V8SIZE>;
    using T_accum_hi = ::aie::accum<cacc48, V4SIZE>;
    using T_buff_GSig = TT_DATA_G;                             // W_REG_BITS buffer for reading G Data
    using T_buff_GSig_vect = ::aie::vector<TT_DATA_G, V4SIZE>; // W_REG_BITS buffer for reading G Data
    using T_buff_zbuff = T_InOut_W_buff<TT_DATA_G>;
    using dataoutVect_t = ::aie::vector<TT_DATA_OUT, V4SIZE>;
    dataVectA_t* vPtrDelayLine = (dataVectA_t*)delayBuff;
    dataVectA_t xbuff = *vPtrDelayLine;
    T_accum* vPtrDelayLineAcc = (T_accum*)delayAcc;
    T_accum acc = *vPtrDelayLineAcc;
    T_accum_hi acc_lo, acc4;

    T_buff_GSig_vect tempBuff;
    T_buff_GSig_vect* __restrict vInDataPtrG = (T_buff_GSig_vect*)inWindowG.data(); // Input pointer for Sig_G
    T_buff_zbuff zbuff[kMacsPerCore];
    T_buff_GSig_vect chess_storage(% chess_alignof(v32cint16)) g_rearrange_buff[TP_G_LEN];
    T_buff_GSig_vect* tempPtr = (T_buff_GSig_vect*)g_rearrange_buff;

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
    gdata_rearrange<TT_DATA_F, TT_DATA_G>(vInDataPtrG, tempPtr, kStreamsPerCore, TP_PHASES, TP_G_LEN, TP_FUNCT_TYPE);

    for (unsigned int i = 0; i < kMacsPerCore; i++) {
        for (unsigned int j = 0; j < (kLanes * kPoints) / FOUR; j++) {
            tempBuff = *tempPtr++;
            zbuff[i].val = upd_v(zbuff[i].val, j, tempBuff);
        }
    }

#define MAC_LOOP_SINGLE_KERNEL(os)                                                                           \
    for (unsigned int k = 0; k < kMacsPerCore; k++) {                                                        \
        xstart = (os - k * kMacStepIncr * kPoints) & (kDataBuffLen - 1);                                     \
        if (k == 0) {                                                                                        \
            acc = mac4_rot(acc, rot, xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr); \
        } else {                                                                                             \
            acc4 = mac4(ext_hi(acc), xbuff, xstart, 0x0000, -FOUR, zbuff[k].val, 0, kZoffset, kMacStepIncr); \
            acc = upd_hi(acc, acc4);                                                                         \
        }                                                                                                    \
    }

    // Outer Loop to do Convolution/Correlation
    for (unsigned int outIndx = 0; outIndx < kLoopCount; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(UNROLL_4) {
#pragma unroll(kDataBuffLen)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kDataBuffLen; inLoopIndx += FOUR) {
                //   xbuff = upd_v(xbuff, (inLoopIndx>>TWO), getc_wss(0));
                readStream(xbuff, ((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) & kBuffIndxRoll, 0);
                if (kStreamsPerCore == TWO) {
                    readStream(xbuff, (((inLoopIndx >> TWO) << (kStreamsPerCore - 1)) + 1) & kBuffIndxRoll, 1);
                } else {
                    getc_wss(1);
                }
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 0);
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + 1);
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + TWO);
                MAC_LOOP_SINGLE_KERNEL((inLoopIndx << (kStreamsPerCore - 1)) + kXstartShift + MAC4ROTDELAY);

                acc_lo = ext_lo(acc);
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
