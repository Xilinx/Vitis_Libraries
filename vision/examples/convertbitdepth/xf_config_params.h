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
/* Optimization type */


#define RO 0 // Resource Optimized (8-pixel implementation)
#define NO 1 // Normal Operation (1-pixel implementation)

/* Conversion Type*/
 

#define XF_CONVERT16UTO8U  0 // set to convert bit depth from unsigned 16-bit to unsigned 8-bit
#define XF_CONVERT16STO8U  0 // set to convert bit depth from signed   16-bit to unsigned 8-bit
#define XF_CONVERT32STO8U  0  // set to convert bit depth from signed   32-bit to unsigned 8-bit
#define XF_CONVERT32STO16U 0 // set to convert bit depth from signed   32-bit to unsigned 16-bit
#define XF_CONVERT32STO16S 0  // set to convert bit depth from signed   32-bit to signed   16-bit
#define XF_CONVERT8UTO16U  0  // set to convert bit depth from unsigned 8-bit  to 16-bit unsigned
#define XF_CONVERT8UTO16S  1  // set to convert bit depth from unsigned 8-bit  to 16-bit signed
#define XF_CONVERT8UTO32S  0  // set to convert bit depth from unsigned 8-bit  to 32-bit unsigned
#define XF_CONVERT16UTO32S 0  // set to convert bit depth from unsigned 16-bit to 32-bit signed
#define XF_CONVERT16STO32S 0  // set to convert bit depth from signed   16-bit to 32-bit signed
