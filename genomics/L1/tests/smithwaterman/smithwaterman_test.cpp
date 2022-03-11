#include <ap_int.h>
#include "sw.h"
#include <iostream>
#include "genseq.hpp"
#include "opencl_sw_maxscore_systolic.cpp"
#include <stdint.h>
#define NUMITER 4
#define NUM_BLOCKS 1

void smithwatermanMaxscore(ap_uint<NUMPACKED * 2>* input, ap_uint<NUMPACKED * 2>* output, int* size) {
#pragma HLS INTERFACE m_axi port = input offset = slave bundle = gmem depth = 1536
#pragma HLS INTERFACE m_axi port = output offset = slave bundle = gmem depth = 96
#pragma HLS INTERFACE m_axi port = size offset = slave bundle = gmem depth = 1
#pragma HLS INTERFACE s_axilite port = input bundle = control
#pragma HLS INTERFACE s_axilite port = output bundle = control
#pragma HLS INTERFACE s_axilite port = size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control
    opencl_sw_maxscore(input, output, size);
}

void convertInOut(unsigned int* input, unsigned int* output, int* size) {
    const uint32_t c_dataWidth = NUMPACKED * 2;
    ap_uint<c_dataWidth> in[UINTSZ * NUMPACKED * PACKEDSZ];
    ap_uint<c_dataWidth> out[NUMPACKED * 3];

    memcpy(in, input, UINTSZ * PACKEDSZ * NUMPACKED);
    smithwatermanMaxscore(in, out, size);
}

int main(int argc, char* argv[]) {
    unsigned int* output;
    unsigned int* input;
    unsigned int* outputGolden = new unsigned int;
    int* size;
    int hwBlockSize = NUMPACKED * NUMITER;
    int totalSamples = NUM_BLOCKS * NUMITER * NUMPACKED;
    std::cout << "Number of SmithWaterman instances on FPGA:" << NUMPACKED << "\n";
    std::cout << "Total processing elements:" << MAXPE * NUMPACKED << "\n";
    std::cout << "Length of reference string:" << MAXCOL << "\n";
    std::cout << "Length of read(query) string:" << MAXROW << "\n";
    std::cout << "Read-Ref pair block size(HOST to FPGA):" << NUMITER << "\n"; // BLOCK_SIZE
    std::cout << "---------------------------------------\n";
    input = generatePackedNReadRefPair(totalSamples, MAXROW, MAXCOL, &outputGolden, 0);
    output = new unsigned int[UINTSZ * hwBlockSize * 3];
    size = new int;
    *size = UINTSZ;
    convertInOut(input, output, size);
}
