/*
 * Copyright 2019 Xilinx, Inc.
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

/**
 *
 * @file keccak256.hpp
 * @brief header file for KECCAK-256 algorithm.
 * This file is part of Vitis Security Library.
 *
 * @detail KECCAK_f is the permutation function which is equivalent to KECCAK-p[1600,24] as defined in the standard.
 * keccak256Digest is the main digest part which is responsible for absorbing the input 64-bit message stream into
 * 1600-bit
 * blocks,
 * and squeezing the specific bits of the state array which calculated by the KECCAK_f as the digest according to the
 * suffix of the algorithm.
 *
 */

#ifndef _XF_SECURITY_KECCAK256_HPP_
#define _XF_SECURITY_KECCAK256_HPP_

#include <ap_int.h>
#include <hls_stream.h>

// for debug
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace security {
namespace internal {

// @brief 1600-bit Processing block
struct blockType {
    ap_uint<64> M[25];
    blockType() {
#pragma HLS array_partition variable = M dim = 1
    }
};

/**
 *
 * @brief The implementation of rotate left (circular left shift) operation.
 * The algorithm reference is : "SHA-3 Standard : Permutation-Based Hash and Extendable-Output Functions".
 *
 * @tparam w The bit width of input x, default value is 64.
 *
 * @param n Number of bits for input x to be shifted.
 * @param x Word to be rotated.
 *
 */

template <unsigned int w = 64>
ap_uint<w> ROTL(
    // inputs
    ap_uint<w> x,
    unsigned int n) {
#pragma HLS inline

    return ((x << n) | (x >> (w - n)));

} // end ROTL

/**
 *
 * @brief The implementation of KECCAK-f permutation function.
 *
 * The algorithm reference is : "SHA-3 Standard : Permutation-Based Hash and Extendable-Output Functions".
 * The implementation is modified for better performance.
 *
 * @param stateArray The 5*5*64 state array defined in standard.
 *
 */

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

/**
 *
 * @brief This function performs the computation of KECCAK[sha-3 standard submitted in 2011].
 *
 * The algorithm reference is : "The KECCAK SHA-3 submission-Version 3".
 * Diff with standard SHA-3 lies in multi-rate padding strategy, 0x01 for keccak vs 0x06 for sha-3.
 * The implementation is modified for better performance.
 *
 * @tparam hashLen The width of the digest in byte, default value is 32.
 *
 * @param msgStrm The message being hashed.
 * @param msgLenStrm Message length in byte.
 * @param endMsgLenStrm The flag to signal end of input message stream.
 * @param digestStrm Output digest stream.
 * @param endDigestStrm End flag for output digest stream.
 *
 */

template <unsigned int hashLen = 32>
void keccakDigest(
    // inputs
    hls::stream<ap_uint<64> >& msgStrm,
    hls::stream<ap_uint<128> >& msgLenStrm,
    hls::stream<bool>& endMsgLenStrm,
    // outputs
    hls::stream<ap_uint<8 * hashLen> >& digestStrm,
    hls::stream<bool>& endDigestStrm) {
    // max data width in byte for 1 single block
    const ap_uint<8> sizeR = 200 - (hashLen << 1);

    // number of message word for filling up 1 full block
    const ap_uint<5> numMsgWord = sizeR >> 3;

    bool endFlag = endMsgLenStrm.read();

LOOP_KECCAK_MAIN:
    while (!endFlag) {
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
        // read message length in byte
        ap_uint<128> msgLen = msgLenStrm.read();

        // total number of blocks to digest
        ap_uint<128> blkNumReg = msgLen / sizeR;
        ap_uint<128> blkNum = blkNumReg + 1;
#pragma HLS resource variable = blkNumReg core = DivnS // latency = 20
#if !defined(__SYNTHESIS__) && __XF_SECURITY_KECCAK_DEBUG__ == 1
        std::cout << "blkNum = " << std::dec << blkNum << std::endl;
#endif

        // state array
        blockType stateArray;
#pragma HLS array_partition variable = stateArray.M complete
    LOOP_INIT_STATE_ARRAYS:
        for (ap_uint<5> i = 0; i < 25; i++) {
#pragma HLS unroll
            stateArray.M[i] = 0x0UL;
        }

        // number of bytes left
        ap_uint<128> left = msgLen;

    LOOP_KECCAK_DIGEST_NBLK:
        for (ap_uint<128> n = 0; n < blkNum; n++) {
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
            // still have full message block to handle
            if ((left >> 3) >= numMsgWord) {
            // generate 1 full message block
            LOOP_GEN_FULL_MESSAGE_BLK:
                for (ap_uint<5> i = 0; i < numMsgWord; ++i) {
#pragma HLS pipeline II = 1
                    // XXX algorithm assumes little-endian
                    ap_uint<64> msgReg = msgStrm.read();
                    stateArray.M[i] ^= msgReg;
                }
                // decrease number of bytes left by max data width in byte for 1 single block
                left -= sizeR;
                // left message words cannot make up a full message block
            } else {
            // generate the last block
            LOOP_GEN_LAST_BLK:
                for (ap_uint<5> i = 0; i < numMsgWord; ++i) {
#pragma HLS pipeline II = 1
                    // still have full message words
                    if (i < (left >> 3)) {
                        // XXX algorithm assumes little-endian
                        ap_uint<64> msgReg = msgStrm.read();
                        stateArray.M[i] ^= msgReg;
                    } else if (i == (left >> 3)) {
                        // xor 0x01 at the end of the message, while in contrast 0x06 for sha-3 in NIPS standard.
                        ap_uint<3> e = left & 0x7UL;
                        if (e == 0) {
                            // contains no message byte
                            stateArray.M[i] ^= 0x0000000000000001UL;
                        } else if (e == 1) {
                            // contains 1 message byte
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x00000000000000ffUL;
                            stateArray.M[i] ^= (msgReg | 0x0000000000000100UL);
                        } else if (e == 2) {
                            // contains 2 message bytes
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x000000000000ffffUL;
                            stateArray.M[i] ^= (msgReg | 0x0000000000010000UL);
                        } else if (e == 3) {
                            // contains 3 message bytes
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x0000000000ffffffUL;
                            stateArray.M[i] ^= (msgReg | 0x0000000001000000UL);
                        } else if (e == 4) {
                            // contains 4 message bytes
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x00000000ffffffffUL;
                            stateArray.M[i] ^= (msgReg | 0x0000000100000000UL);
                        } else if (e == 5) {
                            // contains 5 message bytes
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x000000ffffffffffUL;
                            stateArray.M[i] ^= (msgReg | 0x0000010000000000UL);
                        } else if (e == 6) {
                            // contains 6 message bytes
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x0000ffffffffffffUL;
                            stateArray.M[i] ^= (msgReg | 0x0001000000000000UL);
                        } else {
                            // contains 7 message bytes
                            // XXX algorithm assumes little-endian
                            ap_uint<64> msgReg = msgStrm.read();
                            msgReg &= 0x00ffffffffffffffUL;
                            stateArray.M[i] ^= (msgReg | 0x0100000000000000UL);
                        }
                    }
                    if (i == (numMsgWord - 1)) {
                        stateArray.M[i] ^= 0x8000000000000000UL;
                    }
                }
            }
#if !defined(__SYNTHESIS__) && __XF_SECURITY_KECCAK_DEBUG__ == 1
            std::cout << "left = " << std::dec << left << std::endl;
#endif

#if !defined(__SYNTHESIS__) && __XF_SECURITY_KECCAK_DEBUG__ == 1
            for (ap_uint<5> i = 0; i < 25; i++) {
                std::cout << "stateArray[" << std::dec << i << "] = " << std::hex << stateArray.M[i] << std::endl;
            }
#endif

            // permutation
            KECCAK_f(stateArray.M);
        }

        // emit digest
        ap_uint<512> digest;
    LOOP_KECCAK_EMIT:
        for (ap_uint<4> i = 0; i < 8; i++) {
#pragma HLS unroll
            // XXX algorithm assumes little-endian which is the same as HLS
            // thus no need to switch the byte order
            digest.range(64 * i + 63, 64 * i) = stateArray.M[i];
        }
        digestStrm.write(digest.range(8 * hashLen - 1, 0));
        endDigestStrm.write(false);

        // still have message to handle
        endFlag = endMsgLenStrm.read();
    }

    endDigestStrm.write(true);

} // end keccakDigest

} // namespace internal

/**
 *
 * @brief Top function of KECCAK-256.
 *
 * The algorithm reference is : "The KECCAK SHA-3 submission-Version 3".
 *
 * @param msgStrm The message being hashed.
 * @param msgLenStrm Message length in byte.
 * @param endMsgLenStrm The flag to signal end of input message stream.
 * @param digestStrm Output digest stream.
 * @param endDigestStrm End flag for output digest stream.
 *
 */

static void keccak_256(
    // inputs
    hls::stream<ap_uint<64> >& msgStrm,
    hls::stream<ap_uint<128> >& msgLenStrm,
    hls::stream<bool>& endMsgLenStrm,
    // outputs
    hls::stream<ap_uint<256> >& digestStrm,
    hls::stream<bool>& endDigestStrm

    ) {
    internal::keccakDigest<32>(msgStrm, msgLenStrm, endMsgLenStrm, digestStrm, endDigestStrm);

} // end keccak_256

} // namespace security
} // namespace xf

#endif
