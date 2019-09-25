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

#ifndef __XF_TRANSFORM_CONFIG__
#define __XF_TRANSFORM_CONFIG__
#include <ap_int.h>
#include <math.h>
#include "hls_stream.h"
#include "common/xf_common.hpp"
#include "xf_config_params.h"
#include "imgproc/xf_warp_transform.hpp"

#if GRAY
#define TYPE XF_8UC1
#else
#define TYPE XF_8UC3
#endif
void warp_transform_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1>& _src,
                          xf::cv::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1>& _dst,
                          float* R);
#endif
