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

#ifndef _XFCOMPRESSION_ZSTD_COMPRESS_HPP_
#define _XFCOMPRESSION_ZSTD_COMPRESS_HPP_

/**
 * @file zstd_decompress.hpp
 * @brief Header for modules used in ZSTD compression kernel.
 *
 * This file is part of Vitis Data Compression Library.
 */

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include <ap_int.h>
#include <stdint.h>

#include "axi_stream_utils.hpp"
#include "zstd_specs.hpp"
#include "stream_upsizer.hpp"
#include "stream_downsizer.hpp"
#include "compress_utils.hpp"
#include "huffman_treegen.hpp"
#include "zstd_encoders.hpp"
#include "zstd_compress_internal.hpp"

namespace xf {
namespace compression {
namespace details {

template <int BLOCK_SIZE, int MIN_BLK_SIZE = 1020>
void inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStrdStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    constexpr uint16_t c_storedDepth = MIN_BLK_SIZE + 4;
    hls::stream<ap_uint<8> > bufStream;
#pragma HLS STREAM variable = bufStream depth = c_storedDepth
#pragma HLS BIND_STORAGE variable = bufStream type = fifo impl = srl

    uint16_t bufIdx = 0;
    bool not_done = true;
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 1> inVal;
    IntVectorStream_dt<8, 1> outVal;
    uint16_t blk_n = 0;
stream_blocks:
    while (not_done) {
        uint32_t dataSize = 0;
        bufIdx = 0;
    buffer_min_block:
        for (inVal = inStream.read(); bufIdx < MIN_BLK_SIZE && inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            bufStream << inVal.data[0];
            ++bufIdx;
            ++dataSize;
        }
        bool strdBlkFlag = (inVal.strobe == 0);
        outVal.strobe = 1;
    // Below loop won't execute for stored block case
    // for regular block, inStream is read once in previous loop
    stream_one_block:
        while ((inVal.strobe > 0) && (strdBlkFlag == false) && (dataSize < BLOCK_SIZE)) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = bufStream.read();
            bufStream << inVal.data[0];

            outStream << outVal;
            outStrdStream << outVal;
            ++dataSize;
            // Avoid extra read after last value in this block
            if (dataSize < BLOCK_SIZE) inVal = inStream.read();
        }

    stream_rem_strd_block:
        while (bufIdx > 0) {
#pragma HLS PIPEINE II = 1
            outVal.data[0] = bufStream.read();
            outStrdStream << outVal;
            if (!strdBlkFlag) outStream << outVal;
            --bufIdx;
        }

        not_done = (inVal.strobe > 0);
        if (dataSize) { // avoid sending twice for input size aligned to block size
            inVal.strobe = 0;
            outStrdStream << inVal;
            if (!strdBlkFlag) outStream << inVal;
            // send metadata to packer
            metaVal.strobe = 1;
            metaVal.data[0] = dataSize;
            blockMetaStream << metaVal;
            if (BLOCK_SIZE > (64 * 1024)) {
                metaVal.data[0] = dataSize >> 16;
            } else {
                metaVal.data[0] = 0;
            }
            blockMetaStream << metaVal;
        }
    }
    // end of file
    metaVal.strobe = 0;
    inVal.strobe = 0;
    blockMetaStream << metaVal;
    outStream << inVal;
    outStrdStream << inVal;
}

template <int MAX_FREQ_DWIDTH, class META_DT = ap_uint<MAX_FREQ_DWIDTH> >
void bitstreamCollector(hls::stream<META_DT>& lzMetaStream,
                        hls::stream<ap_uint<16> >& hfLitMetaStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& hfLitBitstream,
                        hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& litEncodedStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& seqEncodedStream,
                        hls::stream<META_DT>& bscMetaStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    // Collect encoded literals and sequences data and send ordered data to output
    IntVectorStream_dt<8, 4> outVal;
    uint8_t fseHdrBuf[48];
#pragma HLS ARRAY_PARTITION variable = fseHdrBuf complete
bsCol_main:
    while (true) {
        ap_uint<64> bitStream = 0;
        uint8_t byteCount = 0;
        // First write the FSE headers in order litblen->litlen->offset->matlen
        bool readFseHdr = false;
        auto fhVal = fseHeaderStream.read();
        if (fhVal.strobe == 0) break;
        // read meta data from LZ Compress, keep seqCnt and send rest to packer
        uint16_t seqCnt = 0;
    lz_meta_collect:
        for (uint8_t i = 0; i < 2; ++i) {
            auto lzMeta = lzMetaStream.read();
            bscMetaStream << lzMeta;
            seqCnt = lzMeta; // keeps the last value, that is seqCnt
        }
        uint16_t hdrBsLen[2] = {0, 0};
        uint8_t fseHIdx = 0;
    // Buffer fse header bitstreams for litlen and offset
    buff_fse_header_bs:
        for (uint8_t i = 0; i < 2 && seqCnt > 0; ++i) {
            if (readFseHdr) fhVal = fseHeaderStream.read();
            readFseHdr = true;
        buffer_llof_fsebs:
            for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPELINE II = 1
                fseHdrBuf[fseHIdx] = fhVal.data[0];
                fseHdrBuf[fseHIdx + 1] = fhVal.data[1];
                fseHIdx += fhVal.strobe;
                hdrBsLen[i] += fhVal.strobe;
            }
        }
        // write extra data as 0s
        fseHdrBuf[fseHIdx] = 0;
        fseHdrBuf[fseHIdx + 1] = 0;
        outVal.strobe = 4;
        uint16_t hdrBsSize = 0;
        // Send FSE header for literal header
        if (readFseHdr) fhVal = fseHeaderStream.read();
        readFseHdr = true;
    send_lit_fse_header:
        for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPEINE II = 1
            bitStream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = fhVal.data[0];
            bitStream.range(((byteCount + 2) * 8) - 1, (1 + byteCount) * 8) = fhVal.data[1];
            byteCount += fhVal.strobe;
            hdrBsSize += fhVal.strobe;
            if (byteCount > 3) {
                outVal.data[0] = bitStream.range(7, 0);
                outVal.data[1] = bitStream.range(15, 8);
                outVal.data[2] = bitStream.range(23, 16);
                outVal.data[3] = bitStream.range(31, 24);
                outStream << outVal;
                // adjust buffer and strobe
                bitStream >>= 32;
                byteCount -= 4;
            }
        }
        // Flush stream
        if (byteCount) {
            outVal.data[0] = bitStream.range(7, 0);
            outVal.data[1] = bitStream.range(15, 8);
            outVal.data[2] = bitStream.range(23, 16);
            outVal.data[3] = bitStream.range(31, 24);
            outVal.strobe = byteCount;
            outStream << outVal;
            byteCount = 0;
            bitStream = 0;
            outVal.strobe = 4;
        }
        // send size of literal codes fse header
        bscMetaStream << hdrBsSize;
        // Send FSE encoded bitstream for literal header
        uint8_t litEncSize = 0;
    send_lit_fse_bitstream:
        for (fhVal = litEncodedStream.read(); fhVal.strobe > 0; fhVal = litEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            bitStream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = fhVal.data[0];
            bitStream.range(((byteCount + 2) * 8) - 1, (1 + byteCount) * 8) = fhVal.data[1];
            byteCount += fhVal.strobe;
            litEncSize += fhVal.strobe;
            if (byteCount > 3) {
                outVal.data[0] = bitStream.range(7, 0);
                outVal.data[1] = bitStream.range(15, 8);
                outVal.data[2] = bitStream.range(23, 16);
                outVal.data[3] = bitStream.range(31, 24);
                outStream << outVal;
                // adjust buffer and strobe
                bitStream >>= 32;
                byteCount -= 4;
            }
        }
        // Flush stream
        if (byteCount) {
            outVal.data[0] = bitStream.range(7, 0);
            outVal.data[1] = bitStream.range(15, 8);
            outVal.data[2] = bitStream.range(23, 16);
            outVal.data[3] = bitStream.range(31, 24);
            outVal.strobe = byteCount;
            outStream << outVal;
            byteCount = 0;
            bitStream = 0;
            outVal.strobe = 4;
        }
        // send size of literal codes fse bitstream
        bscMetaStream << (uint16_t)litEncSize;
        // send huffman encoded bitstreams for literals
        uint8_t litStreamCnt = hfLitMetaStream.read();
        // write huffman bitstream sizes to packer meta
        bscMetaStream << (uint16_t)litStreamCnt;
    read_huf_strm_sizes:
        for (uint8_t i = 0; i < litStreamCnt; ++i) { // compressed sizes
            auto _cmpS = hfLitMetaStream.read();
            bscMetaStream << _cmpS;
        }
    send_all_hf_bitstreams:
        for (uint8_t i = 0; i < litStreamCnt; ++i) {
        send_huf_lit_bitstream:
            for (fhVal = hfLitBitstream.read(); fhVal.strobe > 0; fhVal = hfLitBitstream.read()) {
#pragma HLS PIPELINE II = 1
                bitStream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = fhVal.data[0];
                bitStream.range(((byteCount + 2) * 8) - 1, (1 + byteCount) * 8) = fhVal.data[1];
                byteCount += fhVal.strobe;
                if (byteCount > 3) {
                    outVal.data[0] = bitStream.range(7, 0);
                    outVal.data[1] = bitStream.range(15, 8);
                    outVal.data[2] = bitStream.range(23, 16);
                    outVal.data[3] = bitStream.range(31, 24);
                    outStream << outVal;
                    // adjust buffer and strobe
                    bitStream >>= 32;
                    byteCount -= 4;
                }
            }
        }
        // Flush stream, 4-huffman bitstreams are anyways continuous byte-by-byte
        if (byteCount) {
            outVal.data[0] = bitStream.range(7, 0);
            outVal.data[1] = bitStream.range(15, 8);
            outVal.data[2] = bitStream.range(23, 16);
            outVal.data[3] = bitStream.range(31, 24);
            outVal.strobe = byteCount;
            outStream << outVal;
            byteCount = 0;
            bitStream = 0;
            outVal.strobe = 4;
        }
        // Send FSE header for litlen and offsets from buffer
        // send sizes first
        bscMetaStream << hdrBsLen[0]; // litlen
        bscMetaStream << hdrBsLen[1]; // offset
    send_llof_fse_header_bs:
        for (uint8_t i = 0; i < fseHIdx; i += 4) {
#pragma HLS PIPELINE II = 1
            bitStream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = fseHdrBuf[i];
            bitStream.range(((byteCount + 2) * 8) - 1, (1 + byteCount) * 8) = fseHdrBuf[i + 1];
            bitStream.range(((byteCount + 3) * 8) - 1, (2 + byteCount) * 8) = fseHdrBuf[i + 2];
            bitStream.range(((byteCount + 4) * 8) - 1, (3 + byteCount) * 8) = fseHdrBuf[i + 3];
            byteCount += ((i < fseHIdx - 4) ? 4 : fseHIdx - i);
            if (byteCount > 3) {
                outVal.data[0] = bitStream.range(7, 0);
                outVal.data[1] = bitStream.range(15, 8);
                outVal.data[2] = bitStream.range(23, 16);
                outVal.data[3] = bitStream.range(31, 24);
                outStream << outVal;
                // adjust buffer and strobe
                bitStream >>= 32;
                byteCount -= 4;
            }
        }
        // Send matlen fse header
        hdrBsSize = 0;
        fhVal.strobe = 0;
        if (seqCnt > 0) fhVal = fseHeaderStream.read();
    send_ml_fse_header_bs:
        for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPELINE II = 1
            bitStream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = fhVal.data[0];
            bitStream.range(((byteCount + 2) * 8) - 1, (1 + byteCount) * 8) = fhVal.data[1];
            byteCount += fhVal.strobe;
            hdrBsSize += fhVal.strobe;
            if (byteCount > 3) {
                outVal.data[0] = bitStream.range(7, 0);
                outVal.data[1] = bitStream.range(15, 8);
                outVal.data[2] = bitStream.range(23, 16);
                outVal.data[3] = bitStream.range(31, 24);
                outStream << outVal;
                // adjust buffer and strobe
                bitStream >>= 32;
                byteCount -= 4;
            }
        }
        // Flush stream
        if (byteCount && seqCnt > 0) {
            outVal.data[0] = bitStream.range(7, 0);
            outVal.data[1] = bitStream.range(15, 8);
            outVal.data[2] = bitStream.range(23, 16);
            outVal.data[3] = bitStream.range(31, 24);
            outVal.strobe = byteCount;
            outStream << outVal;
            byteCount = 0;
            bitStream = 0;
            outVal.strobe = 4;
        }
        // send size of matlen fse header
        bscMetaStream << hdrBsSize;
        uint16_t seqBsSize = 0;
        // send sequences bitstream
        IntVectorStream_dt<8, 4> seqVal;
        seqVal.strobe = 0;
        if (seqCnt > 0) seqVal = seqEncodedStream.read();
    send_seq_fse_bitstream:
        for (; seqVal.strobe > 0; seqVal = seqEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            bitStream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = seqVal.data[0];
            bitStream.range(((byteCount + 2) * 8) - 1, (byteCount + 1) * 8) = seqVal.data[1];
            bitStream.range(((byteCount + 3) * 8) - 1, (byteCount + 2) * 8) = seqVal.data[2];
            bitStream.range(((byteCount + 4) * 8) - 1, (byteCount + 3) * 8) = seqVal.data[3];
            byteCount += seqVal.strobe;
            seqBsSize += seqVal.strobe;
            if (byteCount > 3) {
                outVal.data[0] = bitStream.range(7, 0);
                outVal.data[1] = bitStream.range(15, 8);
                outVal.data[2] = bitStream.range(23, 16);
                outVal.data[3] = bitStream.range(31, 24);
                outStream << outVal;
                // adjust buffer and strobe
                bitStream >>= 32;
                byteCount -= 4;
            }
        }
        // send the remaining data in bitVector
        if (byteCount) {
            outVal.data[0] = bitStream.range(7, 0);
            outVal.data[1] = bitStream.range(15, 8);
            outVal.data[2] = bitStream.range(23, 16);
            outVal.data[3] = bitStream.range(31, 24);
            outVal.strobe = byteCount;
            outStream << outVal;
            // adjust buffer and strobe
            byteCount = 0;
            bitStream = 0;
            outVal.strobe = 4;
        }
        // send size of sequences fse bitstream
        bscMetaStream << seqBsSize;
        // end of block data
        outVal.strobe = 0;
        outStream << outVal;
    }
    // dump end of data from remaining input streams
    hfLitBitstream.read();
    litEncodedStream.read();
    seqEncodedStream.read();
    // end of data
    outVal.strobe = 0;
    outStream << outVal;
}

template <int BLOCK_SIZE, int MIN_BLK_SIZE, int MAX_FREQ_DWIDTH, class META_DT = ap_uint<MAX_FREQ_DWIDTH> >
void packCompressedFrame(hls::stream<IntVectorStream_dt<16, 1> >& packerMetaStream,
                         hls::stream<META_DT>& bscMetaStream,
                         hls::stream<IntVectorStream_dt<8, 4> >& bscBitstream,
                         hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    // Collect encoded literals and sequences data and send formatted data to output
    constexpr uint8_t c_fcsFlag = ((BLOCK_SIZE < (64 * 1024)) ? 1 : 2); // use 16-bit or 32-bit frame content size
    constexpr bool isSingleSegment = 0;                                 // set if using 1 block/frame
    bool blockRead = false;
    IntVectorStream_dt<8, 4> outVal;
    ap_uint<64> bitstream = 0;
    uint8_t byteCount = 0;
    ap_uint<8> frameHeaderBuf[14];
    uint8_t fhIdx = 0;
    uint8_t fhdFixedOff = 6;
    uint32_t outSize = 0;

    /* Frame format
     * <Magic_Number><Frame_Header><Data_Block(s).....><Content_Checksum>
     * 	  4 bytes      2-14 bytes      n bytes....          0-4 bytes
     */
    /* Frame_Header format
     * <Frame_Header_Descriptor><Window_Descriptor><Dictionary_Id><Frame_Content_Size>
     * 		    1 byte				  0-1 byte		  0-4 bytes         0-8 bytes
     */
    // Add Magic_Number
    frameHeaderBuf[0] = xf::compression::details::c_magicNumber.range(7, 0);
    frameHeaderBuf[1] = xf::compression::details::c_magicNumber.range(15, 8);
    frameHeaderBuf[2] = xf::compression::details::c_magicNumber.range(23, 16);
    frameHeaderBuf[3] = xf::compression::details::c_magicNumber.range(31, 24);
    // Add frame header
    ap_uint<8> frameHeader = 0;
    frameHeader.range(7, 6) = c_fcsFlag;
    // we have 1 block/frame, therefore single segment flag can be set
    // so that window size becomes equal to frame content size
    frameHeaderBuf[4] = frameHeader;
    uint8_t windowLog = bitsUsed31((uint32_t)BLOCK_SIZE);
    uint32_t windowBase = (uint32_t)1 << windowLog;
    frameHeaderBuf[5].range(7, 3) = windowLog - 10;
    frameHeaderBuf[5].range(2, 0) = (uint8_t)((8 * (BLOCK_SIZE - windowBase)) >> windowLog);
zstd_frame_packer:
    while (true) {
        fhIdx = fhdFixedOff;
        // Read all the metadata needed to write frame header
        // read block meta data
        auto meta = packerMetaStream.read();
        if (meta.strobe == 0) break;
        /*** Start: Frame Content Size --- Valid ONLY with 1 block/frame case***/
        // In regular case, it should be sum of sizes of input blocks in a frame
        // block size can be max 128KB, using 3 bytes, if less, then 2nd read will be 0, with highest bit indicating
        // last data block
        ap_uint<24> fcs = 0;
        fcs.range(15, 0) = meta.data[0]; // write 16-bit part of size
        meta = packerMetaStream.read();
        fcs.range(23, 16) = (uint8_t)meta.data[0]; // remaining 8-bit of size
        // write fcs as block size for raw block case, to output
        outVal.data[1] = fcs.range(7, 0);
        outVal.data[2] = fcs.range(15, 8);
        outVal.data[3] = fcs.range(23, 16);

        if (c_fcsFlag == 1) fcs -= 256;
        frameHeaderBuf[fhIdx] = fcs.range(7, 0);
        frameHeaderBuf[fhIdx + 1] = fcs.range(15, 8);
        fhIdx += 2;
        if (c_fcsFlag == 2) { // not expecting more than 32-bits, may be up to 64-bits in standard
            frameHeaderBuf[fhIdx] = fcs.range(23, 16);
            frameHeaderBuf[fhIdx + 1] = 0;
            fhIdx += 2;
        }
        /*** End: Frame Content Size ***/
        // Now write frame header, pre-fixed with relevant meta data
        // Write the size of frame header and block size, this is not part of the format, it is for use in next module
        outVal.strobe = 4;
        outVal.data[0] = (uint8_t)fhIdx;
        outStream << outVal;
        // Write Frame header
        bitstream = 0;
        byteCount = 0;
    send_frame_header:
        for (uint8_t i = 0; i < fhIdx; ++i) {
#pragma HLS PIPELINE II = 1
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = frameHeaderBuf[i];
            ++byteCount;
            if (byteCount > 3) {
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                outVal.data[2] = bitstream.range(23, 16);
                outVal.data[3] = bitstream.range(31, 24);
                outStream << outVal;
                byteCount -= 4;
                bitstream >>= 32;
                outSize += 4;
            }
        }
        // skip stored block
        if (fcs < MIN_BLK_SIZE) {
            // send strobe 0
            outVal.strobe = 0;
            outStream << outVal;
            continue;
        }

        // Read all block meta data and add 3-byte block header
        auto litCnt = bscMetaStream.read();
        auto seqCnt = bscMetaStream.read();

        ap_uint<40> litSecHdr = 0;
        ap_uint<32> seqSecHdr = 0; //<1-3 bytes seq cnt><1 byte symCompMode>
        uint32_t blockSize = 0;
        ap_uint<16> streamSizes[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = streamSizes complete
        uint8_t hfStreamCnt = 0;
        uint16_t hfStrmSize = 0;
    read_meta_sizes:
        for (uint8_t i = 0; i < 7 + hfStreamCnt; ++i) {
#pragma HLS PIPELINE II = 1
            auto hdrSzVal = bscMetaStream.read();
            streamSizes[i] = hdrSzVal;
            if (i == 2) {
                hfStreamCnt = hdrSzVal; // 3rd value is number of literal streams
            } else {
                if (i > 2 + hfStreamCnt) blockSize += hdrSzVal; // add the size of compressed sequence bitstreams
                if (i > 2 && i - 3 < hfStreamCnt) {
                    hfStrmSize += hdrSzVal; // add huffman stream sizes
                }
            }
        }

        /*	Header Memory format
         * <-Block Header-><---------------- Literal Section -------------><------------ Sequences Section ------------>
         * 				   <Section_Header><HF Header><FSE Header, Bitstream><SeqCnt><SymCompMode><FSE
         * Tables
         * LL-OF-ML>
         * 					   5 bytes		  1 byte	 	128 bytes       1-3bytes
         * 1
         * byte
         * 0-63 bytes
         * 	  3 bytes							134 byte                          Upto
         * 67
         * bytes
         */
        // set sequence section header
        uint8_t seqBytes = 1;
        if (seqCnt < 128) {
            seqSecHdr.range(7, 0) = seqCnt; // use 1 byte only
        } else if (seqCnt > 127 && seqCnt < 32512) {
            seqBytes = 2;
            // calculate Number_of_Sequences = ((byte0-128) << 8) + byte1
            seqSecHdr.range(15, 8) = (uint8_t)seqCnt;
            seqSecHdr.range(7, 0) = (uint8_t)((seqCnt >> 8) & 0x0000FF) + 128;
        } else {
            seqBytes = 3;
            seqSecHdr.range(7, 0) = 255;
            // calculate Number_of_Sequences = byte1 + (byte2<<8) + 0x7F00
            seqSecHdr.range(23, 8) = seqCnt - 32512;
        }
        // ll,of,ml compression modes
        if (seqCnt > 0) {
            seqSecHdr.range((8 * (seqBytes + 1)) - 1, (8 * seqBytes)) = (uint8_t)((2 << 6) + (2 << 4) + (2 << 2));
            ++seqBytes;
        }
        // set literal section header
        uint8_t litSecHdrBCnt = 3 + (2 * (uint8_t)(hfStreamCnt > 1));
        uint8_t lsSzBits = 10 + (8 * (uint8_t)(hfStreamCnt > 1));
        litSecHdr.range(1, 0) = 2;                         // Block type -> Compressed
        litSecHdr.range(3, 2) = (hfStreamCnt > 1) ? 3 : 0; // Size format
        // regenerated size is litCnt
        litSecHdr.range(3 + lsSzBits, 4) = (uint32_t)litCnt; // original size
        // compressed size--> <HF tree Desc:<1byte HF header><fseBSHeader><fseBitstream>><0-6bytes jumpTable><HF Streams
        // <1><2><3><4>>
        litSecHdr.range(3 + (2 * lsSzBits), 4 + lsSzBits) =
            (uint32_t)(1 + streamSizes[0] + streamSizes[1] + (6 * (uint8_t)(hfStreamCnt > 1)) + hfStrmSize);
        // calculate block size, by adding bytes needed by different headers
        // sequence fse bitstreams size already added to blockSize
        blockSize += (litSecHdrBCnt + (uint32_t)litSecHdr.range(3 + (2 * lsSzBits), 4 + lsSzBits)) + seqBytes;
        // Write Block header
        /*
         * Block_Header: 3 bytes
         * <Last_Block><Block_Type><Block_Size>
         *    bit 0		 bits 1-2    bits 3-23
         */
        // Block Type = 2 (Compressed Block), 1 always last block in frame
        ap_uint<24> blockHeader = (uint32_t)1 + (2 << 1);
        blockHeader.range(23, 3) = blockSize;
        // Write Block Header -> Block Content
        bitstream.range(((byteCount + 3) * 8) - 1, byteCount * 8) = blockHeader;
        byteCount += 3;
        if (byteCount > 3) {
            outVal.data[0] = bitstream.range(7, 0);
            outVal.data[1] = bitstream.range(15, 8);
            outVal.data[2] = bitstream.range(23, 16);
            outVal.data[3] = bitstream.range(31, 24);
            outStream << outVal;
            byteCount -= 4;
            bitstream >>= 32;
            outSize += 4;
        }
    // Write Block Content
    // Write Literal Section header
    write_lit_sec_hdr:
        for (uint8_t i = 0; i < litSecHdrBCnt; ++i) {
#pragma HLS PIPELINE II = 1
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = litSecHdr.range(7, 0);
            ++byteCount;
            if (byteCount > 3) {
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                outVal.data[2] = bitstream.range(23, 16);
                outVal.data[3] = bitstream.range(31, 24);
                outStream << outVal;
                byteCount -= 4;
                bitstream >>= 32;
                outSize += 4;
            }
            litSecHdr >>= 8;
        }
        // Write fse header
        bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = (uint8_t)(streamSizes[0] + streamSizes[1]);
        ++byteCount;
    // read literal fse header, bitstream and literal huffman bitstreams
    write_lit_sec:
        for (uint8_t k = 0; k < 3; ++k) {
            // check if buffer full
            if (byteCount > 3) {
                // fill output register
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                outVal.data[2] = bitstream.range(23, 16);
                outVal.data[3] = bitstream.range(31, 24);
                // send output
                outStream << outVal;
                // adjust buffer
                byteCount -= 4;
                bitstream >>= 32;
                outSize += 4;
            }
            uint32_t itrSize = ((k < 2) ? (uint32_t)streamSizes[k] : (uint32_t)hfStrmSize);
        write_lithd_fse_data:
            for (uint16_t i = 0; i < itrSize; i += 4) {
#pragma HLS PIPELINE II = 1
                auto bsVal = bscBitstream.read();
                bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = bsVal.data[0];
                bitstream.range(((byteCount + 2) * 8) - 1, (byteCount + 1) * 8) = bsVal.data[1];
                bitstream.range(((byteCount + 3) * 8) - 1, (byteCount + 2) * 8) = bsVal.data[2];
                bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 3) * 8) = bsVal.data[3];
                byteCount += bsVal.strobe;
                if (byteCount > 3) {
                    // fill output register
                    outVal.data[0] = bitstream.range(7, 0);
                    outVal.data[1] = bitstream.range(15, 8);
                    outVal.data[2] = bitstream.range(23, 16);
                    outVal.data[3] = bitstream.range(31, 24);
                    // send output
                    outStream << outVal;
                    // adjust buffer and strobe
                    byteCount -= 4;
                    bitstream >>= 32;
                    outSize += 4;
                }
            }
            // Write jump table and huffman streams
            if (k == 1 && hfStreamCnt > 1) {
                bitstream.range(((byteCount + 2) * 8) - 1, byteCount * 8) = streamSizes[3];
                bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 2) * 8) = streamSizes[4];
                // fill output register
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                outVal.data[2] = bitstream.range(23, 16);
                outVal.data[3] = bitstream.range(31, 24);
                // send output
                outStream << outVal;
                // adjust buffer
                bitstream >>= 32;
                outSize += 4;
                bitstream.range(((byteCount + 2) * 8) - 1, byteCount * 8) = streamSizes[5];
                byteCount += 2;
            }
        }
    // write sequence section header
    send_seq_sec_hdr:
        for (uint8_t i = 0; i < seqBytes; ++i) {
#pragma HLS PIPELINE II = 1
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = seqSecHdr.range(7, 0);
            ++byteCount;
            seqSecHdr >>= 8;
            if (byteCount > 3) {
                // fill output register
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                outVal.data[2] = bitstream.range(23, 16);
                outVal.data[3] = bitstream.range(31, 24);
                // send output
                outStream << outVal;
                // adjust buffer and strobe
                byteCount -= 4;
                bitstream >>= 32;
                outSize += 4;
            }
        }
        // write sequence headers and bitstreams (write entire available bitstream)
        IntVectorStream_dt<8, 4> seqVal;
        seqVal.strobe = 0;
        if (seqCnt > 0) seqVal = bscBitstream.read();
    send_seq_fse_bitstreams:
        for (; seqVal.strobe > 0; seqVal = bscBitstream.read()) {
#pragma HLS PIPELINE II = 1
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = seqVal.data[0];
            bitstream.range(((byteCount + 2) * 8) - 1, (byteCount + 1) * 8) = seqVal.data[1];
            bitstream.range(((byteCount + 3) * 8) - 1, (byteCount + 2) * 8) = seqVal.data[2];
            bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 3) * 8) = seqVal.data[3];
            byteCount += seqVal.strobe;
            if (byteCount > 3) {
                // fill output register
                outVal.data[0] = bitstream.range(7, 0);
                outVal.data[1] = bitstream.range(15, 8);
                outVal.data[2] = bitstream.range(23, 16);
                outVal.data[3] = bitstream.range(31, 24);
                // send output
                outStream << outVal;
                // adjust buffer and strobe
                byteCount -= 4;
                bitstream >>= 32;
                outSize += 4;
            }
        }
        if (byteCount) {
            // fill output register
            outVal.data[0] = bitstream.range(7, 0);
            outVal.data[1] = bitstream.range(15, 8);
            outVal.data[2] = bitstream.range(23, 16);
            outVal.data[3] = bitstream.range(31, 24);
            outVal.strobe = byteCount;
            // send output
            outStream << outVal;
            outSize += byteCount;
        }
        // send strobe 0
        outVal.strobe = 0;
        outStream << outVal;
        // dump zero strobe for no sequence case
        if (seqCnt == 0) bscBitstream.read();
    }
    // dump strobe 0
    bscBitstream.read();
}

template <int MIN_BLK_SIZE = 1020>
void streamCmpStrdFrame(hls::stream<IntVectorStream_dt<8, 1> >& inStrBStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& inCmpBStream,
                        hls::stream<bool>& strdBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    IntVectorStream_dt<8, 4> outVal;
    ap_uint<24> strdBlockHeader = 1; // bit-0 = 1, indicating last block, bits 1-2 = 0, indicating raw block

stream_cmp_file:
    while (true) {
        // Read Raw block into Buffer
        uint16_t mIdx = 0;
        auto inRawVal = inStrBStream.read();
        if (inRawVal.strobe == 0) break;
        // Read and forward each frame
        bool readFlag = false;
        auto cmpReg = inCmpBStream.read();
        uint8_t hdrLen = cmpReg.data[0];
        strdBlockHeader.range(10, 3) = (uint8_t)cmpReg.data[1];
        strdBlockHeader.range(18, 11) = (uint8_t)cmpReg.data[2];
        strdBlockHeader.range(23, 19) = (uint8_t)cmpReg.data[3];
        bool strdBlkFlag = false;
        // skip stored block
        if (strdBlockHeader.range(23, 3) < MIN_BLK_SIZE) {
            strdBlkFlag = true;
        } else {
            // written only if block is greater than minimum block size
            strdBlkFlag = strdBlockFlagStream.read();
        }
    write_strd_frame:
        for (uint8_t i = 0; i < hdrLen; i += 4) {
#pragma HLS PIPELINE II = 1
            cmpReg = inCmpBStream.read();
            outVal.data[0] = cmpReg.data[0];
            outVal.data[1] = cmpReg.data[1];
            outVal.data[2] = cmpReg.data[2];
            outVal.data[3] = cmpReg.data[3];
            if (!strdBlkFlag) {
                outVal.strobe = 4;
            } else {
                outVal.strobe = ((i + 4 > hdrLen) ? (hdrLen % 4) : 4);
            }
            outStream << outVal;
        }
        if (strdBlkFlag) {
            // Write stored block header
            outVal.data[0] = strdBlockHeader.range(7, 0);
            outVal.data[1] = strdBlockHeader.range(15, 8);
            outVal.data[2] = strdBlockHeader.range(23, 16);
            outVal.data[3] = inRawVal.data[0]; // 1 data already read
            outVal.strobe = 4;
            outStream << outVal;
        }
        outVal.strobe = 0;
        bool done = false;
    write_output_block:
        while (!done) {
#pragma HLS PIPELINE II = 1
            if (inRawVal.strobe > 0) {
                inRawVal = inStrBStream.read();
                outVal.data[0] = inRawVal.data[0];
                outVal.strobe = inRawVal.strobe;
            }
            if (cmpReg.strobe > 0) cmpReg = inCmpBStream.read();
            // write to output
            if (strdBlkFlag) {
                if (outVal.strobe > 0) outStream << outVal;
                outVal.strobe = 0;
            } else {
                if (cmpReg.strobe > 0) outStream << cmpReg;
            }
            done = (inRawVal.strobe == 0 && cmpReg.strobe == 0);
        }
    }
    // end of file
    outVal.strobe = 0;
    outStream << outVal;
}

template <int IN_DWIDTH>
void zstdAxiu2hlsStream(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                        hls::stream<IntVectorStream_dt<IN_DWIDTH, 1> >& outHlsStream) {
    constexpr uint8_t c_keepDWidth = IN_DWIDTH / 8;
    IntVectorStream_dt<IN_DWIDTH, 1> outVal;
    outVal.strobe = 1;
    auto inAxiVal = inStream.read();
axi_to_hls:
    for (; inAxiVal.last == false; inAxiVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
        outVal.data[0] = inAxiVal.data;
        outHlsStream << outVal;
    }
    uint8_t strb = countSetBits<c_keepDWidth>((int)(inAxiVal.keep));
    if (strb) { // write last byte if valid
        outVal.data[0] = inAxiVal.data;
        outHlsStream << outVal;
    }
    outVal.strobe = 0;
    outHlsStream << outVal;
}

template <int OUT_DWIDTH>
void zstdHlsVectorStream2axiu(hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> >& hlsInStream,
                              hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
    constexpr uint8_t c_bytesInWord = OUT_DWIDTH / 8;
    ap_axiu<OUT_DWIDTH, 0, 0, 0> outVal;
    ap_uint<OUT_DWIDTH * 2> outReg;
    uint8_t bytesInReg = 0;
    outVal.keep = -1;
    outVal.last = false;
hls_to_axi:
    for (auto inVal = hlsInStream.read(); inVal.strobe > 0; inVal = hlsInStream.read()) {
#pragma HLS PIPELINE II = 1
        // Write data to output register
        for (uint8_t i = 0; i < c_bytesInWord; ++i) {
#pragma HLS UNROLL
            if (i < inVal.strobe) {
                outReg.range(((bytesInReg + i + 1) * 8) - 1, (bytesInReg + i) * 8) = inVal.data[i];
            }
        }
        bytesInReg += inVal.strobe;
        if (bytesInReg > c_bytesInWord - 1) {
            outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
            outStream << outVal;
            outReg >>= OUT_DWIDTH;
            bytesInReg -= c_bytesInWord;
        }
    }
    if (bytesInReg) {
        outVal.keep = ((1 << bytesInReg) - 1);
        outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
        outStream << outVal;
    }
    outVal.last = true;
    outVal.keep = 0;
    outVal.data = 0;
    outStream << outVal;
}

} // details

/**
 * @brief This module compresses the input file read from input stream.
 *        It produces the ZSTD compressed data at the output stream.
 *
 * @tparam BLOCK_SIZE ZStd block size
 * @tparam LZWINDOW_SIZE LZ77 history size or Window size
 * @tparam MIN_BLCK_SIZE Minimum block size, less than that will be considered stored block
 * @tparam PARALLEL_HUFFMAN Number of Huffman encoding units used
 * @tparam PARALLEL_LIT_STREAMS Number of parallel literal streams encoded using huffman
 * @tparam MIN_MATCH Minimum match in LZ77
 *
 * @param inStream input stream
 * @param outStream output stream
 */
template <int BLOCK_SIZE,
          int LZWINDOW_SIZE,
          int MIN_BLCK_SIZE,
          int PARALLEL_HUFFMAN = 8,
          int PARALLEL_LIT_STREAMS = 4,
          int MIN_MATCH = 3>
void zstdCompressCore(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    // zstd compression main module
    constexpr uint32_t c_strdStreamDepth = BLOCK_SIZE * 3;
    constexpr int c_freq_dwidth = maxBitsUsed(BLOCK_SIZE);
    constexpr int c_blen_dwidth = maxBitsUsed(c_freq_dwidth);
    constexpr int c_litUpSDepth = BLOCK_SIZE / 8;
    constexpr int c_halfBlockDepth = BLOCK_SIZE / 2;
    constexpr int c_seqBlockDepth = 1 + (BLOCK_SIZE - 1) / 6;
    constexpr int c_stbUpSDepth = c_strdStreamDepth / 8;

    // Internal streams
    hls::stream<IntVectorStream_dt<8, 1> > inBlockStream("inBlockStream");
    hls::stream<IntVectorStream_dt<8, 1> > strdBlockStream("strdBlockStream");
    hls::stream<IntVectorStream_dt<8, 1> > strdDsBlockStream("strdDsBlockStream");
    hls::stream<IntVectorStream_dt<16, 1> > packerMetaStream("packerMetaStream");
    hls::stream<ap_uint<68> > stbUpsizedStream("stbUpsizedStream"); // 3 URAMs
#pragma HLS STREAM variable = inBlockStream depth = 16
#pragma HLS STREAM variable = strdBlockStream depth = 16
#pragma HLS STREAM variable = strdDsBlockStream depth = 64
#pragma HLS STREAM variable = packerMetaStream depth = 16
#pragma HLS STREAM variable = stbUpsizedStream depth = c_stbUpSDepth
#pragma HLS BIND_STORAGE variable = stbUpsizedStream type = FIFO impl = URAM

    hls::stream<IntVectorStream_dt<8, 1> > litStream("litStream");
    hls::stream<IntVectorStream_dt<8, 1> > reverseLitStream("reverseLitStream"); // Taking 1 BRAM
    hls::stream<IntVectorStream_dt<8, 1> > dszLitStream("litStream");
    hls::stream<ap_uint<68> > litUpsizedStream("litUpsizedStream"); // 1 URAM
#pragma HLS STREAM variable = litStream depth = 16
#pragma HLS STREAM variable = reverseLitStream depth = 4096
#pragma HLS STREAM variable = dszLitStream depth = 1024
#pragma HLS STREAM variable = litUpsizedStream depth = c_litUpSDepth
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = URAM

    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > seqStream("seqStream");
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > reverseSeqStream(
        "reverseSeqStream"); // Taking 4 BRAMs
#pragma HLS STREAM variable = seqStream depth = 32
#pragma HLS STREAM variable = reverseSeqStream depth = 4096

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > litFreqStream("litFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > seqFreqStream("seqFreqStream");
#pragma HLS STREAM variable = litFreqStream depth = 16
#pragma HLS STREAM variable = seqFreqStream depth = 128

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > wghtFreqStream("wghtFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > freqStream("freqStream");
#pragma HLS STREAM variable = wghtFreqStream depth = 256
#pragma HLS STREAM variable = freqStream depth = 128

    hls::stream<ap_uint<c_freq_dwidth> > lzMetaStream("lzMetaStream");
    hls::stream<bool> rleFlagStream("rleFlagStream");
    hls::stream<bool> strdBlockFlagStream("strdBlockFlagStream");
    hls::stream<ap_uint<c_freq_dwidth> > litCntStream("litCntStream");
#pragma HLS STREAM variable = lzMetaStream depth = 16
#pragma HLS STREAM variable = rleFlagStream depth = 4
#pragma HLS STREAM variable = strdBlockFlagStream depth = 4
#pragma HLS STREAM variable = litCntStream depth = 8

    hls::stream<IntVectorStream_dt<8, 2> > fseHeaderStream("fseHeaderStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseLitTableStream("fseLitTableStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseSeqTableStream("fseSeqTableStream");
#pragma HLS STREAM variable = fseHeaderStream depth = 128
#pragma HLS STREAM variable = fseLitTableStream depth = 1024
#pragma HLS STREAM variable = fseSeqTableStream depth = 4096

    hls::stream<ap_uint<16> > hufLitMetaStream("hufLitMetaStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hufCodeStream;
    hls::stream<IntVectorStream_dt<4, 1> > hufWeightStream("hufWeightStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hfEncodedLitStream;
    hls::stream<IntVectorStream_dt<8, 2> > hfLitBitstream("hfLitBitstream"); // Taking 18 BRAMs
#pragma HLS STREAM variable = hufLitMetaStream depth = 128
#pragma HLS STREAM variable = hufCodeStream depth = 16
#pragma HLS STREAM variable = hufWeightStream depth = 512
#pragma HLS STREAM variable = hfEncodedLitStream depth = 128
#pragma HLS STREAM variable = hfLitBitstream depth = c_halfBlockDepth

    hls::stream<IntVectorStream_dt<8, 2> > litEncodedStream("litEncodedStream");
    hls::stream<IntVectorStream_dt<8, 4> > seqEncodedStream(
        "seqEncodedStream"); // update with packed structure (Taking 35 BRAMs)
    hls::stream<ap_uint<16> > bscMetaStream("bscMetaStream");
    hls::stream<IntVectorStream_dt<8, 4> > bscBitstream("bscBitstream");
#pragma HLS STREAM variable = litEncodedStream depth = 128
#pragma HLS STREAM variable = seqEncodedStream depth = c_seqBlockDepth
#pragma HLS STREAM variable = bscMetaStream depth = 128
#pragma HLS STREAM variable = bscBitstream depth = 8192

    hls::stream<IntVectorStream_dt<8, 4> > cmpFrameStream("cmpFrameStream");
#pragma HLS STREAM variable = cmpFrameStream depth = 128

#pragma HLS dataflow

    // Module-1: Input reading and LZ77 compression
    {
        details::inputDistributer<BLOCK_SIZE, MIN_BLCK_SIZE>(inStream, inBlockStream, strdBlockStream,
                                                             packerMetaStream);
        // Upsize raw data for raw block
        details::simpleStreamUpsizer<8, 64, 4>(strdBlockStream, stbUpsizedStream);
        // LZ77 compression of input blocks to get separate streams
        // for literals, sequences (litlen, metlen, offset), literal frequencies and sequences frequencies
        details::getLitSequences<BLOCK_SIZE, c_freq_dwidth>(inBlockStream, litStream, seqStream, litFreqStream,
                                                            seqFreqStream, rleFlagStream, strdBlockFlagStream,
                                                            lzMetaStream, litCntStream);
        // Upsize literals
        details::simpleStreamUpsizer<8, 64, 4>(litStream, litUpsizedStream);
        // Buffer in stream URAM and downsize literals
        details::bufferDownsizer<64, 8, 4>(litUpsizedStream, dszLitStream);
    }
    // Module-2: Encoding table generation and data preparation
    {
        // Buffer, reverse and break input literal stream into 4 streams of 1/4th size
        details::preProcessLitStream<BLOCK_SIZE, c_freq_dwidth, PARALLEL_LIT_STREAMS>(dszLitStream, litCntStream,
                                                                                      reverseLitStream);
        // Reverse sequences stream
        details::reverseSeq<BLOCK_SIZE, c_freq_dwidth, MIN_MATCH>(seqStream, reverseSeqStream);
        // generate hufffman tree and get codes-bitlens
        zstdTreegenStream<c_freq_dwidth, details::c_maxZstdHfBits>(litFreqStream, hufCodeStream, hufWeightStream,
                                                                   wghtFreqStream);
        // feed frequency data to fse table gen from literals and sequences
        details::frequencySequencer<c_freq_dwidth>(wghtFreqStream, seqFreqStream, freqStream);
        // generate FSE Tables for litlen, matlen, offset and literal-bitlen
        details::fseTableGen(freqStream, fseHeaderStream, fseLitTableStream, fseSeqTableStream);
    }
    // Module-3: Encoding literal and sequences
    {
        // Huffman encoding of literal stream
        details::zstdHuffmanEncoder<details::c_maxZstdHfBits>(reverseLitStream, rleFlagStream, hufCodeStream,
                                                              hfEncodedLitStream, hufLitMetaStream);
        // HUffman bitstream packer
        details::zstdHuffBitPacker<details::c_maxZstdHfBits>(hfEncodedLitStream, hfLitBitstream);
        // FSE encoding of literals
        details::fseEncodeLitHeader(hufWeightStream, fseLitTableStream, litEncodedStream);
        // FSE encode sequences generated by lz77 compression
        details::fseEncodeSequences(reverseSeqStream, fseSeqTableStream, seqEncodedStream);
    }
    // Module-4: Output block and frame packing
    {
        // pack compressed data into single sequential block stream
        details::bitstreamCollector<c_freq_dwidth>(lzMetaStream, hufLitMetaStream, hfLitBitstream, fseHeaderStream,
                                                   litEncodedStream, seqEncodedStream, bscMetaStream, bscBitstream);
        details::packCompressedFrame<BLOCK_SIZE, MIN_BLCK_SIZE, c_freq_dwidth>(packerMetaStream, bscMetaStream,
                                                                               bscBitstream, cmpFrameStream);

        // Buffer in stream URAM and downsize raw block data
        details::bufferDownsizer<64, 8, 4>(stbUpsizedStream, strdDsBlockStream);
        // Output compressed or raw block based on input flag stream
        details::streamCmpStrdFrame<MIN_BLCK_SIZE>(strdDsBlockStream, cmpFrameStream, strdBlockFlagStream, outStream);
    }
}

/**
 * @brief This module is top level wrapper for zstd compression core module
 *        It compresses the input file read from input axi stream.
 *        It produces the ZSTD compressed data at the output axi stream.
 *
 * @tparam IN_DWIDTH Input stream data bit-width
 * @tparam OUT_DWIDTH Output stream data bit-width
 * @tparam BLOCK_SIZE ZStd block size
 * @tparam LZWINDOW_SIZE LZ77 history size or Window size
 * @tparam MIN_BLCK_SIZE Minimum block size, less than that will be considered stored block
 *
 * @param inStream input stream
 * @param outStream output stream
 */
template <int IN_DWIDTH, int OUT_DWIDTH, int BLOCK_SIZE, int LZWINDOW_SIZE, int MIN_BLCK_SIZE = 1020>
void zstdCompressStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                           hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
#pragma HLS DATAFLOW
    hls::stream<IntVectorStream_dt<IN_DWIDTH, 1> > inZstdStream("inZstdStream");
    hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> > outCompressedStream("outCompressedStream");

#pragma HLS STREAM variable = inZstdStream depth = 8
#pragma HLS STREAM variable = outCompressedStream depth = 8

    // AXI 2 HLS Stream
    xf::compression::details::zstdAxiu2hlsStream<IN_DWIDTH>(inStream, inZstdStream);

    // Zlib Compress Stream IO Engine
    xf::compression::zstdCompressCore<BLOCK_SIZE, LZWINDOW_SIZE, MIN_BLCK_SIZE>(inZstdStream, outCompressedStream);

    // HLS 2 AXI Stream
    xf::compression::details::zstdHlsVectorStream2axiu<OUT_DWIDTH>(outCompressedStream, outStream);
}

} // compression
} // xf
#endif
