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
#include "compress_utils.hpp"
#include "huffman_treegen.hpp"
#include "zstd_encoders.hpp"
#include "zstd_compress_internal.hpp"

namespace xf {
namespace compression {
namespace details {

template <int BLOCK_SIZE>
void inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    bool not_done = true;
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 1> inVal;
    uint16_t blk_n = 0;
stream_blocks:
    while (not_done) {
        uint32_t dataSize = 0;
    stream_one_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << inVal;
            dataSize += inVal.strobe;
            if (dataSize > BLOCK_SIZE - 1) break;
        }
        not_done = (inVal.strobe > 0);
        if (dataSize) { // avoid sending twice for input size aligned to block size
            inVal.strobe = 0;
            outStream << inVal;
            // printf("%u. Written block size: %u\n", blk_n++, dataSize);
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
}

void bitstreamCollector(hls::stream<ap_uint<16> >& hfLitMetaStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& hfLitBitstream,
                        hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream,
                        hls::stream<IntVectorStream_dt<8, 2> >& litEncodedStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& seqEncodedStream,
                        hls::stream<ap_uint<16> >& bscMetaStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    // Collect encoded literals and sequences data and send ordered data to output
    IntVectorStream_dt<8, 4> outVal;
    ap_uint<16> outMeta;
    uint8_t fseHdrBuf[48];
bsCol_main:
    while (true) {
        IntVectorStream_dt<8, 7> bitVector;
        // First write the FSE headers in order litblen->litlen->offset->matlen
        bool readFseHdr = false;
        auto fhVal = fseHeaderStream.read();
        if (fhVal.strobe == 0) break;
        // printf("Start block data ordering\n");
        uint16_t hdrBsLen[2] = {0, 0};
        uint8_t fseHIdx = 0;
    // Buffer fse header bitstreams for litlen and offset
    buff_fse_header_bs:
        for (uint8_t i = 0; i < 2; ++i) {
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
            // printf("ll->of fse header BS len: %u\n", hdrBsLen[i]);
        }
        // write extra data as 0s
        fseHdrBuf[fseHIdx] = 0;
        fseHdrBuf[fseHIdx + 1] = 0;
        outVal.strobe = 4;
        bitVector.strobe = 0;
        uint16_t hdrBsSize = 0;
    // Send FSE header for literal header
    send_lit_fse_header:
        for (fhVal = fseHeaderStream.read(); fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPEINE II = 1
            bitVector.data[bitVector.strobe] = fhVal.data[0];
            bitVector.data[bitVector.strobe + 1] = fhVal.data[1];
            bitVector.strobe += fhVal.strobe;
            hdrBsSize += fhVal.strobe;
            if (bitVector.strobe > 3) {
                outVal.data[0] = bitVector.data[0];
                outVal.data[1] = bitVector.data[1];
                outVal.data[2] = bitVector.data[2];
                outVal.data[3] = bitVector.data[3];
                outStream << outVal;
                bitVector.strobe -= 4;
                // shift data from MSB
                bitVector.data[0] = bitVector.data[4];
                bitVector.data[1] = bitVector.data[5];
                bitVector.data[2] = bitVector.data[6];
            }
        }
        // Flush stream
        if (bitVector.strobe) {
            outVal.data[0] = bitVector.data[0];
            outVal.data[1] = bitVector.data[1];
            outVal.data[2] = bitVector.data[2];
            outVal.data[3] = bitVector.data[3];
            outVal.strobe = bitVector.strobe;
            outStream << outVal;
            bitVector.strobe = 0;
            outVal.strobe = 4;
        }
        // send size of literal codes fse header
        bscMetaStream << hdrBsSize;
        // printf("lit-blen fse header BS len: %u\n", hdrBsSize);
        // Send FSE encoded bitstream for literal header
        uint8_t litEncSize = 0;
    send_lit_fse_bitstream:
        for (fhVal = litEncodedStream.read(); fhVal.strobe > 0; fhVal = litEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            bitVector.data[bitVector.strobe] = fhVal.data[0];
            bitVector.data[bitVector.strobe + 1] = fhVal.data[1];
            bitVector.strobe += fhVal.strobe;
            // for (int i = 0; i < fhVal.strobe; ++i) printf("%d. lfsebs: %u\n", litEncSize + i,
            // (uint8_t)fhVal.data[i]);
            litEncSize += fhVal.strobe;
            if (bitVector.strobe > 3) {
                outVal.data[0] = bitVector.data[0];
                outVal.data[1] = bitVector.data[1];
                outVal.data[2] = bitVector.data[2];
                outVal.data[3] = bitVector.data[3];
                outStream << outVal;
                bitVector.strobe -= 4;
                // shift data from MSB
                bitVector.data[0] = bitVector.data[4];
                bitVector.data[1] = bitVector.data[5];
                bitVector.data[2] = bitVector.data[6];
            }
        }
        // Flush stream
        if (bitVector.strobe) {
            outVal.data[0] = bitVector.data[0];
            outVal.data[1] = bitVector.data[1];
            outVal.data[2] = bitVector.data[2];
            outVal.data[3] = bitVector.data[3];
            outVal.strobe = bitVector.strobe;
            outStream << outVal;
            bitVector.strobe = 0;
            outVal.strobe = 4;
        }
        // send size of literal codes fse bitstream
        bscMetaStream << (uint16_t)litEncSize;
        // printf("lit header fse encoded BS len: %u\n", litEncSize);
        // send huffman encoded bitstreams for literals
        uint8_t litStreamCnt = hfLitMetaStream.read();
        // write huffman bitstream sizes to packer meta
        bscMetaStream << (uint16_t)litStreamCnt;
    // printf("lit bs stream count: %u\n", litStreamCnt);
    read_huf_strm_sizes:
        for (uint8_t i = 0; i < litStreamCnt; ++i) { // compressed sizes
            auto _cmpS = hfLitMetaStream.read();
            bscMetaStream << _cmpS;
            // printf("%u. hf stream size: %u\n", i, (uint16_t)_cmpS);
        }
    send_all_hf_bitstreams:
        for (uint8_t i = 0; i < litStreamCnt; ++i) {
        send_huf_lit_bitstream:
            for (fhVal = hfLitBitstream.read(); fhVal.strobe > 0; fhVal = hfLitBitstream.read()) {
#pragma HLS PIPELINE II = 1
                bitVector.data[bitVector.strobe] = fhVal.data[0];
                bitVector.data[bitVector.strobe + 1] = fhVal.data[1];
                bitVector.strobe += fhVal.strobe;
                if (bitVector.strobe > 3) {
                    outVal.data[0] = bitVector.data[0];
                    outVal.data[1] = bitVector.data[1];
                    outVal.data[2] = bitVector.data[2];
                    outVal.data[3] = bitVector.data[3];
                    outStream << outVal;
                    bitVector.strobe -= 4;
                    // shift data from MSB
                    bitVector.data[0] = bitVector.data[4];
                    bitVector.data[1] = bitVector.data[5];
                    bitVector.data[2] = bitVector.data[6];
                }
            }
        }
        // Flush stream, 4-huffman bitstreams are anyways continuous byte-by-byte
        if (bitVector.strobe) {
            outVal.data[0] = bitVector.data[0];
            outVal.data[1] = bitVector.data[1];
            outVal.data[2] = bitVector.data[2];
            outVal.data[3] = bitVector.data[3];
            outVal.strobe = bitVector.strobe;
            outStream << outVal;
            bitVector.strobe = 0;
            outVal.strobe = 4;
        }
        // Send FSE header for litlen and offsets from buffer
        // send sizes first
        bscMetaStream << hdrBsLen[0]; // litlen
        bscMetaStream << hdrBsLen[1]; // offset
                                      // printf("litlen fse bs len: %u\n", hdrBsLen[0]);
    // printf("offset fse bs len: %u\n", hdrBsLen[1]);
    send_llof_fse_header_bs:
        for (uint8_t i = 0; i < fseHIdx; i += 2) {
#pragma HLS PIPELINE II = 1
            bitVector.data[bitVector.strobe] = fseHdrBuf[i];
            bitVector.data[bitVector.strobe + 1] = fseHdrBuf[i + 1];
            bitVector.strobe += 1 + (uint8_t)(i + 1 < fseHIdx);
            if (bitVector.strobe > 3) {
                outVal.data[0] = bitVector.data[0];
                outVal.data[1] = bitVector.data[1];
                outVal.data[2] = bitVector.data[2];
                outVal.data[3] = bitVector.data[3];
                outStream << outVal;
                bitVector.strobe -= 4;
                // shift data from MSB
                bitVector.data[0] = bitVector.data[4];
                bitVector.data[1] = bitVector.data[5];
                bitVector.data[2] = bitVector.data[6];
            }
        }
        // Send matlen fse header
        hdrBsSize = 0;
    send_ml_fse_header_bs:
        for (fhVal = fseHeaderStream.read(); fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPELINE II = 1
            bitVector.data[bitVector.strobe] = fhVal.data[0];
            bitVector.data[bitVector.strobe + 1] = fhVal.data[1];
            // printf("%u. mtln fse hbs: %u\n", hdrBsSize, (uint8_t)fhVal.data[0]);
            // printf("%u. mtln fse hbs: %u\n", hdrBsSize + 1, (uint8_t)fhVal.data[1]);
            bitVector.strobe += fhVal.strobe;
            hdrBsSize += fhVal.strobe;
            if (bitVector.strobe > 3) {
                outVal.data[0] = bitVector.data[0];
                outVal.data[1] = bitVector.data[1];
                outVal.data[2] = bitVector.data[2];
                outVal.data[3] = bitVector.data[3];
                outStream << outVal;
                bitVector.strobe -= 4;
                // shift data from MSB
                bitVector.data[0] = bitVector.data[4];
                bitVector.data[1] = bitVector.data[5];
                bitVector.data[2] = bitVector.data[6];
            }
        }
        // Flush stream
        if (bitVector.strobe) {
            outVal.data[0] = bitVector.data[0];
            outVal.data[1] = bitVector.data[1];
            outVal.data[2] = bitVector.data[2];
            outVal.data[3] = bitVector.data[3];
            outVal.strobe = bitVector.strobe;
            outStream << outVal;
            bitVector.strobe = 0;
            outVal.strobe = 4;
        }
        // send size of matlen fse header
        bscMetaStream << hdrBsSize;
        // printf("matlen fse bs len: %u\n", hdrBsSize);
        uint16_t seqBsSize = 0;
    // send sequences bitstream
    send_seq_fse_bitstream:
        for (auto seqVal = seqEncodedStream.read(); seqVal.strobe > 0; seqVal = seqEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            bitVector.data[bitVector.strobe] = seqVal.data[0];
            bitVector.data[bitVector.strobe + 1] = seqVal.data[1];
            bitVector.data[bitVector.strobe + 2] = seqVal.data[2];
            bitVector.data[bitVector.strobe + 3] = seqVal.data[3];
            bitVector.strobe += seqVal.strobe;
            seqBsSize += seqVal.strobe;
            if (bitVector.strobe > 3) {
                // fill output register
                outVal.data[0] = bitVector.data[0];
                outVal.data[1] = bitVector.data[1];
                outVal.data[2] = bitVector.data[2];
                outVal.data[3] = bitVector.data[3];
                // send output
                outStream << outVal;
                bitVector.strobe -= 4;
                bitVector.data[0] = bitVector.data[4];
                bitVector.data[1] = bitVector.data[5];
                bitVector.data[2] = bitVector.data[6];
            }
        }
        // send the remaining data in bitVector
        if (bitVector.strobe) {
            outVal.data[0] = bitVector.data[0];
            outVal.data[1] = bitVector.data[1];
            outVal.data[2] = bitVector.data[2];
            outVal.data[3] = bitVector.data[3];
            outVal.strobe = bitVector.strobe;
            // send output
            outStream << outVal;
        }
        // send size of sequences fse bitstream
        bscMetaStream << seqBsSize;
        // printf("seq fse encoded bs len: %u\n", seqBsSize);
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

template <int BLOCK_SIZE, int MAX_FREQ_DWIDTH>
void packCompressedFrame(hls::stream<IntVectorStream_dt<16, 1> >& packerMetaStream,
                         hls::stream<ap_uint<MAX_FREQ_DWIDTH> >& lzMetaStream,
                         hls::stream<ap_uint<16> >& bscMetaStream,
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
    // if (isSingleSegment == 0) {
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
        // printf("frame content size: %u\n", (uint16_t)fcs);
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

        // IntVectorStream_dt<8, 7> bitVector;
        // Write Frame header
        outVal.strobe = 4;
        // bitVector.strobe = 0;
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
        // Read all block meta data and add 3-byte block header
        auto litCnt = lzMetaStream.read();
        auto seqCnt = lzMetaStream.read();

        ap_uint<40> litSecHdr = 0;
        ap_uint<32> seqSecHdr = 0; //<1-3 bytes seq cnt><1 byte symCompMode>
        uint32_t blockSize = 0;
        ap_uint<16> streamSizes[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
         * <-Block Header-><------------------ Literal Section ---------------><-------------- Sequences Section
         * -------------->
         * 				   <Section_Header><HF Header><FSE Header + Bitstream><Num
         * Sequences><SymCompMode><FSE
         * Tables LL-OF-ML>
         * 					   5 bytes		   1 byte	 	128 bytes
         * 1-3
         * bytes
         * 1 byte		0-63 bytes
         * 	  3 bytes							134 bytes
         * Upto
         * 67 bytes
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
        seqSecHdr.range((8 * (seqBytes + 1)) - 1, (8 * seqBytes)) = (uint8_t)((2 << 6) + (2 << 4) + (2 << 2));
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
        // printf("origLitSize: %u, cmpLitSize: %u\n", (uint32_t)litSecHdr.range(21, 4), (uint32_t)litSecHdr.range(39,
        // 22));
        // calculate block size, by adding bytes needed by different headers
        // sequence fse bitstreams size already added to blockSize
        blockSize += (litSecHdrBCnt + (uint32_t)litSecHdr.range(3 + (2 * lsSzBits), 4 + lsSzBits)) + (seqBytes + 1);
        // Write Block header
        /*
         * Block_Header: 3 bytes
         * <Last_Block><Block_Type><Block_Size>
         *    bit 0		 bits 1-2    bits 3-23
         */
        // Block Type = 2 (Compressed Block), 1 always last block in frame
        ap_uint<24> blockHeader = (uint32_t)1 + (2 << 1);
        // printf("blockSize: %u, blockHeader: %u\n", blockSize, (uint8_t)blockHeader);
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
        // printf("Size of lit fse header bitstream: %u\n", (uint8_t)(streamSizes[0] + streamSizes[1]));
        bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = (uint8_t)(streamSizes[0] + streamSizes[1]);
        ++byteCount;
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
    // read literal fse header and bitstream
    write_lithd_fse_header:
        for (uint16_t i = 0; i < streamSizes[0]; i += 4) {
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
    write_lithd_fse_bitstream:
        for (uint16_t i = 0; i < streamSizes[1]; i += 4) {
#pragma HLS PIPELINE II = 1
            auto bsVal = bscBitstream.read();
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = bsVal.data[0];
            bitstream.range(((byteCount + 2) * 8) - 1, (byteCount + 1) * 8) = bsVal.data[1];
            bitstream.range(((byteCount + 3) * 8) - 1, (byteCount + 2) * 8) = bsVal.data[2];
            bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 3) * 8) = bsVal.data[3];
            byteCount += bsVal.strobe;
            // printf("strobe: %u\n", (uint8_t)bsVal.strobe);
            // for (int k = 0; k < bsVal.strobe; ++k) printf("%u. fse h bs: %u\n", i + k, (uint8_t)bsVal.data[k]);
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
        // printf("Cmp %u stream sizes: %u %u %u\n", hfStreamCnt, (uint16_t)streamSizes[3], (uint16_t)streamSizes[4],
        // (uint16_t)streamSizes[5]);
        // printf("Out bytes: %u, bytes in buffer: %u\n", outSize, byteCount);
        // for (int i = 0; i < byteCount; ++i) printf("%d.Have %u\n", i, (uint8_t)bitstream.range((i * 8) + 7, i * 8));
        if (hfStreamCnt > 1) {
            bitstream.range(((byteCount + 2) * 8) - 1, byteCount * 8) = streamSizes[3];
            bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 2) * 8) = streamSizes[4];
            // fill output register
            outVal.data[0] = bitstream.range(7, 0);
            outVal.data[1] = bitstream.range(15, 8);
            outVal.data[2] = bitstream.range(23, 16);
            outVal.data[3] = bitstream.range(31, 24);
            // printf("Wrote: %u %u %u %u\n", (uint8_t)outVal.data[0], (uint8_t)outVal.data[1], (uint8_t)outVal.data[2],
            // (uint8_t)outVal.data[3]);
            // send output
            outStream << outVal;
            // adjust buffer
            bitstream >>= 32;
            outSize += 4;
            bitstream.range(((byteCount + 2) * 8) - 1, byteCount * 8) = streamSizes[5];
            byteCount += 2;
        }
        // check if buffer full
        if (byteCount > 3) {
            // fill output register
            outVal.data[0] = bitstream.range(7, 0);
            outVal.data[1] = bitstream.range(15, 8);
            outVal.data[2] = bitstream.range(23, 16);
            outVal.data[3] = bitstream.range(31, 24);
            // printf("Wrote: %u %u %u %u\n", (uint8_t)outVal.data[0], (uint8_t)outVal.data[1], (uint8_t)outVal.data[2],
            // (uint8_t)outVal.data[3]);
            // send output
            outStream << outVal;
            // adjust buffer
            byteCount -= 4;
            bitstream >>= 32;
            outSize += 4;
        }
    // Write literal huffman bitstreams
    send_huf_lit_bitstreams:
        for (uint16_t i = 0; i < hfStrmSize; i += 4) {
#pragma HLS PIPELINE II = 1
            auto hbsVal = bscBitstream.read();
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = hbsVal.data[0];
            bitstream.range(((byteCount + 2) * 8) - 1, (byteCount + 1) * 8) = hbsVal.data[1];
            bitstream.range(((byteCount + 3) * 8) - 1, (byteCount + 2) * 8) = hbsVal.data[2];
            bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 3) * 8) = hbsVal.data[3];

            /*for (unsigned k = 0; k < bsVal.strobe; ++k) {
                printf("%u. hf bs: %u\n", i, (uint8_t)(hbsVal.data[k]));
            }*/
            byteCount += hbsVal.strobe;
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
    // write sequence section header
    send_seq_sec_hdr:
        for (uint8_t i = 0; i < seqBytes + 1; ++i) {
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
    // int k=0;
    // write sequence headers and bitstreams (write entire available bitstream)
    send_seq_fse_bitstreams:
        for (auto seqVal = bscBitstream.read(); seqVal.strobe > 0; seqVal = bscBitstream.read()) {
#pragma HLS PIPELINE II = 1
            bitstream.range(((byteCount + 1) * 8) - 1, byteCount * 8) = seqVal.data[0];
            bitstream.range(((byteCount + 2) * 8) - 1, (byteCount + 1) * 8) = seqVal.data[1];
            bitstream.range(((byteCount + 3) * 8) - 1, (byteCount + 2) * 8) = seqVal.data[2];
            bitstream.range(((byteCount + 4) * 8) - 1, (byteCount + 3) * 8) = seqVal.data[3];
            byteCount += seqVal.strobe;
            // printf("%d. sq strobe: %u\n", k++, (uint8_t)seqVal.strobe);
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
    }
    // dump strobe 0
    bscBitstream.read();
    // indicate end of data
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
        // printf("%c", (char)inAxiVal.data);
        outHlsStream << outVal;
    }
    uint8_t strb = countSetBits<c_keepDWidth>((int)(inAxiVal.keep));
    if (strb) { // write last byte if valid
        outVal.data[0] = inAxiVal.data;
        // printf("%c||End", (char)inAxiVal.data);
        outHlsStream << outVal;
    }
    outVal.strobe = 0;
    outHlsStream << outVal;
    // printf("\nSend EOS\n");
}

template <int OUT_DWIDTH>
void zstdHlsVectorStream2axiu(hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> >& hlsInStream,
                              hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
    constexpr uint8_t c_bytesInWord = OUT_DWIDTH / 8;
    ap_axiu<OUT_DWIDTH, 0, 0, 0> outVal;
    ap_uint<OUT_DWIDTH * 2> outReg;
    uint8_t bytesInReg = 0;
    // int oc = 0;
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
        // oc += inVal.strobe;
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
    // printf("outcnt: %d\n", oc);
}

} // details

/**
 * @brief This module compresses the input file read from input stream.
 *        It produces the ZSTD compressed data at the output stream.
 *
 * @tparam BLOCK_SIZE ZStd block size
 * @tparam WINDOW_SIZE LZ77 history size or Window size
 * @tparam PARALLEL_HUFFMAN Number of Huffman encoding units used
 *
 * @param inStream input stream
 * @param outStream output stream
 */
template <int BLOCK_SIZE, int LZWINDOW_SIZE, int PARALLEL_HUFFMAN = 8, int PARALLEL_LIT_STREAMS = 4, int MIN_MATCH = 3>
void zstdCompressCore(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    // zstd compression main module
    constexpr int c_freq_dwidth = maxBitsUsed(BLOCK_SIZE);
    constexpr int c_blen_dwidth = maxBitsUsed(c_freq_dwidth);

    hls::stream<IntVectorStream_dt<8, 1> > inBlockStream("inBlockStream");
    hls::stream<IntVectorStream_dt<16, 1> > packerMetaStream("packerMetaStream");
#pragma HLS STREAM variable = packerMetaStream depth = 16
    hls::stream<IntVectorStream_dt<8, 1> > litStream("litStream");
    hls::stream<IntVectorStream_dt<8, 1> > reverseLitStream("reverseLitStream");
#pragma HLS STREAM variable = reverseLitStream depth = 2048
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > seqStream("seqStream");
#pragma HLS STREAM variable = seqStream depth = 4096
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > reverseSeqStream("reverseSeqStream");
#pragma HLS STREAM variable = reverseSeqStream depth = 256
    hls::stream<DSVectorStream_dt<details::Sequence_dt<8>, 1> > seqCodeStream("seqCodeStream");
#pragma HLS STREAM variable = seqCodeStream depth = 256
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > litFreqStream("litFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > seqFreqStream("seqFreqStream");
#pragma HLS STREAM variable = seqFreqStream depth = 128
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > wghtFreqStream("wghtFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > freqStream("freqStream");
    hls::stream<ap_uint<c_freq_dwidth> > lzMetaStream("lzMetaStream");
    hls::stream<bool> rleFlagStream("rleFlagStream");

    hls::stream<ap_uint<16> > hufLitMetaStream("hufLitMetaStream");
#pragma HLS STREAM variable = hufLitMetaStream depth = 128
    hls::stream<IntVectorStream_dt<8, 2> > fseHeaderStream("fseHeaderStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseLitTableStream("fseLitTableStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseSeqTableStream("fseSeqTableStream");

    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hufCodeStream;
    hls::stream<IntVectorStream_dt<4, 1> > hufWeightStream("hufWeightStream");
#pragma HLS STREAM variable = hufWeightStream depth = 256
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hfEncodedLitStream;
#pragma HLS STREAM variable = hfEncodedLitStream depth = 128
    hls::stream<IntVectorStream_dt<8, 2> > hfLitBitstream("hfLitBitstream");
#pragma HLS STREAM variable = hfLitBitstream depth = 4096
    hls::stream<IntVectorStream_dt<8, 2> > litEncodedStream("litEncodedStream");
    hls::stream<IntVectorStream_dt<8, 4> > seqEncodedStream("seqEncodedStream"); // update with packed structure
    hls::stream<ap_uint<16> > bscMetaStream("bscMetaStream");
#pragma HLS STREAM variable = bscMetaStream depth = 128
    hls::stream<IntVectorStream_dt<8, 4> > bscBitstream("bscBitstream");
#pragma HLS STREAM variable = bscBitstream depth = 4096

#pragma HLS dataflow

    details::inputDistributer<BLOCK_SIZE>(inStream, inBlockStream, packerMetaStream);
    // Module-1
    // LZ77 compression of input blocks to get separate streams
    // for literals, sequences (litlen, metlen, offset), literal frequencies and sequences frequencies
    details::getLitSequences<BLOCK_SIZE, c_freq_dwidth>(inBlockStream, litStream, seqStream, litFreqStream,
                                                        seqFreqStream, rleFlagStream, lzMetaStream);

    // Module-2
    {
        // Buffer, reverse and break input literal stream into 4 streams of 1/4th size
        details::preProcessLitStream<BLOCK_SIZE, c_freq_dwidth, PARALLEL_LIT_STREAMS>(litStream, reverseLitStream);
        // Reverse sequences stream
        details::reverseSeq<BLOCK_SIZE, c_freq_dwidth, MIN_MATCH>(seqStream, reverseSeqStream, seqCodeStream);
        // generate hufffman tree and get codes-bitlens
        zstdTreegenStream<c_freq_dwidth, details::c_maxZstdHfBits>(litFreqStream, hufCodeStream, hufWeightStream,
                                                                   wghtFreqStream);
        // feed frequency data to fse table gen from literals and sequences
        details::frequencySequencer<c_freq_dwidth>(wghtFreqStream, seqFreqStream, freqStream);
        // generate FSE Tables for litlen, matlen, offset and literal-bitlen
        details::fseTableGen(freqStream, fseHeaderStream, fseLitTableStream, fseSeqTableStream);
    }
    // Module-3
    {
        // Huffman encoding of literal stream
        details::zstdHuffmanEncoder<details::c_maxZstdHfBits>(reverseLitStream, rleFlagStream, hufCodeStream,
                                                              hfEncodedLitStream, hufLitMetaStream);
        // HUffman bitstream packer
        details::zstdHuffBitPacker<details::c_maxZstdHfBits>(hfEncodedLitStream, hfLitBitstream);
        // FSE encoding of literals
        details::fseEncodeLitHeader(hufWeightStream, fseLitTableStream, litEncodedStream);
        // FSE encode sequences generated by lz77 compression
        details::fseEncodeSequences(reverseSeqStream, seqCodeStream, fseSeqTableStream, seqEncodedStream);
    }
    // Module-4
    // pack compressed data into single sequential block stream
    details::bitstreamCollector(hufLitMetaStream, hfLitBitstream, fseHeaderStream, litEncodedStream, seqEncodedStream,
                                bscMetaStream, bscBitstream);
    details::packCompressedFrame<BLOCK_SIZE, c_freq_dwidth>(packerMetaStream, lzMetaStream, bscMetaStream, bscBitstream,
                                                            outStream);
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
template <int IN_DWIDTH, int OUT_DWIDTH, int BLOCK_SIZE, int LZWINDOW_SIZE, int MIN_BLCK_SIZE = 1024>
void zstdCompressStreaming(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                           hls::stream<ap_axiu<OUT_DWIDTH, 0, 0, 0> >& outStream) {
#pragma HLS DATAFLOW
    hls::stream<IntVectorStream_dt<IN_DWIDTH, 1> > inZstdStream("inZstdStream");
    hls::stream<IntVectorStream_dt<8, OUT_DWIDTH / 8> > outCompressedStream("outCompressedStream");

#pragma HLS STREAM variable = inHlsStream depth = 8
#pragma HLS STREAM variable = outCompressedStream depth = 8

    // AXI 2 HLS Stream
    xf::compression::details::zstdAxiu2hlsStream<IN_DWIDTH>(inStream, inZstdStream);

    // Zlib Compress Stream IO Engine
    xf::compression::zstdCompressCore<BLOCK_SIZE, LZWINDOW_SIZE>(inZstdStream, outCompressedStream);

    // HLS 2 AXI Stream
    xf::compression::details::zstdHlsVectorStream2axiu<OUT_DWIDTH>(outCompressedStream, outStream);
}

} // compression
} // xf
#endif
