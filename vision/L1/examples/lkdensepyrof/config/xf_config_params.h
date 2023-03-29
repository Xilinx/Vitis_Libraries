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

#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_CONFIG__
#define __XF_PYR_DENSE_OPTICAL_FLOW_CONFIG__

#include "ap_int.h"
#include "hls_stream.h"
#include "assert.h"
#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "video/xf_pyr_dense_optical_flow_wrapper.hpp"
#include "imgproc/xf_pyr_down.hpp"

#define HEIGHT 512
#define WIDTH 512

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define XF_CV_DEPTH_CURR_IMG 2
#define XF_CV_DEPTH_NEXT_IMG 2
#define XF_CV_DEPTH_STREAM_IN 2
#define XF_CV_DEPTH_STREAM_OUT 2

#define NPPCX XF_NPPC1

#define IN_TYPE XF_8UC1
#define OUT_TYPE XF_32UC1

#define TYPE unsigned char

#define TYPE_FLOW_WIDTH 16
#define TYPE_FLOW_INT 10
#define TYPE_FLOW_TYPE ap_fixed<TYPE_FLOW_WIDTH, TYPE_FLOW_INT>

#define WINSIZE_OFLOW 11

#define NUM_LEVELS 5
#define NUM_ITERATIONS 5

#define NUM_LINES_FINDIT 50

#define XF_USE_URAM false

#define INPUT_PTR_WIDTH 32
#define OUTPUT_PTR_WIDTH 32

void pyr_dense_optical_flow_pyr_down_accel(ap_uint<INPUT_PTR_WIDTH>* img_inp,
                                           ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                                           int in_rows,
                                           int in_cols,
                                           int out_rows,
                                           int out_cols);

void pyr_dense_optical_flow_accel(ap_uint<INPUT_PTR_WIDTH>* _current_img,
                                  ap_uint<INPUT_PTR_WIDTH>* _next_image,
                                  ap_uint<OUTPUT_PTR_WIDTH>* _streamFlowin,
                                  ap_uint<OUTPUT_PTR_WIDTH>* _streamFlowout,
                                  int level,
                                  int scale_up_flag,
                                  float scale_in,
                                  int init_flag,
                                  int cur_img_rows,
                                  int cur_img_cols,
                                  int next_img_rows,
                                  int next_img_cols,
                                  int flow_rows,
                                  int flow_cols,
                                  int flow_iter_rows,
                                  int flow_iter_cols);

#endif
