/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
EUCLIDEAN_DISTANCE kernel code.
This file captures the body of run-time code for the kernel class.
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
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void euclidean_distance_squared<TT_DATA, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT>::euclideanDistMain(
    input_buffer<TT_DATA>& __restrict inWindowP,
    input_buffer<TT_DATA>& __restrict inWindowQ,
    output_buffer<TT_DATA>& __restrict squaredOut) {
    set_rnd_mode<TP_RND>(); //  Rounding mode to control how results are rounded.
    set_sat_mode<TP_SAT>(); //  Saturation mode to prevent overflow on accumulator output.

    constexpr unsigned int kDim = kFixedDimOfED;                // Dimension of ED fixed as 4 to maintain pipelining.
    constexpr int kWregsize = kWRegSizeBytes / sizeof(TT_DATA); // Number of TT_DATA elements in a 256-bit W-register.
    constexpr int kYregsize = kYRegSizeBytes / sizeof(TT_DATA); // Number of TT_DATA elements in a 1024-bit Y-register.
    constexpr unsigned int kPointsPerIter =
        kYregsize / kDim; // Output points per outer iteration: Y-register capacity divided by spatial dimension.
    constexpr unsigned int kloopcnt =
        (TP_LEN / kPointsPerIter); // Outer loop count: one iteration produces kPointsPerIter output points.

    using T_buff_WReg = ::aie::vector<TT_DATA, kWregsize>; // W_REG_BITS (256b) buffer for reading P/Q Data
    using T_buff_YReg = ::aie::vector<TT_DATA, kYregsize>; // Y_REG_BITS (1024b) buffer for storing transposed data
    using outDistVec_t = ::aie::vector<TT_DATA, kPointsPerIter>; // Output vector: one native vector of results
    using T_accSqDist = ::aie::accum<tEDAccType_t<TT_DATA>, kPointsPerIter>; // Accumulator for one vector of results

    T_buff_WReg pbuff;     // Declaration of Pbuff using W(256b) Reg.
    T_buff_WReg qbuff;     // Declaration of Qbuff using W(256b) Reg.
    T_buff_WReg diffBuff;  // Declaration of Result of sub. buff using W(256b) Reg.
    T_buff_YReg tbuff;     // Declaration of Transpose buff using Y(1024b) Reg.
    T_accSqDist sqDistAcc; // Declaration of Accumulator for one vector of results.

    T_buff_WReg* __restrict inPtrP = (T_buff_WReg*)inWindowP.data();    // Input pointer for Point P
    T_buff_WReg* __restrict inPtrQ = (T_buff_WReg*)inWindowQ.data();    // Input pointer for Point Q
    outDistVec_t* __restrict outPtr = (outDistVec_t*)squaredOut.data(); // Output Pointer for ED result

    // Computation of Euclidean Distance.
    for (unsigned int outIndx = 0; outIndx < kloopcnt; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kUnrollFactor) {
            // Load kDim W-register chunks for kPointsPerIter points
            for (unsigned int dimIndx = 0; dimIndx < kDim; dimIndx++) {
                pbuff.insert(0, *inPtrP++);          // Fetch the P data to Register pbuff.
                qbuff.insert(0, *inPtrQ++);          // Fetch the Q data to Register qbuff.
                diffBuff = ::aie::sub(qbuff, pbuff); // Subtraction of P and Q data.
                tbuff.insert(dimIndx, diffBuff);     // Store result into Y-reg.
            }

            // Transpose the data as kPointsPerIter x kDim matrix.
            tbuff = ::aie::transpose(tbuff, kPointsPerIter, kDim);

            // Compute squared distances over TP_DIM dimensions
            sqDistAcc = ::aie::mul_square(tbuff.template extract<kPointsPerIter>(0));
            for (unsigned int sqDimIndx = 1; sqDimIndx < TP_DIM; sqDimIndx++) {
                sqDistAcc = ::aie::mac_square(sqDistAcc, tbuff.template extract<kPointsPerIter>(sqDimIndx));
            }

            // Write kPointsPerIter results to output
            *outPtr++ = sqDistAcc.template to_vector<TT_DATA>(0);

        } // End of outer loop
};        // End of euclideanDistMain()

template <typename TT_DATA, unsigned int TP_LEN>
NOINLINE_DECL void euclidean_distance<TT_DATA, TP_LEN>::euclideanDistMain(input_buffer<TT_DATA>& __restrict squaredIn,
                                                                          output_buffer<TT_DATA>& __restrict sqrtOut) {
    // Sqrt kernel: uses ::aie::sqrt on AIE-1, LUT-based method on AIE-ML/AIE-MLv2.
    static constexpr unsigned int vecSize = m_ksampleSize;
    static constexpr unsigned int kloopcnt =
        (TP_LEN / m_ksampleSize); // Outer loop count to compute ED over given length.
    static constexpr unsigned int numOfParallelAccess =
        NUM_OF_PARALLEL_ACCESS_LUT; // number of parallel accesses from LUT is 4 (parallel loads)
    static constexpr unsigned int kFloatPerXReg = T_buff_512b<float>::getLanes();
    static constexpr unsigned int kFloatPerYReg = T_buff_1024b<float>::getLanes();

    using inVec_t = ::aie::vector<TT_DATA, vecSize>;

    inVec_t* __restrict pIn = (inVec_t*)squaredIn.data(); // Input pointer for squared distance data

#if (__HAS_ACCUM_PERMUTES__ == 0)

    using T_xRegBf16 = typename T_buff_512b<bfloat16>::v_type; // X-register (512b) bfloat16 vector
    using T_wRegBf16 = typename T_buff_256b<bfloat16>::v_type; // W-register (256b) bfloat16 vector
    using T_xRegU16 = typename T_buff_512b<uint16>::v_type;    // X-register (512b) uint16 vector
    using T_xRegInt = typename T_buff_512b<int32>::v_type;     // X-register (512b) int32 vector
    using lutDataType = int16; // lut read data type is int16 to fetch two int8 values from LUT

    // Alias for Accumulator Reg based on given data types
    using T_accFloatXReg_t = ::aie::accum<accfloat, kFloatPerXReg>;
    using T_accFloatYReg_t = ::aie::accum<accfloat, kFloatPerYReg>;
    using T_accIntYReg_t = ::aie::accum<acc32, T_buff_1024b<int32>::getLanes()>;

    // Output pointer: float writes kFloatPerYReg (32) floats per LUT iter;
    //                 bfloat16 writes two kBf16PerWReg (16) chunks per LUT iter.
    using outFloatVec_t = ::aie::vector<float, kFloatPerYReg>;
    outFloatVec_t* pOutFloat = nullptr;
    T_wRegBf16* pOutBf16 = nullptr;
    if
        constexpr(std::is_same<TT_DATA, float>::value) { pOutFloat = (outFloatVec_t*)sqrtOut.data(); }
    else {
        pOutBf16 = (T_wRegBf16*)sqrtOut.data();
    }

    T_wRegBf16* pInBf16; // intermediate pointer for input data with data type of bfloat16.

    // Accumulator
    T_accFloatXReg_t acc16Fp; // Declaration of Float Accumulator with size 16
    T_accFloatYReg_t acc32Fp; // Declaration of Float Accumulator with size 32
    T_accIntYReg_t acc32;     // Declaration of Integer Accumulator with size 32

    static constexpr unsigned int kBf16PerXReg = T_buff_512b<bfloat16>::getLanes();
    static constexpr unsigned int kBf16PerWReg = T_buff_256b<bfloat16>::getLanes();
    static constexpr unsigned int kU16PerXReg = T_buff_512b<uint16>::getLanes();

    if
        constexpr(std::is_same<TT_DATA, float>::value) {
            // Vectorized zero-initialization of float-to-bfloat16 conversion buffer.
            using convBf16XVec_t = ::aie::vector<bfloat16, kBf16PerXReg>;
            convBf16XVec_t* __restrict pZeroInit = (convBf16XVec_t*)convertFloatToBfloat16Buff;
            for (unsigned int i = 0; i < (TP_LEN / kBf16PerXReg); i++)
                chess_prepare_for_pipelining chess_loop_count(TP_LEN / kBf16PerXReg) {
                    *pZeroInit++ = ::aie::zeros<bfloat16, kBf16PerXReg>();
                }
            // Convert vecSize squared float values per step to bfloat16 for LUT-based sqrt.
            using f32ToBf16AccVec_t = ::aie::accum<accfloat, vecSize>;
            using bf16CastVec_t = ::aie::vector<bfloat16, vecSize>;
            bf16CastVec_t* pInBf16CastBuf = (bf16CastVec_t*)convertFloatToBfloat16Buff;
            for (unsigned int i = 0; i < kloopcnt; i++) chess_prepare_for_pipelining chess_loop_count(kloopcnt) {
                    f32ToBf16AccVec_t f32ToBf16Acc;
                    f32ToBf16Acc.from_vector(*pIn++, 0);
                    *pInBf16CastBuf++ = f32ToBf16Acc.template to_vector<bfloat16>();
                }
            pInBf16 = (T_wRegBf16*)convertFloatToBfloat16Buff;
        }
    else {
        // No conversion needed for bfloat16 input.
        pInBf16 = (T_wRegBf16*)squaredIn.data();
    } // End of If condition to check input data type.

    // Vectors
    T_xRegBf16 inVecBf16 = ::aie::zeros<bfloat16, kBf16PerXReg>(); // Initialize input vector of data tye bfloat16
    T_wRegBf16 vecSqrtOfMantissaLow =
        ::aie::zeros<bfloat16, kBf16PerWReg>(); // Initialize vector which stores LSB of Mantissa
    T_wRegBf16 vecSqrtOfMantissaHigh =
        ::aie::zeros<bfloat16, kBf16PerWReg>(); // Initialize vector which stores MSB of Mantissa
    T_wRegBf16 vecExponentData =
        ::aie::zeros<bfloat16, kBf16PerWReg>(); // Initialize a vector which stores Exponent Data
    T_xRegU16 bitExtractVec, lowerBitMask, upperBitMask,
        exponentAdjustVec;                // Declaration of unsigned short vectors with size 32
    T_xRegInt indexOffset0, indexOffset1; // Declaration of integer vectors with size 16
    T_wRegBf16 sqrtResLow, sqrtResHigh;   // Declaration of bfloat16 vectors with size 16

    constexpr unsigned int kLutLoopCnt =
        (TP_LEN / T_buff_512b<lutDataType>::getLanes()); // Loop count for LUT-based sqrt: one iteration processes one
                                                         // X-register of bfloat16 values
    unsigned int sizeOfLut =
        (LUT_SIZE /
         sizeof(lutDataType)); // LUT_SIZE is the Num of Elements in each lut i.e., 1024 with int8. Here 512 with int16.
    unsigned int stepBits =
        sizeof(lutDataType); // StepBits are 2 to fetch LUT values. int16 is the data type of each lut.

    // pointers to the LUT of SQRT
    lutDataType* __restrict lut0 = (lutDataType*)sqrtLUT0;
    lutDataType* __restrict lut1 = (lutDataType*)sqrtLUT1;

    // LUT initialization
    using lut_type = ::aie::lut<numOfParallelAccess, lutDataType, lutDataType>; // lookup table type

    // Declaring LUT with the LUT array size and pointers to the LUT array
    // Note size should be the LUT size which for each LUT is half the actual array size since we mirror data inside the
    // array
    lut_type ed_lut(sizeOfLut, lut0, lut1);

    // Parallel lookup declaration
    // First argument is the index data type. Under the hood, that should be int32
    ::aie::parallel_lookup<int16, lut_type, ::aie::lut_oor_policy::truncate> ed_lookup(
        ed_lut, stepBits); // declare lookup method

    // vectorized masks
    lowerBitMask = ::aie::broadcast<uint16, kU16PerXReg>(MASK_LOWEST_BITS);
    upperBitMask = ::aie::broadcast<uint16, kU16PerXReg>(MASK_UPPER_BITS);
    exponentAdjustVec = ::aie::broadcast<uint16, kU16PerXReg>(EXPONENT_ADJUSTMENT_CONSTANT);

    // Loop to compute SquareRoot using (2^((x-127)/2) * sqrtofMantissa_from_lut)
    for (int indx = 0; indx < kLutLoopCnt; indx++) chess_prepare_for_pipelining chess_loop_range(kBf16PerXReg, ) {
            inVecBf16.insert(0, *pInBf16++);
            inVecBf16.insert(1, *pInBf16++);

            // Extract LSB 8-bits
            bitExtractVec = ::aie::bit_and(inVecBf16.template cast_to<uint16>(), lowerBitMask);

            // Indx * 4
            acc32.from_vector(bitExtractVec, kLutIndexUpshift);

            // Extract MSB 7-bits
            bitExtractVec = ::aie::bit_and(inVecBf16.template cast_to<uint16>(), upperBitMask);

            // index computation to fetch lut values
            indexOffset0 = (acc32.template extract<16>(0)).template to_vector<int32>();
            indexOffset1 = (acc32.template extract<16>(1)).template to_vector<int32>();

            // fetch lut values based on index and convert that data to bfloat16
            vecSqrtOfMantissaLow = ed_lookup.fetch(indexOffset0).cast_to<bfloat16>();
            vecSqrtOfMantissaHigh = ed_lookup.fetch(indexOffset1).cast_to<bfloat16>();

            // MSB + 16384 (2^14 -- 2^(15-1))
            bitExtractVec = ::aie::add(bitExtractVec, exponentAdjustVec);
            acc32.from_vector(bitExtractVec, 0);

            // exp/2
            bitExtractVec = acc32.template to_vector<uint16>(1);
            vecExponentData.insert(0, bitExtractVec.template extract<16>(0).template cast_to<bfloat16>());

            // 2^(exp/2) * sqrt()
            acc16Fp = ::aie::mul(vecSqrtOfMantissaLow, vecExponentData);
            sqrtResLow = acc16Fp.template to_vector<bfloat16>(0);
            vecExponentData.insert(0, bitExtractVec.template extract<16>(1).template cast_to<bfloat16>());
            acc16Fp = ::aie::mul(vecSqrtOfMantissaHigh, vecExponentData);
            sqrtResHigh = acc16Fp.template to_vector<bfloat16>(0);

            // compute sqrt and write the results into io buffer out
            if
                constexpr(std::is_same<TT_DATA, float>::value) {
                    // Float: concat 32 bfloat16, convert to 32 float, write as one chunk.
                    inVecBf16 = concat(sqrtResLow, sqrtResHigh);
                    acc32Fp.template from_vector(inVecBf16);
                    *pOutFloat++ = acc32Fp.template to_vector<float>(0);
                }
            else {
                // Bfloat16: write two 16-element bfloat16 chunks per LUT iteration.
                *pOutBf16++ = sqrtResLow;
                *pOutBf16++ = sqrtResHigh;
            } // End of If Condition to check output data type.
        }     // End of LoopCount.
#else
    // Sqrt computation on AIE-1 Architecture
    using outVec_t = ::aie::vector<TT_DATA, vecSize>; // Output vector for sqrt results (float, vecSize elements)
    outVec_t* __restrict pOut = (outVec_t*)sqrtOut.data();
    inVec_t inVecSquared = ::aie::zeros<TT_DATA, vecSize>(); // Staging vector for one input chunk before sqrt
    for (unsigned int outIndx = 0; outIndx < kloopcnt; outIndx++)
        chess_prepare_for_pipelining chess_loop_count(kUnrollFactor) {
            *pOut++ = ::aie::sqrt(inVecSquared.insert(0, *pIn++));
        }
#endif

}; // End of euclideanDistMain()
} //  End of namespace euclidean_distance {
} //  End of namespace aie {
} //  End of namespace dsp {
} // End of  namespace xf {
