/*
 * (c) Copyright 2020 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef _XFCOMPRESSION_ZLIB_COMPRESS_DETAILS_HPP_
#define _XFCOMPRESSION_ZLIB_COMPRESS_DETAILS_HPP_

/**
 * @file zlib_compress_details.hpp
 * @brief Header for modules used in ZLIB compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <ap_int.h>
#include <fstream>
#include <iostream>
#include <stdlib.h>
#include <string>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "zlib_specs.hpp"
#include "lz_optional.hpp"
#include "lz_compress.hpp"
#include "huffman_treegen.hpp"
#include "huffman_encoder.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_downsizer.hpp"
#include "stream_upsizer.hpp"
#include "xf_security/adler32.hpp"
#include "xf_security/crc32.hpp"

namespace xf {
namespace compression {
namespace details {

template <int SLAVES>
void streamEosDistributor(hls::stream<bool>& inStream, hls::stream<bool> outStream[SLAVES]) {
    do {
        bool i = inStream.read();
        for (int n = 0; n < SLAVES; n++) outStream[n] << i;
        if (i == 0) break;
    } while (1);
}

template <int SLAVES>
void streamSizeDistributor(hls::stream<uint32_t>& inStream, hls::stream<uint32_t> outStream[SLAVES]) {
    do {
        uint32_t i = inStream.read();
        for (int n = 0; n < SLAVES; n++) outStream[n] << i;
        if (i == 0) break;
    } while (1);
}

template <int DWIDTH>
void streamDuplicator(hls::stream<ap_uint<DWIDTH> >& inHlsStream,
                      hls::stream<bool>& inHlsStreamEos,
                      hls::stream<ap_uint<DWIDTH> >& outZlibStream,
                      hls::stream<ap_uint<DWIDTH> >& outChecksumStream) {
    for (bool eos = inHlsStreamEos.read(); eos == false; eos = inHlsStreamEos.read()) {
        ap_uint<DWIDTH> tempVal = inHlsStream.read();
        outZlibStream << tempVal;
        outChecksumStream << tempVal;
    }
}

template <int DWIDTH, int STRATEGY = 0> // STRATEGY: 0: GZIP; 1: ZLIB
void checksumWrapper(hls::stream<ap_uint<DWIDTH> >& inStream,
                     hls::stream<uint32_t>& inSizeStream,
                     hls::stream<ap_uint<32> >& checksum) {
    constexpr int c_parallel_byte = DWIDTH / 8;
    hls::stream<ap_uint<32> > inLenStrm;
    hls::stream<ap_uint<32> > checkStrm;
    hls::stream<bool> endInLenStrm;
    hls::stream<bool> endOutStrm;

#pragma HLS STREAM variable = inLenStrm depth = 4
#pragma HLS STREAM variable = endInLenStrm depth = 4
#pragma HLS STREAM variable = endOutStrm depth = 4

    for (ap_uint<32> size = inSizeStream.read(); size != 0; size = inSizeStream.read()) {
        if (STRATEGY == 0)
            checkStrm << ~0;
        else
            checkStrm << 0;
        inLenStrm << size;
        endInLenStrm << 0;
        endInLenStrm << 1;
        if (STRATEGY == 0) {
            xf::security::crc32<c_parallel_byte>(checkStrm, inStream, inLenStrm, endInLenStrm, checksum, endOutStrm);
        } else {
            xf::security::adler32<c_parallel_byte>(checkStrm, inStream, inLenStrm, endInLenStrm, checksum, endOutStrm);
        }
        endOutStrm.read();
        endOutStrm.read();
    }
}

template <int IN_DWIDTH, int BLCK_SIZE, int MIN_BLCK_SIZE = 1024>
void streamBlockMaker(hls::stream<ap_uint<IN_DWIDTH> >& inStream,
                      hls::stream<ap_uint<IN_DWIDTH> >& outputStream,
                      hls::stream<ap_uint<IN_DWIDTH> >& outStoredData,
                      hls::stream<uint32_t>& outStoredBlckSize,
                      hls::stream<uint32_t>& outputBlockSizeStream,
                      hls::stream<uint32_t>& inSizeStream) {
    constexpr int c_parallelByte = IN_DWIDTH / 8;
    for (uint32_t inputSize = inSizeStream.read(); inputSize != 0; inputSize = inSizeStream.read()) {
        ap_uint<32> no_blocks = (inputSize - 1) / BLCK_SIZE + 1;
        uint32_t readSize = 0;

        for (uint32_t i = 0; i < no_blocks; i++) {
            uint32_t block_length = BLCK_SIZE;
            if (readSize + BLCK_SIZE > inputSize) block_length = inputSize - readSize;

            // Very Small Block Handling
            if (block_length < MIN_BLCK_SIZE)
                outStoredBlckSize << block_length;
            else
                outputBlockSizeStream << block_length;

            for (uint32_t j = 0; j < block_length; j += c_parallelByte) {
#pragma HLS PIPELINE II = 1
                ap_uint<IN_DWIDTH> tmpOut = inStream.read();
                if (block_length < MIN_BLCK_SIZE) {
                    outStoredData << tmpOut;
                } else {
                    outputStream << tmpOut;
                }
            }
            readSize += BLCK_SIZE;
        }
    }

    outputBlockSizeStream << 0;
}

template <int DATAWIDTH = 512, int BURST_SIZE = 16, int MAX_BLOCK_SIZE = 64 * 1024>
void mm2sZlib(const ap_uint<DATAWIDTH>* in,
              hls::stream<ap_uint<DATAWIDTH> >& outStream,
              hls::stream<uint32_t>& outSizeStream,
              uint32_t input_size) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int block_length = MAX_BLOCK_SIZE;
    const int no_blocks = (input_size - 1) / block_length + 1;
    uint32_t readBlockSize = 0, rIdx = 0;

    for (uint32_t i = 0; i < no_blocks; i++) {
        uint32_t transferSize = block_length;
        if (readBlockSize + block_length > input_size) transferSize = input_size - readBlockSize;
        outSizeStream << transferSize;
        readBlockSize += transferSize;
        uint32_t readSize = 0;
        bool is_pending = true;
        while (is_pending) {
            uint32_t pendingBytes = (transferSize > readSize) ? (transferSize - readSize) : 0;
            is_pending = (pendingBytes > 0) ? true : false;
            if (pendingBytes) {
                uint32_t pendingWords = (pendingBytes - 1) / c_wordSize + 1;
                uint32_t burstSize = (pendingWords > BURST_SIZE) ? BURST_SIZE : pendingWords;
            gmem_read:
                for (uint32_t midx = 0; midx < burstSize; midx++) {
                    outStream << in[rIdx + midx];
                }
                rIdx += burstSize;
                readSize += burstSize * c_wordSize;
            }
        }
    }
    outSizeStream << 0;
}

template <int IN_DATAWIDTH, int OUT_DATAWIDTH>
void streamDownSizerZlib(hls::stream<ap_uint<IN_DATAWIDTH> >& inStream,
                         hls::stream<uint32_t>& inSizeStream,
                         hls::stream<ap_uint<OUT_DATAWIDTH> >& outStream,
                         hls::stream<uint32_t>& outSizeStream) {
    const int c_byteWidth = 8;
    const int c_inputWord = IN_DATAWIDTH / c_byteWidth;
    const int c_outWord = OUT_DATAWIDTH / c_byteWidth;
    const int factor = c_inputWord / c_outWord;
    ap_uint<IN_DATAWIDTH> inBuffer = 0;

downsizer_top:
    for (uint32_t inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        outSizeStream << inSize;
        uint32_t outSizeV = (inSize - 1) / c_outWord + 1;
    downsizer_assign:
        for (uint32_t itr = 0; itr < outSizeV; itr++) {
#pragma HLS PIPELINE II = 1
            int idx = itr % factor;
            if (idx == 0) inBuffer = inStream.read();
            ap_uint<OUT_DATAWIDTH> tmpValue = inBuffer.range((idx + 1) * OUT_DATAWIDTH - 1, idx * OUT_DATAWIDTH);
            outStream << tmpValue;
        }
    }
    outSizeStream << 0;
}

template <int IN_WIDTH, int OUT_WIDTH>
void upsizerZlib(hls::stream<ap_uint<IN_WIDTH> >& inStream,
                 hls::stream<bool>& inStreamEos,
                 hls::stream<bool>& inFileEos,
                 hls::stream<bool>& outFileEos,
                 hls::stream<ap_uint<OUT_WIDTH> >& outStream,
                 hls::stream<bool>& outStreamEos) {
    const int c_byteWidth = 8;
    const int c_upsizeFactor = OUT_WIDTH / IN_WIDTH;
    const int c_wordSize = OUT_WIDTH / c_byteWidth;

    while (1) {
        bool eosFile = inFileEos.read();
        outFileEos << eosFile;
        if (eosFile == true) break;

        ap_uint<OUT_WIDTH> outBuffer = 0;
        uint32_t byteIdx = 0;
        bool eos_flag = false;
    stream_upsizer:
        do {
#pragma HLS PIPELINE II = 1
            if (byteIdx == c_upsizeFactor) {
                outStream << outBuffer;
                outStreamEos << false;
                byteIdx = 0;
            }
            ap_uint<IN_WIDTH> inValue = inStream.read();
            eos_flag = inStreamEos.read();
            outBuffer.range((byteIdx + 1) * IN_WIDTH - 1, byteIdx * IN_WIDTH) = inValue;
            byteIdx++;
        } while (eos_flag == false);

        if (byteIdx) {
            outStream << outBuffer;
            outStreamEos << 0;
        }

        outStream << 0;
        outStreamEos << 1;
    }
}

template <int DATAWIDTH = 512, int BURST_SIZE = 16, int MAX_BLOCK_SIZE = 64 * 1024>
void s2mmZlib(ap_uint<DATAWIDTH>* out,
              hls::stream<ap_uint<DATAWIDTH> >& inStream,
              hls::stream<bool>& inStreamEos,
              hls::stream<bool>& inFileEos,
              hls::stream<uint32_t>& inSizeStream,
              uint32_t* compressedSize) {
    const int c_byteSize = 8;
    const int c_wordSize = DATAWIDTH / c_byteSize;
    const int block_length = MAX_BLOCK_SIZE;
    const int blkStride = block_length / c_wordSize;

    uint32_t blkIdx = 0, blkCntr = 0;

    while (1) {
        bool eosFile = inFileEos.read();
        if (eosFile == true) break;

        bool eos = false;
        ap_uint<DATAWIDTH> dummy = 0;
    s2mm_eos_simple:
        for (int j = 0; eos == false; j += BURST_SIZE) {
            for (int i = 0; i < BURST_SIZE; i++) {
#pragma HLS PIPELINE II = 1
                ap_uint<DATAWIDTH> tmp = (eos == true) ? dummy : inStream.read();
                bool eos_tmp = (eos == true) ? true : inStreamEos.read();
                out[blkIdx + j + i] = tmp;
                eos = eos_tmp;
            }
        }
        blkIdx += blkStride;
        blkCntr++;
    }

    for (uint32_t j = 0; j < blkCntr; j++) {
        compressedSize[j] = inSizeStream.read();
    }
}

template <int IN_DWIDTH, int OUT_DWIDTH, int BLCK_LEN, int STRATEGY = 0>
void zlibCompressStreamingPacker(hls::stream<ap_uint<IN_DWIDTH> >& inStoredStream,
                                 hls::stream<ap_uint<OUT_DWIDTH> >& inStream,
                                 hls::stream<ap_uint<OUT_DWIDTH> >& outStream,
                                 hls::stream<bool>& inStreamEos,
                                 hls::stream<bool>& outStreamEos,
                                 hls::stream<uint32_t>& storedBlckSize,
                                 hls::stream<bool>& inFileEos,
                                 hls::stream<uint32_t>& inputCmpBlockSizeStream,
                                 hls::stream<uint32_t>& outputTotalSizeStream,
                                 hls::stream<uint32_t>& inSizeStream,
                                 hls::stream<ap_uint<32> >& checksumStream,
                                 hls::stream<bool>& outNumBlockStreamEos) {
    constexpr int c_parallelByte = OUT_DWIDTH / 8;
    constexpr int c_format = 0x8B1F;
    constexpr int c_variant_realcode = 0x0808;
    for (ap_uint<32> inSize = inSizeStream.read(); inSize != 0; inSize = inSizeStream.read()) {
        uint32_t size = 0, lbuf_idx = 0, prodLbuf = 0;
        bool onlyOnce = true;
        ap_uint<32> no_blocks = (inSize - 1) / BLCK_LEN + 1;
        outNumBlockStreamEos << 0;
        // Gzip
        if (STRATEGY == 0) {
            outStream << c_format;
            outStreamEos << 0;
            outStream << c_variant_realcode;
            outStreamEos << 0;
            outStream << 0x0;
            outStreamEos << 0;
            outStream << 0x0;
            outStreamEos << 0;
            outStream << 0x0300;
            outStreamEos << 0;
            outStream << 0x0078;
            outStreamEos << 0;
            size += 12;
        }
        // Zlib
        else {
            outStream << 0x0178;
            outStreamEos << 0;
            size += 2;
        }
        ap_uint<2 * OUT_DWIDTH> lcl_buffer = 0;
        for (uint32_t i = 0; i < no_blocks; i++) {
            // One-time multiplication calculation for index
            prodLbuf = lbuf_idx * 8;
            uint32_t size_read = 0;
            // Stored Block Handling
            if (i == no_blocks - 1 && !storedBlckSize.empty()) {
                uint32_t sizeStrdBlck = storedBlckSize.read();
                // Stored Block Headers
                lcl_buffer.range(prodLbuf + IN_DWIDTH - 1, prodLbuf) = 0x0;
                lbuf_idx++;
                prodLbuf = lbuf_idx * 8;
                lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = sizeStrdBlck;
                outStream << lcl_buffer;
                outStreamEos << 0;
                lcl_buffer >>= OUT_DWIDTH;
                lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = ~sizeStrdBlck;
                outStream << lcl_buffer;
                outStreamEos << 0;
                lcl_buffer >>= OUT_DWIDTH;

                for (uint32_t sz = 0; sz < sizeStrdBlck; sz++) {
#pragma HLS PIPELINE II = 1
                    // Check for PARALLEL BYTE data and write to output stream
                    if (lbuf_idx >= c_parallelByte) {
                        outStream << lcl_buffer.range(OUT_DWIDTH - 1, 0);
                        outStreamEos << 0;
                        lcl_buffer >>= OUT_DWIDTH;
                        lbuf_idx -= c_parallelByte;
                    }
                    lcl_buffer.range((lbuf_idx * 8) + IN_DWIDTH - 1, lbuf_idx * 8) = inStoredStream.read();
                    lbuf_idx++;
                }
                size += (sizeStrdBlck + 5);
                continue;
            }

        loop_aligned:
            for (bool eos = inStreamEos.read(); eos == 0; eos = inStreamEos.read()) {
#pragma HLS PIPELINE II = 1
                // Reading Input Data
                lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = inStream.read();

                // Writing output into memory
                outStream << lcl_buffer.range(OUT_DWIDTH - 1, 0);
                outStreamEos << 0;
                // Shifting by Global datawidth
                lcl_buffer >>= OUT_DWIDTH;
                size_read += c_parallelByte;
            }

            uint32_t blockSize = inputCmpBlockSizeStream.read();
            size += blockSize;

            // Calculation for byte processing
            uint32_t leftBytes = blockSize - size_read;

            // Left bytes from each block
            lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = inStream.read();
            lbuf_idx += leftBytes;

            // Check for PARALLEL BYTE data and write to output stream
            if (lbuf_idx >= c_parallelByte) {
                outStream << lcl_buffer.range(OUT_DWIDTH - 1, 0);
                outStreamEos << 0;
                lcl_buffer >>= OUT_DWIDTH;
                lbuf_idx -= c_parallelByte;
            }
            inFileEos.read();
        }

        // Left Over Data Handling
        if (lbuf_idx && onlyOnce) {
            if (lbuf_idx < c_parallelByte) {
                lcl_buffer.range(OUT_DWIDTH - 1, (lbuf_idx * 8)) = 1;
                size++;
                onlyOnce = false;
            }
            outStream << lcl_buffer.range(OUT_DWIDTH - 1, 0);
            outStreamEos << 0;
            lcl_buffer >>= OUT_DWIDTH;
            lbuf_idx = 0;
        }

        // Last Block Processing
        if (lbuf_idx == 0 && onlyOnce) {
            lcl_buffer.range(IN_DWIDTH - 1, 0) = 1;
            lbuf_idx++;
            onlyOnce = false;
            size++;
        }

        prodLbuf = lbuf_idx * 8;
        lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = 0x0;
        outStream << lcl_buffer;
        outStreamEos << 0;
        lcl_buffer >>= OUT_DWIDTH;
        lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = 0xFFFF;
        outStream << lcl_buffer;
        outStreamEos << 0;
        lcl_buffer >>= OUT_DWIDTH;
        size += 4;

        // CRC Processing
        ap_uint<32> checksum = checksumStream.read();
        lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = checksum;
        outStream << lcl_buffer;
        outStreamEos << 0;
        checksum >>= OUT_DWIDTH;
        lcl_buffer >>= OUT_DWIDTH;
        lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = checksum;
        outStream << lcl_buffer;
        outStreamEos << 0;
        lcl_buffer >>= OUT_DWIDTH;
        size += 4;

        // Input Size for Gzip
        if (STRATEGY == 0) {
            lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = inSize;
            outStream << lcl_buffer;
            outStreamEos << 0;
            inSize >>= OUT_DWIDTH;
            lcl_buffer >>= OUT_DWIDTH;
            lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = inSize;
            outStream << lcl_buffer;
            outStreamEos << 0;
            lcl_buffer >>= OUT_DWIDTH;
            size += 4;
        }

        // Leftover Bytes
        lcl_buffer.range(prodLbuf + OUT_DWIDTH - 1, prodLbuf) = 0;
        outStream << lcl_buffer;
        lcl_buffer >>= OUT_DWIDTH;
        if (lbuf_idx) {
            outStream << lcl_buffer;
            outStreamEos << 0;
        }
        outStreamEos << 1;

        outputTotalSizeStream << size;
    }
    inFileEos.read();
    outNumBlockStreamEos << 1;
}

template <class SIZE_DT = uint64_t,
          class BLOCKSIZE_DT = uint32_t,
          int DWIDTH = 64,
          int NUM_BLOCKS = 4,
          int BLOCK_SIZE = 32768>
void multicoreDistributor(hls::stream<ap_uint<DWIDTH> >& inStream,
                          hls::stream<SIZE_DT>& inSizeStream,
                          hls::stream<ap_uint<DWIDTH> > distStream[NUM_BLOCKS],
                          hls::stream<BLOCKSIZE_DT> distSizeStream[NUM_BLOCKS]) {
    constexpr int incr = DWIDTH / 8;
    ap_uint<4> core = 0;
    SIZE_DT readSize = 0;
    SIZE_DT inputSize = inSizeStream.read();

    while (readSize < inputSize) {
        core %= NUM_BLOCKS;
        BLOCKSIZE_DT outputSize = ((readSize + BLOCK_SIZE) > inputSize) ? (inputSize - readSize) : BLOCK_SIZE;
        readSize += outputSize;
        distSizeStream[core] << outputSize;
        for (BLOCKSIZE_DT j = 0; j < outputSize; j += incr) {
#pragma HLS PIPELINE II = 1
            ap_uint<DWIDTH> outData = inStream.read();
            distStream[core] << outData;
        }
        core++;
    }

    for (int i = 0; i < NUM_BLOCKS; i++) {
        distSizeStream[i] << 0;
    }
}

template <class SIZE_DT = uint64_t, int DWIDTH = 64, int NUM_BLOCKS = 4, int BLOCK_SIZE = 32768>
void multicoreMerger(hls::stream<ap_uint<DWIDTH> > inStream[NUM_BLOCKS],
                     hls::stream<bool> inStreamEos[NUM_BLOCKS],
                     hls::stream<ap_uint<4> > inSizeStream[NUM_BLOCKS],
                     hls::stream<ap_uint<DWIDTH> >& outStream,
                     hls::stream<bool>& outStreamEos,
                     hls::stream<SIZE_DT>& outSizeStream) {
    constexpr int incr = DWIDTH / 8;
    uint32_t outSize = 0;
    ap_uint<NUM_BLOCKS> is_pending;
    ap_uint<2 * DWIDTH> inputWindow;
    uint32_t factor = DWIDTH / 8;
    uint32_t inputIdx = 0;

    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
        is_pending.range(i, i) = 1;
    }

    while (is_pending) {
        for (int i = 0; i < NUM_BLOCKS; i++) {
            bool blockDone = false;
            for (; (blockDone == false) && (is_pending(i, i) == true);) {
#pragma HLS PIPELINE II = 1
                assert((inputIdx + factor) <= 2 * (DWIDTH / 8));

                uint8_t inSize = inSizeStream[i].read();
                outSize += inSize;
                inputWindow.range((inputIdx + factor) * 8 - 1, inputIdx * 8) = inStream[i].read();
                bool eosFlag = inStreamEos[i].read();

                ap_uint<DWIDTH> outData = inputWindow.range(DWIDTH - 1, 0);
                inputIdx += inSize;

                blockDone = (inSize) ? false : true;

                if (inputIdx >= factor) {
                    outStream << outData;
                    outStreamEos << 0;
                    inputWindow >>= DWIDTH;
                    inputIdx -= factor;
                }

                // checks per engine end of data or not
                is_pending.range(i, i) = (eosFlag) ? false : true;
            }
        }
    }

    if (inputIdx) {
        outStream << inputWindow.range(DWIDTH - 1, 0);
        outStreamEos << 0;
    }

    // send end of stream data
    outStream << 0;
    outStreamEos << 1;
    outSizeStream << outSize;
}

template <int NUM_BLOCK = 4>
void zlibTreegenScheduler(hls::stream<uint32_t> lz77InTree[NUM_BLOCK],
                          hls::stream<bool> lz77TreeBlockEos[NUM_BLOCK],
                          hls::stream<uint32_t>& lz77OutTree,
                          hls::stream<bool>& lz77OutTreeEos,
                          hls::stream<uint8_t>& outIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    ap_uint<NUM_BLOCK> is_pending;
    bool eos_tmp[NUM_BLOCK];
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
        is_pending.range(i, i) = 1;
        eos_tmp[i] = false;
    }
    while (is_pending) {
        for (uint8_t i = 0; i < NUM_BLOCK; i++) {
            bool eos = eos_tmp[i] ? eos_tmp[i] : lz77TreeBlockEos[i].read();
            is_pending.range(i, i) = eos ? 0 : 1;
            eos_tmp[i] = eos;
            if (!eos) {
                outIdxNum << i;
                lz77OutTreeEos << 0;
                for (uint16_t j = 0; j < c_litDistCodeCnt; j++) {
                    uint32_t tmpVal = lz77InTree[i].read();
                    lz77OutTree << tmpVal;
                }
            }
        }
    }
    lz77OutTreeEos << 1;
    outIdxNum << 0xFF;
}

template <int NUM_BLOCK = 4>
void zlibTreegenDistributor(hls::stream<uint16_t> StreamCode[NUM_BLOCK],
                            hls::stream<uint8_t> StreamSize[NUM_BLOCK],
                            hls::stream<uint16_t>& streamSerialCode,
                            hls::stream<uint8_t>& streamSerialSize,
                            hls::stream<uint8_t>& inIdxNum) {
    constexpr int c_litDistCodeCnt = 286 + 30;
    for (uint8_t i = inIdxNum.read(); i != 0xFF; i = inIdxNum.read()) {
        for (uint16_t j = 0; j < c_litDistCodeCnt; j++) {
            StreamSize[i] << streamSerialSize.read();
            StreamCode[i] << streamSerialCode.read();
        }
        for (uint16_t inSize = streamSerialSize.read(); inSize != 0; inSize = streamSerialSize.read()) {
            StreamSize[i] << inSize;
            StreamCode[i] << streamSerialCode.read();
        }
        StreamSize[i] << 0;
    }
}

template <int NUM_BLOCK = 4>
void zlibTreegenStreamWrapper(hls::stream<uint32_t> lz77Tree[NUM_BLOCK],
                              hls::stream<bool> lz77TreeBlockEos[NUM_BLOCK],
                              hls::stream<uint16_t> StreamCode[NUM_BLOCK],
                              hls::stream<uint8_t> StreamSize[NUM_BLOCK]) {
#pragma HLS dataflow
    constexpr int c_litDistCodeCnt = 286 + 30;
    constexpr int c_depthlitDistStream = NUM_BLOCK * c_litDistCodeCnt;
    hls::stream<uint32_t> lz77SerialTree("lz77SerialTree");
    hls::stream<bool> lz77SerialTreeEos("lz77SerialTreeEos");
    hls::stream<uint16_t> streamSerialCode("streamSerialCode");
    hls::stream<uint8_t> streamSerialSize("streamSerialSize");
    hls::stream<uint8_t> idxNum("idxNum");
#pragma HLS STREAM variable = lz77SerialTree depth = c_depthlitDistStream
#pragma HLS STREAM variable = streamSerialCode depth = 32
#pragma HLS STREAM variable = streamSerialSize depth = 32
#pragma HLS STREAM variable = idxNum depth = 32

    zlibTreegenScheduler<NUM_BLOCK>(lz77Tree, lz77TreeBlockEos, lz77SerialTree, lz77SerialTreeEos, idxNum);
    zlibTreegenStream(lz77SerialTree, lz77SerialTreeEos, streamSerialCode, streamSerialSize);
    zlibTreegenDistributor<NUM_BLOCK>(StreamCode, StreamSize, streamSerialCode, streamSerialSize, idxNum);
}

} // End namespace details
} // End namespace compression
} // End namespace xf

#endif // _XFCOMPRESSION_ZLIB_COMPRESS_DETAILS_HPP_
