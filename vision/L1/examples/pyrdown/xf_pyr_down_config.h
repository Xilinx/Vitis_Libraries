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

#ifndef _XF_PYR_DOWN_CONFIG_
#define _XF_PYR_DOWN_CONFIG_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "xf_config_params.h"
#include "common/xf_utility.hpp"
#include "imgproc/xf_pyr_down.hpp"
#define NPC_T XF_NPPC1
#if RGBA
#define TYPE XF_8UC3
#define CH_TYPE XF_RGB

#else
#define TYPE XF_8UC1
#define CH_TYPE XF_GRAY
#endif

#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 256

void pyr_down_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                    ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                    int in_rows,
                    int in_cols,
                    int out_rows,
                    int out_cols);
#endif
