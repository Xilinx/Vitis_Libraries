/***************************************************************************
Copyright (c) 2018, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#include "xf_kalmanfilter_config.h"

extern "C" {

void kalmanfilter_accel(ap_uint<32>* in_A,
                        ap_uint<32>* in_B,
                        ap_uint<32>* in_Uq,
                        ap_uint<32>* in_Dq,
                        ap_uint<32>* in_H,
                        ap_uint<32>* in_X0,
                        ap_uint<32>* in_U0,
                        ap_uint<32>* in_D0,
                        ap_uint<32>* in_R,
                        ap_uint<32>* in_u,
                        ap_uint<32>* in_y,
                        unsigned char control_flag,
                        ap_uint<32>* out_X,
                        ap_uint<32>* out_U,
                        ap_uint<32>* out_D) {
    // clang-format off
    #pragma HLS INTERFACE m_axi      port=in_A      offset=slave  bundle=gmem0
    #pragma HLS INTERFACE m_axi      port=in_B      offset=slave  bundle=gmem1
    #pragma HLS INTERFACE m_axi      port=in_Uq     offset=slave  bundle=gmem2
    #pragma HLS INTERFACE m_axi      port=in_Dq     offset=slave  bundle=gmem3
    #pragma HLS INTERFACE m_axi      port=in_H      offset=slave  bundle=gmem4
    #pragma HLS INTERFACE m_axi      port=in_X0     offset=slave  bundle=gmem5
    #pragma HLS INTERFACE m_axi      port=in_U0     offset=slave  bundle=gmem6
    #pragma HLS INTERFACE m_axi      port=in_D0     offset=slave  bundle=gmem7
    #pragma HLS INTERFACE m_axi      port=in_R      offset=slave  bundle=gmem8
    #pragma HLS INTERFACE m_axi      port=in_u      offset=slave  bundle=gmem9
    #pragma HLS INTERFACE m_axi      port=in_y      offset=slave  bundle=gmem10
    #pragma HLS INTERFACE m_axi      port=out_X     offset=slave  bundle=gmem11
    #pragma HLS INTERFACE m_axi      port=out_U     offset=slave  bundle=gmem12
    #pragma HLS INTERFACE m_axi      port=out_D     offset=slave  bundle=gmem13
    #pragma HLS INTERFACE s_axilite  port=control_flag 	          bundle=control
    #pragma HLS INTERFACE s_axilite  port=return 		          bundle=control
    // clang-format on

    xf::cv::Mat<TYPE, KF_N, KF_N, NPC1> A_mat;
    xf::cv::Mat<TYPE, KF_N, KF_C, NPC1> B_mat;
    xf::cv::Mat<TYPE, KF_N, KF_N, NPC1> Uq_mat;
    xf::cv::Mat<TYPE, KF_N, 1, NPC1> Dq_mat;
    xf::cv::Mat<TYPE, KF_M, KF_N, NPC1> H_mat;
    xf::cv::Mat<TYPE, KF_N, 1, NPC1> X0_mat;
    xf::cv::Mat<TYPE, KF_N, KF_N, NPC1> U0_mat;
    xf::cv::Mat<TYPE, KF_N, 1, NPC1> D0_mat;
    xf::cv::Mat<TYPE, KF_M, 1, NPC1> R_mat;
    xf::cv::Mat<TYPE, KF_C, 1, NPC1> u_mat;
    xf::cv::Mat<TYPE, KF_M, 1, NPC1> y_mat;

    xf::cv::Mat<TYPE, KF_N, 1, NPC1> Xout_mat;
    xf::cv::Mat<TYPE, KF_N, KF_N, NPC1> Uout_mat;
    xf::cv::Mat<TYPE, KF_N, 1, NPC1> Dout_mat;

    // clang-format off
    #pragma HLS STREAM variable=A_mat.data depth=2
    #pragma HLS STREAM variable=B_mat.data depth=2
    #pragma HLS STREAM variable=Uq_mat.data depth=2
    #pragma HLS STREAM variable=Dq_mat.data depth=2
    #pragma HLS STREAM variable=H_mat.data depth=2
    #pragma HLS STREAM variable=X0_mat.data depth=2
    #pragma HLS STREAM variable=U0_mat.data depth=2
    #pragma HLS STREAM variable=D0_mat.data depth=2
    #pragma HLS STREAM variable=R_mat.data depth=2
    #pragma HLS STREAM variable=u_mat.data depth=2
    #pragma HLS STREAM variable=y_mat.data depth=2
    #pragma HLS STREAM variable=Xout_mat.data depth=2
    #pragma HLS STREAM variable=Uout_mat.data depth=2
    #pragma HLS STREAM variable=Dout_mat.data depth=2
    // clang-format on

    // clang-format off
    #pragma HLS DATAFLOW
    // clang-format on

    // Retrieve xf::cv::Mat objects from img_in data:
    xf::cv::Array2xfMat<32, TYPE, KF_N, KF_N, NPC1>(in_A, A_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_N, KF_N, NPC1>(in_Uq, Uq_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_N, KF_N, NPC1>(in_U0, U0_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_M, KF_N, NPC1>(in_H, H_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_N, KF_C, NPC1>(in_B, B_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_N, 1, NPC1>(in_Dq, Dq_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_N, 1, NPC1>(in_X0, X0_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_N, 1, NPC1>(in_D0, D0_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_M, 1, NPC1>(in_R, R_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_M, 1, NPC1>(in_y, y_mat);
    xf::cv::Array2xfMat<32, TYPE, KF_C, 1, NPC1>(in_u, u_mat);

    // Run xfOpenCV kernel:
    xf::cv::KalmanFilter<KF_N, KF_M, KF_C, KF_MTU, KF_MMU, XF_USE_URAM, 0, TYPE, NPC1>(
        A_mat, B_mat, Uq_mat, Dq_mat, H_mat, X0_mat, U0_mat, D0_mat, R_mat, u_mat, y_mat, Xout_mat, Uout_mat, Dout_mat,
        control_flag);

    xf::cv::xfMat2Array<32, TYPE, KF_N, KF_N, NPC1>(Uout_mat, out_U);
    xf::cv::xfMat2Array<32, TYPE, KF_N, 1, NPC1>(Dout_mat, out_D);
    xf::cv::xfMat2Array<32, TYPE, KF_N, 1, NPC1>(Xout_mat, out_X);

    return;
} // End of kernel

} // End of extern C