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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "./create_kernel.h"
#include "../src/enc/vp8enci.h"
#include "../src/dec/common.h"
#include "../src/utils/utils.h"

#include "vp8_hls_syn.h"
#include "vp8_AsyncConfig.h"

#include "../src/utils/profiling.h"

//#include <CL/cl_ext.h>//before 17.3
#include <CL/cl_ext_xilinx.h>

NearLosslessPara nearpara;
ResidualPara residualpara;
AnalyzePara analyzepara;
EncLoopPara enclooppara;
EncLoopPara* encloopparaAsync;

oclHardware hardware;
oclSoftware software;

oclKernelInfo nearlossless;
oclKernelInfo residualimage;
oclKernelInfo analyze;

oclKernelInfo encloop;

template <typename T>
T* aligned_allocator(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
};

uint32_t RoundUp(uint32_t value, uint32_t mutiple) {
    uint32_t remain_size = value % mutiple;
    if (remain_size != 0) {
        value += (mutiple - remain_size);
    }
    return value;
}

void GenNearlosslessInfo(int xsize, int ysize, int* device_size) {
    int device_width;
    int global_width;
    int device_height;
#ifdef HANDLE_MULTI_PIXELS_PER_ITEM
    device_width = RoundUp(xsize, GRX_SIZE * PIXELS_PER_ITEM);
    global_width = RoundUp(xsize - PADDING_SIZE, GRX_SIZE * PIXELS_PER_ITEM);
    device_height = RoundUp(ysize, GRY_SIZE);
#elif defined USE_VECTOR
    if (xsize > IMAGE_4K) {
        device_width = RoundUp(xsize, VECTOR_GRX_SIZE_4K * VECTOR_LENGTH);
        global_width = RoundUp(xsize - VECTOR_WIDTH_PADDING, VECTOR_GRX_SIZE_4K * VECTOR_LENGTH);
        device_height = RoundUp(ysize, VECTOR_GRY_SIZE_4K);
    } else {
        device_width = RoundUp(xsize, VECTOR_GRX_SIZE * VECTOR_LENGTH);
        global_width = RoundUp(xsize - VECTOR_WIDTH_PADDING, VECTOR_GRX_SIZE * VECTOR_LENGTH);
        device_height = RoundUp(ysize, VECTOR_GRY_SIZE);
    }
#else
    device_width = RoundUp(xsize, GRX_SIZE);
    global_width = RoundUp(xsize - PADDING_SIZE, GRX_SIZE);
    device_height = RoundUp(ysize, GRY_SIZE);
#endif
    *device_size = device_width * (device_height + PADDING_SIZE) * sizeof(uint32_t);

    nearpara.width = xsize;
    nearpara.height = ysize;
#ifdef HANDLE_MULTI_PIXELS_PER_ITEM
    nearpara.lwidth = GRX_SIZE * PIXELS_PER_ITEM + PADDING_SIZE;
    nearpara.edgewidth = GRX_SIZE * PIXELS_PER_ITEM - (global_width - xsize);
#else
    nearpara.lwidth = GRX_SIZE + PADDING_SIZE;
    nearpara.edgewidth = GRX_SIZE - (global_width - xsize);
#endif
    nearpara.lheight = GRY_SIZE + PADDING_SIZE;
}

int SetNearlosslessArg(int device_size) {
    int arg = 2;
    int status = 0;
    cl_int err;

    nearpara.input_argb =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, device_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    nearpara.output_argb =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, device_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    // set args

    err = clSetKernelArg(nearlossless.mKernel, arg++, sizeof(int), &(nearpara.width));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(nearlossless.mKernel, arg++, sizeof(int), &(nearpara.height));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(nearlossless.mKernel, arg++, sizeof(int), &(nearpara.lwidth));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(nearlossless.mKernel, arg++, sizeof(int), &(nearpara.lheight));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(nearlossless.mKernel, arg++, sizeof(int), &(nearpara.edgewidth));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clFinish(hardware.mQueue);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    return status;

Err:
    releaseKernel(nearlossless);
    releaseSoftware(software);
    clReleaseMemObject(nearpara.input_argb);
    clReleaseMemObject(nearpara.output_argb);
    releaseHardware(hardware);

    return status;
}

void GenResidualImageInfo(int width, int height, int* frame_size2) {
    const int kNumPredModes = 14;

    residualpara.width = width;
    residualpara.height = height;
    residualpara.group_width = GRX_SIZE_RESIDUAL;
    residualpara.group_height = GRY_SIZE_RESIDUAL;
    residualpara.residual_size = ((width + residualpara.group_width - 1) / residualpara.group_width) *
                                 residualpara.group_width *
                                 ((height + residualpara.group_height - 1) / residualpara.group_height) *
                                 residualpara.group_height * kNumPredModes * sizeof(uint32_t);
    *frame_size2 = (residualpara.residual_size / kNumPredModes) + (width + 1) * sizeof(uint32_t);
}

int SetResidualImageArg(int residual_size, int frame_size2) {
    int arg = 0;
    int result = 0;
    cl_int status;

    residualpara.buffer_argb =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY, frame_size2, NULL, &status);
    if (CL_SUCCESS != status) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(status));
        result = -1;
        goto Error;
    }

    residualpara.buffer_residual =
        clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY /* | CL_MEM_ALLOC_HOST_PTR*/,
                       residual_size, NULL, &status);
    if (CL_SUCCESS != status) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(status));
        result = -1;
        goto Error;
    }

    status = clSetKernelArg(residualimage.mKernel, arg++, sizeof(int), &(residualpara.width));
    if (status != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(status));
        result = -1;
        goto Error;
    }

    status = clSetKernelArg(residualimage.mKernel, arg++, sizeof(int), &(residualpara.group_height));
    if (status != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(status));
        result = -1;
        goto Error;
    }

    status = clSetKernelArg(residualimage.mKernel, arg++, sizeof(cl_mem), &(residualpara.buffer_argb));
    if (status != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(status));
        result = -1;
        goto Error;
    }

    status = clSetKernelArg(residualimage.mKernel, arg++, sizeof(cl_mem), &(residualpara.buffer_residual));
    if (status != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(status));
        result = -1;
        goto Error;
    }

    return result;

Error:
    releaseKernel(residualimage);
    releaseSoftware(software);
    clReleaseMemObject(residualpara.buffer_argb);
    clReleaseMemObject(residualpara.buffer_residual);
    releaseHardware(hardware);

    return result;
}

int SetAnalyzeArg(int xsize, int ysize) {
    int arg = 0;
    int status = 0;
    cl_int err;

    AnalyzeInputInfo input_info;
    const int mb_w = (xsize + 15) >> 4;
    const int mb_h = (ysize + 15) >> 4;
    const int preds_w = 4 * mb_w + 1;
    const int preds_h = 4 * mb_h + 1;
    const int y_stride = xsize;
    const int uv_width = (xsize + 1) >> 1;
    const int uv_height = (ysize + 1) >> 1;
    const int uv_stride = uv_width;
    const int expand_yheight = RoundUp(ysize, 16);
    const int expand_uvheight = RoundUp(uv_height, 8);

    int mb_size;
    int preds_size;
    int nz_size;
    int info_size;
    int output_size;
    int alphas_size;
    uint64_t y_size;
    uint64_t u_size;
    uint64_t v_size;

    int expand_y_size = 0;
    int expand_uv_size = 0;
    if (expand_yheight > ysize) {
        expand_y_size = (expand_yheight - ysize) * xsize;
    }

    if (expand_uvheight > uv_height) {
        expand_uv_size = (expand_uvheight - uv_height) * uv_width;
    }

    input_info.width = xsize;
    input_info.height = ysize;
    input_info.mb_w = mb_w;
    input_info.mb_h = mb_h;
    input_info.y_stride = y_stride;
    input_info.uv_stride = uv_stride;
    input_info.preds_w = preds_w;
    input_info.top_stride = mb_w * 16;

    mb_size = mb_w * mb_h * sizeof(uint8_t);
    preds_size = preds_w * preds_h * sizeof(uint8_t);
    nz_size = (mb_w + 1) * sizeof(uint32_t) + WEBP_ALIGN_CST;
    y_size = (uint64_t)y_stride * ysize;
    u_size = (uint64_t)uv_stride * uv_height;
    v_size = (uint64_t)uv_stride * uv_height;
    info_size = sizeof(input_info);
    // output_size = sizeof(AnalyzeOutput);
    output_size = mb_h * sizeof(int);
    alphas_size = mb_h * (MAX_ALPHA + 1) * sizeof(int);

    input_info.mb_size = mb_size;
    input_info.preds_size = preds_size;
    input_info.nz_size = nz_size;
    input_info.y_size = y_size;
    input_info.u_size = u_size;
    input_info.v_size = v_size;

    analyzepara.mb_info =
        clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, 3 * mb_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    analyzepara.preds =
        clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, preds_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    analyzepara.y =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, y_size + expand_y_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    analyzepara.u = clCreateBuffer(hardware.mContext, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, u_size + expand_uv_size,
                                   NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    analyzepara.v = clCreateBuffer(hardware.mContext, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, v_size + expand_uv_size,
                                   NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    analyzepara.output_alpha =
        clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, output_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    // analyzepara.output_alpha = clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY,
    //                                           sizeof(cl_int), NULL, &err);
    // if(CL_SUCCESS != err) {
    //   fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
    //   status = -1;
    //   goto Err;
    // }

    analyzepara.output_uvalpha =
        clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, output_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    // analyzepara.output_uvalpha = clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY,
    //                                             sizeof(cl_int), NULL, &err);
    // if(CL_SUCCESS != err) {
    //   fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
    //   status = -1;
    //   goto Err;
    // }

    analyzepara.alphas =
        clCreateBuffer(hardware.mContext, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, alphas_size, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.y));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.u));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.v));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.mb_info));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.preds));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.output_alpha));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.output_uvalpha));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(cl_mem), &(analyzepara.alphas));
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clSetKernelArg(analyze.mKernel, arg++, sizeof(AnalyzeInputInfo), &input_info);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    return status;

Err:
    releaseKernel(analyze);
    releaseSoftware(software);
    clReleaseMemObject(analyzepara.mb_info);
    clReleaseMemObject(analyzepara.preds);
    clReleaseMemObject(analyzepara.y);
    clReleaseMemObject(analyzepara.u);
    clReleaseMemObject(analyzepara.v);
    // clReleaseMemObject(analyzepara.output_data);
    clReleaseMemObject(analyzepara.output_alpha);
    clReleaseMemObject(analyzepara.output_uvalpha);
    releaseHardware(hardware);

    return status;
}

int SetEncLoopArg(int xsize, int ysize) {
    int arg = 0;
    int status = 0;
    cl_int err;

    StopProfilingWatch watch;
    double watch_time;
    int watch_count;

    /* const int mb_w = (xsize + 15) >> 4; */
    /* const int mb_h = (ysize + 15) >> 4; */
    /* const int preds_w = 4 * mb_w + 1; */
    /* const int preds_h = 4 * mb_h + 1; */

    /* const int y_width = xsize; */
    /* const int y_height = ysize; */

    /* const int uv_width = (xsize + 1) >> 1; */
    /* const int uv_height = (ysize + 1) >> 1; */

    /* const int y_stride = y_width; */
    /* const int uv_stride = uv_width; */

    /* const int expand_yheight = RoundUp(ysize, 16); */
    /* const int expand_uvheight = RoundUp(uv_height, 8); */

    /* uint64_t y_size = 0; */
    /* uint64_t uv_size = 0; */

    /* int mb_size = 0; */
    /* int preds_size = 0; */
    /* int nz_size = 0; */
    /* int top_data_size = 0; */
    /* int quant_matrix_size = 0; */
    /* int coeffs_size = 0; */
    /* int stats_size = 0; */
    /* int level_cost_size = 0; */
    /* int bw_buf_size = 0; */
    /* int sse_size = 0; */
    /* int block_count_size = 0; */
    /* int extra_info_size = 0; */
    /* int max_edge_size = 0; */
    /* int bit_count_size = 0; */
    /* int expand_y_size = 0; */
    /* int expand_uv_size = 0; */
    /* uint64_t output_size = 0; */
    /* int output_tokens_size = 0; */

    /* y_size = y_width * y_height * sizeof(uint8_t); */
    /* uv_size = uv_width * uv_height * sizeof(uint8_t); */
    /* output_size = MAX_NUM_MB_W * MAX_NUM_MB_H * 512 * sizeof(uint16_t); */

    /* mb_size = mb_w * mb_h * sizeof(uint8_t); */
    /* preds_size = preds_w * preds_h * sizeof(uint8_t) + preds_w + 1; */
    /* nz_size = (mb_w + 1 + 1) * sizeof(uint32_t)/\* + WEBP_ALIGN_CST*\/; */
    /* top_data_size = mb_w * 16 * sizeof(uint8_t); */
    /* quant_matrix_size = sizeof(VP8EncMatrix); */
    /* coeffs_size = NUM_CTX * NUM_PROBAS * NUM_TYPES * NUM_BANDS * sizeof(uint8_t); */
    /* stats_size = NUM_CTX * NUM_PROBAS * NUM_TYPES * NUM_BANDS * sizeof(uint32_t); */
    /* level_cost_size = NUM_CTX * NUM_PROBAS * NUM_TYPES * (MAX_VARIABLE_LEVEL + 1) * sizeof(uint16_t); */
    /* bw_buf_size = 408000 * sizeof(uint8_t); */
    /* sse_size = 4 * sizeof(uint64_t); */
    /* block_count_size = 3 * sizeof(int); */
    /* extra_info_size = mb_w * mb_h * sizeof(uint8_t); */
    /* max_edge_size = NUM_MB_SEGMENTS * sizeof(int); */
    /* bit_count_size = 4 * 3 * sizeof(uint64_t); */
    /* output_tokens_size = sizeof(uint16_t) * PAGE_COUNT * TOKENS_COUNT_PER_PAGE;  */

    /* if (expand_yheight > y_height) { */
    /* 	expand_y_size = (expand_yheight - y_height) * y_width; */
    /* } */

    /* if (expand_uvheight > uv_height) { */
    /* 	expand_uv_size = (expand_uvheight - uv_height) * uv_width; */
    /* } */

    /* StartProfiling(&watch);  */

    enclooppara.input =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, SIZE_P_INFO, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }
    printf("INFO: Buffer .input created \n");

    enclooppara.y =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, SIZE_P_YSRC, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    printf("INFO: Buffer .y created \n");

    enclooppara.u =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, SIZE_P_USRC, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    printf("INFO: Buffer .u created \n");

    enclooppara.v =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, SIZE_P_VSRC, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    printf("INFO: Buffer .v created \n");

    enclooppara.output =
        clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, SIZE_P_OUT, NULL, &err);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    printf("INFO: Buffer .output created \n");

    watch_time = 0.0;
    StopProfiling(&watch, &watch_time, &watch_count);
    printf("INFO: Create buffers finished. Computation time is %f (ms) \n\n", watch_time);

    return status;

Err:
    releaseKernel(encloop);
    releaseSoftware(software);
    clReleaseMemObject(enclooppara.input);
    clReleaseMemObject(enclooppara.y);
    clReleaseMemObject(enclooppara.u);
    clReleaseMemObject(enclooppara.v);
    clReleaseMemObject(enclooppara.output);
    releaseHardware(hardware);

    return status;
}

int CreateKernel(const char* xclbinpath) {
    using namespace xf::common::utils_sw;
    Logger logger(std::cout, std::cerr);

    StopProfilingWatch watch;
    double watch_time;
    int watch_count;

    fprintf(stderr, "INFO: CreateKernel start. \n");
    StartProfiling(&watch);

    int status = 0;
    int device_size;
    cl_int err;

    const cl_device_type deviceType = CL_DEVICE_TYPE_ACCELERATOR;

    // char target_device_name[1001] = "xilinx:aws-vu9p-f1:4ddr-xpr-2pr:4.0";
    // target_device_name = "xilinx_xil-accel-rd-ku115_4ddr-xpr_4_0"
    char target_device_name[101] = WEBPDSA;

    hardware = getOclHardware(deviceType, target_device_name);
    if (!hardware.mQueue) {
        fprintf(stderr, "%s %d getOclHardware\n", __func__, __LINE__);
        return -1;
    }

    strcpy(software.mFileName, xclbinpath);

    getOclSoftware(software, hardware);

    encloop.mKernelPred = new cl_kernel[NasyncDepth * Ninstances];
    encloop.mKernelAC = new cl_kernel[NasyncDepth * Ninstances];

    for (int i = 0; i < Ninstances; i++) {
        for (int j = 0; j < NasyncDepth; j++) {
            std::string namepred = "webp_IntraPredLoop2_NoOut_" + std::to_string(i + 1);
            std::string nameac = "webp_2_ArithmeticCoding_" + std::to_string(i + 1);

            encloop.mKernelPred[i * NasyncDepth + j] = clCreateKernel(software.mProgram, namepred.c_str(), NULL);
            logger.logCreateKernel(err);
            if (encloop.mKernelPred[i * NasyncDepth + j] == 0) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                status = -1;
                return status;
            }

            encloop.mKernelAC[i * NasyncDepth + j] = clCreateKernel(software.mProgram, nameac.c_str(), NULL);
            logger.logCreateKernel(err);
            if (encloop.mKernelAC[i * NasyncDepth + j] == 0) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                status = -1;
                return status;
            }
        };
    };

    watch_time = 0.0;
    StopProfiling(&watch, &watch_time, &watch_count);

    fprintf(stderr, "INFO: CreateKernel finished. Computation time is %f (ms) \n\n", watch_time);

    return status;
}

int ReleaseKernel() {
    fprintf(stderr, "INFO: Release Kernel. \n");

    for (int i = 0; i < Ninstances * NasyncDepth; i++) {
        clReleaseKernel(encloop.mKernelPred[i]);
        clReleaseKernel(encloop.mKernelAC[i]);
    }

    delete[] encloop.mKernelPred;
    delete[] encloop.mKernelAC;

    return 0;
}

int SetKernelArg(int xsize, int ysize) {
    int status = 0;
    // int frame_size2;

    // GenNearlosslessInfo(xsize, ysize, &device_size);
    // status = SetNearlosslessArg(device_size);

    // GenResidualImageInfo(xsize, ysize, &frame_size2);
    // status = SetResidualImageArg(residualpara.residual_size, frame_size2);

    // status = SetAnalyzeArg(xsize, ysize);

    status = SetEncLoopArg(xsize, ysize);

    // printf("INFO: SetKernelArg() finished \n\n");

    return status;
}

// Create device buffers
int CreateDeviceBuffers(const int Numbatch) {
    int status = 0;
    cl_int err;

    StopProfilingWatch watch;
    double watch_time;
    int watch_count;

    fprintf(stderr, "INFO: Create buffers started.\n");

    StartProfiling(&watch);

    encloopparaAsync = new EncLoopPara[Ninstances * NasyncDepth];

    for (int i = 0; i < Ninstances; i++) {
        for (int j = 0; j < NasyncDepth; j++) {
            cl_mem_ext_ptr_t bankmem_input;
            cl_mem_ext_ptr_t bankmem_y;
            cl_mem_ext_ptr_t bankmem_u;
            cl_mem_ext_ptr_t bankmem_v;
            cl_mem_ext_ptr_t bankmem_output;
            cl_mem_ext_ptr_t bankmem_prob;
            cl_mem_ext_ptr_t bankmem_bw;
            cl_mem_ext_ptr_t bankmem_ret;
            cl_mem_ext_ptr_t bankmem_pred;

            if (i == 0) {
                bankmem_input.flags = XCL_MEM_DDR_BANK3;
                bankmem_y.flags = XCL_MEM_DDR_BANK3;
                bankmem_u.flags = XCL_MEM_DDR_BANK3;
                bankmem_v.flags = XCL_MEM_DDR_BANK3;
                bankmem_output.flags = XCL_MEM_DDR_BANK3;
                bankmem_prob.flags = XCL_MEM_DDR_BANK3;
                bankmem_bw.flags = XCL_MEM_DDR_BANK3;
                bankmem_ret.flags = XCL_MEM_DDR_BANK3;
                bankmem_pred.flags = XCL_MEM_DDR_BANK3;
            } else if (i == 1) {
                bankmem_input.flags = XCL_MEM_DDR_BANK3;
                bankmem_y.flags = XCL_MEM_DDR_BANK3;
                bankmem_u.flags = XCL_MEM_DDR_BANK3;
                bankmem_v.flags = XCL_MEM_DDR_BANK3;
                bankmem_output.flags = XCL_MEM_DDR_BANK3;
                bankmem_prob.flags = XCL_MEM_DDR_BANK3;
                bankmem_bw.flags = XCL_MEM_DDR_BANK3;
                bankmem_ret.flags = XCL_MEM_DDR_BANK3;
                bankmem_pred.flags = XCL_MEM_DDR_BANK3;
            } else if (i == 2) {
                bankmem_input.flags = XCL_MEM_DDR_BANK0;
                bankmem_y.flags = XCL_MEM_DDR_BANK0;
                bankmem_u.flags = XCL_MEM_DDR_BANK0;
                bankmem_v.flags = XCL_MEM_DDR_BANK0;
                bankmem_output.flags = XCL_MEM_DDR_BANK0;
                bankmem_prob.flags = XCL_MEM_DDR_BANK0;
                bankmem_bw.flags = XCL_MEM_DDR_BANK0;
                bankmem_ret.flags = XCL_MEM_DDR_BANK0;
                bankmem_pred.flags = XCL_MEM_DDR_BANK0;
            } else if (i == 3) {
                bankmem_input.flags = XCL_MEM_DDR_BANK0;
                bankmem_y.flags = XCL_MEM_DDR_BANK0;
                bankmem_u.flags = XCL_MEM_DDR_BANK0;
                bankmem_v.flags = XCL_MEM_DDR_BANK0;
                bankmem_output.flags = XCL_MEM_DDR_BANK0;
                bankmem_prob.flags = XCL_MEM_DDR_BANK0;
                bankmem_bw.flags = XCL_MEM_DDR_BANK0;
                bankmem_ret.flags = XCL_MEM_DDR_BANK0;
                bankmem_pred.flags = XCL_MEM_DDR_BANK0;
            } else if (i == 4) {
                bankmem_input.flags = XCL_MEM_DDR_BANK1;
                bankmem_y.flags = XCL_MEM_DDR_BANK1;
                bankmem_u.flags = XCL_MEM_DDR_BANK1;
                bankmem_v.flags = XCL_MEM_DDR_BANK1;
                bankmem_output.flags = XCL_MEM_DDR_BANK1;
                bankmem_prob.flags = XCL_MEM_DDR_BANK1;
                bankmem_bw.flags = XCL_MEM_DDR_BANK1;
                bankmem_ret.flags = XCL_MEM_DDR_BANK1;
                bankmem_pred.flags = XCL_MEM_DDR_BANK1;
            } else if (i == 5) {
                bankmem_input.flags = XCL_MEM_DDR_BANK2;
                bankmem_y.flags = XCL_MEM_DDR_BANK2;
                bankmem_u.flags = XCL_MEM_DDR_BANK2;
                bankmem_v.flags = XCL_MEM_DDR_BANK2;
                bankmem_output.flags = XCL_MEM_DDR_BANK2;
                bankmem_prob.flags = XCL_MEM_DDR_BANK2;
                bankmem_bw.flags = XCL_MEM_DDR_BANK2;
                bankmem_ret.flags = XCL_MEM_DDR_BANK2;
                bankmem_pred.flags = XCL_MEM_DDR_BANK2;
            }

            bankmem_input.param = 0;
            bankmem_y.param = 0;
            bankmem_u.param = 0;
            bankmem_v.param = 0;
            bankmem_output.param = 0;
            bankmem_prob.param = 0;
            bankmem_bw.param = 0;
            bankmem_ret.param = 0;
            bankmem_pred.param = 0;

            const uint32_t offset_info = Get_Busoffset_info_32bits() * sizeof(uint32_t);
            encloopparaAsync[i * NasyncDepth + j].inputcpu = aligned_allocator<uint8_t>(offset_info * Numbatch);
            encloopparaAsync[i * NasyncDepth + j].ycpu = aligned_allocator<uint8_t>(SIZE32_MEM_YSRC * 4);
            encloopparaAsync[i * NasyncDepth + j].ucpu = aligned_allocator<uint8_t>(SIZE32_MEM_UVSRC * 4);
            encloopparaAsync[i * NasyncDepth + j].vcpu = aligned_allocator<uint8_t>(SIZE32_MEM_UVSRC * 4);
            encloopparaAsync[i * NasyncDepth + j].probcpu = aligned_allocator<uint8_t>(SIZE8_MEM_PROB * Numbatch);
            encloopparaAsync[i * NasyncDepth + j].bwcpu = aligned_allocator<uint8_t>(SIZE8_MEM_BW);
            encloopparaAsync[i * NasyncDepth + j].retcpu = aligned_allocator<uint8_t>(SIZE8_MEM_RET);
            encloopparaAsync[i * NasyncDepth + j].predcpu = aligned_allocator<uint8_t>(SIZE8_MEM_PRED);

            bankmem_input.obj = encloopparaAsync[i * NasyncDepth + j].inputcpu;
            bankmem_y.obj = encloopparaAsync[i * NasyncDepth + j].ycpu;
            bankmem_u.obj = encloopparaAsync[i * NasyncDepth + j].ucpu;
            bankmem_v.obj = encloopparaAsync[i * NasyncDepth + j].vcpu;
            bankmem_output.obj = NULL;
            bankmem_prob.obj = encloopparaAsync[i * NasyncDepth + j].probcpu;
            bankmem_bw.obj = encloopparaAsync[i * NasyncDepth + j].bwcpu;
            bankmem_ret.obj = encloopparaAsync[i * NasyncDepth + j].retcpu;
            bankmem_pred.obj = encloopparaAsync[i * NasyncDepth + j].predcpu;

            // input
            encloopparaAsync[i * NasyncDepth + j].input =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               offset_info * Numbatch, &bankmem_input, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }
            // fprintf(stderr, "INFO: Buffer .input created \n");

            // y
            encloopparaAsync[i * NasyncDepth + j].y =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE32_MEM_YSRC * 4, &bankmem_y, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }
            // fprintf(stderr, "INFO: Buffer .y created \n");

            // u
            encloopparaAsync[i * NasyncDepth + j].u =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE32_MEM_UVSRC * 4, &bankmem_u, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }

            // v
            encloopparaAsync[i * NasyncDepth + j].v =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE32_MEM_UVSRC * 4, &bankmem_v, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }
            // fprintf(stderr, "INFO: Buffer .v created \n");

            // output
            encloopparaAsync[i * NasyncDepth + j].output =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX, SIZE32_MEM_LEVEL * 4,
                               &bankmem_output, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }
            // fprintf(stderr, "INFO: Buffer .output created \n");

            // output_prob
            encloopparaAsync[i * NasyncDepth + j].output_prob =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE8_MEM_PROB * Numbatch, &bankmem_prob, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }

            // output_bw
            encloopparaAsync[i * NasyncDepth + j].output_bw =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE8_MEM_BW, &bankmem_bw, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }

            // output_ret
            encloopparaAsync[i * NasyncDepth + j].output_ret =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE8_MEM_RET, &bankmem_ret, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }

            // output_pred
            encloopparaAsync[i * NasyncDepth + j].output_pred =
                clCreateBuffer(hardware.mContext, CL_MEM_READ_WRITE | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                               SIZE8_MEM_PRED, &bankmem_pred, &err);
            if (CL_SUCCESS != err) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            }
            // fprintf(stderr, "INFO: Buffer .output_pred created \n");
        }
    }

    err = clFinish(hardware.mQueue);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
    }

    watch_time = 0.0;
    StopProfiling(&watch, &watch_time, &watch_count);
    fprintf(stderr, "INFO: Create buffers finished. Computation time is %f (ms) \n\n", watch_time);

    /* Err: */
    /*  releaseKernel(encloop); */
    /*  releaseSoftware(software); */
    /*  /\* clReleaseMemObject(encloopparaAsync.input); *\/ */
    /*  /\* clReleaseMemObject(encloopparaAsync.y); *\/ */
    /*  /\* clReleaseMemObject(encloopparaAsync.u); *\/ */
    /*  /\* clReleaseMemObject(encloopparaAsync.v); *\/ */
    /*  /\* clReleaseMemObject(encloopparaAsync.output);  *\/ */
    /*  releaseHardware(hardware); */

    return status;
};

// Release device buffers
int ReleaseDeviceBuffers() {
    for (int i = 0; i < Ninstances * NasyncDepth; i++) {
        clReleaseMemObject(encloopparaAsync[i].input);
        clReleaseMemObject(encloopparaAsync[i].y);
        clReleaseMemObject(encloopparaAsync[i].u);
        clReleaseMemObject(encloopparaAsync[i].v);
        clReleaseMemObject(encloopparaAsync[i].output);
        clReleaseMemObject(encloopparaAsync[i].output_prob);
        clReleaseMemObject(encloopparaAsync[i].output_pred);
        clReleaseMemObject(encloopparaAsync[i].output_bw);
        clReleaseMemObject(encloopparaAsync[i].output_ret);

        free(encloopparaAsync[i].inputcpu);
        free(encloopparaAsync[i].ycpu);
        free(encloopparaAsync[i].ucpu);
        free(encloopparaAsync[i].vcpu);
        free(encloopparaAsync[i].probcpu);
        free(encloopparaAsync[i].bwcpu);
        free(encloopparaAsync[i].predcpu);
        free(encloopparaAsync[i].retcpu);
    };

    delete[] encloopparaAsync;

    return 0;
}
