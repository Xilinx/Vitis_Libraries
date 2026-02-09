/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_KALMANFILTER_CONFIG_H_
#define _XF_KALMANFILTER_CONFIG_H_

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"

#include "video/xf_kalmanfilter.hpp"

#define KF_N 16
#define KF_M 16
#define KF_C 16
#define KF_MTU 2
#define KF_MMU 2
#define XF_USE_URAM 0
#define KF_EKF 0
#define XF_CV_DEPTH_A 2
#define XF_CV_DEPTH_B 2
#define XF_CV_DEPTH_UQ 2
#define XF_CV_DEPTH_DQ 2
#define XF_CV_DEPTH_H 2
#define XF_CV_DEPTH_X0 2
#define XF_CV_DEPTH_U0 2
#define XF_CV_DEPTH_D0 2
#define XF_CV_DEPTH_R 2
#define XF_CV_DEPTH_U 2
#define XF_CV_DEPTH_Y 2
#define XF_CV_DEPTH_XOUT 2
#define XF_CV_DEPTH_UOUT 2
#define XF_CV_DEPTH_DOUT 2

// Set the pixel depth:
#define IN_TYPE XF_32FC1
#define OUT_TYPE XF_32FC1

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

// Set the optimization type:
#define NPPCX XF_NPPC1

#endif
//_XF_KALMANFILTER_CONFIG_H_
