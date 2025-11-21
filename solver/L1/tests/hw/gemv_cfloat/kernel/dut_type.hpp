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


#ifndef _DUT_TYPE_HPP_
#define _DUT_TYPE_HPP_

#include <complex>

typedef std::complex<float> MATRIX_IN_T;
typedef std::complex<float> MATRIX_OUT_T;
typedef std::complex<float> OUT_TYPE;
typedef std::complex<float> IN_TYPE;

#define STRINGIZE(x) #x
#define TOSTR(x) STRINGIZE(x)
#define DATA_PATH TOSTR(_DATA_PATH)


//const unsigned int M = _ROWSA;
//const unsigned int K = _COLSA;
//const unsigned int N = _COLSB;
const unsigned int M = 32;
const unsigned int K = 16;
const unsigned int N = 1;
const unsigned ROWA= M;
const unsigned COLA= K;
const unsigned ROWB= K;
const unsigned COLB= N;
const unsigned ROWC= M;
const unsigned COLC= N;
const unsigned BR = 4;
const unsigned BC = 4;
//const unsigned BR = 32;
//const unsigned BC = 32;


#endif
