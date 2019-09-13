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

#ifndef _XF_MIN_MAX_LOC_CONFIG_H_
#define _XF_MIN_MAX_LOC_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.h"
#include "core/xf_min_max_loc.hpp"


#if T_8U
	#define PTYPE XF_8UC1
	#define INTYPE unsigned char
#endif
#if T_16U
	#define PTYPE XF_16UC1
	#define INTYPE unsigned short
#endif
#if T_16S
	#define PTYPE XF_16SC1
	#define INTYPE short
#endif
#if T_32S
	#define PTYPE XF_32SC1
	#define INTYPE unsigned int
#endif

#if NO
	#define _NPPC XF_NPPC1
#endif

#if RO
	#define _NPPC XF_NPPC8
#endif

/*  set the height and weight  */
#define HEIGHT 2160
#define WIDTH  3840

void min_max_loc_accel(xf::Mat<PTYPE, HEIGHT, WIDTH, _NPPC> &imgInput, int32_t &min_value, int32_t &max_value, unsigned short &_min_locx, unsigned short &_min_locy, unsigned short &_max_locx, unsigned short &_max_locy);

#endif
