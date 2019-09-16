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
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_DEMOSIACING_CONFIG_H_
#define _XF_DEMOSIACING_CONFIG_H_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_demosaicing.hpp"
#include "xf_config_params.h"

// Resolve input and output pixel type:
#if T_8U
#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_8UC3
#define PTR_IN_WIDTH 8
#define PTR_OUT_WIDTH 32
#endif
#if T_10U
#define IN_TYPE XF_10UC1
#define OUT_TYPE XF_10UC3
#define PTR_IN_WIDTH 16
#define PTR_OUT_WIDTH 32
#endif
#if T_12U
#define IN_TYPE XF_12UC1
#define OUT_TYPE XF_12UC3
#define PTR_IN_WIDTH 16
#define PTR_OUT_WIDTH 32
#endif
#if T_16U
#define IN_TYPE XF_16UC1
#define OUT_TYPE XF_16UC3
#define PTR_IN_WIDTH 16
#define PTR_OUT_WIDTH 64
#endif

// Resolve optimization type:
#define NPC1 XF_NPPC1

// Resolve Bayer pattern:
#if BAYER_PATTERN == 0
#define XF_PATTERN XF_BAYER_BG
#elif BAYER_PATTERN == 1
#define XF_PATTERN XF_BAYER_GB
#elif BAYER_PATTERN == 2
#define XF_PATTERN XF_BAYER_GR
#elif BAYER_PATTERN == 3
#define XF_PATTERN XF_BAYER_RG
#else
#define XF_PATTERN XF_BAYER_BG
#endif

#endif // _XF_DEMOSAICING_CONFIG_H_
