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

#ifndef _XF_GAUSSIAN_FILTER_LUT_HPP_
#define _XF_GAUSSIAN_FILTER_LUT_HPP_

#include "common/xf_types.h"

/*****************************************************************************
 * Gaussian Filters
 *****************************************************************************/
 

#define CENTER_COEFF_3x3   40588 // Q0.16
unsigned short wts_3x3[2] = {743, 5493}; // Q0.16

/*
#define CENTER_COEFF_5x5   994 // Q0.12
uint12_q0 wts_5x5[3] = {55, 235, 483}; // Q0.12
*/

#define CENTER_COEFF_5x5   15063 // Q0.16 (0.229861*65535)
unsigned short wts_5x5[5] = {47, 411, 845, 3569, 7332}; // Q0.16

/*
#define CENTER_COEFF_7x7   496 // Q0.12
uint12_q0 wts_7x7[6] = {18, 26, 79, 114, 238, 344}; // Q0.12
*/

#define CENTER_COEFF_7x7   7693 // Q0.16  (0.117396*65535)
//uint12_q0 wts_7x7[9] = {0.000163, 0.001023, 0.003080, 0.004448,0.006422, 0.019332, 0.027913, 0.058195, 0.084027}; // Q0.16
unsigned short wts_7x7[9] = {10, 64, 195, 282, 407, 1225, 1770, 3690, 5328}; // Q0.16



#endif//_AU_GAUSSIAN_FILTER_LUT_HPP_
