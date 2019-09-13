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

#include "xf_headers.h"
#include "xf_kalmanfilter_config.h"

void error_check(cv::KalmanFilter kf, float *Xout_ptr, float *Uout_ptr, float *Dout_ptr, bool tu_or_mu, float *error_out)
{

	ap_uint<32> nan_xf = 0x7fc00000;

	cv::Mat Uout(KF_N, KF_N, CV_32FC1,  Uout_ptr);
	cv::Mat Dout = cv::Mat::zeros(KF_N, KF_N, CV_32FC1);
	for(int i=0; i<KF_N; i++)
		Dout.at<float>(i,i) = Dout_ptr[i];

	cv::Mat Pout(KF_N, KF_N, CV_32FC1);
	Pout = ( (Uout * Dout) * Uout.t() );

	float tu_max_error_P=-10000;
	float tu_max_error_P_reference;
	float tu_max_error_P_kernel;
	int tu_max_error_P_index[2];
	for(int i=0; i<KF_N;i++)
	{
		for(int j=0; j<KF_N;j++)
		{
			float kernel_output = Pout.at<float>(i,j);
			float refernce_output;
			if(tu_or_mu==0)
				refernce_output = (float)kf.errorCovPre.at<double>(i,j);
			else
				refernce_output = (float)kf.errorCovPost.at<double>(i,j);

			float error = fabs(kernel_output - refernce_output);

			ap_uint<32> error_int = *(int*)&error;

			if(error >  tu_max_error_P || error_int==nan_xf){
				tu_max_error_P = error;
				tu_max_error_P_reference = refernce_output;
				tu_max_error_P_kernel = kernel_output;
				tu_max_error_P_index[0]=i;
				tu_max_error_P_index[1]=j;
			}

		}
	}

	float tu_max_error_X=-10000;
	float tu_max_error_X_reference;
	float tu_max_error_X_kernel;
	int tu_max_error_X_index;
	for(int i=0; i<KF_N;i++)
	{
		float kernel_output = Xout_ptr[i];

		float refernce_output;
		if(tu_or_mu==0)
			refernce_output = (float)kf.statePre.at<double>(i);
		else
			refernce_output = (float)kf.statePost.at<double>(i);

		float error = fabs(kernel_output - refernce_output);

		ap_uint<32> error_int = *(int*)&error;

		if(error >  tu_max_error_X || error_int==nan_xf){
			tu_max_error_X = error;
			tu_max_error_X_reference = refernce_output;
			tu_max_error_X_kernel = kernel_output;
			tu_max_error_X_index=i;
		}

	}

	if(tu_max_error_X > tu_max_error_P)
		*error_out = tu_max_error_X;
	else
		*error_out = tu_max_error_P;
}

int main(int argc, char *argv[])
{
	//Control Flag Register
	int INIT_EN 		= 1;
	int TIMEUPDATE_EN 	= 2;
	int MEASUPDATE_EN 	= 4;
	int XOUT_EN_TU 		= 8;
	int UDOUT_EN_TU		= 16;
	int XOUT_EN_MU 		= 32;
	int UDOUT_EN_MU		= 64;

#if __SDSCC__
	//#if 1
	fprintf(stderr,"\n*** SDS memory for Arguments");
	float *A_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(float));
	float *B_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*KF_C*sizeof(float));
	float *Uq_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(float));
	float *Dq_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*sizeof(float));
	float *H_ptr 	= (float *)sds_alloc_non_cacheable(KF_M*KF_N*sizeof(float));
	float *X0_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*sizeof(float));
	float *U0_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(float));
	float *D0_ptr 	= (float *)sds_alloc_non_cacheable(KF_N*sizeof(float));
	float *R_ptr 	= (float *)sds_alloc_non_cacheable(KF_M*sizeof(float));
	float *u_ptr 	= (float *)sds_alloc_non_cacheable(KF_C*sizeof(float));
	float *y_ptr 	= (float *)sds_alloc_non_cacheable(KF_M*sizeof(float));
	float *Xout_ptr = (float *)sds_alloc_non_cacheable(KF_N*sizeof(float));
	float *Uout_ptr = (float *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(float));
	float *Dout_ptr = (float *)sds_alloc_non_cacheable(KF_N*sizeof(float));

	double *A_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(double));
	double *B_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*KF_C*sizeof(double));
	double *Q_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(double));
	double *Uq_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(double));
	double *Dq_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*sizeof(double));
	double *H_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_M*KF_N*sizeof(double));
	double *X0_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*sizeof(double));
	double *P0_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(double));
	double *U0_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*KF_N*sizeof(double));
	double *D0_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_N*sizeof(double));
	double *R_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_M*sizeof(double));
	double *u_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_C*sizeof(double));
	double *y_ptr_dp 	= (double *)sds_alloc_non_cacheable(KF_M*sizeof(double));


#else

	float *A_ptr 	= (float *)malloc(KF_N*KF_N*sizeof(float));
	float *B_ptr 	= (float *)malloc(KF_N*KF_C*sizeof(float));
	float *Q_ptr 	= (float *)malloc(KF_N*KF_N*sizeof(float));
	float *Uq_ptr 	= (float *)malloc(KF_N*KF_N*sizeof(float));
	float *Dq_ptr 	= (float *)malloc(KF_N*sizeof(float));
	float *H_ptr 	= (float *)malloc(KF_M*KF_N*sizeof(float));
	float *X0_ptr 	= (float *)malloc(KF_N*sizeof(float));
	float *P0_ptr 	= (float *)malloc(KF_N*KF_N*sizeof(float));
	float *U0_ptr 	= (float *)malloc(KF_N*KF_N*sizeof(float));
	float *D0_ptr 	= (float *)malloc(KF_N*sizeof(float));
	float *R_ptr 	= (float *)malloc(KF_M*sizeof(float));
	float *u_ptr 	= (float *)malloc(KF_C*sizeof(float));
	float *y_ptr 	= (float *)malloc(KF_M*sizeof(float));
	float *Xout_ptr = (float *)malloc(KF_N*sizeof(float));
	float *Uout_ptr = (float *)malloc(KF_N*KF_N*sizeof(float));
	float *Dout_ptr = (float *)malloc(KF_N*sizeof(float));

	double *A_ptr_dp 	= (double *)malloc(KF_N*KF_N*sizeof(double));
	double *B_ptr_dp 	= (double *)malloc(KF_N*KF_C*sizeof(double));
	double *Q_ptr_dp 	= (double *)malloc(KF_N*KF_N*sizeof(double));
	double *Uq_ptr_dp 	= (double *)malloc(KF_N*KF_N*sizeof(double));
	double *Dq_ptr_dp 	= (double *)malloc(KF_N*sizeof(double));
	double *H_ptr_dp 	= (double *)malloc(KF_M*KF_N*sizeof(double));
	double *X0_ptr_dp 	= (double *)malloc(KF_N*sizeof(double));
	double *P0_ptr_dp 	= (double *)malloc(KF_N*KF_N*sizeof(double));
	double *U0_ptr_dp 	= (double *)malloc(KF_N*KF_N*sizeof(double));
	double *D0_ptr_dp 	= (double *)malloc(KF_N*sizeof(double));
	double *R_ptr_dp 	= (double *)malloc(KF_M*sizeof(double));
	double *u_ptr_dp 	= (double *)malloc(KF_C*sizeof(double));
	double *y_ptr_dp 	= (double *)malloc(KF_M*sizeof(double));

#endif

	//Initialize Transition matrix A
	int Acnt=0;
	for(int i=0; i<KF_N;i++){
		for(int j=0; j<KF_N;j++){
			double val = ((double)rand()/(double)(RAND_MAX)) * 2.0;
			A_ptr_dp[Acnt] = val;
			A_ptr[Acnt++] = (float)val;
		}
	}

	//Initialize Control matrix B
	int Bcnt=0;
	for(int i=0; i<KF_N;i++){
		for(int j=0; j<KF_C;j++){
			double val = ((double)rand()/(double)(RAND_MAX)) * 1.0;
			B_ptr_dp[Bcnt] = val;
			B_ptr[Bcnt++] = (float)val;
		}
	}

	//Initialize Measurement matrix H
	int Hcnt=0;
	for(int i=0; i<KF_M;i++){
		for(int j=0; j<KF_N;j++){
			double val = ((double)rand()/(double)(RAND_MAX)) * 0.001;
			H_ptr_dp[Hcnt] = val;
			H_ptr[Hcnt++] = (float)val;
		}
	}

	//Initialize State matrix X0
	for(int i=0; i<KF_N;i++){
		double val = ((double)rand()/(double)(RAND_MAX)) * 5.0;
		X0_ptr_dp[i] = val;
		X0_ptr[i] = (float)val;
	}

	//Initialize Measurement noise covariance matrix R
	for(int i=0; i<KF_M;i++){
		double val = ((double)rand()/(double)(RAND_MAX)) * 0.01;
		R_ptr_dp[i] = val;
		R_ptr[i] = (float)val;
	}

	//Initialize U matrix for initial error estimate covariance matrix P
	for(int i=0; i<KF_N;i++)
	{
		for(int jn=(-i), j = 0; j<KF_N; jn++, j++)
		{
			int index = j + i*KF_N;
			if(jn<0)
			{
				U0_ptr_dp[index] = 0;
				U0_ptr[index] = 0;
			}else if(jn==0){
				U0_ptr_dp[index] = 1;
				U0_ptr[index] = 1;
			}
			else
			{
				double val = ((double)rand()/(double)(RAND_MAX)) * 1.0;
				U0_ptr_dp[index] = val;
				U0_ptr[index] = (float)val;
			}
		}
	}

	//Initialize D matrix for initial error estimate covariance matrix P.(only diagonal elements)
	for(int i=0; i<KF_N;i++)
	{
		double val = ((double)rand()/(double)(RAND_MAX)) * 1.0;
		D0_ptr_dp[i] = val;
		D0_ptr[i] = (float)val;
	}

	//Initialize U matrix for process noise covariance matrix Q.
	for(int i=0; i<KF_N;i++)
	{
		for(int jn=(-i), j = 0; j<KF_N; jn++, j++)
		{
			int index = j + i*KF_N;
			if(jn<0)
			{
				Uq_ptr_dp[index] = 0;
				Uq_ptr[index] = 0;
			}else if(jn==0){
				Uq_ptr_dp[index] = 1;
				Uq_ptr[index] = 1;
			}
			else
			{
				double val = ((double)rand()/(double)(RAND_MAX)) * 1.0;
				Uq_ptr_dp[index] = val;
				Uq_ptr[index] = (float)val;
			}
		}
	}

	//Initialize D matrix for process noise covariance matrix Q.(only diagonal elements)
	for(int i=0; i<KF_N;i++)
	{
		double val = ((double)rand()/(double)(RAND_MAX)) * 1.0;
		Dq_ptr_dp[i] = val;
		Dq_ptr[i] = (float)val;
	}

	cv::Mat A(KF_N, KF_N, CV_64FC1,  A_ptr_dp);
	cv::Mat B(KF_N, KF_C, CV_64FC1,  B_ptr_dp);

	cv::Mat Uq(KF_N, KF_N,CV_64FC1,  Uq_ptr_dp);
	cv::Mat Dq = cv::Mat::zeros(KF_N, KF_N, CV_64FC1);
	for(int i=0; i<KF_N; i++)
		Dq.at<double>(i,i) = Dq_ptr_dp[i];
	cv::Mat Q(KF_N, KF_N,CV_64FC1);
	Q = Uq * Dq * Uq.t();

	cv::Mat H(KF_M, KF_N, CV_64FC1,  H_ptr_dp);
	cv::Mat X0(KF_N, 1,   CV_64FC1);
	for(int i=0; i<KF_N; i++)
		X0.at<double>(i) = X0_ptr_dp[i];

	cv::Mat U0(KF_N, KF_N,CV_64FC1,  U0_ptr_dp);
	cv::Mat D0 = cv::Mat::zeros(KF_N, KF_N, CV_64FC1);
	for(int i=0; i<KF_N; i++)
		D0.at<double>(i,i) = D0_ptr_dp[i];
	cv::Mat P0(KF_N, KF_N,CV_64FC1);
	P0 = U0 * D0 * U0.t();

	cv::Mat R = cv::Mat::zeros(KF_M, KF_M, CV_64FC1);
	for(int i=0; i<KF_M; i++)
		R.at<double>(i,i) = R_ptr_dp[i];
	cv::Mat uk(KF_C, 1,    CV_64FC1);
	cv::Mat zk(KF_M ,1,    CV_64FC1);


	xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	A_mat;
#if KF_C!=0
	xf::Mat<KF_TYPE, KF_N, KF_C, KF_NPC> 	B_mat;
#endif	
	xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	Uq_mat;
	xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		Dq_mat;
	xf::Mat<KF_TYPE, KF_M, KF_N, KF_NPC> 	H_mat;
	xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		X0_mat;
	xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	U0_mat;
	xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		D0_mat;
	xf::Mat<KF_TYPE, KF_M, 1, KF_NPC> 		R_mat;
#if KF_C!=0	
	xf::Mat<KF_TYPE, KF_C, 1, KF_NPC> 		u_mat;
#endif
	xf::Mat<KF_TYPE, KF_M, 1, KF_NPC> 		y_mat;
	xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		Xout_mat;
	xf::Mat<KF_TYPE, KF_N, KF_N, KF_NPC> 	Uout_mat;
	xf::Mat<KF_TYPE, KF_N, 1, KF_NPC> 		Dout_mat;

	//Copy data to	A_mat
	for(int index=0; index < KF_N*KF_N; index++)
	{
		A_mat.write_float(index,A_ptr[index]);
	}
#if KF_C!=0
	//Copy data to	B_mat
	for(int index=0; index < KF_N*KF_C; index++)
	{
		B_mat.write_float(index,B_ptr[index]);
	}
#endif
	//Copy data to	Uq_mat
	for(int index=0; index < KF_N*KF_N; index++)
	{
		Uq_mat.write_float(index,Uq_ptr[index]);
	}

	//Copy data to	Dq_mat
	for(int index=0; index < KF_N; index++)
	{
		Dq_mat.write_float(index,Dq_ptr[index]);
	}

	//Copy data to	H_mat
	for(int index=0; index < KF_M*KF_N; index++)
	{
		H_mat.write_float(index,H_ptr[index]);
	}

	//Copy data to	X0_mat
	for(int index=0; index < KF_N; index++)
	{
		X0_mat.write_float(index,X0_ptr[index]);
	}

	//Copy data to	U0_mat
	for(int index=0; index < KF_N*KF_N; index++)
	{
		U0_mat.write_float(index,U0_ptr[index]);
	}

	//Copy data to	D0_mat
	for(int index=0; index < KF_N; index++)
	{
		D0_mat.write_float(index,D0_ptr[index]);
	}

	//Copy data to	R_mat
	for(int index=0; index < KF_M; index++)
	{
		R_mat.write_float(index,R_ptr[index]);
	}

	fprintf(stderr,"\n Kalman Filter Verification");
	fprintf(stderr,"\n Number of state variables: %d", KF_N);
	fprintf(stderr,"\n Number of measurements: %d", KF_M);
	fprintf(stderr,"\n Number of control input: %d\n", KF_C);

	//Initialization: OpenCv Kalman Filter in Double Precision
	cv::KalmanFilter kf(KF_N, KF_M, KF_C, CV_64F);
	kf.statePost = X0;
	kf.errorCovPost = P0;
	kf.transitionMatrix = A;
	kf.processNoiseCov = Q;
	kf.measurementMatrix = H;
	kf.measurementNoiseCov =R;
	kf.controlMatrix = B;

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

	//Initialization: Xilinx Kalman filter in Single Precision
	kalmanfilter_accel(A_mat, 
#if KF_C!=0	
	B_mat, 
#endif	
	Uq_mat, Dq_mat,  H_mat, X0_mat, U0_mat, D0_mat, R_mat, 
#if KF_C!=0	
	u_mat, 
#endif	
	y_mat, Xout_mat, Uout_mat, Dout_mat, INIT_EN);

#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	fprintf(stderr,"\n Hardware Latency for Initialization = %d cycles",(int)hw_cycles);
#endif

	//Initialize control input
	for(int i=0; i<KF_C;i++){
		double val = ((double)rand()/(double)(RAND_MAX)) * 10.0;
		u_ptr[i] = (float)val;
		uk.at<double>(i) = val;
	}
#if KF_C!=0	
	//Copy data to	u_mat
	for(int index=0; index < KF_C; index++)
	{
		u_mat.write_float(index,u_ptr[index]);
	}
#endif

	//Time Update:OpenCv Kalman Filter in Double Precision
	kf.predict(uk);

#if __SDSCC__
	hw_ctr.start();
#endif

	//Time Update: Xilinx Kalman filter in Single Precision
	kalmanfilter_accel(A_mat, 
#if KF_C!=0	
	B_mat, 
#endif	
	Uq_mat, Dq_mat,  H_mat, X0_mat, U0_mat, D0_mat, R_mat, 
#if KF_C!=0	
	u_mat, 
#endif	
	y_mat, Xout_mat, Uout_mat, Dout_mat, TIMEUPDATE_EN + XOUT_EN_TU + UDOUT_EN_TU);

#if __SDSCC__
	hw_ctr.stop();
	hw_cycles = hw_ctr.avg_cpu_cycles();
	fprintf(stderr,"\n Hardware Latency for Time Update = %d cycles",(int)hw_cycles);
#endif

	//Initialize measurements
	for(int i=0; i<KF_M;i++){
		double val = ((double)rand()/(double)(RAND_MAX)) * 5.0;
		y_ptr[i] = (float)val;
		zk.at<double>(i) = val;
	}
	//Copy data to	y_mat
	for(int index=0; index < KF_M; index++)
	{
		y_mat.write_float(index,y_ptr[index]);
	}

	//Measurement Update: OpenCv Kalman Filter in Double Precision
	kf.correct(zk);

#if __SDSCC__
	hw_ctr.start();
#endif

	//Measurement Update: Xilinx Kalman filter in Single Precision
	kalmanfilter_accel(A_mat, 
#if KF_C!=0	
	B_mat, 
#endif	
	Uq_mat, Dq_mat,  H_mat, X0_mat, U0_mat, D0_mat, R_mat, 
#if KF_C!=0	
	u_mat, 
#endif	
	y_mat, Xout_mat, Uout_mat, Dout_mat, MEASUPDATE_EN + XOUT_EN_MU + UDOUT_EN_MU);

#if __SDSCC__
	hw_ctr.stop();
	hw_cycles = hw_ctr.avg_cpu_cycles();
	fprintf(stderr,"\n Hardware Latency for Measurement Update = %d cycles",(int)hw_cycles);
#endif

	//Copy from	Xout_mat
	for(int index=0; index < KF_N; index++)
	{
		Xout_ptr[index] = Xout_mat.read_float(index);
	}

	//Copy from	Uout_mat
	for(int index=0; index < KF_N*KF_N; index++)
	{
		Uout_ptr[index] = Uout_mat.read_float(index);
	}
	//Copy from	Dout_mat
	for(int index=0; index < KF_N; index++)
	{
		Dout_ptr[index] = Dout_mat.read_float(index);
	}

	float error;
	error_check(kf, Xout_ptr, Uout_ptr, Dout_ptr, 1, &error);

	if(error < 0.001f)
	{
		fprintf(stderr,"\n********** Test Pass\n");
		return 0;
	}
	else
	{
		fprintf(stderr,"\n********** Test Fail\n");
		return -1;
	}

}
