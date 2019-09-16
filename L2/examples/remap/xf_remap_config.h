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

#ifndef _XF_REMAP_CONFIG_H_
#define _XF_REMAP_CONFIG_H_

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_remap.hpp"
#include "xf_config_params.h"

// Resolve interpolation type:
#if INTERPOLATION == 0
#define XF_INTERPOLATION_TYPE XF_INTERPOLATION_NN
#else
#define XF_INTERPOLATION_TYPE XF_INTERPOLATION_BILINEAR
#endif

// Set the image type and maps pixel depth:
#if GRAY
#define PTR_IMG_WIDTH 8
#else
#define PTR_IMG_WIDTH 32
#endif
#define TYPE_XY XF_32FC1
#define PTR_MAP_WIDTH 32

#if GRAY
#define TYPE XF_8UC1
#define CHANNELS 1
#else // RGB
#define TYPE XF_8UC3
#define CHANNELS 3
#endif

// Set the optimization type:
// Only XF_NPPC1 is available for this algorithm currently
#define NPC XF_NPPC1

#define HEIGHT 1080
#define WIDTH 1920

#endif
