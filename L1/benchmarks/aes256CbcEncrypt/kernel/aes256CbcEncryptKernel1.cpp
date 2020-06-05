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
#include "xf_security/cbc.hpp"

// @brief analyze the configurations and scan the text block by block
template <unsigned int _burstLength, unsigned int _channelNumber, unsigned int _keyWidth>
static void readBlock(ap_uint<512>* ptr,
                      hls::stream<ap_uint<512> >& textBlkStrm,
                      hls::stream<ap_uint<64> >& msgNumStrm1,
                      hls::stream<ap_uint<64> >& msgNumStrm2,
                      hls::stream<ap_uint<64> >& taskNumStrm1,
                      hls::stream<ap_uint<64> >& taskNumStrm2,
                      hls::stream<ap_uint<64> >& taskNumStrm3,
                      hls::stream<ap_uint<128> >& IVStrm,
                      hls::stream<ap_uint<_keyWidth> >& cipherkeyStrm) {
    // number of message blocks in 128-bit
    ap_uint<64> msgNum;

    // number of tasks in a single PCIe block
    ap_uint<64> taskNum;

    // initialization vector
    ap_uint<128> IV;

    // cipherkey
    ap_uint<_keyWidth> key;

// scan for the configurations data
LOOP_SCAN_CFG:
    for (unsigned char i = 0; i < _channelNumber; i++) {
#pragma HLS pipeline II = 1
        // read a block from DDR
        ap_uint<512> axiBlock = ptr[i];

        // number of message blocks in 128-bit
        msgNum = axiBlock.range(511, 448);

        // number of tasks in a single PCIe block
        taskNum = axiBlock.range(447, 384);

        // intilization vector
        IV = axiBlock.range(383, 256);

        // ciphyerkey
        key = axiBlock.range(_keyWidth - 1, 0);
        if (i == 0) {
            // inform splitText
            msgNumStrm1.write(msgNum);
            taskNumStrm1.write(taskNum);
            IVStrm.write(IV);
            cipherkeyStrm.write(key);
            // inform mergeResult
            msgNumStrm2.write(msgNum);
            taskNumStrm2.write(taskNum);
            // inform cipher mode
            taskNumStrm3.write(taskNum);
        }
    }

// scan for the text messages
LOOP_SCAN_TEXT:
    for (ap_uint<64> i = 0; i < msgNum * taskNum * _channelNumber / 4; i += _burstLength) {
        // set the burst length for each burst read
        const int burstLen = ((i + _burstLength) > msgNum * taskNum * _channelNumber / 4)
                                 ? (int)(msgNum * taskNum * _channelNumber - i)
                                 : _burstLength;

        // do a burst read
        for (int j = 0; j < burstLen; ++j) {
#pragma HLS pipeline II = 1
            ap_uint<512> t = ptr[_channelNumber + i + j];
            textBlkStrm.write(t);
        }
    }
} // end readBlock

// @brief split the text block into multi-channel text streams
template <unsigned int _channelNumber, unsigned int _keyWidth>
static void splitText(hls::stream<ap_uint<512> >& textBlkStrm,
                      hls::stream<ap_uint<64> >& msgNumStrm,
                      hls::stream<ap_uint<64> >& taskNumStrm,
                      hls::stream<ap_uint<128> >& IVInStrm,
                      hls::stream<ap_uint<_keyWidth> >& cipherkeyInStrm,
                      hls::stream<ap_uint<128> > textStrm[_channelNumber],
                      hls::stream<bool> endTextStrm[_channelNumber],
                      hls::stream<ap_uint<128> > IVStrm[_channelNumber],
                      hls::stream<ap_uint<_keyWidth> > cipherkeyStrm[_channelNumber]) {
    // initialization vector
    ap_uint<128> IV = IVInStrm.read();

    // cipherkey
    ap_uint<_keyWidth> key = cipherkeyInStrm.read();

    // number of message blocks in 128-bit
    ap_uint<64> msgNum = msgNumStrm.read();

    // number of tasks in a single PCIe block
    ap_uint<64> taskNum = taskNumStrm.read();

LOOP_MULTI_TASK:
    for (ap_uint<64> n = 0; n < taskNum; n++) {
        unsigned char ch = 0;

        ap_uint<64> iEnd = msgNum * _channelNumber / 4;
    LOOP_SPLIT_TEXT:
        for (ap_uint<64> i = 0; i < iEnd; i++) {
#pragma HLS pipeline II = 1
            // read 4 text messages in 1 block
            ap_uint<512> textBlk = textBlkStrm.read();
            ap_uint<128> blockReg[4];
#pragma HLS array_partition variable = blockReg complete
            blockReg[3] = textBlk.range(511, 384);
            blockReg[2] = textBlk.range(383, 256);
            blockReg[1] = textBlk.range(255, 128);
            blockReg[0] = textBlk.range(127, 0);
        LOOP_DISTRIBUTION:
            for (unsigned char n = 0; n < _channelNumber; n++) {
#pragma HLS unroll
                if ((n >= ch) && (n < (ch + 4))) {
                    textStrm[n].write(blockReg[n & 0x3]);
                    endTextStrm[n].write(false);
                }
            }

            // increment the channel pointer
            if (ch == (_channelNumber - 4)) {
                ch = 0;
            } else {
                ch += 4;
            }
        }

    LOOP_SEND_IV_KEY:
        for (unsigned char j = 0; j < _channelNumber; j++) {
#pragma HLS unroll
            IVStrm[j].write(IV);
            cipherkeyStrm[j].write(key);
        }

    // send the end flag for current task
    LOOP_END_FLAG:
        for (unsigned int ch = 0; ch < _channelNumber; ++ch) {
#pragma HLS unroll
            endTextStrm[ch].write(true);
        }
    }
} // end splitText

// @brief top of scan
template <unsigned int _channelNumber, unsigned int _keyWidth, unsigned int _burstLength>
static void scanMultiChannel(ap_uint<512>* ptr,
                             hls::stream<ap_uint<128> > IVStrm[_channelNumber],
                             hls::stream<ap_uint<_keyWidth> > cipherkeyStrm[_channelNumber],
                             hls::stream<ap_uint<128> > textStrm[_channelNumber],
                             hls::stream<bool> endTextStrm[_channelNumber],
                             hls::stream<ap_uint<64> >& msgNumStrm,
                             hls::stream<ap_uint<64> >& taskNumStrm,
                             hls::stream<ap_uint<64> >& taskNumStrm2) {
#pragma HLS dataflow

    enum { fifoDepth = 2 * _burstLength };

    hls::stream<ap_uint<512> > textBlkStrm("textBlkStrm");
#pragma HLS stream variable = textBlkStrm depth = fifoDepth
#pragma HLS RESOURCE variable = textBlkStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > msgNumStrm1;
#pragma HLS stream variable = msgNumStrm1 depth = 2
#pragma HLS RESOURCE variable = msgNumStrm1 core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > taskNumStrm1;
#pragma HLS stream variable = taskNumStrm1 depth = 2
#pragma HLS RESOURCE variable = taskNumStrm1 core = FIFO_LUTRAM
    hls::stream<ap_uint<128> > IVInStrm;
#pragma HLS stream variable = IVInStrm depth = 32
#pragma HLS resource variable = IVInStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<_keyWidth> > cipherkeyInStrm;
#pragma HLS stream variable = cipherkeyInStrm depth = 32
#pragma HLS resource variable = cipherkeyInStrm core = FIFO_LUTRAM

    readBlock<_burstLength, _channelNumber, _keyWidth>(ptr, textBlkStrm, msgNumStrm, msgNumStrm1, taskNumStrm,
                                                       taskNumStrm1, taskNumStrm2, IVInStrm, cipherkeyInStrm);

    splitText<_channelNumber, _keyWidth>(textBlkStrm, msgNumStrm1, taskNumStrm1, IVInStrm, cipherkeyInStrm, textStrm,
                                         endTextStrm, IVStrm, cipherkeyStrm);
} // end scanMultiChannel

// @brief cipher mode in parallel
template <unsigned int _channelNumber, unsigned int _keyWidth>
static void cipherModeParallel(hls::stream<ap_uint<128> > IVStrm[_channelNumber],
                               hls::stream<ap_uint<_keyWidth> > cipherkeyStrm[_channelNumber],
                               hls::stream<ap_uint<128> > textInStrm[_channelNumber],
                               hls::stream<bool> endTextInStrm[_channelNumber],
                               hls::stream<ap_uint<128> > textOutStrm[_channelNumber],
                               hls::stream<bool> endTextOutStrm[_channelNumber]) {
#pragma HLS dataflow

    for (unsigned int m = 0; m < _channelNumber; m++) {
#pragma HLS unroll
        // XXX cipher mode core is called here
        // the API called should be accordance with the _keyWidth
        xf::security::internal::aesCbcEncrypt<_keyWidth>(textInStrm[m], endTextInStrm[m], cipherkeyStrm[m], IVStrm[m],
                                                         textOutStrm[m], endTextOutStrm[m]);
    }
} // end cipherModeParallel

// @brief run tasks in sequence
template <unsigned int _channelNumber, unsigned int _keyWidth>
static void cipherModeProcess(hls::stream<ap_uint<64> >& taskNumStrm,
                              hls::stream<ap_uint<128> > IVStrm[_channelNumber],
                              hls::stream<ap_uint<_keyWidth> > cipherkeyStrm[_channelNumber],
                              hls::stream<ap_uint<128> > textInStrm[_channelNumber],
                              hls::stream<bool> endTextInStrm[_channelNumber],
                              hls::stream<ap_uint<128> > textOutStrm[_channelNumber],
                              hls::stream<bool> endTextOutStrm[_channelNumber]) {
    // number of tasks
    ap_uint<64> taskNum = taskNumStrm.read();

    // call paralleled cipher mode taskNum times
    for (ap_uint<64> i = 0; i < taskNum; i++) {
        cipherModeParallel<_channelNumber, _keyWidth>(IVStrm, cipherkeyStrm, textInStrm, endTextInStrm, textOutStrm,
                                                      endTextOutStrm);
    }
} // end cipherModeProcess

// @brief merge the multi-channel result into block stream
template <unsigned int _burstLength, unsigned int _channelNumber>
static void mergeResult(hls::stream<ap_uint<64> >& msgNumStrm,
                        hls::stream<ap_uint<64> >& taskNumStrm,
                        hls::stream<ap_uint<128> > textStrm[_channelNumber],
                        hls::stream<bool> endTextStrm[_channelNumber],
                        hls::stream<ap_uint<512> >& outStrm,
                        hls::stream<unsigned int>& burstLenStrm) {
    // burst length for each wirte-out operation
    unsigned int burstLen = 0;

    // number of message blocks in 128-bit
    ap_uint<64> msgNum = msgNumStrm.read();

    // number of tasks in a single PCIe block
    ap_uint<64> taskNum = taskNumStrm.read();

LOOP_TASK:
    for (ap_uint<64> n = 0; n < taskNum; n++) {
        unsigned char ch = 0;
        ap_uint<64> iEnd = msgNum * _channelNumber / 4;

    LOOP_MERGE_RESULT:
        for (ap_uint<64> i = 0; i < iEnd; i++) {
#pragma HLS pipeline II = 1
            ap_uint<512> axiBlock;
            ap_uint<128> blockReg[4];
#pragma HLS array_partition variable = blockReg complete
        LOOP_MERGE:
            for (unsigned char n = 0; n < _channelNumber; n++) {
                if ((n >= ch) && (n < (ch + 4))) {
                    blockReg[n & 0x3] = textStrm[n].read();
                    bool e = endTextStrm[n].read();
                }
            }
            axiBlock.range(511, 384) = blockReg[3];
            axiBlock.range(383, 256) = blockReg[2];
            axiBlock.range(255, 128) = blockReg[1];
            axiBlock.range(127, 0) = blockReg[0];

            // switch channels
            if (ch == (_channelNumber - 4)) {
                ch = 0;
            } else {
                ch += 4;
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
            bool e = endTextStrm[i].read();
        }
    }

    // deal with the condition that we didn't hit the burst boundary
    if (burstLen != 0) {
        burstLenStrm.write(burstLen);
    }
    // end the burst write operation
    burstLenStrm.write(0);
} // end mergeResult

// @brief burst write out to DDR
template <unsigned int _burstLength>
static void writeOut(hls::stream<unsigned int>& burstLenStrm,
                     hls::stream<ap_uint<512> >& blockStrm,
                     ap_uint<512>* ptr) {
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
} // end writeOut

// @brief top of kernel
extern "C" void aes256CbcEncryptKernel_1(ap_uint<512> inputData[(1 << 30) + 100], ap_uint<512> outputData[1 << 30]) {
#pragma HLS dataflow

    const unsigned int _channelNumber = 12;
    const unsigned int _keyWidth = 256;
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

    hls::stream<ap_uint<128> > IVStrm[_channelNumber];
#pragma HLS stream variable = IVStrm depth = 32
#pragma HLS resource variable = IVStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<_keyWidth> > cipherkeyStrm[_channelNumber];
#pragma HLS stream variable = cipherkeyStrm depth = 32
#pragma HLS resource variable = cipherkeyStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<128> > textInStrm[_channelNumber];
#pragma HLS stream variable = textInStrm depth = 65
#pragma HLS resource variable = textInStrm core = FIFO_LUTRAM
    hls::stream<bool> endTextInStrm[_channelNumber];
#pragma HLS stream variable = endTextInStrm depth = 65
#pragma HLS resource variable = endTextInStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > msgNumStrm;
#pragma HLS stream variable = msgNumStrm depth = 64
#pragma HLS resource variable = msgNumStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > taskNumStrm;
#pragma HLS stream variable = taskNumStrm depth = 64
#pragma HLS resource variable = taskNumStrm core = FIFO_LUTRAM
    hls::stream<ap_uint<64> > taskNumStrm2;
#pragma HLS stream variable = taskNumStrm2 depth = 64
#pragma HLS resource variable = taskNumStrm2 core = FIFO_LUTRAM
    hls::stream<ap_uint<128> > textOutStrm[_channelNumber];
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

    scanMultiChannel<_channelNumber, _keyWidth, _burstLength>(inputData, IVStrm, cipherkeyStrm, textInStrm,
                                                              endTextInStrm, msgNumStrm, taskNumStrm, taskNumStrm2);

    cipherModeProcess<_channelNumber, _keyWidth>(taskNumStrm2, IVStrm, cipherkeyStrm, textInStrm, endTextInStrm,
                                                 textOutStrm, endTextOutStrm);

    mergeResult<_burstLength, _channelNumber>(msgNumStrm, taskNumStrm, textOutStrm, endTextOutStrm, outStrm,
                                              burstLenStrm);

    writeOut<_burstLength>(burstLenStrm, outStrm, outputData);

} // end aes256CbcEncryptKernel_1
