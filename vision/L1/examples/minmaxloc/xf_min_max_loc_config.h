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

#ifndef _XF_MIN_MAX_LOC_CONFIG_H_
#define _XF_MIN_MAX_LOC_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.hpp"
#include "core/xf_min_max_loc.hpp"

#if T_8U
#define PTYPE XF_8UC1
#define INTYPE unsigned char
#endif
#if T_16U
#define PTYPE XF_16UC1
#define INTYPE unsigned short
#endif
#if T_16S
#define PTYPE XF_16SC1
#define INTYPE short
#endif
#if T_32S
#define PTYPE XF_32SC1
#define INTYPE unsigned int
#endif

#if NO
#define _NPPC XF_NPPC1
#endif

#if RO
#define _NPPC XF_NPPC8
#endif

/*  set the height and weight  */
#define HEIGHT 128
#define WIDTH 128

void min_max_loc_accel(xf::cv::Mat<PTYPE, HEIGHT, WIDTH, _NPPC>& imgInput,
                       int32_t& min_value,
                       int32_t& max_value,
                       unsigned short& _min_locx,
                       unsigned short& _min_locy,
                       unsigned short& _max_locx,
                       unsigned short& _max_locy);

#endif
