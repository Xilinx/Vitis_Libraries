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

#define SPC 1 // Normal Operation
#define MPC 0 // Resource Optimized

/*  set the type of thresholding*/
#define THRESH_TYPE XF_THRESHOLD_TYPE_BINARY_INV

#define INPUT_PTR_WIDTH 64
#define OUTPUT_PTR_WIDTH 64

/* Median kernel smoothening parameter */
#define WINDOW_SIZE 3

/* Filter width for canny */
#define FILTER_WIDTH 3

#define L1NORM 1
#define L2NORM 0

#define XF_USE_URAM true

#define RGB 0
#define GRAY 1

/* Gaussian filter params */
#define FILTER_SIZE_3 1
#define FILTER_SIZE_5 0
#define FILTER_SIZE_7 0

#define GAUSSIAN_INPUT_PTR_WIDTH 32
#define GAUSSIAN_OUTPUT_PTR_WIDTH 8
/* Gaussian filter Param ends  */

/* OTSU kernel params */
// Set the pixel depth:
#define OTSU_PIXEL_TYPE XF_8UC1

#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2
#define XF_CV_DEPTH_IN_1 2
#define XF_CV_DEPTH_OUT_1 2