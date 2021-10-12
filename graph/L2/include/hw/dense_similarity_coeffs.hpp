/*
 * Copyright 2021 Xilinx, Inc.
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

#ifndef __XF_GRAPH_DENSE_SIMILARITY_COEFFS_HPP_
#define __XF_GRAPH_DENSE_SIMILARITY_COEFFS_HPP_

#include "similarity/dense_similarity_int.hpp"
#include "similarity/sort_top_k.hpp"
#include "similarity/enums.hpp"

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace graph {
namespace internal {

template <int CHNM, int WData>
void loadSource(ap_int<32> similarity_type,
                ap_int<32> source_nm,

                ap_int<WData>* weight,
                ap_int<WData>* coeffs,
                hls::stream<ap_int<WData> >& source_weight) {
#pragma HLS INLINE off

    for (ap_int<32> i = 0; i < source_nm; i++) {
#pragma HLS PIPELINE II = 1
        source_weight.write(weight[i] * coeffs[i]);
    }
}

// calculate loop number for loading data in DDR/HBM
template <int CHNM>
void loopNumGen(ap_int<32> num, ap_int<32>& loop_num) {
    ap_int<32> base = num / CHNM;
    ap_int<4> fraction = num % CHNM;
    loop_num = fraction == 0 ? base : (ap_int<32>)(base + 1);
}

// generate control parameters for loading data
template <int CHNM, int WData>
void loadControl(ap_int<32> similarity_type, ap_int<32> vertex_num, ap_int<32> edge_num, ap_int<32> data_num[4]) {
#pragma HLS INLINE off

#ifndef __SYNTHESIS__
    std::cout << "loading control: vertex_num=" << vertex_num << " edge_num=" << edge_num << std::endl;
#endif

    loopNumGen<CHNM>(vertex_num * edge_num, data_num[0]);
    data_num[1] = data_num[0];
    data_num[2] = data_num[0];
    data_num[3] = data_num[0];
}

template <int CHNM, int WData>
void load(ap_int<32> num, ap_int<WData * CHNM>* data, hls::stream<ap_int<WData * CHNM> >& strm) {
#pragma HLS INLINE off

    ap_int<WData * CHNM> in;
    ap_int<32> base = num(31, 6);
    ap_int<32> fraction = num(5, 0);

#ifndef __SYNTHESIS__
    std::cout << "loading data: num=" << num << " base=" << base << " fraction=" << fraction << std::endl;
#endif

    ap_int<32> addr = 0;

load_base:
    for (ap_int<32> i = 0; i < base; i++) {
        for (ap_int<32> j = 0; j < 64; j++) {
#pragma HLS PIPELINE II = 1

            in = data[addr];
            addr++;
            strm.write(in);
        }
    }

    addr(31, 6) = base;
load_fraction:
    for (ap_int<32> i = 0; i < fraction; i++) {
#pragma HLS PIPELINE II = 1

        in = data[addr];
        addr++;

#ifndef __SYNTHESIS__
        std::cout << "loading data: fraction=" << std::hex << in << std::dec << std::endl;
#endif

        strm.write(in);
    }
}

template <int CHNM, int WData>
void loadData(ap_int<32> data_num[4],

              ap_int<WData * CHNM>* data0,
              ap_int<WData * CHNM>* data1,
              ap_int<WData * CHNM>* data2,
              ap_int<WData * CHNM>* data3,

              hls::stream<ap_int<WData * CHNM> >& strm0,
              hls::stream<ap_int<WData * CHNM> >& strm1,
              hls::stream<ap_int<WData * CHNM> >& strm2,
              hls::stream<ap_int<WData * CHNM> >& strm3) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    load<CHNM, WData>(data_num[0], data0, strm0);

    load<CHNM, WData>(data_num[1], data1, strm1);

    load<CHNM, WData>(data_num[2], data2, strm2);

    load<CHNM, WData>(data_num[3], data3, strm3);
}

template <int CHNM, int WData>
void loadPU(ap_int<32> similarity_type,
            ap_int<32> vertex_num,
            ap_int<32> edge_num,

            ap_int<WData * CHNM>* data0,
            ap_int<WData * CHNM>* data1,
            ap_int<WData * CHNM>* data2,
            ap_int<WData * CHNM>* data3,

            hls::stream<ap_int<WData * CHNM> >& strm0,
            hls::stream<ap_int<WData * CHNM> >& strm1,
            hls::stream<ap_int<WData * CHNM> >& strm2,
            hls::stream<ap_int<WData * CHNM> >& strm3) {
#pragma HLS INLINE off

    ap_int<32> data_num[4];
#pragma HLS array_partition variable = data_num complete

    loadControl<CHNM, WData>(similarity_type, vertex_num, edge_num, data_num);

    loadData<CHNM, WData>(data_num, data0, data1, data2, data3, strm0, strm1, strm2, strm3);
}

template <int PU>
void loadConfig(ap_int<32>* config,
                ap_int<32>& k,
                ap_int<32>& source_num,
                ap_int<32>& similarity_type,
                ap_int<32>& data_type,
                ap_int<32> start_id[PU],
                ap_int<32> vertex_num[PU],
                ap_int<32> edge_num[PU],
                hls::stream<ap_int<32> >& config_strm) {
#pragma HLS INLINE off

    k = config[0];
    source_num = config[1];
    similarity_type = config[2];
    data_type = config[3];

    for (ap_int<8> i = 0; i < PU; i++) {
        start_id[i] = config[4 + i];
    }

    for (ap_int<8> i = 0; i < PU; i++) {
        vertex_num[i] = config[4 + PU + i];
    }

    for (ap_int<8> i = 0; i < PU; i++) {
        edge_num[i] = config[4 + 2 * PU + i];
    }

    for (ap_int<8> i = 1; i < 3 * PU + 4; i++) {
        config_strm.write(config[i]);
    }

#ifndef __SYNTHESIS__
    for (int i = 0; i < 3 * PU + 4; i++) std::cout << "config" << i << " = " << config[i] << std::endl;
#endif
}

template <int CHNM, int WData>
void loadData3PU(ap_int<32> similarity_type,
                 ap_int<32> vertex_num[8],
                 ap_int<32> edge_num[8],

                 ap_int<WData * CHNM>* dataIn00,
                 ap_int<WData * CHNM>* dataIn01,
                 ap_int<WData * CHNM>* dataIn02,
                 ap_int<WData * CHNM>* dataIn03,

                 ap_int<WData * CHNM>* dataIn10,
                 ap_int<WData * CHNM>* dataIn11,
                 ap_int<WData * CHNM>* dataIn12,
                 ap_int<WData * CHNM>* dataIn13,

                 ap_int<WData * CHNM>* dataIn20,
                 ap_int<WData * CHNM>* dataIn21,
                 ap_int<WData * CHNM>* dataIn22,
                 ap_int<WData * CHNM>* dataIn23,

                 hls::stream<ap_int<WData * CHNM> > strm0[4],
                 hls::stream<ap_int<WData * CHNM> > strm1[4],
                 hls::stream<ap_int<WData * CHNM> > strm2[4],
                 hls::stream<ap_int<WData * CHNM> > strm3[4]) {
#pragma HLS INLINE

    loadPU<CHNM, WData>(similarity_type, vertex_num[0], edge_num[0], dataIn00, dataIn01, dataIn02, dataIn03, strm0[0],
                        strm1[0], strm2[0], strm3[0]);

    loadPU<CHNM, WData>(similarity_type, vertex_num[1], edge_num[1], dataIn10, dataIn11, dataIn12, dataIn13, strm0[1],
                        strm1[1], strm2[1], strm3[1]);

    loadPU<CHNM, WData>(similarity_type, vertex_num[2], edge_num[2], dataIn20, dataIn21, dataIn22, dataIn23, strm0[2],
                        strm1[2], strm2[2], strm3[2]);
}

void feedResult(hls::stream<ap_int<32> >& row_strm,
                hls::stream<float>& similarity_strm,
                hls::stream<bool>& end_strm,
                ap_int<32>* result_id,
                float* similarity) {
    ap_int<32> addr = 0;
    bool end = end_strm.read();
    while (!end) {
        ap_int<32> row_tmp = row_strm.read();
        float similarity_tmp = similarity_strm.read();
        end = end_strm.read();

#ifndef __SYNTHESIS__
        std::cout << std::dec << "addr=" << addr << " row=" << row_tmp << " similarity=" << similarity_tmp << std::endl;
#endif
        result_id[addr] = row_tmp;
        similarity[addr] = similarity_tmp;
        addr++;
    }
}

template <int CHNM, int WData, int RAM_SZ, int MAXK>
void denseSimilarityTop3PU(ap_int<32> k,
                           ap_int<32> source_num,
                           ap_int<32> similarity_type,
                           ap_int<32> data_type,

                           ap_int<32> start_id[8],
                           ap_int<32> vertex_num[8],
                           ap_int<32> edge_num[8],

                           hls::stream<ap_int<32> >& config_strm,
                           hls::stream<ap_int<32> >& sourceWeight,

                           ap_int<WData * CHNM>* dataIn00,
                           ap_int<WData * CHNM>* dataIn01,
                           ap_int<WData * CHNM>* dataIn02,
                           ap_int<WData * CHNM>* dataIn03,

                           ap_int<WData * CHNM>* dataIn10,
                           ap_int<WData * CHNM>* dataIn11,
                           ap_int<WData * CHNM>* dataIn12,
                           ap_int<WData * CHNM>* dataIn13,

                           ap_int<WData * CHNM>* dataIn20,
                           ap_int<WData * CHNM>* dataIn21,
                           ap_int<WData * CHNM>* dataIn22,
                           ap_int<WData * CHNM>* dataIn23,

                           hls::stream<ap_int<WData> >& resultID,
                           hls::stream<float>& similarity,
                           hls::stream<bool>& end_strm) {
#pragma HLS INLINE off
#pragma HLS DATAFLOW

    const int PU = 3;

    hls::stream<ap_int<WData * CHNM> > strm_in0[PU];
#pragma HLS stream variable = strm_in0 depth = 512
#pragma HLS array_partition variable = strm_in0 complete
#pragma HLS resource variable = strm_in0 core = FIFO_BRAM
    hls::stream<ap_int<WData * CHNM> > strm_in1[PU];
#pragma HLS stream variable = strm_in1 depth = 512
#pragma HLS array_partition variable = strm_in1 complete
#pragma HLS resource variable = strm_in1 core = FIFO_BRAM
    hls::stream<ap_int<WData * CHNM> > strm_in2[PU];
#pragma HLS stream variable = strm_in2 depth = 512
#pragma HLS array_partition variable = strm_in2 complete
#pragma HLS resource variable = strm_in2 core = FIFO_BRAM
    hls::stream<ap_int<WData * CHNM> > strm_in3[PU];
#pragma HLS stream variable = strm_in3 depth = 512
#pragma HLS array_partition variable = strm_in3 complete
#pragma HLS resource variable = strm_in3 core = FIFO_BRAM

#ifndef __SYNTHESIS__
    std::cout << "loading data" << std::endl;
#endif

    loadData3PU<CHNM, WData>(similarity_type, vertex_num, edge_num, dataIn00, dataIn01, dataIn02, dataIn03, dataIn10,
                             dataIn11, dataIn12, dataIn13, dataIn20, dataIn21, dataIn22, dataIn23, strm_in0, strm_in1,
                             strm_in2, strm_in3);

    hls::stream<ap_int<WData> > row_strm0;
#pragma HLS stream variable = row_strm0 depth = 8
#pragma HLS resource variable = row_strm0 core = FIFO_SRL
    hls::stream<float> similarity_strm0;
#pragma HLS stream variable = similarity_strm0 depth = 8
#pragma HLS resource variable = similarity_strm0 core = FIFO_SRL
    hls::stream<bool> end_strm0;
#pragma HLS stream variable = end_strm0 depth = 8
#pragma HLS resource variable = end_strm0 core = FIFO_SRL

#ifndef __SYNTHESIS__
    std::cout << "processing similarity" << std::endl;
#endif

    xf::graph::denseSimilarity<CHNM, PU, WData, RAM_SZ>(config_strm, sourceWeight, strm_in0, strm_in1, strm_in2,
                                                        strm_in3, row_strm0, similarity_strm0, end_strm0);

#ifndef __SYNTHESIS__
    std::cout << "sorting for topK result" << std::endl;
#endif

    xf::graph::sortTopK<float, ap_int<32>, MAXK>(row_strm0, similarity_strm0, end_strm0, resultID, similarity, end_strm,
                                                 k, true);
}

} // namespace internal

/**
 * @brief similarity function for dense graph. It support both Jaccard and Cosine Similarity.
 *
 * @tparam CHNM the channel number of input data
 * @tparam WData the width of input data
 * @tparam RAM_SZ the log size of internal URAM
 * @tparam MAXK the max supporting number of insert sort function
 *
 * @param config the control parameter of the primitive which contains: sourceNUM, similarityType, dataType,
 * startID, rowNUM and colNUM of each processing unit(PU)
 * @param sourceWeight input weight as source vertex for computing similarity
 * @param sourceCoeffs input coefficient as a scale factor for the source vertex
 * @param dataIn00 input muti-channel data from HBM0 for PU0
 * @param dataIn01 input muti-channel data from HBM1 for PU0
 * @param dataIn02 input muti-channel data from HBM2 for PU0
 * @param dataIn03 input muti-channel data from HBM3 for PU0
 * @param dataIn10 input muti-channel data from HBM4 for PU1
 * @param dataIn11 input muti-channel data from HBM5 for PU1
 * @param dataIn12 input muti-channel data from HBM6 for PU1
 * @param dataIn13 input muti-channel data from HBM7 for PU1
 * @param dataIn20 input muti-channel data from HBM8 for PU2
 * @param dataIn21 input muti-channel data from HBM9 for PU2
 * @param dataIn22 input muti-channel data from HBM10 for PU2
 * @param dataIn23 input muti-channel data from HBM11 for PU2
 * @param rowID output result ID
 * @param similarity output similarity value corresponding to its ID
 */

template <int CHNM, int WData, int RAM_SZ, int MAXK>
void denseSimilarityCoeffs(ap_int<32>* config,
                           ap_int<32>* sourceWeight,
                           ap_int<32>* sourceCoeffs,

                           ap_int<32 * CHNM>* dataIn00,
                           ap_int<32 * CHNM>* dataIn01,
                           ap_int<32 * CHNM>* dataIn02,
                           ap_int<32 * CHNM>* dataIn03,

                           ap_int<32 * CHNM>* dataIn10,
                           ap_int<32 * CHNM>* dataIn11,
                           ap_int<32 * CHNM>* dataIn12,
                           ap_int<32 * CHNM>* dataIn13,

                           ap_int<32 * CHNM>* dataIn20,
                           ap_int<32 * CHNM>* dataIn21,
                           ap_int<32 * CHNM>* dataIn22,
                           ap_int<32 * CHNM>* dataIn23,

                           ap_int<32>* resultID,
                           float* similarity) {
#pragma HLS INLINE off

    const int PU = 3;

    ap_int<32> k;
    ap_int<32> source_num;
    ap_int<32> similarity_type;
    ap_int<32> data_type;

    ap_int<32> start_id[PU];
#pragma HLS ARRAY_PARTITION variable = start_id complete
    ap_int<32> vertex_num[PU];
#pragma HLS ARRAY_PARTITION variable = vertex_num complete
    ap_int<32> edge_num[PU];
#pragma HLS ARRAY_PARTITION variable = edge_num complete
    hls::stream<ap_int<32> > config_strm;
#pragma HLS stream variable = config_strm depth = 512
#pragma HLS resource variable = config_strm core = FIFO_BRAM

#ifndef __SYNTHESIS__
    std::cout << "loading config" << std::endl;
#endif

    xf::graph::internal::loadConfig<PU>(config, k, source_num, similarity_type, data_type, start_id, vertex_num,
                                        edge_num, config_strm);

    hls::stream<ap_int<WData> > source_weight;
#pragma HLS stream variable = source_weight depth = 512
#pragma HLS resource variable = source_weight core = FIFO_BRAM

#ifndef __SYNTHESIS__
    std::cout << "loading source" << std::endl;
#endif

    xf::graph::internal::loadSource<CHNM, WData>(similarity_type, source_num, sourceWeight, sourceCoeffs,
                                                 source_weight);

    hls::stream<ap_int<WData> > row_strm;
#pragma HLS stream variable = row_strm depth = 512
#pragma HLS resource variable = row_strm core = FIFO_BRAM
    hls::stream<float> similarity_strm;
#pragma HLS stream variable = similarity_strm depth = 512
#pragma HLS resource variable = similarity_strm core = FIFO_BRAM
    hls::stream<bool> end_strm;
#pragma HLS stream variable = end_strm depth = 512
#pragma HLS resource variable = end_strm core = FIFO_SRL

    xf::graph::internal::denseSimilarityTop3PU<CHNM, WData, RAM_SZ, MAXK>(
        k, source_num, similarity_type, data_type, start_id, vertex_num, edge_num, config_strm, source_weight,

        dataIn00, dataIn01, dataIn02, dataIn03, dataIn10, dataIn11, dataIn12, dataIn13, dataIn20, dataIn21, dataIn22,
        dataIn23,

        row_strm, similarity_strm, end_strm);

#ifndef __SYNTHESIS__
    std::cout << "returning results" << std::endl;
#endif

    xf::graph::internal::feedResult(row_strm, similarity_strm, end_strm, resultID, similarity);
}

} // namespace graph
} // namespace xf

#endif
