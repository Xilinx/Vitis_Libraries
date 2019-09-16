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

#ifndef _XF_MEAN_SHIFT_CONFIG_H
#define _XF_MEAN_SHIFT_CONFIG_H

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_mean_shift.hpp"
#include "xf_config_params.h"

// set 1 for video
#define VIDEO_INPUT 0

// total number of frames to be tracked
#define TOTAL_FRAMES 100

#define XF_HEIGHT 1080
#define XF_WIDTH  1920

// Modify the information on objects to be tracked
// coordinate system uses (row, col) where row = 0 and col = 0 correspond to the top-left corner of the input image
const int X1[XF_MAX_OBJECTS]= {50, 170, 300};          // col coordinates of the top-left corner of all the objects to be tracked
const int Y1[XF_MAX_OBJECTS]= {50, 200, 400};          // row coordinates of the top-left corner of all the objects to be tracked
const int WIDTH_MST[XF_MAX_OBJECTS] = {100, 100, 100};       // width of all the objects to be tracked (measured from top-left corner)
const int HEIGHT_MST[XF_MAX_OBJECTS] = {100, 100, 100};     // height of all the objects to be tracked (measured from top-left corner)      


#define IN_TYPE unsigned int

#endif 


