/*
 * Copyright (C) 2023 Advanced Micro Devices, Inc.
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
#include "xf_config_params.h"

// Requried Vision modules
#include "imgproc/xf_bpc.hpp"
#include "imgproc/xf_rgbir.hpp"
#include "imgproc/xf_gaincontrol.hpp"
#include "imgproc/xf_autowhitebalance.hpp"
#include "imgproc/xf_demosaicing.hpp"
#include "imgproc/xf_gtm.hpp"
#include "imgproc/xf_quantizationdithering.hpp"
#include "imgproc/xf_lensshadingcorrection.hpp"
#include "imgproc/xf_colorcorrectionmatrix.hpp"
#include "imgproc/xf_black_level.hpp"
#include "imgproc/xf_aec.hpp"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"
#include "imgproc/xf_gammacorrection.hpp"
#include "imgproc/xf_ltm.hpp"
#include "imgproc/xf_3dlut.hpp"
#include "imgproc/xf_hdrdecompand.hpp"
#include "imgproc/xf_degamma.hpp"
#include "imgproc/xf_convertTo.hpp"
#include "imgproc/xf_ispstats.hpp"
#include "imgproc/xf_duplicateimage.hpp"

#define S_DEPTH 4096

// --------------------------------------------------------------------
// Macros definations
// --------------------------------------------------------------------

// Useful macro functions definations
#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(XF_SRC_T, XF_NPPC)
#define OUT_DATA_WIDTH _DATA_WIDTH_(XF_16UC1, XF_NPPC)

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

#define HIST_SIZE_AEC 4096
#define HIST_SIZE_AWB 256

#define BLACK_LEVEL 32
#define MAX_PIX_VAL (1 << (XF_DTPIXELDEPTH(XF_SRC_T, XF_NPPC))) - 1

#define MAX_ZONES 64
#define NUM_OUT_CH 3
#define STATS_SIZE_AEC 4096
#define STATS_SIZE_AWB 256
#define FINAL_BINS_NUM 4
#define MERGE_BINS 0
#define DEGAMMA_KP 8

static constexpr int FILTERSIZE1 = 5, FILTERSIZE2 = 3;
static constexpr int LUT_DIM = 33;
static constexpr int SQ_LUTDIM = LUT_DIM * LUT_DIM;
// HW Registers
typedef struct {
    uint16_t width;
    uint16_t height;
    uint16_t bayer_phase;
} HW_STRUCT_REG;

// Defining mode reg bit position
enum mode_reg_bit {
    AWB_EN_LSB = 0,
    TM_EN_LSB = 1,
    QnD_EN_LSB = 2,
    LTM_EN_LSB = 3,
    GTM_EN_LSB = 4,
    CCM_EN_LSB = 5,
    LUT3D_EN_LSB = 6,
    CSC_EN_LSB = 7
};

#endif //_XF_ISP_TYPES_H_
