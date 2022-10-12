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

#ifndef _XF_HDR_DECOMPAND_CONFIG_H_
#define _XF_HDR_DECOMPAND_CONFIG_H_

#include "hls_stream.h"
#include <ap_int.h>
#include "common/xf_common.hpp"
#include "common/xf_structs.hpp"

#include "xf_config_params.h"
#include "imgproc/xf_hdrdecompand.hpp"

/* Set the image height and width */
#define HEIGHT 1080
#define WIDTH 1920

// Resolve the optimization type:
#if SPC
#define NPC1 XF_NPPC1
#define INPUT_PTR_WIDTH 16
#define OUTPUT_PTR_WIDTH 32
#endif
#if MPC
#define NPC1 XF_NPPC2
#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 64
#endif

// Set the input and output pixel depth:

#if T_12U
#define IN_TYPE XF_16UC1
#define OUT_TYPE XF_32UC1
#else
#define IN_TYPE XF_16UC1
#define OUT_TYPE XF_32UC1
#endif

void hdrdecompand_accel(ap_uint<INPUT_PTR_WIDTH>* img_in,
                        ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                        int params[3][4][3],
                        unsigned short bayerp,
                        int height,
                        int width);

#endif //_XF_HDR_DECOMPAND_CONFIG_H_
