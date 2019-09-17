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

#ifndef _XF_ARITHM_CONFIG_H_
#define _XF_ARITHM_CONFIG_H_

#include "hls_stream.h"
#include <ap_int.h>
#include "xf_config_params.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#include "core/xf_arithm.hpp"

#define HEIGHT 2160
#define WIDTH 3840

// Resolve function name:
#if FUNCT_NUM == 0
#define FUNCT_NAME add
#elif FUNCT_NUM == 1
#define FUNCT_NAME addS
#elif FUNCT_NUM == 2
#define FUNCT_NAME subtract
#elif FUNCT_NUM == 3
#define FUNCT_NAME SubS
#elif FUNCT_NUM == 4
#define FUNCT_NAME SubRS
#define FUNCT_SUBRS
#elif FUNCT_NUM == 5
#define FUNCT_NAME multiply
#define FUNCT_MULTIPLY
#elif FUNCT_NUM == 6
#define FUNCT_NAME absdiff
#elif FUNCT_NUM == 7
#define FUNCT_NAME bitwise_and
#elif FUNCT_NUM == 8
#define FUNCT_NAME bitwise_xor
#elif FUNCT_NUM == 9
#define FUNCT_NAME bitwise_not
#define FUNCT_BITWISENOT
#elif FUNCT_NUM == 10
#define FUNCT_NAME bitwise_or
#elif FUNCT_NUM == 11
#define FUNCT_NAME min
#elif FUNCT_NUM == 12
#define FUNCT_NAME max
#elif FUNCT_NUM == 13
#define FUNCT_NAME set
#elif FUNCT_NUM == 14
#define FUNCT_NAME zero
#define FUNCT_ZERO
#elif FUNCT_NUM == 15
#define FUNCT_NAME compare
#else
#define FUNCT_NAME add
#endif

// Resolve pixel precision:

#if NO
#define NPC1 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#endif

#if T_16S
#if GRAY
#define TYPE XF_16SC1
#if NO
#define PTR_WIDTH 16
#else
#define PTR_WIDTH 128
#endif
#else
#define TYPE XF_16SC3
#if NO
#define PTR_WIDTH 64
#else
#define PTR_WIDTH 512
#endif
#endif
#endif

#if T_8U
#if GRAY
#define TYPE XF_8UC1
#if NO
#define PTR_WIDTH 8
#else
#define PTR_WIDTH 64
#endif
#else
#define TYPE XF_8UC3
#if NO
#define PTR_WIDTH 24
#else
#define PTR_WIDTH 256
#endif
#endif
#endif

#endif // end of _XF_ARITHM_CONFIG_H_
