/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef __XILINX_ETHASH_SHA3__
#define __XILINX_ETHASH_SHA3__

#include <ap_int.h>
#include <hls_stream.h>

struct blockType {
    ap_uint<64> M[25];
    blockType() {
#pragma HLS array_partition variable = M dim = 1
    }
};

template <unsigned int w = 64>
ap_uint<w> ROTL(
    // inputs
    ap_uint<w> x,
    unsigned int n) {
#pragma HLS inline

    return ((x << n) | (x >> (w - n)));

} // end ROTL

static void KECCAK_f(
    // in-out
    ap_uint<64> stateArray[25]) {
    // round index for iota
    const ap_uint<64> roundIndex[24] = {0x0000000000000001, 0x0000000000008082, 0x800000000000808a, 0x8000000080008000,
                                        0x000000000000808b, 0x0000000080000001, 0x8000000080008081, 0x8000000000008009,
                                        0x000000000000008a, 0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
                                        0x000000008000808b, 0x800000000000008b, 0x8000000000008089, 0x8000000000008003,
                                        0x8000000000008002, 0x8000000000000080, 0x000000000000800a, 0x800000008000000a,
                                        0x8000000080008081, 0x8000000000008080, 0x0000000080000001, 0x8000000080008008};
#pragma HLS resource variable = roundIndex core = ROM_2P_LUTRAM

LOOP_5_STEP_MAPPING:
    for (ap_uint<5> rnd = 0; rnd < 24; rnd++) {
#pragma HLS pipeline II = 1
        // 1st step theta
        ap_uint<64> rowReg[5];
#pragma HLS array_partition variable = rowReg complete
    LOOP_THETA_1:
        for (ap_uint<3> i = 0; i < 5; i++) {
#pragma HLS unroll
            rowReg[i] =
                stateArray[i] ^ stateArray[i + 5] ^ stateArray[i + 10] ^ stateArray[i + 15] ^ stateArray[i + 20];
        }

    LOOP_THETA_2:
        for (ap_uint<3> i = 0; i < 5; i++) {
#pragma HLS unroll
            ap_uint<64> tmp = rowReg[(i + 4) % 5] ^ ROTL<64>(rowReg[(i + 1) % 5], 1);
        LOOP_CALCULATE_THETA:
            for (ap_uint<5> j = 0; j < 25; j += 5) {
#pragma HLS unroll
                stateArray[i + j] ^= tmp;
            }
        }

        // 2nd step rho, and 3rd step pi
        ap_uint<64> tmpStateArray[24];
#pragma HLS array_partition variable = tmpStateArray dim = 1
        {
            tmpStateArray[0] = ROTL<64>(stateArray[1], 1);
            tmpStateArray[1] = ROTL<64>(stateArray[10], 3);
            tmpStateArray[2] = ROTL<64>(stateArray[7], 6);
            tmpStateArray[3] = ROTL<64>(stateArray[11], 10);
            tmpStateArray[4] = ROTL<64>(stateArray[17], 15);
            tmpStateArray[5] = ROTL<64>(stateArray[18], 21);
            tmpStateArray[6] = ROTL<64>(stateArray[3], 28);
            tmpStateArray[7] = ROTL<64>(stateArray[5], 36);
            tmpStateArray[8] = ROTL<64>(stateArray[16], 45);
            tmpStateArray[9] = ROTL<64>(stateArray[8], 55);
            tmpStateArray[10] = ROTL<64>(stateArray[21], 2);
            tmpStateArray[11] = ROTL<64>(stateArray[24], 14);
            tmpStateArray[12] = ROTL<64>(stateArray[4], 27);
            tmpStateArray[13] = ROTL<64>(stateArray[15], 41);
            tmpStateArray[14] = ROTL<64>(stateArray[23], 56);
            tmpStateArray[15] = ROTL<64>(stateArray[19], 8);
            tmpStateArray[16] = ROTL<64>(stateArray[13], 25);
            tmpStateArray[17] = ROTL<64>(stateArray[12], 43);
            tmpStateArray[18] = ROTL<64>(stateArray[2], 62);
            tmpStateArray[19] = ROTL<64>(stateArray[20], 18);
            tmpStateArray[20] = ROTL<64>(stateArray[14], 39);
            tmpStateArray[21] = ROTL<64>(stateArray[22], 61);
            tmpStateArray[22] = ROTL<64>(stateArray[9], 20);
            tmpStateArray[23] = ROTL<64>(stateArray[6], 44);
        }

        {
            stateArray[10] = tmpStateArray[0];
            stateArray[7] = tmpStateArray[1];
            stateArray[11] = tmpStateArray[2];
            stateArray[17] = tmpStateArray[3];
            stateArray[18] = tmpStateArray[4];
            stateArray[3] = tmpStateArray[5];
            stateArray[5] = tmpStateArray[6];
            stateArray[16] = tmpStateArray[7];
            stateArray[8] = tmpStateArray[8];
            stateArray[21] = tmpStateArray[9];
            stateArray[24] = tmpStateArray[10];
            stateArray[4] = tmpStateArray[11];
            stateArray[15] = tmpStateArray[12];
            stateArray[23] = tmpStateArray[13];
            stateArray[19] = tmpStateArray[14];
            stateArray[13] = tmpStateArray[15];
            stateArray[12] = tmpStateArray[16];
            stateArray[2] = tmpStateArray[17];
            stateArray[20] = tmpStateArray[18];
            stateArray[14] = tmpStateArray[19];
            stateArray[22] = tmpStateArray[20];
            stateArray[9] = tmpStateArray[21];
            stateArray[6] = tmpStateArray[22];
            stateArray[1] = tmpStateArray[23];
        }

    // 4th step chi
    LOOP_CHI:
        for (ap_uint<5> j = 0; j < 25; j += 5) {
#pragma HLS unroll
            ap_uint<64> stateReg[5];
#pragma HLS array_partition variable = stateReg complete
        LOOP_INIT_STATEREG:
            for (ap_uint<3> i = 0; i < 5; i++) {
#pragma HLS unroll
                stateReg[i] = stateArray[j + i];
            }
        LOOP_CALCULATE_CHI:
            for (ap_uint<3> i = 0; i < 5; i++) {
#pragma HLS unroll
                stateArray[j + i] ^= (~stateReg[(i + 1) % 5]) & stateReg[(i + 2) % 5];
            }
        }

        // 5th step iota
        stateArray[0] ^= roundIndex[rnd];
    }

} // end KECCAK_f

inline ap_uint<512> sha3_512_40(ap_uint<512> input) {
    // This is a special version of sha3_512 for ethash, limited on input message size and padding method
    ap_uint<64> state[25];
#pragma HLS array_partition variable = state dim = 1
    for (int i = 0; i < 25; i++) {
#pragma HLS unroll
        state[i] = 0;
    }

    for (int i = 0; i < 5; i++) {
#pragma HLS unroll
        state[i] ^= input.range(i * 64 + 63, i * 64);
    }
    state[5] ^= 0x0000000000000001UL; // delim is 01 for sha3 in ethereum, and 06 for sha3 in NIST
    state[8] ^= 0x8000000000000000UL;

    KECCAK_f(state);

    ap_uint<512> digest = 0;
    for (int i = 0; i < 8; i++) {
#pragma HLS unroll
        digest.range(64 * i + 63, 64 * i) = state[i];
    }
    return digest;
}

inline ap_uint<512> sha3_512_64(ap_uint<512> input) {
    // This is a special version of sha3_512 for ethash, limited on input message size and padding method
    ap_uint<64> state[25];
#pragma HLS array_partition variable = state dim = 1
    for (int i = 0; i < 25; i++) {
#pragma HLS unroll
        state[i] = 0;
    }

    for (int i = 0; i < 8; i++) {
#pragma HLS unroll
        state[i] ^= input.range(i * 64 + 63, i * 64);
    }
    state[8] ^= 0x0000000000000001UL; // delim is 01 for sha3 in ethereum, and 06 for sha3 in NIST
    state[8] ^= 0x8000000000000000UL;

    KECCAK_f(state);

    ap_uint<512> digest = 0;
    for (int i = 0; i < 8; i++) {
#pragma HLS unroll
        digest.range(64 * i + 63, 64 * i) = state[i];
    }
    return digest;
}

inline ap_uint<512> sha3_512_32(ap_uint<512> input) {
    // This is a special version of sha3_512 for ethash, limited on input message size and padding method
    ap_uint<64> state[25];
#pragma HLS array_partition variable = state dim = 1
    for (int i = 0; i < 25; i++) {
#pragma HLS unroll
        state[i] = 0;
    }

    for (int i = 0; i < 4; i++) {
#pragma HLS unroll
        state[i] ^= input.range(i * 64 + 63, i * 64);
    }
    state[4] ^= 0x0000000000000001UL; // delim is 01 for sha3 in ethereum, and 06 for sha3 in NIST
    state[8] ^= 0x8000000000000000UL;

    KECCAK_f(state);

    ap_uint<512> digest = 0;
    for (int i = 0; i < 8; i++) {
#pragma HLS unroll
        digest.range(64 * i + 63, 64 * i) = state[i];
    }
    return digest;
}

inline ap_uint<256> sha3_256_96(ap_uint<512> input0, ap_uint<512> input1) {
    // This is a special version of sha3_512 for ethash, limited on input message size and padding method
    ap_uint<64> state[25];
#pragma HLS array_partition variable = state dim = 1
    for (int i = 0; i < 25; i++) {
#pragma HLS unroll
        state[i] = 0;
    }

    for (int i = 0; i < 8; i++) {
#pragma HLS unroll
        state[i] ^= input0.range(i * 64 + 63, i * 64);
    }
    for (int i = 0; i < 4; i++) {
#pragma HLS unroll
        state[i + 8] ^= input1.range(i * 64 + 63, i * 64);
    }
    state[12] ^= 0x0000000000000001UL; // delim is 01 for sha3 in ethereum, and 06 for sha3 in NIST
    state[16] ^= 0x8000000000000000UL;

    KECCAK_f(state);

    ap_uint<256> digest = 0;
    for (int i = 0; i < 4; i++) {
#pragma HLS unroll
        digest.range(64 * i + 63, 64 * i) = state[i];
    }
    return digest;
}

inline ap_uint<256> sha3_256_32(ap_uint<512> input0, ap_uint<512> input1) {
    // This is a special version of sha3_512 for ethash, limited on input message size and padding method
    ap_uint<64> state[25];
#pragma HLS array_partition variable = state dim = 1
    for (int i = 0; i < 25; i++) {
#pragma HLS unroll
        state[i] = 0;
    }

    for (int i = 0; i < 4; i++) {
#pragma HLS unroll
        state[i] ^= input0.range(i * 64 + 63, i * 64);
    }

    state[4] ^= 0x0000000000000001UL; // delim is 01 for sha3 in ethereum, and 06 for sha3 in NIST
    state[16] ^= 0x8000000000000000UL;

    KECCAK_f(state);

    ap_uint<256> digest = 0;
    for (int i = 0; i < 4; i++) {
#pragma HLS unroll
        digest.range(64 * i + 63, 64 * i) = state[i];
    }
    return digest;
}
#endif // __XILINX_ETHASH_SHA3__
