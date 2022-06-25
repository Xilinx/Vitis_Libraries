/*
 * Copyright 2022 Xilinx, Inc.
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

#ifndef HLS_CLUSTER_HISTOGRAM_CPP
#define HLS_CLUSTER_HISTOGRAM_CPP

#include "stdio.h"
#include "hls_cluster_histogram.hpp"

#define FLOAT_MAX 3.402823466e+38F

unsigned get_uram(unsigned idx0,
                  unsigned idx1,
#ifndef __SYNTHESIS__
                  std::vector<std::vector<ap_uint<64> > >& histograms
#else
                  ap_uint<64> histograms[4096][20]
#endif
                  ) {
    ap_uint<64> uram_tmp = histograms[idx0][idx1 / 2];
    return idx1 % 2 == 0 ? uram_tmp.range(31, 0) : uram_tmp.range(63, 32);
}

inline float compute_8(float in[8]) {
    float tmp_x0 = in[0] + in[1];
    float tmp_x1 = in[2] + in[3];
    float tmp_x2 = in[4] + in[5];
    float tmp_x3 = in[6] + in[7];
    float tmp_x4 = tmp_x0 + tmp_x1;
    float tmp_x5 = tmp_x2 + tmp_x3;
    return tmp_x4 + tmp_x5;
}

void GetIdx(unsigned int numNonempty,
            unsigned int nonempty_histo[4096],
            unsigned int histo_size[4096],
            hls::stream<unsigned int>& stream_idx,
            hls::stream<unsigned int>& stream_a_size0) {
GETIDX:
    for (unsigned int i = 0; i < numNonempty; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 10000 max = 10000
#pragma HLS pipeline
        unsigned int idx = nonempty_histo[i];
        stream_idx.write(idx);
        unsigned int tmp = histo_size[idx];
        stream_a_size0.write(tmp);
    }
}

void GetA(bool isEntropy,
          unsigned int numNonempty,
          unsigned int histo_size[4096],
#ifndef __SYNTHESIS__
          std::vector<std::vector<ap_uint<64> > >& histograms,
#else
          ap_uint<64> histograms[4096][20],
#endif
          unsigned int histo_totalcnt[4096],
          unsigned int nonempty_histo[4096],
          hls::stream<unsigned int>& stream_idx,
          hls::stream<unsigned int>& stream_b_size0,
          hls::stream<unsigned int>& stream_b_size1,
          hls::stream<unsigned int>& stream_size0,
          hls::stream<unsigned int>& stream_a,
          hls::stream<unsigned int>& stream_a_size0,
          hls::stream<unsigned int>& stream_a_size1,
          hls::stream<unsigned int>& stream_a_count) {
    unsigned char count_a = 0;
    unsigned int count_context = 0;
    unsigned int idx;
    unsigned char a_size;
    unsigned int a_total_count;
    unsigned char size;
GETA_OUT:
    while (count_context < numNonempty) {
#pragma HLS LOOP_TRIPCOUNT min = 10000 max = 10000
#pragma HLS pipeline
        if (count_a == 0) {
            idx = stream_idx.read();
            a_size = stream_a_size0.read();
            stream_a_size1.write(a_size);
            if (!isEntropy) {
                unsigned char b_size = stream_b_size0.read();
                stream_b_size1.write(b_size);
                size = hls::max(a_size, b_size);
            } else {
                size = a_size;
            }
            // printf("GetA isEntropy=%d, idx=%d, count_context=%d, count_a=%d,
            // size=%d, a_size=%d\n",
            //  isEntropy, idx, count_context, count_a, size, a_size);
            stream_size0.write(size);
            a_total_count = histo_totalcnt[idx];
            stream_a_count.write(a_total_count);
        }
        unsigned int tmp = get_uram(idx, count_a, histograms); // histograms[idx][count_a];
        stream_a.write(tmp);
        count_a++;
        if (count_a == a_size) {
            count_a = 0;
            count_context++;
        }
    }
}

void GetB(bool isEntropy,
          unsigned int numNonempty,
          unsigned int refSize,
          unsigned int ref_histo[40],
          unsigned int ref_totalcount,
          hls::stream<unsigned int>& stream_b,
          hls::stream<unsigned int>& stream_b_size0,
          hls::stream<unsigned int>& stream_b_count) {
    unsigned int count_context = 0;
    unsigned char count_b = 0;
    unsigned char b_size;
    unsigned int b_total_count;
    if (!isEntropy) {
    GETB_OUT:
        while (count_context < numNonempty) {
#pragma HLS LOOP_TRIPCOUNT min = 10000 max = 10000
            if (count_b == 0) {
                b_size = refSize;
                stream_b_size0.write(b_size);
                b_total_count = ref_totalcount;
                stream_b_count.write(b_total_count);
            }
            stream_b.write(ref_histo[count_b]);
            count_b++;
            if (count_b == b_size) {
                count_b = 0;
                count_context++;
            }
        }
    }
}

void DoHistogramDistanceEntropy(bool isEntropy,
                                unsigned int numNonempty,
                                hls::stream<unsigned int>& stream_size0,
                                hls::stream<unsigned int>& stream_size1,
                                hls::stream<unsigned int>& stream_size2,
                                hls::stream<unsigned int>& stream_a,
                                hls::stream<unsigned int>& stream_a_size1,
                                hls::stream<unsigned int>& stream_a_count,
                                hls::stream<unsigned int>& stream_b,
                                hls::stream<unsigned int>& stream_b_size1,
                                hls::stream<unsigned int>& stream_b_count,
                                hls::stream<float>& stream_dist_total,
                                hls::stream<float>& stream_dist) {
    int count_debug = 0;
    unsigned int count_context = 0;
    unsigned char count_s = 0;
    unsigned char a_size;
    unsigned int a_total_count;
    unsigned char b_size;
    unsigned int b_total_count;
    unsigned int sum_count = 0;
    unsigned char size = 0;
    float total;
    float totallog2;
DISTANCE_OUT:
    while (count_context < numNonempty) {
#pragma HLS LOOP_TRIPCOUNT min = 10000 max = 10000
        if (count_s == 0) {
            sum_count = 0;
            a_size = stream_a_size1.read();
            a_total_count = stream_a_count.read();
            if (!isEntropy) {
                b_size = stream_b_size1.read();
                b_total_count = stream_b_count.read();
                total = a_total_count + b_total_count;
            } else {
                total = a_total_count;
            }
            totallog2 = total == 0 ? 0 : hls::log2(total);
            size = stream_size0.read();
            stream_size1.write(size);
            stream_size2.write(size);
            // printf("DoHist count_context=%d, count_s=%d, size=%d\n", count_context,
            // count_s, size);
        }
        unsigned int counts;
        if (!isEntropy) {
            unsigned int a_counts = a_size > count_s ? stream_a.read() : 0;
            unsigned int b_counts = b_size > count_s ? stream_b.read() : 0;
            counts = a_counts + b_counts;
        } else {
            unsigned int tmp = stream_a.read();
            counts = tmp;
        }
        float countlog2 = counts == 0 ? 0 : hls::log2((float)counts);
        bool flag = counts == total;
        sum_count += flag ? 0 : counts;
        float tmp = flag ? 0 : counts * countlog2;
        stream_dist.write(tmp);
        count_s++;
        if (count_s == size) {
            // printf("DoHist Write stream_dist_total %d %d %d\n", count_context,
            // count_s, size);
            count_debug++;
            stream_dist_total.write(sum_count * totallog2);
            count_s = 0;
            count_context++;
        }
    }
    // printf("stream_dist_total in=%d\n", count_debug);
}

void GroupSum(unsigned int numNonempty,
              hls::stream<unsigned int>& stream_size1,
              hls::stream<float>& stream_dist,
              hls::stream<float>& stream_sum) {
    int count_debug = 0;
    unsigned int count_context = 0;
    unsigned char count_s = 0;
    float sum_array[8];
    unsigned char size;
GROUPSUM_OUT:
    while (count_context < numNonempty) {
#pragma HLS LOOP_TRIPCOUNT min = 10000 max = 10000
        if (count_s == 0) {
            size = stream_size1.read();
        }
        unsigned char idx = count_s % 8;
        sum_array[idx] = stream_dist.read();
        if (idx == 7) {
            float sum_part = compute_8(sum_array);
            stream_sum.write(sum_part);
            // printf("GroupSum Write stream_sum %d %d %d\n", count_context, count_s,
            // size);
            count_debug++;
        }
        count_s++;
        if (count_s == size) {
            count_s = 0;
            count_context++;
        }
    }
    // printf("stream_sum in=%d\n", count_debug);
}

void GetDist(bool isEntropy,
             unsigned int numNonempty,
             unsigned int j,
             float histo_entropy[4096],
             float ref_entropy,
             float dists[4096],
             unsigned int best[4096],
             unsigned int& largest_idx,
             hls::stream<unsigned int>& stream_size2,
             hls::stream<float>& stream_dist_total,
             hls::stream<float>& stream_sum) {
    int count_debug = 0;
    float dist_std;
    unsigned int count_context = 0;
    unsigned char count_s = 0;
    largest_idx = 0;
    unsigned char size;
    float sum_dist = 0;
    float reg_curr;
    float reg0;
    float reg1;
    float reg2;
    unsigned short addr_curr = 0;
    unsigned short addr0 = 0xffff;
    unsigned short addr1 = 0xffff;
    unsigned short addr2 = 0xffff;
GET_DIST_OUT:
    while (count_context < numNonempty) {
#pragma HLS LOOP_TRIPCOUNT min = 1250 max = 1250
        if (count_s == 0) {
            size = stream_size2.read() / 8;
            sum_dist = 0;
            // printf("GetDist count_context=%d, count_s=%d, size=%d\n",
            // count_context, count_s, size);
        }
        // printf("GetDist count_context=%d, count_s=%d, size=%d\n", count_context,
        // count_s, size);
        sum_dist += stream_sum.read();
        // printf("GetDist Read stream_sum %d %d %d\n", count_context, count_s,
        // size);
        if (count_s == size - 1) {
            count_debug++;
            // printf("GetDist Read stream_dist_total %d %d %d\n", count_context,
            // count_s, size);
            float tmp = stream_dist_total.read();
            dist_std = tmp - sum_dist;

            // update dist, may update same addess
            addr_curr = count_context;
            if (addr_curr == addr0) {
                reg_curr = reg0;
            } else if (addr_curr == addr1) {
                reg_curr = reg1;
            } else if (addr_curr == addr2) {
                reg_curr = reg2;
            } else {
                reg_curr = dists[addr_curr];
            }

            float tmp_largest = dists[largest_idx];
            if (!isEntropy) {
                if (dist_std - histo_entropy[addr_curr] - ref_entropy < reg_curr) {
                    best[addr_curr] = j;
                    reg_curr = dist_std - histo_entropy[addr_curr] - ref_entropy;
                }
            } else {
                reg_curr = dist_std;
            }
            if (reg_curr > tmp_largest) {
                largest_idx = addr_curr;
            }

            dists[addr_curr] = reg_curr;
            reg2 = reg1;
            reg1 = reg0;
            reg0 = reg_curr;
            addr2 = addr1;
            addr1 = addr0;
            addr0 = addr_curr;
        }
        count_s++;
        if (count_s == size) {
            count_s = 0;
            count_context++;
        }
    }
    // printf("stream_sum out=%d\n", count_debug);
}

void hls_HistogramDistance(bool isEntropy,
                           unsigned int numNonempty,
                           unsigned int j,
                           unsigned int histo_size[4096],
#ifndef __SYNTHESIS__
                           std::vector<std::vector<ap_uint<64> > >& histograms,
#else
                           ap_uint<64> histograms[4096][20],
#endif
                           unsigned int histo_totalcnt[4096],
                           float histo_entropy[4096],
                           unsigned int nonempty_histo[4096],
                           unsigned int refSize,
                           unsigned int ref_histo[40],
                           unsigned int ref_totalcount,
                           float ref_entropy,
                           float dists[4096],
                           unsigned int best[4096],
                           unsigned int& largest_idx) {
    hls::stream<unsigned int> stream_size0("stream_size0");
#pragma HLS STREAM variable = stream_size0 depth = 64
    hls::stream<unsigned int> stream_size1("stream_size1");
#pragma HLS STREAM variable = stream_size1 depth = 64
    hls::stream<unsigned int> stream_size2("stream_size2");
#pragma HLS STREAM variable = stream_size2 depth = 64

    hls::stream<unsigned int> stream_a("stream_a");
#pragma HLS STREAM variable = stream_a depth = 64
    hls::stream<unsigned int> stream_a_size0("stream_a_size0");
#pragma HLS STREAM variable = stream_a_size0 depth = 64
    hls::stream<unsigned int> stream_a_size1("stream_a_size1");
#pragma HLS STREAM variable = stream_a_size1 depth = 64
    hls::stream<unsigned int> stream_a_count("stream_a_count");
#pragma HLS STREAM variable = stream_a_count depth = 64

    hls::stream<unsigned int> stream_b("stream_b");
#pragma HLS STREAM variable = stream_b depth = 64
    hls::stream<unsigned int> stream_b_size0("stream_b_size0");
#pragma HLS STREAM variable = stream_b_size0 depth = 64
    hls::stream<unsigned int> stream_b_size1("stream_b_size1");
#pragma HLS STREAM variable = stream_b_size1 depth = 64
    hls::stream<unsigned int> stream_b_count("stream_b_count");
#pragma HLS STREAM variable = stream_b_count depth = 64

    hls::stream<float> stream_dist_total("stream_dist_total");
#pragma HLS STREAM variable = stream_dist_total depth = 64
    hls::stream<float> stream_dist("stream_dist");
#pragma HLS STREAM variable = stream_dist depth = 64
    hls::stream<float> stream_sum("stream_sum");
#pragma HLS STREAM variable = stream_sum depth = 64
    hls::stream<unsigned int> stream_idx("stream_idx");
#pragma HLS STREAM variable = stream_idx depth = 64

// clang-format on
#pragma HLS dataflow

    GetIdx(numNonempty, nonempty_histo, histo_size, stream_idx, stream_a_size0);

    GetB(isEntropy, numNonempty, refSize, ref_histo, ref_totalcount, stream_b, stream_b_size0, stream_b_count);

    GetA(isEntropy, numNonempty, histo_size, histograms, histo_totalcnt, nonempty_histo, stream_idx, stream_b_size0,
         stream_b_size1, stream_size0, stream_a, stream_a_size0, stream_a_size1, stream_a_count);

    DoHistogramDistanceEntropy(isEntropy, numNonempty, stream_size0, stream_size1, stream_size2, stream_a,
                               stream_a_size1, stream_a_count, stream_b, stream_b_size1, stream_b_count,
                               stream_dist_total, stream_dist);

    GroupSum(numNonempty, stream_size1, stream_dist, stream_sum);

    GetDist(isEntropy, numNonempty, j, histo_entropy, ref_entropy, dists, best, largest_idx, stream_size2,
            stream_dist_total, stream_sum);
}

int hls_ClusterHisgtogram(unsigned int largest_idx,
                          unsigned int numNonempty,
                          unsigned int nonempty_histo[4096],
                          unsigned int histo_totalcnt[4096],
                          unsigned int histo_size[4096],
#ifndef __SYNTHESIS__
                          std::vector<std::vector<ap_uint<64> > >& histograms,
#else
                          ap_uint<64> histograms[4096][20],
#endif
                          unsigned int histo_size_clusd[128],
#ifndef __SYNTHESIS__
                          std::vector<std::vector<unsigned int> >& histograms_clusd,
#else
                          unsigned int histograms_clusd[128][40],
#endif
                          unsigned char histogram_symbols[4096]) {
    unsigned char max_histograms = 128;
    float min_distance = 64.0;
    unsigned int best[4096];
    float dists[4096];
    float entropy[4096];
    unsigned int total_count[4096];
    float out_entropy[4096];
    float histo_entropy[4096];

    unsigned int size_b = 0;
    unsigned int total_count_b = 0;
    float hls_entropy_b = 0;
    unsigned int data_b[40];
    unsigned int tmp_largest_idx;

    hls_HistogramDistance(true, numNonempty, 0, histo_size, histograms, histo_totalcnt, histo_entropy, nonempty_histo,
                          size_b, data_b, total_count_b, hls_entropy_b, entropy, best, tmp_largest_idx);

INIT_1:
    for (unsigned int i = 0; i < numNonempty; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 4096 max = 4096
#pragma HLS pipeline
        unsigned int idx = nonempty_histo[i];
        histo_entropy[idx] = entropy[i];
        best[i] = 0;
        dists[i] = FLOAT_MAX;
    }

    unsigned int numHisto_clusd = 0;
    unsigned int max_count = hls::min((int)max_histograms, (int)numNonempty);
    largest_idx = nonempty_histo[largest_idx];
    dists[largest_idx] = FLOAT_MAX;
    unsigned int idx = largest_idx;
FIRST_SCAN:
    while (numHisto_clusd < max_count && dists[largest_idx] >= min_distance) {
#pragma HLS LOOP_TRIPCOUNT min = 128 max = 128
        histogram_symbols[idx] = numHisto_clusd;
        unsigned char data_size = histo_size[idx];
    GEN_REF1:
        for (unsigned char k = 0; k < data_size; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 40 max = 40
            histograms_clusd[numHisto_clusd][k] = get_uram(idx, k, histograms); // histograms[idx][k];
            data_b[k] = get_uram(idx, k, histograms);                           // histograms[idx][k];
        }
        histo_size_clusd[numHisto_clusd] = data_size;
        size_b = data_size;
        total_count[numHisto_clusd] = histo_totalcnt[idx];
        total_count_b = histo_totalcnt[idx];
        out_entropy[numHisto_clusd] = histo_entropy[idx];
        hls_entropy_b = histo_entropy[idx];
// printf("push idx=%d, size_b=%d, total_count_b=%d, hls_entropy_b=%f\n",
//  idx, size_b, total_count_b, hls_entropy_b);
#pragma HLS ALLOCATION function instances = hls_HistogramDistance limit = 1
        hls_HistogramDistance(false, numNonempty, 0, histo_size, histograms, histo_totalcnt, entropy, nonempty_histo,
                              size_b, data_b, total_count_b, hls_entropy_b, dists, best, largest_idx);
        idx = nonempty_histo[largest_idx];
        numHisto_clusd++;
    }

INIT_2:
    for (unsigned int j = 0; j < numNonempty; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 4096 max = 4096
#pragma HLS pipeline
        best[j] = 0;
        dists[j] = FLOAT_MAX;
    }

SECOND_SCAN:
    for (unsigned int j = 0; j < numHisto_clusd; j++) {
#pragma HLS LOOP_TRIPCOUNT min = 128 max = 128
        size_b = histo_size_clusd[j];
        total_count_b = total_count[j];
        hls_entropy_b = out_entropy[j];
    GEN_REF2:
        for (unsigned char k = 0; k < size_b; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 40 max = 40
#pragma HLS pipeline
            data_b[k] = histograms_clusd[j][k];
        }
        hls_HistogramDistance(false, numNonempty, j, histo_size, histograms, histo_totalcnt, entropy, nonempty_histo,
                              size_b, data_b, total_count_b, hls_entropy_b, dists, best, largest_idx);
    }

OUTPUT1:
    for (unsigned int i = 0; i < numNonempty; i++) {
#pragma HLS LOOP_TRIPCOUNT min = 4096 max = 4096
        unsigned int idx_in = nonempty_histo[i];
        unsigned int idx_out = best[i];
        unsigned int other_data_size = histo_size[idx_in];
        unsigned int total_count = histo_totalcnt[idx_in];
        if (other_data_size > histo_size_clusd[idx_out]) {
            histo_size_clusd[idx_out] = other_data_size;
        }
    OUTPUT2:
        for (unsigned char k = 0; k < other_data_size; ++k) {
#pragma HLS LOOP_TRIPCOUNT min = 40 max = 40
            unsigned int data_tmp = get_uram(idx_in, k, histograms); // histograms[idx_in][k];
            histograms_clusd[idx_out][k] += data_tmp * numHisto_clusd;
        }
        histogram_symbols[idx_in] = idx_out;
    }
    return numHisto_clusd;
}

// clang-format off
void hls_fastclusterHistogram_wrapper(
    unsigned int largest_idx, 
    unsigned int numNonempty,
    unsigned int nonempty_histo[4096],
    unsigned int numHisto,
    unsigned int histo_totalcnt[4096],
    unsigned int histo_size[4096], 
#ifndef __SYNTHESIS__
    std::vector<std::vector<ap_uint<64> > >& histograms,
#else
    ap_uint<64> histograms[4096][20],
#endif
    unsigned int& numHisto_clusd,
    unsigned int histo_size_clusd[128],
#ifndef __SYNTHESIS__
    std::vector<std::vector<unsigned int> >& histograms_clusd,
#else
    unsigned int histograms_clusd[128][40],
#endif
    unsigned char histogram_symbols[4096]
) {
  //printf("[KERNEL] hls_fastclusterHistogram_wrapper in %d %d %d %d\n", largest_idx,
  //        numNonempty, numHisto, numHisto_clusd);
    // clang-format on
    if (numHisto > 1) {
        if (numNonempty == 0) {
            numHisto_clusd = 1;
        } else {
            numHisto_clusd = hls_ClusterHisgtogram(largest_idx, numNonempty, nonempty_histo, histo_totalcnt, histo_size,
                                                   histograms, histo_size_clusd, histograms_clusd, histogram_symbols);
            // printf("[KERNEL]size= %d\n", numNonempty);
            // for(int i=0; i<numNonempty; i++) {
            //  printf("[KERNEL] hls_fastclusterHistogram_wrapper %d %d %d %d\n",
            //          i, histo_totalcnt[i], histo_size[i], histograms[i][1]);
            //}
        }
    } else {
        numHisto_clusd = 1;
        histo_size_clusd[0] = histo_size[0];
    HISTO:
        for (unsigned char k = 0; k < histo_size[0]; k++) {
#pragma HLS LOOP_TRIPCOUNT min = 40 max = 40
#pragma HLS pipeline
            histograms_clusd[0][k] = get_uram(0, k, histograms); // histograms[0][k];
        }
    }
    // printf("[KERNEL] hls_fastclusterHistogram_wrapper out %d %d %d %d\n", largest_idx,
    //        numNonempty, numHisto, numHisto_clusd);
}

void buildCTXHistogram(uint32_t numHisto_ptr,
                       uint8_t* ctx_map_ptr,

                       uint32_t& numHisto_clusd_ptr,
                       int32_t* histograms_clusdin_ptr,
                       uint32_t& histo_size_clusdin_ptr) {
    ap_uint<32> num_histograms = numHisto_clusd_ptr;
    uint32_t entry_bits;
    uint32_t floor_log2 = 32 - num_histograms.countLeadingZeros() - 1;
    if ((num_histograms & (num_histograms - 1)) == 0) {
        entry_bits = floor_log2; // power of two
    } else {
        entry_bits = floor_log2 + 1;
    }
    if (numHisto_ptr > 1 && entry_bits >= 4) {
        uint32_t max_tok = 0;
        for (uint32_t k = 0; k < numHisto_ptr; ++k) {
#pragma HLS PIPELINE II = 1
            ap_uint<32> value = ctx_map_ptr[k];
            uint32_t tok;
            if (value < 16) {
                tok = value;
            } else {
                uint32_t n = 32 - value.countLeadingZeros() - 1;
                uint32_t m = value - (1 << n);
                tok = 16 + ((n - 4) << 2) + (m >> (n - 2));
            }
            max_tok = tok > max_tok ? tok : max_tok;
            ++histograms_clusdin_ptr[tok];
        }
        histo_size_clusdin_ptr = (max_tok + 8) / 8 * 8;
    }
}

void load_histo(uint32_t numNonempty_ptr,
                uint32_t* nonempty_histo_ptr,
                int32_t* histograms_ptr,

                uint32_t nonempty_histo_tmp[4096],
#ifndef __SYNTHESIS__
                std::vector<std::vector<ap_uint<64> > >& histograms_tmp
#else
                ap_uint<64> histograms_tmp[4096][20]
#endif
                ) {
    for (int i = 0; i < 4096; i++) {
#pragma HLS PIPELINE II = 1
        for (int j = 0; j < 20; j++) {
#pragma HLS UNROLL
            histograms_tmp[i][j] = 0;
        }
    }

    for (int i = 0; i < numNonempty_ptr; i++) {
        uint32_t reg = nonempty_histo_ptr[i];
        nonempty_histo_tmp[i] = reg;
        for (ap_uint<8> j = 0; j < 20; j++) {
#pragma HLS PIPELINE II = 1
            ap_uint<64> val;
            val.range(31, 0) = histograms_ptr[reg * 40 + j * 2];
            val.range(63, 32) = histograms_ptr[reg * 40 + j * 2 + 1];
            histograms_tmp[reg][j] = val;
        }
    }
}

void load_nonempty(uint32_t* nonempty_histo_ptr, uint32_t nonempty_histo_tmp[4096]) {
    for (int i = 0; i < 4096; i++) {
#pragma HLS PIPELINE II = 1
        nonempty_histo_tmp[i] = nonempty_histo_ptr[i];
    }
}

void load_total_cnt(uint32_t* histo_totalcnt_ptr, uint32_t histo_totalcnt_tmp[4096]) {
    for (int i = 0; i < 4096; i++) {
#pragma HLS PIPELINE II = 1
        histo_totalcnt_tmp[i] = histo_totalcnt_ptr[i];
    }
}

void load_size(uint32_t* histo_size_ptr, uint32_t histo_size_tmp[4096]) {
    for (int i = 0; i < 4096; i++) {
#pragma HLS PIPELINE II = 1
        histo_size_tmp[i] = histo_size_ptr[i];
    }
}

void memset_histo_clusdin(int32_t histograms_clusdin_tmp[40]) {
    for (int i = 0; i < 40; i++) {
#pragma HLS UNROLL
        histograms_clusdin_tmp[i] = 0;
    }
}

void memset_histo_clusd(
#ifndef __SYNTHESIS__
    std::vector<std::vector<unsigned> >& histograms_clusd_tmp
#else
    unsigned histograms_clusd_tmp[128][40]
#endif
    ) {
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 40; j++) {
#pragma HLS PIPELINE II = 1
            histograms_clusd_tmp[i][j] = 0;
        }
    }
}

void memset_ctx_map(uint8_t ctx_map_tmp[4096]) {
    for (int i = 0; i < 4096; i++) {
#pragma HLS PIPELINE II = 1
        ctx_map_tmp[i] = 0;
    }
}

void load_data(uint32_t numNonempty_ptr,
               int32_t* histograms_ptr,
               uint32_t* nonempty_histo_ptr,
               uint32_t* histo_totalcnt_ptr,
               uint32_t* histo_size_ptr,
#ifndef __SYNTHESIS__
               std::vector<std::vector<ap_uint<64> > >& histograms_tmp,
#else
               ap_uint<64> histograms_tmp[4096][20],
#endif
               uint32_t nonempty_histo_tmp[4096],
               uint32_t histo_totalcnt_tmp[4096],
               uint32_t histo_size_tmp[4096],
               int32_t histograms_clusdin_tmp[40],
#ifndef __SYNTHESIS__
               std::vector<std::vector<unsigned> >& histograms_clusd_tmp,
#else
               unsigned histograms_clusd_tmp[128][40],
#endif
               uint8_t ctx_map_tmp[4096]) {
#pragma HLS DATAFLOW
    load_histo(numNonempty_ptr, nonempty_histo_ptr, histograms_ptr, nonempty_histo_tmp, histograms_tmp);

    load_total_cnt(histo_totalcnt_ptr, histo_totalcnt_tmp);

    load_size(histo_size_ptr, histo_size_tmp);

    memset_histo_clusdin(histograms_clusdin_tmp);

    memset_histo_clusd(histograms_clusd_tmp);

    memset_ctx_map(ctx_map_tmp);
}

void write_histo_clusd(
#ifndef __SYNTHESIS__
    std::vector<std::vector<unsigned> >& histograms_clusd_tmp,
#else
    unsigned histograms_clusd_tmp[128][40],
#endif
    int32_t* histograms_clusd_ptr) {
    for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 40; j++) {
#pragma HLS PIPELINE II = 1
            histograms_clusd_ptr[i * 40 + j] = histograms_clusd_tmp[i][j];
        }
    }
}

void write_size_clusd(uint32_t histo_size_clusd_tmp[128], uint32_t* histo_size_clusd_ptr) {
    for (int i = 0; i < 128; i++) {
#pragma HLS PIPELINE II = 1
        histo_size_clusd_ptr[i] = histo_size_clusd_tmp[i];
    }
}

void write_ctx_map(uint8_t ctx_map_tmp[4096], uint8_t* ctx_map_ptr) {
    for (int i = 0; i < 4096; i++) {
#pragma HLS PIPELINE II = 1
        ctx_map_ptr[i] = ctx_map_tmp[i];
    }
}

void write_histo_clusdin(int32_t histograms_clusdin_tmp[40], int32_t* histograms_clusdin_ptr) {
    for (int i = 0; i < 40; i++) {
#pragma HLS PIPELINE II = 1
        histograms_clusdin_ptr[i] = histograms_clusdin_tmp[i];
    }
}

void write_data(
#ifndef __SYNTHESIS__
    std::vector<std::vector<unsigned> >& histograms_clusd_tmp,
#else
    unsigned histograms_clusd_tmp[128][40],
#endif
    uint32_t histo_size_clusd_tmp[128],
    uint8_t ctx_map_tmp[4096],
    int32_t histograms_clusdin_tmp[40],
    int32_t* histograms_clusd_ptr,
    uint32_t* histo_size_clusd_ptr,
    uint8_t* ctx_map_ptr,
    int32_t* histograms_clusdin_ptr) {
#pragma HLS DATAFLOW
    write_histo_clusd(histograms_clusd_tmp, histograms_clusd_ptr);

    write_size_clusd(histo_size_clusd_tmp, histo_size_clusd_ptr);

    write_ctx_map(ctx_map_tmp, ctx_map_ptr);

    write_histo_clusdin(histograms_clusdin_tmp, histograms_clusdin_ptr);
}

/**
 * @brief JXL ANS cluster Histogram kernel
 *
 * @param config                    configuration for the kernel.
 * @param histograms0_ptr           histograms for Block Context Map.
 * @param histo_totalcnt0_ptr       Count of context for histograms for Block Context Map.
 * @param histo_size0_ptr           size for each context
 * @param nonempty_histo0_ptr       indicate which context is empty
 * @param ctx_map0_ptr              the input context map
 * @param histograms_clusd0_ptr     the clustered histogram
 * @param histograms_clusdin0_ptr   the context for the clustered histogram
 * @param histograms1_ptr           histograms for Modular frame tree.
 * @param histo_totalcnt1_ptr       Count of context for histograms for Modular frame tree.
 * @param histo_size1_ptr           size for each context
 * @param nonempty_histo1_ptr       indicate which context is empty
 * @param ctx_map1_ptr              the input context map
 * @param histograms_clusd1_ptr     the clustered histogram
 * @param histograms_clusdin1_ptr   the context for the clustered histogram
 * @param histograms2_ptr           histograms for code from Modular frame.
 * @param histo_totalcnt2_ptr       Count of context for histograms for Modular frame.
 * @param histo_size2_ptr           size for each context
 * @param nonempty_histo2_ptr       indicate which context is empty
 * @param ctx_map2_ptr              the input context map
 * @param histograms_clusd2_ptr     the clustered histogram
 * @param histograms_clusdin2_ptr   the context for the clustered histogram
 * @param histograms3_ptr           histograms for coef orders.
 * @param histo_totalcnt3_ptr       Count of context for histograms for coef orders.
 * @param histo_size3_ptr           size for each context
 * @param nonempty_histo3_ptr       indicate which context is empty
 * @param ctx_map3_ptr              the input context map
 * @param histograms_clusd3_ptr     the clustered histogram
 * @param histograms_clusdin3_ptr   the context for the clustered histogram
 * @param histograms4_ptr           histograms for ac coefficients.
 * @param histo_totalcnt4_ptr       Count of context for histograms for ac coefficients.
 * @param histo_size4_ptr           size for each context
 * @param nonempty_histo4_ptr       indicate which context is empty
 * @param ctx_map4_ptr              the input context map
 * @param histograms_clusd4_ptr     the clustered histogram
 * @param histograms_clusdin4_ptr   the context for the clustered histogram
 */

// clang-format off
void hls_ANSclusterHistogram_core(
    uint32_t numNonempty_ptr,
    uint32_t* nonempty_histo_ptr,

    uint32_t lidx_ptr,
    uint32_t numHisto_ptr,
    uint32_t* histo_totalcnt_ptr,
    uint32_t* histo_size_ptr,
    int32_t* histograms_ptr,

    uint8_t* ctx_map_ptr, 
    uint32_t* histo_size_clusd_ptr,
    int32_t* histograms_clusd_ptr,

    int32_t* histograms_clusdin_ptr,
    uint32_t& numHisto_clusd_ptr, 
    uint32_t& histo_size_clusdin_ptr) {
// clang-format on
// No dataflow, sequentially run
#ifndef __SYNTHESIS__
    std::vector<std::vector<ap_uint<64> > > histograms_tmp(4096, std::vector<ap_uint<64> >(20));
    uint32_t* nonempty_histo_tmp = (uint32_t*)malloc(4096 * sizeof(uint32_t));
    uint32_t* histo_totalcnt_tmp = (uint32_t*)malloc(4096 * sizeof(uint32_t));
    uint32_t* histo_size_tmp = (uint32_t*)malloc(4096 * sizeof(uint32_t));

    uint32_t* histo_size_clusd_tmp = (uint32_t*)malloc(128 * sizeof(uint32_t));
    std::vector<std::vector<unsigned> > histograms_clusd_tmp(128, std::vector<unsigned>(40));
    uint8_t* ctx_map_tmp = (uint8_t*)malloc(4096 * sizeof(uint8_t));

    int32_t* histograms_clusdin_tmp = (int32_t*)malloc(40 * sizeof(int32_t));
#else
    ap_uint<64> histograms_tmp[4096][20];
#pragma HLS BIND_STORAGE impl = URAM variable = histograms_tmp
#pragma HLS ARRAY_PARTITION variable = histograms_tmp complete dim = 2
    uint32_t nonempty_histo_tmp[4096];
#pragma HLS BIND_STORAGE impl = URAM variable = nonempty_histo_tmp
    uint32_t histo_totalcnt_tmp[4096];
#pragma HLS BIND_STORAGE impl = URAM variable = histo_totalcnt_tmp
    uint32_t histo_size_tmp[4096];
#pragma HLS BIND_STORAGE impl = URAM variable = histo_size_tmp
    unsigned histograms_clusd_tmp[128][40];
#pragma HLS BIND_STORAGE impl = LUTRAM variable = histograms_clusd_tmp
    uint32_t histo_size_clusd_tmp[128];
#pragma HLS BIND_STORAGE impl = LUTRAM variable = histo_size_clusd_tmp
    uint8_t ctx_map_tmp[4096];
#pragma HLS BIND_STORAGE impl = URAM variable = ctx_map_tmp
    int32_t histograms_clusdin_tmp[40];
#pragma HLS ARRAY_PARTITION variable = histograms_clusdin_tmp complete dim = 0
#endif

    load_data(numNonempty_ptr, histograms_ptr, nonempty_histo_ptr, histo_totalcnt_ptr, histo_size_ptr, histograms_tmp,
              nonempty_histo_tmp, histo_totalcnt_tmp, histo_size_tmp, histograms_clusdin_tmp, histograms_clusd_tmp,
              ctx_map_tmp);

    hls_fastclusterHistogram_wrapper(lidx_ptr, numNonempty_ptr, nonempty_histo_tmp, numHisto_ptr, histo_totalcnt_tmp,
                                     histo_size_tmp, histograms_tmp, numHisto_clusd_ptr, histo_size_clusd_tmp,
                                     histograms_clusd_tmp, ctx_map_tmp);

    buildCTXHistogram(numHisto_ptr, ctx_map_tmp, numHisto_clusd_ptr, histograms_clusdin_tmp, histo_size_clusdin_ptr);

    write_data(histograms_clusd_tmp, histo_size_clusd_tmp, ctx_map_tmp, histograms_clusdin_tmp, histograms_clusd_ptr,
               histo_size_clusd_ptr, ctx_map_ptr, histograms_clusdin_ptr);
}

namespace xf {
namespace codec {

/**
* @brief JXL ANS cluster Histogram kernel
*
* @param config                    configuration for the kernel.
* @param histograms0_ptr           histograms for Block Context Map.
* @param histo_totalcnt0_ptr       Count of context for histograms for Block Context Map.
* @param histo_size0_ptr           size for each context
* @param nonempty_histo0_ptr       indicate which context is empty
* @param ctx_map0_ptr              the input context map
* @param histograms_clusd0_ptr     the clustered histogram
* @param histograms_clusdin0_ptr   the context for the clustered histogram
* @param histograms1_ptr           histograms for Modular frame tree.
* @param histo_totalcnt1_ptr       Count of context for histograms for Modular frame tree.
* @param histo_size1_ptr           size for each context
* @param nonempty_histo1_ptr       indicate which context is empty
* @param ctx_map1_ptr              the input context map
* @param histograms_clusd1_ptr     the clustered histogram
* @param histograms_clusdin1_ptr   the context for the clustered histogram
* @param histograms2_ptr           histograms for code from Modular frame.
* @param histo_totalcnt2_ptr       Count of context for histograms for Modular frame.
* @param histo_size2_ptr           size for each context
* @param nonempty_histo2_ptr       indicate which context is empty
* @param ctx_map2_ptr              the input context map
* @param histograms_clusd2_ptr     the clustered histogram
* @param histograms_clusdin2_ptr   the context for the clustered histogram
* @param histograms3_ptr           histograms for coef orders.
* @param histo_totalcnt3_ptr       Count of context for histograms for coef orders.
* @param histo_size3_ptr           size for each context
* @param nonempty_histo3_ptr       indicate which context is empty
* @param ctx_map3_ptr              the input context map
* @param histograms_clusd3_ptr     the clustered histogram
* @param histograms_clusdin3_ptr   the context for the clustered histogram
* @param histograms4_ptr           histograms for ac coefficients.
* @param histo_totalcnt4_ptr       Count of context for histograms for ac coefficients.
* @param histo_size4_ptr           size for each context
* @param nonempty_histo4_ptr       indicate which context is empty
* @param ctx_map4_ptr              the input context map
* @param histograms_clusd4_ptr     the clustered histogram
* @param histograms_clusdin4_ptr   the context for the clustered histogram
*/

// clang-format off
 extern "C" void JxlEnc_ans_clusterHistogram(
    uint32_t* config,

    int32_t* histograms0_ptr,
    uint32_t* histo_totalcnt0_ptr,
    uint32_t* histo_size0_ptr,

    uint32_t* nonempty_histo0_ptr,

    uint8_t* ctx_map0_ptr,

    int32_t* histograms_clusd0_ptr,
    uint32_t* histo_size_clusd0_ptr,

    int32_t* histograms_clusdin0_ptr,
    //====================
    int32_t* histograms1_ptr,
    uint32_t* histo_totalcnt1_ptr,
    uint32_t* histo_size1_ptr,

    uint32_t* nonempty_histo1_ptr,

    uint8_t* ctx_map1_ptr,

    int32_t* histograms_clusd1_ptr,
    uint32_t* histo_size_clusd1_ptr,

    int32_t* histograms_clusdin1_ptr,
    //======================
    int32_t* histograms2_ptr,
    uint32_t* histo_totalcnt2_ptr,
    uint32_t* histo_size2_ptr,

    uint32_t* nonempty_histo2_ptr,

    uint8_t* ctx_map2_ptr,

    int32_t* histograms_clusd2_ptr,
    uint32_t* histo_size_clusd2_ptr,

    int32_t* histograms_clusdin2_ptr,
    //======================
    int32_t* histograms3_ptr,
    uint32_t* histo_totalcnt3_ptr,
    uint32_t* histo_size3_ptr,

    uint32_t* nonempty_histo3_ptr,

    uint8_t* ctx_map3_ptr,

    int32_t* histograms_clusd3_ptr,
    uint32_t* histo_size_clusd3_ptr,

    int32_t* histograms_clusdin3_ptr,
    //======================
    int32_t* histograms4_ptr,
    uint32_t* histo_totalcnt4_ptr,
    uint32_t* histo_size4_ptr,

    uint32_t* nonempty_histo4_ptr,

    uint8_t* ctx_map4_ptr,

    int32_t* histograms_clusd4_ptr,
    uint32_t* histo_size_clusd4_ptr,

    int32_t* histograms_clusdin4_ptr
) {
#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histogram_gmem port = histograms0_ptr depth = 163840

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histogram_gmem port = histograms1_ptr depth = 163840

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histogram_gmem port = histograms2_ptr depth = 163840

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histogram_gmem port = histograms3_ptr depth = 163840

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histogram_gmem port = histograms4_ptr depth = 163840

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histocnt_gmem port = histo_totalcnt0_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histocnt_gmem port = histo_totalcnt1_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histocnt_gmem port = histo_totalcnt2_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histocnt_gmem port = histo_totalcnt3_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histocnt_gmem port = histo_totalcnt4_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histosize_gmem port = histo_size0_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histosize_gmem port = histo_size1_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histosize_gmem port = histo_size2_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histosize_gmem port = histo_size3_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = histosize_gmem port = histo_size4_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = nonempty_gmem port = nonempty_histo0_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = nonempty_gmem port = nonempty_histo1_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = nonempty_gmem port = nonempty_histo2_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = nonempty_gmem port = nonempty_histo3_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 1 num_read_outstanding = \
    8 max_write_burst_length = 2 max_read_burst_length = 64 bundle = nonempty_gmem port = nonempty_histo4_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = ctx_gmem port = ctx_map0_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = ctx_gmem port = ctx_map1_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = ctx_gmem port = ctx_map2_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = ctx_gmem port = ctx_map3_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = ctx_gmem port = ctx_map4_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusd_gmem port = histograms_clusd0_ptr depth = 5120

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusd_gmem port = histograms_clusd1_ptr depth = 5120

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusd_gmem port = histograms_clusd2_ptr depth = 5120

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusd_gmem port = histograms_clusd3_ptr depth = 5120

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusd_gmem port = histograms_clusd4_ptr depth = 5120

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histosize_clusd_gmem port = histo_size_clusd0_ptr depth = 128

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histosize_clusd_gmem port = histo_size_clusd1_ptr depth = 128

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histosize_clusd_gmem port = histo_size_clusd2_ptr depth = 128

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histosize_clusd_gmem port = histo_size_clusd3_ptr depth = 128

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histosize_clusd_gmem port = histo_size_clusd4_ptr depth = 128

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusdin_gmem port = histograms_clusdin0_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusdin_gmem port = histograms_clusdin1_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusdin_gmem port = histograms_clusdin2_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusdin_gmem port = histograms_clusdin3_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusdin_gmem port = histograms_clusdin4_ptr depth = 4096

#pragma HLS INTERFACE m_axi offset = slave latency = 32 num_write_outstanding = 8 num_read_outstanding = \
    1 max_write_burst_length = 64 max_read_burst_length = 2 bundle = histo_clusdin_gmem port = config depth = 35
    // clang-format on

    // No dataflow, run sequentially

    uint32_t do_once[5];
    do_once[0] = config[25];
    do_once[1] = config[26];
    do_once[2] = config[27];
    do_once[3] = config[28];
    do_once[4] = config[29];

    uint32_t numHisto0_ptr = config[0];
    uint32_t numNonempty0_ptr = config[5];
    uint32_t lidx0_ptr = config[10];
    uint32_t numHisto_clusd0_ptr;
    uint32_t histo_size_clusdin0_ptr;

    uint32_t numHisto1_ptr = config[1];
    uint32_t numNonempty1_ptr = config[6];
    uint32_t lidx1_ptr = config[11];
    uint32_t numHisto_clusd1_ptr;
    uint32_t histo_size_clusdin1_ptr;

    uint32_t numHisto2_ptr = config[2];
    uint32_t numNonempty2_ptr = config[7];
    uint32_t lidx2_ptr = config[12];

    uint32_t numHisto_clusd2_ptr;
    uint32_t histo_size_clusdin2_ptr;

    uint32_t numHisto3_ptr = config[3];
    uint32_t numNonempty3_ptr = config[8];
    uint32_t lidx3_ptr = config[13];

    uint32_t numHisto_clusd3_ptr;
    uint32_t histo_size_clusdin3_ptr;

    uint32_t numHisto4_ptr = config[4];
    uint32_t numNonempty4_ptr = config[9];
    uint32_t lidx4_ptr = config[14];

    uint32_t numHisto_clusd4_ptr;
    uint32_t histo_size_clusdin4_ptr;

    if (do_once[0] != 0) {
        // clang-format off
    hls_ANSclusterHistogram_core(
        numNonempty0_ptr,
        nonempty_histo0_ptr,

        lidx0_ptr,
        numHisto0_ptr,
        histo_totalcnt0_ptr,
        histo_size0_ptr,
        histograms0_ptr,

        ctx_map0_ptr,
        histo_size_clusd0_ptr,
        histograms_clusd0_ptr,

        histograms_clusdin0_ptr,
        numHisto_clusd0_ptr,
        histo_size_clusdin0_ptr);
        // clang-format on
    }

    if (do_once[1] != 0) {
        // clang-format off
    hls_ANSclusterHistogram_core(
        numNonempty1_ptr,
        nonempty_histo1_ptr,

        lidx1_ptr,
        numHisto1_ptr,
        histo_totalcnt1_ptr,
        histo_size1_ptr,
        histograms1_ptr,

        ctx_map1_ptr,
        histo_size_clusd1_ptr,
        histograms_clusd1_ptr,

        histograms_clusdin1_ptr,
        numHisto_clusd1_ptr,
        histo_size_clusdin1_ptr);
        // clang-format on
    }

    if (do_once[2] != 0) {
        // clang-format off
    hls_ANSclusterHistogram_core(
        numNonempty2_ptr,
        nonempty_histo2_ptr,

        lidx2_ptr,
        numHisto2_ptr,
        histo_totalcnt2_ptr,
        histo_size2_ptr,
        histograms2_ptr,

        ctx_map2_ptr,
        histo_size_clusd2_ptr,
        histograms_clusd2_ptr,

        histograms_clusdin2_ptr,
        numHisto_clusd2_ptr,
        histo_size_clusdin2_ptr);
        // clang-format on
    }

    if (do_once[3] != 0) {
        // clang-format off
    hls_ANSclusterHistogram_core(
        numNonempty3_ptr,
        nonempty_histo3_ptr,

        lidx3_ptr,
        numHisto3_ptr,
        histo_totalcnt3_ptr,
        histo_size3_ptr,
        histograms3_ptr,

        ctx_map3_ptr,
        histo_size_clusd3_ptr,
        histograms_clusd3_ptr,

        histograms_clusdin3_ptr,
        numHisto_clusd3_ptr,
        histo_size_clusdin3_ptr);
        // clang-format on
    }

    if (do_once[4] != 0) {
// clang-format off
    #pragma HLS ALLOCATION function instances = hls_ANSclusterHistogram_core limit = 1
    hls_ANSclusterHistogram_core(
        numNonempty4_ptr,
        nonempty_histo4_ptr,

        lidx4_ptr,
        numHisto4_ptr,
        histo_totalcnt4_ptr,
        histo_size4_ptr,
        histograms4_ptr,

        ctx_map4_ptr,
        histo_size_clusd4_ptr,
        histograms_clusd4_ptr,

        histograms_clusdin4_ptr,
        numHisto_clusd4_ptr,
        histo_size_clusdin4_ptr);
        // clang-format on
    }

    config[19] = numHisto_clusd4_ptr;
    config[24] = histo_size_clusdin4_ptr;
    config[18] = numHisto_clusd3_ptr;
    config[23] = histo_size_clusdin3_ptr;
    config[17] = numHisto_clusd2_ptr;
    config[22] = histo_size_clusdin2_ptr;
    config[16] = numHisto_clusd1_ptr;
    config[21] = histo_size_clusdin1_ptr;
    config[15] = numHisto_clusd0_ptr;
    config[20] = histo_size_clusdin0_ptr;
    // printf("[KERNEL] cluster size = (%d, %d, %d, %d, %d)\n", numHisto_clusd0_ptr, numHisto_clusd1_ptr,
    //        numHisto_clusd2_ptr, numHisto_clusd3_ptr, numHisto_clusd4_ptr);
    // printf("[KERNEL] cluster in size = (%d, %d, %d, %d, %d)\n", histo_size_clusdin0_ptr, histo_size_clusdin1_ptr,
    //        histo_size_clusdin2_ptr, histo_size_clusdin3_ptr, histo_size_clusdin4_ptr);
    // for(int i=0; i<numHisto_clusd2_ptr; i++) {
    //  for(int j=0; j<histo_size_clusd2_ptr[i]; j++) {
    //      printf("[KERNEL] cluster 2 %d %d %d\n", i, j, histograms_clusd2_ptr[i*40+j]);
    //  }
    //}
    // for(int j=0; j<numHisto_clusd2_ptr; j++) {
    //    printf("[KERNEL] cluster in 2 %d %d\n", j, histograms_clusdin2_ptr[j]);
    //}
}

} // namespace codec
} // xf
#endif
