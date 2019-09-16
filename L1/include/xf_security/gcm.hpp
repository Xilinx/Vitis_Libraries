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
 * @file gcm.hpp
 * @brief header file for Galois/Counter Mode (GCM) block cipher mode of operation.
 * This file is part of XF Security Library.
 *
 * @detail Containing GCM mode with AES-128/192/256.
 * GCM = CTR + GMAC.
 *
 */

#ifndef _XF_SECURITY_GCM_HPP_
#define _XF_SECURITY_GCM_HPP_

#include <ap_int.h>
#include <hls_stream.h>

#include "aes.hpp"

// for debug
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace security {
namespace details {

// @brief Encrypt plaintext to cihpertext
template <unsigned int _keyWidth = 256>
static void ciphertext_gen(
    // stream in
    hls::stream<ap_uint<128> >& plaintext,
    hls::stream<bool>& plaintext_e,
    // input cipherkey and initilization vector
    hls::stream<ap_uint<_keyWidth> >& cipherkey,
    hls::stream<ap_uint<96> >& IV_strm,
    // output hash key and E(K,Y0)
    hls::stream<ap_uint<128> >& H_strm,
    hls::stream<ap_uint<128> >& E_K_Y0_strm,
    // stream out
    hls::stream<ap_uint<128> >& ciphertext,
    hls::stream<bool>& ciphertext_e) {
#pragma HLS allocation instances = aesEncrypt limit = 1 function

    // register cipherkey
    ap_uint<_keyWidth> K = cipherkey.read();
#ifndef __SYNTHESIS__
    std::cout << std::endl << "cipherkey = " << std::hex << K << std::endl;
#endif
    xf::security::aesEnc<_keyWidth> cipher;
    cipher.updateKey(K);
    // generate initial counter block
    ap_uint<128> Y0;
    Y0.range(95, 0) = IV_strm.read();
    Y0.range(127, 96) = 0x01000000;
#ifndef __SYNTHESIS__
    std::cout << "Y0 = " << std::hex << Y0 << std::endl;
#endif

    // the hash key
    ap_uint<128> H;

    // the E(K,Y0)
    ap_uint<128> E_K_Y0;

    // intermediate registers to perform the CTR encryption chain
    ap_uint<128> plaintext_r = 0;
    ap_uint<128> input_block = 0;
    ap_uint<128> input_block_r = 0;
    ap_uint<128> output_block = 0;
    ap_uint<128> ciphertext_r = 0;

    // set the iteration controlling flag
    bool first = true;
    bool second = false;
    bool initialization = false;

    bool e = plaintext_e.read();

ciphertext_gen_loop:
    while (!e) {
#pragma HLS PIPELINE II = 1
        // read a block of plaintext, 128 bits
        if (!first && !second) {
            plaintext_r = plaintext.read();
        }
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "plaintext    = " << std::hex << plaintext_r << std::endl;
#endif

        // calculate input_block
        if (first) { // the first iteration calculate the hash key
            input_block.range(127, 64) = 0x0000000000000000;
            input_block.range(63, 0) = 0x0000000000000000;
        } else if (second) { // the second iteration calculate the E(K,Y0)
            input_block = Y0;
        } else if (initialization) { // the third iteration uses Y0+1 as input_block
            input_block_r.range(127, 120) = Y0(7, 0);
            input_block_r.range(119, 112) = Y0(15, 8);
            input_block_r.range(111, 104) = Y0(23, 16);
            input_block_r.range(103, 96) = Y0(31, 24);
            input_block_r.range(95, 88) = Y0(39, 32);
            input_block_r.range(87, 80) = Y0(47, 40);
            input_block_r.range(79, 72) = Y0(55, 48);
            input_block_r.range(71, 64) = Y0(63, 56);
            input_block_r.range(63, 56) = Y0(71, 64);
            input_block_r.range(55, 48) = Y0(79, 72);
            input_block_r.range(47, 40) = Y0(87, 80);
            input_block_r.range(39, 32) = Y0(95, 88);
            input_block_r.range(31, 24) = Y0(103, 96);
            input_block_r.range(23, 16) = Y0(111, 104);
            input_block_r.range(15, 8) = Y0(119, 112);
            input_block_r.range(7, 0) = Y0(127, 120);
            input_block_r.range(31, 0) = input_block_r.range(31, 0) + 1;
            input_block.range(127, 120) = input_block_r(7, 0);
            input_block.range(119, 112) = input_block_r(15, 8);
            input_block.range(111, 104) = input_block_r(23, 16);
            input_block.range(103, 96) = input_block_r(31, 24);
            input_block.range(95, 88) = input_block_r(39, 32);
            input_block.range(87, 80) = input_block_r(47, 40);
            input_block.range(79, 72) = input_block_r(55, 48);
            input_block.range(71, 64) = input_block_r(63, 56);
            input_block.range(63, 56) = input_block_r(71, 64);
            input_block.range(55, 48) = input_block_r(79, 72);
            input_block.range(47, 40) = input_block_r(87, 80);
            input_block.range(39, 32) = input_block_r(95, 88);
            input_block.range(31, 24) = input_block_r(103, 96);
            input_block.range(23, 16) = input_block_r(111, 104);
            input_block.range(15, 8) = input_block_r(119, 112);
            input_block.range(7, 0) = input_block_r(127, 120);
            initialization = false;
        } else {
            input_block_r.range(127, 120) = input_block(7, 0);
            input_block_r.range(119, 112) = input_block(15, 8);
            input_block_r.range(111, 104) = input_block(23, 16);
            input_block_r.range(103, 96) = input_block(31, 24);
            input_block_r.range(95, 88) = input_block(39, 32);
            input_block_r.range(87, 80) = input_block(47, 40);
            input_block_r.range(79, 72) = input_block(55, 48);
            input_block_r.range(71, 64) = input_block(63, 56);
            input_block_r.range(63, 56) = input_block(71, 64);
            input_block_r.range(55, 48) = input_block(79, 72);
            input_block_r.range(47, 40) = input_block(87, 80);
            input_block_r.range(39, 32) = input_block(95, 88);
            input_block_r.range(31, 24) = input_block(103, 96);
            input_block_r.range(23, 16) = input_block(111, 104);
            input_block_r.range(15, 8) = input_block(119, 112);
            input_block_r.range(7, 0) = input_block(127, 120);
            input_block_r.range(31, 0) = input_block_r.range(31, 0) + 1;
            input_block.range(127, 120) = input_block_r(7, 0);
            input_block.range(119, 112) = input_block_r(15, 8);
            input_block.range(111, 104) = input_block_r(23, 16);
            input_block.range(103, 96) = input_block_r(31, 24);
            input_block.range(95, 88) = input_block_r(39, 32);
            input_block.range(87, 80) = input_block_r(47, 40);
            input_block.range(79, 72) = input_block_r(55, 48);
            input_block.range(71, 64) = input_block_r(63, 56);
            input_block.range(63, 56) = input_block_r(71, 64);
            input_block.range(55, 48) = input_block_r(79, 72);
            input_block.range(47, 40) = input_block_r(87, 80);
            input_block.range(39, 32) = input_block_r(95, 88);
            input_block.range(31, 24) = input_block_r(103, 96);
            input_block.range(23, 16) = input_block_r(111, 104);
            input_block.range(15, 8) = input_block_r(119, 112);
            input_block.range(7, 0) = input_block_r(127, 120);
        }
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "input_block  = " << std::hex << input_block << std::endl;
#endif

        // CIPH_k
        cipher.process(input_block, K, output_block);
// xf::security::internal::aesEncrypt<_keyWidth>(input_block, K, output_block);
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "output_block = " << std::hex << output_block << std::endl;
#endif

        // get the ciphertext for current interation by output_block and plaintext
        ciphertext_r = plaintext_r ^ output_block;
#ifndef __SYNTHESIS__
        std::cout << "ciphertext   = " << std::hex << ciphertext_r << std::endl;
#endif

        if (first) { // write out the hash key, and prepare for the second iteration
            H_strm.write(output_block);
            first = false;
            second = true;
        } else if (second) { // write out the E(K,Y0), and prepare for the third iteration
            E_K_Y0_strm.write(output_block);
            second = false;
            initialization = true;
        } else { // write out plaintext
            ciphertext.write(ciphertext_r);
            ciphertext_e.write(0);
        }

        if (!first && !second && !initialization) {
            e = plaintext_e.read();
        }
    }

    ciphertext_e.write(1);

} // end ciphertext_gen

// @brief Decrypt ciphertext to plaintext
template <unsigned int _keyWidth = 256>
void plaintext_gen(
    // stream in
    hls::stream<ap_uint<128> >& ciphertext,
    hls::stream<bool>& ciphertext_e,
    // input cipherkey and initilization vector
    hls::stream<ap_uint<_keyWidth> >& cipherkey,
    hls::stream<ap_uint<96> >& IV_strm,
    // output hash key and E(K,Y0)
    hls::stream<ap_uint<128> >& H_strm,
    hls::stream<ap_uint<128> >& E_K_Y0_strm,
    // stream out
    hls::stream<ap_uint<128> >& plaintext,
    hls::stream<bool>& plaintext_e) {
#pragma HLS allocation instances = aesEncrypt limit = 1 function
    // register cipherkey
    ap_uint<_keyWidth> K = cipherkey.read();
    xf::security::aesEnc<_keyWidth> cipher;
    cipher.updateKey(K);
#ifndef __SYNTHESIS__
    std::cout << std::endl << "cipherkey = " << std::hex << K << std::endl;
#endif

    // generate initial counter block
    ap_uint<128> Y0;
    Y0.range(95, 0) = IV_strm.read();
    Y0.range(127, 96) = 0x01000000;
#ifndef __SYNTHESIS__
    std::cout << "Y0 = " << std::hex << Y0 << std::endl;
#endif

    // the hash key
    ap_uint<128> H;

    // the E(K,Y0)
    ap_uint<128> E_K_Y0;

    // intermediate registers to perform the CTR decryption chain
    ap_uint<128> ciphertext_r = 0;
    ap_uint<128> input_block = 0;
    ap_uint<128> input_block_r = 0;
    ap_uint<128> output_block = 0;
    ap_uint<128> plaintext_r = 0;

    // set the iteration controlling flag
    bool first = true;
    bool second = false;
    bool initialization = false;

    bool e = ciphertext_e.read();

plaintext_gen_loop:
    while (!e) {
#pragma HLS PIPELINE II = 1
        // read a block of ciphertext, 128 bits
        if (!first && !second) {
            ciphertext_r = ciphertext.read();
        }
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "ciphertext    = " << std::hex << ciphertext_r << std::endl;
#endif

        // calculate input_block
        if (first) { // the first iteration calculate the hash key
            input_block.range(127, 64) = 0x0000000000000000;
            input_block.range(63, 0) = 0x0000000000000000;
        } else if (second) { // the second iteration calculate the E(K,Y0)
            input_block = Y0;
        } else if (initialization) { // the third iteration uses Y0+1 as input_block
            input_block_r.range(127, 120) = Y0(7, 0);
            input_block_r.range(119, 112) = Y0(15, 8);
            input_block_r.range(111, 104) = Y0(23, 16);
            input_block_r.range(103, 96) = Y0(31, 24);
            input_block_r.range(95, 88) = Y0(39, 32);
            input_block_r.range(87, 80) = Y0(47, 40);
            input_block_r.range(79, 72) = Y0(55, 48);
            input_block_r.range(71, 64) = Y0(63, 56);
            input_block_r.range(63, 56) = Y0(71, 64);
            input_block_r.range(55, 48) = Y0(79, 72);
            input_block_r.range(47, 40) = Y0(87, 80);
            input_block_r.range(39, 32) = Y0(95, 88);
            input_block_r.range(31, 24) = Y0(103, 96);
            input_block_r.range(23, 16) = Y0(111, 104);
            input_block_r.range(15, 8) = Y0(119, 112);
            input_block_r.range(7, 0) = Y0(127, 120);
            input_block_r.range(31, 0) = input_block_r.range(31, 0) + 1;
            input_block.range(127, 120) = input_block_r(7, 0);
            input_block.range(119, 112) = input_block_r(15, 8);
            input_block.range(111, 104) = input_block_r(23, 16);
            input_block.range(103, 96) = input_block_r(31, 24);
            input_block.range(95, 88) = input_block_r(39, 32);
            input_block.range(87, 80) = input_block_r(47, 40);
            input_block.range(79, 72) = input_block_r(55, 48);
            input_block.range(71, 64) = input_block_r(63, 56);
            input_block.range(63, 56) = input_block_r(71, 64);
            input_block.range(55, 48) = input_block_r(79, 72);
            input_block.range(47, 40) = input_block_r(87, 80);
            input_block.range(39, 32) = input_block_r(95, 88);
            input_block.range(31, 24) = input_block_r(103, 96);
            input_block.range(23, 16) = input_block_r(111, 104);
            input_block.range(15, 8) = input_block_r(119, 112);
            input_block.range(7, 0) = input_block_r(127, 120);
            initialization = false;
        } else {
            input_block_r.range(127, 120) = input_block(7, 0);
            input_block_r.range(119, 112) = input_block(15, 8);
            input_block_r.range(111, 104) = input_block(23, 16);
            input_block_r.range(103, 96) = input_block(31, 24);
            input_block_r.range(95, 88) = input_block(39, 32);
            input_block_r.range(87, 80) = input_block(47, 40);
            input_block_r.range(79, 72) = input_block(55, 48);
            input_block_r.range(71, 64) = input_block(63, 56);
            input_block_r.range(63, 56) = input_block(71, 64);
            input_block_r.range(55, 48) = input_block(79, 72);
            input_block_r.range(47, 40) = input_block(87, 80);
            input_block_r.range(39, 32) = input_block(95, 88);
            input_block_r.range(31, 24) = input_block(103, 96);
            input_block_r.range(23, 16) = input_block(111, 104);
            input_block_r.range(15, 8) = input_block(119, 112);
            input_block_r.range(7, 0) = input_block(127, 120);
            input_block_r.range(31, 0) = input_block_r.range(31, 0) + 1;
            input_block.range(127, 120) = input_block_r(7, 0);
            input_block.range(119, 112) = input_block_r(15, 8);
            input_block.range(111, 104) = input_block_r(23, 16);
            input_block.range(103, 96) = input_block_r(31, 24);
            input_block.range(95, 88) = input_block_r(39, 32);
            input_block.range(87, 80) = input_block_r(47, 40);
            input_block.range(79, 72) = input_block_r(55, 48);
            input_block.range(71, 64) = input_block_r(63, 56);
            input_block.range(63, 56) = input_block_r(71, 64);
            input_block.range(55, 48) = input_block_r(79, 72);
            input_block.range(47, 40) = input_block_r(87, 80);
            input_block.range(39, 32) = input_block_r(95, 88);
            input_block.range(31, 24) = input_block_r(103, 96);
            input_block.range(23, 16) = input_block_r(111, 104);
            input_block.range(15, 8) = input_block_r(119, 112);
            input_block.range(7, 0) = input_block_r(127, 120);
        }
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "input_block  = " << std::hex << input_block << std::endl;
#endif

        // CIPH_k
        cipher.process(input_block, K, output_block);
// xf::security::internal::aesEncrypt<_keyWidth>(input_block, K, output_block);
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "output_block = " << std::hex << output_block << std::endl;
#endif

        // get the plaintext for current interation by output_block and ciphertext
        plaintext_r = ciphertext_r ^ output_block;
#ifndef __SYNTHESIS__
        std::cout << "plaintext   = " << std::hex << plaintext_r << std::endl;
#endif

        if (first) { // write out the hash key, and prepare for the second iteration
            H_strm.write(output_block);
            first = false;
            second = true;
        } else if (second) { // write out the E(K,Y0), and prepare for the third iteration
            E_K_Y0_strm.write(output_block);
            second = false;
            initialization = true;
        } else { // write out plaintext
            plaintext.write(plaintext_r);
            plaintext_e.write(0);
        }

        if (!first && !second && !initialization) {
            e = ciphertext_e.read();
        }
    }

    plaintext_e.write(1);

} // plaintext_gen

// @brief Calculate Z = X * Y in Galois Field(2^128)
static void GF128_mult(
    // input port
    ap_uint<128> X,
    ap_uint<128> Y,
    // output port
    ap_uint<128>& Z) {
#pragma HLS inline

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
#if !defined(__SYNTHESIS__) && __Mult_Debug == 1
    std::cout << "V = " << std::hex << V << std::endl;
#endif

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
#if !defined(__SYNTHESIS__) && __Mult_Debug == 1
    std::cout << "Y_r = " << std::hex << Y_r << std::endl;
#endif

    // calculate multiplication product
    ap_uint<128> P = 0;
multiplication_loop:
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

// @brief Calculate tag according to AAD and ciphertext
static void tag_gen(
    // stream in
    hls::stream<ap_uint<128> >& ciphertext,
    hls::stream<bool>& ciphertext_e,
    // input additional authenticated data
    hls::stream<ap_uint<128> >& AAD_strm,
    // input hash key
    hls::stream<ap_uint<128> >& H_strm,
    // input E(K,Y0)
    hls::stream<ap_uint<128> >& E_K_Y0_strm,
    // stream out
    hls::stream<ap_uint<128> >& tag_strm) {
    // register additional authenticated data
    ap_uint<128> AAD = AAD_strm.read();
#ifndef __SYNTHESIS__
    std::cout << "AAD = " << std::hex << AAD << std::endl;
#endif

    // register hash key
    ap_uint<128> H = H_strm.read();
#ifndef __SYNTHESIS__
    std::cout << "H   = " << std::hex << H << std::endl;
#endif

    // register E(K,Y0)
    ap_uint<128> E_K_Y0 = E_K_Y0_strm.read();
#ifndef __SYNTHESIS__
    std::cout << "E_K_Y0 = " << std::hex << E_K_Y0 << std::endl;
#endif

    // calculate the first value of GHASH chain
    ap_uint<128> mult_in = 0;
    ap_uint<128> mult_result = 0;
    GF128_mult(AAD, H, mult_result);
#ifndef __SYNTHESIS__
    std::cout << "X          = " << std::hex << mult_result << std::endl << std::endl;
#endif

    // intermediate registers to get the final tag
    ap_uint<64> len_A = 0;
    ap_uint<64> len_C = 0;
    ap_uint<128> concatenate_len_AC = 0;

    // intermediate registers to perform the GHASH chain
    ap_uint<128> ciphertext_r = 0;

    // reset the ciphertext length
    len_C = 0;

    bool e = ciphertext_e.read();

tag_gen_loop:
    while (!e) {
#pragma HLS PIPELINE
        // read a block of ciphertext, 128 bits
        ciphertext_r = ciphertext.read();
#if !defined(__SYNTHESIS__) && __Cipher_Debug == 1
        std::cout << "ciphertext    = " << std::hex << ciphertext_r << std::endl;
#endif

        // accumulate the length of ciphertext in bits
        len_C += 128;

        // calculate mult_H
        mult_in = mult_result ^ ciphertext_r;
#ifndef __SYNTHESIS__
        std::cout << "mult_in      = " << std::hex << mult_in << std::endl;
#endif
        GF128_mult(mult_in, H, mult_result);
#ifndef __SYNTHESIS__
        std::cout << "X            = " << std::hex << mult_result << std::endl;
#endif

        e = ciphertext_e.read();
    }

    // calculate len(A)||len(C)
    concatenate_len_AC.range(127, 120) = len_C.range(7, 0);
    concatenate_len_AC.range(119, 112) = len_C.range(15, 8);
    concatenate_len_AC.range(111, 104) = len_C.range(23, 16);
    concatenate_len_AC.range(103, 96) = len_C.range(31, 24);
    concatenate_len_AC.range(95, 88) = len_C.range(39, 32);
    concatenate_len_AC.range(87, 80) = len_C.range(47, 40);
    concatenate_len_AC.range(79, 72) = len_C.range(55, 48);
    concatenate_len_AC.range(71, 64) = len_C.range(63, 56);
    concatenate_len_AC.range(63, 56) = len_A.range(7, 0);
    concatenate_len_AC.range(55, 48) = len_A.range(15, 8);
    concatenate_len_AC.range(47, 40) = len_A.range(23, 16);
    concatenate_len_AC.range(39, 32) = len_A.range(31, 24);
    concatenate_len_AC.range(31, 24) = len_A.range(39, 32);
    concatenate_len_AC.range(23, 16) = len_A.range(47, 40);
    concatenate_len_AC.range(15, 8) = len_A.range(55, 48);
    concatenate_len_AC.range(7, 0) = len_A.range(63, 56);
#ifndef __SYNTHESIS__
    std::cout << "len(A)||len(C) = " << std::hex << concatenate_len_AC << std::endl;
#endif

    // calculate GHASH
    mult_in = mult_result ^ concatenate_len_AC;
    GF128_mult(mult_in, H, mult_result);
#ifndef __SYNTHESIS__
    std::cout << "GHASH        = " << std::hex << mult_result << std::endl << std::endl;
#endif

    // calculate the tag
    ap_uint<128> tag = mult_result ^ E_K_Y0;
    tag_strm.write(tag);

} // end tag_gen

// @brief Duplicate input stream to output streams
static void dup_strm(
    // stream in
    hls::stream<ap_uint<128> >& in_strm,
    hls::stream<bool>& in_e_strm,
    // stream out
    hls::stream<ap_uint<128> >& out1_strm,
    hls::stream<bool>& out1_e_strm,
    hls::stream<ap_uint<128> >& out2_strm,
    hls::stream<bool>& out2_e_strm) {
    ap_uint<128> in_r = 0;

    bool e = in_e_strm.read();

dup_strm_loop:
    while (!e) {
#pragma HLS PIPELINE II = 1
        in_r = in_strm.read();

        out1_strm.write(in_r);
        out1_e_strm.write(0);
        out2_strm.write(in_r);
        out2_e_strm.write(0);

        e = in_e_strm.read();
    }

    out1_e_strm.write(1);
    out2_e_strm.write(1);

} // end dup_strm

/**
 *
 * @brief aesGcmEncrypt is GCM encryption mode with AES single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @tparam _keyWidth The bit-width of the cipher key, which is 128, 192, or 256.
 *
 * @param plaintext Input block stream text to be encrypted, 128 bits.
 * @param plaintext_e End flag of block stream plaintext, 1 bit.
 * @param cipherkey Input cipher key used in encryption, x bits for AES-x.
 * @param IV_strm Initialization vector for the fisrt iteration of AES encrypition, 128 bits.
 * @param AAD_strm Additional authenticated data for calculating the tag, 128 bits.
 * @param ciphertext Output encrypted block stream text, 128 bits.
 * @param ciphertext_e End flag of block stream ciphertext, 1 bit.
 * @param tag_strm The data tag
 *
 */

template <unsigned int _keyWidth = 256>
void aesGcmEncrypt(
    // stream in
    hls::stream<ap_uint<128> >& plaintext,
    hls::stream<bool>& plaintext_e,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<_keyWidth> >& cipherkey,
    hls::stream<ap_uint<96> >& IV_strm,
    hls::stream<ap_uint<128> >& AAD_strm,
    // stream out
    hls::stream<ap_uint<128> >& ciphertext,
    hls::stream<bool>& ciphertext_e,
    // ouput tag
    hls::stream<ap_uint<128> >& tag_strm) {
#pragma HLS DATAFLOW

    // register of hash key
    ap_uint<128> H = 0;

    // register of E(K,Y0)
    ap_uint<128> E_K_Y0 = 0;

    // register to store tag
    ap_uint<128> tag = 0;

    hls::stream<ap_uint<128> > ciphertext_strm("ciphertext_strm");
    hls::stream<bool> ciphertext_e_strm("ciphertext_e_strm");
#pragma HLS RESOURCE variable = ciphertext_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext_strm depth = 32 dim = 1
#pragma HLS RESOURCE variable = ciphertext_e_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext_e_strm depth = 32 dim = 1

    hls::stream<ap_uint<128> > ciphertext_tag_strm("ciphertext_tag_strm");
    hls::stream<bool> ciphertext_tag_e_strm("ciphertext_tag_e_strm");
#pragma HLS RESOURCE variable = ciphertext_tag_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext_tag_strm depth = 32 dim = 1
#pragma HLS RESOURCE variable = ciphertext_tag_e_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext_tag_e_strm depth = 32 dim = 1

    hls::stream<ap_uint<128> > H_strm("H_strm");
#pragma HLS RESOURCE variable = H_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = H_strm depth = 32 dim = 1

    hls::stream<ap_uint<128> > E_K_Y0_strm("E_K_Y0_strm");
#pragma HLS RESOURCE variable = E_K_Y0_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = E_K_Y0_strm depth = 32 dim = 1

    ciphertext_gen<_keyWidth>(plaintext, plaintext_e,              // stream in
                              cipherkey, IV_strm,                  // cipherkey and initialization vector
                              H_strm, E_K_Y0_strm,                 // hash key and E(K,Y0)
                              ciphertext_strm, ciphertext_e_strm); // stream out

    dup_strm(ciphertext_strm, ciphertext_e_strm,          // result stream from ciphertext_gen
             ciphertext, ciphertext_e,                    // to primitive output port
             ciphertext_tag_strm, ciphertext_tag_e_strm); // to tag_gen

    tag_gen(ciphertext_tag_strm, ciphertext_tag_e_strm, // stream in
            AAD_strm, H_strm, E_K_Y0_strm,              // additional authenticated data, hash key, and E(K,Y0)
            tag_strm);                                  // result of data tag

} // end aesGcmEncrypt

/**
 *
 * @brief aesGcmEncrypt is GCM decryption mode with AES single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param ciphertext Input block stream text to be decrypted, 128 bits.
 * @param ciphertext_e End flag of block stream ciphertext, 1 bit.
 * @param cipherkey Input cipher key used in decryption, x bits for AES-x.
 * @param IV_strm Initialization vector for the fisrt iteration of AES decrypition, 128 bits.
 * @param AAD_strm Additional authenticated data for calculating the tag, 128 bits.
 * @param plaintext Output decrypted block stream text, 128 bits.
 * @param plaintext_e End flag of block stream plaintext, 1 bit.
 * @param tag_strm The data tag
 *
 */

template <unsigned int _keyWidth = 256>
void aesGcmDecrypt(
    // stream in
    hls::stream<ap_uint<128> >& ciphertext,
    hls::stream<bool>& ciphertext_e,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<_keyWidth> >& cipherkey,
    hls::stream<ap_uint<96> >& IV_strm,
    hls::stream<ap_uint<128> >& AAD_strm,
    // stream out
    hls::stream<ap_uint<128> >& plaintext,
    hls::stream<bool>& plaintext_e,
    // ouput tag
    hls::stream<ap_uint<128> >& tag_strm) {
#pragma HLS DATAFLOW

    // register of hash key
    ap_uint<128> H = 0;

    // register of E(K,Y0)
    ap_uint<128> E_K_Y0 = 0;

    // register to store tag
    ap_uint<128> tag = 0;

    hls::stream<ap_uint<128> > ciphertext1_strm("ciphertext1_strm");
    hls::stream<bool> ciphertext1_e_strm("ciphertext1_e_strm");
#pragma HLS RESOURCE variable = ciphertext1_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext1_strm depth = 32 dim = 1
#pragma HLS RESOURCE variable = ciphertext1_e_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext1_e_strm depth = 32 dim = 1

    hls::stream<ap_uint<128> > ciphertext2_strm("ciphertext2_strm");
    hls::stream<bool> ciphertext2_e_strm("ciphertext2_e_strm");
#pragma HLS RESOURCE variable = ciphertext2_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext2_strm depth = 32 dim = 1
#pragma HLS RESOURCE variable = ciphertext2_e_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = ciphertext2_e_strm depth = 32 dim = 1

    hls::stream<ap_uint<128> > H_strm("H_strm");
#pragma HLS RESOURCE variable = H_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = H_strm depth = 32 dim = 1

    hls::stream<ap_uint<128> > E_K_Y0_strm("E_K_Y0_strm");
#pragma HLS RESOURCE variable = E_K_Y0_strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = E_K_Y0_strm depth = 32 dim = 1

    dup_strm(ciphertext, ciphertext_e,              // stream from input ports
             ciphertext1_strm, ciphertext1_e_strm,  // to plaintext_gen
             ciphertext2_strm, ciphertext2_e_strm); // to tag_gen

    plaintext_gen<_keyWidth>(ciphertext1_strm, ciphertext1_e_strm, // stream in
                             cipherkey, IV_strm,                   // cipherkey and initialization vector
                             H_strm, E_K_Y0_strm,                  // hash key and E(K,Y0)
                             plaintext, plaintext_e);              // stream to output ports

    tag_gen(ciphertext2_strm, ciphertext2_e_strm, // stream in
            AAD_strm, H_strm, E_K_Y0_strm,        // additional authenticated data, hash key, and E(K,Y0)
            tag_strm);                            // result of data tag

} // aesGcmDecrypt

} // namespace details

/**
 *
 * @brief aes128GcmEncrypt is GCM encryption mode with AES-128 single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param plaintextStrm Input block stream text to be encrypted, 128 bits.
 * @param endPlaintextStrm End flag of block stream plaintext, 1 bit.
 * @param cipherkeyStrm Input cipher key used in encryption, 128 bits.
 * @param IVStrm Initialization vector for the fisrt iteration of AES encrypition, 128 bits.
 * @param AADStrm Additional authenticated data for calculating the tag, 128 bits.
 * @param ciphertextStrm Output encrypted block stream text, 128 bits.
 * @param endCiphertextStrm End flag of block stream ciphertext, 1 bit.
 * @param tagStrm The data tag
 *
 */

static void aes128GcmEncrypt(
    // stream in
    hls::stream<ap_uint<128> >& plaintextStrm,
    hls::stream<bool>& endPlaintextStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<128> >& cipherkeyStrm,
    hls::stream<ap_uint<96> >& IVStrm,
    hls::stream<ap_uint<128> >& AADStrm,
    // stream out
    hls::stream<ap_uint<128> >& ciphertextStrm,
    hls::stream<bool>& endCiphertextStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    details::aesGcmEncrypt<128>(plaintextStrm, endPlaintextStrm, cipherkeyStrm, IVStrm, AADStrm, ciphertextStrm,
                                endCiphertextStrm, tagStrm);

} // end aes128GcmEncrypt

/**
 *
 * @brief aes128GcmDecrypt is GCM decryption mode with AES-128 single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param ciphertextStrm Input block stream text to be decrypted, 128 bits.
 * @param endCiphertextStrm End flag of block stream ciphertext, 1 bit.
 * @param cipherkeyStrm Input cipher key used in decryption, 128 bits.
 * @param IVStrm Initialization vector for the fisrt iteration of AES decrypition, 128 bits.
 * @param AADStrm Additional authenticated data for calculating the tag, 128 bits.
 * @param plaintextStrm Output decrypted block stream text, 128 bits.
 * @param endPlaintextStrm End flag of block stream plaintext, 1 bit.
 * @param tagStrm The data tag
 *
 */

static void aes128GcmDecrypt(
    // stream in
    hls::stream<ap_uint<128> >& ciphertextStrm,
    hls::stream<bool>& endCiphertextStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<128> >& cipherkeyStrm,
    hls::stream<ap_uint<96> >& IVStrm,
    hls::stream<ap_uint<128> >& AADStrm,
    // stream out
    hls::stream<ap_uint<128> >& plaintextStrm,
    hls::stream<bool>& endPlaintextStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    details::aesGcmDecrypt<128>(ciphertextStrm, endCiphertextStrm, cipherkeyStrm, IVStrm, AADStrm, plaintextStrm,
                                endPlaintextStrm, tagStrm);

} // end aes128GcmDecrypt

/**
 *
 * @brief aes192GcmEncrypt is GCM encryption mode with AES-192 single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param plaintextStrm Input block stream text to be encrypted, 128 bits.
 * @param endPlaintextStrm End flag of block stream plaintext, 1 bit.
 * @param cipherkeyStrm Input cipher key used in encryption, 192 bits.
 * @param IVStrm Initialization vector for the fisrt iteration of AES encrypition, 128 bits.
 * @param AADStrm Additional authenticated data for calculating the tag, 128 bits.
 * @param ciphertextStrm Output encrypted block stream text, 128 bits.
 * @param endCiphertextStrm End flag of block stream ciphertext, 1 bit.
 * @param tagStrm The data tag
 *
 */

static void aes192GcmEncrypt(
    // stream in
    hls::stream<ap_uint<128> >& plaintextStrm,
    hls::stream<bool>& endPlaintextStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<192> >& cipherkeyStrm,
    hls::stream<ap_uint<96> >& IVStrm,
    hls::stream<ap_uint<128> >& AADStrm,
    // stream out
    hls::stream<ap_uint<128> >& ciphertextStrm,
    hls::stream<bool>& endCiphertextStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    details::aesGcmEncrypt<192>(plaintextStrm, endPlaintextStrm, cipherkeyStrm, IVStrm, AADStrm, ciphertextStrm,
                                endCiphertextStrm, tagStrm);

} // end aes192GcmEncrypt

/**
 *
 * @brief aes192GcmDecrypt is GCM decryption mode with AES-192 single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param ciphertextStrm Input block stream text to be decrypted, 128 bits.
 * @param endCiphertextStrm End flag of block stream ciphertext, 1 bit.
 * @param cipherkeyStrm Input cipher key used in decryption, 192 bits.
 * @param IVStrm Initialization vector for the fisrt iteration of AES decrypition, 128 bits.
 * @param AADStrm Additional authenticated data for calculating the tag, 128 bits.
 * @param plaintextStrm Output decrypted block stream text, 128 bits.
 * @param endPlaintextStrm End flag of block stream plaintext, 1 bit.
 * @param tagStrm The data tag
 *
 */

static void aes192GcmDecrypt(
    // stream in
    hls::stream<ap_uint<128> >& ciphertextStrm,
    hls::stream<bool>& endCiphertextStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<192> >& cipherkeyStrm,
    hls::stream<ap_uint<96> >& IVStrm,
    hls::stream<ap_uint<128> >& AADStrm,
    // stream out
    hls::stream<ap_uint<128> >& plaintextStrm,
    hls::stream<bool>& endPlaintextStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    details::aesGcmDecrypt<192>(ciphertextStrm, endCiphertextStrm, cipherkeyStrm, IVStrm, AADStrm, plaintextStrm,
                                endPlaintextStrm, tagStrm);

} // end aes192GcmDecrypt

/**
 *
 * @brief aes256GcmEncrypt is GCM encryption mode with AES-256 single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param plaintextStrm Input block stream text to be encrypted, 128 bits.
 * @param endPlaintextStrm End flag of block stream plaintext, 1 bit.
 * @param cipherkeyStrm Input cipher key used in encryption, 256 bits.
 * @param IVStrm Initialization vector for the fisrt iteration of AES encrypition, 128 bits.
 * @param AADStrm Additional authenticated data for calculating the tag, 128 bits.
 * @param ciphertextStrm Output encrypted block stream text, 128 bits.
 * @param endCiphertextStrm End flag of block stream ciphertext, 1 bit.
 * @param tagStrm The data tag
 *
 */

static void aes256GcmEncrypt(
    // stream in
    hls::stream<ap_uint<128> >& plaintextStrm,
    hls::stream<bool>& endPlaintextStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<256> >& cipherkeyStrm,
    hls::stream<ap_uint<96> >& IVStrm,
    hls::stream<ap_uint<128> >& AADStrm,
    // stream out
    hls::stream<ap_uint<128> >& ciphertextStrm,
    hls::stream<bool>& endCiphertextStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    details::aesGcmEncrypt<256>(plaintextStrm, endPlaintextStrm, cipherkeyStrm, IVStrm, AADStrm, ciphertextStrm,
                                endCiphertextStrm, tagStrm);

} // end aes256GcmEncrypt

/**
 *
 * @brief aes256GcmDecrypt is GCM decryption mode with AES-256 single block cipher.
 *
 * The algorithm reference is : "IEEE Standard for Authenticated Encryption with Length Expansion for Storage Devices"
 * The implementation is modified for better performance.
 *
 * @param ciphertextStrm Input block stream text to be decrypted, 128 bits.
 * @param endCiphertextStrm End flag of block stream ciphertext, 1 bit.
 * @param cipherkeyStrm Input cipher key used in decryption, 256 bits.
 * @param IVStrm Initialization vector for the fisrt iteration of AES decrypition, 128 bits.
 * @param AADStrm Additional authenticated data for calculating the tag, 128 bits.
 * @param plaintextStrm Output decrypted block stream text, 128 bits.
 * @param endPlaintextStrm End flag of block stream plaintext, 1 bit.
 * @param tagStrm The data tag
 *
 */

static void aes256GcmDecrypt(
    // stream in
    hls::stream<ap_uint<128> >& ciphertextStrm,
    hls::stream<bool>& endCiphertextStrm,
    // input cipherkey, initilization vector, and additional authenticated data
    hls::stream<ap_uint<256> >& cipherkeyStrm,
    hls::stream<ap_uint<96> >& IVStrm,
    hls::stream<ap_uint<128> >& AADStrm,
    // stream out
    hls::stream<ap_uint<128> >& plaintextStrm,
    hls::stream<bool>& endPlaintextStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
    details::aesGcmDecrypt<256>(ciphertextStrm, endCiphertextStrm, cipherkeyStrm, IVStrm, AADStrm, plaintextStrm,
                                endPlaintextStrm, tagStrm);

} // end aes256GcmDecrypt

} // namespace security
} // namespace xf

#endif
