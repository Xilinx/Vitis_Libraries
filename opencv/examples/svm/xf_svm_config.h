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

#ifndef _XF_SVM_CONFIG_H_
#define _XF_SVM_CONFIG_H_

/////  To set the parameters in the test-bench /////

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "xf_config_params.h"
#include "imgproc/xf_svm.hpp"

typedef	short int int16_t;
typedef	unsigned short int uint16_t;
typedef unsigned char uchar;


#define 	INDEX_ARR_1			0
#define 	INDEX_ARR_2			0

#define 	IN_FRAC_BITS_1		15
#define 	IN_FRAC_BITS_2		15

void svm_accel(xf::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_1, XF_NPPC1> &Input1, xf::Mat<XF_16SC1, 1, IN_ARRAY_SIZE_1, XF_NPPC1> &Input2, unsigned short ind1, unsigned short ind2, unsigned short frac1, unsigned short frac2, unsigned short n, unsigned char &out_frac, ap_int<32> &resultFIX);

#endif  // end of _XF_SVM_CONFIG_H_
