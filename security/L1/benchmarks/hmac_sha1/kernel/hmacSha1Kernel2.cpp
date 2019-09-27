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
 * @file aes256CbcEncryptKernel.cpp
 * @brief kernel code of Cipher Block Chaining (CBC) block cipher mode of operation.
 * This file is part of Vitis Security Library.
 *
 * @detail Containing scan, distribute, encrypt, merge, and write-out functions.
 *
 */

#include <ap_int.h>
#include <hls_stream.h>
#include "xf_security/sha1.hpp"
#include "xf_security/hmac.hpp"
#include "kernel_config.hpp"
#ifndef __SYNTHESIS__
#include <iostream>
#endif

template <int msgW, int lW, int hshW>
struct sha1_wrapper {
    static void hash(hls::stream<ap_uint<msgW> >& msgStrm,
                     hls::stream<ap_uint<64> >& lenStrm,
                     hls::stream<bool>& eLenStrm,
                     hls::stream<ap_uint<5 * msgW> >& hshStrm,
                     hls::stream<bool>& eHshStrm) {
        xf::security::sha1<msgW>(msgStrm, lenStrm, eLenStrm, hshStrm, eHshStrm);
    }
};

static void test_hmac_sha1(hls::stream<ap_uint<32> >& keyStrm,
                           hls::stream<ap_uint<64> >& lenKeyStrm,
                           hls::stream<ap_uint<32> >& msgStrm,
                           hls::stream<ap_uint<64> >& lenStrm,
                           hls::stream<bool>& eLenStrm,
                           hls::stream<ap_uint<160> >& hshStrm,
                           hls::stream<bool>& eHshStrm) {
    xf::security::hmac<32, 32, 64, 160, 64, sha1_wrapper>(keyStrm, lenKeyStrm, msgStrm, lenStrm, eLenStrm, hshStrm,
                                                          eHshStrm);
}

template <unsigned int _burstLength, unsigned int _channelNumber>
static void readIn(ap_uint<512>* ptr,
                   hls::stream<ap_uint<512> >& textInStrm,
                   hls::stream<ap_uint<64> >& textLengthStrm,
                   hls::stream<ap_uint<64> >& textNumStrm,
                   hls::stream<ap_uint<256> >& keyInStrm) {
    // number of message blocks in 128 bits
    ap_uint<64> textLength;
    // number of messages for single PU
    ap_uint<64> textNum;
    // hmac key
    ap_uint<256> key;

// scan for configurations, _channelNumber in total
// actually the same textLength, msgNum
// key is also the same to simplify input table
// and key is treated as different when process next message
LOOP_READ_CONFIG:
    for (unsigned char i = 0; i < _channelNumber; i++) {
#pragma HLS pipeline II = 1
        ap_uint<512> axiBlock = ptr[i];
        textLength = axiBlock.range(511, 448);
        textNum = axiBlock.range(447, 384);
        key = axiBlock.range(255, 0);
        if (i == 0) {
            textLengthStrm.write(textLength);
            textNumStrm.write(textNum);
            keyInStrm.write(key);
        }
#ifndef __SYNTHESIS__
        std::cout << std::hex << "textlen " << textLength << " textnum " << textNum << " key " << key << std::endl;
#endif
    }

    ap_uint<64> totalAxiBlock = textNum * textLength * _channelNumber / 4;
#ifndef __SYNTHESIS__
    std::cout << "totalAxiBlock" << std::hex << totalAxiBlock << std::endl;
#endif

LOOP_READ_DATA:
    for (ap_uint<64> i = 0; i < totalAxiBlock; i++) {
#pragma HLS pipeline II = 1
        ap_uint<512> axiBlock = ptr[_channelNumber + i];
        textInStrm.write(axiBlock);
    }
}

static void writeOneStrmGroup(ap_uint<256> key,
                              ap_uint<64> textLengthInByte,
                              hls::stream<ap_uint<32> >& keyStrm,
                              hls::stream<ap_uint<64> >& keyLenStrm,
                              hls::stream<ap_uint<64> >& msgLenStrm) {
#pragma HLS inline off
    for (ap_uint<8> k = 0; k < (256 / 32); k++) {
#pragma HLS pipeline II = 1
        keyStrm.write(key.range(k * 32 + 31, k * 32));
    }
    keyLenStrm.write(256 / 8);
    msgLenStrm.write(textLengthInByte);
}
template <unsigned int _channelNumber>
static void writeStrmGroups(ap_uint<256> key,
                            ap_uint<64> textLengthInByte,
                            hls::stream<ap_uint<32> > keyStrm[_channelNumber],
                            hls::stream<ap_uint<64> > keyLenStrm[_channelNumber],
                            hls::stream<ap_uint<64> > msgLenStrm[_channelNumber]) {
#pragma HLS dataflow
    for (unsigned int i = 0; i < _channelNumber; i++) {
#pragma HLS unroll
        writeOneStrmGroup(key, textLengthInByte, keyStrm[i], keyLenStrm[i], msgLenStrm[i]);
    }
}

template <unsigned int _channelNumber, unsigned int _burstLength>
static void splitInput(hls::stream<ap_uint<512> >& textInStrm,
                       hls::stream<ap_uint<64> >& textLengthStrm,
                       hls::stream<ap_uint<64> >& textNumStrm,
                       hls::stream<ap_uint<256> >& keyInStrm,
                       hls::stream<ap_uint<32> > keyStrm[_channelNumber],
                       hls::stream<ap_uint<64> > keyLenStrm[_channelNumber],
                       hls::stream<ap_uint<32> > msgStrm[_channelNumber],
                       hls::stream<ap_uint<64> > msgLenStrm[_channelNumber],
                       hls::stream<bool> eMsgLenStrm[_channelNumber]) {
    // number of message blocks in 128 bits
    ap_uint<64> textLength = textLengthStrm.read();
    // transform to message length in bytes
    ap_uint<64> textLengthInByte = textLength * (128 / 8);
    // transform to message length in 32bits
    ap_uint<64> textLengthIn32Bits = textLength * (128 / 32);

    // number of messages for single PU
    ap_uint<64> textNum = textNumStrm.read();
    // hmac key
    ap_uint<256> key = keyInStrm.read();
#ifndef __SYNTHESIS__
    std::cout << std::dec << "txtLen:" << textLength << " txtNum: " << textNum << " key: " << std::hex << key
              << std::endl;
#endif

LOOP_TEXTNUM:
    for (ap_uint<64> i = 0; i < textNum; i++) {
        // write out key and keylength for all channel
        /*
        for(ap_uint<8> j = 0; j < _channelNumber; j++) {
            #pragma HLS unroll
            for(ap_uint<8> k = 0; k < 256 / 32; k++) {
                #pragma HLS pipeline II=1
                keyStrm[j].write(key.range(k * 32 + 31, k * 32));
            }
            keyLenStrm[j].write(256 / 8);//in byte
            msgLenStrm[j].write(textLengthInByte);//in byte
            eMsgLenStrm[j].write(false);
        }
        */
        writeStrmGroups<_channelNumber>(key, textLengthInByte, keyStrm, keyLenStrm, msgLenStrm);
        for (unsigned int j = 0; j < _channelNumber; j++) {
            eMsgLenStrm[j].write(false);
        }
    LOOP_TEXTLEN:
        for (ap_uint<64> j = 0; j < textLengthIn32Bits; j++) {
        LOOP_CHANNELGRP:
            for (unsigned char k = 0; k < _channelNumber; k += 16) {
                ap_uint<512> text = textInStrm.read();

                ap_uint<32> data[16];
                for (unsigned char l = 0; l < 16; l++) {
#pragma HLS unroll
                    data[l] = text.range(32 * l + 31, 32 * l);
                }

                // multiplexer for channels to decider when to write
                for (ap_uint<8> l = 0; l < _channelNumber; l++) {
#pragma HLS unroll
                    if ((l >= k) && (l < k + 16)) {
                        ap_uint<4> m = l.range(3, 0);
                        msgStrm[l].write(data[m]);
                    }
                }
            }
        }
    }
    for (ap_uint<8> l = 0; l < _channelNumber; l++) {
#pragma HLS unroll
        eMsgLenStrm[l].write(true);
    }
}

#ifndef __SYNTHESIS__
template <unsigned int _channelNumber, unsigned int _burstLength>
static void check_splitInput(hls::stream<ap_uint<32> > keyStrm[_channelNumber],
                             hls::stream<ap_uint<64> > keyLenStrm[_channelNumber],
                             hls::stream<ap_uint<32> > msgStrm[_channelNumber],
                             hls::stream<ap_uint<64> > msgLenStrm[_channelNumber],
                             hls::stream<bool> eMsgLenStrm[_channelNumber]) {
    ap_uint<32> key;
    ap_uint<64> keyl;
    ap_uint<32> msg;
    ap_uint<64> msgl;
    bool e;
    for (int i = 0; i < _channelNumber; i++) {
        for (int j = 0; j < 2; j++) {
            keyl = keyLenStrm[i].read();
            msgl = msgLenStrm[i].read();
            e = eMsgLenStrm[i].read();
            std::cout << "kl:" << keyl << " ml:" << msgl << "e:" << e << std::endl;
            for (int m = 0; m < 256 / 32; m++) {
                key = keyStrm[i].read();
                std::cout << "key:" << std::hex << key;
            }
            std::cout << std::endl << std::dec;
            for (int m = 0; m < (64 * 128 / 32); m++) {
                msg = msgStrm[i].read();
                // std::cout << "msg:"<<std::hex<<msg;
            }
            std::cout << std::endl;
        }
        e = eMsgLenStrm[i].read();
        std::cout << "expect: 1, actual:" << e << std::endl;
    }
}
#endif

template <unsigned int _channelNumber>
static void hmacSha1Parallel(hls::stream<ap_uint<32> > keyStrm[_channelNumber],
                             hls::stream<ap_uint<64> > keyLenStrm[_channelNumber],
                             hls::stream<ap_uint<32> > msgStrm[_channelNumber],
                             hls::stream<ap_uint<64> > msgLenStrm[_channelNumber],
                             hls::stream<bool> eMsgLenStrm[_channelNumber],
                             hls::stream<ap_uint<160> > hshStrm[_channelNumber],
                             hls::stream<bool> eHshStrm[_channelNumber]) {
#pragma HLS dataflow
    for (int i = 0; i < _channelNumber; i++) {
#pragma HLS unroll
#ifndef __SYNTHESIS__
        std::cout << std::dec << i << "th channel" << std::endl;
#endif
        test_hmac_sha1(keyStrm[i], keyLenStrm[i], msgStrm[i], msgLenStrm[i], eMsgLenStrm[i], hshStrm[i], eHshStrm[i]);
    }
}

#ifndef __SYNTHESIS___
template <unsigned int _channelNumber>
static void check_hmacSha1Parallel(hls::stream<ap_uint<160> > hshStrm[_channelNumber],
                                   hls::stream<bool> eHshStrm[_channelNumber]) {
    for (int i = 0; i < _channelNumber; i++) {
        bool e;
        ap_uint<160> result;
        e = eHshStrm[i].read();
        while (!e) {
            result = hshStrm[i].read();
            e = eHshStrm[i].read();
        }
    }
}
#endif

template <unsigned int _channelNumber, unsigned int _burstLen>
static void mergeResult(hls::stream<ap_uint<160> > hshStrm[_channelNumber],
                        hls::stream<bool> eHshStrm[_channelNumber],
                        hls::stream<ap_uint<512> >& outStrm,
                        hls::stream<unsigned int>& burstLenStrm) {
    ap_uint<_channelNumber> unfinish;
    for (int i = 0; i < _channelNumber; i++) {
#pragma HLS unroll
        unfinish[i] = 1;
    }

    unsigned int counter = 0;

    while (unfinish != 0) {
        for (int i = 0; i < _channelNumber; i++) {
            bool e = eHshStrm[i].read();
            if (!e) {
                ap_uint<160> hsh = hshStrm[i].read();
                ap_uint<512> tmp = 0;
                tmp.range(159, 0) = hsh.range(159, 0);
                outStrm.write(tmp);
                counter++;
                if (counter == _burstLen) {
                    counter = 0;
                    burstLenStrm.write(_burstLen);
                }
            } else {
                unfinish[i] = 0;
            }
        }
    }
    if (counter != 0) {
        burstLenStrm.write(counter);
    }
    burstLenStrm.write(0);
}

#ifndef __SYNTHESIS__
static void check_mergeResult(hls::stream<ap_uint<512> >& outStrm, hls::stream<unsigned int>& burstLenStrm) {
    unsigned int len;
    len = burstLenStrm.read();
    while (len != 0) {
        std::cout << len << std::endl;
        for (int i = 0; i < len; i++) {
            ap_uint<512> result = 0;
            result = outStrm.read();
            std::cout << "Result: " << std::hex << result << std::endl;
        }
        len = burstLenStrm.read();
    }
}
#endif

template <unsigned int _burstLength, unsigned int _channelNumber>
static void writeOut(hls::stream<ap_uint<512> >& outStrm, hls::stream<unsigned int>& burstLenStrm, ap_uint<512>* ptr) {
    unsigned int burstLen = burstLenStrm.read();
    unsigned int counter = 0;
    while (burstLen != 0) {
        for (unsigned int i = 0; i < burstLen; i++) {
#pragma HLS pipeline II = 1
            ptr[counter] = outStrm.read();
            counter++;
        }
        burstLen = burstLenStrm.read();
    }
}
// @brief top of kernel
extern "C" void hmacSha1Kernel_2(ap_uint<512> inputData[(1 << 30) + 100], ap_uint<512> outputData[1 << 30]) {
#pragma HLS dataflow

    const unsigned int _channelNumber = CH_NM;
    const unsigned int _burstLength = BURST_LEN;
    const unsigned int fifoDepth = _burstLength * 2;
    const unsigned int keyDepth = 256 / 32 * 2;

// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_0 port = inputData 

#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 16 num_read_outstanding = 16 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_1 port = outputData
// clang-format on

#pragma HLS INTERFACE s_axilite port = inputData bundle = control
#pragma HLS INTERFACE s_axilite port = outputData bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    hls::stream<ap_uint<512> > textInStrm;
#pragma HLS stream variable = textInStrm depth = fifoDepth
    hls::stream<ap_uint<64> > textLengthStrm;
#pragma HLS stream variable = textLengthStrm depth = 4
    hls::stream<ap_uint<64> > textNumStrm;
#pragma HLS stream variable = textNumStrm depth = 4
    hls::stream<ap_uint<256> > keyInStrm;
#pragma HLS stream variable = keyInStrm depth = 4

    hls::stream<ap_uint<32> > keyStrm[_channelNumber];
#pragma HLS stream variable = keyStrm depth = keyDepth
    hls::stream<ap_uint<64> > keyLenStrm[_channelNumber];
#pragma HLS stream variable = keyLenStrm depth = 2
    hls::stream<ap_uint<32> > msgStrm[_channelNumber];
#pragma HLS stream variable = msgStrm depth = fifoDepth
    hls::stream<ap_uint<64> > msgLenStrm[_channelNumber];
#pragma HLS stream variable = msgLenStrm depth = 2
    hls::stream<bool> eMsgLenStrm[_channelNumber];
#pragma HLS stream variable = eMsgLenStrm depth = 2

    hls::stream<ap_uint<160> > hshStrm[_channelNumber];
#pragma HLS stream variable = hshStrm depth = 4
    hls::stream<bool> eHshStrm[_channelNumber];
#pragma HLS stream variable = eHshStrm depth = 4

    hls::stream<ap_uint<512> > outStrm;
#pragma HLS stream variable = outStrm depth = fifoDepth
    hls::stream<unsigned int> burstLenStrm;
#pragma HLS stream variable = burstLenStrm depth = 2

    readIn<_burstLength, _channelNumber>(inputData, textInStrm, textLengthStrm, textNumStrm, keyInStrm);

    splitInput<_channelNumber, _burstLength>(textInStrm, textLengthStrm, textNumStrm, keyInStrm, keyStrm, keyLenStrm,
                                             msgStrm, msgLenStrm, eMsgLenStrm);
    // check_splitInput<_channelNumber, _burstLength>(keyStrm, keyLenStrm, msgStrm, msgLenStrm, eMsgLenStrm);

    hmacSha1Parallel<_channelNumber>(keyStrm, keyLenStrm, msgStrm, msgLenStrm, eMsgLenStrm, hshStrm, eHshStrm);

    // check_hmacSha1Parallel<_channelNumber>(hshStrm, eHshStrm);

    mergeResult<_channelNumber, _burstLength>(hshStrm, eHshStrm, outStrm, burstLenStrm);

    // check_mergeResult(outStrm, burstLenStrm);

    writeOut<_burstLength, _channelNumber>(outStrm, burstLenStrm, outputData);

} // end aes256CbcEncryptKernel_1
