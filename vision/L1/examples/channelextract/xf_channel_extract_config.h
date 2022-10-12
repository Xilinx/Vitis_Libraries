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

#ifndef _XF_CHANNEL_EXTRACT_CONFIG_H_
#define _XF_CHANNEL_EXTRACT_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_channel_extract.hpp"
#include "xf_config_params.h"

// Image width and height
#define HEIGHT 128
#define WIDTH 128

#if SPC
#define NPC1 XF_NPPC1
#elif MPC
#if NPC4
#define NPC1 XF_NPPC4
#elif NPC8
#define NPC1 XF_NPPC8
#endif // NPC4
#endif // SPC

#if T_16U

#if SPC
#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 16
#elif MPC
#if NPC4
#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 64
#elif NPC8
#define INPUT_PTR_WIDTH 512
#define OUTPUT_PTR_WIDTH 128
#endif // NPC4
#endif // SPC

#define CV_OUTTYPE CV_16UC1
#define OUTPUT_CH_TYPE XF_16UC1
#if BGR
#define INPUT_CH_TYPE XF_16UC3
#define CV_INTYPE CV_16UC3
#elif BGRA
#define INPUT_CH_TYPE XF_16UC4
#define CV_INTYPE CV_16UC4
#endif // BGR

#elif T_8U

#if SPC
#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 8
#elif MPC
#if NPC4
#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 32
#elif NPC8
#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 64
#endif // NPC4
#endif // SPC

#define CV_OUTTYPE CV_8UC1
#define OUTPUT_CH_TYPE XF_8UC1
#if BGR
#define INPUT_CH_TYPE XF_8UC3
#define CV_INTYPE CV_8UC3
#elif BGRA
#define INPUT_CH_TYPE XF_8UC4
#define CV_INTYPE CV_8UC4
#endif // BGR
#endif // T_16U

#endif //_XF_CHANNEL_EXTRACT_CONFIG_H_

void channel_extract_accel(
    ap_uint<INPUT_PTR_WIDTH>* img_rgba, ap_uint<OUTPUT_PTR_WIDTH>* img_gray, uint16_t channel, int rows, int cols);