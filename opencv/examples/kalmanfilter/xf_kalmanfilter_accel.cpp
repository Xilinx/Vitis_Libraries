/***************************************************************************
Copyright (c) 2019, Xilinx, Inc.
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
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/

#include "xf_kalmanfilter_config.h"

void kalmanfilter_accel(
		xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	&A_mat,
#if KF_C!=0
		xf::Mat<KF_TYPE, KF_N, KF_C, KF_NPC> 	&B_mat,
#endif
		xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	&Uq_mat,
		xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		&Dq_mat,
		xf::Mat<KF_TYPE, KF_M, KF_N, KF_NPC> 	&H_mat,
		xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		&X0_mat,
		xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	&U0_mat,
		xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		&D0_mat,
		xf::Mat<KF_TYPE, KF_M, 1, KF_NPC> 		&R_mat,
#if KF_C!=0
		xf::Mat<KF_TYPE, KF_C, 1, KF_NPC> 		&u_mat,
#endif		
		xf::Mat<KF_TYPE, KF_M, 1, KF_NPC> 		&y_mat,
		xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		&Xout_mat,
		xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	&Uout_mat,
		xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		&Dout_mat,
		unsigned char flag)

{
	xf::KalmanFilter<KF_N, KF_M, KF_C, KF_MTU, KF_MMU, XF_USE_URAM, KF_EKF, KF_TYPE, KF_NPC>
	(A_mat, 
#if KF_C!=0	
	B_mat, 
#endif
	Uq_mat, Dq_mat,  H_mat,X0_mat, U0_mat, D0_mat, R_mat, 
#if KF_C!=0	
	u_mat, 
#endif	
	y_mat, Xout_mat, Uout_mat, Dout_mat, flag);
}

