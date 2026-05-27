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

#ifndef _XF_BPC_CONFIG_H_
#define _XF_BPC_CONFIG_H_
#include "common/xf_common.hpp"
#include "hls_stream.h"
#include "imgproc/xf_bpc.hpp"
#include <ap_int.h>

// Set the image height and width
#define HEIGHT 2160
#define WIDTH 3840

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2

#define T_8U 1
#define T_16U 0

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define INPUT_PTR_WIDTH 256
#define OUTPUT_PTR_WIDTH 256

#endif
//_XF_BPC_CONFIG_H_
