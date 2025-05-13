/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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

#ifndef __FFT_BUFS_H__
#define __FFT_BUFS_H__

#include "fft_com_inc.h"
#include "device_defs.h"

//------------------------------------------------------
// Inter-rank temporary storage buffers

#ifdef __X86SIM__
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_16_tmp1[FFT16_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_16_tmp2[FFT16_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_32_tmp1[FFT32_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_32_tmp2[FFT32_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_64_tmp1[FFT64_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_64_tmp2[FFT64_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_128_tmp1[FFT128_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_128_tmp2[FFT128_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_256_tmp1[FFT256_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_256_tmp2[FFT256_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_512_tmp1[FFT512_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_512_tmp2[FFT512_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_1024_tmp1[FFT1024_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_1024_tmp2[FFT1024_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_2048_tmp1[FFT2048_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_2048_tmp2[FFT2048_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_4096_tmp1[FFT4096_SIZE];
alignas(__ALIGN_BYTE_SIZE__) thread_local extern cint32_t fft_4096_tmp2[FFT4096_SIZE];
#else
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_16_tmp1[FFT16_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_16_tmp2[FFT16_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_32_tmp1[FFT32_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_32_tmp2[FFT32_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_64_tmp1[FFT64_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_64_tmp2[FFT64_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_128_tmp1[FFT128_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_128_tmp2[FFT128_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_256_tmp1[FFT256_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_256_tmp2[FFT256_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_512_tmp1[FFT512_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_512_tmp2[FFT512_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_1024_tmp1[FFT1024_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_1024_tmp2[FFT1024_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_2048_tmp1[FFT2048_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_2048_tmp2[FFT2048_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_4096_tmp1[FFT4096_SIZE];
alignas(__ALIGN_BYTE_SIZE__) extern cint32_t fft_4096_tmp2[FFT4096_SIZE];
#endif

// Twiddle tables
// Half-size integer tables
// This is an optimization possible because in a radix4 unit, the second rank butterflies use the same
// twiddle just 90 degrees (minus j) rotated. Minus J rotation is supported by hw, so only the first
// quadrant need be stores - the other quadrant can be extracted by minus j rotation.

//--------------------
// cint16 tables
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1_cint16_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2_cint16_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw4_cint16_half[2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw8_cint16_half[4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw16_cint16_half[FFT_16 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw32_cint16_half[FFT_32 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw64_cint16_half[FFT_64 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw128_cint16_half[FFT_128 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw256_cint16_half[FFT_256 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw512_cint16_half[FFT_512 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1024_cint16_half[FFT_1024 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2048_cint16_half[FFT_2048 / 2];

// Full (2 quadrant) integer tables
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1_cint16[FFT_1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2_cint16[FFT_2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw4_cint16[FFT_4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw8_cint16[FFT_8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw16_cint16[FFT_16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw32_cint16[FFT_32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw64_cint16[FFT_64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw128_cint16[FFT_128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw256_cint16[FFT_256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw512_cint16[FFT_512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1024_cint16[FFT_1024];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2048_cint16[FFT_2048];

#ifdef __FFT_TRUE_R4__
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_1_2[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_2_4[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_4_8[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_8_16[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_16_32[16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_32_64[32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_64_128[64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_128_256[128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_256_512[256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_512_1024[512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint16_r4_1024_2048[1024];
#endif

//--------------------
// cint15 tables
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1_cint15_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2_cint15_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw4_cint15_half[2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw8_cint15_half[4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw16_cint15_half[FFT_16 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw32_cint15_half[FFT_32 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw64_cint15_half[FFT_64 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw128_cint15_half[FFT_128 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw256_cint15_half[FFT_256 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw512_cint15_half[FFT_512 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1024_cint15_half[FFT_1024 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2048_cint15_half[FFT_2048 / 2];

// Full (2 quadrant) integer tables
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1_cint15[FFT_1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2_cint15[FFT_2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw4_cint15[FFT_4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw8_cint15[FFT_8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw16_cint15[FFT_16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw32_cint15[FFT_32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw64_cint15[FFT_64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw128_cint15[FFT_128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw256_cint15[FFT_256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw512_cint15[FFT_512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw1024_cint15[FFT_1024];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16_t fft_lut_tw2048_cint15[FFT_2048];

#ifdef __FFT_TRUE_R4__
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_1_2[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_2_4[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_4_8[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_8_16[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_16_32[16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_32_64[32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_64_128[64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_128_256[128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_256_512[256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_512_1024[512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint15_r4_1024_2048[1024];
#endif

//-------------------------------
// cint32 tables
// half (1 quadrant)
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1_cint32_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2_cint32_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw4_cint32_half[2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw8_cint32_half[4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw16_cint32_half[FFT_16 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw32_cint32_half[FFT_32 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw64_cint32_half[FFT_64 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw128_cint32_half[FFT_128 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw256_cint32_half[FFT_256 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw512_cint32_half[FFT_512 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1024_cint32_half[FFT_1024 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2048_cint32_half[FFT_2048 / 2];

// cint32 Full (2 quadrant) integer tables
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1_cint32[FFT_1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2_cint32[FFT_2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw4_cint32[FFT_4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw8_cint32[FFT_8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw16_cint32[FFT_16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw32_cint32[FFT_32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw64_cint32[FFT_64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw128_cint32[FFT_128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw256_cint32[FFT_256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw512_cint32[FFT_512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1024_cint32[FFT_1024];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2048_cint32[FFT_2048];

#ifdef __FFT_TRUE_R4__
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_1_2[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_2_4[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_4_8[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_8_16[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_16_32[16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_32_64[32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_64_128[64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_128_256[128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_256_512[256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_512_1024[512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint32_r4_1024_2048[1024];
#endif

//--------------
// cint31 tables
// half (1 quadrant)
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1_cint31_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2_cint31_half[1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw4_cint31_half[2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw8_cint31_half[4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw16_cint31_half[FFT_16 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw32_cint31_half[FFT_32 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw64_cint31_half[FFT_64 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw128_cint31_half[FFT_128 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw256_cint31_half[FFT_256 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw512_cint31_half[FFT_512 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1024_cint31_half[FFT_1024 / 2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2048_cint31_half[FFT_2048 / 2];

// Full (2 quadrant) integer tables
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1_cint31[FFT_1];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2_cint31[FFT_2];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw4_cint31[FFT_4];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw8_cint31[FFT_8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw16_cint31[FFT_16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw32_cint31[FFT_32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw64_cint31[FFT_64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw128_cint31[FFT_128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw256_cint31[FFT_256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw512_cint31[FFT_512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw1024_cint31[FFT_1024];
alignas(__ALIGN_BYTE_SIZE__) extern const cint32_t fft_lut_tw2048_cint31[FFT_2048];

#ifdef __FFT_TRUE_R4__
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_1_2[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_2_4[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_4_8[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_8_16[8];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_16_32[16];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_32_64[32];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_64_128[64];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_128_256[128];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_256_512[256];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_512_1024[512];
alignas(__ALIGN_BYTE_SIZE__) extern const cint16 fft_lut_cint31_r4_1024_2048[1024];
#endif

// cfloat tables
// Full (2 quadrant) float tables.
// Float cannot use the one quadrant trick because float cannot use radix4 functions.
// Why? The result of a butterfly for ints is an acc register, but in float it is a float reg.
// This means that the acc registers are unavailable to store data in float and this means
// there is not the capacity in registers required for the storage of inter-rank values in a radix 4
// stage, hence float uses radix2.
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw1_cfloat[FFT_1];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw2_cfloat[FFT_2];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw4_cfloat[FFT_4];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw8_cfloat[FFT_8];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw16_cfloat[FFT_16];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw32_cfloat[FFT_32];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw64_cfloat[FFT_64];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw128_cfloat[FFT_128];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw256_cfloat[FFT_256];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw512_cfloat[FFT_512];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw1024_cfloat[FFT_1024];
alignas(__ALIGN_BYTE_SIZE__) extern const cfloat fft_lut_tw2048_cfloat[FFT_2048];

#endif /* __FFT_BUFS_H__ */
