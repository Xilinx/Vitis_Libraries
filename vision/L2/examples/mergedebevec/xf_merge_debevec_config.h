/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef _XF_MERGE_DEBEVEC_CONFIG_H
#define _XF_MERGE_DEBEVEC_CONFIG_H

#include "hls_stream.h"
#include <ap_int.h>
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_merge_debevec.hpp"

static constexpr int SRC_NUM   = 4;
static constexpr int NPC       = XF_NPPC2;
static constexpr int IN_TYPE   = XF_8UC3;
static constexpr int OUT_TYPE  = XF_32FC3;

static constexpr int PTR_IN_WIDTH_LOG2  = 9;
static constexpr int PTR_OUT_WIDTH_LOG2 = 9;
static constexpr int PTR_IN_WIDTH       = (1 << PTR_IN_WIDTH_LOG2);
static constexpr int PTR_OUT_WIDTH      = (1 << PTR_OUT_WIDTH_LOG2);

static constexpr int HEIGHT = 2112;
static constexpr int WIDTH  = 2816;

//static constexpr int HEIGHT = 96;
//static constexpr int WIDTH  = 128;

extern "C" {
void mergedebevec_accel(ap_uint<PTR_IN_WIDTH>* src1,
                        ap_uint<PTR_IN_WIDTH>* src2,
                        ap_uint<PTR_OUT_WIDTH>* dst,
                        const float* times,
                        const float* input_response,
                        int height,
                        int width
                       );

void mergedebevec_accel_3(ap_uint<PTR_IN_WIDTH>* src1,
                          ap_uint<PTR_IN_WIDTH>* src2,
                          ap_uint<PTR_IN_WIDTH>* src3,
                          ap_uint<PTR_OUT_WIDTH>* dst,
                          const float* times,
                          const float* input_response,
                          int height,
                          int width
                         );

void mergedebevec_accel_4(ap_uint<PTR_IN_WIDTH>* src1,
                          ap_uint<PTR_IN_WIDTH>* src2,
                          ap_uint<PTR_IN_WIDTH>* src3,
                          ap_uint<PTR_IN_WIDTH>* src4,
                          ap_uint<PTR_OUT_WIDTH>* dst,
                          const float* times,
                          const float* input_response,
                          int height,
                          int width
                         );

}
#endif
