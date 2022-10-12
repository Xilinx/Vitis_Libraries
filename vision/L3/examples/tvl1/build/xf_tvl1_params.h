/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef _XF_TVL1_PARAMS_H_
#define _XF_TVL1_PARAMS_H_

// NPC parameter
#define NPC_TVL_INNERCORE 2
#define NPC_TVL_OTHER 1

// Internal Parameter
#define ERROR_BW 32
#define INVSCALE_BITWIDTH 32
#define IMG_BW 8
#define IMG_TYPE XF_8UC1
#define IMG_F_BITS 0
#define MAX_FLOW_VALUE 6
#define MAX_NUM_LEVELS 16
#define MEDIANBLURFILTERING 5
#define MB_INST 1
#define FLOW_BITWIDTH 32
#define FLOW_TYPE XF_32SC1
#define FLOW_F_BITS 16

// port widths
#define IMAGE_PTR_WIDTH 128
#define INT_PTR_WIDTH FLOW_BITWIDTH* NPC_TVL_INNERCORE
#define OUT_PTR_WIDTH 64 * NPC_TVL_INNERCORE

///////Debug macro
#define LOOP_DEBUG 0
#define LOOP1_CNT 2

// Resize macro
#define MAXDOWNSCALE 2
#define INTERPOLATION 1
#define NEWWIDTH 512
#define NEWHEIGHT 512
#define WIDTH 512
#define HEIGHT 512
#define XF_CV_DEPTH_U1_in 2
#define XF_CV_DEPTH_U1_out 2
#define XF_CV_DEPTH_U2_in 2
#define XF_CV_DEPTH_U2_out 2
#define XF_CV_DEPTH_I1wx 2
#define XF_CV_DEPTH_I1wy 2
#define XF_CV_DEPTH_U1_in 2
#define XF_CV_DEPTH_U2_in 2
#define XF_CV_DEPTH_grad 2
#define XF_CV_DEPTH_rhoc 2
#define XF_CV_DEPTH_p11_in 2
#define XF_CV_DEPTH_p12_in 2
#define XF_CV_DEPTH_p21_in 2
#define XF_CV_DEPTH_p22_in 2
#define XF_CV_DEPTH_p11_out 2
#define XF_CV_DEPTH_p12_out 2
#define XF_CV_DEPTH_p21_out 2
#define XF_CV_DEPTH_p22_out 2
#define XF_CV_DEPTH_V1 2
#define XF_CV_DEPTH_V2 2
#define XF_CV_DEPTH_divP1 2
#define XF_CV_DEPTH_divP2 2
#define XF_CV_DEPTH_U1x 2
#define XF_CV_DEPTH_U1y 2
#define XF_CV_DEPTH_U2x 2
#define XF_CV_DEPTH_U2y 2
#define XF_CV_DEPTH_U1_passEV 2
#define XF_CV_DEPTH_U2_passEV 2
#define XF_CV_DEPTH_U1_out 2
#define XF_CV_DEPTH_U1_out_ddr 2
#define XF_CV_DEPTH_U2_out 2
#define XF_CV_DEPTH_U2_out_ddr 2
#define XF_CV_DEPTH_p11_passDIV 2
#define XF_CV_DEPTH_p12_passDIV 2
#define XF_CV_DEPTH_p21_passDIV 2
#define XF_CV_DEPTH_p22_passDIV 2
#define XF_CV_DEPTH_p11_passUU 2
#define XF_CV_DEPTH_p12_passUU 2
#define XF_CV_DEPTH_p21_passUU 2
#define XF_CV_DEPTH_p22_passUU 2
#define XF_CV_DEPTH_p11_passFG 2
#define XF_CV_DEPTH_p12_passFG 2
#define XF_CV_DEPTH_p21_passFG 2
#define XF_CV_DEPTH_p22_passFG 2
#define XF_CV_DEPTH_U1 2
#define XF_CV_DEPTH_U2 2
#define XF_CV_DEPTH_I0 2
#define XF_CV_DEPTH_I1 2
#define XF_CV_DEPTH_I1x 2
#define XF_CV_DEPTH_I1y 2
#define XF_CV_DEPTH_I1w 2
#define XF_CV_DEPTH_I1_copy1 2
#define XF_CV_DEPTH_I1_copy2 2
#define XF_CV_DEPTH_I1_flowtype 2
#define XF_CV_DEPTH_U1_copy1 2
#define XF_CV_DEPTH_U1_copy2 2
#define XF_CV_DEPTH_U1_copy3 2
#define XF_CV_DEPTH_U1_copy4 2
#define XF_CV_DEPTH_U2_copy1 2
#define XF_CV_DEPTH_U2_copy2 2
#define XF_CV_DEPTH_U2_copy3 2
#define XF_CV_DEPTH_U2_copy4 2
#define XF_CV_DEPTH_I1wx_copy1 2
#define XF_CV_DEPTH_I1wy_copy1 2
#define XF_CV_DEPTH_I1wx_copy2 2
#define XF_CV_DEPTH_I1wy_copy2 2
#define XF_CV_DEPTH_U12 2
#define XF_CV_DEPTH_U1_resize 2
#define XF_CV_DEPTH_U2_resize 2

#endif //_XF_TVL1_PARAMS_H_
