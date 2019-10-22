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

#ifndef _XF_REMAP_CONFIG_H_
#define _XF_REMAP_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_remap.hpp"
#include "xf_config_params.h"

typedef unsigned short int uint16_t;

/* Define width and height of the image	*/
#define XF_WIDTH 128
#define XF_HEIGHT 128

#if GRAY
#define TYPE XF_8UC1
#else
#define TYPE XF_8UC3
#endif

void remap_accel(xf::cv::Mat<TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& inMat,
                 xf::cv::Mat<TYPE, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& remappedMat,
                 xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapxMat,
                 xf::cv::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>& mapyMat);

#endif
