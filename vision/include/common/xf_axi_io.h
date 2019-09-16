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

#ifndef ___XF__AXI_IO__
#define ___XF__AXI_IO__
#include "utils/x_hls_utils.h"
#include <assert.h>

namespace xf {

template<int W, typename T>
void AXIGetBitFields(ap_uint<W> pix, int start, int w, T& val) {
#pragma HLS inline
    assert(start >= 0 && start+w <= W);
    val = (T)pix(start+w-1, start);
}

template<int W>
void AXIGetBitFields(ap_uint<W> pix, int start, int w, float& val) {
#pragma HLS inline
    assert(w == 32 && start >= 0 && start+w <= W);
    fp_struct<float> temp((ap_uint<32>)pix(start+w-1, start));
    val =  temp.to_float();
}

template<int W>
void AXIGetBitFields(ap_uint<W> pix, int start, int w, double& val) {
#pragma HLS inline
    assert(w == 64 && start >= 0 && start+w <= W);
    fp_struct<double> temp((ap_uint<64>)pix(start+w-1, start));
    val = temp.to_double();
}

template<int W, typename T>
void AXIGetBitFields(ap_axiu<W,1,1,1> axi, int start, int w, T& val) {
#pragma HLS inline
    AXIGetBitFields(axi.data, start, w, val);
}

template<int W, typename T>
void AXISetBitFields(ap_uint<W>& pix, int start, int w, T val) {
#pragma HLS inline
    assert(start >= 0 && start+w <= W);
    pix(start+w-1, start) = val;
}

template<int W>
void AXISetBitFields(ap_uint<W>& pix, int start, int w, float val) {
#pragma HLS inline
    assert(w == 32 && start >= 0 && start+w <= W);
    fp_struct<float> temp(val);
    pix(start+w-1, start) = temp.data();
}

template<int W>
void AXISetBitFields(ap_uint<W>& pix, int start, int w, double val) {
#pragma HLS inline
    assert(w == 64 && start >= 0 && start+w <= W);
    fp_struct<double> temp(val);
    pix(start+w-1, start) = temp.data();
}

template<int W, typename T>
void AXISetBitFields(ap_axiu<W,1,1,1>& axi, int start, int w, T val) {
#pragma HLS inline
    AXISetBitFields(axi.data, start, w, val);
}

}; // namespace xf

#endif

