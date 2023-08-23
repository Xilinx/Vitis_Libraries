/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#pragma once

#include <adf.h>

#include "aie_api/aie.hpp"
#include "aie_api/utils.hpp"
#include "aie_api/aie_adf.hpp"

#define KERNEL_RATIO 0.4 // 1.0

#define SIMD_DEPTH 4
#define LENGTH 32
#define LENGTH_FOC LENGTH
#define SPACE_DIMENSION 4
//#define WIN_SIZE 128
#define FIFO_DEPTH 8100
//#define N_SAMPLES 2048
#define INCREMENT_MATRIX (SIMD_DEPTH / SPACE_DIMENSION)
#define INCREMENT_VECTOR SIMD_DEPTH
#define SPEED_OF_SOUND 1540
#define INVERSE_SPEED_OF_SOUND 0.000649350649 //(1/SPEED_OF_SOUND)
#define F_NUMBER 2
#define PI 3.1415926536
#define PI_2 6.28318531
#define BYTE_ALIGNMENT 4
#define WIN_SIZE_VECTOR (LENGTH * BYTE_ALIGNMENT)
#define WIN_SIZE_MATRIX (LENGTH * BYTE_ALIGNMENT * SPACE_DIMENSION)
#define SAMPLING_FREQUENCY 100000000 // 0.000000083
#define POINTS_PER_ITERATION 32
#define WIN_SIZE_INTERPOLATOR (POINTS_PER_ITERATION * SIMD_DEPTH * BYTE_ALIGNMENT)

#define DIM_VECTOR (LENGTH / INCREMENT_VECTOR * SIMD_DEPTH)
#define DIM_MATRIX (LENGTH / INCREMENT_MATRIX * SIMD_DEPTH)

namespace us {
namespace L1 {

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void ones(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void outer(adf::input_buffer<T>& __restrict in1,
           adf::input_buffer<T>& __restrict in2,
           adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void outerStream(adf::input_buffer<T>& __restrict in1,
                 adf::input_buffer<T>& __restrict in2,
                 adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void sumMM(adf::input_buffer<T>& in1, adf::input_buffer<T>& in2, adf::output_buffer<T>& out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void tileVApo(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffMV(adf::input_buffer<T>& __restrict in1,
            adf::input_buffer<T>& __restrict in2,
            adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulMM(adf::input_buffer<T>& __restrict in1,
           adf::input_buffer<T>& __restrict in2,
           adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sum_axis_1(adf::input_buffer<T>& in1, adf::output_buffer<T>& out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void absV(adf::input_buffer<T>& __restrict in, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffVS(adf::input_buffer<T>& __restrict in1,
            adf::input_buffer<T>& __restrict in2,
            adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVV(adf::input_buffer<T>& __restrict in1,
           adf::input_buffer<T>& __restrict in2,
           adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void norm_axis_1(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void divVSSpeedOfSound(adf::input_buffer<T>& in1, adf::output_buffer<T>& out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVSStream(adf::input_buffer<T>& __restrict in1,
                 adf::input_buffer<T>& __restrict in2,
                 adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffVSStreamOut(adf::input_buffer<T>& __restrict in1,
                     adf::input_buffer<T>& __restrict in2,
                     adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVOne(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVVStreamIn1(adf::input_buffer<T>& __restrict in1,
                    adf::input_buffer<T>& __restrict in2,
                    adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffSV(adf::input_buffer<T>& __restrict in1,
            adf::input_buffer<T>& __restrict in2,
            adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void squareV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVV(adf::input_buffer<T>& __restrict in1,
           adf::input_buffer<T>& __restrict in2,
           adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sqrtV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulVSCRS(adf::input_buffer<T>& __restrict in1,
              adf::input_buffer<T>& __restrict in2,
              adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulLinSCRStreamIn(adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffOneLin(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffTwoLin(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffLinOne(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffLinTwo(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulVHalfInt(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void mulLinHalf(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void dataMover(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned VECDIM>
void diffThreeLin(adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
void equalS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM, const unsigned SCALAR>
void lessOrEqualThanS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVPi(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulV1e_16(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVS(adf::input_buffer<T>& __restrict in1,
           adf::input_buffer<T>& __restrict in2,
           adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void mulVSWS(adf::input_buffer<T>& __restrict in1, adf::input_buffer<T>& __restrict in2, adf::output_buffer<T>& out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void absVSW(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void reciprocalV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void cosV(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned LEN, const unsigned INCREMENT, const unsigned VECDIM>
void absVWS(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVHalf(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sumVOneSW(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void mulVVStreamOut(adf::input_buffer<T>& __restrict in1,
                    adf::input_buffer<T>& __restrict in2,
                    adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void diffOneVWW(adf::input_buffer<T>& __restrict in, adf::output_buffer<T>& __restrict out);

template <typename T, const unsigned int LEN, const unsigned int INCREMENT, const unsigned VECDIM>
void sign(adf::input_buffer<T>& __restrict in1, adf::output_buffer<T>& __restrict out);

// debug L2 under L1 level

// template <typename T,
//           const unsigned int LEN,
//           const unsigned int VECDIM>
// void UpdatingImagePoints_line_1d(T start_in,
//          T step_in,
//          adf::input_buffer<T>& __restrict idx_in,
//          adf::output_buffer<T>& __restrict des_out);

template <typename T, const unsigned int LEN, const unsigned int VECDIM>
void UpdatingImagePoints_line_1d(adf::input_buffer<T>& __restrict start_in,
                                 adf::input_buffer<T>& __restrict step_in,
                                 adf::input_buffer<T>& __restrict idx_in,
                                 adf::output_buffer<T>& __restrict des_out);

template <typename T, const unsigned int LEN, const unsigned int VECDIM>
void UpdatingImagePoints_line_1d_1(adf::input_buffer<T>& __restrict start_in,
                                   adf::input_buffer<T>& __restrict step_in,
                                   adf::input_buffer<T>& __restrict idx_in,
                                   adf::output_buffer<T>& __restrict des_out);

} // namespace L1

} // namespace us
