/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
    //		xf::cv::multiply< XF_CONVERT_POLICY_SATURATE,TYPE, HEIGHT, WIDTH,
    // NPC1>(imgInput1,imgInput2,imgOutput,0.05);
    //		xf::cv::Max< TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,imgInput2,imgOutput);
    //		xf::cv::Min< TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,imgInput2,imgOutput);
    //		xf::cv::compare<XF_CMP_NE, TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,imgInput2,imgOutput);
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
    //	xf::cv::MaxS    <TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::MinS    <TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);
    //	xf::cv::set     <TYPE, HEIGHT, WIDTH,NPC1>(imgInput1,scl,imgOutput);

    xf::cv::FUNCT_NAME<
#ifdef EXTRA_PARM
        EXTRA_PARM,
#endif
        TYPE, HEIGHT, WIDTH, NPC1>(imgInput1, scl, imgOutput); // MaxS, MinS, set)
}

#endif
