/*
 * Copyright 2023-2024 Xilinx, Inc.
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
#include "imgproc/xf_gtm.hpp"
#include "imgproc/xf_quantizationdithering.hpp"
#include "imgproc/xf_lensshadingcorrection.hpp"
#include "imgproc/xf_colorcorrectionmatrix.hpp"
#include "imgproc/xf_black_level.hpp"
#include "imgproc/xf_aec.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
#include "imgproc/xf_gammacorrection.hpp"
#include "imgproc/xf_median_blur.hpp"
#include "imgproc/xf_clahe.hpp"
#include "imgproc/xf_hdrmerge.hpp"
#include "imgproc/xf_extract_eframes.hpp"
#include "imgproc/xf_bpc.hpp"
#include "imgproc/xf_rgbir.hpp"
#include "imgproc/xf_3dlut.hpp"
#include "imgproc/xf_degamma.hpp"
#include "imgproc/xf_hdrdecompand.hpp"
#include "imgproc/xf_duplicateimage.hpp"

#define XF_WIDTH 256  // MAX_COLS
#define XF_HEIGHT 168 // MAX_ROWS

#define XF_CV_DEPTH_LEF 3
#define XF_CV_DEPTH_SEF 3
#define XF_CV_DEPTH_IN_0_1 3
#define XF_CV_DEPTH_IN_0_2 3
#define XF_CV_DEPTH_IN_1 3
#define XF_CV_DEPTH_IN_2 3
#define XF_CV_DEPTH_IN_3 3
#define XF_CV_DEPTH_IN_4 3
#define XF_CV_DEPTH_IN_5 3
#define XF_CV_DEPTH_IN_6 3
#define XF_CV_DEPTH_IN_7 3
#define XF_CV_DEPTH_IN_8 3
#define XF_CV_DEPTH_IN_9 3
#define XF_CV_DEPTH_OUT_0 3
#define XF_CV_DEPTH_OUT_1 3
#define XF_CV_DEPTH_OUT_2 3
#define XF_CV_DEPTH_OUT_3 3
#define XF_CV_DEPTH_OUT_4 3
#define XF_CV_DEPTH_OUT_5 3
#define XF_CV_DEPTH_OUT_6 3
#define XF_CV_DEPTH_OUT_7 3
#define XF_CV_DEPTH_3dlut 3
#define XF_CV_DEPTH_OUT_IR 3
#define XF_CV_DEPTH_3XWIDTH 3 * XF_WIDTH

#define XF_NPPC XF_NPPC1 // XF_NPPC1 --1PIXEL , XF_NPPC2--2PIXEL ,XF_NPPC4--4 and XF_NPPC8--8PIXEL

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

#if T_8U
#define CVTYPE unsigned char
#define CV_IN_TYPE CV_8UC1
#define PXL_TYPE unsigned char
#else
#define CVTYPE unsigned short
#define CV_IN_TYPE CV_16UC1
#define PXL_TYPE unsigned short
#endif

#define NUM_STREAMS 4

#define STRM_HEIGHT 168
#define STRM1_ROWS STRM_HEIGHT
#define STRM2_ROWS STRM_HEIGHT
#define STRM3_ROWS STRM_HEIGHT
#define STRM4_ROWS STRM_HEIGHT

#define SLICE_HEIGHT STRM_HEIGHT + 7
#define NUM_SLICES 1

#define SIN_CHANNEL_TYPE XF_8UC1
#define AEC_SIN_CHANNEL_TYPE XF_16UC1

#define USE_HDR_FUSION 0
#define USE_GTM 1
#define USE_LTM 0
#define USE_QnD 0
#define USE_RGBIR 1
#define USE_3DLUT 1
#define USE_DEGAMMA 1
#define USE_AEC 1
#define USE_AWB 1
#define USE_CSC 1

#if USE_HDR_FUSION
#define RD_MULT 2
#define RD_ADD 8

#else
#define RD_MULT 1
#define RD_ADD 0
#endif

#if USE_CSC
#define CV_OUT_TYPE CV_16UC1
#define OUT_PORT 4
#define CH_TYPE 1

#else
#define CV_OUT_TYPE CV_8UC3
#define OUT_PORT 8
#define CH_TYPE 3

#endif

#define WB_TYPE XF_WB_SIMPLE
#define DGAMMA_KP 8

#define AEC_EN 0

#define XF_AXI_GBR 1
#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64
#define LUT_PTR_WIDTH 128

#define NUM_V_BLANK_LINES 8
#define NUM_H_BLANK 8

#define XF_USE_URAM 0 // uram enable

#define MAX_HEIGHT (SLICE_HEIGHT) * 2
#define MAX_WIDTH XF_WIDTH + NUM_H_BLANK

#define S_DEPTH 4096
#define NO_EXPS 2
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

static constexpr int CLIPLIMIT = 32;
static constexpr int TILES_Y_MIN = 2;
static constexpr int TILES_X_MIN = 2;
static constexpr int TILES_Y_MAX = 4;
static constexpr int TILES_X_MAX = 4;
// --------------------------------------------------------------------
// Macros definations
// --------------------------------------------------------------------

// Useful macro functions definitions
#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, XF_NPPC)
//#define OUT_DATA_WIDTH _DATA_WIDTH_(OUT_TYPE, XF_NPPC)
//#define OUT_DATA_WIDTH _DATA_WIDTH_(XF_LTM_T, XF_NPPC)
#define OUT_DATA_WIDTH _DATA_WIDTH_(XF_16UC1, XF_NPPC)

#define AXI_WIDTH_IN _BYTE_ALIGN_(IN_DATA_WIDTH)
#define AXI_WIDTH_OUT _BYTE_ALIGN_(OUT_DATA_WIDTH)

#define NR_COMPONENTS 3
static constexpr int BLOCK_HEIGHT = 32;
static constexpr int BLOCK_WIDTH = 32;
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

#if T_8U
#define AEC_HIST_SIZE 256
#elif T_10U
#define AEC_HIST_SIZE 1024
#else
#define AEC_HIST_SIZE 4096
#endif

#define MAX_PIX_VAL (1 << (XF_DTPIXELDEPTH(IN_TYPE, XF_NPPC))) - 1
static constexpr int FILTERSIZE1 = 5, FILTERSIZE2 = 3;

static constexpr int LUT_DIM = 33;
static constexpr int SQ_LUTDIM = LUT_DIM * LUT_DIM;

// HW Registers
typedef struct {
    uint16_t width;
    uint16_t height;
    //    uint16_t video_format;
    uint16_t bayer_phase;
} HW_STRUCT_REG;

#endif //_XF_ISP_TYPES_H_
