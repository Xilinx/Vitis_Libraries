/*
 * (c) Copyright 2021 Xilinx, Inc. All rights reserved.
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
#ifndef _XFCOMPRESSION_ZLIB_COMPRESS_HPP_
#define _XFCOMPRESSION_ZLIB_COMPRESS_HPP_

/**
 * @file zlib_compress.hpp
 * @brief Header for modules used in ZLIB compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include <ap_int.h>
#include <assert.h>
#include <stdint.h>

#include "compress_utils.hpp"
#include "zlib_compress_details.hpp"
#include "zlib_specs.hpp"
#include "lz_optional.hpp"
#include "lz_compress.hpp"
#include "huffman_treegen.hpp"
#include "huffman_encoder.hpp"
#include "mm2s.hpp"
#include "s2mm.hpp"
#include "stream_downsizer.hpp"
#include "stream_upsizer.hpp"
#include "axi_stream_utils.hpp"
#include "ap_axi_sdata.h"
#include "checksum_wrapper.hpp"

namespace xf {
namespace compression {

template <int CORE_ID = 0,
          int MAX_BLOCK_SIZE = 32 * 1024,
          int MAX_FREQ_DWIDTH = 24,
          int MATCH_LEN = 6,
          int MIN_MATCH = 3,
          int LZ_MAX_OFFSET_LIMIT = 32 * 1024,
          int MAX_MATCH_LEN = 255>
void lz77Compress(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                  hls::stream<IntVectorStream_dt<9, 1> >& lz77OutStream,
                  hls::stream<IntVectorStream_dt<MAX_FREQ_DWIDTH, 1> >& outTreeStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
#pragma HLS STREAM variable = compressedStream depth = 4
#pragma HLS STREAM variable = boosterStream depth = 4

    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT, CORE_ID>(
        inStream, compressedStream);
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    xf::compression::lz77DivideStream<MAX_FREQ_DWIDTH>(boosterStream, lz77OutStream, outTreeStream);
}

template <int CORE_ID = 0,
          int MAX_BLOCK_SIZE = 32 * 1024,
          int MATCH_LEN = 6,
          int MIN_MATCH = 3,
          int LZ_MAX_OFFSET_LIMIT = 32 * 1024,
          int MAX_MATCH_LEN = 255>
void lz77CompressStatic(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                        hls::stream<IntVectorStream_dt<9, 1> >& lz77Out) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > compressedStream("compressedStream");
    hls::stream<IntVectorStream_dt<32, 1> > boosterStream("boosterStream");
#pragma HLS STREAM variable = compressedStream depth = 4
#pragma HLS STREAM variable = boosterStream depth = 4

    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT, CORE_ID>(
        inStream, compressedStream);
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressedStream, boosterStream);
    xf::compression::lz77DivideStatic(boosterStream, lz77Out);
}

void zlibHuffmanEncoder(hls::stream<IntVectorStream_dt<9, 1> >& inStream,
                        hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> >& hufCodeInStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& huffOutStream) {
#pragma HLS dataflow
    hls::stream<IntVectorStream_dt<32, 1> > encodedOutStream("encodedOutStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream("hufCodeStream");
#pragma HLS STREAM variable = encodedOutStream depth = 4
#pragma HLS STREAM variable = hufCodeStream depth = 4

    xf::compression::details::huffmanProcessingUnit(inStream, encodedOutStream);
    xf::compression::huffmanEncoderStream(encodedOutStream, hufCodeInStream, hufCodeStream);
    xf::compression::details::bitPackingStream(hufCodeStream, huffOutStream);
}

void zlibHuffmanEncoderStatic(hls::stream<IntVectorStream_dt<9, 1> >& inStream,
                              hls::stream<IntVectorStream_dt<8, 2> >& huffOut) {
#pragma HLS DATAFLOW
    hls::stream<IntVectorStream_dt<32, 1> > encodedOutStream("encodedOutStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream("hufCodeStream");
#pragma HLS STREAM variable = encodedOutStream depth = 4
#pragma HLS STREAM variable = hufCodeStream depth = 4

    xf::compression::details::huffmanProcessingUnit(inStream, encodedOutStream);
    xf::compression::huffmanEncoderStatic(encodedOutStream, hufCodeStream);
    xf::compression::details::bitPackingStatic(hufCodeStream, huffOut);
}

template <int NUM_BLOCKS = 8, int URAM_BUFFERS = 1>
void gzipMulticoreCompression(hls::stream<ap_uint<64> >& inStream,
                              hls::stream<uint32_t>& inSizeStream,
                              hls::stream<ap_uint<32> >& checksumInitStream,
                              hls::stream<ap_uint<64> >& outStream,
                              hls::stream<bool>& outStreamEos,
                              hls::stream<uint32_t>& outSizeStream,
                              hls::stream<ap_uint<32> >& checksumOutStream,
                              hls::stream<bool>& checksumOutEos,
                              hls::stream<ap_uint<2> >& checksumTypeStream) {
#pragma HLS dataflow
    constexpr int c_blockSizeInKb = 32;
    constexpr int c_blockSize = c_blockSizeInKb * 1024;
    constexpr int c_numBlocks = NUM_BLOCKS;
    constexpr int c_dwidth = 64;
    constexpr int c_maxBlockSize = 32768;
    constexpr int c_minBlockSize = 64;
    constexpr int c_checksumParallelBytes = 8;
    constexpr int c_uramFifoDepth = c_blockSize / 8;
    constexpr int c_freq_dwidth = getDataPortWidth(c_blockSize);
    constexpr int c_size_dwidth = getDataPortWidth(c_checksumParallelBytes);
    constexpr int c_strdBlockDepth = c_minBlockSize / c_checksumParallelBytes;

    // Assertion for Maximum Supported Block Size
    assert(c_blockSize <= c_maxBlockSize);
    // Assertion for Maximum Supported Parallel Cores
    assert(c_numBlocks <= 8);

    hls::stream<ap_uint<c_dwidth> > checksumStream("checksumStream");
    hls::stream<bool> checksumEos("checksumEos");
    hls::stream<ap_uint<32> > checksumSizeStream("checksumSizeStream");
    hls::stream<ap_uint<c_dwidth> > coreStream("coreStream");
    hls::stream<uint32_t> coreSizeStream("coreSizeStream");
    hls::stream<ap_uint<c_dwidth> > distStream[c_numBlocks];
    hls::stream<ap_uint<c_freq_dwidth> > distSizeStream[c_numBlocks];
    hls::stream<ap_uint<c_dwidth> > strdStream;
    hls::stream<ap_uint<16> > strdSizeStream;
    hls::stream<ap_uint<17> > upsizedCntr[c_numBlocks];
    hls::stream<IntVectorStream_dt<8, 1> > downStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<9, 1> > lz77Stream[c_numBlocks];
    hls::stream<IntVectorStream_dt<8, 2> > huffStream[c_numBlocks];
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream[c_numBlocks];
    hls::stream<ap_uint<72> > lz77UpsizedStream[c_numBlocks]; // 72 bits
    hls::stream<ap_uint<9> > lz77PassStream[c_numBlocks];     // 9 bits
    hls::stream<IntVectorStream_dt<9, 1> > lz77DownsizedStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > lz77Tree[c_numBlocks];
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > mergeStream[c_numBlocks];

#pragma HLS STREAM variable = checksumStream depth = 4
#pragma HLS STREAM variable = checksumSizeStream depth = 4
#pragma HLS STREAM variable = checksumEos depth = 4
#pragma HLS STREAM variable = coreStream depth = 4
#pragma HLS STREAM variable = coreSizeStream depth = 4
#pragma HLS STREAM variable = distStream depth = c_uramFifoDepth
#pragma HLS STREAM variable = strdSizeStream depth = 16
#pragma HLS STREAM variable = distSizeStream depth = 8

#pragma HLS STREAM variable = downStream depth = 8
#pragma HLS STREAM variable = strdStream depth = c_strdBlockDepth
#pragma HLS STREAM variable = huffStream depth = 8

#pragma HLS STREAM variable = lz77Tree depth = 8
#pragma HLS STREAM variable = hufCodeStream depth = 8
#pragma HLS STREAM variable = upsizedCntr depth = 8
#pragma HLS STREAM variable = lz77DownsizedStream depth = 8
#pragma HLS STREAM variable = lz77Stream depth = 8

#pragma HLS STREAM variable = mergeStream depth = c_uramFifoDepth

#pragma HLS BIND_STORAGE variable = distStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = mergeStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = lz77Tree type = FIFO impl = SRL

    if (URAM_BUFFERS == 1) {
#pragma HLS STREAM variable = lz77UpsizedStream depth = c_uramFifoDepth
#pragma HLS BIND_STORAGE variable = lz77UpsizedStream type = FIFO impl = URAM
    } else {
#pragma HLS STREAM variable = lz77PassStream depth = c_maxBlockSize
#pragma HLS BIND_STORAGE variable = lz77PassStream type = FIFO impl = BRAM
    }

    // send input data to both checksum and for compression
    xf::compression::details::dataDuplicator<c_dwidth, c_checksumParallelBytes>(
        inStream, inSizeStream, checksumStream, checksumEos, checksumSizeStream, coreStream, coreSizeStream);

    // checksum kernel
    xf::compression::checksum32<c_checksumParallelBytes>(checksumInitStream, checksumStream, checksumSizeStream,
                                                         checksumEos, checksumOutStream, checksumOutEos,
                                                         checksumTypeStream);

    // distrubute block size data into each block in round-robin fashion
    details::multicoreDistributor<c_freq_dwidth, c_dwidth, c_numBlocks, c_blockSize>(
        coreStream, coreSizeStream, strdStream, strdSizeStream, distStream, distSizeStream);

    // Parallel Buffers
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        xf::compression::details::streamDownSizerSize<c_dwidth, 8, c_freq_dwidth>(distStream[i], distSizeStream[i],
                                                                                  downStream[i]);
    }

    // Parallel LZ77 Compress
    lz77Compress<0, c_blockSize, c_freq_dwidth>(downStream[0], lz77Stream[0], lz77Tree[0]);
    if (NUM_BLOCKS >= 2) {
        lz77Compress<1, c_blockSize, c_freq_dwidth>(downStream[1], lz77Stream[1], lz77Tree[1]);
    }
    if (NUM_BLOCKS >= 4) {
        lz77Compress<2, c_blockSize, c_freq_dwidth>(downStream[2], lz77Stream[2], lz77Tree[2]);
        lz77Compress<3, c_blockSize, c_freq_dwidth>(downStream[3], lz77Stream[3], lz77Tree[3]);
    }
    if (NUM_BLOCKS >= 8) {
        lz77Compress<4, c_blockSize, c_freq_dwidth>(downStream[4], lz77Stream[4], lz77Tree[4]);
        lz77Compress<5, c_blockSize, c_freq_dwidth>(downStream[5], lz77Stream[5], lz77Tree[5]);
        lz77Compress<6, c_blockSize, c_freq_dwidth>(downStream[6], lz77Stream[6], lz77Tree[6]);
        lz77Compress<7, c_blockSize, c_freq_dwidth>(downStream[7], lz77Stream[7], lz77Tree[7]);
    }

    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        if (URAM_BUFFERS == 1) {
            xf::compression::details::bufferUpsizer<9, 72>(lz77Stream[i], lz77UpsizedStream[i], upsizedCntr[i]);
        } else {
            xf::compression::details::sendBuffer<9>(lz77Stream[i], lz77PassStream[i], upsizedCntr[i]);
        }
    }

    // Single Call Treegen
    details::zlibTreegenStreamWrapper<c_numBlocks>(lz77Tree, hufCodeStream);

    // Parallel Huffman
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        if (URAM_BUFFERS == 1) {
            xf::compression::details::bufferDownsizer<72, 9>(lz77UpsizedStream[i], lz77DownsizedStream[i],
                                                             upsizedCntr[i]);
        } else {
            xf::compression::details::receiveBuffer<9>(lz77PassStream[i], lz77DownsizedStream[i], upsizedCntr[i]);
        }

        zlibHuffmanEncoder(lz77DownsizedStream[i], hufCodeStream[i], huffStream[i]);

        xf::compression::details::simpleStreamUpsizer<16, c_dwidth, c_size_dwidth>(huffStream[i], mergeStream[i]);
    }

    // read all num block data in round-robin fashion and write into single outstream
    details::multicoreMerger<c_dwidth, c_size_dwidth, c_numBlocks, c_blockSize>(mergeStream, strdStream, strdSizeStream,
                                                                                outStream, outStreamEos, outSizeStream);
}

template <int NUM_BLOCKS = 8, int STRATEGY = 0> // STRATEGY -> 0: GZIP; 1: ZLIB
void gzipMulticoreStaticCompressStream(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                                       hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
#pragma HLS dataflow
    // Constants
    constexpr int c_blockSizeInKb = 32;
    constexpr int c_blockSize = c_blockSizeInKb * 1024;
    constexpr int c_numBlocks = NUM_BLOCKS;
    constexpr int c_dwidth = 64;
    constexpr int c_maxBlockSize = 32768;
    constexpr int c_minBlockSize = 64;
    constexpr int c_checksumParallelBytes = 8;
    constexpr int c_uramFifoDepth = c_blockSize / 8;
    constexpr int c_freq_dwidth = getDataPortWidth(c_blockSize);
    constexpr int c_size_dwidth = getDataPortWidth(c_checksumParallelBytes);
    constexpr int c_strdBlockDepth = c_minBlockSize / c_checksumParallelBytes;

    // Assertion for Maximum Supported Block Size
    assert(c_blockSize <= c_maxBlockSize);
    // Assertion for Maximum Supported Parallel Cores
    assert(c_numBlocks <= 8);

    // Internal Streams
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > coreStream("coreStream");
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > packedStream("packedStream");
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > distStream[c_numBlocks];
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > mergeStream[c_numBlocks];
    hls::stream<ap_uint<c_dwidth> > checksumStream("checksumStream");
    hls::stream<ap_uint<c_dwidth> > strdStream("strdStream");

    hls::stream<IntVectorStream_dt<8, 1> > downStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<9, 1> > lz77Stream[c_numBlocks];
    hls::stream<IntVectorStream_dt<8, 2> > huffStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > lz77Tree[c_numBlocks];

    hls::stream<ap_uint<4> > coreIdxStream("coreIdxStream");
    hls::stream<ap_uint<5> > checksumSizeStream("checksumSizeStream");
    hls::stream<ap_uint<16> > strdSizeStream("strdSizeStream");
    hls::stream<ap_uint<17> > upsizedCntr[c_numBlocks];
    hls::stream<ap_uint<32> > checksumInitStream("checksumInitStream");
    hls::stream<ap_uint<32> > checksumOutStream("checksumOutStream");

    hls::stream<uint32_t> fileSizeStream("fileSizeStream");

    hls::stream<bool> blckEosStream("blckEosStream");

#pragma HLS STREAM variable = checksumStream depth = 4
#pragma HLS STREAM variable = checksumSizeStream depth = 4
#pragma HLS STREAM variable = coreStream depth = 4
#pragma HLS STREAM variable = packedStream depth = 4
#pragma HLS STREAM variable = strdSizeStream depth = 16
#pragma HLS STREAM variable = blckEosStream depth = 72
#pragma HLS STREAM variable = upsizedCntr depth = 8
#pragma HLS STREAM variable = fileSizeStream depth = 16
#pragma HLS STREAM variable = coreIdxStream depth = 16
#pragma HLS STREAM variable = downStream depth = 8
#pragma HLS STREAM variable = huffStream depth = 8
#pragma HLS STREAM variable = lz77Stream depth = 8
#pragma HLS STREAM variable = lz77Tree depth = 8
#pragma HLS STREAM variable = checksumInitStream depth = 4
#pragma HLS STREAM variable = checksumOutStream depth = 16
#pragma HLS STREAM variable = mergeStream depth = c_uramFifoDepth
#pragma HLS STREAM variable = distStream depth = c_uramFifoDepth
#pragma HLS STREAM variable = strdStream depth = c_strdBlockDepth

#pragma HLS BIND_STORAGE variable = distStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = mergeStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = lz77Tree type = FIFO impl = SRL

    // send input data to both checksum and for compression
    xf::compression::details::dataDuplicator<c_dwidth, c_size_dwidth, c_checksumParallelBytes, STRATEGY>(
        inStream, checksumInitStream, checksumStream, checksumSizeStream, coreStream);

    // checksum size less kernel
    if (STRATEGY == 0) { // CRC-32
        xf::compression::details::crc32<c_checksumParallelBytes>(checksumInitStream, checksumStream, checksumSizeStream,
                                                                 checksumOutStream);
    } else { // Adler-32
        xf::compression::details::adler32<c_checksumParallelBytes>(checksumInitStream, checksumStream,
                                                                   checksumSizeStream, checksumOutStream);
    }

    // distribute block size data into each block in round-robin fashion
    details::multicoreDistributor<uint32_t, c_dwidth, c_size_dwidth, c_numBlocks, c_blockSize, c_minBlockSize,
                                  STRATEGY>(coreStream, fileSizeStream, strdStream, strdSizeStream, blckEosStream,
                                            coreIdxStream, distStream);

    // Parallel Buffers
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        xf::compression::details::bufferDownsizer<c_dwidth, 8, c_size_dwidth>(distStream[i], downStream[i]);
    }

    // Parallel LZ77 Compress
    lz77CompressStatic<0, c_blockSize>(downStream[0], lz77Stream[0]);
    if (NUM_BLOCKS >= 2) {
        lz77CompressStatic<1, c_blockSize>(downStream[1], lz77Stream[1]);
    }
    if (NUM_BLOCKS >= 4) {
        lz77CompressStatic<2, c_blockSize>(downStream[2], lz77Stream[2]);
        lz77CompressStatic<3, c_blockSize>(downStream[3], lz77Stream[3]);
    }
    if (NUM_BLOCKS >= 8) {
        lz77CompressStatic<4, c_blockSize>(downStream[4], lz77Stream[4]);
        lz77CompressStatic<5, c_blockSize>(downStream[5], lz77Stream[5]);
        lz77CompressStatic<6, c_blockSize>(downStream[6], lz77Stream[6]);
        lz77CompressStatic<7, c_blockSize>(downStream[7], lz77Stream[7]);
    }

    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        zlibHuffmanEncoderStatic(lz77Stream[i], huffStream[i]);

        xf::compression::details::simpleStreamUpsizer<16, c_dwidth, c_size_dwidth>(huffStream[i], mergeStream[i]);
    }

    // GZIP/ZLIB Data Packer
    xf::compression::details::gzipZlibPackerEngine<c_numBlocks, STRATEGY>(
        mergeStream, packedStream, strdStream, strdSizeStream, fileSizeStream, checksumOutStream, coreIdxStream,
        blckEosStream);

    // Byte Alignment and Packing into a Single Stream
    xf::compression::details::bytePacker<c_dwidth, c_size_dwidth>(packedStream, outStream);
}

template <int NUM_BLOCKS = 8, int STRATEGY = 0, int URAM_BUFFERS = 1> // STRATEGY -> 0: GZIP; 1: ZLIB
void gzipMulticoreCompressStream(hls::stream<IntVectorStream_dt<8, 8> >& inStream,
                                 hls::stream<IntVectorStream_dt<8, 8> >& outStream) {
#pragma HLS dataflow
    // Constants
    constexpr int c_blockSizeInKb = 32;
    constexpr int c_blockSize = c_blockSizeInKb * 1024;
    constexpr int c_numBlocks = NUM_BLOCKS;
    constexpr int c_dwidth = 64;
    constexpr int c_maxBlockSize = 32768;
    constexpr int c_minBlockSize = 64;
    constexpr int c_checksumParallelBytes = 8;
    constexpr int c_uramFifoDepth = c_maxBlockSize / 8;
    constexpr int c_freq_dwidth = getDataPortWidth(c_blockSize);
    constexpr int c_size_dwidth = getDataPortWidth(c_checksumParallelBytes);
    constexpr int c_strdBlockDepth = c_minBlockSize / c_checksumParallelBytes;

    // Assertion for Maximum Supported Block Size
    assert(c_blockSize <= c_maxBlockSize);
    // Assertion for Maximum Supported Parallel Cores
    assert(c_numBlocks <= 8);

    // Internal Streams
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > coreStream("coreStream");
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > packedStream("packedStream");
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > distStream[c_numBlocks];
    hls::stream<ap_uint<c_dwidth + c_size_dwidth> > mergeStream[c_numBlocks];
    hls::stream<ap_uint<c_dwidth> > checksumStream("checksumStream");
    hls::stream<ap_uint<c_dwidth> > strdStream("strdStream");

    hls::stream<IntVectorStream_dt<8, 1> > downStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<9, 1> > lz77Stream[c_numBlocks];
    hls::stream<IntVectorStream_dt<8, 2> > huffStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<9, 1> > lz77DownsizedStream[c_numBlocks];
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > lz77Tree[c_numBlocks];
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<c_maxBits>, 1> > hufCodeStream[c_numBlocks];

    hls::stream<ap_uint<5> > checksumSizeStream("checksumSizeStream");
    hls::stream<ap_uint<16> > strdSizeStream("strdSizeStream");
    hls::stream<ap_uint<17> > upsizedCntr[c_numBlocks];
    hls::stream<ap_uint<32> > checksumInitStream("checksumInitStream");
    hls::stream<ap_uint<32> > checksumOutStream("checksumOutStream");
    hls::stream<ap_uint<72> > lz77UpsizedStream[c_numBlocks]; // 72 bits
    hls::stream<ap_uint<9> > lz77PassStream[c_numBlocks];     // 9 bits
    hls::stream<ap_uint<4> > coreIdxStream("coreIdxStream");
    hls::stream<ap_uint<4> > coreIdxStreamArr[2];

    hls::stream<uint32_t> fileSizeStream("fileSizeStream");

    hls::stream<bool> blckEosStream("blckEosStream");

#pragma HLS STREAM variable = checksumStream depth = 4
#pragma HLS STREAM variable = checksumSizeStream depth = 4
#pragma HLS STREAM variable = coreStream depth = 4
#pragma HLS STREAM variable = packedStream depth = 4
#pragma HLS STREAM variable = strdSizeStream depth = 16
#pragma HLS STREAM variable = blckEosStream depth = 72
#pragma HLS STREAM variable = fileSizeStream depth = 16
#pragma HLS STREAM variable = coreIdxStream depth = 4
#pragma HLS STREAM variable = coreIdxStreamArr depth = 16
#pragma HLS STREAM variable = downStream depth = 8
#pragma HLS STREAM variable = huffStream depth = 8
#pragma HLS STREAM variable = lz77Tree depth = 8
#pragma HLS STREAM variable = hufCodeStream depth = 8
#pragma HLS STREAM variable = checksumInitStream depth = 4
#pragma HLS STREAM variable = checksumOutStream depth = 16
#pragma HLS STREAM variable = mergeStream depth = c_uramFifoDepth
#pragma HLS STREAM variable = distStream depth = c_uramFifoDepth
#pragma HLS STREAM variable = strdStream depth = c_strdBlockDepth
#pragma HLS STREAM variable = upsizedCntr depth = 8
#pragma HLS STREAM variable = lz77DownsizedStream depth = 8
#pragma HLS STREAM variable = lz77Stream depth = 8

#pragma HLS BIND_STORAGE variable = distStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = mergeStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = lz77Tree type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = strdStream type = FIFO impl = SRL

    if (URAM_BUFFERS == 1) {
#pragma HLS STREAM variable = lz77UpsizedStream depth = c_uramFifoDepth
#pragma HLS BIND_STORAGE variable = lz77UpsizedStream type = FIFO impl = URAM
    } else {
#pragma HLS STREAM variable = lz77PassStream depth = c_maxBlockSize
#pragma HLS BIND_STORAGE variable = lz77PassStream type = FIFO impl = BRAM
    }

    // send input data to both checksum and for compression
    xf::compression::details::dataDuplicator<c_dwidth, c_size_dwidth, c_checksumParallelBytes, STRATEGY>(
        inStream, checksumInitStream, checksumStream, checksumSizeStream, coreStream);

    // checksum size less kernel
    if (STRATEGY == 0) { // CRC-32
        xf::compression::details::crc32<c_checksumParallelBytes>(checksumInitStream, checksumStream, checksumSizeStream,
                                                                 checksumOutStream);
    } else { // Adler-32
        xf::compression::details::adler32<c_checksumParallelBytes>(checksumInitStream, checksumStream,
                                                                   checksumSizeStream, checksumOutStream);
    }

    // distribute block size data into each block in round-robin fashion
    details::multicoreDistributor<uint32_t, c_dwidth, c_size_dwidth, c_numBlocks, c_blockSize, c_minBlockSize,
                                  STRATEGY>(coreStream, fileSizeStream, strdStream, strdSizeStream, blckEosStream,
                                            coreIdxStream, distStream);

    // Stream Duplicator
    details::streamDuplicator<4, 2>(coreIdxStream, coreIdxStreamArr);

    // Parallel Buffers
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        xf::compression::details::bufferDownsizer<c_dwidth, 8, c_size_dwidth>(distStream[i], downStream[i]);
    }

    // Parallel LZ77 Compress
    lz77Compress<0, c_blockSize, c_freq_dwidth>(downStream[0], lz77Stream[0], lz77Tree[0]);
    if (NUM_BLOCKS >= 2) {
        lz77Compress<1, c_blockSize, c_freq_dwidth>(downStream[1], lz77Stream[1], lz77Tree[1]);
    }
    if (NUM_BLOCKS >= 4) {
        lz77Compress<2, c_blockSize, c_freq_dwidth>(downStream[2], lz77Stream[2], lz77Tree[2]);
        lz77Compress<3, c_blockSize, c_freq_dwidth>(downStream[3], lz77Stream[3], lz77Tree[3]);
    }
    if (NUM_BLOCKS >= 8) {
        lz77Compress<4, c_blockSize, c_freq_dwidth>(downStream[4], lz77Stream[4], lz77Tree[4]);
        lz77Compress<5, c_blockSize, c_freq_dwidth>(downStream[5], lz77Stream[5], lz77Tree[5]);
        lz77Compress<6, c_blockSize, c_freq_dwidth>(downStream[6], lz77Stream[6], lz77Tree[6]);
        lz77Compress<7, c_blockSize, c_freq_dwidth>(downStream[7], lz77Stream[7], lz77Tree[7]);
    }

    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        if (URAM_BUFFERS == 1) {
            xf::compression::details::bufferUpsizer<9, 72>(lz77Stream[i], lz77UpsizedStream[i], upsizedCntr[i]);
        } else {
            xf::compression::details::sendBuffer<9>(lz77Stream[i], lz77PassStream[i], upsizedCntr[i]);
        }
    }

    // Single Call Treegen
    details::zlibTreegenStreamWrapper<c_numBlocks>(lz77Tree, hufCodeStream, coreIdxStreamArr[0]);

    // Parallel Huffman
    for (uint8_t i = 0; i < c_numBlocks; i++) {
#pragma HLS UNROLL
        if (URAM_BUFFERS == 1) {
            xf::compression::details::bufferDownsizer<72, 9>(lz77UpsizedStream[i], lz77DownsizedStream[i],
                                                             upsizedCntr[i]);
        } else {
            xf::compression::details::receiveBuffer<9>(lz77PassStream[i], lz77DownsizedStream[i], upsizedCntr[i]);
        }

        zlibHuffmanEncoder(lz77DownsizedStream[i], hufCodeStream[i], huffStream[i]);

        xf::compression::details::simpleStreamUpsizer<16, c_dwidth, c_size_dwidth>(huffStream[i], mergeStream[i]);
    }

    // GZIP/ZLIB Data Packer
    xf::compression::details::gzipZlibPackerEngine<c_numBlocks, STRATEGY>(
        mergeStream, packedStream, strdStream, strdSizeStream, fileSizeStream, checksumOutStream, coreIdxStreamArr[1],
        blckEosStream);

    // Byte Alignment and Packing into a Single Stream
    xf::compression::details::bytePacker<c_dwidth, c_size_dwidth>(packedStream, outStream);
}

template <int NUM_BLOCKS = 8, int STRATEGY = 0, int URAM_BUFFERS = 1> // STRATEGY -> 0: GZIP; 1: ZLIB
void gzipMulticoreCompressAxiStream(hls::stream<ap_axiu<64, 0, 0, 0> >& inAxiStream,
                                    hls::stream<ap_axiu<64, 0, 0, 0> >& outAxiStream) {
    constexpr int c_dwidth = 64;
    hls::stream<IntVectorStream_dt<8, 8> > inStream("inStream");
    hls::stream<IntVectorStream_dt<8, 8> > outStream("outStream");

#pragma HLS STREAM variable = inStream depth = 4
#pragma HLS STREAM variable = outStream depth = 4

#pragma HLS BIND_STORAGE variable = inStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = outStream type = FIFO impl = SRL

#pragma HLS dataflow
    xf::compression::details::axiu2hlsStream<c_dwidth>(inAxiStream, inStream);

#ifdef STATIC_MODE
    xf::compression::gzipMulticoreStaticCompressStream<NUM_BLOCKS, STRATEGY>(inStream, outStream);
#else
    xf::compression::gzipMulticoreCompressStream<NUM_BLOCKS, STRATEGY, URAM_BUFFERS>(inStream, outStream);
#endif

    xf::compression::details::hlsStream2axiu<c_dwidth>(outStream, outAxiStream);
}

} // End namespace compression
} // End namespace xf

#endif // _XFCOMPRESSION_ZLIB_COMPRESS_HPP_
