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

#ifndef _XF_REMAP_CONFIG_H_
#define _XF_REMAP_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "common/xf_axi_io.hpp"
#include "common/xf_infra.hpp"
#include "imgproc/xf_remap.hpp"

#define HEIGHT 128
#define WIDTH 128

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_MAP_X 2
#define XF_CV_DEPTH_MAP_Y 2
#define XF_CV_DEPTH_OUT 2

// The type of interpolation, define INTERPOLATION as 0 for Nearest Neighbour
// or 1 for Bilinear
#define INTERPOLATION 1

// Resolve interpolation type:
#if (INTERPOLATION == 0)
#define XF_REMAP_INTERPOLATION_TYPE XF_INTERPOLATION_NN
#else
#define XF_REMAP_INTERPOLATION_TYPE XF_INTERPOLATION_BILINEAR
#endif

// Mat types
#define MAPXY_TYPE XF_32SC1
// Configure this based on the number of rows needed for the remap purpose
// e.g., If its a right to left flip two rows are enough
#define XF_WIN_ROWS 8
#define BARREL 0
#define XF_USE_URAM 1

#define GRAY 0
#define RGB 1

// Set the optimization type3
// XF_NPPC1, XF_NPPC2 are available for this algorithm currently
#define NPPCX XF_NPPC1
#define IN_TYPE XF_8UC3
#define OUT_TYPE XF_8UC3

#define CV_IN_TYPE CV_8UC3
#define CV_OUT_TYPE CV_8UC3

// --------------------------------------------------------------------
// Macros definitions
// --------------------------------------------------------------------

// Useful macro function definitions
#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8)

#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, NPPCX)
#define MAPXY_DATA_WIDTH _DATA_WIDTH_(MAPXY_TYPE, NPPCX)

#define OUT_DATA_WIDTH _DATA_WIDTH_(OUT_TYPE, NPPCX)

#define AXI_WIDTH_IN _BYTE_ALIGN_(IN_DATA_WIDTH)
#define AXI_WIDTH_MAPXY _BYTE_ALIGN_(MAPXY_DATA_WIDTH)
#define AXI_WIDTH_OUT _BYTE_ALIGN_(OUT_DATA_WIDTH)

// --------------------------------------------------------------------
// Internal types
// --------------------------------------------------------------------
// Input/Output AXI video buses
typedef ap_axiu<AXI_WIDTH_IN, 1, 1, 1> InVideoStrmBus_t;
typedef ap_axiu<AXI_WIDTH_MAPXY, 1, 1, 1> MapxyStrmBus_t;
typedef ap_axiu<AXI_WIDTH_OUT, 1, 1, 1> OutVideoStrmBus_t;

// Input/Output AXI video stream
typedef hls::stream<InVideoStrmBus_t> InVideoStrm_t;
typedef hls::stream<MapxyStrmBus_t> MapxyStrm_t;
typedef hls::stream<OutVideoStrmBus_t> OutVideoStrm_t;

void remap_accel( // ap_uint<INPUT_PTR_WIDTH>* img_in,
    InVideoStrm_t& s_axis_video,
    OutVideoStrm_t& m_axis_video,
    MapxyStrm_t& map_x_axi,
    MapxyStrm_t& map_y_axi,
    int rows,
    int cols);

#endif
