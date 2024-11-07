#include "s2mm.h"
using namespace s2mm;

// ------------------------------------------------------------
// Stream Capture
// ------------------------------------------------------------

void capture_streams(TT_SAMPLE (&buff)[NITER][NSTREAM][(memSizeAct * samplesPerRead) / NSTREAM],
                     TT_STREAM sig_i[NSTREAM]) {
// Incoming samples arriving down columns:
CAPTURE:
    for (int ll = 0; ll < NITER; ll++) {
        for (int cc = 0, addr = 0; cc < (memSizeAct) / NSTREAM; cc++) {
#pragma HLS pipeline II = 1
            for (int ss = 0; ss < NSTREAM; ss++) {
                TT_SAMPLE val1, val0;
                (val1, val0) = sig_i[ss].read();
                buff[ll][ss][addr + 0] = val0;
                buff[ll][ss][addr + 1] = val1;
            } // ss
            addr = addr + 2;
        } // cc
    }     // ll
}

// ------------------------------------------------------------
// Read Buffer
// ------------------------------------------------------------

void read_buffer(TT_DATA mem[NITER][memSizeAct],
                 TT_SAMPLE (&buff)[NITER][NSTREAM][(memSizeAct * samplesPerRead) / NSTREAM]) {
    for (int nn = 0; nn < NITER; nn++) {
        for (int rr = 0, mm = 0, ss0 = 0, addr0 = 0; rr < memSizeAct; rr++) {
#pragma HLS PIPELINE II = 1
            int ss1 = (ss0 == NSTREAM - 1) ? 0 : ss0 + 1;
            int addr1 = (ss0 == NSTREAM - 1) ? addr0 + 1 : addr0;
            TT_SAMPLE val0 = buff[nn][ss0][addr0];
            TT_SAMPLE val1 = buff[nn][ss1][addr1];
            if (ss0 == NSTREAM - 1 || ss1 == NSTREAM - 1) {
                addr0 += 1;
            }
            mem[nn][mm++] = (val1, val0);
            ss0 = (ss0 + 2 > NSTREAM - 1) ? ss0 + 2 - NSTREAM : ss0 + 2;
        }
    }
}

// ------------------------------------------------------------
// Used for streams=1
// ------------------------------------------------------------

void s2mm_str1(TT_DATA mem[NITER][memSizeAct], TT_STREAM sig_i[NSTREAM]) {
    for (int nn = 0; nn < NITER; nn++) {
        for (int rr = 0, mm = 0; rr < memSizeAct; rr++) {
#pragma HLS PIPELINE II = 1
            mem[nn][mm++] = sig_i[0].read();
        }
    }
}
// ------------------------------------------------------------
// Wrapper
// ------------------------------------------------------------

void s2mm_wrapper(s2mm::TT_DATA mem[NITER][memSizeAct], s2mm::TT_STREAM sig_i[NSTREAM])

{
#pragma HLS interface m_axi port = mem bundle = gmem offset = slave depth = NITER * memSizeAct
#pragma HLS interface axis port = sig_i
#pragma HLS interface s_axilite port = mem bundle = control
#pragma HLS interface s_axilite port = return bundle = control
#pragma HLS DATAFLOW

    TT_SAMPLE buff[NITER][NSTREAM][(memSizeAct * samplesPerRead) / NSTREAM];

#pragma HLS array_partition variable = buff dim = 1
#pragma HLS array_partition variable = buff dim = 2
#pragma HLS bind_storage variable = buff type = RAM_T2P impl = uram
#pragma HLS dependence variable = buff type = intra false

    if (NSTREAM != 1) {
        // Front end load from DDR4 to PL BRAM:
        capture_streams(buff, sig_i);

        // Back end transmit from PL BRAM to AIE:
        read_buffer(mem, buff);
    } else {
        s2mm_str1(mem, sig_i);
    }
}
