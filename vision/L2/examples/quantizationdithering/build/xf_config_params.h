/*
 * Copyright 2020 Xilinx, Inc.
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

#define MPC 0 // Multiple Pixels per Clock operation
#define SPC 1 // Single Pixel per Clock operation

// port widths
#define INPUT_PTR_WIDTH 128
#define OUTPUT_PTR_WIDTH 128

/* SCALEFACTOR & MAXREPRESENTEDVALUE should power of 2 */
#define SCALEFACTOR 256
#define MAXREPRESENTEDVALUE 65536

/* Input/Ouput pixel depth in bits */
#define INPUTPIXELDEPTH 16
#define OUTPUTPIXELDEPTH 8

/* Input image type */
#define RGB 1
#define GRAY 0

/* Input image Dimensions */
#define WIDTH 1024 // Maximum Input image width
#define HEIGHT 676 // Maximum Input image height
#define XF_CV_DEPTH_IN 2
#define XF_CV_DEPTH_OUT 2