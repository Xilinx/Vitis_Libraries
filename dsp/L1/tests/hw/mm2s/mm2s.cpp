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
//
// Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
//

#include "mm2s.h"
#include <fstream>
using namespace mm2s;
using namespace std;

// ------------------------------------------------------------
// Transmit
// ------------------------------------------------------------
void load_buffer(
    TT_DATA mem[NITER][memSizeAct],
    TT_SAMPLE (&buff)[NITER][NSTREAM_INT]
                     [(samplesPerRead * memSizeAct) / NSTREAM_INT]) // memSizeAct = 256;  2*memSizeAct/NSTREAM_INT = 128
{
LOAD_NITER:
    for (int n = 0; n < NITER; n++) {
    LOAD_SAMP:
        for (int samp = 0, dd = 0, ss = 0; samp < (memSizeAct); samp += 1) {
#pragma HLS pipeline II = 1

            TT_SAMPLE val0, val1, val2, val3;
            if
                constexpr(samplesPerRead == 2) {
                    (val1, val0) = mem[n][samp];
                    if (ss == NSTREAM_INT - 1) {
                        buff[n][NSTREAM_INT - 1][dd] = val0;
                        buff[n][0][dd + 1] = val1;
                    } else {
                        buff[n][ss][dd] = val0;
                        buff[n][ss + 1][dd] = val1;
                    }

                    // Advance:
                    dd = (ss >= NSTREAM_INT - samplesPerRead) ? dd + 1 : dd;
                    ss = (ss >= NSTREAM_INT - samplesPerRead) ? (ss + samplesPerRead - NSTREAM_INT)
                                                              : (ss + samplesPerRead);
                }
            else {
                (val3, val2, val1, val0) = mem[n][samp];
                if (ss == NSTREAM_INT - 1) {
                    buff[n][NSTREAM_INT - 1][dd] = val0;
                    buff[n][0][dd + 1] = val1;
                    buff[n][1][dd + 1] = val2;
                    buff[n][2][dd + 1] = val3;
                } else if (ss == NSTREAM_INT - 2) {
                    buff[n][NSTREAM_INT - 2][dd] = val0;
                    buff[n][NSTREAM_INT - 1][dd] = val1;
                    buff[n][0][dd + 1] = val2;
                    buff[n][1][dd + 1] = val3;
                } else if (ss == NSTREAM_INT - 3) {
                    buff[n][NSTREAM_INT - 3][dd] = val0;
                    buff[n][NSTREAM_INT - 2][dd] = val1;
                    buff[n][NSTREAM_INT - 1][dd] = val2;
                    buff[n][0][dd + 1] = val3;
                } else {
                    buff[n][ss][dd] = val0;
                    buff[n][ss + 1][dd] = val1;
                    buff[n][ss + 2][dd] = val2;
                    buff[n][ss + 3][dd] = val3;
                }

                // Advance:
                dd = (ss >= NSTREAM_INT - samplesPerRead) ? dd + 1 : dd;
                ss = (ss >= NSTREAM_INT - samplesPerRead) ? (ss + samplesPerRead - NSTREAM_INT) : (ss + samplesPerRead);
            }
        } // col
    }
}

void mm2s_str1(TT_DATA mem[NITER][memSizeAct],
               TT_STREAM sig_o[NSTREAM_INT]) // memSizeAct = 256;  2*memSizeAct/NSTREAM_INT = 128
{
LOAD_NITER:
    for (int n = 0; n < NITER; n++) {
    LOAD_SAMP:
        for (int samp = 0, dd = 0, ss = 0; samp < (memSizeAct); samp += 1) {
#pragma HLS pipeline II = 1
            sig_o[0].write(mem[n][samp]);
        }
    }
}

// ------------------------------------------------------------
// Transmit
// ------------------------------------------------------------

void transmit(TT_SAMPLE (&buff)[NITER][NSTREAM_INT][(samplesPerRead * memSizeAct) / NSTREAM_INT],
              TT_STREAM sig_o[NSTREAM_INT]) {
REPEAT:
    for (int nn = 0; nn < NITER; nn++) { // 4
    RUN_DEPTH:
        for (int cc = 0, rr = 0, dd = 0; cc < (samplesPerRead * memSizeAct) / NSTREAM_INT; cc += samplesPerRead) {
#pragma HLS PIPELINE II = 1
        STREAM1:
            for (int ss = 0; ss < NSTREAM_INT; ss++) {
                TT_SAMPLE val0 = buff[nn][ss][cc];
                TT_SAMPLE val1 = buff[nn][ss][cc + 1];
                if
                    constexpr(samplesPerRead == 2) { sig_o[ss].write((val1, val0)); }
                else {
                    TT_SAMPLE val0 = buff[nn][ss][cc];
                    TT_SAMPLE val1 = buff[nn][ss][cc + 1];
                    TT_SAMPLE val2 = buff[nn][ss][cc + 2];
                    TT_SAMPLE val3 = buff[nn][ss][cc + 3];
                    // printf("buff[%d][%d][%d] = %d, %d\n", nn, ss, cc, buff[nn][ss][cc] >>16, buff[nn][ss][cc] %
                    // (1<<15));
                    // printf("buff[%d][%d][%d] = %d, %d\n", nn, ss, cc+1, buff[nn][ss][cc+1] >>16, buff[nn][ss][cc+1] %
                    // (1<<15));
                    // printf("buff[%d][%d][%d] = %d, %d\n", nn, ss, cc+2, buff[nn][ss][cc+2] >>16, buff[nn][ss][cc+2] %
                    // (1<<15));
                    // printf("buff[%d][%d][%d] = %d, %d\n", nn, ss, cc+3, buff[nn][ss][cc+3] >>16, buff[nn][ss][cc+3] %
                    // (1<<15));
                    sig_o[ss].write((val3, val2, val1, val0));
                }
            } // ss
        }     // cc
    }         // nn
}
// ------------------------------------------------------------
// Wrapper
// ------------------------------------------------------------

void mm2s_wrapper(TT_DATA mem[NITER][memSizeAct], TT_STREAM sig_o[NSTREAM_INT]) {
#pragma HLS interface m_axi port = mem bundle = gmem offset = slave depth = memSizeAct * NITER
#pragma HLS interface axis port = sig_o
#pragma HLS interface s_axilite port = mem bundle = control
#pragma HLS interface s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    // Internal buffer:
    TT_SAMPLE buff[NITER][NSTREAM_INT][(samplesPerRead * memSizeAct) / NSTREAM_INT];
#pragma HLS array_partition variable = buff dim = 1
#pragma HLS array_partition variable = buff dim = 2
#pragma HLS bind_storage variable = buff type = RAM_T2P impl = bram
#pragma HLS dependence variable = buff type = intra false

    // // Front end load from DDR4 to PL BRAM:
    if (NSTREAM_INT != 1) {
        load_buffer(mem, buff);
        transmit(buff, sig_o);
    } else {
        mm2s_str1(mem, sig_o);
    }

    // // Back end transmit from PL BRAM to AIE:
}