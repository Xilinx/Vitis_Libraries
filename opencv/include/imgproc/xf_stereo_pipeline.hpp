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

#ifndef _XF_STEREO_PIPELINE_HPP_
#define _XF_STEREO_PIPELINE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "utils/x_hls_traits.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#define _XF_INTER_BITS_ 5

namespace xf {

template<typename T>
struct xfInitUndistortRectifyMap_traits {
    typedef T FRAMET;
    typedef T FRAME2T;
};

// Approximation of 1/x around x=1.
// In most cases (e.g. float), we just do computation in type T.
template<typename T>
T xf_one_over_x_approx(T x) {
    return T(1.0)/x;
}

// if x is ~ 1+delta, then 1/x is 1-delta+..., or 2-x;
template <int W, int I, ap_q_mode Q, ap_o_mode O>
ap_fixed<W,I,Q,O> xf_one_over_x_approx(ap_fixed<W,I,Q,O> x) {
    return 2-x;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
ap_ufixed<W,I,Q,O> xf_one_over_x_approx(ap_ufixed<W,I,Q,O> x) {
    return 2-x;
}

// Approximation of 1/(1+x) around x=0.
// In most cases (e.g. float), we just do computation in type T.
template<typename T>
T xf_one_over_one_plus_x_approx(T x) {
    return T(1.0)/(T(1.0)+x);
}

// if x is ~ 1+delta, then 1/x is 1-delta+delta^2-...
template <int W, int I, ap_q_mode Q, ap_o_mode O>
ap_fixed<W,I,Q,O> xf_one_over_one_plus_x_approx(ap_fixed<W,I,Q,O> x) {
    return 1-x;
}

template <int W, int I, ap_q_mode Q, ap_o_mode O>
ap_ufixed<W,I,Q,O> xf_one_over_one_plus_x_approx(ap_ufixed<W,I,Q,O> x) {
    return 1-x;
}

template <typename FRAMET, typename FRAME2T, typename ROWT, typename COLT, typename ROWOUTT, typename COLOUTT, typename CM_T, int CM_SIZE, int N>
void xFComputeUndistortCoordinates(
		CM_T *cameraMatrix,
		CM_T *distCoeffs,
		CM_T *ir,
		int noRotation,
		ROWT i, COLT j,
		ROWOUTT &u, COLOUTT &v) {
	CM_T zo = 0;
	CM_T k1 = distCoeffs[0];
	CM_T k2 = distCoeffs[1];
	CM_T p1 = distCoeffs[2];
	CM_T p2 = distCoeffs[3];
	CM_T k3 = N>=5? distCoeffs[4] : zo;
	CM_T k4 = N>=8? distCoeffs[5] : zo;
	CM_T k5 = N>=8? distCoeffs[6] : zo;
	CM_T k6 = N>=8? distCoeffs[7] : zo;

	CM_T u0 = cameraMatrix[2];
	CM_T v0 = cameraMatrix[5];
	CM_T fx = cameraMatrix[0];
	CM_T fy = cameraMatrix[4];

	// FRAMET is the type of normalized coordinates.
	// If IR is float, then FRAMET will also be float
	// If IR is ap_fixed, then FRAMET will be some ap_fixed type with more integer bits
	//             typedef typename x_traits<typename x_traits<ROWT,ICMT>::MULT_T,
	//                                       typename x_traits<COLT,ICMT>::MULT_T >::ADD_T FRAMET;
	//    typedef ap_fixed<18,2, AP_TRN, AP_SAT> FRAMET; // Assume frame coordinates are in [-1,1)
	//    typedef CMT FRAMET;
	//typedef float FRAMET;

	FRAMET _x, _y, x, y;
	_x=i*ir[1] + j * ir[0] + ir[2];
	_y=i*ir[4] + j * ir[3] + ir[5];

	if(noRotation) {
		// A special case if there is no rotation: equivalent to cv::initUndistortMap
		x=_x;
		y=_y;
	} else {
		FRAMET w=i*ir[7] + j * ir[6] + ir[8];
		FRAMET winv = xf_one_over_x_approx(w);
		x = (FRAMET)(_x*winv);
		y = (FRAMET)(_y*winv);
	}

	typename hls::x_traits<FRAMET,FRAMET>::MULT_T x2t = x*x, y2t = y*y;  // Full precision result here.
	FRAME2T _2xy = 2*x*y;
	FRAME2T r2 = x2t + y2t;
	FRAME2T x2 = x2t, y2 = y2t;

	FRAMET kr = (1 + FRAMET(FRAMET(k3*r2 + k2)*r2 + k1)*r2);
	FRAME2T krd = FRAMET(FRAMET(k6*r2 + k5)*r2 + k4)*r2;

	if(N >5) kr = kr*xf_one_over_one_plus_x_approx(krd);

	u = fx*(FRAMET(x*kr) + FRAMET(p1*_2xy) + FRAMET(p2*(2*x2 + r2))) + u0;
	v = fy*(FRAMET(y*kr) + FRAMET(p1*(r2 + 2*y2)) + FRAMET(p2*_2xy)) + v0;
}

template< int ROWS, int COLS, int CM_SIZE, typename CM_T, int N, int MAP_T, int NPC, typename MAP_type >
void xFInitUndistortRectifyMapInverseKernel (
		CM_T *cameraMatrix,
		CM_T *distCoeffs,
		CM_T *ir,
		xf::Mat<MAP_T, ROWS, COLS, NPC> &map1,
		xf::Mat<MAP_T, ROWS, COLS, NPC> &map2,
		uint16_t rows, uint16_t cols,
		int noRotation=false)
{
#pragma HLS INLINE OFF

	CM_T cameraMatrixHLS[CM_SIZE];
	CM_T distCoeffsHLS[N];
	CM_T iRnewCameraMatrixHLS[CM_SIZE];
#pragma HLS ARRAY_PARTITION variable=cameraMatrixHLS complete dim=0
#pragma HLS ARRAY_PARTITION variable=distCoeffsHLS complete dim=0
#pragma HLS ARRAY_PARTITION variable=iRnewCameraMatrixHLS complete dim=0

	for (int i=0; i<CM_SIZE; i++)
	{
#pragma HLS PIPELINE II=1
		cameraMatrixHLS[i] = cameraMatrix[i];
		iRnewCameraMatrixHLS[i] = ir[i];
	}
	for (int i=0; i<N; i++)
	{
#pragma HLS PIPELINE II=1
		distCoeffsHLS[i] = distCoeffs[i];
	}

	MAP_type mx;
	MAP_type my;

	assert(rows <= ROWS);
	assert(cols <= COLS);

	int idx = 0;
	loop_height: for(int i=0; i< rows; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		loop_width: for(int j=0; j< cols; j++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
#pragma HLS PIPELINE II=1
			typedef ap_uint<BitWidth<ROWS>::Value> ROWT;
			typedef ap_uint<BitWidth<COLS>::Value> COLT;
			ROWT ifixed = i;
			COLT jfixed = j;

			ap_fixed<1+BitWidth<COLS>::Value+_XF_INTER_BITS_, 1+BitWidth<COLS>::Value, AP_RND, AP_SAT> u;
			ap_fixed<1+BitWidth<ROWS>::Value+_XF_INTER_BITS_, 1+BitWidth<ROWS>::Value, AP_RND, AP_SAT> v;
			xFComputeUndistortCoordinates
			<typename xfInitUndistortRectifyMap_traits<CM_T>::FRAMET, typename xfInitUndistortRectifyMap_traits<CM_T>::FRAME2T,
			ROWT,COLT,ap_fixed<1+BitWidth<COLS>::Value+_XF_INTER_BITS_, 1+BitWidth<COLS>::Value, AP_RND, AP_SAT>,ap_fixed<1+BitWidth<ROWS>::Value+_XF_INTER_BITS_, 1+BitWidth<ROWS>::Value, AP_RND, AP_SAT>,
			CM_T,CM_SIZE,N>
			(cameraMatrixHLS,distCoeffsHLS,iRnewCameraMatrixHLS,noRotation,ifixed,jfixed,u,v);

			float mx = (float)u;
			float my = (float)v;

			map1.write_float(idx,mx);
			map2.write_float(idx++,my);
		}
	}
}


//#pragma SDS data data_mover("_mapx_mat.data":AXIDMA_SIMPLE,"_mapy_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_mapx_mat.data":SEQUENTIAL,"_mapy_mat.data":SEQUENTIAL)
#pragma SDS data copy("_mapx_mat.data"[0:"_mapx_mat.size"])
#pragma SDS data copy("_mapy_mat.data"[0:"_mapy_mat.size"])
#pragma SDS data zero_copy(cameraMatrix[0:_cm_size])
#pragma SDS data zero_copy(distCoeffs[0:_dc_size])
#pragma SDS data zero_copy(ir[0:_cm_size])
template< int CM_SIZE, int DC_SIZE, int MAP_T, int ROWS, int COLS, int NPC >
void InitUndistortRectifyMapInverse (
		ap_fixed<32,12> *cameraMatrix,
		ap_fixed<32,12> *distCoeffs,
		ap_fixed<32,12> *ir,
		xf::Mat<MAP_T, ROWS, COLS, NPC> &_mapx_mat,
		xf::Mat<MAP_T, ROWS, COLS, NPC> &_mapy_mat,
		int _cm_size, int _dc_size)
{
#pragma HLS INLINE OFF

	xFInitUndistortRectifyMapInverseKernel<ROWS,COLS,CM_SIZE,ap_fixed<32,12>,DC_SIZE,MAP_T,NPC,XF_TNAME(MAP_T,NPC)>(cameraMatrix, distCoeffs, ir, _mapx_mat, _mapy_mat, _mapx_mat.rows, _mapx_mat.cols);
}
}
#endif  // _XF_STEREO_PIPELINE_HPP_



