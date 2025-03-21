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

#ifndef _XF_GC_CONFIG_H_
#define _XF_GC_CONFIG_H_

#include "common/xf_common.hpp"
#include "hls_stream.h"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_agc.hpp"
#include "imgproc/xf_autowhitebalance.hpp"

#include <ap_int.h>

#define NPPCX XF_NPPC1

#define T_8U 0
#define T_16U 1

#if T_8U
#define HIST_SIZE 256
#endif
#if T_10U
#define HIST_SIZE 1024
#endif
#if T_16U || T_12U
#define HIST_SIZE 4096
#endif

#if T_8U
#define IN_TYPE XF_8UC3
#elif T_16U
#define IN_TYPE XF_16UC3
#endif

void autogaincontrol_accel(unsigned int histogram[XF_CHANNELS(IN_TYPE, NPPCX)][HIST_SIZE],
                           uint16_t gain[XF_CHANNELS(IN_TYPE, NPPCX)]);
#endif
_XF_GC_CONFIG_H_
