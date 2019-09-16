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
 * @file rc4EncryptKernel.cpp
 * @brief kernel code of Rivest Cipher 4 (also known as ARC4 or ARCFOUR).
 * This file is part of XF Security Library.
 *
 * @detail Containing scan, distribute, encrypt, merge, and write-out functions.
 *
 */

#include <ap_int.h>
#include <hls_stream.h>
#include "xf_security/rc4.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

// @brief analyze the configurations and scan the text block by block.
// XXX this implementation only support the message length not greater than 2^128 byes.
template <unsigned int _burstLength, unsigned int _channelNumber>
static void readBlock(ap_uint<512>* ptr,
                      hls::stream<ap_uint<512> >& textBlkStrm,
                      hls::stream<ap_uint<128> >& msgNumStrm1,
                      hls::stream<ap_uint<128> >& msgNumStrm2,
                      hls::stream<ap_uint<64> >& taskNumStrm1,
                      hls::stream<ap_uint<64> >& taskNumStrm2,
                      hls::stream<ap_uint<64> >& taskNumStrm3,
                      hls::stream<ap_uint<512> >& keyBlkStrm,
                      hls::stream<ap_uint<16> >& keyLenStrm) {
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering readBlock..." << std::endl;
#endif

    // cipherkey
    ap_uint<8> key;

    // scan for the configurations data
    ap_uint<512> axiBlock = ptr[0];

    // number of message blocks in byte
    ap_uint<128> msgNum = axiBlock.range(127, 0);
    // inform other processes
    msgNumStrm1.write(msgNum);
    msgNumStrm2.write(msgNum);

    // number of tasks in a single PCIe block
    ap_uint<64> taskNum = axiBlock.range(191, 128);
    // inform other processes
    taskNumStrm1.write(taskNum);
    taskNumStrm2.write(taskNum);
    taskNumStrm3.write(taskNum);

    // key length in byte
    ap_uint<16> keyLen = axiBlock.range(207, 192);
    keyLenStrm.write(keyLen);

LOOP_SCAN_KEY:
    for (unsigned int i = 0; i < _channelNumber * 4; i++) {
#pragma HLS pipeline II = 1
        axiBlock = ptr[i + 1];
        keyBlkStrm.write(axiBlock);
    }

// scan for the text messages
LOOP_SCAN_TEXT:
    for (ap_uint<192> i = 0; i < msgNum * taskNum * _channelNumber / 64; i += _burstLength) {
        // set the burst length for each burst read
        const int burstLen = ((i + _burstLength) > msgNum * taskNum * _channelNumber / 64)
                                 ? (int)(msgNum * taskNum * _channelNumber / 64 - i)
                                 : _burstLength;

        // do a burst read
        for (int j = 0; j < burstLen; ++j) {
#pragma HLS pipeline II = 1
            ap_uint<512> t = ptr[_channelNumber * 4 + 1 + i + j];
            textBlkStrm.write(t);
        }
    }
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting readBlock..." << std::endl;
#endif
} // end readBlock

// @brief split the text block into multi-channel text streams,
// and duplicate the key streams.
template <unsigned int _channelNumber>
static void splitText(hls::stream<ap_uint<512> >& textBlkStrm,
                      hls::stream<ap_uint<512> >& keyBlkStrm,
                      hls::stream<ap_uint<16> >& keyLenStrm,
                      hls::stream<ap_uint<128> >& msgNumStrm,
                      hls::stream<ap_uint<64> >& taskNumStrm,
                      hls::stream<ap_uint<8> > textStrm[_channelNumber],
                      hls::stream<bool> endTextStrm[_channelNumber],
                      hls::stream<ap_uint<8> > cipherkeyStrm[_channelNumber],
                      hls::stream<bool> endCipherkeyStrm[_channelNumber]) {
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering splitText..." << std::endl;
#endif

    const unsigned int blockII = 64 / _channelNumber;

    // number of message blocks in byte
    ap_uint<128> msgNum = msgNumStrm.read();

    // number of tasks in a single PCIe block
    ap_uint<64> taskNum = taskNumStrm.read();

    // key length in byte
    ap_uint<16> keyLen = keyLenStrm.read();

    // key buffer
    ap_uint<512> keyBlock[4 * _channelNumber];
#pragma HLS array_partition variable = keyBlock complete dim = 1

LOOP_BUFF_KEY:
    for (unsigned int ch = 0; ch < _channelNumber; ch++) {
    LOOP_SINGLE_CH:
        for (unsigned char j = 0; j < 4; j++) {
#pragma HLS pipeline II = 1
            keyBlock[ch * 4 + j] = keyBlkStrm.read();
        }
    }

LOOP_MULTI_TASK:
    for (ap_uint<64> n = 0; n < taskNum; n++) {
    LOOP_SEND_KEY:
        for (unsigned int ch = 0; ch < _channelNumber; ch++) {
#pragma HLS unroll
        LOOP_KEY_BYTE:
            for (ap_uint<16> keyPtr = 0; keyPtr < keyLen; keyPtr++) {
#pragma HLS pipeline II = 1
                cipherkeyStrm[ch].write(keyBlock[keyPtr / 64 + ch * 4].range((keyPtr % 64) * 8 + 7, (keyPtr % 64) * 8));
                endCipherkeyStrm[ch].write(false);
            }
        }

    LOOP_END_KEY:
        for (unsigned int ch = 0; ch < _channelNumber; ch++) {
#pragma HLS unroll
            endCipherkeyStrm[ch].write(true);
        }

        unsigned char ch = 0;

    LOOP_SPLIT_TEXT:
        for (ap_uint<128> i = 0; i < msgNum * _channelNumber / 64; i++) {
#pragma HLS pipeline II = blockII
            // read a text block
            ap_uint<512> textBlk = textBlkStrm.read();

        LOOP_SEND_TEXT:
            for (unsigned int i = 0; i < 64 / _channelNumber; i++) {
#pragma HLS pipeline II = 1
            LOOP_TEXT_CH:
                for (unsigned int ch = 0; ch < _channelNumber; ch++) {
#pragma HLS unroll
                    textStrm[ch].write(
                        textBlk.range(i * 8 + 7 + 64 / _channelNumber * ch * 8, i * 8 + 64 / _channelNumber * ch * 8));
                    endTextStrm[ch].write(false);
                }
            }
        }

    // send the end flag for current task
    LOOP_END_TEXT:
        for (unsigned int ch = 0; ch < _channelNumber; ++ch) {
#pragma HLS unroll
            endTextStrm[ch].write(true);
        }
    }
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting splitText..." << std::endl;
#endif
} // end splitText

// @brief top of scan
template <unsigned int _channelNumber, unsigned int _burstLength>
static void scanMultiChannel(ap_uint<512>* ptr,
                             hls::stream<ap_uint<8> > cipherkeyStrm[_channelNumber],
                             hls::stream<bool> endCipherkeyStrm[_channelNumber],
                             hls::stream<ap_uint<8> > textStrm[_channelNumber],
                             hls::stream<bool> endTextStrm[_channelNumber],
                             hls::stream<ap_uint<128> >& msgNumStrm,
                             hls::stream<ap_uint<64> >& taskNumStrm,
                             hls::stream<ap_uint<64> >& taskNumStrm2) {
#pragma HLS dataflow
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering scanMultiChannel..." << std::endl;
#endif

    enum { textFifoDepth = 2 * _burstLength };
    enum { keyFifoDepth = 2 * 4 * _channelNumber };

    hls::stream<ap_uint<512> > textBlkStrm("textBlkStrm");
#pragma HLS stream variable = textBlkStrm depth = textFifoDepth
#pragma HLS RESOURCE variable = textBlkStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<128> > msgNumStrm1;
#pragma HLS stream variable = msgNumStrm1 depth = 2
#pragma HLS RESOURCE variable = msgNumStrm1 core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > taskNumStrm1;
#pragma HLS stream variable = taskNumStrm1 depth = 2
#pragma HLS RESOURCE variable = taskNumStrm1 core = FIFO_LUTRAM
    hls::stream<ap_uint<512> > keyBlkStrm;
#pragma HLS stream variable = keyBlkStrm depth = keyFifoDepth
#pragma HLS resource variable = keyBlkStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<16> > keyLenStrm;
#pragma HLS stream variable = keyLenStrm depth = 2
#pragma HLS resource variable = keyLenStrm core = FIFO_LUTRAM

    readBlock<_burstLength, _channelNumber>(ptr, textBlkStrm, msgNumStrm, msgNumStrm1, taskNumStrm, taskNumStrm1,
                                            taskNumStrm2, keyBlkStrm, keyLenStrm);

    splitText<_channelNumber>(textBlkStrm, keyBlkStrm, keyLenStrm, msgNumStrm1, taskNumStrm1, textStrm, endTextStrm,
                              cipherkeyStrm, endCipherkeyStrm);
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting scanMultiChannel..." << std::endl;
#endif
} // end scanMultiChannel

// @brief cipher mode in parallel
template <unsigned int _channelNumber>
static void cipherModeParallel(hls::stream<ap_uint<8> > cipherkeyStrm[_channelNumber],
                               hls::stream<bool> endCipherkeyStrm[_channelNumber],
                               hls::stream<ap_uint<8> > textInStrm[_channelNumber],
                               hls::stream<bool> endTextInStrm[_channelNumber],
                               hls::stream<ap_uint<8> > textOutStrm[_channelNumber],
                               hls::stream<bool> endTextOutStrm[_channelNumber]) {
#pragma HLS dataflow
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering cipherModeParallel..." << std::endl;
#endif

LOOP_UNROLL_CORE:
    for (unsigned int m = 0; m < _channelNumber; m++) {
#pragma HLS unroll
        // XXX cipher mode core is called here
        xf::security::rc4(cipherkeyStrm[m], endCipherkeyStrm[m], textInStrm[m], endTextInStrm[m], textOutStrm[m],
                          endTextOutStrm[m]);
    }
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting cipherModeParallel..." << std::endl;
#endif
} // end cipherModeParallel

// @brief run tasks in sequence
template <unsigned int _channelNumber>
static void cipherModeProcess(hls::stream<ap_uint<64> >& taskNumStrm,
                              hls::stream<ap_uint<8> > cipherkeyStrm[_channelNumber],
                              hls::stream<bool> endCipherkeyStrm[_channelNumber],
                              hls::stream<ap_uint<8> > textInStrm[_channelNumber],
                              hls::stream<bool> endTextInStrm[_channelNumber],
                              hls::stream<ap_uint<8> > textOutStrm[_channelNumber],
                              hls::stream<bool> endTextOutStrm[_channelNumber]) {
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering cipherModeProcess..." << std::endl;
#endif
    // number of tasks
    ap_uint<64> taskNum = taskNumStrm.read();

// call paralleled cipher mode taskNum times
LOOP_MULTI_TASK:
    for (ap_uint<64> i = 0; i < taskNum; i++) {
        cipherModeParallel<_channelNumber>(cipherkeyStrm, endCipherkeyStrm, textInStrm, endTextInStrm, textOutStrm,
                                           endTextOutStrm);
    }
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting cipherModeProcess..." << std::endl;
#endif
} // end cipherModeProcess

// @brief merge the multi-channel result into block stream
template <unsigned int _burstLength, unsigned int _channelNumber>
static void mergeResult(hls::stream<ap_uint<128> >& msgNumStrm,
                        hls::stream<ap_uint<64> >& taskNumStrm,
                        hls::stream<ap_uint<8> > textStrm[_channelNumber],
                        hls::stream<bool> endTextStrm[_channelNumber],
                        hls::stream<ap_uint<512> >& outStrm,
                        hls::stream<unsigned int>& burstLenStrm) {
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering mergeResult..." << std::endl;
#endif

    const unsigned int blockII = 64 / _channelNumber;
    // burst length for each wirte-out operation
    unsigned int burstLen = 0;

    // number of message blocks in 128-bit
    ap_uint<64> msgNum = msgNumStrm.read();

    // number of tasks in a single PCIe block
    ap_uint<64> taskNum = taskNumStrm.read();

    bool e[_channelNumber];
LOOP_TASK:
    for (ap_uint<64> n = 0; n < taskNum; n++) {
    LOOP_MERGE:
        for (ap_uint<128> i = 0; i < msgNum * _channelNumber / 64; i++) {
#pragma HLS pipeline II = blockII
            ap_uint<512> axiBlock;
        LOOP_RECV_TEXT:
            for (unsigned int i = 0; i < 64 / _channelNumber; i++) {
#pragma HLS pipeline II = 1
            LOOP_EACH_CH:
                for (unsigned int ch = 0; ch < _channelNumber; ch++) {
#pragma HLS unroll
                    axiBlock.range(i * 8 + 7 + 64 / _channelNumber * ch * 8, i * 8 + 64 / _channelNumber * ch * 8) =
                        textStrm[ch].read();
                    e[ch] = endTextStrm[ch].read();
                }
            }

            // write-out a AXI block data (4 channels)
            outStrm.write(axiBlock);
            // set the burst length
            if (burstLen == _burstLength - 1) {
                burstLenStrm.write(_burstLength);
                burstLen = 0;
            } else {
                burstLen++;
            }
        }

        // remove the end flag for each task
        for (unsigned int i = 0; i < _channelNumber; i++) {
#pragma HLS unroll
            e[i] = endTextStrm[i].read();
        }
    }

    // deal with the condition that we didn't hit the burst boundary
    if (burstLen != 0) {
        burstLenStrm.write(burstLen);
    }
    // end the burst write operation
    burstLenStrm.write(0);
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting mergeResult..." << std::endl;
#endif
} // end mergeResult

// @brief burst write out to DDR
template <unsigned int _burstLength>
static void writeOut(hls::stream<unsigned int>& burstLenStrm,
                     hls::stream<ap_uint<512> >& blockStrm,
                     ap_uint<512>* ptr) {
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Entering writeOut..." << std::endl;
#endif
    ap_uint<128> offset = 0;
    unsigned int bLen = burstLenStrm.read();
    while (bLen) {
    LOOP_BURST_WRITE:
        for (unsigned int j = 0; j < bLen; ++j) {
#pragma HLS pipeline II = 1
            ap_uint<512> block = blockStrm.read();
            ptr[offset * _burstLength + j] = block;
        }
        offset++;
        bLen = burstLenStrm.read();
    }
#if !defined(__SYNTHESIS__) && _XF_SECURITY_RC4_DEBUG_ == 1
    std::cout << "Exiting writeOut..." << std::endl;
#endif
} // end writeOut

// @brief top of kernel
extern "C" void rc4EncryptKernel_2(ap_uint<512> inputData[(1 << 30) + 100], ap_uint<512> outputData[1 << 30]) {
#pragma HLS dataflow

    const unsigned int _channelNumber = 16;
    const unsigned int _burstLength = 128;
    const unsigned int bufferDepth = _burstLength * 2;

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

    hls::stream<ap_uint<8> > cipherkeyStrm[_channelNumber];
#pragma HLS stream variable = cipherkeyStrm depth = 512
#pragma HLS resource variable = cipherkeyStrm core = FIFO_LUTRAM
    hls::stream<bool> endCipherkeyStrm[_channelNumber];
#pragma HLS stream variable = endCipherkeyStrm depth = 512
#pragma HLS resource variable = endCipherkeyStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<8> > textInStrm[_channelNumber];
#pragma HLS stream variable = textInStrm depth = 128
#pragma HLS resource variable = textInStrm core = FIFO_LUTRAM
    hls::stream<bool> endTextInStrm[_channelNumber];
#pragma HLS stream variable = endTextInStrm depth = 128
#pragma HLS resource variable = endTextInStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<128> > msgNumStrm;
#pragma HLS stream variable = msgNumStrm depth = 4
#pragma HLS resource variable = msgNumStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > taskNumStrm;
#pragma HLS stream variable = taskNumStrm depth = 4
#pragma HLS resource variable = taskNumStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > taskNumStrm2;
#pragma HLS stream variable = taskNumStrm2 depth = 4
#pragma HLS resource variable = taskNumStrm2 core = FIFO_LUTRAM
    hls::stream<ap_uint<8> > textOutStrm[_channelNumber];
#pragma HLS stream variable = textOutStrm depth = 128
#pragma HLS resource variable = textOutStrm core = FIFO_LUTRAM
    hls::stream<bool> endTextOutStrm[_channelNumber];
#pragma HLS stream variable = endTextOutStrm depth = 128
#pragma HLS resource variable = endTextOutStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<512> > outStrm("outStrm");
#pragma HLS stream variable = outStrm depth = bufferDepth
#pragma HLS resource variable = outStrm core = FIFO_BRAM
    hls::stream<unsigned int> burstLenStrm;
#pragma HLS stream variable = burstLenStrm depth = 4

    scanMultiChannel<_channelNumber, _burstLength>(inputData, cipherkeyStrm, endCipherkeyStrm, textInStrm,
                                                   endTextInStrm, msgNumStrm, taskNumStrm, taskNumStrm2);

    cipherModeProcess<_channelNumber>(taskNumStrm2, cipherkeyStrm, endCipherkeyStrm, textInStrm, endTextInStrm,
                                      textOutStrm, endTextOutStrm);

    mergeResult<_burstLength, _channelNumber>(msgNumStrm, taskNumStrm, textOutStrm, endTextOutStrm, outStrm,
                                              burstLenStrm);

    writeOut<_burstLength>(burstLenStrm, outStrm, outputData);

} // end rc4EncryptKernel_2
