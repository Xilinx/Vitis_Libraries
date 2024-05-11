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
          unsigned int TP_SAT>
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
                             TP_SAT>::conv_corrMain(input_buffer<TT_DATA_F>& __restrict inWindowF,
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

    constexpr unsigned int DataStepX = ONE;                     // Initilization of slidingmul parameter i.e. DataStepX
    constexpr unsigned int DataStepZ = ONE;                     // Initilization of slidingmul parameter i.e. DataStepZ
    constexpr unsigned int DataStepY = ONE;                     // Initilization of slidingmul parameter i.e. DataStepY
    constexpr unsigned int Lanes = m_kLanes;                    // Num of Lanes
    constexpr unsigned int Points = m_kPoints;                  // Num of Points
    constexpr unsigned int kFLoadbits = getLog2<m_kVecLoadF>(); // No of bits for FLoad
    constexpr unsigned int kAccumLen = ROUND(TP_G_LEN, m_kVecLoadG);               // Len. of Accumulator
    constexpr unsigned int kInLoopLen = ROUND(m_kVecLoadF, Points);                // In loop Length
    constexpr unsigned int kGandFLoadRatio = FLOOR(m_kVecLoadG, m_kVecLoadF);      // Ratio of G load to F Load
    constexpr unsigned int kLoopCount = (CEIL(m_kLoopCount, m_kLanes) / m_kLanes); // Loopcount
    constexpr unsigned int inDataPtrOffset =
        ((m_kVecLoadF & (m_kVecLoadG - 1)) + getLog2<TP_G_LEN>());                 // Offset of G ptr
    constexpr unsigned int kIncrOffsetGSig = getOffsetGsig<TT_DATA_G, TP_G_LEN>(); // Offset to increment G Sig
    unsigned int SwapCount = (ROUND(TP_G_LEN, m_kVecLoadG) >> 1); // Count of elements in G-signal to reverse them

    T_accum acc;             // Declaration of Accumulator
    T_buff_Xbuff xbuff;      // Declaration of xbuff using X(1024b) Reg.
    T_buff_Zbuff zbuff;      // Declaration of zbuff using W(256b) Reg.
    dataVecOut_t dataVecOut; // Declaration of Output Vector to store results into Out buffer.

    T_buff_FSig* __restrict inDataPtrF = (T_buff_FSig*)inWindowF.data(); // Input pointer for Sig_F
    T_buff_GSig* __restrict inDataPtrG = (T_buff_GSig*)inWindowG.data(); // Input pointer for Sig_G
    dataVecOut_t* __restrict outPtr = (dataVecOut_t*)outWindow.data();   // Output Pointer for conv/corr result

    T_buff_FSig* __restrict rdInDataFPtr = (T_buff_FSig*)inWindowF.data(); // Alias Input pointer for Sig_F
    T_buff_GSig* __restrict rdInDataGPtr = (T_buff_GSig*)inWindowG.data(); // Alias Input pointer for Sig_G

    // Alias for sliding_mul API
    using mul_ops = ::aie::sliding_mul_ops<Lanes, Points, DataStepX, DataStepZ, DataStepY, TT_DATA_G, TT_DATA_F,
                                           tConvCorrAccType_t<TT_DATA_F, TT_DATA_G> >;

    if (TP_FUNCT_TYPE == 1) // function type is "CONV"
    {
        T_buff_GSig* __restrict inDataEndPtrG;              // Local Pointer to fetch end address of G data
        T_buff_Zbuff zbuffs, zbuffe;                        // Registers to hold Start and End data samples of G sig.
        constexpr int kVsize = 256 / 8 / sizeof(TT_DATA_G); // Size of vector as per given data type.
        using t_vect = ::aie::vector<TT_DATA_G, kVsize>;    // definition using the vec size.

        inDataEndPtrG = (inDataPtrG + kIncrOffsetGSig); // Move the pointer to End address of G sig.

        t_vect* outStartPtrG = (t_vect*)
            inDataPtrG; // Output pointer pointing to Start of G buffer, used to write the swapped data of G sig
        t_vect* outEndPtrG = (t_vect*)
            inDataEndPtrG; // Output pointer pointing to End of G buffer, used to write the swapped data of G sig

        // reverse the G elements using inplace memory for Conv.
        for (unsigned int i = 0; i < SwapCount; i++) chess_prepare_for_pipelining chess_loop_count(8) {
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
            inDataPtrF = (rdInDataFPtr + (outIndx * (Lanes >> kFLoadbits))); // Reset F Sig Pointer.
            inDataPtrG = rdInDataGPtr;                                       // Reset G Sig Pointer.

            // Initialization of Accumulator with Zeros to flush the previous data.
            acc.val = ::aie::zeros<tConvCorrAccType_t<TT_DATA_F, TT_DATA_G>,
                                   acc.val.size()>(); // null all the elements of accumulator

#pragma unroll(UNROLL_16)
            for (unsigned int inLoopIndx = 0; inLoopIndx < kAccumLen;
                 inLoopIndx++) // Inner Loop to do conv/corr using F and G Data.
            {
                // Load Data of G Signal
                upd_W_buff(zbuff.val, ZERO, inDataPtrG++); // Fetch the G data to Registrer zbuff.

// Load Data of F Signal
#pragma unroll(kGandFLoadRatio)
                for (unsigned int k = 0; k < kGandFLoadRatio; k++) {
                    // Pointer Manipulation to read No of "FLoad" elements into Y_REG_BITS buffer outIndx.e. xbuff
                    upd_W_buff(xbuff.val, ZERO, inDataPtrF++); // update xbuff with F sig data based on vector load.
                    upd_W_buff(xbuff.val, ONE, inDataPtrF);    // update xbuff with F sig data based on vector load.
                    if (Lanes > m_kVecLoadF)                   // m_kVecLoad_Len_F is nothing but FLoad
                    {
                        inDataPtrF++;
                        upd_W_buff(xbuff.val, TWO,
                                   inDataPtrF--); // Fetch the data when Lanes greater than F Vector load
                    }
#pragma unroll(kInLoopLen)
                    for (unsigned int l = 0; l < kInLoopLen; l++) {
                        // Sliding Multiplication of given signals
                        acc.val = mul_ops::mac(acc.val, zbuff.val, (k * kInLoopLen + l) * Points, xbuff.val,
                                               l * Points); // Sliding MAC function Call

                    } // End of kInLoopLen
                }     // End of kGandFLoadRatio
            }         // End of InnerLoop

            dataVecOut =
                acc.val.template to_vector<TT_DATA_OUT>(TP_SHIFT); // Storing accumulator results into out data vector
            *outPtr++ = dataVecOut;                                // write the output results to the iobuffer

        } // End of OuterLoop
};        // End of conv_corrMain()

} //  End of namespace conv_corr {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {
