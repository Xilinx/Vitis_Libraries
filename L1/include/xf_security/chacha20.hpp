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
 * @file chacha20.hpp
 * @brief header file for ChaCha20.
 * This file part of XF Security Library.
 *
 */

#ifndef _XF_SECURITY_CHACHA20_HPP_
#define _XF_SECURITY_CHACHA20_HPP_

#include <ap_int.h>
#include <hls_stream.h>

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
#include <iostream>
#endif

namespace xf {
namespace security {
namespace internal {

typedef ap_uint<512> blockTypeChacha;

#define ROTL(a, b) (((a) << (b)) | ((a) >> (32 - (b))))
#define QR(a, b, c, d)   \
    do {                 \
        a += b;          \
        d ^= a;          \
        d = ROTL(d, 16); \
        c += d;          \
        b ^= c;          \
        b = ROTL(b, 12); \
        a += b;          \
        d ^= a;          \
        d = ROTL(d, 8);  \
        c += d;          \
        b ^= c;          \
        b = ROTL(b, 7);  \
    } while (0);
#define ROUNDS 20

/**
 * @brief chachaEncrpt encrpyts the plain text.
 *
 * @param stateStrm input state stream only once
 * @param packPlainStrm input plain text stream
 * @param numPackPlainStrm the number of effective bytes in each packed data
 * @param ePackPlainStrm the end flag of packPlainStrm
 *
 * @param packCipherStrm output cipher stream
 * @param numPackCipherStrm the number of effective bytes in each packed data
 * @param ePackCipherStrm the end flag of packCipherStrm
 */
void chachaEncrpt(hls::stream<blockTypeChacha>& stateStrm,
                  hls::stream<blockTypeChacha>& packPlainStrm,
                  hls::stream<ap_uint<8> >& numPackPlainStrm,
                  hls::stream<bool>& ePackPlainStrm,
                  hls::stream<blockTypeChacha>& packCipherStrm,
                  hls::stream<ap_uint<8> >& numPackCipherStrm,
                  hls::stream<bool>& ePackCipherStrm) {
    ap_uint<32> x[16];
#pragma HLS array_partition variable = x complete
    ap_uint<32> xs[16];
#pragma HLS array_partition variable = xs complete
    blockTypeChacha cph;

    blockTypeChacha s;
    s = stateStrm.read();
    ap_uint<32> c = s.range(13 * 32 - 1, 12 * 32);
    while (!ePackPlainStrm.read()) {
        for (int i = 0; i < 16; ++i) {
#pragma HLS unroll
            if (i == 12)
                x[12] = c++;
            else
                x[i] = s.range(i * 32 + 31, i * 32);
        } // for
        // x[12]=c++;
        ap_uint<8> num = numPackPlainStrm.read();
        blockTypeChacha plainData = packPlainStrm.read();
        numPackCipherStrm.write(num);
        // 10 loops * 2 rounds/loop = 20 rounds
        for (int i = 0; i < ROUNDS; i += 2) {
#pragma HLS pipeline
            // Odd round
            QR(x[0], x[4], x[8], x[12]);  // column 0
            QR(x[1], x[5], x[9], x[13]);  // column 1
            QR(x[2], x[6], x[10], x[14]); // column 2
            QR(x[3], x[7], x[11], x[15]); // column 3
            // Even round
            QR(x[0], x[5], x[10], x[15]); // diagonal 1 (main diagonal)
            QR(x[1], x[6], x[11], x[12]); // diagonal 2
            QR(x[2], x[7], x[8], x[13]);  // diagonal 3
            QR(x[3], x[4], x[9], x[14]);  // diagonal 4
        }                                 // for

#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
        std::cout << std::endl << "keystream_" << std::dec << (c - 1) << std::endl;
#endif
        for (int i = 0; i < 16; ++i) {
#pragma HLS unroll
            if (i == 12)
                xs[i] = x[i] + c - 1; // s.range(i*32+31, i*32);
            else
                xs[i] = x[i] + s.range(i * 32 + 31, i * 32);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
            std::cout << std::hex << std::setw(2) << std::setfill('0') << xs[i];
#endif
            cph.range(i * 32 + 31, i * 32) = plainData.range(i * 32 + 31, i * 32) ^ xs[i];
        } // for
        packCipherStrm.write(cph);
        ePackCipherStrm.write(false);
    } // while
    ePackCipherStrm.write(true);
#if !defined(__SYNTHESIS__) && XF_SECURITY_DECRYPT_DEBUG == 1
    std::cout << std::endl;
#endif
}

/**
 * @brief generateBlock generates the state matrix from initial key and counter.
 * state matrix:
 *  s[0]   s[1]   s[2]   s[3]
 *  s[4]   s[5]   s[6]   s[7]
 *  s[8]   s[9]   s[10]  s[11]
 *  s[12]  s[13]  s[14]  s[15]
 *
 *
 *  the layout of the data from counteStrm
 *   0-31  bit: counter  s[12]
 *  32-63  bit: nonce1   s[13]
 *  64-95  bit: nonce2   s[14]
 *  96-127 bit: nonce3   s[15]
 * @param keyStrm initial key
 * @param counterNonceStm initial counter and nonce
 * @param stateStrm  output state stream
 *
 */
void generateBlock(hls::stream<ap_uint<256> >& keyStrm,
                   hls::stream<ap_uint<128> >& counterNonceStrm,
                   hls::stream<blockTypeChacha>& stateStrm

                   ) {
    ap_uint<32> input[16];
#pragma HLS array_partition variable = input complete
    /* sigma constant "expand 32-byte k" in little-endian encoding */
    /*  input[0] = ((u32)'e') | ((u32)'x'<<8) | ((u32)'p'<<16) | ((u32)'a'<<24);
        input[1] = ((u32)'n') | ((u32)'d'<<8) | ((u32)' '<<16) | ((u32)'3'<<24);
        input[2] = ((u32)'2') | ((u32)'-'<<8) | ((u32)'b'<<16) | ((u32)'y'<<24);
        input[3] = ((u32)'t') | ((u32)'e'<<8) | ((u32)' '<<16) | ((u32)'k'<<24);
    */
    input[0] = 0x61707865;
    input[1] = 0x3320646e;
    input[2] = 0x79622d32;
    input[3] = 0x6b206574;
    ap_uint<256> key = keyStrm.read();
    for (int i = 0; i < 8; ++i) {
#pragma HLS unroll
        input[i + 4] = key.range(32 * (i + 1) - 1, i * 32);
    }
    ap_uint<128> counter = counterNonceStrm.read();
    for (int i = 0; i < 4; ++i) {
#pragma HLS unroll
        input[i + 12] = counter.range(i * 32 + 31, i * 32);
    }
    blockTypeChacha s;
    for (int i = 0; i < 16; ++i) {
#pragma HLS unroll
        s.range(i * 32 + 31, i * 32) = input[i];
    }

    stateStrm.write(s);
}
/**
 * @brief packMsg converts a few of bytes to a big width data.
 *
 * @param plainStrm input stream with byte
 * @param ePlainStrm end of plainStrm
 * @param packPlainStrm output big width stream
 * @param numPackPlainStrm the number of effective bytes in each packed data
 * @param ePackPlainStrm the end flag of packPlainStrm
 *
 */
void packMsg(hls::stream<ap_uint<8> >& plainStrm,
             hls::stream<bool>& ePlainStrm,
             hls::stream<blockTypeChacha>& packPlainStrm,
             hls::stream<ap_uint<8> >& numPackPlainStrm,
             hls::stream<bool>& ePackPlainStrm) {
    int cnt = 0;
    int counter = 0;
    int j = 0;
    blockTypeChacha x;
    while (!ePlainStrm.read()) {
#pragma HLS pipeline II = 1
        x.range(j * 8 + 7, j * 8) = plainStrm.read();
        if (++j == 64) {
            packPlainStrm.write(x);
            numPackPlainStrm.write(64);
            ePackPlainStrm.write(false);
            j = 0;
        }
    }
    if (j != 0) {
        packPlainStrm.write(x);
        numPackPlainStrm.write(j);
        ePackPlainStrm.write(false);
    }
    ePackPlainStrm.write(true);
}

/**
 * @brief convert2Byte converts big width data to a few of bytes.
 *
 * @param packStrm a big width stream
 * @param numPackStrm each element contains the number of effective bytes
 * @param ePackStrm the end flag of packStrm
 * @param byteStrm  output byte stream
 * @param eByteStrm the end flag of byteStrm
 *
 */
void convert2Byte(hls::stream<blockTypeChacha>& packStrm,
                  hls::stream<ap_uint<8> >& numPackStrm,
                  hls::stream<bool>& ePackStrm,
                  hls::stream<ap_uint<8> >& byteStrm,
                  hls::stream<bool>& eByteStrm) {
    while (!ePackStrm.read()) {
        ap_uint<8> num = numPackStrm.read();
        blockTypeChacha block = packStrm.read();
        for (ap_uint<8> i = 0; i < 64; i++) {
#pragma HLS pipeline II = 1
            if (i < num) {
                ap_uint<8> d = block.range(i * 8 + 7, i * 8);
                byteStrm.write(d);
                eByteStrm.write(false);
            }
        } // for

    } // while
    eByteStrm.write(true);
}
/**
 * @brief chacha20 is a function for stream ciphering
 *
 * @param keyStrm initail key
 * @param counterNonceStm initial counter and nonce
 * @param plainStrm input  plain text to be encrypted
 * @param ePlainStrm the end flag of plainStrm
 * @param cipherStrm  output encrypted text
 * @param eCipherStrm the end flag of cipherStrm
 *
 */
void chacha20Imp(hls::stream<ap_uint<256> >& keyStrm,
                 hls::stream<ap_uint<128> >& counterNonceStrm,
                 hls::stream<ap_uint<8> >& plainStrm,
                 hls::stream<bool>& ePlainStrm,
                 hls::stream<ap_uint<8> >& cipherStrm,
                 hls::stream<bool>& eCipherStrm) {
    hls::stream<blockTypeChacha> stateStrm;
#pragma HLS stream variable = stateStrm depth = 32
    hls::stream<blockTypeChacha> packPlainStrm;
#pragma HLS stream variable = packPlainStrm depth = 32
    hls::stream<ap_uint<8> > numPackPlainStrm;
#pragma HLS stream variable = numPackPlainStrm depth = 32
    hls::stream<bool> ePackPlainStrm;
#pragma HLS stream variable = ePackPlainStrm depth = 32
    hls::stream<blockTypeChacha> packCipherStrm;
#pragma HLS stream variable = packCipherStrm depth = 32
    hls::stream<ap_uint<8> > numPackCipherStrm;
#pragma HLS stream variable = numPackCipherStrm depth = 32
    hls::stream<bool> ePackCipherStrm;
#pragma HLS stream variable = ePackCipherStrm depth = 32

#pragma HLS dataflow
    generateBlock(keyStrm, counterNonceStrm, stateStrm);

    packMsg(plainStrm, ePlainStrm, packPlainStrm, numPackPlainStrm, ePackPlainStrm);

    chachaEncrpt(stateStrm, packPlainStrm, numPackPlainStrm, ePackPlainStrm, packCipherStrm, numPackCipherStrm,
                 ePackCipherStrm);

    convert2Byte(packCipherStrm, numPackCipherStrm, ePackCipherStrm, cipherStrm, eCipherStrm);
}

} // end of namespace internal

/**
 * @brief chahcha20 is a basic function for stream ciphering
 * when key is "keylayout-chacha", its layout in a 256-bit ap_uint<> likes this,
 *
 *   0 -  7  bit:   'k'
 *   8 - 15  bit:   'e'
 *  16 - 23  bit:   'y'
 *  24 - 31  bit:   'l'
 *    ...
 *  232- 239 bit:   'c'
 *  240- 247 bit:   'h'
 *  248- 255 bit:   'a'
 *
 * state matrix:
 *  s[0]   s[1]   s[2]   s[3]
 *  s[4]   s[5]   s[6]   s[7]
 *  s[8]   s[9]   s[10]  s[11]
 *  s[12]  s[13]  s[14]  s[15]
 *
 *
 * 128bits counterNonceStrm = counter 32 bits + nonce 96 bits
 *  the layout of the data from counteStrm
 *   0-31  bit: counter  s[12]
 *  32-63  bit: nonce1   s[13]
 *  64-95  bit: nonce2   s[14]
 *  96-127 bit: nonce3   s[15]
 *
 * @param keyStrm initail key
 * @param counterNonceStm initial counter and nonce
 * @param plainStrm input  plain text to be encrypted
 * @param ePlainStrm the end flag of plainStrm
 * @param cipherStrm  output encrypted text
 * @param eCipherStrm the end flag of cipherStrm
 */
void chacha20(hls::stream<ap_uint<256> >& keyStrm,
              hls::stream<ap_uint<128> >& counterNonceStrm,
              hls::stream<ap_uint<8> >& plainStrm,
              hls::stream<bool>& ePlainStrm,
              hls::stream<ap_uint<8> >& cipherStrm,
              hls::stream<bool>& eCipherStrm) {
    internal::chacha20Imp(keyStrm, counterNonceStrm, plainStrm, ePlainStrm, cipherStrm, eCipherStrm);
}

} // end of namespace security
} // end of namespace xf
#endif // _XF_SECURITY_CHACHA20_HPP_
