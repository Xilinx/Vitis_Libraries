/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef _XF_ISP_CONFIG_PARAMS_H_
#define _XF_ISP_CONFIG_PARAMS_H_

// --------------------------------------------------------------------
// Required files
// --------------------------------------------------------------------
#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "ap_axi_sdata.h"
#include "common/xf_axi_io.hpp"
#include "common/xf_infra.hpp"

// Required Vision modules

#include "imgproc/xf_gaincontrol.hpp"
#include "imgproc/xf_gammacorrection.hpp"
#include "imgproc/xf_resize.hpp"
#include "imgproc/xf_quantizationdithering.hpp"
#include "imgproc/xf_black_level.hpp"
#include "imgproc/xf_median_blur.hpp"
#include "imgproc/xf_clahe.hpp"

//  XF_CV_DEPTHS for all the pipeline functions

#define XF_CV_DEPTH_LSC_OUT 2
#define XF_CV_DEPTH_GAIN_OUT 2
#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define XF_CV_DEPTH_LUT_IN 2
#define XF_CV_DEPTH_LUT_OUT 2

//  common configurations

#define XF_WIDTH 1920  // 2472
#define XF_HEIGHT 1080 // 2064
#define XF_NPPCX XF_NPPC4
#define T_8U 0
#define T_10U 0
#define T_12U 0
#define T_16U 1
#define IN_TYPE XF_16UC1
#define XF_GTM_T XF_8UC1
#define OUT_TYPE XF_16UC1

// pipeline_config_info register info

#define IN_C_TYPE \
    0 // IN_C_TYPE : Pipeline processing single(Mono) or multi-channel(Color) frames          0- Mono, 1 - Color
#define IN_BW_MODE \
    0 // IN_BW_MODE : Input frame bit-depth info                                              0 - 8bit, 1 - 10 bit, 2 -
      // 12 bit, 3 - 14 bit, 4 - 16 bit, 5 - 24 bit
#define OUT_C_TYPE \
    0 // OUT_C_TYPE : (Optional )                                                             0- Mono, 1 - Color
#define OUT_BW_MODE \
    0 // OUT_BW_MODE : output frame bit-depth info                                            0 - 8bit, 1 - 10 bit, 2 -
      // 12 bit, 3 - 14 bit, 4 - 16 bit, 5 - 24 bit
#define NPPCX xf::cv::log2<XF_NPPCX>::fvalue
#define NUM_STREAMS 1

// config info structure

enum config_info_index {
    IN_C_TYPE_INDEX = 0,
    IN_BW_MODE_INDEX = 1,
    OUT_C_TYPE_INDEX = 4,
    OUT_BW_MODE_INDEX = 5,
    NPPCX_INDEX = 8,
    NUM_STREAMS_INDEX = 12
};

//
// max_supported_size info structure
//
enum max_supported_size_index { MAX_WIDTH_INDEX = 0, MAX_HEIGHT_INDEX = 16 };

// Function specific parameters

#define BLACK_LEVEL 32
#define INTERPOLATION 1
#define XF_NEWHEIGHT 1080
#define XF_NEWWIDTH 1920
#define XF_USE_URAM 1
#define MAXDOWNSCALE 2
#define SCALE_FACTOR 256
#define WINDOW_SIZE 3
#define MUL_VALUE_WIDTH 16
#define FL_POS 15
#define USE_DSP 1
static constexpr int CLIPLIMIT = 32;
static constexpr int TILES_Y_MIN = 2;
static constexpr int TILES_X_MIN = 2;
static constexpr int TILES_Y_MAX = 4;
static constexpr int TILES_X_MAX = 4;

// --------------------------------------------------------------------
// Macros definitions
// --------------------------------------------------------------------

// Useful macro function definitions
#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, XF_NPPCX)

#define OUT_DATA_WIDTH _DATA_WIDTH_(XF_GTM_T, XF_NPPCX)

#define AXI_WIDTH_IN _BYTE_ALIGN_(IN_DATA_WIDTH)
#define AXI_WIDTH_OUT _BYTE_ALIGN_(OUT_DATA_WIDTH)

// --------------------------------------------------------------------
// Internal types
// --------------------------------------------------------------------
// Input/Output AXI video buses
typedef ap_axiu<AXI_WIDTH_IN, 1, 1, 1> InVideoStrmBus_t;
typedef ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> OutVideoStrmBus_t;

// Input/Output AXI video stream
typedef hls::stream<InVideoStrmBus_t> InVideoStrm_t;
typedef hls::stream<OutVideoStrmBus_t> OutVideoStrm_t;

#if T_8U
#define HIST_SIZE 256
#elif T_10U
#define HIST_SIZE 1024
#else
#define HIST_SIZE 4096
#endif

// Flags to enable or disable a kernel in the pippeline
// These flags decides whether hardware should be formed for the given kernel

#define XF_BLC_EN 1
#define XF_MEDIAN_EN 1
#define XF_GAIN_EN 1
#define XF_GAMMA_EN 1
#define XF_TM_EN 1   // In this pipeline "XF_TM_EN" is fixed to '1"
#define XF_TM_TYPE 2 // LTM = 0,gtm = 1,xf_QuatizationDithering = 2. In this pipeline "XF_TM_TYPE" is fixed to  "2"
#define XF_CLAHE_EN 1

// Below flags are only informative for the host/driver to know which of the kernels in the pipeline can be bypassable
// during runtime.
// These flags should be updated based on whether the individual kernel in the pipeline is programed with bypassable
// feature or not.
// If the enable flag of a particular kernel is disabled the corresponding bypass flag below should also be disabled.

#define XF_MEDIAN_BYPASS_EN 1
#define XF_GAIN_BYPASS_EN 1
#define XF_GAMMA_BYPASS_EN 1
#define XF_BLC_BYPASS_EN 1
#define XF_CLAHE_BYPASS_EN 1

// const enum for enable shift

enum kernel_index_info {
    XF_BLC_EN_INDEX = 4,
    XF_GAIN_EN_INDEX = 8,
    XF_TM_EN_INDEX = 12,
    XF_TM_TYPE_INDEX = 13,
    XF_GAMMA_EN_INDEX = 15,
    XF_CLAHE_EN_INDEX = 21,
    XF_MEDIAN_EN_INDEX = 22,
    XF_RESIZE_EN_INDEX = 23
};

void ISPPipeline_accel(InVideoStrm_t& s_axis_video,
                       OutVideoStrm_t& m_axis_video,
                       unsigned int common_config,
#if XF_BLC_EN
                       unsigned int blc_config_1,
                       unsigned int blc_config_2,
#endif

                       unsigned int resize_config,

#if XF_GAIN_EN
                       unsigned int gain_control_config_1,
#endif
#if XF_GAMMA_EN
                       unsigned char gamma_lut[256],
#endif
#if XF_CLAHE_EN
                       int clahe_config_1,
                       int clahe_config_2,
#endif
                       unsigned int& pipeline_config_info,
                       unsigned int& max_supported_size,
                       unsigned int& funcs_available,
                       unsigned int& funcs_bypassable,
                       unsigned int funcs_bypass_config);

#endif //_XF_ISP_TYPES_H_
