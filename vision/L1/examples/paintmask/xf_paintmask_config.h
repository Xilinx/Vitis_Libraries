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

#ifndef _XF_PAINTMASK_CONFIG_H_
#define _XF_PAINTMASK_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "imgproc/xf_paintmask.hpp"
#include "xf_config_params.h"

typedef ap_uint<8> ap_uint8_t;
typedef ap_uint<64> ap_uint64_t;

/*  set the height and weight  */
#define HEIGHT 128
#define WIDTH 128

#if RO
#define NPIX XF_NPPC8
#endif
#if NO
#define NPIX XF_NPPC1
#endif

#if GRAY
#define SRC_T XF_8UC1
#define MASK_T XF_8UC1
#else
#define SRC_T XF_8UC3
#define MASK_T XF_8UC3
#endif
void paintmask_accel(xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPIX>& _src,
                     xf::cv::Mat<MASK_T, HEIGHT, WIDTH, NPIX>& in_mask,
                     xf::cv::Mat<SRC_T, HEIGHT, WIDTH, NPIX>& _dst,
                     unsigned char color[XF_CHANNELS(SRC_T, NPIX)]);
#endif // end of _XF_PAINTMASK_CONFIG_H_
