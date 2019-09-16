/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
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

#include "xf_arithm_config.h"

#if ARRAY
void arithm_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgInput1,
                  xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgInput2,
                  xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgOutput) {
    xf::cv::FUNCT_NAME<
#ifdef EXTRA_PARM
        EXTRA_PARM,
#endif
        TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, imgInput2, imgOutput
#ifdef EXTRA_ARG
                                   ,
                                   EXTRA_ARG
#endif
    ); // MaxS, MinS, set)

    //		xf::cv::absdiff< TYPE, HEIGHT, WIDTH, NPC1 >(imgInput1,imgInput2,imgOutput);
    //		xf::cv::add<XF_CONVERT_POLICY_SATURATE, TYPE, HEIGHT, WIDTH, NPC1 >(imgInput1,imgInput2,imgOutput);
    //		xf::cv::subtract<XF_CONVERT_POLICY_SATURATE, TYPE, HEIGHT, WIDTH, NPC1 >(imgInput1,imgInput2,imgOutput);
    //		xf::cv::bitwise_and<TYPE, HEIGHT, WIDTH, NPC1 >(imgInput1,imgInput2,imgOutput);
    //		xf::cv::bitwise_or<TYPE, HEIGHT, WIDTH, NPC1 >(imgInput1,imgInput2,imgOutput);
    //		xf::cv::bitwise_not<TYPE, HEIGHT, WIDTH, NPC1>(imgInput1,imgOutput);
    //		xf::cv::bitwise_xor< TYPE, HEIGHT, WIDTH, NPC1 >(imgInput1,imgInput2,imgOutput);
    //		xf::cv::multiply< XF_CONVERT_POLICY_SATURATE,TYPE, HEIGHT, WIDTH, NPC1
    //>(imgInput1,imgInput2,imgOutput,0.05); 		xf::cv::Max< TYPE, HEIGHT,
    //WIDTH,NPC1>(imgInput1,imgInput2,imgOutput); 		xf::cv::Min< TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,imgInput2,imgOutput);
    //				xf::cv::compare<XF_CMP_NE, TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,imgInput2,imgOutput);
    //		xf::cv::zero<TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,imgOutput);
}

#endif

#if SCALAR
void arithm_accel(xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgInput1,
                  unsigned char scl[XF_CHANNELS(TYPE, NPC1)],
                  xf::cv::Mat<TYPE, HEIGHT, WIDTH, NPC1>& imgOutput) {
    //	xf::cv::addS	<XF_CONVERT_POLICY_SATURATE, TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::SubS	<XF_CONVERT_POLICY_SATURATE, TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::SubRS   <XF_CONVERT_POLICY_SATURATE, TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::compareS<XF_CMP_LE,                  TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::MaxS    < 							 TYPE, HEIGHT,
    // WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::MinS< 							 TYPE, HEIGHT,
    // WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::set<							 TYPE, HEIGHT,
    // WIDTH,NPC1>(imgInput1,scl,imgOutput);

    xf::cv::FUNCT_NAME<
#ifdef EXTRA_PARM
        EXTRA_PARM,
#endif
        TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, scl, imgOutput); // MaxS, MinS, set)
}

#endif
