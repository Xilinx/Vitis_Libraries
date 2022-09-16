// Copyright 2014 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Near-lossless image preprocessing adjusts pixel values to help
// compressibility with a guarantee of maximum deviation between original and
// resulting pixel values.
//
// Author: Jyrki Alakuijala (jyrki@google.com)
// Converted to C by Aleksander Kramarz (akramarz@google.com)

#include <stdlib.h>
#include <stdio.h>

#include "../dsp/lossless.h"
#include "../utils/utils.h"
#include "../utils/profiling.h"
#include "./vp8enci.h"
#include "../../host/create_kernel.h"
// #include "./kernel/oclHelper.h"

#define MIN_DIM_FOR_NEAR_LOSSLESS 64
#define MAX_LIMIT_BITS 5

// Computes quantized pixel value and distance from original value.
static void GetValAndDistance(int a, int initial, int bits, int* const val, int* const distance) {
    const int mask = ~((1 << bits) - 1);
    *val = (initial & mask) | (initial >> (8 - bits));
    *distance = 2 * abs(a - *val);
}

// Clamps the value to range [0, 255].
static int Clamp8b(int val) {
    const int min_val = 0;
    const int max_val = 0xff;
    return (val < min_val) ? min_val : (val > max_val) ? max_val : val;
}

// Quantizes values {a, a+(1<<bits), a-(1<<bits)} and returns the nearest one.
static int FindClosestDiscretized(int a, int bits) {
    int best_val = a, i;
    int min_distance = 256;

    for (i = -1; i <= 1; ++i) {
        int candidate, distance;
        const int val = Clamp8b(a + i * (1 << bits));
        GetValAndDistance(a, val, bits, &candidate, &distance);
        if (i != 0) {
            ++distance;
        }
        // Smallest distance but favor i == 0 over i == -1 and i == 1
        // since that keeps the overall intensity more constant in the
        // images.
        if (distance < min_distance) {
            min_distance = distance;
            best_val = candidate;
        }
    }
    return best_val;
}

// Applies FindClosestDiscretized to all channels of pixel.
static uint32_t ClosestDiscretizedArgb(uint32_t a, int bits) {
    return (FindClosestDiscretized(a >> 24, bits) << 24) | (FindClosestDiscretized((a >> 16) & 0xff, bits) << 16) |
           (FindClosestDiscretized((a >> 8) & 0xff, bits) << 8) | (FindClosestDiscretized(a & 0xff, bits));
}

// Checks if distance between corresponding channel values of pixels a and b
// is within the given limit.
static int IsNear(uint32_t a, uint32_t b, int limit) {
    int k;
    for (k = 0; k < 4; ++k) {
        const int delta = (int)((a >> (k * 8)) & 0xff) - (int)((b >> (k * 8)) & 0xff);
        if (delta >= limit || delta <= -limit) {
            return 0;
        }
    }
    return 1;
}

static int IsSmooth(
    const uint32_t* const prev_row, const uint32_t* const curr_row, const uint32_t* const next_row, int ix, int limit) {
    // Check that all pixels in 4-connected neighborhood are smooth.
    return (IsNear(curr_row[ix], curr_row[ix - 1], limit) && IsNear(curr_row[ix], curr_row[ix + 1], limit) &&
            IsNear(curr_row[ix], prev_row[ix], limit) && IsNear(curr_row[ix], next_row[ix], limit));
}

// Adjusts pixel values of image with given maximum error.
static void NearLossless(int xsize, int ysize, uint32_t* argb, int limit_bits, uint32_t* copy_buffer) {
    int x, y;
    const int limit = 1 << limit_bits;
    uint32_t* prev_row = copy_buffer;
    uint32_t* curr_row = prev_row + xsize;
    uint32_t* next_row = curr_row + xsize;
    memcpy(copy_buffer, argb, xsize * 2 * sizeof(argb[0]));

    for (y = 1; y < ysize - 1; ++y) {
        uint32_t* const curr_argb_row = argb + y * xsize;
        uint32_t* const next_argb_row = curr_argb_row + xsize;
        memcpy(next_row, next_argb_row, xsize * sizeof(argb[0]));
        for (x = 1; x < xsize - 1; ++x) {
            if (!IsSmooth(prev_row, curr_row, next_row, x, limit)) {
                curr_argb_row[x] = ClosestDiscretizedArgb(curr_row[x], limit_bits);
            }
        }
        {
            // Three-way swap.
            uint32_t* const temp = prev_row;
            prev_row = curr_row;
            curr_row = next_row;
            next_row = temp;
        }
    }
}

static int QualityToLimitBits(int quality) {
    // quality mapping:
    //  0..19 -> 5
    //  0..39 -> 4
    //  0..59 -> 3
    //  0..79 -> 2
    //  0..99 -> 1
    //  100   -> 0
    return MAX_LIMIT_BITS - quality / 20;
}

int VP8ApplyNearLossless(int xsize, int ysize, uint32_t* argb, int quality) {
    int i;
    uint32_t* const copy_buffer = (uint32_t*)WebPSafeMalloc(xsize * 3, sizeof(*copy_buffer));
    const int limit_bits = QualityToLimitBits(quality);
    StopProfilingWatch stop_watch;
    StartProfiling(&stop_watch);
    assert(argb != NULL);
    assert(limit_bits >= 0);
    assert(limit_bits <= MAX_LIMIT_BITS);
    if (copy_buffer == NULL) {
        StopProfiling(&stop_watch, &timeVP8ApplyNearLossless, &countVP8ApplyNearLossless);
        return 0;
    }
    // For small icon images, don't attempt to apply near-lossless compression.
    if (xsize < MIN_DIM_FOR_NEAR_LOSSLESS && ysize < MIN_DIM_FOR_NEAR_LOSSLESS) {
        WebPSafeFree(copy_buffer);
        StopProfiling(&stop_watch, &timeVP8ApplyNearLossless, &countVP8ApplyNearLossless);
        return 1;
    }

    for (i = limit_bits; i != 0; --i) {
        NearLossless(xsize, ysize, argb, i, copy_buffer);
    }

    WebPSafeFree(copy_buffer);
    StopProfiling(&stop_watch, &timeVP8ApplyNearLossless, &countVP8ApplyNearLossless);
    return 1;
}

int VP8ApplyNearLosslessOcl(int xsize, int ysize, uint32_t* argb, int quality) {
    const int limit_bits = QualityToLimitBits(quality);
    int status = 1;
    int arg = 1;

    StopProfilingWatch stop_watch;
    StartProfiling(&stop_watch);
    assert(argb != NULL);
    assert(limit_bits >= 0);
    assert(limit_bits <= MAX_LIMIT_BITS);

    // For small icon images, don't attempt to apply near-lossless compression.
    if (xsize < MIN_DIM_FOR_NEAR_LOSSLESS && ysize < MIN_DIM_FOR_NEAR_LOSSLESS) {
        StopProfiling(&stop_watch, &timeVP8ApplyNearLosslessOcl, &countVP8ApplyNearLosslessOcl);
        return 1;
    }

    const int argb_size = xsize * ysize * sizeof(uint32_t);
    int device_width;
    int global_width;
    int global_height;

    size_t globalSize[2];
    size_t localSize[2];

#ifdef HANDLE_MULTI_PIXELS_PER_ITEM
    global_width = RoundUp(xsize - PADDING_SIZE, GRX_SIZE * PIXELS_PER_ITEM);
    global_height = RoundUp(ysize - PADDING_SIZE, GRY_SIZE);
#elif defined USE_VECTOR
    if (xsize > IMAGE_4K) {
        device_width = RoundUp(xsize, VECTOR_GRX_SIZE_4K * VECTOR_LENGTH);
        global_width = RoundUp(xsize - VECTOR_WIDTH_PADDING, VECTOR_GRX_SIZE_4K * VECTOR_LENGTH);
        global_height = RoundUp(ysize - PADDING_SIZE, VECTOR_GRY_SIZE_4K);
    } else {
        device_width = RoundUp(xsize, VECTOR_GRX_SIZE * VECTOR_LENGTH);
        global_width = RoundUp(xsize - VECTOR_WIDTH_PADDING, VECTOR_GRX_SIZE * VECTOR_LENGTH);
        global_height = RoundUp(ysize - PADDING_SIZE, VECTOR_GRY_SIZE);
    }
#else
    global_width = RoundUp(xsize - PADDING_SIZE, GRX_SIZE);
    global_height = RoundUp(ysize - PADDING_SIZE, GRY_SIZE);
#endif

#ifdef HANDLE_MULTI_PIXELS_PER_ITEM
    globalSize[0] = global_width / PIXELS_PER_ITEM;
    globalSize[1] = global_height;
    localSize[0] = GRX_SIZE;
    localSize[1] = GRY_SIZE;
#elif defined USE_VECTOR
    if (xsize > IMAGE_4K) {
        globalSize[0] = global_width / VECTOR_LENGTH;
        globalSize[1] = global_height;
        localSize[0] = VECTOR_GRX_SIZE_4K;
        localSize[1] = VECTOR_GRY_SIZE_4K;
    } else {
        globalSize[0] = global_width / VECTOR_LENGTH;
        globalSize[1] = global_height;
        localSize[0] = VECTOR_GRX_SIZE;
        localSize[1] = VECTOR_GRY_SIZE;
    }
#else
    globalSize[0] = global_width;
    globalSize[1] = global_height;
    localSize[0] = GRX_SIZE;
    localSize[1] = GRY_SIZE;
#endif

    cl_int err;

    err = clEnqueueWriteBuffer(hardware.mQueue, nearpara.input_argb, CL_TRUE, 0, argb_size, argb, 0, NULL, NULL);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    err = clEnqueueWriteBuffer(hardware.mQueue, nearpara.output_argb, CL_TRUE, 0, argb_size, argb, 0, NULL, NULL);
    if (CL_SUCCESS != err) {
        fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
        status = -1;
        goto Err;
    }

    for (int k = limit_bits; k != 0; --k) {
        nearpara.limitbits = k;
        err = clSetKernelArg(nearlossless.mKernel, 7, sizeof(int), &(nearpara.limitbits));
        if (err != CL_SUCCESS) {
            fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            status = -1;
            goto Err;
        }

        if (k % 2 == limit_bits % 2) {
            err = clSetKernelArg(nearlossless.mKernel, 0, sizeof(cl_mem), &(nearpara.input_argb));
            if (err != CL_SUCCESS) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                status = -1;
                goto Err;
            }

            err = clSetKernelArg(nearlossless.mKernel, 1, sizeof(cl_mem), &(nearpara.output_argb));
            if (err != CL_SUCCESS) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                status = -1;
                goto Err;
            }
        } else {
            err = clSetKernelArg(nearlossless.mKernel, 0, sizeof(cl_mem), &(nearpara.output_argb));
            if (err != CL_SUCCESS) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                status = -1;
                goto Err;
            }

            err = clSetKernelArg(nearlossless.mKernel, 1, sizeof(cl_mem), &(nearpara.input_argb));
            if (err != CL_SUCCESS) {
                fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
                status = -1;
                goto Err;
            }
        }

        err = clEnqueueNDRangeKernel(hardware.mQueue, nearlossless.mKernel, 2, 0, globalSize, localSize, 0, NULL, NULL);
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
    }

    if (limit_bits % 2 == 1) {
        err = clEnqueueReadBuffer(hardware.mQueue, nearpara.output_argb, CL_TRUE, 0, argb_size, argb, 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            status = -1;
            goto Err;
        }
    } else {
        err = clEnqueueReadBuffer(hardware.mQueue, nearpara.input_argb, CL_TRUE, 0, argb_size, argb, 0, NULL, NULL);
        if (CL_SUCCESS != err) {
            fprintf(stderr, "%s %d %s\n", __func__, __LINE__, oclErrorCode(err));
            status = -1;
            goto Err;
        }
    }

Err:
    releaseKernel(nearlossless);
    clReleaseMemObject(nearpara.input_argb);
    clReleaseMemObject(nearpara.output_argb);

    StopProfiling(&stop_watch, &timeVP8ApplyNearLosslessOcl, &countVP8ApplyNearLosslessOcl);
    return status;
}
