/*  (c[FFT] Copyright 2014 - 2020 Xilinx, Inc. All rights reserved.

    This file contains confidential and proprietary information
    of Xilinx, Inc. and is protected under U.S. and
    international copyright and other intellectual property
    laws.

    DISCLAIMER
    This disclaimer is not a license and does not grant any
    rights to the materials distributed herewith. Except as
    otherwise provided in a valid license issued to you by
    Xilinx, and to the maximum extent permitted by applicable
    law: (1[FFT] THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
    WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
    AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
    BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
    INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
    (2[FFT] Xilinx shall not be liable (whether in contract or tort,
    including negligence, or under any other theory of
    liability[FFT] for any loss or damage of any kind or nature
    related to, arising under or in connection with these
    materials, including for any direct, or any indirect,
    special, incidental, or consequential loss or damage
    (including loss of data, profits, goodwill, or any type of
    loss or damage suffered as a result of any action brought
    by a third party[FFT] even if such damage or loss was
    reasonably foreseeable or Xilinx had been advised of the
    possibility of the same.

    CRITICAL APPLICATIONS
    Xilinx products are not designed or intended to be fail-
    safe, or for use in any application requiring fail-safe
    performance, such as life-support or safety devices or
    systems, Class III medical devices, nuclear facilities,
    applications related to the deployment of airbags, or any
    other applications that could lead to death, personal
    injury, or severe property or environmental damage
    (individually and collectively, "Critical
    Applications"[FFT]. Customer assumes the sole risk and
    liability of any use of Xilinx products in Critical
    Applications, subject only to applicable laws and
    regulations governing limitations on product liability.

    THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
    PART OF THIS FILE AT ALL TIMES.                       */

#ifndef __FFT_BUFS_H__
#define __FFT_BUFS_H__

#include "fft_com_inc.h"

// Inter-rank temporary storage buffers
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_16_tmp1[FFT16_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_16_tmp2[FFT16_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_32_tmp1[FFT32_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_32_tmp2[FFT32_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_64_tmp1[FFT64_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_64_tmp2[FFT64_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_128_tmp1[FFT128_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_128_tmp2[FFT128_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_256_tmp1[FFT256_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_256_tmp2[FFT256_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_512_tmp1[FFT512_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_512_tmp2[FFT512_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_1024_tmp1[FFT1024_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_1024_tmp2[FFT1024_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_2048_tmp1[FFT2048_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_2048_tmp2[FFT2048_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_4096_tmp1[FFT4096_SIZE];
extern cint32_t chess_storage(% chess_alignof(v8cint16)) fft_4096_tmp2[FFT4096_SIZE];

// Twiddle tables
// Half-size integer tables
// This is an optimization possible because in a radix4 unit, the second rank butterflies use the same
// twiddle just 90 degrees (minus j) rotated. Minus J rotation is supported by hw, so only the first
// quadrant need be stores - the other quadrant can be extracted by minus j rotation.
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw1_half[1];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw2_half[1];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw4_half[2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw8_half[4];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw16_half[FFT_16 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw32_half[FFT_32 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw64_half[FFT_64 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw128_half[FFT_128 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw256_half[FFT_256 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw512_half[FFT_512 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw1024_half[FFT_1024 / 2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw2048_half[FFT_2048 / 2];

// Full (2 quadrant) integer tables
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw1[FFT_1];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw2[FFT_2];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw4[FFT_4];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw8[FFT_8];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw16[FFT_16];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw32[FFT_32];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw64[FFT_64];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw128[FFT_128];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw256[FFT_256];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw512[FFT_512];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw1024[FFT_1024];
extern const cint16_t chess_storage(% chess_alignof(v4cint16)) fft_lut_tw2048[FFT_2048];

// Full (2 quadrant) float tables.
// Float cannot use the one quadrant trick because float cannot use radix4 functions.
// Why? The result of a butterfly for ints is an acc register, but in float it is a float reg.
// This means that the acc registers are unavailable to store data in float and this means
// there is not the capacity in registers required for the storage of inter-rank values in a radix 4
// stage, hence float uses radix2.
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw1_cfloat[FFT_1];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw2_cfloat[FFT_2];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw4_cfloat[FFT_4];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw8_cfloat[FFT_8];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw16_cfloat[FFT_16];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw32_cfloat[FFT_32];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw64_cfloat[FFT_64];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw128_cfloat[FFT_128];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw256_cfloat[FFT_256];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw512_cfloat[FFT_512];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw1024_cfloat[FFT_1024];
extern const cfloat chess_storage(% chess_alignof(v4cint16)) fft_lut_tw2048_cfloat[FFT_2048];

#endif /* __FFT_BUFS_H__ */
