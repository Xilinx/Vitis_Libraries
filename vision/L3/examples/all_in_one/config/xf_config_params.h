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
#include "ap_axi_sdata.h"
#include "common/xf_axi_io.hpp"

// Requried Vision modules
#include "imgproc/xf_bpc.hpp"
#include "imgproc/xf_gaincontrol.hpp"
#include "imgproc/xf_autowhitebalance.hpp"
#include "imgproc/xf_demosaicing.hpp"
#include "imgproc/xf_gtm.hpp"
#include "imgproc/xf_quantizationdithering.hpp"
#include "imgproc/xf_lensshadingcorrection.hpp"
#include "imgproc/xf_colorcorrectionmatrix.hpp"
#include "imgproc/xf_black_level.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
#include "imgproc/xf_gammacorrection.hpp"
#include "imgproc/xf_ltm.hpp"
#include "imgproc/xf_rgbir.hpp"
#include "imgproc/xf_3dlut.hpp"
#include "imgproc/xf_hdrmerge.hpp"
#include "imgproc/xf_extract_eframes.hpp"
#include "imgproc/xf_hdrdecompand.hpp"
#include "imgproc/xf_degamma.hpp"
#include "imgproc/xf_duplicateimage.hpp"
#include "imgproc/xf_aec.hpp"

#define XF_WIDTH 1920  // 3840  // MAX_COLS
#define XF_HEIGHT 1080 // 2160 // MAX_ROWS

#define MAX_HEIGHT 2160
#define MAX_WIDTH 1928

#define XF_CV_DEPTH_imgInput 3
#define XF_CV_DEPTH_imgInput1 3
#define XF_CV_DEPTH_hdr_out 3
#define XF_CV_DEPTH_LEF 3
#define XF_CV_DEPTH_SEF 3
#define XF_CV_DEPTH_rggb_out 3
#define XF_CV_DEPTH_rggb_out_stats 3
#define XF_CV_DEPTH_rggb_out_aec 3
#define XF_CV_DEPTH_aec_out 3
#define XF_CV_DEPTH_fullir_out 3
#define XF_CV_DEPTH_bpc_out 3
#define XF_CV_DEPTH_blc_out 3
#define XF_CV_DEPTH_dgamma_out 3
#define XF_CV_DEPTH_lsc_out 3
#define XF_CV_DEPTH_gain_out 3
#define XF_CV_DEPTH_demosaic_out 3
#define XF_CV_DEPTH_awb_out 3
#define XF_CV_DEPTH_aecin 3
#define XF_CV_DEPTH_dst 3
#define XF_CV_DEPTH_ccm 3
#define XF_CV_DEPTH_3dlut 3
#define XF_CV_DEPTH_lut_out 3
#define XF_CV_DEPTH_3XWIDTH 3 * XF_WIDTH

#define XF_NPPCX XF_NPPC1 // XF_NPPC1 --1PIXEL , XF_NPPC2--2PIXEL ,XF_NPPC4--4 and XF_NPPC8--8PIXEL

#define XF_BAYER_PATTERN XF_BAYER_GR // bayer pattern Used in gaincontrol, demosaicing, rgbir2bayer

#define DEBUG 0

#define T_8U 0
#define T_10U 0
#define T_12U 0
#define T_16U 1

#define USE_HDR 1
#define USE_HDR_FUSION 0
#define USE_RGBIR 0
#define USE_AEC 1
#define USE_DEGAMMA 1
#define USE_AWB 1
#define USE_CCM 1
#define USE_LTM 1
#define USE_GTM 0
#define USE_QND 0
#define USE_3DLUT 0
#define USE_CSC 0

#define SCALEFACTOR 256 // used in ref
#define MAXREPRESENTEDVALUE 65536

#define DEGAMMA_KP 8

#define XF_CCM_TYPE XF_CCM_bt2020_bt709 /* Used in ccm */

#if T_8U
#define IN_TYPE XF_8UC1  // XF_8UC1
#define XF_GTM_T XF_8UC3 // XF_8UC3
#define OUT_TYPE XF_8UC3 // XF_8UC3
#elif T_16U
#define IN_TYPE XF_16UC1  // XF_8UC1
#define XF_GTM_T XF_8UC3  // XF_8UC3
#define OUT_TYPE XF_16UC3 // XF_8UC3
#elif T_10U
#define IN_TYPE XF_10UC1  // XF_8UC1
#define XF_GTM_T XF_8UC3  // XF_8UC3
#define OUT_TYPE XF_10UC3 // XF_8UC3
#elif T_12U
#define IN_TYPE XF_12UC1  // XF_8UC1
#define XF_GTM_T XF_8UC3  // XF_8UC3
#define OUT_TYPE XF_12UC3 // XF_8UC3
#endif

#define SIN_CHANNEL_TYPE XF_8UC1 /* Used in gtm */
#define AEC_SIN_CHANNEL_TYPE XF_16UC1
static constexpr int ERROR_THRESHOLD = 5;

#if T_8U
#define CVTYPE unsigned char
#define CV_IN_TYPE CV_8UC1
#define CV_OUT_TYPE CV_8UC3
#define CV_GTM_IN_TYPE CV_8UC3

#define CV_GTM_TYPE CV_8UC3
#define MAX_PIX 255.0
#else
#define CVTYPE unsigned short
#define CV_IN_TYPE CV_16UC1
#define CV_OUT_TYPE CV_16UC3
#define CV_GTM_IN_TYPE CV_16UC3

#define CV_GTM_TYPE CV_8UC3
#define MAX_PIX 65535.0

#endif

#define WB_TYPE XF_WB_GRAY /*"0" for GRAY,"1" for SIMPLE*/

#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128
#define LUT_PTR_WIDTH 128

#define NUM_V_BLANK_LINES 8 /* Used in HDR */
#define NUM_H_BLANK 8       /* Used in HDR */

#define XF_USE_URAM 0 // uram enable Used in HDR, rgbir2bayer, lut3d

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

#if T_8U
#define AEC_HIST_SIZE 256
#elif T_10U
#define AEC_HIST_SIZE 1024
#else
#define AEC_HIST_SIZE 4096
#endif

// --------------------------------------------------------------------
// Macros definations
// --------------------------------------------------------------------

// Useful macro functions definations
#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, XF_NPPCX)
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

static constexpr int FILTERSIZE1 = 5, FILTERSIZE2 = 3;
static constexpr int LUT_DIM = 33;
static constexpr int SQ_LUTDIM = LUT_DIM * LUT_DIM;
// HW Registers
typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t bayer_phase;
} HW_STRUCT_REG;

#endif //_XF_ISP_TYPES_H_
