/**********

  Copyright (c) 2017, Xilinx, Inc.
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

**********/

#ifndef WEBP_CREAT_NEARLERNEL_H_
#define WEBP_CREAT_NEARLERNEL_H_

#include "../src/webp/types.h"
#include "../src/enc/kernel/oclHelper.h"
#include "../src/dec/common.h"
#include "xf_utils_sw/logger.hpp"

#include "vp8_hls_syn.h"

#ifdef __cplusplus
extern "C" {
#endif

// define macro for kernel nearlossless
#define GRX_SIZE 256 // Work group size for kernel nearlossless
#define GRY_SIZE 16  // Work group size for kernel nearlossless
#define PADDING_SIZE 4

// #define HANDLE_MULTI_PIXELS_PER_ITEM        // ON  : handle multi pixels per item
// OFF : handle one pixel per item
#define PIXELS_PER_ITEM 4

#define USE_VECTOR // ON  : The input para tyoe is uint16
#define VECTOR_GRX_SIZE 128
#define VECTOR_GRY_SIZE 16
#define VECTOR_WIDTH_PADDING 16
#define VECTOR_HEIGHT_PADDING 2
#define VECTOR_LENGTH 16

#define VECTOR_GRX_SIZE_4K 256
#define VECTOR_GRY_SIZE_4K 8
#define IMAGE_4K 2048

// define macro for kernel residualimage
#define GRX_SIZE_RESIDUAL 256 // Work group size for kernel residualimage
#define GRY_SIZE_RESIDUAL 16  // Work group size for kernel residualimage

#define MAX_ALPHA 255 // 8b of precision for susceptibilities.

#define ANALYZE_GRX_SIZE 240 // Support only under 4k image
#define ENCLOOP_GRX_SIZE 135 // Support only under 4k image

#define USE_C_KERNEL

typedef struct NearLosslessPara {
    cl_mem input_argb;
    cl_mem output_argb;
    cl_int width;
    cl_int height;
    cl_int lwidth;
    cl_int lheight;
    cl_int edgewidth;
    cl_int limitbits;
} NearLosslessPara;

typedef struct ResidualPara {
    cl_mem buffer_argb;
    cl_mem buffer_residual;
    cl_int width;
    cl_int height;
    cl_int exact;
    int group_width;
    int group_height;
    int residual_size;
} ResidualPara;

typedef struct AnalyzePara {
    cl_mem mb_info;
    cl_mem preds;
    cl_mem y;
    cl_mem u;
    cl_mem v;
    cl_mem output_data;
    cl_mem output_alpha;
    cl_mem output_uvalpha;
    cl_mem alphas;
    cl_int method;
} AnalyzePara;

typedef struct AnalyzeInputInfo {
    cl_int width;
    cl_int height;
    cl_int mb_w;
    cl_int mb_h;
    cl_int y_stride;
    cl_int uv_stride;
    cl_int preds_w;
    cl_int top_stride;
    size_t mb_size;
    size_t preds_size;
    size_t nz_size;
    uint64_t y_size;
    uint64_t u_size;
    uint64_t v_size;
} AnalyzeInputInfo;

typedef struct AnalyzeOutput {
    int alpha;
    int uv_alpha;
} AnalyzeOutput;

typedef struct EncLoopPara {
    cl_mem input;
    cl_mem y;       // input/output width * height
    cl_mem u;       // input/output (width + 1) / 2 * (height + 1) / 2
    cl_mem v;       // input/output (width + 1) / 2 * (height + 1) / 2
    cl_mem mb_info; // No longer used:// input/output 4 * ((width + 15) / 16) * ((height + 15) / 16)
    cl_mem preds;   // No longer used:// input/output (4 * ((width + 15) / 16) + 1) * (4 * ((height + 15) / 16) + 1)
    cl_mem nz;      // No longer used:// output (((width + 15) / 16) + 1) * 4 + 31
    cl_mem y_top;   // No longer used:// output ((width + 15) / 16)*16
    cl_mem uv_top;  // No longer used:// output ((width + 15) / 16)*16
    cl_mem quant_matrix; // No longer used:// input 3 * (96 * 3 + 192 * 2) VP8Matrix
    cl_mem coeffs;       // No longer used:// input 4 * 8 * 3 * 11
    cl_mem stats;        // No longer used:// No longer used:// input 4 * 4 * 8 * 3 * 11
    cl_mem level_cost;   // No longer used:// input 2 * 4 * 8 * 3 * 68
    cl_mem segment;      // No longer used:// input
    cl_mem bw_buf;       // No longer used:// output 1 * 408000B
    cl_mem sse;          // No longer used:// output 4 * 8
    cl_mem block_count;  // No longer used:// output 3 * 4
    cl_mem extra_info;   // No longer used:// output ((width + 15) / 16) * ((height + 15) / 16)
    cl_mem max_edge;     // No longer used:// output 4 * 5
    cl_mem bit_count;    // No longer used:// output 8 * 4 * 3
    cl_mem sse_count;    // No longer used:
    cl_mem output;       // Output of new kernel-1, used by kernel-2
    cl_mem output_prob;  // Output of new kernel-1, used for probability table passed to kernel-2, also used for
                         // enc->prob_.coeff_
    cl_mem output_bw;    // Output of kernel-2, used for AC
    cl_mem output_ret;  // Output of kernel-2, used for propagating return-value from Intra-prediction of kernel-1 hided
                        // in pout_level
    cl_mem output_pred; // Output of kernel-2 used for propagating return-value from  Intra-prediction of kernel-1 hided
                        // in pout_level
    cl_mem output_data; // No longer used: output
    cl_mem output_tokens; // No longer used:uint16_t tokens_[PAGE_COUNT * TOKENS_COUNT_PER_PAGE];

    // AllPicInfo* inputcpu;
    uint8_t* inputcpu;
    uint8_t* ycpu;
    uint8_t* ucpu;
    uint8_t* vcpu;
    uint8_t* probcpu;
    uint8_t* predcpu;
    uint8_t* bwcpu;
    uint8_t* retcpu;

    cl_mem ysub;
    cl_mem usub;
    cl_mem vsub;
};

typedef struct EncloopInputData {
    cl_int width;
    cl_int height;
    cl_int filter_sharpness;
    cl_int show_compressed;
    cl_int extra_info_type;
    cl_int stats_add;
    cl_int simple;
    cl_int num_parts;
    cl_int max_i4_header_bits;
    cl_int lf_stats_status;
    cl_int use_skip_proba;
    cl_int method;
    cl_int rd_opt;
} EncloopInputData;

typedef struct EncloopSegmentData {
    int quant[NUM_MB_SEGMENTS];
    int fstrength[NUM_MB_SEGMENTS];

    int max_edge[NUM_MB_SEGMENTS];

    int min_disto[NUM_MB_SEGMENTS];
    int lambda_i16[NUM_MB_SEGMENTS];
    int lambda_i4[NUM_MB_SEGMENTS];
    int lambda_uv[NUM_MB_SEGMENTS];
    int lambda_mode[NUM_MB_SEGMENTS];
    int tlambda[NUM_MB_SEGMENTS];
    int lambda_trellis_i16[NUM_MB_SEGMENTS];
    int lambda_trellis_i4[NUM_MB_SEGMENTS];
    int lambda_trellis_uv[NUM_MB_SEGMENTS];
} EncloopSegmentData;

typedef struct VP8EncMatrix {
    uint32_t q_[16];       // quantizer steps
    uint32_t iq_[16];      // reciprocals, fixed point.
    uint32_t bias_[16];    // rounding bias
    uint32_t zthresh_[16]; // value below which a coefficient is zeroed
    uint32_t sharpen_[16]; // frequency boosters for slight sharpening
} VP8EncMatrix;

typedef struct EncLoopOutputData {
    int32_t range;
    int32_t value;
    int32_t run;
    int32_t nb_bits;
    int32_t pos;
    int32_t max_pos;
    int32_t error;
    int32_t max_i4_header_bits;
    // for token buf
    int32_t cur_page_;
    int32_t page_count_;
    int32_t left_;      // how many free tokens left before the page is full
    int32_t page_size_; // number of tokens per page
    int32_t error_;     // true in case of malloc error

    // uint64_t sse_count;
} EncLoopOutputData;

extern NearLosslessPara nearpara;
extern ResidualPara residualpara;
extern AnalyzePara analyzepara;
extern EncLoopPara enclooppara;
extern EncLoopPara* encloopparaAsync;
extern oclHardware hardware;
extern oclSoftware software;
extern oclKernelInfo nearlossless;
extern oclKernelInfo residualimage;
extern oclKernelInfo analyze;
extern oclKernelInfo encloop;

uint32_t RoundUp(uint32_t value, uint32_t mutiple);

// creat kernel
int SetKernelArg(int xsize, int ysize);

// creat kernel
int CreateKernel(const char* xclbinpath);
int ReleaseKernel();
int CreateDeviceBuffers(const int);
int ReleaseDeviceBuffers();

// set arguments for nearlossless kernel.
int SetNearlosslessArg(int device_size);

// Generate infomatin for nearlossless kernel.
void GenNearlosslessInfo(int xsize, int ysize, int* device_size);

// set arguments for residualimage kernel.
int SetResidualImageArg(int residual_size, int frame_size2);

// Generate infomatin for residualimage kernel.
void GenResidualImageInfo(int xsize, int ysize, int* frame_size2);

// set arguments for analyze kernel.
int SetAnalyzeArg(int xsize, int ysize);

// set arguments for encloop kernel.
int SetEncLoopArg(int xsize, int ysize);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WEBP_CREAT_NEARLERNEL_H_
