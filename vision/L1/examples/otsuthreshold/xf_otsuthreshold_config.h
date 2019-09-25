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

#ifndef _XF_OTSUTHRESHOLD_CONFIG_H_
#define _XF_OTSUTHRESHOLD_CONFIG_H_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_otsuthreshold.hpp"
#include "xf_config_params.h"

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#define HEIGHT 128
#define WIDTH 128

#if NO
#define NPPC XF_NPPC1
#define IN_TYPE ap_uint<8>
#define NPC1 0
#endif

#if RO
#define NPPC XF_NPPC8
#define IN_TYPE ap_uint<64>
#define NPC1 3
#endif

void otsuthreshold_accel(xf::cv::Mat<XF_8UC1, HEIGHT, WIDTH, NPPC>& imgInput, unsigned char& Otsuval);

#endif
