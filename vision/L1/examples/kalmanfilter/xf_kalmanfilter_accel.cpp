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

#include "xf_kalmanfilter_config.h"

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
                        unsigned char flag)

{
    xf::cv::KalmanFilter<KF_N, KF_M, KF_C, KF_MTU, KF_MMU, XF_USE_URAM, KF_EKF, KF_TYPE, KF_NPC>(
        A_mat,
#if KF_C != 0
        B_mat,
#endif
        Uq_mat, Dq_mat, H_mat, X0_mat, U0_mat, D0_mat, R_mat,
#if KF_C != 0
        u_mat,
#endif
        y_mat, Xout_mat, Uout_mat, Dout_mat, flag);
}
