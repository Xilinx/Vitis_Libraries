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
EUCLIDEAN_DISTANCE kernal code.
This file captures the body of run-time code for the kernal class.
Coding conventions
TT_      template type suffix
TP_      template parameter suffix
*/

#pragma once
#include <adf.h>
#include <vector>
#include "euclidean_distance.hpp"
#include "euclidean_distance_utils.hpp"

using namespace adf;

namespace xf {
namespace dsp {
namespace aie {
namespace euclidean_distance {

//-----------------------------------------------------------------------------------------------------
// Euclidean Distance Kernel Class
template <typename TT_DATA,
          typename TT_DATA_OUT,
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT,
          unsigned int TP_IS_OUTPUT_SQUARED>
NOINLINE_DECL void
euclidean_distance_squared<TT_DATA, TT_DATA_OUT, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT, TP_IS_OUTPUT_SQUARED>::
    euclideanDistMain(input_buffer<TT_DATA>& __restrict inWindowP,
                      input_buffer<TT_DATA>& __restrict inWindowQ,
                      output_buffer<TT_DATA_OUT>& __restrict squaredOut) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    unsigned int kinnerLoopCnt = (m_kLanes / m_kVecLoad); // Innerloop count to iterate over input data
    unsigned int kloopcnt = (TP_LEN / m_kLanes);          // Outer loop count to compute ED over given length.
    constexpr unsigned int kExtractElem = m_kVecLoad; // Constant to extarct elements from vector to transpos ethe data.
    constexpr unsigned int kDim = kFixedDimOfED;      // Dimension of ED fixed as 4 to maintain pipeling.

    using T_buff_WReg = T_InOut_W_buff<TT_DATA>;               // W_REG_BITS (256b) buffer for reading P Data
    using T_buff_YReg = T_InOut_Y_buff<TT_DATA>;               // Y_REG_BITS (1024b) buffer for storing transposed data
    using outDistVec_t = ::aie::vector<TT_DATA_OUT, m_kLanes>; // Output vector based on output data type and lanes
    using T_accum = T_acc_ED<TT_DATA>;                         // Alias for Accumulator Reg based on given data types
    using T_accVecLoad = ::aie::accum<tEDAccType_t<TT_DATA>, m_kVecLoad>; // Alias for Accumulator based on AIE load

    T_buff_WReg pbuff; // Declaration of Pbuff using W(256b) Reg.
    T_buff_WReg qbuff; // Declaration of Qbuff using W(256b) Reg.
    T_buff_WReg rbuff; // Declaration of Result of sub. buff using W(256b) Reg.
    T_buff_YReg tbuff; // Declaration of Transpose buff using Y(1024b) Reg.
    T_accum acc;       // Declaration of Accumulator based on lanes of given input data type (float/bfloat16)
    T_accVecLoad
        accVecLoad; // Declaration of Accumulator based on AIE load ((256/samples(float) or (256/samples(bfloat16)))

    T_buff_WReg* __restrict inPtrP = (T_buff_WReg*)inWindowP.data();    // Input pointer for Point P
    T_buff_WReg* __restrict inPtrQ = (T_buff_WReg*)inWindowQ.data();    // Input pointer for Point Q
    outDistVec_t* __restrict outPtr = (outDistVec_t*)squaredOut.data(); // Output Pointer for ED result

    // Computation of Euclidean Distance.
    for (unsigned int outIndx = 0; outIndx < kloopcnt; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kUnrollFactor) {
            for (unsigned int inIndx = 0; inIndx < kinnerLoopCnt; inIndx++) {
                // Fetching the data from P and Q points to maximum
                for (unsigned int dataIndx = 0; dataIndx < kDim; dataIndx++) {
                    // Load Data of Point P and Point Q
                    upd_W_buff(pbuff.val, 0, inPtrP++); // Fetch the P data to Registrer pbuff.
                    upd_W_buff(qbuff.val, 0, inPtrQ++); // Fetch the Q data to Registrer qbuff.

                    // Subtraction of P and Q data.
                    rbuff.val = ::aie::sub(qbuff.val, pbuff.val);

                    // store the res. of subtraction into vector with size of 32.
                    tbuff.val.insert(dataIndx, rbuff.val);
                }

                // Re-arrange squared results as per given dimension like
                tbuff.val = ::aie::transpose(tbuff.val, kExtractElem, kDim); // Transpose the data as 8 x 4 matrix.

                // squared results as a vector via accumulator
                accVecLoad = ::aie::mul_square(tbuff.val.template extract<kExtractElem>(0));
                for (unsigned int sampIndx = 1; sampIndx < kDim; sampIndx++) {
                    accVecLoad = ::aie::mac_square(accVecLoad, tbuff.val.template extract<kExtractElem>(sampIndx));
                }
                acc.val.insert(inIndx, accVecLoad); // store the result of accumulator into lane based accumulator.
            }                                       // End of InnerLoop

#if (__HAS_ACCUM_PERMUTES__ == 1)
            *outPtr++ = (TP_IS_OUTPUT_SQUARED == 0)
                            ? ::aie::sqrt(acc.val.template to_vector<TT_DATA>(0))
                            : acc.val.template to_vector<TT_DATA>(
                                  0); // writing output of squared or sqrt one to the out buff for AIE-1
#else
            // AIE-ML
            if
                constexpr((std::is_same<TT_DATA_OUT, float>::value) && (TP_IS_OUTPUT_SQUARED == 0)) {
                    *outPtr++ = to_v16bfloat16(
                        acc.val); // convert sqaured res. of FLOAT to BFLOAT16 to the out buffer when arc is AIE-ML
                }
            else {
                *outPtr++ =
                    acc.val.template to_vector<TT_DATA_OUT>(0); // Writing only squared results when ARC. is AIE-ML
            }

#endif
        } // End of outer loop
};        // End of euclideanDistMain()

template <typename TT_DATA, typename TT_DATA_OUT, unsigned int TP_LEN, unsigned int TP_IS_OUTPUT_SQUARED>
NOINLINE_DECL void euclidean_distance<TT_DATA, TT_DATA_OUT, TP_LEN, TP_IS_OUTPUT_SQUARED>::euclideanDistMain(
    input_buffer<TT_DATA>& __restrict squaredIn, output_buffer<TT_DATA_OUT>& __restrict sqrtOut) {
// This kernel only works on AIE-ML when user asked SQRT res. of ED
#if (__HAS_ACCUM_PERMUTES__ == 0)
    if
        constexpr(TP_IS_OUTPUT_SQUARED == 0) {
            static constexpr unsigned int vecSize = m_ksampleSize;
            ::aie::vector<bfloat16, kVecSize16OfBf16>* pin =
                (::aie::vector<bfloat16, kVecSize16OfBf16>*)squaredIn.data();
            ::aie::vector<TT_DATA_OUT, vecSize>* pout = (::aie::vector<TT_DATA_OUT, vecSize>*)sqrtOut.data();

            ::aie::vector<bfloat16, kVecSize32OfBf16> x =
                ::aie::zeros<bfloat16, kVecSize32OfBf16>(); // Initialize bfloat16 vector with Zeros
            ::aie::vector<bfloat16, kVecSize32OfBf16> ylo =
                ::aie::zeros<bfloat16, kVecSize32OfBf16>(); // Initialize bfloat16 vector with Zeros
            ::aie::vector<bfloat16, kVecSize32OfBf16> yhi =
                ::aie::zeros<bfloat16, kVecSize32OfBf16>(); // Initialize bfloat16 vector with Zeros
            ::aie::vector<bfloat16, kVecSize32OfBf16> z =
                ::aie::zeros<bfloat16, kVecSize32OfBf16>(); // Initialize bfloat16 vector with Zeros
            ::aie::vector<uint16, kVecSize32OfBf16> a, c1, c2, c3,
                c4;                                             // Declaration of unsigned short vectors with size 32
            v16int32 k0, k1;                                    // Declaration of integer vectors with size 32
            v64int8 y1, y2;                                     // Declaration of int8 vectors with size 64
            v16bfloat16 vecBf16One, vecBf16Two;                 // Declaration of bfloat16 vectors with size 16
            ::aie::accum<accfloat, kVecSizeOfAccum16> acc16;    // Declaration of Float Accumulator with size 16
            ::aie::accum<accfloat, kVecSizeOfAccum32> acc32Fp;  // Declaration of Float Accumulator with size 16
            ::aie::accum<acc32, kVecSizeOfAccum32> acc32;       // Declaration of Integer Accumulator with size 32
            unsigned int loopcnt = (TP_LEN / kVecSize32OfBf16); // Loopcount to compute sqrt with LUTs

            // pointers to the LUT of SQRT
            int8* __restrict lut0 = (int8*)sqrtLUT0;
            int8* __restrict lut1 = (int8*)sqrtLUT1;

            // vectorized masks git
            c1 = broadcast_to_v32uint16(0x00ff);
            c2 = broadcast_to_v32uint16(0x7f00);
            c3 = broadcast_to_v32uint16(16384);

            // Loop to compute SquareRoot.
            for (int indx = 0; indx < loopcnt; indx++)
                chess_prepare_for_pipelining chess_loop_range(kVecSize32OfBf16, ) {
                    x = insert(x, 0, *pin++);
                    x = insert(x, 1, *pin++);

                    // Extract LSB 8-bits
                    a = band(v32uint16(x), c1);

                    // Indx * 4
                    acc32 = sups(a, kUpshiftFactor2);

                    // Extract MSB 7-bits
                    a = band(v32uint16(x), c2);
                    k0 = v16int32(extract_v16acc32(acc32, 0));
                    k1 = v16int32(extract_v16acc32(acc32, 1));
                    load_lut_2x_int8(lut0, lut1, k0, y1, y2);
                    ylo = v32bfloat16(shuffle(v32int16(y1), v32int16(y2), shuffle_T16_16x4_lo));
                    load_lut_2x_int8(lut0, lut1, k1, y1, y2);
                    yhi = v32bfloat16(shuffle(v32int16(y1), v32int16(y2), shuffle_T16_16x4_lo));

                    // MSB + 16384
                    a = add(a, c3);
                    acc32 = sups(a, 0);

                    // exp/2
                    a = v32uint16(lsrs(acc32, 1));
                    z = insert(z, 0, v16bfloat16(extract_v16uint16(a, 0)));

                    // 2^(exp/2) * sqrt()
                    acc16 = mul_elem_16_2(ylo, z);
                    vecBf16One = to_v16bfloat16(acc16);
                    z = insert(z, 0, v16bfloat16(extract_v16uint16(a, 1)));
                    acc16 = mul_elem_16_2(yhi, z);
                    vecBf16Two = to_v16bfloat16(acc16);

                    // compute sqrt and write the results into io buffer out
                    if
                        constexpr(std::is_same<TT_DATA_OUT, float>::value) {
                            // Float
                            x = concat(vecBf16One, vecBf16Two);
                            acc32Fp.template from_vector(x);
                            *pout++ = v32float(acc32Fp);
                        }
                    else {
                        // Bfloat16
                        if
                            constexpr(std::is_same<TT_DATA_OUT, bfloat16>::value) {
                                *pout++ = vecBf16One;
                                *pout++ = vecBf16Two;
                            }
                    } // End of If Condition.
                }     // End of LoopCount.
        }             // End of TP_IS_OUTPUT_SQUARED
#endif
}; // End of euclideanDistMain()
} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {
