/*
 * Copyright 2021 Xilinx, Inc.
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

#include <vector>
#include <math.h>
#include <cmath>
#include <iostream>
#include <fstream>

#include "kernel_poseidon.hpp"
#include "utils.hpp"

int main() {
    std::cout << "Read input \n";
    std::string inputFile = "input_vector.dat";
    std::vector<ap_uint<256> > inputVec;
    readDataFromFile_0<256>(inputFile, inputVec);
    std::cout << "Read input done\n";

    std::cout << "Read golden\n";
    std::string goldenFile = "golden_vector.dat";
    std::vector<ap_uint<256> > goldenVec;
    readDataFromFile_0<256>(goldenFile, goldenVec);
    unsigned int goldenLength = goldenVec.size();

    std::cout << "Read round_consts\n";
    std::string constsFile = "round_consts.dat";
    std::vector<ap_uint<256> > consts_vec;
    readDataFromFile_0<256>(constsFile, consts_vec);

    std::cout << "Read mds_matrix\n";
    std::string matrixFile = "mds_matrix.dat";
    std::vector<ap_uint<256> > matrix_vec;
    readDataFromFile_1<256>(matrixFile, matrix_vec);

    ap_uint<256> prime = 0;
    convertStr2Int<256>(PRIME, &prime);

    std::cout << std::dec << "Poseidon_128 Instance: t=" << t << ", alpt=" << alph << ", n=" << n << ", M=" << M
              << ", sbox=" << sbox << ", field=" << field << ", security_margin=" << security_margin << ", RF=" << RF
              << ", RP=" << RP << std::endl;

    hls::stream<ap_uint<256> > mdsMatrixStrm("mdsMatrixStrm");
    for (int i = 0; i < t * t; i++) {
        mdsMatrixStrm.write(matrix_vec.at(i));
    }

    hls::stream<ap_uint<256> > roundConstantsStrm("roundConstantsStrm");
    for (int k = 0; k < (t * RF + t * RP); k++) {
        roundConstantsStrm.write(consts_vec.at(k));
    }

    hls::stream<ap_uint<256> > inStrm("inputStrm");
    for (int n = 0; n < inputLength; n++) {
        inStrm.write(swapBytes256(inputVec.at(n)));
    }
    ap_uint<256> poseidon_out = 0;
    ap_uint<256> golden = 0;

    int nerr = 0;

#if !defined(__SYNTHESIS__)

    kernel_poseidon(inputLength, inStrm, roundConstantsStrm, mdsMatrixStrm, prime, &poseidon_out);

    switch (inputLength) {
        case 3: {
            golden = swapBytes256(goldenVec.at(0));
            std::cout << std::hex << "Check Test result_0: golden=" << golden << ", output=" << poseidon_out
                      << std::endl;
            if (golden != poseidon_out) {
                std::cout << std::hex << "Test Mismatched: golden=" << golden << ", output=" << poseidon_out
                          << std::endl;
                nerr += 1;
            }
            break;
        }
        case 4: {
            golden = swapBytes256(goldenVec.at(1));
            std::cout << std::hex << "Check Test result_1: golden=" << golden << ", output=" << poseidon_out
                      << std::endl;
            if (golden != poseidon_out) {
                std::cout << std::hex << "Test Mismatched: golden=" << golden << ", output=" << poseidon_out
                          << std::endl;
                nerr += 1;
            }
            break;
        }
        case 5: {
            golden = swapBytes256(goldenVec.at(2));
            std::cout << std::hex << "Check Test result_2: golden=" << golden << ", output=" << poseidon_out
                      << std::endl;
            if (golden != poseidon_out) {
                std::cout << std::hex << "Test Mismatched: golden=" << golden << ", output=" << poseidon_out
                          << std::endl;
                nerr += 1;
            }
            break;
        }
        case 6: {
            golden = swapBytes256(goldenVec.at(3));
            std::cout << std::hex << "Check Test result_3: golden=" << golden << ", output=" << poseidon_out
                      << std::endl;
            if (golden != poseidon_out) {
                std::cout << std::hex << "Test Mismatched: golden=" << golden << ", output=" << poseidon_out
                          << std::endl;
                nerr += 1;
            }
            break;
        }
        case 8: {
            golden = swapBytes256(goldenVec.at(4));
            std::cout << std::hex << "Check Test result_4: golden=" << golden << ", output=" << poseidon_out
                      << std::endl;
            if (golden != poseidon_out) {
                std::cout << std::hex << "Test Mismatched: golden=" << golden << ", output=" << poseidon_out
                          << std::endl;
                nerr += 1;
            }
            break;
        }
        case 10: {
            golden = swapBytes256(goldenVec.at(5));
            std::cout << std::hex << "Check Test result_5: golden=" << golden << ", output=" << poseidon_out
                      << std::endl;
            if (golden != poseidon_out) {
                std::cout << std::hex << "Test Mismatched: golden=" << golden << ", output=" << poseidon_out
                          << std::endl;
                nerr += 1;
            }
            break;
        }
        default:
            std::cout << std::hex << "Test result: output=" << poseidon_out << std::endl;
    }

    if (nerr != 0) {
        std::cout << "Test failed, nerr = " << nerr << "\n";
    } else {
        std::cout << "Test passed \n";
    }
    return nerr;
}
#endif
