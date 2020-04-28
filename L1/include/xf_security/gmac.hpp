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
 * @file gmac.hpp
 * @brief header file for Advanced Encryption Standard related working mode GMAC.
 * This file is part of Vitis Security Library.
 *
 * @detail Containing multiplication in GF128 field, preMAC function to generate E(K,Y0) and hash subkey, and 2
 * overloads for GCM and GMAC.
 */

#ifndef _XF_SECURITY_GMAC_HPP_
#define _XF_SECURITY_GMAC_HPP_

#include <ap_int.h>
#include <hls_stream.h>

#include "aes.hpp"

// for debug
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace security {
namespace internal {

/**
 *
 * @brief Calculate Z = X * Y in Galois Field(2^128)
 *
 * The algorithm reference is: "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param X The first operand.
 * @param Y The second operand.
 * @param Z The product.
 *
 */

static void GF128_mult(
    // input port
    ap_uint<128> X,
    ap_uint<128> Y,
    // output port
    ap_uint<128>& Z) {
    // register and reshape input X
    ap_uint<128> V = X;
    V.range(127, 120) = X.range(7, 0);
    V.range(119, 112) = X.range(15, 8);
    V.range(111, 104) = X.range(23, 16);
    V.range(103, 96) = X.range(31, 24);
    V.range(95, 88) = X.range(39, 32);
    V.range(87, 80) = X.range(47, 40);
    V.range(79, 72) = X.range(55, 48);
    V.range(71, 64) = X.range(63, 56);
    V.range(63, 56) = X.range(71, 64);
    V.range(55, 48) = X.range(79, 72);
    V.range(47, 40) = X.range(87, 80);
    V.range(39, 32) = X.range(95, 88);
    V.range(31, 24) = X.range(103, 96);
    V.range(23, 16) = X.range(111, 104);
    V.range(15, 8) = X.range(119, 112);
    V.range(7, 0) = X.range(127, 120);
    V.reverse();

    // register and reshape input Y
    ap_uint<128> Y_r = Y;
    Y_r.range(127, 120) = Y.range(7, 0);
    Y_r.range(119, 112) = Y.range(15, 8);
    Y_r.range(111, 104) = Y.range(23, 16);
    Y_r.range(103, 96) = Y.range(31, 24);
    Y_r.range(95, 88) = Y.range(39, 32);
    Y_r.range(87, 80) = Y.range(47, 40);
    Y_r.range(79, 72) = Y.range(55, 48);
    Y_r.range(71, 64) = Y.range(63, 56);
    Y_r.range(63, 56) = Y.range(71, 64);
    Y_r.range(55, 48) = Y.range(79, 72);
    Y_r.range(47, 40) = Y.range(87, 80);
    Y_r.range(39, 32) = Y.range(95, 88);
    Y_r.range(31, 24) = Y.range(103, 96);
    Y_r.range(23, 16) = Y.range(111, 104);
    Y_r.range(15, 8) = Y.range(119, 112);
    Y_r.range(7, 0) = Y.range(127, 120);
    Y_r.reverse();

    // calculate multiplication product
    ap_uint<128> P = 0;
LOOP_MULT:
    for (int i = 0; i < 128; i++) {
#pragma HLS pipeline II = 1
        if (1 == Y_r[i]) {
            P = P ^ V;
        }
        if (0 == V[127]) {
            V = V << 1;
        } else {
            V = V << 1;
            V.range(127, 64) = V.range(127, 64) ^ 0x0000000000000000;
            V.range(63, 0) = V.range(63, 0) ^ 0x0000000000000087;
        }
    }

    // reshape and write out the result
    P.reverse();
    Z.range(127, 120) = P.range(7, 0);
    Z.range(119, 112) = P.range(15, 8);
    Z.range(111, 104) = P.range(23, 16);
    Z.range(103, 96) = P.range(31, 24);
    Z.range(95, 88) = P.range(39, 32);
    Z.range(87, 80) = P.range(47, 40);
    Z.range(79, 72) = P.range(55, 48);
    Z.range(71, 64) = P.range(63, 56);
    Z.range(63, 56) = P.range(71, 64);
    Z.range(55, 48) = P.range(79, 72);
    Z.range(47, 40) = P.range(87, 80);
    Z.range(39, 32) = P.range(95, 88);
    Z.range(31, 24) = P.range(103, 96);
    Z.range(23, 16) = P.range(111, 104);
    Z.range(15, 8) = P.range(119, 112);
    Z.range(7, 0) = P.range(127, 120);

} // end GF128_mult

/**
 *
 * @brief preGMAC generates H and E_K_Y0 based on AES block cipher.
 *
 * The algorithm reference is: "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for less resource utilizations while having a reasonable latency.
 *
 * @tparam _keyWidth	The bit-width of the cipherkey.
 *
 * @param cipherkeyStrm Input cipherkey, x bits for AES-x.
 * @param IVStrm		Initialization vector for the fisrt iteration of AES encrypition.
 * @param HStrm			The hash key.
 * @param EKY0Strm		E(K, Y0) as specified in the standard.
 * @param lenPldStrm	Length of the payload stream (hard-code to zero).
 *
 */

template <unsigned int _keyWidth>
void preGMAC(hls::stream<ap_uint<_keyWidth> >& cipherkeyStrm,
             hls::stream<ap_uint<96> >& IVStrm,
             hls::stream<ap_uint<128> >& HStrm,
             hls::stream<ap_uint<128> >& EKY0Strm) {
#pragma HLS allocation instances = updateKey limit = 1 function
#pragma HLS allocation instances = process limit = 1 function

    // register cipherkeyStrm
    ap_uint<_keyWidth> key = cipherkeyStrm.read();
    xf::security::aesEnc<_keyWidth> cipher;
    cipher.updateKey(key);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
    std::cout << std::endl << "cipherkey = " << std::hex << key << std::endl;
#endif

    // generate initial counter block
    ap_uint<128> Y0;
    // XXX: the bit-width of IV is restricted to 96 bits in this implementation
    ap_uint<96> IV = IVStrm.read();
    Y0.range(95, 0) = IV.range(95, 0);
    Y0.range(127, 96) = 0x01000000;
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
    std::cout << "Y0 = " << std::hex << Y0 << std::endl;
#endif

    ap_uint<128> inputBlock = 0;
    ap_uint<128> outputBlock = 0;
LOOP_GEN:
    for (int i = 0; i < 2; i++) {
#pragma HLS pipeline II = 1
        inputBlock = (i == 0) ? inputBlock : Y0;
        cipher.process(inputBlock, key, outputBlock);
        if (i == 0) {
            HStrm.write(outputBlock);
        } else {
            EKY0Strm.write(outputBlock);
        }
    }

} // end preGMAC

/**
 *
 * @brief genGMAC This function calculates the MAC using AAD and payload streams.
 * This overload is used by GCM.
 *
 * The algorithm reference is: "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for less resource utilizations while having a reasonable latency.
 *
 * @param AADStrm		Additional authenticated data stream.
 * @param lenAADStrm	Length of AAD in bits.
 * @param pldStrm		Payload stream.
 * @param lenPldStrm	Length of the payload in bits.
 * @param HStrm			The hash key.
 * @param EKY0Strm		E(K, Y0) as specified in the standard.
 * @param endLenStrm	Flag to signal the end of the length streams.
 * @param tagStrm		The MAC.
 * @param endTagStrm	Flag to signal the end of the MAC stream.
 *
 */

void genGMAC(hls::stream<ap_uint<128> >& AADStrm,
             hls::stream<ap_uint<64> >& lenAADStrm,
             hls::stream<ap_uint<128> >& pldStrm,
             hls::stream<ap_uint<64> >& lenPldStrm,
             hls::stream<ap_uint<128> >& HStrm,
             hls::stream<ap_uint<128> >& EKY0Strm,
             hls::stream<bool>& endLenStrm,
             hls::stream<ap_uint<128> >& tagStrm,
             hls::stream<bool>& endTagStrm) {
#pragma HLS allocation instances = GF128_mult limit = 1 function

    bool end = endLenStrm.read();

    while (!end) {
#pragma HLS loop_tripcount min = 1 max = 1 avg = 1
        // register lenAAD
        ap_uint<64> lenAAD = lenAADStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "lenAAD = " << std::hex << lenAAD << std::endl;
#endif

        // register hash key
        ap_uint<128> H = HStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "H = " << std::hex << H << std::endl;
#endif

        // register E(K,Y0)
        ap_uint<128> EKY0 = EKY0Strm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "E(K,Y0) = " << std::hex << EKY0 << std::endl;
#endif

        ap_uint<128> multIn = 0;
        ap_uint<128> multResult = 0;

    LOOP_FIRST_HALF:
        for (ap_uint<64> i = 0; i < lenAAD / 128; i++) {
#pragma HLS pipeline
            ap_uint<128> AAD = AADStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "AAD = " << std::hex << AAD << std::endl;
#endif

            // calculate the input for GHASH
            multIn = multResult ^ AAD;

            // calculate GHASH
            GF128_mult(multIn, H, multResult);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "X = " << std::hex << multResult << std::endl;
#endif
        }

        // we didn't hit the block boundary of the AAD
        if ((lenAAD % 128) > 0) {
            ap_uint<128> AAD = AADStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "AAD = " << std::hex << AAD << std::endl;
#endif
            AAD.range(127, lenAAD % 128) = 0;

            // calculate the input for GHASH
            multIn = multResult ^ AAD;

            // calculate GHASH
            GF128_mult(multIn, H, multResult);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "X = " << std::hex << multResult << std::endl;
#endif
        }

        // register lenPld
        ap_uint<64> lenPld = lenPldStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "lenPld = " << std::hex << lenPld << std::endl;
#endif

    LOOP_SECOND_HALF:
        for (ap_uint<64> i = 0; i < lenPld / 128; i++) {
#pragma HLS pipeline
            ap_uint<128> pld = pldStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "pld = " << std::hex << pld << std::endl;
#endif

            // calculate the input for GHASH
            multIn = multResult ^ pld;

            // calculate GHASH
            GF128_mult(multIn, H, multResult);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "X = " << std::hex << multResult << std::endl;
#endif
        }

        // we didn't hit the block boundary of the payload
        if ((lenPld % 128) > 0) {
            ap_uint<128> pld = pldStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "pld = " << std::hex << pld << std::endl;
#endif
            pld.range(127, lenPld % 128) = 0;

            // calculate the input for GHASH
            multIn = multResult ^ pld;

            // calculate GHASH
            GF128_mult(multIn, H, multResult);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
            std::cout << "X = " << std::hex << multResult << std::endl;
#endif
        }

        // concatenate the lengths into 1
        ap_uint<128> lenAC = 0;

        // calculate len(A)||len(C)
        lenAC.range(127, 120) = lenPld.range(7, 0);
        lenAC.range(119, 112) = lenPld.range(15, 8);
        lenAC.range(111, 104) = lenPld.range(23, 16);
        lenAC.range(103, 96) = lenPld.range(31, 24);
        lenAC.range(95, 88) = lenPld.range(39, 32);
        lenAC.range(87, 80) = lenPld.range(47, 40);
        lenAC.range(79, 72) = lenPld.range(55, 48);
        lenAC.range(71, 64) = lenPld.range(63, 56);
        lenAC.range(63, 56) = lenAAD.range(7, 0);
        lenAC.range(55, 48) = lenAAD.range(15, 8);
        lenAC.range(47, 40) = lenAAD.range(23, 16);
        lenAC.range(39, 32) = lenAAD.range(31, 24);
        lenAC.range(31, 24) = lenAAD.range(39, 32);
        lenAC.range(23, 16) = lenAAD.range(47, 40);
        lenAC.range(15, 8) = lenAAD.range(55, 48);
        lenAC.range(7, 0) = lenAAD.range(63, 56);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "lenAC = " << std::hex << lenAC << std::endl;
#endif

        // calculate the input for GHASH
        multIn = multResult ^ lenAC;

        // calculate GHASH
        GF128_mult(multIn, H, multResult);

        // calculate the MAC
        ap_uint<128> tag = multResult ^ EKY0;

        // emit MAC
        tagStrm.write(tag);
        endTagStrm.write(false);

        // last message?
        end = endLenStrm.read();
    }

    endTagStrm.write(true);

} // end genGMAC

/**
 *
 * @brief genGMAC This function calculates the MAC using only AAD stream.
 * This overload is used by GMAC.
 *
 * The algorithm reference is: "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for less resource utilizations while having a reasonable latency.
 *
 * @param AADStrm		Additional authenticated data stream.
 * @param lenAADStrm	Length of AAD in bits.
 * @param HStrm			The hash key.
 * @param EKY0Strm		E(K, Y0) as specified in the standard.
 * @param tagStrm		The MAC.
 *
 */

void genGMAC(hls::stream<ap_uint<128> >& AADStrm,
             hls::stream<ap_uint<64> >& lenAADStrm,
             hls::stream<ap_uint<128> >& HStrm,
             hls::stream<ap_uint<128> >& EKY0Strm,
             hls::stream<ap_uint<128> >& tagStrm) {
#pragma HLS allocation instances = GF128_mult limit = 1 function

    // register lenAAD
    ap_uint<64> lenAAD = lenAADStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
    std::cout << "lenAAD = " << std::hex << lenAAD << std::endl;
#endif

    // register hash key
    ap_uint<128> H = HStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
    std::cout << "H = " << std::hex << H << std::endl;
#endif

    // register E(K,Y0)
    ap_uint<128> EKY0 = EKY0Strm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
    std::cout << "E(K,Y0) = " << std::hex << EKY0 << std::endl;
#endif

    ap_uint<128> multIn = 0;
    ap_uint<128> multResult = 0;

LOOP_FIRST_HALF:
    for (ap_uint<64> i = 0; i < lenAAD / 128; i++) {
#pragma HLS pipeline
        ap_uint<128> AAD = AADStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "AAD = " << std::hex << AAD << std::endl;
#endif

        // calculate the input for GHASH
        multIn = multResult ^ AAD;

        // calculate GHASH
        GF128_mult(multIn, H, multResult);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "X = " << std::hex << multResult << std::endl;
#endif
    }

    // we didn't hit the block boundary of the AAD
    if ((lenAAD % 128) > 0) {
        ap_uint<128> AAD = AADStrm.read();
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "AAD = " << std::hex << AAD << std::endl;
#endif
        AAD.range(127, lenAAD % 128) = 0;

        // calculate the input for GHASH
        multIn = multResult ^ AAD;

        // calculate GHASH
        GF128_mult(multIn, H, multResult);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
        std::cout << "X = " << std::hex << multResult << std::endl;
#endif
    }

    // concatenate the lengths into 1
    ap_uint<128> lenAC = 0;

    // calculate len(A)||len(C)
    lenAC.range(127, 64) = 0;
    lenAC.range(63, 56) = lenAAD.range(7, 0);
    lenAC.range(55, 48) = lenAAD.range(15, 8);
    lenAC.range(47, 40) = lenAAD.range(23, 16);
    lenAC.range(39, 32) = lenAAD.range(31, 24);
    lenAC.range(31, 24) = lenAAD.range(39, 32);
    lenAC.range(23, 16) = lenAAD.range(47, 40);
    lenAC.range(15, 8) = lenAAD.range(55, 48);
    lenAC.range(7, 0) = lenAAD.range(63, 56);
#if !defined(__SYNTHESIS__) && (_XF_SECURITY_GMAC_DEBUG_ == 1)
    std::cout << "lenAC = " << std::hex << lenAC << std::endl;
#endif

    // calculate the input for GHASH
    multIn = multResult ^ lenAC;

    // calculate GHASH
    GF128_mult(multIn, H, multResult);

    // calculate the MAC
    ap_uint<128> tag = multResult ^ EKY0;
    tagStrm.write(tag);

} // end genGMAC

/**
 *
 * @brief gmac The top of GMAC.
 *
 * Galois Message Authentication Code (GMAC) is a mechanism to provide data origin authentication.
 *
 * @tparam _keyWidth	The bit-width of the cipherkey.
 *
 * @param dataStrm Input text stream to be authenticated.
 * @param lenDataStrm The length of the data in bits.
 * @param cipherkeyStrm Input cihperkey to calculate the hash subkey and E(K,Y0).
 * @param IVStrm Initialization vector.
 * @param tagStrm The MAC stream.
 *
 */

template <unsigned int _keyWidth>
void gmacTop(hls::stream<ap_uint<128> >& dataStrm,
             hls::stream<ap_uint<64> >& lenDataStrm,
             hls::stream<ap_uint<_keyWidth> >& cipherkeyStrm,
             hls::stream<ap_uint<96> >& IVStrm,
             hls::stream<ap_uint<128> >& tagStrm) {
#pragma HLS DATAFLOW

    // the hash subkey
    hls::stream<ap_uint<128> > HStrm("HStrm");
#pragma HLS RESOURCE variable = HStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = HStrm depth = 32 dim = 1

    // the E(K,Y0)
    hls::stream<ap_uint<128> > EKY0Strm("EKY0Strm");
#pragma HLS RESOURCE variable = EKY0Strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = EKY0Strm depth = 32 dim = 1

    preGMAC<_keyWidth>(cipherkeyStrm, IVStrm, HStrm, EKY0Strm);

    genGMAC(dataStrm, lenDataStrm, HStrm, EKY0Strm, tagStrm);

} // end gmacTop

} // namespace internal

/**
 *
 * @brief GMAC using AES-128 block cipher.
 *
 * Galois Message Authentication Code (GMAC) is a mechanism to provide data origin authentication.
 *
 * @param dataStrm Input text stream to be authenticated.
 * @param lenDataStrm The length of the data in bits.
 * @param cipherkeyStrm Input cihperkey to calculate the hash subkey and E(K,Y0).
 * @param IVStrm Initialization vector.
 * @param tagStrm The MAC stream.
 *
 */

void aes128Gmac(hls::stream<ap_uint<128> >& dataStrm,
                hls::stream<ap_uint<64> >& lenDataStrm,
                hls::stream<ap_uint<128> >& cipherkeyStrm,
                hls::stream<ap_uint<96> >& IVStrm,
                hls::stream<ap_uint<128> >& tagStrm) {
    internal::gmacTop<128>(dataStrm, lenDataStrm, cipherkeyStrm, IVStrm, tagStrm);
} // end aes128Gmac

/**
 *
 * @brief GMAC using AES-192 block cipher.
 *
 * Galois Message Authentication Code (GMAC) is a mechanism to provide data origin authentication.
 *
 * @param dataStrm Input text stream to be authenticated.
 * @param lenDataStrm The length of the data in bits.
 * @param cipherkeyStrm Input cihperkey to calculate the hash subkey and E(K,Y0).
 * @param IVStrm Initialization vector.
 * @param tagStrm The MAC stream.
 *
 */

void aes192Gmac(hls::stream<ap_uint<128> >& dataStrm,
                hls::stream<ap_uint<64> >& lenDataStrm,
                hls::stream<ap_uint<192> >& cipherkeyStrm,
                hls::stream<ap_uint<96> >& IVStrm,
                hls::stream<ap_uint<128> >& tagStrm) {
    internal::gmacTop<192>(dataStrm, lenDataStrm, cipherkeyStrm, IVStrm, tagStrm);
} // end aes192Gmac

/**
 *
 * @brief GMAC using AES-256 block cipher.
 *
 * Galois Message Authentication Code (GMAC) is a mechanism to provide data origin authentication.
 *
 * @param dataStrm Input text stream to be authenticated.
 * @param lenDataStrm The length of the data in bits.
 * @param cipherkeyStrm Input cihperkey to calculate the hash subkey and E(K,Y0).
 * @param IVStrm Initialization vector.
 * @param tagStrm The MAC stream.
 *
 */

void aes256Gmac(hls::stream<ap_uint<128> >& dataStrm,
                hls::stream<ap_uint<64> >& lenDataStrm,
                hls::stream<ap_uint<256> >& cipherkeyStrm,
                hls::stream<ap_uint<96> >& IVStrm,
                hls::stream<ap_uint<128> >& tagStrm) {
    internal::gmacTop<256>(dataStrm, lenDataStrm, cipherkeyStrm, IVStrm, tagStrm);
} // end aes256Gmac

} // namespace security
} // namespace xf

#endif // XF_SECURITY_GMAC_HPP_
