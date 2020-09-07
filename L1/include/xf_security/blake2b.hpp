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
 * @file blake2b.hpp
 * @brief header file for BLAKE2B related functions, including mixing, compression, padding, and computing functions.
 * This file is part of Vitis Security Library.
 *
 * @detail The algorithm takes a message and a key of arbitrary length (0 <= message length <= 2^128 bytes, 0 <= key
 * length <= 64 bytes) as its input,
 * and produces a specified length (1 <= output length <= 64 bytes) digest".
 * Notice that the key is optional to be added to the hash process, you can get an unkeyed hashing by simply setting the
 * key length to zero.
 * A special case is that both key and message length are set to zero, which means an unkeyed hashing with an empty
 * message will be executed.
 *
 */

#ifndef _XF_SECURITY_BLAKE2B_HPP_
#define _XF_SECURITY_BLAKE2B_HPP_

#include <ap_int.h>
#include <hls_stream.h>

// for debug
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace security {
namespace internal {

// @brief 1024-bit Processing block
struct blockType {
    ap_uint<64> M[16];
};

/**
 * @brief Generate 1024-bit processing blocks by padding (pipeline).
 *
 * The algorithm reference is : "The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)".
 * The optimization goal of this function is to yield a 1024-bit block per cycle.
 *
 * @tparam w Bit width of the message words in block, default value is 64.
 *
 * @param msg_strm The message being hashed.
 * @param msg_len_strm Message length in byte (0 <= msg_len <= 2^128).
 * @param key_strm The optional key.
 * @param key_len_strm Key length in byte (0 <= key_len <= 64).
 * @param end_len_strm The flag to signal end of input message stream.
 * @param blk_strm The 1024-bit hash block.
 * @param nblk_strm The number of hash block for this message.
 * @param end_nblk_strm End flag for number of hash block.
 * @param msg_len_out_strm Message length pass on to the digest process.
 * @param key_len_out_strm Key length pass on to the digest process.
 *
 */

template <unsigned int w = 64>
void generateBlock(
    // inputs
    hls::stream<ap_uint<w> >& msg_strm,
    hls::stream<ap_uint<128> >& msg_len_strm,
    hls::stream<ap_uint<w> >& key_strm,
    hls::stream<ap_uint<8> >& key_len_strm,
    hls::stream<bool>& end_len_strm,
    // outputs
    hls::stream<blockType>& blk_strm,
    hls::stream<ap_uint<128> >& nblk_strm,
    hls::stream<bool>& end_nblk_strm,
    hls::stream<ap_uint<128> >& msg_len_out_strm,
    hls::stream<ap_uint<8> >& key_len_out_strm) {
    bool endFlag = end_len_strm.read();

LOOP_PREPROCESSING_MAIN:
    while (!endFlag) {
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
        // read message length in byte
        ap_uint<128> msg_len = msg_len_strm.read();

        // read key length in byte
        ap_uint<8> key_len = key_len_strm.read();

        // total number blocks to digest in 1024-bit
        ap_uint<128> blk_num;
        // still need to send a zero block if both of the key length and message length is zero
        if ((key_len == 0) && (msg_len == 0)) {
            blk_num = 1;
        } else {
            blk_num = (key_len > 0) + (msg_len >> 7) + ((msg_len % 128) > 0);
        }
#if !defined(__SYNTHESIS__) && __XF_SECURITY_BLAKE2B_DEBUG_ == 1
        std::cout << "blk_num = " << std::dec << blk_num << std::endl;
#endif

        // inform digest function
        msg_len_out_strm.write(msg_len);
        key_len_out_strm.write(key_len);
        nblk_strm.write(blk_num);
        end_nblk_strm.write(false);

        // generate key block
        if (key_len > 0) {
            // key block
            blockType k;
#pragma HLS array_partition variable = k.M complete

        LOOP_GEN_KEY_BLK:
            for (ap_uint<5> i = 0; i < 16; i++) {
#pragma HLS unroll
                if (i < (key_len >> 3)) {
                    // still have full key words
                    // XXX algorithm assumes little-endian
                    k.M[i] = key_strm.read();
                } else if (i > (key_len >> 3)) {
                    // no key word to read
                    k.M[i] = 0UL;
                } else {
                    // pad the 64-bit key word with specific zero bytes
                    ap_uint<3> e = key_len & 0x7UL;
                    if (e == 0) {
                        // contains no key byte
                        k.M[i] = 0x0UL;
                    } else if (e == 1) {
                        // contains 1 key byte
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x00000000000000ffUL;
                    } else if (e == 2) {
                        // contains 2 key bytes
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x000000000000ffffUL;
                    } else if (e == 3) {
                        // contains 3 key bytes
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x0000000000ffffffUL;
                    } else if (e == 4) {
                        // contains 4 key bytes
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x00000000ffffffffUL;
                    } else if (e == 5) {
                        // contains 5 key bytes
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x000000ffffffffffUL;
                    } else if (e == 6) {
                        // contains 6 key bytes
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x0000ffffffffffffUL;
                    } else {
                        // contains 7 key bytes
                        ap_uint<w> l = key_strm.read();
                        // XXX algorithm assumes little-endian
                        k.M[i] = l & 0x00ffffffffffffffUL;
                    }
                }
            }

            // send key block
            blk_strm.write(k);
        }

    LOOP_GEN_FULL_MSG_BLKS:
        for (ap_uint<128> j = 0; j < (ap_uint<128>)(msg_len >> 7); ++j) {
#pragma HLS pipeline II = 16
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
            // message block
            blockType b0;

        // this block will hold 16 words (64-bit for each) of message
        LOOP_GEN_ONE_FULL_BLK:
            for (ap_uint<5> i = 0; i < 16; ++i) {
                // XXX algorithm assumes little-endian
                b0.M[i] = msg_strm.read();
            }

            // send the full block
            blk_strm.write(b0);
        }

        // number of bytes left which needs to be padded as a new full block
        ap_uint<7> left = (ap_uint<7>)(msg_len & 0x7fUL);

        // not end at block boundary, pad the remaining message bytes to a full block
        // or the special case of an unkeyed empty message, send a zero block
        if ((left > 0) | ((msg_len == 0) && (key_len == 0))) {
            // last message block
            blockType b;
#pragma HLS array_partition variable = b.M complete

        LOOP_PAD_ZEROS:
            for (ap_uint<5> i = 0; i < 16; i++) {
#pragma HLS unroll
                if (i < (left >> 3)) {
                    // still have full message words
                    // XXX algorithm assumes little-endian
                    b.M[i] = msg_strm.read();
                } else if (i > (left >> 3)) {
                    // no meesage word to read
                    b.M[i] = 0UL;
                } else {
                    // pad the 64-bit message word with specific zero bytes
                    ap_uint<3> e = left & 0x7UL;
                    if (e == 0) {
                        // contains no message byte
                        b.M[i] = 0x0UL;
                    } else if (e == 1) {
                        // contains 1 message byte
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x00000000000000ffUL;
                    } else if (e == 2) {
                        // contains 2 message bytes
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x000000000000ffffUL;
                    } else if (e == 3) {
                        // contains 3 message bytes
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x0000000000ffffffUL;
                    } else if (e == 4) {
                        // contains 4 message bytes
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x00000000ffffffffUL;
                    } else if (e == 5) {
                        // contains 5 message bytes
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x000000ffffffffffUL;
                    } else if (e == 6) {
                        // contains 6 message bytes
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x0000ffffffffffffUL;
                    } else {
                        // contains 7 message bytes
                        ap_uint<w> l = msg_strm.read();
                        // XXX algorithm assumes little-endian
                        b.M[i] = l & 0x00ffffffffffffffUL;
                    }
                }
            }

            // emit last block
            blk_strm.write(b);
        }

        // still have message to handle
        endFlag = end_len_strm.read();
    }

    end_nblk_strm.write(true);

} // end generateBlock

/**
 *
 * @brief The implementation of rotate right (circular right shift) operation.
 *
 * The algorithm reference is : "The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)".
 *
 * @tparam w The bit width of input x, default value is 64.
 * @tparam n Number of bits for input x to be shifted.
 *
 * @param x Word to be rotated.
 *
 */

template <unsigned int w = 64, unsigned int n = 0>
ap_uint<w> ROTR(
    // inputs
    ap_uint<w> x) {
#pragma HLS inline

    return ((x >> n) | (x << (w - n)));

} // end ROTR

/**
 * @brief Mixing function G as defined in standard.
 *
 * The algorithm reference is : "The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)".
 *
 * @tparam Bit width of the words, default value is 64.
 *
 * @param v Working vector.
 * @param a The first index.
 * @param b The second index.
 * @param c The third index.
 * @param d the fourth index.
 * @param x The first input working word.
 * @param y The second input working word.
 *
 */

template <unsigned int w = 64>
void G(
    // in-out
    ap_uint<w> v[16],
    // inputs
    ap_uint<4> a,
    ap_uint<4> b,
    ap_uint<4> c,
    ap_uint<4> d,
    ap_uint<w> x,
    ap_uint<w> y) {
    //#pragma HLS inline

    v[a] = v[a] + v[b] + x;
    v[d] = ROTR<w, 32>(v[d] ^ v[a]);
    v[c] = v[c] + v[d];
    v[b] = ROTR<w, 24>(v[b] ^ v[c]);
    v[a] = v[a] + v[b] + y;
    v[d] = ROTR<w, 16>(v[d] ^ v[a]);
    v[c] = v[c] + v[d];
    v[b] = ROTR<w, 63>(v[b] ^ v[c]);

} // end G

/**
 * @brief Mixing 2 halves for unroll purpose.
 *
 * @tparam Bit width of the words, default value is 64.
 *
 * @param v Working vector.
 * @param vi0 The 1st index for working vector.
 * @param vi1 The 2nd index for working vector.
 * @param vi2 The 3rd index for working vector.
 * @param vi3 The 4th index for working vector.
 * @param vi4 The 5th index for working vector.
 * @param vi5 The 6th index for working vector.
 * @param vi6 The 7th index for working vector.
 * @param vi7 The 8th index for working vector.
 * @param vi8 The 9th index for working vector.
 * @param vi9 The 10th index for working vector.
 * @param vi10 The 11th index for working vector.
 * @param vi11 The 12th index for working vector.
 * @param vi12 The 13th index for working vector.
 * @param vi13 The 14th index for working vector.
 * @param vi14 The 15th index for working vector.
 * @param vi15 The 16th index for working vector.
 * @param y Message vector.
 * @param mi0 The 1st index for message vector.
 * @param mi1 The 2nd index for message vector.
 * @param mi2 The 3rd index for message vector.
 * @param mi3 The 4th index for message vector.
 * @param mi4 The 5th index for message vector.
 * @param mi5 The 6th index for message vector.
 * @param mi6 The 7th index for message vector.
 * @param mi7 The 8th index for message vector.
 *
 */

template <unsigned int w = 64>
void halfMixing(
    // in-out
    ap_uint<w> v[16],
    // inputs
    ap_uint<4> vi0,
    ap_uint<4> vi1,
    ap_uint<4> vi2,
    ap_uint<4> vi3,
    ap_uint<4> vi4,
    ap_uint<4> vi5,
    ap_uint<4> vi6,
    ap_uint<4> vi7,
    ap_uint<4> vi8,
    ap_uint<4> vi9,
    ap_uint<4> vi10,
    ap_uint<4> vi11,
    ap_uint<4> vi12,
    ap_uint<4> vi13,
    ap_uint<4> vi14,
    ap_uint<4> vi15,
    ap_uint<w> m[16],
    ap_uint<4> mi0,
    ap_uint<4> mi1,
    ap_uint<4> mi2,
    ap_uint<4> mi3,
    ap_uint<4> mi4,
    ap_uint<4> mi5,
    ap_uint<4> mi6,
    ap_uint<4> mi7) {
#pragma HLS inline off

    G<w>(v, vi0, vi1, vi2, vi3, m[mi0], m[mi1]);
    G<w>(v, vi4, vi5, vi6, vi7, m[mi2], m[mi3]);
    G<w>(v, vi8, vi9, vi10, vi11, m[mi4], m[mi5]);
    G<w>(v, vi12, vi13, vi14, vi15, m[mi6], m[mi7]);

} // end halfMixing

/**
 * @brief Compression function F as defined in standard.
 *
 * The optimization goal of this function is for better performance.
 * The algorithm reference is : "The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)".
 *
 * @tparam w Bit width of the words, default value is 64.
 * @tparam round Number of rounds, 12 for BLAKE2b and 10 for BLAKE2s.
 *
 * @param h State vector.
 * @param blake2b_iv Initialization vector.
 * @param m Message block vector.
 * @param t Offset counter.
 * @param last Final block indicator.
 *
 */

template <unsigned int w = 64, unsigned int round = 12>
void Compress(
    // in-out
    ap_uint<w> h[8],
    // inputs
    ap_uint<w> blake2b_iv[8],
    ap_uint<w> m[16],
    ap_uint<2 * w> t,
    bool last) {
    // message schedule sigma
    const ap_uint<4> sigma[12][16] = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
        {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4}, {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
        {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13}, {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
        {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11}, {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
        {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5}, {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
        {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}};
#pragma HLS array_partition variable = sigma complete dim = 1

    // working variables
    ap_uint<w> v[16];
#pragma HLS array_partition variable = v complete
LOOP_INIT_WORKING_VARIABLES:
    for (ap_uint<4> i = 0; i < 8; i++) {
#pragma HLS unroll
        v[i] = h[i];
        v[i + 8] = blake2b_iv[i];
    }
    // xor with low word of the total number of bytes
    v[12] ^= t.range(w - 1, 0);
    // high word
    v[13] ^= t.range(2 * w - 1, w);
    // invert v[14] if its final block
    if (last) {
        v[14] = ~v[14];
    }
#if !defined(__SYNTHESIS__) && __XF_SECURITY_BLAKE2B_DEBUG_ == 1
    for (unsigned int i = 0; i < 16; i++) {
        std::cout << "v[" << i << "] = " << std::hex << v[i] << std::endl;
    }
#endif

LOOP_CRYPTOGRAPHIC_MIXING:
    for (ap_uint<5> i = 0; i < round; i++) {
#pragma HLS pipeline off
        halfMixing<w>(v, 0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15, m, sigma[i][0], sigma[i][1], sigma[i][2],
                      sigma[i][3], sigma[i][4], sigma[i][5], sigma[i][6], sigma[i][7]);

        halfMixing<w>(v, 0, 5, 10, 15, 1, 6, 11, 12, 2, 7, 8, 13, 3, 4, 9, 14, m, sigma[i][8], sigma[i][9],
                      sigma[i][10], sigma[i][11], sigma[i][12], sigma[i][13], sigma[i][14], sigma[i][15]);
#if !defined(__SYNTHESIS__) && __XF_SECURITY_BLAKE2B_DEBUG_ == 1
        for (unsigned int i = 0; i < 16; i++) {
            std::cout << "v[" << i << "] = " << std::hex << v[i] << std::endl;
        }
#endif
    }

LOOP_XOR_2_HALVES:
    for (ap_uint<4> i = 0; i < 8; i++) {
#pragma HLS unroll
        h[i] ^= v[i] ^ v[i + 8];
#if !defined(__SYNTHESIS__) && __XF_SECURITY_BLAKE2B_DEBUG_ == 1
        std::cout << "h[" << i << "] = " << std::hex << h[i] << std::endl;
#endif
    }

} // end Compress

/**
 * @brief The implementation of the digest prcoess of BLAKE2.
 *
 * The algorithm reference is : "The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)".
 * The optimization goal of this function is for better performance.
 *
 * @tparam w Bit width of the words, default value is 64.
 *
 * @param blk_strm The 512-bit hash block.
 * @param nblk_strm The number of hash block for this message.
 * @param end_nblk_strm End flag for number of hash block.
 * @param key_len_strm Key length in byte (0 <= key_len <= 64).
 * @param msg_len_strm Message length in byte (0 <= msg_len <= 2^128).
 * @param out_len_strm Result hash value length in byte (0 < out_len < 64).
 * @param digest_strm The full digest stream (result is stored in the lower out_len bytes).
 * @param end_digest_strm Flag to signal the end of the result.
 *
 */

template <unsigned int w>
void blake2bDigest(
    // inputs
    hls::stream<blockType>& blk_strm,
    hls::stream<ap_uint<128> >& nblk_strm,
    hls::stream<bool>& end_nblk_strm,
    hls::stream<ap_uint<8> >& key_len_strm,
    hls::stream<ap_uint<128> >& msg_len_strm,
    hls::stream<ap_uint<8> >& out_len_strm,
    // ouputs
    hls::stream<ap_uint<8 * w> >& digest_strm,
    hls::stream<bool>& end_digest_strm) {
    // initialization vector
    ap_uint<64> blake2b_iv[8] = {0x6A09E667F3BCC908, 0xBB67AE8584CAA73B, 0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
                                 0x510E527FADE682D1, 0x9B05688C2B3E6C1F, 0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179};
#pragma HLS array_partition variable = blake2b_iv complete

    bool endFlag = end_nblk_strm.read();

LOOP_BLAKE2B_MAIN:
    while (!endFlag) {
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
        // state vector
        ap_uint<w> h[8];
#pragma HLS array_partition variable = h complete

        // read key length in byte
        ap_uint<w> key_len = key_len_strm.read();

        // read result hash value length in byte
        ap_uint<w> out_len = out_len_strm.read();

        // read message length in byte
        ap_uint<128> msg_len = msg_len_strm.read();

    // initialize state vector with initialization vector
    LOOP_INIT_STATE_VECTOR:
        for (ap_uint<4> i = 0; i < 8; i++) {
#pragma HLS unroll
            h[i] = blake2b_iv[i];
        }
        h[0] ^= ap_uint<w>(0x0000000001010000) ^ (key_len << 8) ^ out_len;

        // total number blocks to digest
        ap_uint<128> blkNum = nblk_strm.read();

        // total number of bytes
        ap_uint<2 * w> t = 0;

        // last block flag
        bool last = false;

    LOOP_BLAKE2B_DIGEST_NBLK:
        for (ap_uint<128> n = 0; n < blkNum; n++) {
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
#pragma HLS pipeline off
            // input block
            blockType blk = blk_strm.read();
#if !defined(__SYNTHESIS__) && __XF_SECURITY_BLAKE2B_DEBUG_ == 1
            for (ap_uint<5> i = 0; i < 16; i++) {
                std::cout << "M[" << i << "] = " << std::hex << blk.M[i] << std::endl;
            }
#endif

            // unkeyed hashing
            if (key_len == 0) {
                // empty message
                if (msg_len == 0) {
                    last = true;
                    t = 0;
                    // the last block
                } else if (n == (blkNum - 1)) {
                    last = true;
                    t = msg_len;
                    // still have blocks to digest
                } else {
                    t += 128;
                }
                // optional key is selected
            } else {
                // empty message
                if (msg_len == 0) {
                    last = true;
                    t = 128;
                    // the last block
                } else if (n == (blkNum - 1)) {
                    last = true;
                    t = 128 + msg_len;
                    // still have blocks to digest
                } else {
                    t += 128;
                }
            }
#if !defined(__SYNTHESIS__) && __XF_SECURITY_BLAKE2B_DEBUG_ == 1
            std::cout << "t = " << std::hex << t << std::endl;
#endif

            // hash core
            Compress<w, 12>(h, blake2b_iv, blk.M, t, last);
        }

        // emit digest
        ap_uint<512> digest;
    LOOP_EMIT_DIGEST:
        for (ap_uint<4> i = 0; i < 8; i++) {
#pragma HLS unroll
            digest.range(w * i + w - 1, w * i) = h[i];
        }
        digest_strm.write(digest);
        end_digest_strm.write(false);

        endFlag = end_nblk_strm.read();
    }

    end_digest_strm.write(true);

} // end blake2bDigest

} // namespace internal

/**
 * @brief Top function of BLAKE2B.
 *
 * The algorithm reference is : "The BLAKE2 Cryptographic Hash and Message Authentication Code (MAC)".
 * The implementation dataflows the sub-modules.
 *
 * @param msg_strm The message being hashed.
 * @param msg_len_strm Message length in byte (0 <= msg_len <= 2^128).
 * @param key_strm The optional key.
 * @param key_len_strm Key length in byte (0 <= key_len <= 64).
 * @param out_len_strm Result hash value length in byte (0 < out_len < 64).
 * @param end_len_strm The flag to signal end of input message stream.
 * @param digest_strm The digest (hash value) stream.
 * @param end_digest_strm Flag to signal the end of the result.
 *
 */

template <unsigned int w>
void blake2b(
    // inputs
    hls::stream<ap_uint<w> >& msg_strm,
    hls::stream<ap_uint<128> >& msg_len_strm,
    hls::stream<ap_uint<w> >& key_strm,
    hls::stream<ap_uint<8> >& key_len_strm,
    hls::stream<ap_uint<8> >& out_len_strm,
    hls::stream<bool>& end_len_strm,
    // ouputs
    hls::stream<ap_uint<8 * w> >& digest_strm,
    hls::stream<bool>& end_digest_strm) {
#pragma HLS dataflow

    // 1024-bit processing block stream
    hls::stream<internal::blockType> blk_strm("blk_strm");
#pragma HLS stream variable = blk_strm depth = 32
#pragma HLS resource variable = blk_strm core = FIFO_LUTRAM

    // number of blocks stream
    hls::stream<ap_uint<128> > nblk_strm("nblk_strm");
#pragma HLS stream variable = nblk_strm depth = 32
#pragma HLS resource variable = nblk_strm core = FIFO_LUTRAM

    // end flag of number of blocks stream
    hls::stream<bool> end_nblk_strm("end_nblk_strm");
#pragma HLS stream variable = end_nblk_strm depth = 32
#pragma HLS resource variable = end_nblk_strm core = FIFO_LUTRAM

    // key length stream from generateBlock to blake2bDigest
    hls::stream<ap_uint<8> > key_len_out_strm("key_len_out_strm");
#pragma HLS stream variable = key_len_out_strm depth = 32
#pragma HLS resource variable = key_len_out_strm core = FIFO_LUTRAM

    // message length stream from generateBlock to blake2bDigest
    hls::stream<ap_uint<128> > msg_len_out_strm("msg_len_out_strm");
#pragma HLS stream variable = msg_len_out_strm depth = 32
#pragma HLS resource variable = msg_len_out_strm core = FIFO_LUTRAM

    // padding key (optional) and message words into blocks
    internal::generateBlock<w>(msg_strm, msg_len_strm, key_strm, key_len_strm, end_len_strm,            // in
                               blk_strm, nblk_strm, end_nblk_strm, msg_len_out_strm, key_len_out_strm); // out

    // digest processing blocks into hash value
    internal::blake2bDigest<w>(blk_strm, nblk_strm, end_nblk_strm, key_len_out_strm, msg_len_out_strm,
                               out_len_strm,                  // in
                               digest_strm, end_digest_strm); // out

} // end blake2b

} // namespace security
} // namespace xf

#endif
