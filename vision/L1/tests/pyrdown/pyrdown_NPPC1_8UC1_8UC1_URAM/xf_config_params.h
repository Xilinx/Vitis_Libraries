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

#ifndef _XF_PYR_DOWN_CONFIG_
#define _XF_PYR_DOWN_CONFIG_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"

#include "common/xf_utility.hpp"
#include "imgproc/xf_pyr_down.hpp"

#define WIDTH 128
#define HEIGHT 128

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define OUT_WIDTH ((WIDTH + 1) >> 1)
#define OUT_HEIGHT ((HEIGHT + 1) >> 1)

#define GRAY 1
#define RGB 0
#define XF_USE_URAM 1

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC1

#define INPUT_PTR_WIDTH 8
#define OUTPUT_PTR_WIDTH 8

void pyr_down_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                    ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                    int in_rows,
                    int in_cols,
                    int out_rows,
                    int out_cols);
#endif
