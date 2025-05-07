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
    TT_SAMPLE (&buff)[NITER][NSTREAM][(2 * memSizeAct) / NSTREAM]) // memSizeAct = 256;  2*memSizeAct/NSTREAM = 128
{
LOAD_NITER:
    for (int n = 0; n < NITER; n++) {
    LOAD_SAMP:
        for (int samp = 0, dd = 0, ss = 0; samp < (memSizeAct); samp += 1) {
#pragma HLS pipeline II = 1
            TT_SAMPLE val0, val1;
            (val1, val0) = mem[n][samp];
            if (ss == NSTREAM - 1) {
                buff[n][NSTREAM - 1][dd] = val0;
                buff[n][0][dd + 1] = val1;
            } else {
                buff[n][ss][dd] = val0;
                buff[n][ss + 1][dd] = val1;
            }

            // Advance:
            dd = (ss >= NSTREAM - 2) ? dd + 1 : dd;
            ss = (ss >= NSTREAM - 2) ? (ss + 2 - NSTREAM) : (ss + 2);
        } // col
    }
}

void mm2s_str1(TT_DATA mem[NITER][memSizeAct],
               TT_STREAM sig_o[NSTREAM]) // memSizeAct = 256;  2*memSizeAct/NSTREAM = 128
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

void transmit(TT_SAMPLE (&buff)[NITER][NSTREAM][(2 * memSizeAct) / NSTREAM], TT_STREAM sig_o[NSTREAM]) {
REPEAT:
    for (int nn = 0; nn < NITER; nn++) { // 4
    RUN_DEPTH:
        for (int cc = 0, rr = 0, dd = 0; cc < (2 * memSizeAct) / NSTREAM;
             cc += 2) { // Send two samples per stream word // 4100/5
#pragma HLS PIPELINE II = 1
        STREAM1:
            for (int ss = 0; ss < NSTREAM; ss++) {
                TT_SAMPLE val0 = buff[nn][ss][cc];
                TT_SAMPLE val1 = buff[nn][ss][cc + 1];
                sig_o[ss].write((val1, val0));
            } // ss
        }     // cc
    }         // nn
}
// ------------------------------------------------------------
// Wrapper
// ------------------------------------------------------------

void mm2s_wrapper(TT_DATA mem[NITER][memSizeAct], TT_STREAM sig_o[NSTREAM]) {
#pragma HLS interface m_axi port = mem bundle = gmem offset = slave depth = memSizeAct * NITER
#pragma HLS interface axis port = sig_o
#pragma HLS interface s_axilite port = mem bundle = control
#pragma HLS interface s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    // Internal buffer:
    TT_SAMPLE buff[NITER][NSTREAM][(2 * memSizeAct) / NSTREAM];
#pragma HLS array_partition variable = buff dim = 1
#pragma HLS array_partition variable = buff dim = 2
#pragma HLS bind_storage variable = buff type = RAM_T2P impl = bram
#pragma HLS dependence variable = buff type = intra false

    // // Front end load from DDR4 to PL BRAM:
    if (NSTREAM != 1) {
        load_buffer(mem, buff);
        transmit(buff, sig_o);
    } else {
        mm2s_str1(mem, sig_o);
    }

    // // Back end transmit from PL BRAM to AIE:
}