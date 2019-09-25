/*
 * Copyright 2019 Xilinx, Inc.
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

#ifndef _XF_CONVERT_BITDEPTH_CONFIG_H_
#define _XF_CONVERT_BITDEPTH_CONFIG_H_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "xf_config_params.h"
#include "core/xf_convert_bitdepth.hpp"

/* config width and height */
#define WIDTH 128
#define HEIGHT 128

#if RO
#define _NPC XF_NPPC8
#elif NO
#define _NPC XF_NPPC1
#endif

#if XF_CONVERT8UTO16U
#define IN_TYPE unsigned char
#define _SRC_T XF_8UC1
#define _DST_T XF_16UC1
#define OCV_INTYPE CV_8UC1
#define OCV_OUTTYPE CV_16UC1
#define CONVERT_TYPE XF_CONVERT_8U_TO_16U
#endif

#if XF_CONVERT8UTO16S
#define IN_TYPE unsigned char
#define _SRC_T XF_8UC1
#define _DST_T XF_16SC1
#define OCV_INTYPE CV_8UC1
#define OCV_OUTTYPE CV_16SC1
#define CONVERT_TYPE XF_CONVERT_8U_TO_16S
#endif

#if XF_CONVERT8UTO32S
#define _SRC_T XF_8UC1
#define _DST_T XF_32SC1
#define OCV_INTYPE CV_8UC1
#define OCV_OUTTYPE CV_32SC1
#define CONVERT_TYPE XF_CONVERT_8U_TO_32S
#define IN_TYPE unsigned char
#endif

#if XF_CONVERT16UTO32S
#define IN_TYPE unsigned short int
#define _SRC_T XF_16UC1
#define _DST_T XF_32SC1
#define OCV_INTYPE CV_16UC1
#define OCV_OUTTYPE CV_32SC1
#define CONVERT_TYPE XF_CONVERT_16U_TO_32S
#endif

#if XF_CONVERT16STO32S
#define IN_TYPE short int
#define _SRC_T XF_16SC1
#define _DST_T XF_32SC1
#define OCV_INTYPE CV_16SC1
#define OCV_OUTTYPE CV_32SC1
#define CONVERT_TYPE XF_CONVERT_16S_TO_32S
#endif

#if XF_CONVERT16UTO8U
#define IN_TYPE unsigned short int
#define _SRC_T XF_16UC1
#define _DST_T XF_8UC1
#define OCV_INTYPE CV_16UC1
#define OCV_OUTTYPE CV_8UC1
#define CONVERT_TYPE XF_CONVERT_16U_TO_8U
#endif

#if XF_CONVERT16STO8U
#define IN_TYPE short int
#define _SRC_T XF_16SC1
#define _DST_T XF_8UC1
#define CONVERT_TYPE XF_CONVERT_16S_TO_8U
#define OCV_INTYPE CV_16SC1
#define OCV_OUTTYPE CV_8UC1
#endif

#if XF_CONVERT32STO8U
#define _SRC_T XF_32SC1
#define _DST_T XF_8UC1
#define OCV_INTYPE CV_32SC1
#define OCV_OUTTYPE CV_8UC1
#define CONVERT_TYPE XF_CONVERT_32S_TO_8U
#define IN_TYPE int
#endif

#if XF_CONVERT32STO16U
#define _SRC_T XF_32SC1
#define _DST_T XF_16UC1
#define OCV_INTYPE CV_32SC1
#define OCV_OUTTYPE CV_16UC1
#define CONVERT_TYPE XF_CONVERT_32S_TO_16U
#define IN_TYPE int
#endif

#if XF_CONVERT32STO16S
#define _SRC_T XF_32SC1
#define _DST_T XF_16SC1
#define OCV_INTYPE CV_32SC1
#define OCV_OUTTYPE CV_16SC1
#define CONVERT_TYPE XF_CONVERT_32S_TO_16S
#define IN_TYPE int
#endif

void convert_bitdepth_accel(xf::cv::Mat<_SRC_T, HEIGHT, WIDTH, _NPC>& imgInput,
                            xf::cv::Mat<_DST_T, HEIGHT, WIDTH, _NPC>& imgOutput,
                            ap_int<4> _convert_type,
                            int shift);

#endif // _XF_CONVERT_BITDEPTH_CONFIG_H_
