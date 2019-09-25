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

#ifndef _XF_CUSTOM_CONV_CONFIG_H_
#define _XF_CUSTOM_CONV_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_custom_convolution.hpp"
#include "xf_config_params.h"

/*  specify the shift parameter */
#define SHIFT 15

/* set the height and width */
#define HEIGHT 128
#define WIDTH 128

#if RO
#define NPC_T XF_NPPC8
#endif
#if NO
#define NPC_T XF_NPPC1
#endif
#if GRAY
#if OUT_8U

#define INTYPE XF_8UC1
#define OUTTYPE XF_8UC1

#endif

#if OUT_16S

#define INTYPE XF_8UC1
#define OUTTYPE XF_16SC1

#endif

#else
#if OUT_8U

#define INTYPE XF_8UC3
#define OUTTYPE XF_8UC3

#endif

#if OUT_16S

#define INTYPE XF_8UC3
#define OUTTYPE XF_16SC3

#endif
#endif
void Filter2d_accel(xf::cv::Mat<INTYPE, HEIGHT, WIDTH, NPC_T>& _src,
                    xf::cv::Mat<OUTTYPE, HEIGHT, WIDTH, NPC_T>& _dst,
                    short int* filter_ptr,
                    unsigned char shift);

#endif // end of _XF_CUSTOM_CONV_CONFIG_H_
