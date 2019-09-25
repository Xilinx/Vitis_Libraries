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

#ifndef _XF_KALMANFILTER_CONFIG_H_
#define _XF_KALMANFILTER_CONFIG_H_

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "xf_config_params.h"

#include "video/xf_kalmanfilter.hpp"

#define KF_TYPE XF_32FC1
#define KF_NPC XF_NPPC1

void kalmanfilter_accel(xf::cv::Mat<KF_TYPE, KF_N, KF_N, KF_NPC>& A_mat,
#if KF_C != 0
                        xf::cv::Mat<KF_TYPE, KF_N, KF_C, KF_NPC>& B_mat,
#endif
                        xf::cv::Mat<KF_TYPE, KF_N, KF_N, KF_NPC>& Uq_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, 1, KF_NPC>& Dq_mat,
                        xf::cv::Mat<KF_TYPE, KF_M, KF_N, KF_NPC>& H_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, 1, KF_NPC>& X0_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, KF_N, KF_NPC>& U0_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, 1, KF_NPC>& D0_mat,
                        xf::cv::Mat<KF_TYPE, KF_M, 1, KF_NPC>& R_mat,
#if KF_C != 0
                        xf::cv::Mat<KF_TYPE, KF_C, 1, KF_NPC>& u_mat,
#endif
                        xf::cv::Mat<KF_TYPE, KF_M, 1, KF_NPC>& y_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, 1, KF_NPC>& Xout_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, KF_N, KF_NPC>& Uout_mat,
                        xf::cv::Mat<KF_TYPE, KF_N, 1, KF_NPC>& Dout_mat,
                        unsigned char flag);

#endif //_XF_KALMANFILTER_CONFIG_H_
