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
#ifndef _XF_ISP_TYPES_H_
#define _XF_ISP_TYPES_H_

// --------------------------------------------------------------------
// Required files
// --------------------------------------------------------------------
#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
//#include "common/xf_utility.h"
#include "ap_axi_sdata.h"
#include "common/xf_axi_io.hpp"
#include "xf_config_params.h"

// Requried Vision modules
#include "imgproc/xf_bpc.hpp"
#include "imgproc/xf_gaincontrol.hpp"
#include "imgproc/xf_autowhitebalance.hpp"
#include "imgproc/xf_demosaicing.hpp"
#include "imgproc/xf_ltm.hpp"
#include "imgproc/xf_quantizationdithering.hpp"
#include "imgproc/xf_lensshadingcorrection.hpp"
#include "imgproc/xf_colorcorrectionmatrix.hpp"
#include "imgproc/xf_black_level.hpp"
#include "imgproc/xf_aec.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
#include "imgproc/xf_gammacorrection.hpp"
#include "imgproc/xf_hdrmerge.hpp"

#define XF_WIDTH 3840  // MAX_COLS
#define XF_HEIGHT 2160 // MAX_ROWS

#define XF_CV_DEPTH_IN_DR1 2
#define XF_CV_DEPTH_IN_DR2 2
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_IN_2 2
#define XF_CV_DEPTH_BPC_OUT 2
#define XF_CV_DEPTH_GAIN_OUT 2
#define XF_CV_DEPTH_DEMOSAIC_OUT 2
#define XF_CV_DEPTH_IMPOP 2
#define XF_CV_DEPTH_LTM_IN 2
#define XF_CV_DEPTH_LSC_OUT 2
#define XF_CV_DEPTH_DST 2
#define XF_CV_DEPTH_AEC_IN 2
#define XF_CV_DEPTH_OUT 2

#define S_DEPTH 4096

#define NO_EXPS 2

#define XF_NPPCX XF_NPPC1 // XF_NPPC1 --1PIXEL , XF_NPPC2--2PIXEL ,XF_NPPC4--4 and XF_NPPC8--8PIXEL

#define XF_BAYER_PATTERN XF_BAYER_RG // bayer pattern

#define T_8U 0
#define T_10U 0
#define T_12U 0
#define T_16U 1

#define XF_CCM_TYPE XF_CCM_bt2020_bt709

#if T_8U
#define IN_TYPE XF_8UC1  // XF_8UC1
#define XF_LTM_T XF_8UC3 // XF_8UC3
#define OUT_TYPE XF_8UC3 // XF_8UC3
#elif T_16U
#define IN_TYPE XF_16UC1  // XF_8UC1
#define XF_LTM_T XF_8UC3  // XF_8UC3
#define OUT_TYPE XF_16UC3 // XF_8UC3
#elif T_10U
#define IN_TYPE XF_10UC1  // XF_8UC1
#define XF_LTM_T XF_8UC3  // XF_8UC3
#define OUT_TYPE XF_10UC3 // XF_8UC3
#elif T_12U
#define IN_TYPE XF_12UC1  // XF_8UC1
#define XF_LTM_T XF_8UC3  // XF_8UC3
#define OUT_TYPE XF_12UC3 // XF_8UC3
#endif

#define SIN_CHANNEL_TYPE XF_8UC1

#define WB_TYPE XF_WB_SIMPLE

#define AEC_EN 0

#define XF_AXI_GBR 1

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 32

#define XF_USE_URAM 0 // uram enable

#if T_8U
#define W_B_SIZE 256
#endif
#if T_10U
#define W_B_SIZE 1024
#endif
#if T_12U
#define W_B_SIZE 4096
#endif
#if T_16U
#define W_B_SIZE 65536
#endif

// --------------------------------------------------------------------
// Macros definitions
// --------------------------------------------------------------------

// Useful macro functions definitions
#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, XF_NPPCX)
//#define OUT_DATA_WIDTH _DATA_WIDTH_(OUT_TYPE, XF_NPPCX)
//#define OUT_DATA_WIDTH _DATA_WIDTH_(XF_LTM_T, XF_NPPCX)
#define OUT_DATA_WIDTH _DATA_WIDTH_(XF_16UC1, XF_NPPCX)

#define AXI_WIDTH_IN _BYTE_ALIGN_(IN_DATA_WIDTH)
#define AXI_WIDTH_OUT _BYTE_ALIGN_(OUT_DATA_WIDTH)

#define NR_COMPONENTS 3
static constexpr int BLOCK_HEIGHT = 64;
static constexpr int BLOCK_WIDTH = 64;
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

#define BLACK_LEVEL 32
#define MAX_PIX_VAL (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPCX))) - 1

// HW Registers
typedef struct {
    uint16_t width;
    uint16_t height;
    //    uint16_t video_format;
    uint16_t bayer_phase;
} HW_STRUCT_REG;

#endif //_XF_ISP_TYPES_H_
