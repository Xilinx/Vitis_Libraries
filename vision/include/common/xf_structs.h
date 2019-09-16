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

#ifndef _XF_STRUCTS_H_
#define _XF_STRUCTS_H_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif

#ifndef __SYNTHESIS__
#include <iostream>
#endif
#include <math.h>
#include <stdio.h>
#include <assert.h>
#include "xf_types.h"
#include "hls_stream.h"

#if __SDSCC__
#include "sds_lib.h"
#endif



namespace xf {

template<typename T>
T float2ap_uint(float val) {
	T *val_out = (T *)(&val);
	return *val_out;
}

template<typename T>
float ap_uint2float(T val) {
	float *val_out = (float *)(&val);
	return *val_out;
}

//----------------------------------------------------------------------------------------------------//
//  LOCAL STEREO BLOCK MATCHING UTILITY
//----------------------------------------------------------------------------------------------------//
template<int WSIZE, int NDISP, int NDISP_UNIT>
class xFSBMState {
  public:
    // pre-filtering (normalization of input images)
    int preFilterType;       // =HLS_STEREO_BM_XSOBEL_TEST
    int preFilterSize;       // averaging window size: ~5x5..21x21
    int preFilterCap;        // the output of pre-filtering is clipped by [-preFilterCap,preFilterCap]

    // correspondence using Sum of Absolute Difference (SAD)
    int SADWindowSize;       // ~5x5..21x21 // defined in macro
    int minDisparity;        // minimum disparity (can be negative)
    int numberOfDisparities; // maximum disparity - minimum disparity (> 0)

    // post-filtering
    int textureThreshold;    // the disparity is only computed for pixels

    // with textured enough neighborhood
    int uniquenessRatio;     // accept the computed disparity d* only if
                             // SAD(d) >= SAD(d*)*(1 + uniquenessRatio/100.)
                             // for any d != d*+/-1 within the search range.

    //int speckleWindowSize; // disparity variation window
    //int speckleRange;      // acceptable range of variation in window

    int ndisp_unit;
    int sweepFactor;
    int remainder;

    xFSBMState() {
        preFilterType = XF_STEREO_PREFILTER_SOBEL_TYPE; // Default Sobel filter
        preFilterSize = WSIZE;
        preFilterCap = 31;
        SADWindowSize = WSIZE;
        minDisparity = 0;
        numberOfDisparities = NDISP;
        textureThreshold = 10;
        uniquenessRatio = 15;
        sweepFactor = (NDISP / NDISP_UNIT) + ((NDISP % NDISP_UNIT) != 0);
        ndisp_unit = NDISP_UNIT;
        remainder = NDISP_UNIT * sweepFactor - NDISP;
    }
};
//----------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------//
// Template class of Point_
//----------------------------------------------------------------------------------------------------//
template<typename T>
class Point_ {
  public:
    Point_();
    Point_(T _x, T _y);
    Point_(const Point_& pt);
    ~Point_();

    T x, y;
};

/* Member functions of Point_ class */
template<typename T> inline Point_<T>::Point_() {
}
template<typename T> inline Point_<T>::Point_(T _x, T _y) : x(_x), y(_y) {
}
template<typename T> inline Point_<T>::Point_(const Point_<T>& pt) : x(pt.x), y(pt.y) {
}
template<typename T> inline Point_<T>::~Point_() {
}

typedef Point_<int> Point;
//----------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------//
// Template class of Size_
//----------------------------------------------------------------------------------------------------//
template<typename T>
class Size_ {
  public:
    Size_();
    Size_(T _width, T _height);
    Size_(const Size_<T>& sz);
    Size_(const Point_<T>& pt);
    T area();
    ~Size_();

    T width, height;
};

/* Member functions of Size_ class */
template<typename T> inline Size_<T>::Size_() {
}
template<typename T> inline Size_<T>::Size_(T _width, T _height) : width(_width), height(_height) {
}
template<typename T> inline Size_<T>::Size_(const Size_<T>& sz) : width(sz.width), height(sz.height) {
}
template<typename T> inline Size_<T>::Size_(const Point_<T>& pt) : width(pt.x), height(pt.y) {
}
template<typename T> inline T Size_<T>::area() {
    return width * height;
}
template<typename T> inline Size_<T>::~Size_() {
}

typedef Size_<int> Size;
//----------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------//
// Template class of Rect_
//----------------------------------------------------------------------------------------------------//
template<typename T>
class Rect_ {
  public:
    Rect_();
    Rect_(T _x, T _y, T _width, T _height);
    Rect_(const Rect_& rect);
    Rect_(const Point_<T>& pt, const Size_<T>& sz);
    T area();
    Size_<T> size();
    Point_<T> tl(); // top-left point(inside);
    Point_<T> tr(); // top-right point(outside);
    Point_<T> bl(); // bottom-left point(outside);
    Point_<T> br(); // bottom-right point(outside);
    bool bContains(const Point_<T>& pt);
    ~Rect_();

    T x, y, width, height;
};

/* Member functions of Rect_ class */
template<typename T> inline Rect_<T>::Rect_() {
}
template<typename T> inline Rect_<T>::Rect_(T _x, T _y, T _width, T _height) : x(_x), y(_y), width(_width), height(_height) {
}
template<typename T> inline Rect_<T>::Rect_(const Rect_<T>& rect) : x(rect.x), y(rect.y), width(rect.width), height(rect.height) {
}
template<typename T> inline Rect_<T>::Rect_(const Point_<T>& pt, const Size_<T>& sz) : x(pt.x), y(pt.y), width(sz.width), height(sz.height) {
}
template<typename T> inline T Rect_<T>::area() {
    return width * height;
}
template<typename T> inline Size_<T> Rect_<T>::size() {
    return Size_<T>(width, height);
}
template<typename T> inline Point_<T> Rect_<T>::tl() {
    return Point_<T>(x, y);
}
template<typename T> inline Point_<T> Rect_<T>::tr() {
    return Point_<T>(x + width, y);
}
template<typename T> inline Point_<T> Rect_<T>::bl() {
    return Point_<T>(x, y + height);
}
template<typename T> inline Point_<T> Rect_<T>::br() {
    return Point_<T>(x + width, y + height);
}
template<typename T> inline bool Rect_<T>::bContains(const Point_<T>& pt) {
    return (pt.x >= x && pt.x < x + width && pt.y >= y && pt.y < y + height);
}
template<typename T> inline Rect_<T>::~Rect_() {
}

typedef Rect_<int> Rect;
//----------------------------------------------------------------------------------------------------//

//----------------------------------------------------------------------------------------------------//
// Template class of Scalar
//----------------------------------------------------------------------------------------------------//
template<int N, typename T>
class Scalar {
public:
    Scalar() {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
        assert(N > 0);
    }
    Scalar(T v0) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
        assert(N >= 1 && "Scalar must have enough channels for constructor.");
        val[0] = v0;
    }
    Scalar(T v0, T v1) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
        assert(N >= 2 && "Scalar must have enough channels for constructor.");
        val[0] = v0;
        val[1] = v1;
    }
    Scalar(T v0, T v1, T v2) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
        assert(N >= 3 && "Scalar must have enough channels for constructor.");
        val[0] = v0;
        val[1] = v1;
        val[2] = v2;
    }
    Scalar(T v0, T v1, T v2, T v3) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
        assert(N >= 4 && "Scalar must have enough channels for constructor.");
        val[0] = v0;
        val[1] = v1;
        val[2] = v2;
        val[3] = v3;
    }

    void operator =(T value);
    Scalar<N, T> operator +(T value);
    Scalar<N, T> operator +(Scalar<N, T> s);
    Scalar<N, T> operator -(T value);
    Scalar<N, T> operator -(Scalar<N, T> s);
    Scalar<N, T> operator *(T value);
    Scalar<N, T> operator *(Scalar<N, T> s);
    Scalar<N, T> operator /(T value);
    Scalar<N, T> operator /(Scalar<N, T> s);

    T val[N];
};

template<int N, typename T>
void Scalar<N, T>::operator =(T value) {
#pragma HLS inline
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        val[k] = value;
    }
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator +(T value) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] + value;
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator +(Scalar<N, T> s) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] + s.val[k];
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator -(T value) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] - value;
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator -(Scalar<N, T> s) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] - s.val[k];
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator *(T value) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] * value;
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator *(Scalar<N, T> s) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] * s.val[k];
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator /(T value) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] / value;
    }
    return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator /(Scalar<N, T> s) {
#pragma HLS inline
    Scalar<N, T> res;
    for (int k = 0; k < N; k++) {
#pragma HLS unroll
        res.val[k] = val[k] / s.val[k];
    }
    return res;
}
//----------------------------------------------------------------------------------------------------//



//----------------------------------------------------------------------------------------------------//
// Template class of Mat
//----------------------------------------------------------------------------------------------------//
template<int T, int ROWS, int COLS, int NPC>
class Mat {

  public:
    unsigned char allocatedFlag;            // flag to mark memory allocation in this class
    int rows, cols, size;                   // actual image size

#ifdef __SDSVHLS__
    typedef XF_TNAME(T,NPC) DATATYPE;
#else                                       // When not being built for V-HLS
    typedef struct {
        XF_CTUNAME(T,NPC) chnl[XF_NPIXPERCYCLE(NPC)][XF_CHANNELS(T,NPC)];
    } __attribute__ ((packed)) DATATYPE;
#endif

//#if (defined  (__SDSCC__) ) || (defined (__SYNTHESIS__))
#if defined (__SYNTHESIS__) && !defined (__SDA_MEM_MAP__)
    DATATYPE *data __attribute((xcl_array_geometry((ROWS)*(COLS>> (XF_BITSHIFT(NPC))))));//data[ ROWS * ( COLS >> ( XF_BITSHIFT ( NPC ) ) ) ];
#else
    DATATYPE *data;
#endif


    Mat();                                  // default constructor
    Mat(Size _sz);
    Mat(int _rows, int _cols);
    Mat(int _size, int _rows, int _cols);
    Mat(int _rows, int _cols, void *_data);
    Mat(const Mat&);                        // copy constructor

    ~Mat();

    Mat& operator= (const Mat&);            // Assignment operator
//  XF_TNAME(T, XF_NPPC1) operator() (unsigned int r, unsigned int c);
//  XF_CTUNAME(T, NPC) operator() (unsigned int r, unsigned int c, unsigned int ch);
    XF_TNAME(T,NPC) read(int index);
    float read_float(int index);
    void write(int index, XF_TNAME(T,NPC) val);
    void write_float(int index, float val);

    void init (int _rows, int _cols, bool allocate=true);
    void copyTo (void* fromData);
    unsigned char* copyFrom ();

    const int type() const;
    const int depth() const;
    const int channels() const;

    template<int DST_T>
    void convertTo (Mat<DST_T, ROWS, COLS, NPC> &dst, int otype, double alpha=1, double beta=0);
};

template<int T, int ROWS, int COLS, int NPC>
const int Mat<T,ROWS, COLS, NPC>::type() const {
#pragma HLS inline
    return (T);
}

template< int T, int ROWS, int COLS, int NPC>
const int Mat<T, ROWS, COLS, NPC>::depth() const {
#pragma HLS inline
    return XF_DTPIXELDEPTH(T,NPC);
}
template< int T, int ROWS, int COLS, int NPC>
const int Mat< T, ROWS, COLS,NPC>::channels() const {
#pragma HLS inline
    return XF_CHANNELS(T,NPC);
}

template<int T, int ROWS, int COLS, int NPPC>
inline void Mat<T, ROWS, COLS, NPPC>::init(int _rows, int _cols, bool allocate) {
#pragma HLS inline

    assert((_rows > 0) && (_rows <= ROWS) && (_cols > 0) && (_cols <= COLS)
            && "The number of rows and columns must be less than the template arguments.");

    rows = _rows;
    cols = _cols;
    size = _rows * (_cols >> (XF_BITSHIFT(NPPC)));

    if(allocate){
#ifndef __SYNTHESIS__
#ifdef __SDSCC__
    	data = (DATATYPE*)sds_alloc_non_cacheable(rows*(cols>>(XF_BITSHIFT(NPPC)))*sizeof(DATATYPE));
#else
    	data = (DATATYPE*)malloc(rows*(cols>>(XF_BITSHIFT(NPPC)))*sizeof(DATATYPE));
#endif

    	if (data == NULL) {
    		fprintf(stderr, "\nFailed to allocate memory\n");
    	} else {
    		allocatedFlag = 1;
    	}
#else
//#if (defined __SDSCC__) || (defined __SYNTHESIS__)
    	//void _ssdm_op_alloc(XF_TNAME(T,NPPC)*&);
#ifndef __SDA_MEM_MAP__	
    	void _ssdm_op_alloc(DATATYPE *&);
    	_ssdm_op_alloc(data);
#endif
#endif
    }

}

/*
template <int T, int ROWS, int COLS, int NPC>
inline XF_TNAME(T, XF_NPPC1) Mat<T, ROWS, COLS, NPC>::operator() (unsigned int r, unsigned int c) {

    XF_TNAME(T, XF_NPPC1) pix_val;

    unsigned int mask      = 0xFFFFFFFF;
    unsigned int npc_c     = c >> (XF_BITSHIFT(NPC));
    unsigned int npc_cols  = cols >> (XF_BITSHIFT(NPC));
    unsigned int pix_idx   = c & ~(mask << (XF_BITSHIFT(NPC)));

#ifdef __SDSVHLS__
    unsigned int bit_width= XF_DTPIXELDEPTH(T, NPC);

    pix_val = data[(r*npc_cols) + npc_c].range((pix_idx+1)*bit_width-1, pix_idx*bit_width);
#else
    unsigned int pix_depth = XF_DTPIXELDEPTH(T, NPC) / XF_CHANNELS(T,NPC);

    for (int ch=0; ch < XF_CHANNELS(T,NPC); ch++) {
       pix_val.range((ch+1)*pix_depth-1, ch*pix_depth) = data[r*npc_cols + npc_c].chnl[ch].range((pix_idx+1)*pix_depth-1, pix_idx*pix_depth);
    }
#endif

    return pix_val;
}

template <int T, int ROWS, int COLS, int NPC>
inline XF_CTUNAME(T, NPC) Mat<T, ROWS, COLS, NPC>::operator() (unsigned int r, unsigned int c, unsigned int ch) {

    XF_CTUNAME(T, NPC) pix_val;

    unsigned int mask      = 0xFFFFFFFF;
    unsigned int npc_c     = c >> (XF_BITSHIFT(NPC));
    unsigned int npc_cols  = cols >> (XF_BITSHIFT(NPC));
    unsigned int pix_idx   = c & ~(mask << (XF_BITSHIFT(NPC)));

#ifdef __SDSVHLS__
    unsigned int bit_width = XF_DTPIXELDEPTH(T, NPC);
    unsigned int pix_depth = XF_DTPIXELDEPTH(T, NPC) / XF_CHANNELS(T, NPC);

    pix_val = data[(r*npc_cols) + npc_c].range(pix_idx*bit_width + (ch+1)*pix_depth - 1, pix_idx*bit_width + ch*pix_depth);
#else
    unsigned int pix_depth = XF_DTPIXELDEPTH(T, NPC) / XF_CHANNELS(T, NPC);

    pix_val = data[r*npc_cols + npc_c].chnl[ch].range((pix_idx+1)*pix_depth-1, pix_idx*pix_depth);
#endif

    return pix_val;
}
*/
/*Copy constructor definition*/
template <int T, int ROWS, int COLS, int NPC>
inline Mat<T, ROWS, COLS, NPC>::Mat(const Mat& src) {

    init(src.rows, src.cols);

    for(int i =0; i< (rows*(cols>>(XF_BITSHIFT(NPC))));++i){
	data[i] = src.data[i];
    }
}

/*Assignment operator definition*/
template <int T, int ROWS, int COLS, int NPC>
inline Mat<T, ROWS, COLS, NPC>& Mat<T, ROWS, COLS, NPC>::operator=(const Mat& src) {

    if (this == &src)
    {
        return *this; // For self-assignment cases
    }

    // Cleaning up old data memory if any
    if (data != NULL) {
#ifndef __SYNTHESIS__
  #ifdef __SDSCC__
        sds_free(data);
  #else
        free(data);
  #endif
#endif
    }

    allocatedFlag = 0;

    init(src.rows, src.cols);

    for(int i =0; i< (rows*(cols>>(XF_BITSHIFT(NPC))));++i){
    	data[i] = src.data[i];
    }

    return *this;
}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat() {
#pragma HLS inline

    init(ROWS, COLS);
}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat(int _rows, int _cols, void *_data) {
#pragma HLS inline

#if defined __SDA_MEM_MAP__//(__SDSCC__)  && defined (__SYNTHESIS__)
	init(_rows, _cols, false);
	data = (DATATYPE *)_data;
#else
	init(_rows, _cols);
	copyTo(_data);
#endif

}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat(int _rows, int _cols) {
#pragma HLS inline

    init(_rows, _cols);
}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat(Size _sz) {
#pragma HLS inline

    init(_sz.height, _sz.width);
}

template<int T, int ROWS, int COLS, int NPPC>
inline XF_TNAME(T,NPPC) Mat<T, ROWS, COLS, NPPC>::read(int index){
#ifdef __SDSVHLS__
	return data[index];
#else
    int pixdepth = XF_PIXELWIDTH(T,NPPC);    //Total bits that make up the pixel
    int bitdepth = pixdepth / XF_CHANNELS(T,NPPC); //Total bits that make up each channel of the pixel
	DATATYPE tmp_val = data[index];
	XF_TNAME(T,NPPC) pack_val;

	// Packing data
	for(int i=0; i< NPPC;++i){
#pragma HLS UNROLL
		for(int j=0; j<XF_CHANNELS(T,NPPC);++j){
			pack_val.range((i*pixdepth+j*bitdepth) + bitdepth-1, (i*pixdepth+j*bitdepth)) = tmp_val.chnl[i][j];
		}
	}

	return pack_val;
#endif
}
template<int T, int ROWS, int COLS, int NPPC>
inline float Mat<T, ROWS, COLS, NPPC>::read_float(int index){

	ap_uint<32> val = read(index);
	float *val_out = (float *)(&val);
	return *val_out;

}
template<int T, int ROWS, int COLS, int NPPC>
inline void Mat<T, ROWS, COLS, NPPC>::write(int index, XF_TNAME(T,NPPC) val){
#ifdef __SDSVHLS__
	data[index] = val;
#else
    int pixdepth = XF_PIXELWIDTH(T,NPPC);    //Total bits that make up the pixel
    int bitdepth = pixdepth / XF_CHANNELS(T,NPPC); //Total bits that make up each channel of the pixel
	DATATYPE tmp_val;

	// Unpacking data
	for(int i=0; i< NPPC;++i){
		for(int j=0; j<XF_CHANNELS(T,NPPC);++j){
		 tmp_val.chnl[i][j] = val.range((i*pixdepth+j*bitdepth) + bitdepth-1, (i*pixdepth+j*bitdepth));
		}
	}
	data[index] = tmp_val;
#endif
}

template<int T, int ROWS, int COLS, int NPPC>
inline void Mat<T, ROWS, COLS, NPPC>::write_float(int index, float float_val){

	
	float val = float_val;
	ap_uint<32> *val_out = (ap_uint<32> *)(&val);
	write(index,*val_out);

}

template<int T, int ROWS, int COLS, int NPPC>
inline void Mat<T, ROWS, COLS, NPPC>::copyTo(void*_input) {
#pragma HLS inline

    XF_PTSNAME(T,NPPC) *input = (XF_PTSNAME(T,NPPC)*)_input;
    XF_CTUNAME(T,NPPC) in_val;

    int packcols  = cols >> XF_BITSHIFT(NPPC); 		//Total columns after considering parallelism
    int pixdepth = XF_PIXELWIDTH(T,NPPC);    		//Total bits that make up the pixel
    int bitdepth = pixdepth / XF_CHANNELS(T,NPPC); 	//Total bits that make up each channel of the pixel
    int nppc     = XF_NPIXPERCYCLE(NPPC);

    for (int r=0; r < rows; r++) {
        for (int c=0; c < packcols; c++) {
        	for (int p=0; p < nppc; p++) {
        	    for (int ch=0; ch < XF_CHANNELS(T,NPPC); ch++) {
					
					if (T == XF_32FC1) {
						in_val = float2ap_uint< ap_uint<32> >(input[XF_CHANNELS(T, NPPC)*((r*packcols + c)*nppc + p) + ch]);
					} else {
						in_val = input[XF_CHANNELS(T, NPPC)*((r*packcols + c)*nppc + p) + ch];
					}
#ifdef __SDSVHLS__
   	                	data[r*packcols + c].range((p*pixdepth)+(ch+1)*bitdepth-1, (p*pixdepth)+ch*bitdepth) = in_val;
#else
   	                	data[r*packcols + c].chnl[p][ch] = in_val;
#endif
   	                }
            }
        }
    }
}


template<int T, int ROWS, int COLS, int NPPC>
inline unsigned char* Mat<T, ROWS, COLS, NPPC>::copyFrom() {
#pragma HLS inline

//	int packcols  = cols >> XF_BITSHIFT(NPPC); //Total columns after considering parallelism
	int pixdepth = XF_PIXELWIDTH(T,NPPC);    //Total bits that make up the pixel
	int bitdepth = pixdepth / XF_CHANNELS(T,NPPC); //Total bits that make up each channel of the pixel
	int nppc     = XF_NPIXPERCYCLE(NPPC);

	int cv_nbytes = bitdepth/8;

	unsigned char *value = (unsigned char*)malloc(rows*cols*(XF_CHANNELS(T,NPPC))*(sizeof(unsigned char))*cv_nbytes);

	int xf_npc_idx = 0;
	int diff_ptr   = 0;
	int xf_ptr     = 0;
	int cv_ptr     = 0;

	for (int r=0; r < rows; r++) {
		for (int c=0; c < cols; c++) {
			for (int ch=0; ch < XF_CHANNELS(T,NPPC); ch++) {
				for (int b=0; b < cv_nbytes; ++b) {
#ifdef __SDSVHLS__
					value[cv_ptr++] = data[xf_ptr].range((xf_npc_idx*pixdepth)+(ch*bitdepth)+(b+1)*8-1, (xf_npc_idx*pixdepth)+(ch*bitdepth)+b*8);
#else
					value[cv_ptr++] = data[xf_ptr].chnl[xf_npc_idx][ch].range((b+1)*8-1, b*8);
#endif
				}
			}
			if(xf_npc_idx==nppc-1) {
				xf_npc_idx = 0;
				xf_ptr++;
			} else {
				xf_npc_idx++;
			}

		}
	}

	return (unsigned char*)value;
}

/* Member functions of Mat class */
template<int T, int ROWS, int COLS, int NPPC>
template<int DST_T>
inline void Mat<T, ROWS, COLS, NPPC>::convertTo(Mat<DST_T, ROWS, COLS, NPPC> &dst, int otype, double alpha, double beta) {

    assert((XF_CHANNELS(T,NPPC) == 1) && "Multi-channel images not supported");

    XF_TNAME(T,     NPPC) tmp_in_pix;
    XF_TNAME(DST_T, NPPC) tmp_out_pix;

    XF_CTUNAME(T,NPPC) 		in_pix;
    XF_CTUNAME(DST_T,NPPC)  out_pix;

    int min, max;

    if(DST_T== XF_8UC1) {
        min = 0; max = 255;
    }
    else if(DST_T == XF_16UC1) {
        min = 0; max = 65535;
    }
    else if(DST_T == XF_16SC1) {
        min = -32768; max = 32767;
    }
    else if(DST_T == XF_32SC1) {
        min = -2147483648; max = 2147483647;
    }
    else{
	assert(1 && "Output image type not supoorted. XF_8UC1, XF_16UC1, XF_16SC1 and XF_32SC1 are valid");
    }

#define __SATCAST(X) ( X >= max ? max : (X < 0 ? 0 : lround(X)) )

    for(int i=0;i<rows;i++){
    	for(int j=0; j<cols>>(XF_BITSHIFT(NPPC));j++)
    	{

    		int IN_STEP   = XF_PIXELDEPTH(XF_DEPTH(    T, NPPC));
    		int OUT_STEP  = XF_PIXELDEPTH(XF_DEPTH(DST_T, NPPC));
    		int in_shift  = 0;
    		int out_shift = 0;

    		for (int k = 0; k < (1<<(XF_BITSHIFT(NPPC))); k++)
    		{
#ifdef __SDSVHLS__
    			in_pix = data[(i*cols>>(XF_BITSHIFT(NPPC)))+j].range(in_shift+IN_STEP-1, in_shift);
#else
    			in_pix = data[(i*cols>>(XF_BITSHIFT(NPPC)))+j].chnl[k][0];
#endif

    			if(otype == XF_CONVERT_16U_TO_8U || otype == XF_CONVERT_16S_TO_8U ||otype == XF_CONVERT_32S_TO_8U || otype == XF_CONVERT_32S_TO_16U || otype == XF_CONVERT_32S_TO_16S) {

    				float tmp = (float)(in_pix * alpha + beta);
    				in_pix = __SATCAST(tmp);

    				if(in_pix < min)
    					in_pix = min;
    				if(in_pix > max)
    					in_pix = max;

    				tmp_out_pix.range(out_shift+ OUT_STEP-1, out_shift) = in_pix;
    			} else {
    				if ((((XF_PTNAME(XF_DEPTH(DST_T, NPPC)))in_pix * alpha)+beta) > max){
    					tmp_out_pix.range(out_shift+OUT_STEP-1, out_shift) = max;

    				} else if ((((XF_PTNAME(XF_DEPTH(DST_T, NPPC)))in_pix * alpha)+beta) < min){
    					tmp_out_pix.range(out_shift+OUT_STEP-1, out_shift) = min;

    				} else {
    					tmp_out_pix.range(out_shift+OUT_STEP-1, out_shift) = __SATCAST(in_pix * alpha + beta);
    				}
    			}

    			out_pix = tmp_out_pix.range(out_shift+OUT_STEP-1, out_shift);
#ifdef __SDSVHLS__
    			dst.data[(i*cols>>(XF_BITSHIFT(NPPC)))+j].range(out_shift+OUT_STEP-1, out_shift) = out_pix;
#else
    			dst.data[(i*cols>>(XF_BITSHIFT(NPPC)))+j].chnl[k][0] = out_pix;
#endif

    			in_shift  = in_shift + IN_STEP;
    			out_shift = out_shift + OUT_STEP;

    		}
    	}
    }
}



template<int SRC_T, int ROWS, int COLS, int NPC>
Mat<SRC_T, ROWS, COLS, NPC>::~Mat() {

#ifndef __SYNTHESIS__
    if (data != NULL && allocatedFlag == 1) {
    #ifdef __SDSCC__
       sds_free(data);
    #else
       free(data);
    #endif
    }
#endif
}
//----------------------------------------------------------------------------------------------------//

}; // end of 'xf' namespace

#endif // _XF_STRUCTS_H_
