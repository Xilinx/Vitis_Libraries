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
 * This file is part of XF Security Library.
 *
 * @detail GMAC encryption/decryption mode in AES-256.
 */

#ifndef _XF_SECURITY_GMAC_HPP_
#define _XF_SECURITY_GMAC_HPP_

#include <ap_int.h>
#include <hls_stream.h>

#include "gcm.hpp"

// for debug
#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace security {
namespace details {

/**
 * @brief generateEKY0 generates H and E_K_Y0 based on AES-256.
 *
 * @param keyStrm input key used in encryption, 256 bits for AES-256.
 * @param ivStrm  initialization vector for the fisrt iteration of AES encrypition, 128 bits.
 * @param aadStrm output 128 bits zero.
 * @param hStrm   generate H by encrypting 128-bit 0.
 * @param ekyStrm   generate E_K_Y0 by encrypting 128-bit data including IV.
 *
 */

static void generateEKY0(hls::stream<ap_uint<256> >& keyStrm,
                         hls::stream<ap_uint<96> >& ivStrm,
                         hls::stream<ap_uint<128> >& aadStrm,
                         hls::stream<ap_uint<128> >& hStrm,
                         hls::stream<ap_uint<128> >& ekyStrm) {
#pragma HLS allocation instances = Aes256_Encrypt_one_word limit = 1 function

    // register keyStrm
    ap_uint<256> key = keyStrm.read();
#ifndef __SYNTHESIS__
    std::cout << std::endl << "cipherkey = " << std::hex << key << std::endl;
#endif

    // generate initial counter block
    ap_uint<128> y0;
    y0.range(95, 0) = ivStrm.read();
    y0.range(127, 96) = 0x01000000;
#ifndef __SYNTHESIS__
    std::cout << "Y0 = " << std::hex << y0 << std::endl;
#endif

    ap_uint<128> inputBlock = 0;
    ap_uint<128> outputBlock = 0;
    aadStrm.write(inputBlock);
    for (int i = 0; i < 2; ++i) {
#pragma HLS PIPELINE II = 1
        inputBlock = (i == 0) ? inputBlock : y0;
        xf::security::aes256Encrypt(inputBlock, key, outputBlock);
        if (i == 0)
            hStrm.write(outputBlock);
        else
            ekyStrm.write(outputBlock);
    }
}

} // namespace details

/**
 * @brief aes256Gmac is the basic encryption mode of AES-256.
 *
 * Galois Message Authentication Code (GMAC) is a mechanism to provide data origin authentication.
 *
 * @param dataStrm is input stream text to be authenticated, 128 bits.
 * @param eDataStrm is end flag of stream data, 1 bit.
 * @param keyStrm is input key used in encryption, 256 bits for AES-256.
 * @param ivStrm is initialization vector for the fisrt iteration of AES encrypition, 128 bits.
 * @param tagStrm is the data tag
 *
 */

static void aes256Gmac(
    // stream in
    hls::stream<ap_uint<128> >& dataStrm,
    hls::stream<bool>& eDataStrm,
    // input keyStrm, initilization vector
    hls::stream<ap_uint<256> >& keyStrm,
    hls::stream<ap_uint<96> >& ivStrm,
    // ouput tag
    hls::stream<ap_uint<128> >& tagStrm) {
#pragma HLS DATAFLOW
    // here aadStrm is just for adapting to the interface of tag_gen, zero.
    hls::stream<ap_uint<128> > aadStrm;
#pragma HLS RESOURCE variable = aadStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = aadStrm depth = 32 dim = 1

    hls::stream<ap_uint<128> > hStrm("hStrm");
#pragma HLS RESOURCE variable = hStrm core = FIFO_LUTRAM
#pragma HLS STREAM variable = hStrm depth = 32 dim = 1

    hls::stream<ap_uint<128> > eky0Strm("eky0Strm");
#pragma HLS RESOURCE variable = eky0Strm core = FIFO_LUTRAM
#pragma HLS STREAM variable = eky0Strm depth = 32 dim = 1

    details::generateEKY0(keyStrm, ivStrm, aadStrm, hStrm, eky0Strm);

    details::tag_gen(dataStrm, eDataStrm,      // stream in
                     aadStrm, hStrm, eky0Strm, // additional authenticated data, hash key, and E(K,Y0)
                     tagStrm);                 // result of data tag

    return;
}

} // namespace security
} // namespace xf

#endif // XF_SECURITY_GMAC_HPP_
