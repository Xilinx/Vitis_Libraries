/*
 * Copyright 2021 Xilinx, Inc.
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

#include "ap_fixed.h"

// MRD defines 1.15 format where the 1 is a "sign" bit
const int TOTAL_BITS_IN = 16;
const int I_BITS_IN = 1;

// sufficient for all but matrix types 6 and 7
const int TOTAL_BITS_OUT = 28;
const int I_BITS_OUT = 13;

typedef ap_fixed<TOTAL_BITS_IN, I_BITS_IN, AP_RND_CONV> MATRIX_IN_T;
typedef ap_fixed<TOTAL_BITS_OUT, I_BITS_OUT, AP_RND_CONV> MATRIX_OUT_T;

#endif
