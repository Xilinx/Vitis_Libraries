/*
 * Copyright 2022 Xilinx, Inc.
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
#include "ap_int.h"
#include <hls_stream.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <iterator>
using namespace std;

#define UPDATE_COUNT_HASH
//#define SHIFT_REG_SCAN

#define HASH 1
#define SCL 32
#define NUM_SMALL 64
#define NUM_SMALL_LOG 6

#define W_V (32)
#define W_AGG (32)
#define W_URAM (64)
#define DWIDTHS (256)

#define W_V_1 (26)   // 64M
#define W_AGG_1 (64) // 72 - 26 = 46, URAM bitwidth
#define W_URAM_1 (90)
typedef int DVERTEX;
typedef double DWEIGHT;
typedef int DF_V_T;
typedef ap_uint<64> AP2_V_T;
typedef int DF_E_T;
typedef int DF_D_T;
typedef float DF_W_T;
typedef int SMALL_T;
#define FIX_POINT
#ifdef FIX_POINT
typedef ap_uint<64> DF_WI_T;
#else
typedef float DF_WI_T;
#endif
