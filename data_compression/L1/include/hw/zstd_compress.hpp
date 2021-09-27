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

template <int BLOCK_SIZE, int MIN_BLK_SIZE = 128>
void inputBufferMinBlock(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                         hls::stream<bool>& rawBlockFlagStream,
                         hls::stream<IntVectorStream_dt<8, 1> >& outStream) {
    // write data and indicate if it should be raw block or not
    bool not_done = true;
    bool rawFlagNotSent = true;
    IntVectorStream_dt<8, 1> inVal;
stream_data:
    while (not_done) {
        // read data size in bytes
        uint32_t dataSize = 0;
        inVal.strobe = 1;
        rawFlagNotSent = true;
    send_in_block:
        while (inVal.strobe > 0 && dataSize < BLOCK_SIZE) {
#pragma HLS PIPELINE II = 1
            inVal = inStream.read();
            if (inVal.strobe > 0) {
                outStream << inVal;
                ++dataSize;
                // indicate if more data than minimum block size
                if (dataSize > MIN_BLK_SIZE && rawFlagNotSent) {
                    rawBlockFlagStream << false;
                    rawFlagNotSent = false;
                }
            }
        }
        if (dataSize > 0 && dataSize < 1 + MIN_BLK_SIZE) rawBlockFlagStream << true;
        // end of block for last block with data
        if (dataSize > 0 && inVal.strobe == 0) outStream << inVal;
        // end of block/file
        inVal.strobe = 0;
        outStream << inVal;
        // terminate condition
        not_done = (dataSize == BLOCK_SIZE);
    }
    // for end of files, value must be false
    rawBlockFlagStream << false;
}

template <int BLOCK_SIZE>
void __inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                        hls::stream<bool>& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                        hls::stream<IntVectorStream_dt<8, 1> >& outStrdStream,
                        hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Send input blocks for compression or raw block packer and metadata to block packer.
    IntVectorStream_dt<16, 1> metaVal;
    IntVectorStream_dt<8, 1> inVal;
stream_blocks:
    while (true) {
        uint32_t dataSize = 0;
        auto isRawBlock = rawBlockFlagStream.read();
    send_block:
        for (inVal = inStream.read(); inVal.strobe > 0; inVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
            if (!isRawBlock) outStream << inVal;
            outStrdStream << inVal;
            ++dataSize;
        }
        // End of block/file
        if (!isRawBlock) outStream << inVal;
        outStrdStream << inVal;
        // write meta data
        if (dataSize > 0) {
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
        } else {
            // strobe 0 for end of data, exit condition
            metaVal.strobe = 0;
            blockMetaStream << metaVal;
            break;
        }
    }
}

template <int BLOCK_SIZE, int MIN_BLK_SIZE = 128>
void inputDistributer(hls::stream<IntVectorStream_dt<8, 1> >& inStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStream,
                      hls::stream<IntVectorStream_dt<8, 1> >& outStrdStream,
                      hls::stream<IntVectorStream_dt<16, 1> >& blockMetaStream) {
    // Create blocks of size BLOCK_SIZE and send metadata to block packer.
    // Internal streams
    hls::stream<IntVectorStream_dt<8, 1> > intmDataStream("intmDataStream");
    hls::stream<bool> rawBlockFlagStream("rawBlockFlagStream");

#pragma HLS STREAM variable = intmDataStream depth = 256
#pragma HLS STREAM variable = rawBlockFlagStream depth = 4

#pragma HLS dataflow
    xf::compression::details::inputBufferMinBlock<BLOCK_SIZE, MIN_BLK_SIZE>(inStream, rawBlockFlagStream,
                                                                            intmDataStream);

    xf::compression::details::__inputDistributer<BLOCK_SIZE>(intmDataStream, rawBlockFlagStream, outStream,
                                                             outStrdStream, blockMetaStream);
}

void readfseS2Bram(uint8_t* fseHdrBuf,
                   uint8_t& fseHIdx,
                   uint16_t& hdrBsLen,
                   IntVectorStream_dt<8, 2>& fhVal,
                   hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream) {
#pragma HLS inline off
buffer_llofml_fsebs: // taking 1.3K LUTs
    for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPELINE II = 1
        fseHdrBuf[fseHIdx] = fhVal.data[0];
        fseHdrBuf[fseHIdx + 1] = fhVal.data[1];
        fseHIdx += fhVal.strobe;
        hdrBsLen += fhVal.strobe;
    }
}

template <int MAX_FREQ_DWIDTH, int DBYTES = 4, int HFBYTES = 2, class META_DT = ap_uint<MAX_FREQ_DWIDTH> >
void bytestreamCollector(hls::stream<META_DT>& lzMetaStream,
                         hls::stream<ap_uint<16> >& hfLitMetaStream,
                         hls::stream<IntVectorStream_dt<8, HFBYTES> >& hfLitBitstream,
                         hls::stream<IntVectorStream_dt<8, 2> >& fseHeaderStream,
                         hls::stream<IntVectorStream_dt<8, 2> >& litEncodedStream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& seqEncodedStream,
                         hls::stream<META_DT>& seqEncSizeStream,
                         hls::stream<META_DT>& bscMetaStream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& outStream) {
    // Collect encoded literals and sequences data and send ordered data to output
    uint8_t fseHdrBuf[72];
#pragma HLS ARRAY_RESHAPE variable = fseHdrBuf type = cyclic factor = 2 dim = 1
#pragma HLS BIND_STORAGE variable = fseHdrBuf type = ram_2p impl = lutram
//#pragma HLS ARRAY_PARTITION variable = fseHdrBuf complete
// int rc = 0;
bsCol_main:
    while (true) {
        IntVectorStream_dt<8, DBYTES> outVal;
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
        uint16_t hdrBsLen[3] = {0, 0, 0};
#pragma HLS ARRAY_PARTITION variable = hdrBsLen complete
        uint8_t fseHIdx = 0;
    // Buffer fse header bitstreams for litlen and offset
    buff_fse_header_bs:
        for (uint8_t i = 0; i < 3 && seqCnt > 0; ++i) {
            if (readFseHdr) fhVal = fseHeaderStream.read();
            readFseHdr = true;
            readfseS2Bram(fseHdrBuf, fseHIdx, hdrBsLen[i], fhVal, fseHeaderStream);
        }
        uint16_t hdrBsSize = 0;
        // Send FSE header for literal header
        if (readFseHdr) fhVal = fseHeaderStream.read();
    // readFseHdr = true;
    send_lit_fse_header:
        for (; fhVal.strobe > 0; fhVal = fseHeaderStream.read()) {
#pragma HLS PIPEINE II = 1
            outVal.data[0] = fhVal.data[0];
            outVal.data[1] = fhVal.data[1];
            hdrBsSize += fhVal.strobe;
            outVal.strobe = fhVal.strobe;
            outStream << outVal;
        }
        // send size of literal codes fse header
        bscMetaStream << hdrBsSize;

        // Buffer fse header bitstreams for matlen
        // fhVal.strobe = 0;
        // if (seqCnt > 0) fhVal = fseHeaderStream.read();
        // readfseS2Bram(fseHdrBuf, fseHIdx, hdrBsLen[2], fhVal, fseHeaderStream);

        // Send FSE encoded bitstream for literal header
        uint8_t litEncSize = 0;
    send_lit_fse_bitstream:
        for (fhVal = litEncodedStream.read(); fhVal.strobe > 0; fhVal = litEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = fhVal.data[0];
            outVal.data[1] = fhVal.data[1];
            litEncSize += fhVal.strobe;
            outVal.strobe = fhVal.strobe;
            outStream << outVal;
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
        // Send sizes of FSE headers for litlen, offsets and matlen
        bscMetaStream << hdrBsLen[0]; // litlen
        bscMetaStream << hdrBsLen[1]; // offset
        bscMetaStream << hdrBsLen[2]; // matlen
        // read and send size of sequences fse bitstream
        auto seqBsSize = ((seqCnt > 0) ? seqEncSizeStream.read() : (META_DT)0);
        bscMetaStream << seqBsSize;
    // All metadata sent now
    // send huffman bitstreams
    send_all_hf_bitstreams:
        for (uint8_t i = 0; i < litStreamCnt; ++i) {
            IntVectorStream_dt<8, HFBYTES> hfLitVal;
        //++rc;
        send_huf_lit_bitstream:
            for (hfLitVal = hfLitBitstream.read(); hfLitVal.strobe > 0; hfLitVal = hfLitBitstream.read()) {
#pragma HLS PIPELINE II = 1
                for (uint8_t k = 0; k < HFBYTES; ++k) {
#pragma HLS UNROLL
                    outVal.data[k] = hfLitVal.data[k];
                }
                outVal.strobe = hfLitVal.strobe;
                outStream << outVal;
            }
        }
    // Send FSE header for litlen, offsets and matlen from buffer
    send_llof_fse_header_bs:
        for (uint8_t i = 0; i < fseHIdx; i += 2) {
#pragma HLS PIPELINE II = 1
            outVal.data[0] = fseHdrBuf[i];
            outVal.data[1] = fseHdrBuf[i + 1];
            outVal.strobe = ((i < fseHIdx - 2) ? 2 : fseHIdx - i);
            outStream << outVal;
        }
        // send sequences bitstream
        outVal.strobe = 0;
        if (seqCnt > 0) outVal = seqEncodedStream.read();
    send_seq_fse_bitstream:
        for (; outVal.strobe > 0; outVal = seqEncodedStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
        }
        // end of block
        outVal.strobe = 0;
        outStream << outVal;
    }
    // dump end of data from remaining input streams
    hfLitBitstream.read();
    litEncodedStream.read();
    seqEncodedStream.read();
}

template <int BLOCK_SIZE,
          int MIN_BLK_SIZE,
          int MAX_FREQ_DWIDTH,
          int DBYTES = 4,
          class META_DT = ap_uint<MAX_FREQ_DWIDTH> >
void packCompressedFrame(hls::stream<IntVectorStream_dt<16, 1> >& packerMetaStream,
                         hls::stream<META_DT>& bscMetaStream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& bscBitstream,
                         hls::stream<IntVectorStream_dt<8, DBYTES> >& outStream) {
    // Collect encoded literals and sequences data and send formatted data to output
    constexpr bool isSingleSegment = 0; // set if using 1 block/frame
    bool blockRead = false;
    IntVectorStream_dt<8, DBYTES> outVal;
    ap_uint<8> frameHeaderBuf[14];
#pragma HLS ARRAY_PARTITION variable = frameHeaderBuf complete
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
    uint8_t windowLog = bitsUsed31((uint32_t)BLOCK_SIZE);
    uint32_t windowBase = (uint32_t)1 << windowLog;
    frameHeaderBuf[5].range(7, 3) = windowLog - 10;
    frameHeaderBuf[5].range(2, 0) = (uint8_t)((8 * (BLOCK_SIZE - windowBase)) >> windowLog);
zstd_frame_packer:
    while (true) {
        int outSize = 0;
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
        // fill output buffer
        outVal.strobe = 4;
        outVal.data[0] = fcs.range(7, 0);
        outVal.data[1] = fcs.range(15, 8);
        outVal.data[2] = fcs.range(23, 16);
        outVal.data[3] = 0;
        outStream << outVal;

        if (fcs > 255) {
            uint8_t c_fcsFlag = ((fcs < (64 * 1024)) ? 1 : 2);
            // write full fcs (to read as per fhIdx
            if (c_fcsFlag == 1) fcs -= 256;
            frameHeaderBuf[6] = fcs.range(7, 0);
            frameHeaderBuf[7] = fcs.range(15, 8);
            frameHeaderBuf[8] = fcs.range(23, 16);
            frameHeaderBuf[9] = 0;
            // set increment
            fhIdx += (1 << c_fcsFlag);
            // we have 1 block/frame, therefore single segment flag can be set
            // so that window size becomes equal to frame content size
            frameHeader.range(7, 6) = c_fcsFlag;
        } else {
            // do not write frame content size to header
        }
        frameHeaderBuf[4] = frameHeader;
    /*** End: Frame Content Size ***/
    // Now write frame header, pre-fixed with relevant meta data
    // Write the size of frame header and block size, this is not part of the format, it is for use in next module
    // Write Frame header
    send_frame_header:
        for (uint8_t i = 0; i < fhIdx; i += DBYTES) {
#pragma HLS PIPELINE II = 1
            for (uint8_t k = 0; k < DBYTES; ++k) {
#pragma HLS UNROLL
                outVal.data[k] = (i + k < fhIdx) ? frameHeaderBuf[i + k] : (ap_uint<8>)0;
            }
            outVal.strobe = ((i < fhIdx - DBYTES) ? DBYTES : (fhIdx - i));
            outStream << outVal;
            outSize += outVal.strobe;
        }
        // end of header
        outVal.strobe = 0;
        outStream << outVal;
        // skip stored block
        if (fcs < MIN_BLK_SIZE + 1) { // *** MIN_BLK_SIZE < 256 COMPULSORY ***
            // send strobe 0, to indicate end of block for frame data
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
        outVal.data[0] = blockHeader.range(7, 0);
        outVal.data[1] = blockHeader.range(15, 8);
        outVal.data[2] = blockHeader.range(23, 16);
        outVal.data[3] = 0;
        outVal.strobe = 3;
        outStream << outVal;
        outSize += outVal.strobe;
        // Write Block Content
        // Write Literal Section header
        if (DBYTES < 5) {
        write_lit_sec_hdr:
            for (uint8_t i = 0; i < litSecHdrBCnt; i += DBYTES) {
#pragma HLS PIPELINE II = 1
                for (uint8_t k = 0; k < DBYTES; ++k) {
#pragma HLS UNROLL
                    outVal.data[k] = litSecHdr.range(7 + (k * 8), k * 8);
                }
                outVal.strobe = ((i < litSecHdrBCnt - DBYTES) ? DBYTES : (litSecHdrBCnt - i));
                outStream << outVal;
                litSecHdr >>= (DBYTES * 8);
                outSize += outVal.strobe;
            }
        } else {
            for (uint8_t k = 0; k < 5; ++k) { // litSecHdr is 40-bits => 5 bytes
#pragma HLS UNROLL
                outVal.data[k] = litSecHdr.range(7 + (k * 8), k * 8);
            }
            outVal.strobe = litSecHdrBCnt;
            outStream << outVal;
            outSize += litSecHdrBCnt;
        }
        // Write fse header
        outVal.data[0] = (uint8_t)(streamSizes[0] + streamSizes[1]);
        outVal.strobe = 1;
        outStream << outVal;
        outSize += outVal.strobe;
    // read literal fse header, bitstream and literal huffman bitstreams
    write_lit_sec:
        for (uint8_t k = 0; k < 3; ++k) {
            uint32_t itrSize = ((k < 2) ? (uint32_t)streamSizes[k] : (uint32_t)hfStrmSize);
        write_lithd_fse_data:
            for (uint16_t i = 0; i < itrSize; i += outVal.strobe) {
#pragma HLS PIPELINE II = 1
                outVal = bscBitstream.read();
                outStream << outVal;
                outSize += outVal.strobe;
            }
            // Write jump table and huffman streams
            if (k == 1 && hfStreamCnt > 1) {
                outVal.data[0] = streamSizes[3].range(7, 0);
                outVal.data[1] = streamSizes[3].range(15, 8);
                outVal.data[2] = streamSizes[4].range(7, 0);
                outVal.data[3] = streamSizes[4].range(15, 8);
                if (DBYTES < 8) {
                    outVal.strobe = 4;
                    outStream << outVal;
                    outSize += outVal.strobe;
                    outVal.data[0] = streamSizes[5].range(7, 0);
                    outVal.data[1] = streamSizes[5].range(15, 8);
                    outVal.strobe = 2;
                    outStream << outVal;
                } else {
                    outVal.data[4] = streamSizes[5].range(7, 0);
                    outVal.data[5] = streamSizes[5].range(15, 8);
                    outVal.strobe = 6;
                    outStream << outVal;
                }
                outSize += outVal.strobe;
            }
        }
        // write sequence section header
        // seqSecHdr can be max 32-bits and at least 8-bits
        outVal.data[0] = seqSecHdr.range(7, 0);
        outVal.data[1] = seqSecHdr.range(15, 8);
        outVal.data[2] = seqSecHdr.range(23, 16);
        outVal.data[3] = seqSecHdr.range(31, 24);
        outVal.strobe = seqBytes;
        outStream << outVal;
        outSize += outVal.strobe;
    // write sequence headers and bitstreams(entire remaining 16-bit and 32-bit input bitstreams)
    send_seq_fse_hdrs:
        for (outVal = bscBitstream.read(); outVal.strobe > 0; outVal = bscBitstream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
            outSize += outVal.strobe;
        }
        // send strobe 0
        outVal.strobe = 0;
        outStream << outVal;
        // printf("CMP out size: %d\n", outSize);
    }
}

template <int IN_DWIDTH, int STB_WIDTH>
void skipPassRawBlock(hls::stream<ap_uint<IN_DWIDTH + STB_WIDTH> >& inRawStream,
                      hls::stream<ap_uint<2> >& inStbFlagStream,
                      hls::stream<ap_uint<IN_DWIDTH + STB_WIDTH> >& outRawStream,
                      hls::stream<ap_uint<2> >& outStbFlagStream1,
                      hls::stream<ap_uint<2> >& outStbFlagStream2) {
    // read and dump the raw block data when not needed
    ap_uint<IN_DWIDTH + STB_WIDTH> inVal;
    bool stbFlagStrobe = 1;
    ap_uint<2> outFlagVal = 1; // <stb Flag 1-bit><strobe 1-bit>
outer_rbk_loop:
    while (true) {
        inVal = inRawStream.read();
        if (inVal.range(STB_WIDTH - 1, 0) == 0) break;
        auto inFlagVal = inStbFlagStream.read();
        bool isRawBlk = inFlagVal.range(1, 1);
        stbFlagStrobe = inFlagVal.range(0, 0);
        // if last block is present (as control reached here) and strobe for stbFlagStream is 0
        // therefore it is minimum block case, so set isRawBlk flag
        if (stbFlagStrobe == 0) isRawBlk = 1;
        outFlagVal.range(1, 1) = (ap_uint<1>)isRawBlk;
        outStbFlagStream1 << outFlagVal;
        outStbFlagStream2 << outFlagVal;
    skip_pass_raw_block_loop:
        for (; inVal.range(STB_WIDTH - 1, 0) > 0; inVal = inRawStream.read()) {
#pragma HLS PIPELINE II = 1
            if (isRawBlk) outRawStream << inVal;
        }
        // send strobe 0 for end of block
        if (isRawBlk) outRawStream << inVal;
    }
    if (stbFlagStrobe > 0) inStbFlagStream.read();
    // end of data
    inVal = 0;
    outRawStream << inVal;
    outFlagVal = 0;
    outStbFlagStream1 << outFlagVal;
    outStbFlagStream2 << outFlagVal;
}

template <int DBYTES = 4>
void skipPassCmpBlock(hls::stream<IntVectorStream_dt<8, DBYTES> >& inCmpStream,
                      hls::stream<ap_uint<2> >& rawBlockFlagStream,
                      hls::stream<IntVectorStream_dt<8, DBYTES> >& outCmpStream) {
    IntVectorStream_dt<8, DBYTES> outVal;
stream_skip_cmp_blk:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
    stream_cmp_frm_hdr_blk:
        for (auto i = 0; i < 2; ++i) {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpStream.read(); outVal.strobe > 0; outVal = inCmpStream.read()) {
#pragma HLS PIPELINE II = 1
                if (isRawBlk == false || i == 0) outCmpStream << outVal;
            }
            outVal.strobe = 0;
            if (isRawBlk == false || i == 0) outCmpStream << outVal;
        }
    }
}

void streamCmpStrdFrame(hls::stream<IntVectorStream_dt<8, 4> >& inRawBStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& inCmpBStream,
                        hls::stream<ap_uint<2> >& rawBlockFlagStream,
                        hls::stream<IntVectorStream_dt<8, 4> >& outStream) {
    IntVectorStream_dt<8, 4> outVal;
    ap_uint<24> strdBlockHeader = 1; // bit-0 = 1, indicating last block, bits 1-2 = 0, indicating raw block
stream_cmp_file:
    while (true) {
        auto rawBlkFlag = rawBlockFlagStream.read();
        bool isRawBlk = rawBlkFlag.range(1, 1);
        bool rwbStrobe = rawBlkFlag.range(0, 0);
        if (rwbStrobe == 0) break;
        // read frame content size
        outVal = inCmpBStream.read();
        strdBlockHeader.range(10, 3) = (uint8_t)outVal.data[0];
        strdBlockHeader.range(18, 11) = (uint8_t)outVal.data[1];
        strdBlockHeader.range(23, 19) = (uint8_t)outVal.data[2];
    // write the frame header, written for each block of input data as stated in its meta data
    write_frame_header:
        for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
            outStream << outVal;
        }
        if (isRawBlk) {
            // Write stored block header
            outVal.data[0] = strdBlockHeader.range(7, 0);
            outVal.data[1] = strdBlockHeader.range(15, 8);
            outVal.data[2] = strdBlockHeader.range(23, 16);
            outVal.strobe = 3;
            outStream << outVal;
        write_raw_blk_data:
            for (auto rbVal = inRawBStream.read(); rbVal.strobe > 0; rbVal = inRawBStream.read()) {
#pragma HLS PIPELINE II = 1
                outStream << rbVal;
            }
        } else {
        write_or_skip_cmp_blk_data:
            for (outVal = inCmpBStream.read(); outVal.strobe > 0; outVal = inCmpBStream.read()) {
#pragma HLS PIPELINE II = 1
                outStream << outVal;
            }
        }
    }
    // dump last strobe 0
    inRawBStream.read();
    // end of file
    outVal.strobe = 0;
    outStream << outVal;
}

template <int IN_DWIDTH>
void zstdAxiu2hlsStream(hls::stream<ap_axiu<IN_DWIDTH, 0, 0, 0> >& inStream,
                        hls::stream<IntVectorStream_dt<8, IN_DWIDTH / 8> >& outHlsStream) {
    constexpr uint8_t c_keepDWidth = IN_DWIDTH / 8;
    IntVectorStream_dt<8, c_keepDWidth> outVal;
    outVal.strobe = c_keepDWidth;
    auto inAxiVal = inStream.read();
axi_to_hls:
    for (; inAxiVal.last == false; inAxiVal = inStream.read()) {
#pragma HLS PIPELINE II = 1
        for (uint8_t i = 0; i < c_keepDWidth; ++i) {
#pragma HLS UNROLL
            outVal.data[i] = inAxiVal.data.range((i * 8) + 7, i * 8);
        }
        outHlsStream << outVal;
    }
    uint8_t strb = countSetBits<c_keepDWidth>((int)(inAxiVal.keep));
    if (strb) { // write last byte if valid
        for (uint8_t i = 0; i < c_keepDWidth; ++i) {
#pragma HLS UNROLL
            outVal.data[i] = inAxiVal.data.range((i * 8) + 7, i * 8);
        }
        outVal.strobe = strb;
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
    uint32_t outCnt = 0;
hls_to_axi:
    for (auto inVal = hlsInStream.read(); inVal.strobe > 0; inVal = hlsInStream.read()) {
#pragma HLS PIPELINE II = 1
        // Write data to output register
        for (uint8_t i = 0; i < c_bytesInWord; ++i) {
#pragma HLS UNROLL
            uint16_t idx = (bytesInReg + i) * 8;
            outReg.range(idx + 7, idx) = inVal.data[i];
        }
        bytesInReg += inVal.strobe;
        outCnt += inVal.strobe;
        if (bytesInReg > c_bytesInWord - 1) {
            outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
            outStream << outVal;
            outReg >>= OUT_DWIDTH;
            bytesInReg -= c_bytesInWord;
        }
    }
    if (bytesInReg) {
        outVal.keep = (((uint16_t)1 << bytesInReg) - 1);
        outVal.data = outReg.range(OUT_DWIDTH - 1, 0);
        outStream << outVal;
    }
    // send eos
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
    constexpr uint32_t c_freq_dwidth = maxBitsUsed(BLOCK_SIZE);
    constexpr uint32_t c_dataUpSDepth = BLOCK_SIZE / 8;
    constexpr uint32_t c_hfLitStreamDepth = BLOCK_SIZE / 2;
    constexpr uint32_t c_seqBlockDepth = BLOCK_SIZE / 8;

    // Internal streams
    hls::stream<IntVectorStream_dt<8, 1> > inBlockStream("inBlockStream");
    hls::stream<IntVectorStream_dt<8, 1> > strdBlockStream("strdBlockStream");
    hls::stream<IntVectorStream_dt<8, 4> > strdDsBlockStream("strdDsBlockStream");
    hls::stream<IntVectorStream_dt<16, 1> > packerMetaStream("packerMetaStream");
    hls::stream<ap_uint<68> > rawUpsizedFinalStream("rawUpsizedFinalStream");
    hls::stream<ap_uint<68> > rwbUpsizedStream("rwbUpsizedStream"); // 1 URAM
#pragma HLS STREAM variable = inBlockStream depth = 16
#pragma HLS STREAM variable = strdBlockStream depth = 16
#pragma HLS STREAM variable = strdDsBlockStream depth = 16
#pragma HLS STREAM variable = packerMetaStream depth = 16
#pragma HLS STREAM variable = rawUpsizedFinalStream depth = 16
#pragma HLS STREAM variable = rwbUpsizedStream depth = c_dataUpSDepth
#pragma HLS BIND_STORAGE variable = rwbUpsizedStream type = FIFO impl = URAM
#pragma HLS BIND_STORAGE variable = rawUpsizedFinalStream type = FIFO impl = LUTRAM

    hls::stream<IntVectorStream_dt<8, 1> > litStream("litStream");
    hls::stream<IntVectorStream_dt<8, 1> > reverseLitStream("reverseLitStream");
    hls::stream<IntVectorStream_dt<8, 1> > dszLitStream("litStream");
    hls::stream<ap_uint<68> > litUpsizedStream("litUpsizedStream"); // 1 URAM
#pragma HLS STREAM variable = litStream depth = 64
#pragma HLS STREAM variable = reverseLitStream depth = 16
#pragma HLS STREAM variable = dszLitStream depth = 8
#pragma HLS STREAM variable = litUpsizedStream depth = c_dataUpSDepth
#pragma HLS BIND_STORAGE variable = litUpsizedStream type = FIFO impl = URAM

    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > seqStream("seqStream");
    hls::stream<DSVectorStream_dt<details::Sequence_dt<c_freq_dwidth>, 1> > reverseSeqStream(
        "reverseSeqStream"); // 1 URAM
#pragma HLS STREAM variable = seqStream depth = 32
// 4K depth needed to keep reading input sequences even if previous block decoding waits for fse table generation
#pragma HLS STREAM variable = reverseSeqStream depth = 4096

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > litFreqStream("litFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > seqFreqStream("seqFreqStream");
#pragma HLS STREAM variable = litFreqStream depth = 16
#pragma HLS STREAM variable = seqFreqStream depth = 128

    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > wghtFreqStream("wghtFreqStream");
    hls::stream<IntVectorStream_dt<c_freq_dwidth, 1> > freqStream("freqStream");
#pragma HLS STREAM variable = wghtFreqStream depth = 8
#pragma HLS STREAM variable = freqStream depth = 128

    hls::stream<ap_uint<c_freq_dwidth> > lzMetaStream("lzMetaStream");
    hls::stream<bool> rleFlagStream("rleFlagStream");
    hls::stream<ap_uint<2> > rwbFlagStream("rwbFlagStream");
    hls::stream<ap_uint<2> > rwbFinalFlagStream1("rwbFinalFlagStream1");
    hls::stream<ap_uint<2> > rwbFinalFlagStream2("rwbFinalFlagStream2");
    hls::stream<ap_uint<c_freq_dwidth> > litCntStream("litCntStream");
#pragma HLS STREAM variable = lzMetaStream depth = 16
#pragma HLS STREAM variable = rleFlagStream depth = 4
#pragma HLS STREAM variable = rwbFlagStream depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream1 depth = 4
#pragma HLS STREAM variable = rwbFinalFlagStream2 depth = 4
#pragma HLS STREAM variable = litCntStream depth = 8

    hls::stream<IntVectorStream_dt<8, 2> > fseHeaderStream("fseHeaderStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseLitTableStream("fseLitTableStream");
    hls::stream<IntVectorStream_dt<36, 1> > fseSeqTableStream("fseSeqTableStream");
#pragma HLS STREAM variable = fseHeaderStream depth = 128
#pragma HLS STREAM variable = fseLitTableStream depth = 8
#pragma HLS STREAM variable = fseSeqTableStream depth = 8

    hls::stream<ap_uint<16> > hufLitMetaStream("hufLitMetaStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hufCodeStream;
    hls::stream<IntVectorStream_dt<4, 1> > hufWeightStream("hufWeightStream");
    hls::stream<DSVectorStream_dt<HuffmanCode_dt<details::c_maxZstdHfBits>, 1> > hfEncodedLitStream;
    hls::stream<IntVectorStream_dt<8, 2> > hfLitBitstream("hfLitBitstream");
#pragma HLS STREAM variable = hufLitMetaStream depth = 128
#pragma HLS STREAM variable = hufCodeStream depth = 8
#pragma HLS STREAM variable = hufWeightStream depth = 8
#pragma HLS STREAM variable = hfEncodedLitStream depth = 256
#pragma HLS STREAM variable = hfLitBitstream depth = c_hfLitStreamDepth

    hls::stream<IntVectorStream_dt<8, 2> > litEncodedStream("litEncodedStream");
    hls::stream<ap_uint<c_freq_dwidth> > seqEncSizeStream("seqEncSizeStream");
    hls::stream<ap_uint<68> > seqEncodedStream("seqEncodedStream");
    hls::stream<IntVectorStream_dt<8, 4> > seqEncodedDszStream("seqEncodedDszStream");
#pragma HLS STREAM variable = litEncodedStream depth = 128
#pragma HLS STREAM variable = seqEncSizeStream depth = 4
#pragma HLS STREAM variable = seqEncodedDszStream depth = 8
#pragma HLS STREAM variable = seqEncodedStream depth = c_seqBlockDepth
#pragma HLS BIND_STORAGE variable = seqEncodedStream type = FIFO impl = URAM

    hls::stream<ap_uint<16> > bscMetaStream("bscMetaStream");
    hls::stream<IntVectorStream_dt<8, 4> > bsc32Bitstream("bsc32Bitstream");
#pragma HLS STREAM variable = bscMetaStream depth = 128
#pragma HLS STREAM variable = bsc32Bitstream depth = 256

    hls::stream<IntVectorStream_dt<8, 4> > cmpFrameStream("cmpFrameStream");
    hls::stream<IntVectorStream_dt<8, 4> > cmpFrameFinalStream("cmpFrameFinalStream");
#pragma HLS STREAM variable = cmpFrameStream depth = 64
#pragma HLS STREAM variable = cmpFrameFinalStream depth = 16

#pragma HLS dataflow

    // Module-1: Input reading and LZ77 compression
    {
        details::inputDistributer<BLOCK_SIZE, MIN_BLCK_SIZE>(inStream, inBlockStream, strdBlockStream,
                                                             packerMetaStream);
        // Upsize raw data for raw block
        details::simpleStreamUpsizer<8, 64, 4>(strdBlockStream, rwbUpsizedStream);
        // LZ77 compression of input blocks to get separate streams
        // for literals, sequences (litlen, metlen, offset), literal frequencies and sequences frequencies
        details::getLitSequences<BLOCK_SIZE, c_freq_dwidth>(inBlockStream, litStream, seqStream, litFreqStream,
                                                            seqFreqStream, rleFlagStream, rwbFlagStream, lzMetaStream,
                                                            litCntStream);
        details::skipPassRawBlock<64, 4>(rwbUpsizedStream, rwbFlagStream, rawUpsizedFinalStream, rwbFinalFlagStream1,
                                         rwbFinalFlagStream2);
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
        // Huffman bitstream packer
        details::zstdHuffBitPacker<details::c_maxZstdHfBits>(hfEncodedLitStream, hfLitBitstream);
        // FSE encoding of literals
        details::fseEncodeLitHeader(hufWeightStream, fseLitTableStream, litEncodedStream);
        // FSE encode sequences generated by lz77 compression
        details::fseEncodeSequences(reverseSeqStream, fseSeqTableStream, seqEncodedStream, seqEncSizeStream);
    }
    // Module-4: Output block and frame packing
    {
        // Buffer in stream URAM and downsize fse encoded sequence bitstream
        details::bufferDownsizerVec<64, 32, 4>(seqEncodedStream, seqEncodedDszStream);
        // collect data from different input byte streams and output 2 continuous streams
        details::bytestreamCollector<c_freq_dwidth>(lzMetaStream, hufLitMetaStream, hfLitBitstream, fseHeaderStream,
                                                    litEncodedStream, seqEncodedDszStream, seqEncSizeStream,
                                                    bscMetaStream, bsc32Bitstream);
        // pack compressed data into single sequential block stream
        details::packCompressedFrame<BLOCK_SIZE, MIN_BLCK_SIZE, c_freq_dwidth>(packerMetaStream, bscMetaStream,
                                                                               bsc32Bitstream, cmpFrameStream);
        details::skipPassCmpBlock(cmpFrameStream, rwbFinalFlagStream1, cmpFrameFinalStream);
        // Buffer in stream URAM and downsize raw block data
        details::bufferDownsizerVec<64, 32, 4>(rawUpsizedFinalStream, strdDsBlockStream);
        // Output compressed or raw block based on input flag stream
        details::streamCmpStrdFrame(strdDsBlockStream, cmpFrameFinalStream, rwbFinalFlagStream2, outStream);
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
