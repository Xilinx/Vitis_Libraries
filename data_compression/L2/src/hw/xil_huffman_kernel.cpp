/*
 * (c) Copyright 2019 Xilinx, Inc. All rights reserved.
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
#include "huffman_kernel.hpp"

const int c_gmem_burst_size = (2 * GMEM_BURST_SIZE);

#if (H_COMPUTE_UNIT == 1)
namespace cu1
#elif (H_COMPUTE_UNIT == 2)
namespace cu2
#endif
{
void bitPacking(hls::stream<uint16_t>& inStream,
                hls::stream<uint8_t>& inStreamSize,
                hls::stream<uintOutV_t>& outStream,
                hls::stream<uint16_t>& outStreamSize) {
    ap_uint<64> localBits = 0;
    uint32_t localBits_idx = 0;
    bool flag_run = false;

bitpack:
    for (uint8_t size = inStreamSize.read(); size != 0; size = inStreamSize.read()) {
#pragma HLS PIPELINE II = 1
        localBits.range(size + localBits_idx - 1, localBits_idx) = inStream.read();
        localBits_idx += size;

        if (localBits_idx >= 16) {
            ap_uint<16> pack_byte = 0;
            pack_byte = localBits.range(15, 0);
            localBits >>= 16;
            localBits_idx -= 16;
            outStreamSize << 2;
            outStream << pack_byte;
        }
        flag_run = true;
    }

    if (flag_run == false) {
        outStreamSize << 0;
        return;
    }

    int leftBits = localBits_idx % 8;
    int val = 0;

    if (leftBits) {
        // 3 bit header
        localBits.range(localBits_idx + 3 - 1, localBits_idx) = 0;
        localBits_idx += 3;

        leftBits = localBits_idx % 8;

        if (leftBits != 0) {
            val = BIT - leftBits;
            localBits.range(localBits_idx + val - 1, localBits_idx) = 0;
            localBits_idx += val;
        }

        if (localBits_idx >= 16) {
            ap_uint<16> pack_byte = 0;
            pack_byte = localBits.range(15, 0);
            localBits >>= 16;
            localBits_idx -= 16;
            outStreamSize << 2;
            outStream << pack_byte;
        }

        // Zero bytes and complement as per type0 z_sync_flush
        localBits.range(localBits_idx + 8 - 1, localBits_idx) = 0x00;
        localBits.range(localBits_idx + 16 - 1, localBits_idx + 8) = 0x00;
        localBits.range(localBits_idx + 24 - 1, localBits_idx + 16) = 0xff;
        localBits.range(localBits_idx + 32 - 1, localBits_idx + 24) = 0xff;
        localBits_idx += 32;

    } else {
        // Zero bytes and complement as per type0 z_sync_flush
        localBits.range(localBits_idx + 8 - 1, localBits_idx) = 0x00;
        localBits.range(localBits_idx + 16 - 1, localBits_idx + 8) = 0x00;
        localBits.range(localBits_idx + 24 - 1, localBits_idx + 16) = 0x00;
        localBits.range(localBits_idx + 32 - 1, localBits_idx + 24) = 0xff;
        localBits.range(localBits_idx + 40 - 1, localBits_idx + 32) = 0xff;
        localBits_idx += 40;
    }

    for (uint32_t i = 0; i < localBits_idx; i += 8) {
        uint16_t pack_byte = 0;
        pack_byte = localBits.range(7, 0);
        localBits >>= 8;
        outStreamSize << 1;
        outStream << pack_byte;
    }

    outStreamSize << 0;
}

void dynamicHuffman(hls::stream<encodedV_dt>& inStream,
                    hls::stream<uint16_t>& outStream,
                    hls::stream<uint8_t>& outStreamSize,
                    uint32_t input_size,
                    hls::stream<uint32_t>& inStreamLCodes,
                    hls::stream<uint32_t>& inStreamLBlen,
                    hls::stream<uint32_t>& inStreamDCodes,
                    hls::stream<uint32_t>& inStreamDBlen,
                    hls::stream<uint32_t>& inStreamBLCodes,
                    hls::stream<uint32_t>& inStreamBLBlen,
                    hls::stream<uint32_t>& inStreamMaxCode) {
    if (input_size == 0) {
        outStreamSize << 0;
        return;
    }

    uint8_t extra_lbits[LENGTH_CODES] /* extra bits for each length code */
        = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

    uint8_t extra_dbits[D_CODES] /* extra bits for each distance code */
        = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    uint16_t litmtree_codes[LTREE_SIZE];
    uint8_t litmtree_blen[LTREE_SIZE];

    uint16_t distree_codes[DTREE_SIZE];
    uint8_t dtree_blen[DTREE_SIZE];

    uint16_t bitlentree_codes[BLTREE_SIZE];
    uint8_t bitlentree_blen[BLTREE_SIZE];

    for (uint16_t i = 0; i < LTREE_SIZE; i++) {
        litmtree_codes[i] = inStreamLCodes.read();
        litmtree_blen[i] = inStreamLBlen.read();
    }

    for (uint16_t i = 0; i < DTREE_SIZE; i++) {
        distree_codes[i] = inStreamDCodes.read();
        dtree_blen[i] = inStreamDBlen.read();
    }

    for (uint16_t i = 0; i < BLTREE_SIZE; i++) {
        bitlentree_codes[i] = inStreamBLCodes.read();
        bitlentree_blen[i] = inStreamBLBlen.read();
    }

    uint16_t lit_max_code = inStreamMaxCode.read();
    uint16_t dst_max_code = inStreamMaxCode.read();
    uint16_t bl_max_code = inStreamMaxCode.read();

    uint8_t bl_order[BL_CODES] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    // This holds output stream data
    uintOutV_t tmpOut;

    uint32_t out_cntr = 0;
    uint32_t total_outCntr = 0;

    uint32_t size = (input_size - 1) / 4 + 1;
    // printf("input_size %d size %d \n", input_size, size);
    uint32_t outByteCnt = 0;
    uint8_t length = 0;
    uint8_t value1, value2;

    ap_uint<64> localBits = 0;
    uint32_t localBits_idx = 0;

    // TODO: Remove this 5 to 4 for final version
    uint8_t start_of_block = 4;
    uint8_t start_of_block_len = 3;

    // Send Dynamic Huffman Block flag ------ Step 1

    // All the blocks run in kernel are no end of blocks
    // Add a special end of block in host with block rep = 3.
    outStream << start_of_block;
    outStreamSize << start_of_block_len;

    // Send Dynamic Huffman Trees Literas/Distances --------- Step 2

    // lcodes
    outStream << ((lit_max_code + 1) - 257);
    outStreamSize << 5;

    // dcodes
    outStream << ((dst_max_code + 1) - 1);
    outStreamSize << 5;

    // blcodes
    outStream << ((bl_max_code + 1) - 4);
    outStreamSize << 4;

// Send BL length data
send_bltree:
    for (int rank = 0; rank < bl_max_code + 1; rank++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
#pragma HLS PIPELINE II = 1
        outStream << bitlentree_blen[bl_order[rank]];
        outStreamSize << 3;
    } // BL data copy loop

    // **********************************
    //        Send Literal Tree Start
    // **********************************

    int prevlen = -1;               // Last emitted Length
    int curlen = 0;                 // Length of Current Code
    int nextlen = litmtree_blen[0]; // Length of next code
    int count = 0;
    int max_count = 7; // Max repeat count
    int min_count = 4; // Min repeat count

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

send_ltree:
    for (int n = 0; n <= lit_max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 286 max = 286
        curlen = nextlen;
        nextlen = litmtree_blen[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
        lit_cnt:
            for (uint8_t cnt = count; cnt != 0; --cnt) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
#pragma HLS PIPELINE II = 1
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
            }
            count = 0;

        } else if (curlen != 0) {
            if (curlen != prevlen) {
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                // printf("In curlen != prevlen code %d blen %d\n", code, blen);
                outStream << code;
                outStreamSize << blen;
                count--;
            }
            uint16_t blen = bitlentree_blen[REP_3_6];
            uint16_t code = bitlentree_codes[REP_3_6];
            outStream << code;
            outStreamSize << blen;

            outStream << count - 3;
            outStreamSize << 2;

        } else if (count <= 10) {
            uint16_t blen = bitlentree_blen[REPZ_3_10];
            uint16_t code = bitlentree_codes[REPZ_3_10];
            outStream << code;
            outStreamSize << blen;

            outStream << count - 3;
            outStreamSize << 3;

        } else {
            outStream << bitlentree_codes[REPZ_11_138];
            outStreamSize << bitlentree_blen[REPZ_11_138];

            outStream << count - 11;
            outStreamSize << 7;
        }

        count = 0;
        prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }

    } // send_ltree for-loop ends here

    // ************************************
    //        Send literal tree end
    // ************************************

    // **********************************
    //        Send Distance Tree Start
    // **********************************

    prevlen = -1;            // Last emitted Length
    curlen = 0;              // Length of Current Code
    nextlen = dtree_blen[0]; // Length of next code
    count = 0;
    max_count = 7; // Max repeat count
    min_count = 4; // Min repeat count

    // printf("dtree_blen_0 %d dtree_blen_1 %d\n", dtree_blen[0], dtree_blen[1]);

    if (nextlen == 0) {
        max_count = 138;
        min_count = 3;
    }

send_dtree:
    for (int n = 0; n <= dst_max_code; n++) {
#pragma HLS LOOP_TRIPCOUNT min = 64 max = 64
        // printf("In distance loop \n");
        curlen = nextlen;
        nextlen = dtree_blen[n + 1];

        if (++count < max_count && curlen == nextlen) {
            continue;
        } else if (count < min_count) {
        // printf("In count mincount path \n");
        dist_cnt:
            for (uint8_t cnt = count; cnt != 0; --cnt) {
#pragma HLS LOOP_TRIPCOUNT min = 10 max = 10
#pragma HLS PIPELINE II = 1
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
            }
            count = 0;
        } else if (curlen != 0) {
            if (curlen != prevlen) {
                uint16_t blen = bitlentree_blen[curlen];
                uint16_t code = bitlentree_codes[curlen];
                outStream << code;
                outStreamSize << blen;
                count--;
            }
            uint16_t blen = bitlentree_blen[REP_3_6];
            uint16_t code = bitlentree_codes[REP_3_6];
            outStream << code;
            outStreamSize << blen;
            outStream << count - 3;
            outStreamSize << 2;

        } else if (count <= 10) {
            uint16_t blen = bitlentree_blen[REPZ_3_10];
            uint16_t code = bitlentree_codes[REPZ_3_10];
            outStream << code;
            outStreamSize << blen;

            outStream << count - 3;
            outStreamSize << 3;
        } else {
            uint16_t blen = bitlentree_blen[REPZ_11_138];
            uint16_t code = bitlentree_codes[REPZ_11_138];
            outStream << code;
            outStreamSize << blen;
            outStream << count - 11;
            outStreamSize << 7;
        }
        count = 0;
        prevlen = curlen;
        if (nextlen == 0) {
            max_count = 138, min_count = 3;
        } else if (curlen == nextlen) {
            max_count = 6, min_count = 3;
        } else {
            max_count = 7, min_count = 4;
        }
    } // send_dtree for-loop ends here

// ************************************
//        Send distance tree end
// ************************************

// printf("Compressed Data \n");
// ************************************************************* //
//                      Trees are sent                           //
// ************************************************************* //

#include "gzip_tables.h"

    uint8_t next_state = WRITE_TOKEN;
    uint8_t tCh = 0;
    uint8_t tLen = 0;
    uint16_t tOffset = 0;

    bool flag_lit_rep = false;
    bool flag_ml_extra = false;
    bool flag_ml_dist_rep = false;

    uint16_t lcode = 0;
    uint16_t dcode = 0;
    uint32_t lextra = 0;
    uint32_t dextra = 0;

    bool flag_out = false;
huffman_loop:
    for (uint32_t inIdx = 0; (inIdx < size) || (next_state != WRITE_TOKEN);) {
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
        ap_uint<16> outValue;
        if (next_state == WRITE_TOKEN) {
            encoded_dt tmpEncodedValue = inStream.read();
            inIdx++;
            tCh = tmpEncodedValue.range(7, 0);
            tLen = tmpEncodedValue.range(15, 8);
            tOffset = tmpEncodedValue.range(31, 16);

            dcode = d_code(tOffset, dist_code);
            lcode = length_code[tLen];

            if (tLen > 0) {
                next_state = ML_DIST_REP;
            } else {
                next_state = LIT_REP;
            }
        } else if (next_state == ML_DIST_REP) {
            // printf("In ML_DIST_REP %d\n", localBits_idx);
            uint16_t code_s = litmtree_codes[lcode + LITERALS + 1];
            uint16_t bitlen = litmtree_blen[lcode + LITERALS + 1];
            lextra = extra_lbits[lcode];

            outStream << code_s;
            outStreamSize << bitlen;

            if (lextra)
                next_state = ML_EXTRA;
            else
                next_state = DIST_REP;

        } else if (next_state == LIT_REP) {
            // printf("In LIT_REP %d \n", localBits_idx);
            uint16_t bitlen = litmtree_blen[tCh];
            uint16_t code = litmtree_codes[tCh];

            outStream << code;
            outStreamSize << bitlen;

            next_state = WRITE_TOKEN;

        } else if (next_state == DIST_REP) {
            // printf("In DIST_REP %d \n", localBits_idx);
            uint16_t code_s = distree_codes[dcode];
            uint16_t bitlen = dtree_blen[dcode];
            dextra = extra_dbits[dcode];

            outStream << code_s;
            outStreamSize << bitlen;

            if (dextra)
                next_state = DIST_EXTRA;
            else
                next_state = WRITE_TOKEN;

        } else if (next_state == ML_EXTRA) {
            // printf("In ML_EXTRA \n");
            if (lextra) {
                tLen -= base_length[lcode];
                outStream << tLen;
                outStreamSize << lextra;

                next_state = DIST_REP;
            } else
                next_state = DIST_REP;

        } else if (next_state == DIST_EXTRA) {
            if (dextra) {
                tOffset -= base_dist[dcode];
                outStream << tOffset;
                outStreamSize << dextra;
                next_state = WRITE_TOKEN;
            } else
                next_state = WRITE_TOKEN;
        }
    }

    uint32_t first_push = 0;

    // End of block as per GZip standard
    outStream << litmtree_codes[256];
    outStreamSize << litmtree_blen[256];

    outStreamSize << 0;

} // Dynamic Huffman Funciton Ends here

void huffmanCore(hls::stream<uint512_t>& inStream512,
                  hls::stream<uint512_t>& outStream512,
                  hls::stream<uint16_t>& outStream512Size,
                  uint32_t input_size,
                  uint32_t core_idx,
                  hls::stream<uint32_t>& inStreamLTreeCode,
                  hls::stream<uint32_t>& inStreamLTreeBlen,
                  hls::stream<uint32_t>& inStreamDTreeCode,
                  hls::stream<uint32_t>& inStreamDTreeBlen,
                  hls::stream<uint32_t>& inStreamBLTreeCode,
                  hls::stream<uint32_t>& inStreamBLTreeBlen,
                  hls::stream<uint32_t>& inStreamMaxCode) {
    // printf("input_size %d core_idx %d \n", input_size, core_idx);
    hls::stream<encodedV_dt> inStream("inStream");
    hls::stream<uintOutV_t> huffOut("huffOut");
    hls::stream<uint16_t> huffOutSize("huffOutSize");
    hls::stream<uint16_t> bitVals("bitVals");
    hls::stream<uint8_t> bitLen("bitLen");
#pragma HLS STREAM variable = inStream depth = c_gmem_burst_size
#pragma HLS STREAM variable = huffOut depth = 2048
#pragma HLS STREAM variable = huffOutSize depth = c_gmem_burst_size
#pragma HLS STREAM variable = bitVals depth = c_gmem_burst_size
#pragma HLS STREAM variable = bitLen depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = huffOut core = FIFO_SRL
#pragma HLS RESOURCE variable = huffOutSize core = FIFO_SRL
#pragma HLS RESOURCE variable = bitVals core = FIFO_SRL
#pragma HLS RESOURCE variable = bitLen core = FIFO_SRL

#pragma HLS dataflow
    // Read data from kernel 1 in stream downsized manner
    // Write the data back to ddr and read it in host
    xf::compression::streamDownsizer<uint32_t, GMEM_DWIDTH, 32>(inStream512, inStream, input_size);

    dynamicHuffman(inStream, bitVals, bitLen, input_size, inStreamLTreeCode, inStreamLTreeBlen, inStreamDTreeCode,
                   inStreamDTreeBlen, inStreamBLTreeCode, inStreamBLTreeBlen, inStreamMaxCode);

    bitPacking(bitVals, bitLen, huffOut, huffOutSize);

    xf::compression::upsizerSizeStream<uint16_t, 16, GMEM_DWIDTH>(huffOut, huffOutSize, outStream512, outStream512Size);
}

void huffman(const uint512_t* in,
             uint512_t* out,
             uint32_t input_idx[PARALLEL_BLOCK],
             uint32_t output_idx[PARALLEL_BLOCK],
             uint32_t input_size[PARALLEL_BLOCK],
             uint32_t output_size[PARALLEL_BLOCK],
             uint32_t* dyn_litmtree_codes,
             uint32_t* dyn_distree_codes,
             uint32_t* dyn_bitlentree_codes,
             uint32_t* dyn_litmtree_blen,
             uint32_t* dyn_dtree_blen,
             uint32_t* dyn_bitlentree_blen,
             uint32_t* dyn_max_codes,
             uint32_t n_blocks) {
    // printf("n_blocks %d \n", n_blocks);

    hls::stream<uint512_t> inStream512_0("huffManinStream512_0");
    hls::stream<uint16_t> outStream512Size_0("outStream512Size_0");
    hls::stream<uint512_t> outStream512_0("outStream512_0");

    hls::stream<uint32_t> strlitmtree_codes_0("stream_litmtree_codes_0");
    hls::stream<uint32_t> strlitmtree_blen_0("stream_litmtree_blen_0");

    hls::stream<uint32_t> strdistree_codes_0("stream_distree_codes_0");
    hls::stream<uint32_t> strdtree_blen_0("stream_dtree_blen_0");

    hls::stream<uint32_t> strbitlentree_codes_0("stream_bitlentree_codes_0");
    hls::stream<uint32_t> strbitlentree_blen_0("stream_bitlentree_blen_0");
    hls::stream<uint32_t> strmax_code_0("strmax_code_0");
#pragma HLS STREAM variable = outStream512Size_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_0 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_0 core = FIFO_SRL

#if PARALLEL_BLOCK > 1
    hls::stream<uint512_t> inStream512_1("huffManinStream512_1");
    hls::stream<uint16_t> outStream512Size_1("outStream512Size_1");
    hls::stream<uint512_t> outStream512_1("outStream512_1");
    hls::stream<uint32_t> strlitmtree_codes_1("stream_litmtree_codes_1");
    hls::stream<uint32_t> strlitmtree_blen_1("stream_litmtree_blen_1");

    hls::stream<uint32_t> strdistree_codes_1("stream_distree_codes_1");
    hls::stream<uint32_t> strdtree_blen_1("stream_dtree_blen_1");

    hls::stream<uint32_t> strbitlentree_codes_1("stream_bitlentree_codes_1");
    hls::stream<uint32_t> strbitlentree_blen_1("stream_bitlentree_blen_1");
    hls::stream<uint32_t> strmax_code_1("strmax_code_1");
#pragma HLS STREAM variable = outStream512Size_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_1 depth = c_gmem_burst_size

#pragma HLS STREAM variable = strdistree_codes_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_1 depth = c_gmem_burst_size

#pragma HLS STREAM variable = strbitlentree_codes_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_1 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_1 core = FIFO_SRL

#pragma HLS RESOURCE variable = strdistree_codes_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_1 core = FIFO_SRL

#pragma HLS RESOURCE variable = strbitlentree_codes_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_1 core = FIFO_SRL
#endif

#if PARALLEL_BLOCK > 2
    hls::stream<uint512_t> inStream512_2("huffManinStream512_2");
    hls::stream<uint16_t> outStream512Size_2("outStream512Size_2");
    hls::stream<uint512_t> outStream512_2("outStream512_2");
    hls::stream<uint32_t> strlitmtree_codes_2("stream_litmtree_codes_2");
    hls::stream<uint32_t> strlitmtree_blen_2("stream_litmtree_blen_2");
    hls::stream<uint32_t> strdistree_codes_2("stream_distree_codes_2");
    hls::stream<uint32_t> strdtree_blen_2("stream_dtree_blen_2");
    hls::stream<uint32_t> strbitlentree_codes_2("stream_bitlentree_codes_2");
    hls::stream<uint32_t> strbitlentree_blen_2("stream_bitlentree_blen_2");
    hls::stream<uint32_t> strmax_code_2("strmax_code_2");
#pragma HLS STREAM variable = outStream512Size_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_2 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_2 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_3("huffManinStream512_3");
    hls::stream<uint16_t> outStream512Size_3("outStream512Size_3");
    hls::stream<uint512_t> outStream512_3("outStream512_3");
    hls::stream<uint32_t> strlitmtree_codes_3("stream_litmtree_codes_3");
    hls::stream<uint32_t> strlitmtree_blen_3("stream_litmtree_blen_3");
    hls::stream<uint32_t> strdistree_codes_3("stream_distree_codes_3");
    hls::stream<uint32_t> strdtree_blen_3("stream_dtree_blen_3");
    hls::stream<uint32_t> strbitlentree_codes_3("stream_bitlentree_codes_3");
    hls::stream<uint32_t> strbitlentree_blen_3("stream_bitlentree_blen_3");
    hls::stream<uint32_t> strmax_code_3("strmax_code_3");
#pragma HLS STREAM variable = outStream512Size_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_3 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_3 core = FIFO_SRL

#endif

#if PARALLEL_BLOCK > 4

    hls::stream<uint512_t> inStream512_4("huffManinStream512_4");
    hls::stream<uint16_t> outStream512Size_4("outStream512Size_4");
    hls::stream<uint512_t> outStream512_4("outStream512_4");
    hls::stream<uint32_t> strlitmtree_codes_4("stream_litmtree_codes_4");
    hls::stream<uint32_t> strlitmtree_blen_4("stream_litmtree_blen_4");
    hls::stream<uint32_t> strdistree_codes_4("stream_distree_codes_4");
    hls::stream<uint32_t> strdtree_blen_4("stream_dtree_blen_4");
    hls::stream<uint32_t> strbitlentree_codes_4("stream_bitlentree_codes_4");
    hls::stream<uint32_t> strbitlentree_blen_4("stream_bitlentree_blen_4");
    hls::stream<uint32_t> strmax_code_4("strmax_code_4");
#pragma HLS STREAM variable = outStream512Size_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_4 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_4 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_5("huffManinStream512_5");
    hls::stream<uint16_t> outStream512Size_5("outStream512Size_5");
    hls::stream<uint512_t> outStream512_5("outStream512_5");
    hls::stream<uint32_t> strlitmtree_codes_5("stream_litmtree_codes_5");
    hls::stream<uint32_t> strlitmtree_blen_5("stream_litmtree_blen_5");
    hls::stream<uint32_t> strdistree_codes_5("stream_distree_codes_5");
    hls::stream<uint32_t> strdtree_blen_5("stream_dtree_blen_5");
    hls::stream<uint32_t> strbitlentree_codes_5("stream_bitlentree_codes_5");
    hls::stream<uint32_t> strbitlentree_blen_5("stream_bitlentree_blen_5");
    hls::stream<uint32_t> strmax_code_5("strmax_code_5");
#pragma HLS STREAM variable = outStream512Size_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_5 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_5 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_6("huffManinStream512_6");
    hls::stream<uint16_t> outStream512Size_6("outStream512Size_6");
    hls::stream<uint512_t> outStream512_6("outStream512_6");
    hls::stream<uint32_t> strlitmtree_codes_6("stream_litmtree_codes_6");
    hls::stream<uint32_t> strlitmtree_blen_6("stream_litmtree_blen_6");
    hls::stream<uint32_t> strdistree_codes_6("stream_distree_codes_6");
    hls::stream<uint32_t> strdtree_blen_6("stream_dtree_blen_6");
    hls::stream<uint32_t> strbitlentree_codes_6("stream_bitlentree_codes_6");
    hls::stream<uint32_t> strbitlentree_blen_6("stream_bitlentree_blen_6");
    hls::stream<uint32_t> strmax_code_6("strmax_code_6");
#pragma HLS STREAM variable = outStream512Size_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_6 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_6 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_7("huffManinStream512_7");
    hls::stream<uint16_t> outStream512Size_7("outStream512Size_7");
    hls::stream<uint512_t> outStream512_7("outStream512_7");
    hls::stream<uint32_t> strlitmtree_codes_7("stream_litmtree_codes_7");
    hls::stream<uint32_t> strlitmtree_blen_7("stream_litmtree_blen_7");
    hls::stream<uint32_t> strdistree_codes_7("stream_distree_codes_7");
    hls::stream<uint32_t> strdtree_blen_7("stream_dtree_blen_7");
    hls::stream<uint32_t> strbitlentree_codes_7("stream_bitlentree_codes_7");
    hls::stream<uint32_t> strbitlentree_blen_7("stream_bitlentree_blen_7");
    hls::stream<uint32_t> strmax_code_7("strmax_code_7");
#pragma HLS STREAM variable = outStream512Size_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_codes_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strlitmtree_blen_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdistree_codes_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strdtree_blen_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_codes_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strbitlentree_blen_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = strmax_code_7 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_codes_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strlitmtree_blen_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdistree_codes_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strdtree_blen_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_codes_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strbitlentree_blen_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = strmax_code_7 core = FIFO_SRL

#endif

#pragma HLS dataflow
    xf::compression::mm2sFreq<GMEM_DWIDTH, GMEM_BURST_SIZE>(
        in, dyn_litmtree_codes, dyn_litmtree_blen, dyn_distree_codes, dyn_dtree_blen, dyn_bitlentree_codes,
        dyn_bitlentree_blen, dyn_max_codes, input_idx, inStream512_0,
#if PARALLEL_BLOCK > 1
        inStream512_1,
#endif
#if PARALLEL_BLOCK > 2
        inStream512_2, inStream512_3,
#endif
#if PARALLEL_BLOCK > 4
        inStream512_4, inStream512_5, inStream512_6, inStream512_7,
#endif
        strlitmtree_codes_0,
#if PARALLEL_BLOCK > 1
        strlitmtree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
        strlitmtree_codes_2, strlitmtree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
        strlitmtree_codes_4, strlitmtree_codes_5, strlitmtree_codes_6, strlitmtree_codes_7,
#endif
        strlitmtree_blen_0,
#if PARALLEL_BLOCK > 1
        strlitmtree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
        strlitmtree_blen_2, strlitmtree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
        strlitmtree_blen_4, strlitmtree_blen_5, strlitmtree_blen_6, strlitmtree_blen_7,
#endif
        strdistree_codes_0,
#if PARALLEL_BLOCK > 1
        strdistree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
        strdistree_codes_2, strdistree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
        strdistree_codes_4, strdistree_codes_5, strdistree_codes_6, strdistree_codes_7,
#endif
        strdtree_blen_0,
#if PARALLEL_BLOCK > 1
        strdtree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
        strdtree_blen_2, strdtree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
        strdtree_blen_4, strdtree_blen_5, strdtree_blen_6, strdtree_blen_7,
#endif
        strbitlentree_codes_0,
#if PARALLEL_BLOCK > 1
        strbitlentree_codes_1,
#endif
#if PARALLEL_BLOCK > 2
        strbitlentree_codes_2, strbitlentree_codes_3,
#endif
#if PARALLEL_BLOCK > 4
        strbitlentree_codes_4, strbitlentree_codes_5, strbitlentree_codes_6, strbitlentree_codes_7,
#endif
        strbitlentree_blen_0,
#if PARALLEL_BLOCK > 1
        strbitlentree_blen_1,
#endif
#if PARALLEL_BLOCK > 2
        strbitlentree_blen_2, strbitlentree_blen_3,
#endif
#if PARALLEL_BLOCK > 4
        strbitlentree_blen_4, strbitlentree_blen_5, strbitlentree_blen_6, strbitlentree_blen_7,
#endif
        strmax_code_0,
#if PARALLEL_BLOCK > 1
        strmax_code_1,
#endif
#if PARALLEL_BLOCK > 2
        strmax_code_2, strmax_code_3,
#endif
#if PARALLEL_BLOCK > 4
        strmax_code_4, strmax_code_5, strmax_code_6, strmax_code_7,
#endif
        n_blocks, input_size);

    huffmanCore(inStream512_0, outStream512_0, outStream512Size_0, input_size[0], 0, strlitmtree_codes_0,
                 strlitmtree_blen_0, strdistree_codes_0, strdtree_blen_0, strbitlentree_codes_0, strbitlentree_blen_0,
                 strmax_code_0);

#if PARALLEL_BLOCK > 1
    huffmanCore(inStream512_1, outStream512_1, outStream512Size_1, input_size[1], 1, strlitmtree_codes_1,
                 strlitmtree_blen_1, strdistree_codes_1, strdtree_blen_1, strbitlentree_codes_1, strbitlentree_blen_1,
                 strmax_code_1);
#endif

#if PARALLEL_BLOCK > 2
    huffmanCore(inStream512_2, outStream512_2, outStream512Size_2, input_size[2], 2, strlitmtree_codes_2,
                 strlitmtree_blen_2, strdistree_codes_2, strdtree_blen_2, strbitlentree_codes_2, strbitlentree_blen_2,
                 strmax_code_2);

    huffmanCore(inStream512_3, outStream512_3, outStream512Size_3, input_size[3], 3, strlitmtree_codes_3,
                 strlitmtree_blen_3, strdistree_codes_3, strdtree_blen_3, strbitlentree_codes_3, strbitlentree_blen_3,
                 strmax_code_3);
#endif

#if PARALLEL_BLOCK > 4
    huffmanCore(inStream512_4, outStream512_4, outStream512Size_4, input_size[4], 4, strlitmtree_codes_4,
                 strlitmtree_blen_4, strdistree_codes_4, strdtree_blen_4, strbitlentree_codes_4, strbitlentree_blen_4,
                 strmax_code_4);

    huffmanCore(inStream512_5, outStream512_5, outStream512Size_5, input_size[5], 5, strlitmtree_codes_5,
                 strlitmtree_blen_5, strdistree_codes_5, strdtree_blen_5, strbitlentree_codes_5, strbitlentree_blen_5,
                 strmax_code_5);

    huffmanCore(inStream512_6, outStream512_6, outStream512Size_6, input_size[6], 6, strlitmtree_codes_6,
                 strlitmtree_blen_6, strdistree_codes_6, strdtree_blen_6, strbitlentree_codes_6, strbitlentree_blen_6,
                 strmax_code_6);

    huffmanCore(inStream512_7, outStream512_7, outStream512Size_7, input_size[7], 7, strlitmtree_codes_7,
                 strlitmtree_blen_7, strdistree_codes_7, strdtree_blen_7, strbitlentree_codes_7, strbitlentree_blen_7,
                 strmax_code_7);
#endif

    xf::compression::s2mmCompress<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH>(
        out, output_idx, outStream512_0,
#if PARALLEL_BLOCK > 1
        outStream512_1,
#endif
#if PARALLEL_BLOCK > 2
        outStream512_2, outStream512_3,
#endif
#if PARALLEL_BLOCK > 4
        outStream512_4, outStream512_5, outStream512_6, outStream512_7,
#endif
        outStream512Size_0,
#if PARALLEL_BLOCK > 1
        outStream512Size_1,
#endif
#if PARALLEL_BLOCK > 2
        outStream512Size_2, outStream512Size_3,
#endif
#if PARALLEL_BLOCK > 4
        outStream512Size_4, outStream512Size_5, outStream512Size_6, outStream512Size_7,
#endif
        output_size);
}
} // namespace cu1

extern "C" {
#if (H_COMPUTE_UNIT == 1)
void xilHuffman_cu1
#elif (H_COMPUTE_UNIT == 2)
void xilHuffman_cu2
#endif
    (uint512_t* in,
     uint512_t* out,
     uint32_t* in_block_size,
     uint32_t* compressd_size,
     uint32_t* dyn_litmtree_codes,
     uint32_t* dyn_distree_codes,
     uint32_t* dyn_bitlentree_codes,
     uint32_t* dyn_litmtree_blen,
     uint32_t* dyn_dtree_blen,
     uint32_t* dyn_bitlentree_blen,
     uint32_t* dyn_max_codes,
     uint32_t block_size_in_kb,
     uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1

#pragma HLS INTERFACE m_axi port = dyn_litmtree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_distree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_bitlentree_codes offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_litmtree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_dtree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_bitlentree_blen offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_max_codes offset = slave bundle = gmem1

#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_litmtree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_distree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bitlentree_codes bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_litmtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_bitlentree_blen bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_max_codes bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    // printf("In Huffman kernel \n");

    int block_idx = 0;
    int block_length = block_size_in_kb * 1024;
    int no_blocks = (input_size - 1) / block_length + 1;
    uint32_t max_block_size = block_size_in_kb * 1024;

    // printf("Huffman no_blocks %d block_size_in_kb %d \n", no_blocks, block_size_in_kb);

    // printf("no_blocks %d \n", no_blocks);
    bool small_block[PARALLEL_BLOCK];
    uint32_t input_block_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t output_idx[PARALLEL_BLOCK];
    uint32_t output_block_size[PARALLEL_BLOCK];
    uint32_t small_block_inSize[PARALLEL_BLOCK];
    uint32_t max_lit_limit[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = max_lit_limit dim = 0 complete

    // Figure out total blocks and block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int n_blocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) n_blocks = no_blocks - i;

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
            if (j < n_blocks) {
                uint32_t inBlockSize = in_block_size[i + j];

                if (inBlockSize < MIN_BLOCK_SIZE) {
                    small_block[j] = 1;
                    small_block_inSize[j] = inBlockSize;
                    input_block_size[j] = 0;
                    input_idx[j] = 0;
                } else {
                    small_block[j] = 0;
                    input_block_size[j] = inBlockSize;
                    // printf("%s - inBlockSize %d \n", __FUNCTION__, inBlockSize);
                    input_idx[j] = (i + j) * max_block_size * 4;
                    output_idx[j] = (i + j) * max_block_size;
                }
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
            max_lit_limit[j] = 0;
        }
#if (H_COMPUTE_UNIT == 1)
        // printf("Huffman CU 1\n");
        cu1::huffman(in, out, input_idx, output_idx, input_block_size, output_block_size, dyn_litmtree_codes,
                     dyn_distree_codes, dyn_bitlentree_codes, dyn_litmtree_blen, dyn_dtree_blen, dyn_bitlentree_blen,
                     dyn_max_codes, n_blocks);
#elif (H_COMPUTE_UNIT == 2)
        // printf("Huffman CU 2\n");
        cu2::huffman(in, out, input_idx, output_idx, input_block_size, output_block_size, dyn_litmtree_codes,
                     dyn_distree_codes, dyn_bitlentree_codes, dyn_litmtree_blen, dyn_dtree_blen, dyn_bitlentree_blen,
                     dyn_max_codes, n_blocks);
#endif

        for (int k = 0; k < n_blocks; k++) {
            if (max_lit_limit[k]) {
                in_block_size[block_idx] = input_block_size[k];
            } else {
                in_block_size[block_idx] = output_block_size[k];
                // printf("%s - compressed size %d \n", __FUNCTION__, in_block_size[block_idx]);
            }

            if (small_block[k] == 1) in_block_size[block_idx] = small_block_inSize[k];

            block_idx++;
        }
    } // Main loop ends here
    // printf("Huffman kernel done \n");
}
}
