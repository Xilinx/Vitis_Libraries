/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_CONFIG_PARAMS_IMPL_H_
#define _XF_CONFIG_PARAMS_IMPL_H_

#include "common/xf_types.hpp"
#include "common/xf_params.hpp"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "dnn/xf_preprocess_generic.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_resize.hpp"
#include "imgproc/xf_rgb2rgba.hpp"
#include <ap_int.h>
#include <hls_stream.h>
#include <ap_float.h>

#define _XF_SYNTHESIS_ 1
// Max image resoultion
static constexpr int WIDTH = 128;
static constexpr int HEIGHT = 128;

// Enable or disable color conversion
#define RGB2RGBA 1
#define XF_AXI_GBR 1

// Multiple enable support
// for Output Format
#define XF_INT8 1
#define XF_FP16 1
#define XF_BF16 1
#define XF_FP32 1

static constexpr int NPPCX = XF_NPPC4;

// preprocess kernel params out = (in - a) * b
// a (alpha or mean), b (beta or scale) and out (output value) are fixed point values
// and below params are used to configure
// the width and integer bits
static constexpr int WIDTH_A = 9;
static constexpr int IBITS_A = 8; // A is 9-bit wide and 8-bits are integer bits
static constexpr int WIDTH_B = 17;
static constexpr int IBITS_B = 1; // B is 17-bit wide and 1-bit is integer bit
static constexpr int WIDTH_OUT = 8;
static constexpr int IBITS_OUT = 8; // Output is 8-bit wide and all 8-bits are integer bits

// Resize configuration parameters
static constexpr int NEWWIDTH = 1920; // Maximum output image width
static constexpr int NEWHEIGHT = 1080;

static constexpr int MAXDOWNSCALE = 9;

static constexpr int INTERPOLATION = 1;

#define XF_USE_URAM 1

// Utility Macros and Functions - DO NOT EDIT any fileds after this line

#if RGB2RGBA
#define _XF_RGBA_ 1
#else
#define _XF_RGBA_ 0
#endif

// Bit positions
#define XF_FP32_BIT 3
#define XF_BF16_BIT 2
#define XF_FP16_BIT 1
#define XF_INT8_BIT 0

// Combined selection
#define SELECT (XF_FP32 << XF_FP32_BIT | XF_BF16 << XF_BF16_BIT | XF_FP16 << XF_FP16_BIT | XF_INT8 << XF_INT8_BIT)

// First, define all ENABLE_* macros to default (0)
#define ENABLE_FP32 0
#define ENABLE_BF16 0
#define ENABLE_FP16 0
#define ENABLE_INT8 0

// Then update their values as per selection
#if XF_FP32
#undef ENABLE_FP32
#define ENABLE_FP32 1
#elif XF_BF16
#undef ENABLE_BF16
#define ENABLE_BF16 1
#elif XF_FP16
#undef ENABLE_FP16
#define ENABLE_FP16 1
#elif XF_INT8
#undef ENABLE_INT8
#define ENABLE_INT8 1
#else
#error "No valid datatype defined"
#endif

#if ENABLE_FP32 //
#define XF_T float
#if _XF_RGBA_
static constexpr int OUT_TYPE = XF_32FC4;
#define XF_CV_TYPE CV_32FC4
#else
static constexpr int OUT_TYPE = XF_32FC3;
#define XF_CV_TYPE CV_32FC3
#endif
#define CV_PIX_TYPE float
#define TB_PIX_TYPE unsigned int
#elif ENABLE_FP16 //
#define XF_T half
#if _XF_RGBA_
static constexpr int OUT_TYPE = XF_16UC4;
#define XF_CV_TYPE CV_16UC4
#else
static constexpr int OUT_TYPE = XF_16UC3;
#define XF_CV_TYPE CV_16UC3
#endif
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#elif ENABLE_BF16 //
#define XF_T ap_float<16, 8>
#if _XF_RGBA_
#define OUT_TYPE XF_16UC4
#define XF_CV_TYPE CV_16UC4
#else
#define OUT_TYPE XF_16UC3
#define XF_CV_TYPE CV_16UC3
#endif
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#else // INT8 is default
#define XF_T ap_ufixed<WIDTH_A, IBITS_A, AP_RND>
#if _XF_RGBA_
static constexpr int OUT_TYPE = XF_8UC4;
#define XF_CV_TYPE CV_8UC4
#else
static constexpr int OUT_TYPE = XF_8UC3;
#define XF_CV_TYPE CV_8UC3
#endif
#define CV_PIX_TYPE unsigned char
#define TB_PIX_TYPE unsigned char
#endif

static constexpr int IN_TYPE = XF_8UC3;

#if RGB2RGBA
static constexpr int OUT_TYPE_NEW = XF_8UC4;
#else
static constexpr int OUT_TYPE_NEW = XF_8UC3;
#endif

// Pixels processed per cycle

#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, NPPCX)
#define OUT_DATA_WIDTH _DATA_WIDTH_(OUT_TYPE, NPPCX)

#define AXI_WIDTH_IN _BYTE_ALIGN_(IN_DATA_WIDTH)
#define AXI_WIDTH_OUT _BYTE_ALIGN_(OUT_DATA_WIDTH)

// Input/Output AXI video buses
typedef ap_axiu<AXI_WIDTH_IN, 1, 1, 1> InStrmBus_t;
typedef ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> OutStrmBus_t;

// Input/Output AXI video stream
typedef hls::stream<InStrmBus_t> InStream;
typedef hls::stream<OutStrmBus_t> OutStream;

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_CH_SWAP 2
#define XF_CV_DEPTH_RESIZE_OUT 2
#define XF_CV_DEPTH_CROP 2
#define XF_CV_DEPTH_OUT 2

void preprocess_accel(InStream& s_axis_video,  // Input AXI stream
                      OutStream& m_axis_video, // output image pointer
                      uint32_t params_int[2 * XF_CHANNELS(OUT_TYPE, NPPCX)],
                      int in_img_width,
                      int in_img_height,
                      int resize_width,
                      int resize_height,
                      uint32_t datatype);

#endif
