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
#ifndef _XFCOMPRESSION_ZLIB_COMPRESS_HPP_
#define _XFCOMPRESSION_ZLIB_COMPRESS_HPP_

/**
 * @file zlib_compress.hpp
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

namespace xf {
namespace compression {

template <int MAX_BLOCK_SIZE = 64 * 1024,
          int MATCH_LEN = 6,
          int MIN_MATCH = 3,
          int LZ_MAX_OFFSET_LIMIT = 32 * 1024,
          int MAX_MATCH_LEN = 255>
void lz77Compress(hls::stream<ap_uint<8> >& inStream,
                  hls::stream<ap_uint<9> >& lz77Out,
                  hls::stream<bool>& lz77OutEos,
                  hls::stream<uint32_t>& outStreamTree,
                  hls::stream<bool>& outStreamTreeBlockEos,
                  hls::stream<uint32_t>& byteCompressedSize,
                  hls::stream<uint32_t>& compressedSize,
                  hls::stream<uint32_t>& sizeStream) {
#pragma HLS dataflow
    constexpr int c_slaves = 3;
    hls::stream<ap_uint<32> > compressdStream("compressdStream");
    hls::stream<ap_uint<32> > boosterStream("boosterStream");
    hls::stream<uint32_t> outSize[c_slaves];
#pragma HLS STREAM variable = compressdStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16
#pragma HLS STREAM variable = outSize depth = 16

    xf::compression::details::streamSizeDistributor<c_slaves>(sizeStream, outSize);
    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, outSize[0]);
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressdStream, boosterStream, outSize[1]);
    xf::compression::lz77DivideStream(boosterStream, lz77Out, lz77OutEos, outStreamTree, outStreamTreeBlockEos,
                                      byteCompressedSize, compressedSize, outSize[2]);
}

template <int MAX_BLOCK_SIZE = 64 * 1024,
          int MATCH_LEN = 6,
          int MIN_MATCH = 3,
          int LZ_MAX_OFFSET_LIMIT = 32 * 1024,
          int MAX_MATCH_LEN = 255>
void lz77CompressStatic(hls::stream<ap_uint<8> >& inStream,
                        hls::stream<ap_uint<9> >& lz77Out,
                        hls::stream<bool>& lz77Eos,
                        hls::stream<uint32_t>& sizeStream) {
#pragma HLS dataflow
    constexpr int c_slaves = 3;
    hls::stream<ap_uint<32> > compressdStream("compressdStream");
    hls::stream<ap_uint<32> > boosterStream("boosterStream");
    hls::stream<uint32_t> outSize[c_slaves];
#pragma HLS STREAM variable = compressdStream depth = 16
#pragma HLS STREAM variable = boosterStream depth = 16

    xf::compression::details::streamSizeDistributor<c_slaves>(sizeStream, outSize);
    xf::compression::lzCompress<MAX_BLOCK_SIZE, uint32_t, MATCH_LEN, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, outSize[0]);
    xf::compression::lzBooster<MAX_MATCH_LEN>(compressdStream, boosterStream, outSize[1]);
    xf::compression::lz77DivideStatic(boosterStream, lz77Out, lz77Eos, outSize[2]);
}

void zlibHuffmanEncoder(hls::stream<ap_uint<9> >& inStream,
                        hls::stream<ap_uint<16> >& huffOut,
                        hls::stream<bool>& huffOutEos,
                        hls::stream<bool>& huffFileEos,
                        hls::stream<uint32_t>& huffOutSize,
                        hls::stream<uint32_t>& inputSize,
                        hls::stream<uint32_t>& encodedSize,
                        hls::stream<uint16_t>& StreamCode,
                        hls::stream<uint8_t>& StreamSize) {
#pragma HLS dataflow
    constexpr int c_slaves = 2;
    hls::stream<uint32_t> inSize[c_slaves];
    hls::stream<ap_uint<32> > encodedOutStream("encodedOutStream");
    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
#pragma HLS STREAM variable = encodedOutStream depth = 32
#pragma HLS STREAM variable = bitVals depth = 32
#pragma HLS STREAM variable = bitLen depth = 32
#pragma HLS STREAM variable = inSize depth = 32

    xf::compression::details::streamSizeDistributor<c_slaves>(encodedSize, inSize);
    xf::compression::details::huffmanProcessingUnit(inStream, encodedOutStream, inputSize);
    xf::compression::huffmanEncoderStream(encodedOutStream, bitVals, bitLen, inSize[0], StreamCode, StreamSize);

    xf::compression::details::bitPackingStream(bitVals, bitLen, huffOut, huffOutEos, huffFileEos, huffOutSize,
                                               inSize[1]);
}

void zlibHuffmanEncoderStatic(hls::stream<ap_uint<9> >& inStream,
                              hls::stream<ap_uint<16> >& huffOut,
                              hls::stream<bool>& huffOutEos,
                              hls::stream<bool>& outFileEos,
                              hls::stream<uint32_t>& huffOutSize,
                              hls::stream<bool>& inEos) {
#pragma HLS dataflow
    hls::stream<ap_uint<32> > encodedOutStream("encodedOutStream");
    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
    hls::stream<uint32_t> inSizeV("huffInSize");
    hls::stream<bool> endOfBlock("endOfBlock");
    hls::stream<bool> endOfStream[3];
#pragma HLS STREAM variable = encodedOutStream depth = 32
#pragma HLS STREAM variable = bitVals depth = 32
#pragma HLS STREAM variable = bitLen depth = 32

    xf::compression::details::streamDistributor<3>(inEos, endOfStream);
    xf::compression::details::huffmanProcessingUnitStatic(inStream, encodedOutStream, endOfStream[0], endOfBlock);
    xf::compression::huffmanEncoderStatic(encodedOutStream, bitVals, bitLen, endOfBlock, endOfStream[1]);
    xf::compression::details::bitPackingStatic(bitVals, bitLen, huffOut, huffOutEos, outFileEos, huffOutSize,
                                               endOfStream[2]);
}

template <int MAX_BLOCK_SIZE = 64 * 1024>
void zlibCompressStatic(hls::stream<ap_uint<8> >& inStream,
                        hls::stream<ap_uint<16> >& outStream,
                        hls::stream<bool>& outStreamEos,
                        hls::stream<bool>& outFileEos,
                        hls::stream<uint32_t>& outSizeStream,
                        hls::stream<uint32_t>& inSize) {
#pragma HLS DATAFLOW
    // internal streams
    constexpr int depthBlockSizeInBytes = 2 * MAX_BLOCK_SIZE;
    hls::stream<ap_uint<9> > lz77Stream("lz77Stream");
    hls::stream<uint32_t> lz77CompressedSize("lz77CompressedSize");
    hls::stream<bool> lz77Eos("lz77Eos");

    hls::stream<uint16_t> StreamCode("maxCodeStream");
    hls::stream<uint8_t> StreamSize("maxCodeSize");

#pragma HLS STREAM variable = lz77Stream depth = 32
#pragma HLS STREAM variable = lz77Eos depth = 32
#pragma HLS STREAM variable = StreamCode depth = 32
#pragma HLS STREAM variable = StreamSize depth = 32

    // call lz77 compression
    lz77CompressStatic<MAX_BLOCK_SIZE>(inStream, lz77Stream, lz77Eos, inSize);

    // call huffman encoder
    zlibHuffmanEncoderStatic(lz77Stream, outStream, outStreamEos, outFileEos, outSizeStream, lz77Eos);
}

template <int MAX_BLOCK_SIZE = 64 * 1024>
void zlibCompress(hls::stream<ap_uint<8> >& inStream,
                  hls::stream<ap_uint<16> >& outStream,
                  hls::stream<bool>& outStreamEos,
                  hls::stream<bool>& outFileEos,
                  hls::stream<uint32_t>& outSizeStream,
                  hls::stream<uint32_t>& inSize) {
#pragma HLS DATAFLOW
    // internal streams
    constexpr int depthBlockSizeInBytes = 2 * MAX_BLOCK_SIZE / 8;
    hls::stream<ap_uint<9> > lz77Stream("lz77Stream");
    hls::stream<ap_uint<72> > lz77UpsizedStream("lz77UpsizedStream");
    hls::stream<uint32_t> lz77UpsizedStreamSize("lz77UpsizedStreamSize");
    hls::stream<ap_uint<9> > lz77DownsizedStream("lz77DownsizedStream");
    hls::stream<bool> lz77StreamEos("lz77StreamEos");
    hls::stream<uint32_t> lz77Tree("lz77Tree");
    hls::stream<bool> lz77TreeBlockEos("lz77TreeBlockEos");
    hls::stream<bool> lz77TreeBlockEosV("lz77TreeBlockEosV");
    hls::stream<uint32_t> lz77CompressedSize("lz77CompressedSize");
    hls::stream<uint32_t> lz77ByteCompressedSize("lz77ByteCompressedSize");

    hls::stream<uint16_t> StreamCode("maxCodeStream");
    hls::stream<uint8_t> StreamSize("maxCodeSize");

#pragma HLS STREAM variable = lz77Stream depth = 32
#pragma HLS STREAM variable = lz77StreamEos depth = 32
#pragma HLS STREAM variable = lz77UpsizedStream depth = depthBlockSizeInBytes
#pragma HLS STREAM variable = lz77UpsizedStreamSize depth = 32
#pragma HLS STREAM variable = lz77TreeBlockEos depth = 32
#pragma HLS STREAM variable = lz77TreeBlockEosV depth = 32
#pragma HLS STREAM variable = lz77Stream depth = 32
#pragma HLS STREAM variable = lz77Tree depth = 512
#pragma HLS STREAM variable = lz77CompressedSize depth = 32
#pragma HLS STREAM variable = lz77ByteCompressedSize depth = 32
#pragma HLS STREAM variable = StreamCode depth = 32
#pragma HLS STREAM variable = StreamSize depth = 32

#pragma HLS BIND_STORAGE variable = lz77UpsizedStream type = FIFO impl = URAM

    // call lz77 compression
    lz77Compress<MAX_BLOCK_SIZE>(inStream, lz77Stream, lz77StreamEos, lz77Tree, lz77TreeBlockEos,
                                 lz77ByteCompressedSize, lz77CompressedSize, inSize);

    // Pass Upsizer
    xf::compression::details::passUpsizer<9, 72>(lz77Stream, lz77StreamEos, lz77UpsizedStream, lz77UpsizedStreamSize,
                                                 lz77TreeBlockEos, lz77TreeBlockEosV);

    // call treegen
    xf::compression::zlibTreegenStream(lz77Tree, lz77TreeBlockEosV, StreamCode, StreamSize);

    // Pass Downsizer
    xf::compression::details::passDownsizer<72, 9>(lz77UpsizedStream, lz77UpsizedStreamSize, lz77DownsizedStream);

    // call huffman encoder
    zlibHuffmanEncoder(lz77DownsizedStream, outStream, outStreamEos, outFileEos, outSizeStream, lz77ByteCompressedSize,
                       lz77CompressedSize, StreamCode, StreamSize);
}

template <class data_t, int DWIDTH = 512, int BURST_SIZE = 16, int MAX_BLOCK_SIZE = 64 * 1024>
void zlibCompressMM(const data_t* in, data_t* out, uint32_t* compressd_size, uint32_t input_size) {
#pragma HLS DATAFLOW
    hls::stream<ap_uint<8> > inStream("inStream");
    hls::stream<ap_uint<DWIDTH> > inStreamVec("inStreamVec");
    hls::stream<uint32_t> inStreamVecSize("inStreamVecSize");
    hls::stream<uint32_t> inSizeStream("inSizeStream");
    hls::stream<ap_uint<16> > outStreamVec("outStreamVec");
    hls::stream<uint32_t> outSizeStream("outSizeStream");
    hls::stream<bool> outStreamVecEos("outStreamVecEos");
    hls::stream<bool> outFileEos("outFileEos");
    hls::stream<bool> outFileEosV("outFileEosV");
    hls::stream<ap_uint<DWIDTH> > outStream("outStream");
    hls::stream<bool> outStreamEos("outStreamEos");

#pragma HLS STREAM variable = inStream depth = 1024
#pragma HLS STREAM variable = inStreamVec depth = 1024
#pragma HLS STREAM variable = inStreamVecSize depth = 1024
#pragma HLS STREAM variable = inSizeStream depth = 1024
#pragma HLS STREAM variable = outStream depth = 1024
#pragma HLS STREAM variable = outStreamEos depth = 1024
#pragma HLS STREAM variable = outFileEos depth = 1024
#pragma HLS STREAM variable = outFileEosV depth = 1024
#pragma HLS STREAM variable = outStreamVec depth = 1024
#pragma HLS STREAM variable = outStreamVecEos depth = 1024
#pragma HLS STREAM variable = outSizeStream depth = 1024

    // mm2s
    details::mm2sZlib<DWIDTH, BURST_SIZE, MAX_BLOCK_SIZE>(in, inStreamVec, inStreamVecSize, input_size);

    // Stream Downsizer
    details::streamDownSizerZlib<DWIDTH, 8>(inStreamVec, inStreamVecSize, inStream, inSizeStream);

    // ZLIB Compress Stream IO
    zlibCompress<MAX_BLOCK_SIZE>(inStream, outStreamVec, outStreamVecEos, outFileEos, outSizeStream, inSizeStream);

    // Stream Upsizer
    details::upsizerZlib<16, DWIDTH>(outStreamVec, outStreamVecEos, outFileEos, outFileEosV, outStream, outStreamEos);

    // s2mm
    details::s2mmZlib<DWIDTH, BURST_SIZE, MAX_BLOCK_SIZE>(out, outStream, outStreamEos, outFileEosV, outSizeStream,
                                                          compressd_size);
}

template <class OUTSIZE_DT = uint64_t, int NUM_BLOCKS = 4, int DWIDTH = 64, int BLOCK_SIZE = 32768>
void gzipMulticoreCompression(hls::stream<ap_uint<DWIDTH> >& inStream,
                              hls::stream<OUTSIZE_DT>& inSizeStream,
                              hls::stream<ap_uint<DWIDTH> >& outStream,
                              hls::stream<bool>& outStreamEos,
                              hls::stream<OUTSIZE_DT>& outSizeStream) {
#pragma HLS dataflow
    constexpr int blockSizeInBytes = BLOCK_SIZE / 8;
    constexpr int depthBlockSizeInBytes = 2 * BLOCK_SIZE / 8;
    constexpr int depthTreeSize = 320;
    typedef ap_uint<17> BLOCKSIZE_DT;

    assert(BLOCK_SIZE <= 65536);

    hls::stream<ap_uint<DWIDTH> > distStream[NUM_BLOCKS];
    hls::stream<BLOCKSIZE_DT> distSizeStream[NUM_BLOCKS];
    hls::stream<ap_uint<8> > downStream[NUM_BLOCKS];
    hls::stream<uint32_t> downSizeStream[NUM_BLOCKS];
    hls::stream<ap_uint<16> > huffStream[NUM_BLOCKS];
    hls::stream<ap_uint<9> > lz77Stream[NUM_BLOCKS];
    hls::stream<ap_uint<72> > lz77UpsizedStream[NUM_BLOCKS];
    hls::stream<uint32_t> lz77UpsizedStreamSize[NUM_BLOCKS];
    hls::stream<ap_uint<9> > lz77DownsizedStream[NUM_BLOCKS];
    hls::stream<bool> lz77StreamEos[NUM_BLOCKS];
    hls::stream<bool> huffStreamEos[NUM_BLOCKS];
    hls::stream<bool> outFileEos[NUM_BLOCKS];
    hls::stream<bool> lz77TreeBlockEos[NUM_BLOCKS];
    hls::stream<bool> lz77TreeBlockEosV[NUM_BLOCKS];
    hls::stream<uint8_t> StreamSize[NUM_BLOCKS];
    hls::stream<uint16_t> StreamCode[NUM_BLOCKS];
    hls::stream<uint32_t> compressedSize[NUM_BLOCKS];
    hls::stream<uint32_t> lz77Tree[NUM_BLOCKS];
    hls::stream<uint32_t> lz77CompressedSize[NUM_BLOCKS];
    hls::stream<uint32_t> lz77ByteCompressedSize[NUM_BLOCKS];
    hls::stream<ap_uint<DWIDTH> > mergeStream[NUM_BLOCKS];
    hls::stream<bool> mergeStreamEos[NUM_BLOCKS];
    hls::stream<ap_uint<4> > mergeSizeStream[NUM_BLOCKS];

#pragma HLS STREAM variable = distStream depth = blockSizeInBytes
#pragma HLS STREAM variable = distSizeStream depth = 32
#pragma HLS STREAM variable = downStream depth = 32
#pragma HLS STREAM variable = downSizeStream depth = 32
#pragma HLS STREAM variable = huffStream depth = 32
#pragma HLS STREAM variable = huffStreamEos depth = 32
#pragma HLS STREAM variable = lz77Stream depth = 32
#pragma HLS STREAM variable = lz77StreamEos depth = 32
#pragma HLS STREAM variable = lz77UpsizedStream depth = depthBlockSizeInBytes
#pragma HLS STREAM variable = lz77DownsizedStream depth = 32
#pragma HLS STREAM variable = lz77UpsizedStreamSize depth = 32

#pragma HLS STREAM variable = lz77Tree depth = depthTreeSize
#pragma HLS STREAM variable = lz77TreeBlockEos depth = 32
#pragma HLS STREAM variable = lz77TreeBlockEosV depth = 32
#pragma HLS STREAM variable = compressedSize depth = 32

#pragma HLS STREAM variable = StreamCode depth = 32
#pragma HLS STREAM variable = StreamSize depth = 32
#pragma HLS STREAM variable = lz77CompressedSize depth = 32
#pragma HLS STREAM variable = compressedSize depth = 32
#pragma HLS STREAM variable = inSizeStream depth = 32
#pragma HLS STREAM variable = lz77ByteCompressedSize depth = 32
#pragma HLS STREAM variable = mergeStream depth = blockSizeInBytes
#pragma HLS STREAM variable = mergeStreamEos depth = blockSizeInBytes
#pragma HLS STREAM variable = mergeSizeStream depth = blockSizeInBytes

#pragma HLS BIND_STORAGE variable = distStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = mergeStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = mergeStreamEos type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = mergeSizeStream type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lz77Tree type = FIFO impl = SRL
#pragma HLS BIND_STORAGE variable = lz77UpsizedStream type = FIFO impl = URAM

    // distrubute block size data into each block in round-robin fashion
    details::multicoreDistributor<OUTSIZE_DT, BLOCKSIZE_DT, DWIDTH, NUM_BLOCKS, BLOCK_SIZE>(inStream, inSizeStream,
                                                                                            distStream, distSizeStream);

    // Parallel LZ77
    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
        xf::compression::details::streamDownSizerSize<DWIDTH, 8>(distStream[i], distSizeStream[i], downStream[i],
                                                                 downSizeStream[i]);

        lz77Compress(downStream[i], lz77Stream[i], lz77StreamEos[i], lz77Tree[i], lz77TreeBlockEos[i],
                     lz77ByteCompressedSize[i], lz77CompressedSize[i], downSizeStream[i]);

        xf::compression::details::passUpsizer<9, 72>(lz77Stream[i], lz77StreamEos[i], lz77UpsizedStream[i],
                                                     lz77UpsizedStreamSize[i], lz77TreeBlockEos[i],
                                                     lz77TreeBlockEosV[i]);
    }

    // Single Call Treegen
    details::zlibTreegenStreamWrapper<NUM_BLOCKS>(lz77Tree, lz77TreeBlockEosV, StreamCode, StreamSize);

    // Parallel Huffman
    for (uint8_t i = 0; i < NUM_BLOCKS; i++) {
#pragma HLS UNROLL
        xf::compression::details::passDownsizer<72, 9>(lz77UpsizedStream[i], lz77UpsizedStreamSize[i],
                                                       lz77DownsizedStream[i]);

        zlibHuffmanEncoder(lz77DownsizedStream[i], huffStream[i], huffStreamEos[i], outFileEos[i], compressedSize[i],
                           lz77ByteCompressedSize[i], lz77CompressedSize[i], StreamCode[i], StreamSize[i]);

        xf::compression::details::simpleStreamUpsizer<16, DWIDTH>(huffStream[i], huffStreamEos[i], compressedSize[i],
                                                                  outFileEos[i], mergeStream[i], mergeStreamEos[i],
                                                                  mergeSizeStream[i]);
    }

    // read all num block data in round-robin fashion and write into single outstream
    details::multicoreMerger<OUTSIZE_DT, DWIDTH, NUM_BLOCKS, BLOCK_SIZE>(mergeStream, mergeStreamEos, mergeSizeStream,
                                                                         outStream, outStreamEos, outSizeStream);
}

template <int DWIDTH = 512, int NUM_BLOCK = 4, int BURST_SIZE = 16, int BLOCK_SIZE = 32768>
void zlibCompressMultiEngineMM(const ap_uint<DWIDTH>* in,
                               ap_uint<DWIDTH>* out,
                               uint32_t* outBlockSize,
                               uint32_t inputSize) {
#pragma HLS dataflow
    constexpr int depthBlockSizeInBytes = 2 * BLOCK_SIZE / 8;
    constexpr int depthTreeSize = 512 * NUM_BLOCK;

    hls::stream<ap_uint<8> > inStream[NUM_BLOCK];
    hls::stream<ap_uint<16> > outStream[NUM_BLOCK];
    hls::stream<ap_uint<9> > lz77Stream[NUM_BLOCK];
    hls::stream<ap_uint<72> > lz77UpsizedStream[NUM_BLOCK];
    hls::stream<uint32_t> lz77UpsizedStreamSize[NUM_BLOCK];
    hls::stream<ap_uint<9> > lz77DownsizedStream[NUM_BLOCK];
    hls::stream<bool> lz77StreamEos[NUM_BLOCK];
    hls::stream<bool> outStreamEos[NUM_BLOCK];
    hls::stream<bool> outFileEos[NUM_BLOCK];
    hls::stream<bool> lz77TreeBlockEos[NUM_BLOCK];
    hls::stream<bool> lz77TreeBlockEosV[NUM_BLOCK];
    hls::stream<uint8_t> StreamSize[NUM_BLOCK];
    hls::stream<uint16_t> StreamCode[NUM_BLOCK];
    hls::stream<uint32_t> compressedSize[NUM_BLOCK];
    hls::stream<uint32_t> lz77Tree[NUM_BLOCK];
    hls::stream<uint32_t> inSizeStream[NUM_BLOCK];
    hls::stream<uint32_t> lz77CompressedSize[NUM_BLOCK];
    hls::stream<uint32_t> lz77ByteCompressedSize[NUM_BLOCK];
    hls::stream<uint32_t> inBaseIdx[NUM_BLOCK];

#pragma HLS STREAM variable = inStream depth = 2048
#pragma HLS STREAM variable = outStream depth = 32
#pragma HLS STREAM variable = outStreamEos depth = 32
#pragma HLS STREAM variable = lz77Stream depth = 32
#pragma HLS STREAM variable = lz77StreamEos depth = 32
#pragma HLS STREAM variable = lz77UpsizedStream depth = depthBlockSizeInBytes
#pragma HLS STREAM variable = lz77DownsizedStream depth = 32
#pragma HLS STREAM variable = lz77UpsizedStreamSize depth = 32

#pragma HLS STREAM variable = lz77Tree depth = depthTreeSize
#pragma HLS STREAM variable = lz77TreeBlockEos depth = 32
#pragma HLS STREAM variable = lz77TreeBlockEosV depth = 32
#pragma HLS STREAM variable = compressedSize depth = 32

#pragma HLS STREAM variable = StreamCode depth = 32
#pragma HLS STREAM variable = StreamSize depth = 32
#pragma HLS STREAM variable = lz77CompressedSize depth = 32
#pragma HLS STREAM variable = compressedSize depth = 32
#pragma HLS STREAM variable = inSizeStream depth = 32
#pragma HLS STREAM variable = lz77ByteCompressedSize depth = 32

#pragma HLS BIND_STORAGE variable = lz77UpsizedStream type = FIFO impl = URAM

    // MM to multiple streams
    xf::compression::details::mm2multStreamBlockGenerator<8, NUM_BLOCK, BLOCK_SIZE, DWIDTH, BURST_SIZE>(
        in, inStream, inSizeStream, inBaseIdx, inputSize);

    // Parallel LZ77
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        lz77Compress(inStream[i], lz77Stream[i], lz77StreamEos[i], lz77Tree[i], lz77TreeBlockEos[i],
                     lz77ByteCompressedSize[i], lz77CompressedSize[i], inSizeStream[i]);

        xf::compression::details::passUpsizer<9, 72>(lz77Stream[i], lz77StreamEos[i], lz77UpsizedStream[i],
                                                     lz77UpsizedStreamSize[i], lz77TreeBlockEos[i],
                                                     lz77TreeBlockEosV[i]);
    }

    // Single Call Treegen
    details::zlibTreegenStreamWrapper<NUM_BLOCK>(lz77Tree, lz77TreeBlockEosV, StreamCode, StreamSize);

    // Parallel Huffman
    for (uint8_t i = 0; i < NUM_BLOCK; i++) {
#pragma HLS UNROLL
        xf::compression::details::passDownsizer<72, 9>(lz77UpsizedStream[i], lz77UpsizedStreamSize[i],
                                                       lz77DownsizedStream[i]);

        zlibHuffmanEncoder(lz77DownsizedStream[i], outStream[i], outStreamEos[i], outFileEos[i], compressedSize[i],
                           lz77ByteCompressedSize[i], lz77CompressedSize[i], StreamCode[i], StreamSize[i]);
    }

    // Multiple Streams to MM
    xf::compression::details::multStream2MMBlockReceiver<16, NUM_BLOCK, DWIDTH, BURST_SIZE, BLOCK_SIZE>(
        outStream, outStreamEos, outFileEos, compressedSize, inBaseIdx, out, outBlockSize);
}

template <int IN_DWIDTH, int OUT_DWIDTH, int BLOCK_SIZE, int STRATEGY = 0, int MIN_BLCK_SIZE = 1024> // STRATEGY: 0:
// GZIP; 1: ZLIB
void zlibCompressStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                           hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream,
                           hls::stream<ap_axiu<32, 0, 0, 0> >& inSizeStream,
                           hls::stream<ap_axiu<32, 0, 0, 0> >& outSizeStream) {
    constexpr int c_no_slaves = 3;
#pragma HLS DATAFLOW
    hls::stream<ap_uint<IN_DWIDTH> > inHlsStream("inHlsStream");
    hls::stream<bool> inHlsStreamEos("inHlsStreamEos");
    hls::stream<uint32_t> inHlsSizeStream("inHlsSizeStream");
    hls::stream<ap_uint<IN_DWIDTH> > inDataStream1("inDataStream1");
    hls::stream<ap_uint<IN_DWIDTH> > inDataStream2("inDataStream2");
    hls::stream<ap_uint<IN_DWIDTH> > inZlibStream("inZlibStream");
    hls::stream<ap_uint<IN_DWIDTH> > inStoredStream("inStoredStream");
    hls::stream<uint32_t> storedBlckSize("storedBlckSize");
    hls::stream<uint32_t> zlibBlockSizeStream("zlibBlockSizeStream");
    hls::stream<ap_uint<32> > checksumResult("checksumResult");
    hls::stream<ap_uint<OUT_DWIDTH> > outCompressedStream("outCompressedStream");
    hls::stream<bool> outCompressedStreamEos("outCompressedStreamEos");
    hls::stream<bool> outFileEos("outFileEos");
    hls::stream<uint32_t> blockCompressedSizeStream("blockCompressedSizeStream");
    hls::stream<ap_uint<OUT_DWIDTH> > outPackedStream("outPackedStream");
    hls::stream<bool> outPackedStreamEos("outPackedStreamEos");
    hls::stream<uint32_t> totalSizeStream("totalSizeStream");
    hls::stream<bool> numBlockStreamEos("numBlockStreamEos");
    hls::stream<uint32_t> inHlsSizeStreamVec[c_no_slaves];

#pragma HLS STREAM variable = inHlsSizeStream depth = 4
#pragma HLS STREAM variable = inHlsSizeStreamVec depth = 4
#pragma HLS STREAM variable = zlibBlockSizeStream depth = 4
#pragma HLS STREAM variable = blockCompressedSizeStream depth = 4
#pragma HLS STREAM variable = storedBlckSize depth = 4
#pragma HLS STREAM variable = inStoredStream depth = 4
#pragma HLS STREAM variable = totalSizeStream depth = 4
#pragma HLS STREAM variable = inHlsStream depth = 4
#pragma HLS STREAM variable = inHlsStreamEos depth = 4
#pragma HLS STREAM variable = outCompressedStream depth = 4
#pragma HLS STREAM variable = outCompressedStreamEos depth = 4

    // AXI 2 HLS Stream
    xf::compression::details::axiu2hlsStreamSizeEos<IN_DWIDTH>(inStream, inHlsStream, inHlsStreamEos, inHlsSizeStream,
                                                               inSizeStream);

    // Size Distributor
    xf::compression::details::streamSizeDistributor<c_no_slaves>(inHlsSizeStream, inHlsSizeStreamVec);

    // Data Stream Duplicator
    xf::compression::details::streamDuplicator<IN_DWIDTH>(inHlsStream, inHlsStreamEos, inDataStream1, inDataStream2);

    // Zlib/Gzip Block Maker
    xf::compression::details::streamBlockMaker<IN_DWIDTH, BLOCK_SIZE, MIN_BLCK_SIZE>(
        inDataStream1, inZlibStream, inStoredStream, storedBlckSize, zlibBlockSizeStream, inHlsSizeStreamVec[0]);

    // Checksum
    xf::compression::details::checksumWrapper<IN_DWIDTH, STRATEGY>(inDataStream2, inHlsSizeStreamVec[1],
                                                                   checksumResult);

    // Zlib Compress Stream IO Engine
    xf::compression::zlibCompress<BLOCK_SIZE>(inZlibStream, outCompressedStream, outCompressedStreamEos, outFileEos,
                                              blockCompressedSizeStream, zlibBlockSizeStream);

    // Packer
    xf::compression::details::zlibCompressStreamingPacker<IN_DWIDTH, OUT_DWIDTH, BLOCK_SIZE, STRATEGY>(
        inStoredStream, outCompressedStream, outPackedStream, outCompressedStreamEos, outPackedStreamEos,
        storedBlckSize, outFileEos, blockCompressedSizeStream, totalSizeStream, inHlsSizeStreamVec[2], checksumResult,
        numBlockStreamEos);

    // HLS 2 AXI Stream
    xf::compression::details::hlsStreamSize2axiu<OUT_DWIDTH>(outPackedStream, outPackedStreamEos, outStream,
                                                             outSizeStream, totalSizeStream, numBlockStreamEos);
}

template <int IN_DWIDTH, int OUT_DWIDTH, int BLOCK_SIZE, int STRATEGY = 0, int MIN_BLCK_SIZE = 1024> // STRATEGY: 0:
// GZIP; 1: ZLIB
void zlibCompressStaticStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                                 hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream,
                                 hls::stream<ap_axiu<32, 0, 0, 0> >& inSizeStream,
                                 hls::stream<ap_axiu<32, 0, 0, 0> >& outSizeStream) {
    constexpr int c_no_slaves = 3;
#pragma HLS DATAFLOW
    hls::stream<ap_uint<IN_DWIDTH> > inHlsStream("inHlsStream");
    hls::stream<bool> inHlsStreamEos("inHlsStreamEos");
    hls::stream<uint32_t> inHlsSizeStream("inHlsSizeStream");
    hls::stream<ap_uint<IN_DWIDTH> > inDataStream1("inDataStream1");
    hls::stream<ap_uint<IN_DWIDTH> > inDataStream2("inDataStream2");
    hls::stream<ap_uint<IN_DWIDTH> > inZlibStream("inZlibStream");
    hls::stream<ap_uint<IN_DWIDTH> > inStoredStream("inStoredStream");
    hls::stream<uint32_t> storedBlckSize("storedBlckSize");
    hls::stream<uint32_t> zlibBlockSizeStream("zlibBlockSizeStream");
    hls::stream<ap_uint<32> > checksumResult("checksumResult");
    hls::stream<ap_uint<OUT_DWIDTH> > outCompressedStream("outCompressedStream");
    hls::stream<bool> outCompressedStreamEos("outCompressedStreamEos");
    hls::stream<bool> outFileEos("outFileEos");
    hls::stream<uint32_t> blockCompressedSizeStream("blockCompressedSizeStream");
    hls::stream<ap_uint<OUT_DWIDTH> > outPackedStream("outPackedStream");
    hls::stream<bool> outPackedStreamEos("outPackedStreamEos");
    hls::stream<uint32_t> totalSizeStream("totalSizeStream");
    hls::stream<bool> numBlockStreamEos("numBlockStreamEos");
    hls::stream<uint32_t> inHlsSizeStreamVec[c_no_slaves];

#pragma HLS STREAM variable = inHlsSizeStream depth = 4
#pragma HLS STREAM variable = inHlsSizeStreamVec depth = 4
#pragma HLS STREAM variable = zlibBlockSizeStream depth = 4
#pragma HLS STREAM variable = blockCompressedSizeStream depth = 4
#pragma HLS STREAM variable = storedBlckSize depth = 4
#pragma HLS STREAM variable = inStoredStream depth = 4
#pragma HLS STREAM variable = totalSizeStream depth = 4
#pragma HLS STREAM variable = inHlsStream depth = 4
#pragma HLS STREAM variable = inHlsStreamEos depth = 4
#pragma HLS STREAM variable = outCompressedStream depth = 4
#pragma HLS STREAM variable = outCompressedStreamEos depth = 4

    // AXI 2 HLS Stream
    xf::compression::details::axiu2hlsStreamSizeEos<IN_DWIDTH>(inStream, inHlsStream, inHlsStreamEos, inHlsSizeStream,
                                                               inSizeStream);

    // Size Distributor
    xf::compression::details::streamSizeDistributor<c_no_slaves>(inHlsSizeStream, inHlsSizeStreamVec);

    // Data Stream Duplicator
    xf::compression::details::streamDuplicator<IN_DWIDTH>(inHlsStream, inHlsStreamEos, inDataStream1, inDataStream2);

    // Zlib/Gzip Block Maker
    xf::compression::details::streamBlockMaker<IN_DWIDTH, BLOCK_SIZE, MIN_BLCK_SIZE>(
        inDataStream1, inZlibStream, inStoredStream, storedBlckSize, zlibBlockSizeStream, inHlsSizeStreamVec[0]);

    // Checksum
    xf::compression::details::checksumWrapper<IN_DWIDTH, STRATEGY>(inDataStream2, inHlsSizeStreamVec[1],
                                                                   checksumResult);

    // Zlib Compress Stream IO Engine
    xf::compression::zlibCompressStatic<BLOCK_SIZE>(inZlibStream, outCompressedStream, outCompressedStreamEos,
                                                    outFileEos, blockCompressedSizeStream, zlibBlockSizeStream);

    // Packer
    xf::compression::details::zlibCompressStreamingPacker<IN_DWIDTH, OUT_DWIDTH, BLOCK_SIZE, STRATEGY>(
        inStoredStream, outCompressedStream, outPackedStream, outCompressedStreamEos, outPackedStreamEos,
        storedBlckSize, outFileEos, blockCompressedSizeStream, totalSizeStream, inHlsSizeStreamVec[2], checksumResult,
        numBlockStreamEos);

    // HLS 2 AXI Stream
    xf::compression::details::hlsStreamSize2axiu<OUT_DWIDTH>(outPackedStream, outPackedStreamEos, outStream,
                                                             outSizeStream, totalSizeStream, numBlockStreamEos);
}

} // End namespace compression
} // End namespace xf

#endif // _XFCOMPRESSION_ZLIB_COMPRESS_HPP_
