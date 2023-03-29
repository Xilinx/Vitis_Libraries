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

#ifndef _XF_SVM_CONFIG_H_
#define _XF_SVM_CONFIG_H_

/////  To set the parameters in the test-bench
/////

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_svm.hpp"

#define IN_ARRAY_SIZE_1 200
#define IN_ARRAY_SIZE_2 200
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2

#define TOTAL_ARRAY_ELEMENTS 200
#define NO_OF_KERNEL_ELEMENTS 200
#define INDEX_ARR_1 0
#define INDEX_ARR_2 0

#define IN_FRAC_BITS_1 15
#define IN_FRAC_BITS_2 15

// Set the optimization type:
#define NPPCX XF_NPPC1
#define IN_TYPE XF_16SC1
#define OUT_TYPE XF_64SC1
#define INPUT_PTR_WIDTH 16
#define OUTPUT_PTR_WIDTH 64

#endif
// end of _XF_SVM_CONFIG_H_
