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

#ifndef _XF_CHANNEL_COMBINE_CONFIG_H_
#define _XF_CHANNEL_COMBINE_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_channel_combine.hpp"
#include "xf_config_params.h"

#define HEIGHT 128
#define WIDTH 128

#if FOUR_INPUT
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC4
#define CV_TYPE CV_8UC4
#endif

#if THREE_INPUT
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC3
#define CV_TYPE CV_8UC3
#endif

#if TWO_INPUT
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC2
#endif

#if TWO_INPUT

void channel_combine_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput1,
                           xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput2,
                           xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC>& imgOutput);
#elif THREE_INPUT
void channel_combine_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput1,
                           xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput2,
                           xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput3,
                           xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC>& imgOutput);
#else
void channel_combine_accel(xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput1,
                           xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput2,
                           xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput3,
                           xf::cv::Mat<IN_TYPE, HEIGHT, WIDTH, NPC>& imgInput4,
                           xf::cv::Mat<OUT_TYPE, HEIGHT, WIDTH, NPC>& imgOutput);
#endif

#endif //_XF_CHANNEL_COMBINE_CONFIG_H_
