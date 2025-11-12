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
          unsigned int TP_LEN,
          unsigned int TP_DIM,
          unsigned int TP_API,
          unsigned int TP_RND,
          unsigned int TP_SAT>
NOINLINE_DECL void euclidean_distance_squared<TT_DATA, TP_LEN, TP_DIM, TP_API, TP_RND, TP_SAT>::euclideanDistMain(
    input_buffer<TT_DATA>& __restrict inWindowP,
    input_buffer<TT_DATA>& __restrict inWindowQ,
    output_buffer<TT_DATA>& __restrict squaredOut) {
    set_rnd_mode<TP_RND>(); //  Rounding and Saturation to avoid bit errors.
    set_sat_mode<TP_SAT>(); //  Saturation to avoid the overflow issue.

    unsigned int kinnerLoopCnt = (m_kLanes / m_kVecLoad); // Innerloop count to iterate over input data
    unsigned int kloopcnt = (TP_LEN / m_kLanes);          // Outer loop count to compute ED over given length.
    constexpr unsigned int kExtractElem = m_kVecLoad; // Constant to extarct elements from vector to transpos ethe data.
    constexpr unsigned int kDim = kFixedDimOfED;      // Dimension of ED fixed as 4 to maintain pipeling.
    constexpr int kWregsize = kBuffSize32Byte / sizeof(TT_DATA);  // Vector size of 256 bits for given data type.
    constexpr int kYregsize = kBuffSize128Byte / sizeof(TT_DATA); // Vector size of 256 bits for given data type.

    using T_buff_WReg = ::aie::vector<TT_DATA, kWregsize>; // W_REG_BITS (256b) buffer for reading P Data
    using T_buff_YReg = ::aie::vector<TT_DATA, kYregsize>; // Y_REG_BITS (1024b) buffer for storing transposed data
    using outDistVec_t = ::aie::vector<TT_DATA, m_kLanes>; // Output vector based on output data type and lanes
    using T_accum = T_acc_ED<TT_DATA>;                     // Alias for Accumulator Reg based on given data types
    using T_accVecLoad = ::aie::accum<tEDAccType_t<TT_DATA>, m_kVecLoad>; // Alias for Accumulator based on AIE load

    T_buff_WReg pbuff;       // Declaration of Pbuff using W(256b) Reg.
    T_buff_WReg qbuff;       // Declaration of Qbuff using W(256b) Reg.
    T_buff_WReg rbuff;       // Declaration of Result of sub. buff using W(256b) Reg.
    T_buff_YReg tbuff;       // Declaration of Transpose buff using Y(1024b) Reg.
    T_accum acc;             // Declaration of Accumulator based on lanes of given input data type (float/bfloat16)
    T_accVecLoad accVecLoad; // Declaration of Accumulator based on AIE load.

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
                    pbuff.insert(0, *inPtrP++); // Fetch the P data to Registrer pbuff.
                    qbuff.insert(0, *inPtrQ++); // Fetch the Q data to Registrer qbuff.
                    // Subtraction of P and Q data.
                    rbuff = ::aie::sub(qbuff, pbuff);
                    // store the res. of subtraction into vector with size of 32.
                    tbuff.insert(dataIndx, rbuff);
                }

                // Re-arrange squared results as per given dimension like
                tbuff = ::aie::transpose(tbuff, kExtractElem, kDim); // Transpose the data as 8 x 4 matrix.

                // squared results as a vector via accumulator
                accVecLoad = ::aie::mul_square(tbuff.template extract<kExtractElem>(0));
                for (unsigned int sampIndx = 1; sampIndx < TP_DIM; sampIndx++) {
                    accVecLoad = ::aie::mac_square(accVecLoad, tbuff.template extract<kExtractElem>(sampIndx));
                }

                acc.val.insert(inIndx, accVecLoad); // store the result of accumulator into lane based accumulator.
            }                                       // End of InnerLoop

            // Writing only squared results as output
            *outPtr++ = acc.val.template to_vector<TT_DATA>(0);

        } // End of outer loop
};        // End of euclideanDistMain()

template <typename TT_DATA, unsigned int TP_LEN>
NOINLINE_DECL void euclidean_distance<TT_DATA, TP_LEN>::euclideanDistMain(input_buffer<TT_DATA>& __restrict squaredIn,
                                                                          output_buffer<TT_DATA>& __restrict sqrtOut) {
    // This kernel only works on AIE-ML when user asked SQRT res. of ED
    static constexpr unsigned int vecSize = m_ksampleSize;
    static constexpr unsigned int kloopcnt =
        (TP_LEN / m_ksampleSize); // Outer loop count to compute ED over given length.
    static constexpr unsigned int numOfParallelAccess =
        NUM_OF_PARALLEL_ACCESS_LUT; // number of parallel accesses from LUT is 4 (parallel loads)
    static constexpr unsigned int kSamples512bInFloatVec = T_buff_512b<float>::getLanes();
    static constexpr unsigned int kSamples1024bInFloatVec = T_buff_1024b<float>::getLanes();

    using inVec_t = ::aie::vector<TT_DATA, vecSize>;
    using outVec_t = ::aie::vector<TT_DATA, vecSize>;

    inVec_t* __restrict pIn = (inVec_t*)squaredIn.data();    // Input pointer for Squared data
    outVec_t* __restrict pOut = (inVec_t*)sqrtOut.data();    // Output pointer for ED output
    inVec_t inVecSquared = ::aie::zeros<TT_DATA, vecSize>(); // Input vector for Squared data

#if (__HAS_ACCUM_PERMUTES__ == 0)

    using T_buff_512b_bfloat16 = typename T_buff_512b<bfloat16>::v_type; // 512b of bfloat16 vector
    using T_buff_256b_bfloat16 = typename T_buff_256b<bfloat16>::v_type; // 256b of bfloat16 vector
    using T_buff_512b_uint16 = typename T_buff_512b<uint16>::v_type;     // 512b of uint16 vector
    using T_buff_512b_int32 = typename T_buff_512b<int32>::v_type;       // 512b of int32 vector
    using lutDataType = int16; // lut read data type is int16 to fetch two int8 values from LUT

    // Alias for Accumulator Reg based on given data types
    using T_accfloat16Vec_t = ::aie::accum<accfloat, kSamples512bInFloatVec>;
    using T_accfloat32Vec_t = ::aie::accum<accfloat, kSamples1024bInFloatVec>;
    using T_acc32Vec_t = ::aie::accum<acc32, T_buff_1024b<int32>::getLanes()>;

    T_buff_256b_bfloat16* pInBf16; // intermediate pointer for input data with data type of bfloat16.

    // Accumulator
    T_accfloat16Vec_t acc16Fp; // Declaration of Float Accumulator with size 16
    T_accfloat32Vec_t acc32Fp; // Declaration of Float Accumulator with size 32
    T_acc32Vec_t acc32;        // Declaration of Integer Accumulator with size 32

    static constexpr unsigned int kSamples512bInBf16Vec = T_buff_512b<bfloat16>::getLanes();
    static constexpr unsigned int kSamples256bInBf16Vec = T_buff_256b<bfloat16>::getLanes();
    static constexpr unsigned int kSamples512bInUint16Vec = T_buff_512b<uint16>::getLanes();
    static constexpr unsigned int kSamples256bInUint16Vec = T_buff_256b<uint16>::getLanes();

    // define a pointer to the BF16 vector with size as 32 byte
    T_buff_512b_bfloat16* pInVecFloatToBf16 = (T_buff_512b_bfloat16*)convertFloatToBfloat16Buff;

    if
        constexpr(std::is_same<TT_DATA, float>::value) {
            // convert squared float input as squared BF16 input to compue sqrt() on AIE-ML
            for (unsigned int insquaredIndx = 0; insquaredIndx < kloopcnt; insquaredIndx++)
                chess_prepare_for_pipelining chess_loop_count(kloopcnt) {
                    acc32Fp = ::aie::zeros<accfloat, vecSize>();            // Intialize accumulator with zeros.
                    acc32Fp.from_vector(inVecSquared.insert(0, *pIn++), 0); // copy vector data into accumulator
                    *pInVecFloatToBf16++ =
                        acc32Fp.template to_vector<bfloat16>(); // convert sqaured res. of FLOAT to BFLOAT16
                }

            // Assign pointer to the memory where BF16 input samples placed.
            pInBf16 = (T_buff_256b_bfloat16*)&convertFloatToBfloat16Buff;
        }
    else {
        // No conversion if input data type is BF16
        pInBf16 = (T_buff_256b_bfloat16*)squaredIn.data();

    } // End of If condition to check input data type.

    // Vectors
    T_buff_512b_bfloat16 inVecBf16 =
        ::aie::zeros<bfloat16, kSamples512bInBf16Vec>(); // Initialize input vector of data tye bfloat16
    T_buff_256b_bfloat16 vecSqrtOfMantissaLow =
        ::aie::zeros<bfloat16, kSamples256bInBf16Vec>(); // Initialize vector which stores LSB of Mantissa
    T_buff_256b_bfloat16 vecSqrtOfMantissaHigh =
        ::aie::zeros<bfloat16, kSamples256bInBf16Vec>(); // Initialize vector which stores MSB of Mantissa
    T_buff_256b_bfloat16 vecExponentData =
        ::aie::zeros<bfloat16, kSamples256bInBf16Vec>(); // Initialize a vector which stores Exponent Data
    T_buff_512b_uint16 bitExtractVec, lowerBitMask, upperBitMask,
        exponentAdjustVec;                        // Declaration of unsigned short vectors with size 32
    T_buff_512b_int32 indexOffset0, indexOffset1; // Declaration of integer vectors with size 16
    T_buff_256b_bfloat16 vecBf16One, vecBf16Two;  // Declaration of bfloat16 vectors with size 16

    unsigned int loopcnt = (TP_LEN / T_buff_512b<lutDataType>::getLanes()); // Loopcount to compute sqrt with LUTs
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

    // vectorized masks git
    lowerBitMask = ::aie::broadcast<uint16, kSamples512bInUint16Vec>(MASK_LOWEST_BITS);
    upperBitMask = ::aie::broadcast<uint16, kSamples512bInUint16Vec>(MASK_UPPER_BITS);
    exponentAdjustVec = ::aie::broadcast<uint16, kSamples512bInUint16Vec>(EXPONENT_ADJUSTMENT_CONSTANT);

    // Loop to compute SquareRoot using (2^((x-127)/2) * sqrtofMantissa_from_lut)
    for (int indx = 0; indx < loopcnt; indx++) chess_prepare_for_pipelining chess_loop_range(kSamples512bInBf16Vec, ) {
            inVecBf16.insert(0, *pInBf16++);
            inVecBf16.insert(1, *pInBf16++);

            // Extract LSB 8-bits
            bitExtractVec = ::aie::bit_and(inVecBf16.template cast_to<uint16>(), lowerBitMask);

            // Indx * 4
            acc32.from_vector(bitExtractVec, kUpshiftFactor2);

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
            vecBf16One = acc16Fp.template to_vector<bfloat16>(0);
            vecExponentData.insert(0, bitExtractVec.template extract<16>(1).template cast_to<bfloat16>());
            acc16Fp = ::aie::mul(vecSqrtOfMantissaHigh, vecExponentData);
            vecBf16Two = acc16Fp.template to_vector<bfloat16>(0);

            // compute sqrt and write the results into io buffer out
            if
                constexpr(std::is_same<TT_DATA, float>::value) {
                    // Float
                    inVecBf16 = concat(vecBf16One, vecBf16Two);
                    acc32Fp.template from_vector(inVecBf16);
                    *pOut++ = acc32Fp.template to_vector<float>(0);
                }
            else {
                // Bfloat16
                if
                    constexpr(std::is_same<TT_DATA, bfloat16>::value) {
                        *pOut++ = vecBf16One;
                        *pOut++ = vecBf16Two;
                    } // End of If condition to check input data type for bfloat16.
            }         // End of If Condition to check input data type for float.
        }             // End of LoopCount.
#else
    // Sqrt computation on AIE-1 Architecture
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
