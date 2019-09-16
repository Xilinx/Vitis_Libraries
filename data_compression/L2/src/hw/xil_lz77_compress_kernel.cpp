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
#include "lz77_compress_kernel.hpp"

#define LTREE_SIZE 1024
#define DTREE_SIZE 64

const int c_gmem_burst_size = (2 * GMEM_BURST_SIZE);

#if (C_COMPUTE_UNIT == 1)
namespace cu1
#elif (C_COMPUTE_UNIT == 2)
namespace cu2
#endif
{
void lz77Divide(hls::stream<compressd_dt>& inStream,
                 hls::stream<ap_uint<32> >& outStream,
                 hls::stream<uint16_t>& outStreamSize,
                 hls::stream<uint32_t>& outStreamTree,
                 uint32_t input_size,
                 uint32_t core_idx) {
    if (input_size == 0) {
        outStreamSize << 0;
        outStreamTree << 9999;
        return;
    }

    uint32_t lcl_dyn_ltree[LTREE_SIZE];
    uint32_t lcl_dyn_dtree[DTREE_SIZE];

ltree_init:
    for (uint32_t i = 0; i < LTREE_SIZE; i++) lcl_dyn_ltree[i] = 0;

dtree_init:
    for (uint32_t i = 0; i < DTREE_SIZE; i++) lcl_dyn_dtree[i] = 0;

    int length = 0;
    int code = 0;
    int n = 0;

// Dynamic Huffman Frequency counts
// Based on GZip spec
#include "gzip_tables.h"

    uint32_t out_cntr = 0;
    uint8_t match_len = 0;
    uint32_t loc_idx = 0;

    compressd_dt nextEncodedValue = inStream.read();
lz77Divide:
    for (uint32_t i = 0; i < input_size; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 1048576 max = 1048576
#pragma HLS PIPELINE II = 1
#pragma HLS dependence variable = lcl_dyn_ltree inter false
#pragma HLS dependence variable = lcl_dyn_dtree inter false
        compressd_dt tmpEncodedValue = nextEncodedValue;
        if (i < (input_size - 1)) nextEncodedValue = inStream.read();

        uint8_t tCh = tmpEncodedValue.range(7, 0);
        uint8_t tLen = tmpEncodedValue.range(15, 8);
        uint16_t tOffset = tmpEncodedValue.range(31, 16);

        uint32_t ltreeIdx, dtreeIdx;
        if (tLen > 0) {
            ltreeIdx = length_code[tLen - 3] + LITERALS + 1;
            dtreeIdx = d_code(tOffset, dist_code);

            ////printme("tLen %d \n", tLen);
            // dyn_ltree[ltreeIdx]++;
            i += (tLen - 1);
            tmpEncodedValue.range(15, 8) = tLen - 3;
            // printme("%s tLen %d \n", __FUNCTION__, tLen);
            ////printme("match_length %d offset %d d_code %d\n", tLen - 1, tOffset, d_code(tOffset, dist_code));
        } else {
            ltreeIdx = tCh;
            dtreeIdx = 63;
            // dyn_ltree[ltreeIdx]++;
            ////printme("literals %c i %d\n", tCh, i);
        }
        lcl_dyn_ltree[ltreeIdx]++;
        lcl_dyn_dtree[dtreeIdx]++;

        outStream << tmpEncodedValue;
        out_cntr += 4;
        ////printme("tCh %d tLen %d tOffset1 %d tOffset2 %d \n", tCh, tLen, (uint8_t)tOffset, (uint8_t)(tOffset >> 8));

        if (out_cntr >= 512) {
            outStreamSize << out_cntr;
            out_cntr = 0;
        }
    }

    if (out_cntr) outStreamSize << out_cntr;

    outStreamSize << 0;

    for (uint32_t i = 0; i < LTREE_SIZE; i++) outStreamTree << lcl_dyn_ltree[i];

    for (uint32_t j = 0; j < DTREE_SIZE; j++) outStreamTree << lcl_dyn_dtree[j];
}

void lz77Core(hls::stream<uint512_t>& inStream512,
               hls::stream<uint512_t>& outStream512,
               hls::stream<uint16_t>& outStream512Size,
               hls::stream<uint32_t>& outStreamTree,
               uint32_t max_lit_limit[PARALLEL_BLOCK],
               uint32_t input_size,
               uint32_t core_idx) {
    ////printme("calcntr %d \n", calcntr++);
    uint32_t left_bytes = 64;
    hls::stream<ap_uint<BIT> > inStream("inStream");
    hls::stream<compressd_dt> compressdStream("compressdStream");
    hls::stream<compressd_dt> boosterStream("boosterStream");
    hls::stream<compressd_dt> boosterStream_freq("boosterStream");
    hls::stream<uint8_t> litOut("litOut");
    hls::stream<lz77_compressd_dt> lenOffsetOut("lenOffsetOut");
    hls::stream<ap_uint<32> > lz77Out("lz77Out");
    hls::stream<uint16_t> lz77OutSize("lz77OutSize");
#pragma HLS STREAM variable = inStream depth = c_gmem_burst_size
#pragma HLS STREAM variable = compressdStream depth = c_gmem_burst_size
#pragma HLS STREAM variable = boosterStream depth = c_gmem_burst_size
#pragma HLS STREAM variable = litOut depth = gz_max_literal_count
#pragma HLS STREAM variable = lenOffsetOut depth = c_gmem_burst_size
#pragma HLS STREAM variable = lz77Out depth = 1024
#pragma HLS STREAM variable = lz77OutSize depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = inStream core = FIFO_SRL
#pragma HLS RESOURCE variable = compressdStream core = FIFO_SRL
#pragma HLS RESOURCE variable = boosterStream core = FIFO_SRL
#pragma HLS RESOURCE variable = litOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lenOffsetOut core = FIFO_SRL
#pragma HLS RESOURCE variable = lz77Out core = FIFO_SRL
#pragma HLS RESOURCE variable = lz77OutSize core = FIFO_SRL

#pragma HLS dataflow
    xf::compression::streamDownsizer<uint32_t, GMEM_DWIDTH, 8>(inStream512, inStream, input_size);
    xf::compression::lzCompress<MATCH_LEN, MATCH_LEVEL, LZ_DICT_SIZE, BIT, MIN_OFFSET, MIN_MATCH, LZ_MAX_OFFSET_LIMIT>(
        inStream, compressdStream, input_size, left_bytes);
    xf::compression::lzBooster<MAX_MATCH_LEN, OFFSET_WINDOW>(compressdStream, boosterStream, input_size, left_bytes);
    lz77Divide(boosterStream, lz77Out, lz77OutSize, outStreamTree, input_size, core_idx);
    xf::compression::upsizerSizeStream<uint16_t, 32, GMEM_DWIDTH>(lz77Out, lz77OutSize, outStream512, outStream512Size);
}

void lz77(const uint512_t* in,
          uint512_t* out,
          const uint32_t input_idx[PARALLEL_BLOCK],
          const uint32_t output_idx[PARALLEL_BLOCK],
          const uint32_t input_size[PARALLEL_BLOCK],
          uint32_t output_size[PARALLEL_BLOCK],
          uint32_t max_lit_limit[PARALLEL_BLOCK],
          uint32_t* dyn_ltree_freq,
          uint32_t* dyn_dtree_freq) {
    hls::stream<uint512_t> inStream512_0("lz77inStream512_0");
    hls::stream<uint16_t> outStream512Size_0("outStream512Size_0");
    hls::stream<uint512_t> outStream512_0("outStream512_0");
    hls::stream<uint32_t> stream_ltree_0("stream_ltree_0");
#pragma HLS STREAM variable = outStream512Size_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_0 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_0 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_0 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_0 core = FIFO_SRL

#if PARALLEL_BLOCK > 1
    hls::stream<uint512_t> inStream512_1("lz77inStream512_1");
    hls::stream<uint16_t> outStream512Size_1("outStream512Size_1");
    hls::stream<uint512_t> outStream512_1("outStream512_1");
    hls::stream<uint32_t> stream_ltree_1("stream_ltree_1");
#pragma HLS STREAM variable = outStream512Size_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_1 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_1 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_1 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_1 core = FIFO_SRL
#endif

#if PARALLEL_BLOCK > 2
    hls::stream<uint512_t> inStream512_2("lz77inStream512_2");
    hls::stream<uint16_t> outStream512Size_2("outStream512Size_2");
    hls::stream<uint512_t> outStream512_2("outStream512_2");
    hls::stream<uint32_t> stream_ltree_2("stream_ltree_2");
#pragma HLS STREAM variable = outStream512Size_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_2 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_2 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_2 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_2 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_3("lz77inStream512_3");
    hls::stream<uint16_t> outStream512Size_3("outStream512Size_3");
    hls::stream<uint512_t> outStream512_3("outStream512_3");
    hls::stream<uint32_t> stream_ltree_3("stream_ltree_3");
#pragma HLS STREAM variable = outStream512Size_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_3 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_3 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_3 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_3 core = FIFO_SRL
#endif

#if PARALLEL_BLOCK > 4
    hls::stream<uint512_t> inStream512_4("lz77inStream512_4");
    hls::stream<uint16_t> outStream512Size_4("outStream512Size_4");
    hls::stream<uint512_t> outStream512_4("outStream512_4");
    hls::stream<uint32_t> stream_ltree_4("stream_ltree_4");
#pragma HLS STREAM variable = outStream512Size_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_4 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_4 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_4 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_4 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_5("lz77inStream512_5");
    hls::stream<uint16_t> outStream512Size_5("outStream512Size_5");
    hls::stream<uint512_t> outStream512_5("outStream512_5");
    hls::stream<uint32_t> stream_ltree_5("stream_ltree_5");
#pragma HLS STREAM variable = outStream512Size_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_5 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_5 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_5 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_5 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_6("lz77inStream512_6");
    hls::stream<uint16_t> outStream512Size_6("outStream512Size_6");
    hls::stream<uint512_t> outStream512_6("outStream512_6");
    hls::stream<uint32_t> stream_ltree_6("stream_ltree_6");
#pragma HLS STREAM variable = outStream512Size_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_6 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_6 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_6 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_6 core = FIFO_SRL

    hls::stream<uint512_t> inStream512_7("lz77inStream512_7");
    hls::stream<uint16_t> outStream512Size_7("outStream512Size_7");
    hls::stream<uint512_t> outStream512_7("outStream512_7");
    hls::stream<uint32_t> stream_ltree_7("stream_ltree_7");
#pragma HLS STREAM variable = outStream512Size_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = inStream512_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = outStream512_7 depth = c_gmem_burst_size
#pragma HLS STREAM variable = stream_ltree_7 depth = c_gmem_burst_size

#pragma HLS RESOURCE variable = outStream512Size_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = inStream512_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = outStream512_7 core = FIFO_SRL
#pragma HLS RESOURCE variable = stream_ltree_7 core = FIFO_SRL
#endif

////printme("input_size 0 %d input_size 1 %d\n", input_size[0], input_size[1]);
#pragma HLS dataflow
    xf::compression::gzMm2s<GMEM_DWIDTH, GMEM_BURST_SIZE>(in, input_idx, inStream512_0,
#if PARALLEL_BLOCK > 1
                                                          inStream512_1,
#endif
#if PARALLEL_BLOCK > 2
                                                          inStream512_2, inStream512_3,
#endif
#if PARALLEL_BLOCK > 4
                                                          inStream512_4, inStream512_5, inStream512_6, inStream512_7,
#endif

                                                          input_size);

    lz77Core(inStream512_0, outStream512_0, outStream512Size_0, stream_ltree_0, max_lit_limit, input_size[0], 0);
#if PARALLEL_BLOCK > 1
    lz77Core(inStream512_1, outStream512_1, outStream512Size_1, stream_ltree_1, max_lit_limit, input_size[1], 1);
#endif

#if PARALLEL_BLOCK > 2
    lz77Core(inStream512_2, outStream512_2, outStream512Size_2, stream_ltree_2, max_lit_limit, input_size[2], 2);
    lz77Core(inStream512_3, outStream512_3, outStream512Size_3, stream_ltree_3, max_lit_limit, input_size[3], 3);
#endif

#if PARALLEL_BLOCK > 4
    lz77Core(inStream512_4, outStream512_4, outStream512Size_4, stream_ltree_4, max_lit_limit, input_size[4], 4);
    lz77Core(inStream512_5, outStream512_5, outStream512Size_5, stream_ltree_5, max_lit_limit, input_size[5], 5);
    lz77Core(inStream512_6, outStream512_6, outStream512Size_6, stream_ltree_6, max_lit_limit, input_size[6], 6);
    lz77Core(inStream512_7, outStream512_7, outStream512Size_7, stream_ltree_7, max_lit_limit, input_size[7], 7);
#endif

    xf::compression::s2mmCompressFreq<uint32_t, GMEM_BURST_SIZE, GMEM_DWIDTH>(
        out, dyn_ltree_freq, dyn_dtree_freq, output_idx, outStream512_0,
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
        stream_ltree_0,
#if PARALLEL_BLOCK > 1
        stream_ltree_1,
#endif
#if PARALLEL_BLOCK > 2
        stream_ltree_2, stream_ltree_3,
#endif
#if PARALLEL_BLOCK > 4
        stream_ltree_4, stream_ltree_5, stream_ltree_6, stream_ltree_7,
#endif
        output_size);
}
} // namespace cu1

extern "C" {
#if (C_COMPUTE_UNIT == 1)
void xilLz77_cu1
#elif (C_COMPUTE_UNIT == 2)
void xilLz77_cu2
#endif
    (const uint512_t* in,
     uint512_t* out,
     uint32_t* compressd_size,
     uint32_t* in_block_size,
     uint32_t* dyn_ltree_freq,
     uint32_t* dyn_dtree_freq,
     uint32_t block_size_in_kb,
     uint32_t input_size) {
#pragma HLS INTERFACE m_axi port = in offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = out offset = slave bundle = gmem0
#pragma HLS INTERFACE m_axi port = compressd_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = in_block_size offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_ltree_freq offset = slave bundle = gmem1
#pragma HLS INTERFACE m_axi port = dyn_dtree_freq offset = slave bundle = gmem1
#pragma HLS INTERFACE s_axilite port = in bundle = control
#pragma HLS INTERFACE s_axilite port = out bundle = control
#pragma HLS INTERFACE s_axilite port = compressd_size bundle = control
#pragma HLS INTERFACE s_axilite port = in_block_size bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_ltree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = dyn_dtree_freq bundle = control
#pragma HLS INTERFACE s_axilite port = block_size_in_kb bundle = control
#pragma HLS INTERFACE s_axilite port = input_size bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    ////printme("In LZ77 kernel \n");

    int block_idx = 0;
    int block_length = block_size_in_kb * 1024;
    int no_blocks = (input_size - 1) / block_length + 1;
    uint32_t max_block_size = block_size_in_kb * 1024;

    bool small_block[PARALLEL_BLOCK];
    uint32_t input_block_size[PARALLEL_BLOCK];
    uint32_t input_idx[PARALLEL_BLOCK];
    uint32_t output_idx[PARALLEL_BLOCK];
    uint32_t output_block_size[PARALLEL_BLOCK];
    uint32_t max_lit_limit[PARALLEL_BLOCK];
    uint32_t small_block_inSize[PARALLEL_BLOCK];
#pragma HLS ARRAY_PARTITION variable = input_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = input_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_idx dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = output_block_size dim = 0 complete
#pragma HLS ARRAY_PARTITION variable = max_lit_limit dim = 0 complete

    // Figure out total blocks & block sizes
    for (int i = 0; i < no_blocks; i += PARALLEL_BLOCK) {
        int nblocks = PARALLEL_BLOCK;
        if ((i + PARALLEL_BLOCK) > no_blocks) {
            nblocks = no_blocks - i;
        }

        for (int j = 0; j < PARALLEL_BLOCK; j++) {
            if (j < nblocks) {
                uint32_t inBlockSize = in_block_size[i + j];
                if (inBlockSize < MIN_BLOCK_SIZE) {
                    small_block[j] = 1;
                    small_block_inSize[j] = inBlockSize;
                    input_block_size[j] = 0;
                    input_idx[j] = 0;
                } else {
                    small_block[j] = 0;
                    ////printme("%s - inBlockSize %d \n",__FUNCTION__, inBlockSize);
                    input_block_size[j] = inBlockSize;
                    input_idx[j] = (i + j) * max_block_size;
                    output_idx[j] = (i + j) * max_block_size * 4;
                }
            } else {
                input_block_size[j] = 0;
                input_idx[j] = 0;
            }
            output_block_size[j] = 0;
            max_lit_limit[j] = 0;
        }

#if (C_COMPUTE_UNIT == 1)
        // Call for parallel compression
        cu1::lz77(in, out, input_idx, output_idx, input_block_size, output_block_size, max_lit_limit, dyn_ltree_freq,
                  dyn_dtree_freq);
#elif (C_COMPUTE_UNIT == 2)
        ////printme("LZ77 CU2 start --- no_blocks %d \n", no_blocks);
        // Call for parallel compression
        cu2::lz77(in, out, input_idx, output_idx, input_block_size, output_block_size, max_lit_limit, dyn_ltree_freq,
                  dyn_dtree_freq);
#endif

        for (int k = 0; k < nblocks; k++) {
            if (max_lit_limit[k]) {
                compressd_size[block_idx] = input_block_size[k];
            } else {
                compressd_size[block_idx] = output_block_size[k];
                ////printme("%s - compressed size %d \n", __FUNCTION__, compressd_size[block_idx]);
            }

            if (small_block[k] == 1) {
                compressd_size[block_idx] = small_block_inSize[k];
            }
            block_idx++;
        }
    }
}
}
