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
#ifndef _XF_CROP_CONFIG_H_
#define _XF_CROP_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_crop.hpp"

typedef ap_uint<8> ap_uint8_t;
typedef ap_uint<64> ap_uint64_t;

/*  set the height and weight  */
#define HEIGHT 2160
#define WIDTH 3840
#define MEMORYMAPPED_ARCH 1 // 0 is stream ,1 is memorymapped

#if MEMORYMAPPED_ARCH
#define XF_CV_DEPTH_IN -1
#define XF_CV_DEPTH_OUT -1
#define XF_CV_DEPTH_OUT_1 -1
#define XF_CV_DEPTH_OUT_2 -1
#else
#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define XF_CV_DEPTH_OUT_1 2
#define XF_CV_DEPTH_OUT_2 2
#endif

#define NUM_ROI 3

/*  set the type of input image*/
#define GRAY 1

#define NPPCX XF_NPPC1
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC1

#define INPUT_CH_TYPE 1
#define OUTPUT_CH_TYPE 1

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

#endif
// end of _XF_CROP_CONFIG_H_
