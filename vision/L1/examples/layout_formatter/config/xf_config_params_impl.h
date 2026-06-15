/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

#ifndef _XF_CONFIG_PARAMS_IMPL_H_
#define _XF_CONFIG_PARAMS_IMPL_H_

//=============================================================================
// CONFIGURATION STRATEGY: COMPILE-TIME vs RUNTIME SELECTION
//=============================================================================
// This design supports both compile-time and runtime configuration:
//
// COMPILE-TIME (macros below):
//   - Determines which hardware paths are SYNTHESIZED into the IP
//   - Multiple layouts/datatypes can be enabled simultaneously (set to 1)
//   - Enabling more options increases hardware resource usage
//   - All enabled options will be available for runtime selection
//
// RUNTIME (function parameters: layout_format, datatype, out_channels):
//   - Selects which pre-compiled hardware path to execute
//   - Runtime parameters MUST match one of the compile-time enabled options
//   - Allows dynamic switching between enabled formats without reconfiguration
//
// EXAMPLE:
//   Compile-time: XF_INT8=1, XF_FP16=1, _XF_NHWC_=1, _XF_NCHW_=1
//   Hardware synthesized with: INT8+FP16 datatypes, NHWC+NCHW layouts
//   Runtime: Can switch between (INT8,NHWC), (INT8,NCHW), (FP16,NHWC), (FP16,NCHW)
//
// INTERFACE SIZING:
//   - Buffers/AXI interfaces are sized for the LARGEST enabled datatype
//=============================================================================

#include "common/xf_common.hpp"
#include "common/xf_utility.hpp"
#include "imgproc/xf_layout_formatter.hpp"
#include <ap_int.h>
#include <hls_stream.h>
#include "common/xf_params.hpp"

// Max height and Max width
static constexpr int WIDTH = 128;
static constexpr int HEIGHT = 128;

// Pixels processed per cycle
#define NPPCX XF_NPPC4

//=============================================================================
// USER CONFIGURATION: ENABLE REQUIRED LAYOUTS AND DATATYPES
//=============================================================================
// Set to 1 to enable, 0 to disable.
// Multiple options CAN and SHOULD be enabled for multi-format support.
// Hardware resources increase with more enabled options.

// Layout Formats (Data Order/Arrangement)
#ifndef _XF_NHWC_
#define _XF_NHWC_ 1 // NHWC: Height-Width-Channel (interleaved channels)
#endif
#ifndef _XF_NCHW_
#define _XF_NCHW_ 1 // NCHW: Channel-Height-Width (planar/separated channels)
#endif
#ifndef _XF_HCWNC4_
#define _XF_HCWNC4_ 1 // HCWNC4: Height-Channel(4)-Width (4-channel aligned)
#endif
#ifndef _XF_HCWNC8_
#define _XF_HCWNC8_ 0 // HCWNC8: Height-Channel(8)-Width (8-channel aligned)
#endif

// Channel Configuration
#ifndef _XF_RGBA_
#define _XF_RGBA_ 1 // 1=RGBA (4 channels), 0=RGB (3 channels)
#endif

// Data Types (Multiple can be enabled - interfaces sized for largest)
#ifndef XF_FP32
#define XF_FP32 1 // 32-bit floating point
#endif
#ifndef XF_BF16
#define XF_BF16 1 // 16-bit brain float (BFloat16)
#endif
#ifndef XF_FP16
#define XF_FP16 1 // 16-bit floating point (IEEE 754 half)
#endif
#ifndef XF_INT8
#define XF_INT8 1 // 8-bit integer (unsigned char)
#endif
//=============================================================================

//=============================================================================
// INTERNAL CONFIGURATION - DO NOT EDIT BELOW THIS LINE
//=============================================================================
// The following macros encode enabled options as bitfields for runtime validation

// Datatype Bit Positions (for SELECT_TYPE bitfield encoding)
#define XF_FP32_BIT 3 // Bit 3: FP32 enabled
#define XF_BF16_BIT 2 // Bit 2: BF16 enabled
#define XF_FP16_BIT 1 // Bit 1: FP16 enabled
#define XF_INT8_BIT 0 // Bit 0: INT8 enabled

// Layout Bit Positions (for SELECT_ORDER bitfield encoding)
#define XF_NCHW_BIT 3   // Bit 3: NCHW enabled
#define XF_HCWNC8_BIT 2 // Bit 2: HCWNC8 enabled
#define XF_HCWNC4_BIT 1 // Bit 1: HCWNC4 enabled
#define XF_NHWC_BIT 0   // Bit 0: NHWC enabled

// Combined Bitfield Selection (passed as template parameters)
// These encode ALL enabled options as bits in a single integer
// Runtime validation uses: if ((DATATYPE & (1 << data_type)) == 0) -> error

#define SELECT_TYPE (XF_FP32 << XF_FP32_BIT | XF_BF16 << XF_BF16_BIT | XF_FP16 << XF_FP16_BIT | XF_INT8 << XF_INT8_BIT)
#define SELECT_ORDER \
    (_XF_NCHW_ << XF_NCHW_BIT | _XF_HCWNC8_ << XF_HCWNC8_BIT | _XF_HCWNC4_ << XF_HCWNC4_BIT | _XF_NHWC_ << XF_NHWC_BIT)

// First, define all Datatype ENABLE_* macros to default (0)
#define ENABLE_FP32 0
#define ENABLE_BF16 0
#define ENABLE_FP16 0
#define ENABLE_INT8 0

// Then update their values as per selection
#if XF_FP32
#undef ENABLE_FP32
#define ENABLE_FP32 1
#elif XF_BF16
#undef ENABLE_BF16
#define ENABLE_BF16 1
#elif XF_FP16
#undef ENABLE_FP16
#define ENABLE_FP16 1
#elif XF_INT8
#undef ENABLE_INT8
#define ENABLE_INT8 1
#endif

// Validation: At least one datatype must be enabled
#if !(XF_FP32 || XF_BF16 || XF_FP16 || XF_INT8)
#error "No valid datatype defined - enable at least one datatype option"
#endif

// First, define all Layout ENABLE_* macros to default (0)
#define ENABLE_NHWC 0
#define ENABLE_HCWNC4 0
#define ENABLE_HCWNC8 0
#define ENABLE_NCHW 0

// Then update their values as per selection
#if _XF_HCWNC8_
#undef ENABLE_HCWNC8
#define ENABLE_HCWNC8 1
#elif _XF_HCWNC4_
#undef ENABLE_HCWNC4
#define ENABLE_HCWNC4 1
#elif _XF_NHWC_
#undef ENABLE_NHWC
#define ENABLE_NHWC 1
#elif _XF_NCHW_
#undef ENABLE_NCHW
#define ENABLE_NCHW 1
#endif

// Validation: At least one layout must be enabled
#if !(_XF_NHWC_ || _XF_NCHW_ || _XF_HCWNC4_ || _XF_HCWNC8_)
#error "No valid layout defined - enable at least one layout option"
#endif

//=============================================================================
// CONFIGURATION SUMMARY
//=============================================================================
// This summary shows the current compile-time configuration.
//
// ENABLED DATATYPES:
#if XF_FP32
#pragma message "  - FP32 (32-bit float) ENABLED"
#endif
#if XF_BF16
#pragma message "  - BF16 (16-bit bfloat) ENABLED"
#endif
#if XF_FP16
#pragma message "  - FP16 (16-bit float) ENABLED"
#endif
#if XF_INT8
#pragma message "  - INT8 (8-bit integer) ENABLED"
#endif

//
// ENABLED LAYOUTS:
#if _XF_NHWC_
#pragma message "  - NHWC (Height-Width-Channel) ENABLED"
#endif
#if _XF_NCHW_
#pragma message "  - NCHW (Channel-Height-Width) ENABLED"
#endif
#if _XF_HCWNC4_
#pragma message "  - HCWNC4 (4-channel aligned) ENABLED"
#endif
#if _XF_HCWNC8_
#pragma message "  - HCWNC8 (8-channel aligned) ENABLED"
#endif
//

// CHANNEL CONFIGURATION:
#if _XF_RGBA_
#pragma message "  - RGBA Mode: 4 channels"
#else
#pragma message "  - RGB Mode: 3 channels"
#endif

//
// INTERFACE SIZING (based on largest enabled datatype):
//   - SELECT_TYPE bitfield: (binary representation of enabled datatypes)
//   - SELECT_ORDER bitfield: (binary representation of enabled layouts)
//
// To modify configuration, edit the "USER CONFIGURATION" section above.
//=============================================================================

//=============================================================================
// TYPE DEFINITIONS PER LAYOUT
//=============================================================================
// Note: Type definitions are duplicated across layout sections because:
//   1. Each layout may need different IN_TYPE and OUT_TYPE configurations
//   2. The priority order (FP32 > BF16 > FP16 > INT8) determines which types
//      are used when multiple datatypes are enabled
//   3. This allows compile-time selection of the "active" datatype while
//      still synthesizing hardware support for all enabled datatypes
//   4. The largest datatype determines buffer/interface sizing (see above)
//
// The #elif chain below selects ONE active configuration per layout for
// compile-time type definitions, while SELECT_TYPE bitfield enables runtime
// validation of ALL enabled datatypes.
//=============================================================================

//***************NHWC******************
#if ENABLE_NHWC
#if ENABLE_FP32
#if _XF_RGBA_
#define CV_TYPE CV_32FC4
#define IN_TYPE XF_32FC4
#else
static constexpr int IN_TYPE = XF_32FC3;
#define CV_TYPE CV_32FC3
#endif
#define XF_T float
#define CV_PIX_TYPE float
#define TB_PIX_TYPE unsigned int
#elif ENABLE_BF16
#if _XF_RGBA_
#define CV_TYPE CV_16UC4
static constexpr int IN_TYPE = XF_16UC4;
#else
#define CV_TYPE CV_16UC3
static constexpr int IN_TYPE = XF_16UC3;
#endif
#define XF_T ap_float<16, 8>
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#elif ENABLE_FP16
#if _XF_RGBA_
#define CV_TYPE CV_16UC4
static constexpr int IN_TYPE = XF_16UC4;
#else
#define CV_TYPE CV_16UC3
static constexpr int IN_TYPE = XF_16UC3;
#endif
#define XF_T half
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#else // INT8
#if _XF_RGBA_
#define CV_TYPE CV_8UC4
static constexpr int IN_TYPE = XF_8UC4;
#else
static constexpr int IN_TYPE = XF_8UC3;
#define CV_TYPE CV_8UC3
#endif
#define XF_T unsigned char
#define CV_PIX_TYPE unsigned char
#define TB_PIX_TYPE unsigned char
#endif
static constexpr int OUT_TYPE = IN_TYPE;

//**************HCWNC4*****************
#elif ENABLE_HCWNC4
#if ENABLE_FP32 //
#define CV_TYPE CV_32FC4
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_32FC4;
#else
static constexpr int IN_TYPE = XF_32FC3;
#endif
static constexpr int OUT_TYPE = XF_32FC4;
#define XF_T float
#define CV_PIX_TYPE float
#define TB_PIX_TYPE unsigned int
#elif ENABLE_BF16 //
#define CV_TYPE CV_16UC4
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_16UC4;
#else
static constexpr int IN_TYPE = XF_16UC3;
#endif
static constexpr int OUT_TYPE = XF_16UC4;
#define XF_T ap_float<16, 8>
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#elif ENABLE_FP16
#define CV_TYPE CV_16UC4
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_16UC4;
#else
static constexpr int IN_TYPE = XF_16UC3;
#endif
#define XF_T half
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
static constexpr int OUT_TYPE = XF_16UC4;
#else // INT8
#define CV_TYPE CV_8UC4
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_8UC4;
#else
static constexpr int IN_TYPE = XF_8UC3;
#endif
static constexpr int OUT_TYPE = XF_8UC4;
#define XF_T unsigned char
#define CV_PIX_TYPE unsigned char
#define TB_PIX_TYPE unsigned char
#endif

//**************HCWNC8*****************
#elif ENABLE_HCWNC8
#if ENABLE_FP32 //
#define CV_TYPE CV_32FC(8)
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_32FC4;
#else
static constexpr int IN_TYPE = XF_32FC3;
#endif
static constexpr int OUT_TYPE = XF_32FC8;
#define XF_T float
#define CV_PIX_TYPE float
#define TB_PIX_TYPE unsigned int
#elif ENABLE_BF16 //
#define CV_TYPE CV_16UC(8)
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_16UC4;
#else
static constexpr int IN_TYPE = XF_16UC3;
#endif
static constexpr int OUT_TYPE = XF_16UC8;
#define XF_T ap_float<16, 8>
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#elif ENABLE_FP16
#define CV_TYPE CV_16UC(8)
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_16UC4;
#else
static constexpr int IN_TYPE = XF_16UC3;
#endif
static constexpr int OUT_TYPE = XF_16UC8;
#define XF_T half
#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE unsigned short
#else // INT8
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_8UC4;
#else
static constexpr int IN_TYPE = XF_8UC3;
#endif
static constexpr int OUT_TYPE = XF_8UC8;
#define CV_TYPE CV_8UC(8)
#define XF_T unsigned char
#define CV_PIX_TYPE unsigned char
#define TB_PIX_TYPE unsigned char
#endif

//************Default******************
#else
#define CV_TYPE CV_8UC3
static constexpr int OUT_TYPE = XF_8UC3;
#endif

//***************NCHW******************
#if ENABLE_FP32
#define OUT_TYPE_NCHW XF_32FC1
#define CV_TYPE_NCHW CV_32FC1
#if ENABLE_NCHW
#if _XF_RGBA_
#define IN_TYPE XF_32FC4
#else
static constexpr int IN_TYPE = XF_32FC3;
#endif
#endif
#elif ENABLE_BF16
#define CV_TYPE_NCHW CV_16UC1
static constexpr int OUT_TYPE_NCHW = XF_16UC1;
#if ENABLE_NCHW
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_16UC4;
#else
static constexpr int IN_TYPE = XF_16UC3;
#endif
#endif
#elif ENABLE_FP16
#define CV_TYPE_NCHW CV_16UC1
static constexpr int OUT_TYPE_NCHW = XF_16UC1;
#if ENABLE_NCHW
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_16UC4;
#else
static constexpr int IN_TYPE = XF_16UC3;
#endif
#endif
#else // INT8
#define CV_TYPE_NCHW CV_8UC1
static constexpr int OUT_TYPE_NCHW = XF_8UC1;
#if ENABLE_NCHW
#if _XF_RGBA_
static constexpr int IN_TYPE = XF_8UC4;
#else
static constexpr int IN_TYPE = XF_8UC3;
#endif
#endif
#endif

#if _XF_NCHW_
#if ENABLE_FP32
//#define XF_T float
//#define CV_PIX_TYPE float
#define TB_PIX_TYPE_NCHW unsigned int
#define TB_PIX_TYPE unsigned int
#elif ENABLE_BF16
//#define XF_T ap_float<16, 8>
//#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE_NCHW unsigned short
#define TB_PIX_TYPE unsigned short
#elif ENABLE_FP16
#define CV_TYPE_NCHW CV_16UC1
//#define XF_T half
//#define CV_PIX_TYPE cv::float16_t
#define TB_PIX_TYPE_NCHW unsigned short
#define TB_PIX_TYPE unsigned short
#else // INT8
#define CV_TYPE_NCHW CV_8UC1
//#define XF_T unsigned char
//#define CV_PIX_TYPE unsigned char
#define TB_PIX_TYPE_NCHW unsigned char
#define TB_PIX_TYPE unsigned char
#endif
#endif

//=============================================================================
// AXI STREAM INTERFACE WIDTH CALCULATION
//=============================================================================

// Calculates the bit width of AXI stream interfaces based on:
//   - Pixel data type (IN_TYPE, OUT_TYPE)
//   - Number of pixels processed per cycle (NPPCX)
//   - Byte alignment requirements (rounded up to multiple of 8 bits)
//
// Formula: width = pixels_per_cycle × bits_per_pixel, rounded to byte boundary
//=============================================================================

#define _DATA_WIDTH_(_T, _N) (XF_PIXELWIDTH(_T, _N) * XF_NPIXPERCYCLE(_N))
#define _BYTE_ALIGN_(_N) ((((_N) + 7) / 8) * 8) // Round up to next byte boundary
#define IN_DATA_WIDTH _DATA_WIDTH_(IN_TYPE, NPPCX)
#define OUT_DATA_WIDTH _DATA_WIDTH_(OUT_TYPE, NPPCX)
#define OUT_DATA_WIDTH_NCHW _DATA_WIDTH_(OUT_TYPE_NCHW, NPPCX)
#define AXI_WIDTH_IN _BYTE_ALIGN_(IN_DATA_WIDTH)
#define AXI_WIDTH_OUT _BYTE_ALIGN_(OUT_DATA_WIDTH)
#define AXI_WIDTH_OUT_NCHW _BYTE_ALIGN_(OUT_DATA_WIDTH_NCHW)

// Input/Output AXI video buses
typedef ap_axiu<AXI_WIDTH_IN, 1, 1, 1> InStrmBus_t;

// Input/Output AXI video stream
typedef hls::stream<InStrmBus_t> InStream;

//=============================================================================
// POWER-OF-2 CEILING MACRO (for AXI width alignment)
//=============================================================================
// AXI interfaces require power-of-2 data widths. This macro rounds up to the
// nearest power of 2 using compiler intrinsics for efficiency.
//
// Algorithm: Uses __builtin_clz (Count Leading Zeros) GCC/Clang intrinsic
//   - __builtin_clz(x) counts leading zero bits in a 32-bit unsigned integer
//   - For x=5 (binary: 00000000000000000000000000000101), clz=29
//   - 32 - clz gives position of highest set bit (log2_floor + 1)
//   - Using (n-1) then adding back via shift gives ceiling
//
// Example: n=37 → (37-1)=36 → clz(36)=26 → 32-26=6 → 2^6=64 (next power of 2)
// Example: n=32 → (32-1)=31 → clz(31)=27 → 32-27=5 → 2^5=32 (already power of 2)
// Example: n=1  → returns 1 (special case)
//

//=============================================================================
#define NEXT_POW2_CEIL_U32(n) (((n) <= 1u) ? 1u : (1u << (32 - __builtin_clz((n)-1u))))

#define NEXT_POW2_CEIL_U32_1(n) (((n) <= 1u) ? 1u : (1u << (32 - __builtin_clz((n)-1u))))

//=============================================================================
// AXI MEMORY-MAPPED (AXI-MM) INTERFACE CONFIGURATION
//=============================================================================

// OUTPUT_PTR_WIDTH: Pointer width for AXI-MM interface, must be power-of-2
// XF_CV_DEPTH: FIFO depth for internal xf::cv::Mat objects
//   - Positive values: Implements a hls::stream buffer
//   - -1: Implements a memory buffer
//=============================================================================

#define OUTPUT_PTR_WIDTH NEXT_POW2_CEIL_U32(AXI_WIDTH_OUT)
#define OUTPUT_PTR_WIDTH_NCHW NEXT_POW2_CEIL_U32_1(AXI_WIDTH_OUT_NCHW)

#define XF_CV_DEPTH_IN 2   // Input buffer depth (streaming for input)
#define XF_CV_DEPTH_OUT -1 // Output buffer depth (memory buffer for output)

//=============================================================================
// ACCELERATOR FUNCTION SIGNATURE
//=============================================================================
// Main layout formatter accelerator function with runtime-selectable options
//
// Parameters:
//   img_inp:        Input AXI4-Stream (video stream interface)
//   img_out:        Output AXI-MM pointer for NHWC/HCWNC4/HCWNC8 layouts
//   img_out1-4:     Output AXI-MM pointers for NCHW layout (per-channel planes)
//   in_img_height:  Input image height in pixels
//   in_img_width:   Input image width in pixels
//   layout_format:  Runtime layout selection (must be already compile-time enabled)
//                   Values: layout_format::{XF_NHWC, XF_NCHW, XF_HCWNC4, XF_HCWNC8}
//   datatype:       Runtime datatype selection (must be already compile-time enabled)
//                   Values: data_types::{INT8, FP16, BF16, FP32}
//   out_channels:   Number of output channels (1, 3, or 4)
//
// Note: Runtime parameters are validated against SELECT_TYPE and SELECT_ORDER
//       bitfields. Mismatches will trigger assertions in debug builds.
//=============================================================================

void layout_formatter_accel(InStream& img_inp,
                            ap_uint<OUTPUT_PTR_WIDTH>* img_out,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* img_out1,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* img_out2,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* img_out3,
                            ap_uint<OUTPUT_PTR_WIDTH_NCHW>* img_out4,
                            int in_img_height,
                            int in_img_width,
                            int layout_format,
                            int datatype,
                            int out_channels);
#endif
