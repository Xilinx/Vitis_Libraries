/*
 * Copyright 2019 Xilinx, Inc.
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
#include "config.hpp"
#include <ap_int.h>
#include <hls_stream.h>
#define N 100   // 999 // 100*100*100*100
#define K (KC)  // 128 // 20 //512 //100 //512 //40 //512 //40 //128 //1024
#define D (DIM) // 512 // 16 //128 //1024 //24 // 512 //80 //128   //1024
#include "xf_DataAnalytics/clustering/kmeansTrain.hpp"
#if !defined(__SYNTHESIS__) && XF_DATA_ANALYTICS_DEBUG == 1
#include <iostream>
#endif
typedef double DType;
// typedef float DType;

template <typename DT, int sz>
union conv {};
template <>
union conv<double, 64> {
    double dt;
    unsigned long long ut;
};

void kMeansCore(ap_uint<512> data[1 + (N + K) * ((D * sizeof(DType) * 8 + 511) / 512)],
                ap_uint<512> centers[(PCU + K) * ((D * sizeof(DType) * 8 + 511) / 512)]) {
// clang-format off
#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 64 num_read_outstanding = 64 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_0 port = data 

#pragma HLS INTERFACE m_axi offset = slave latency = 64 \
	num_write_outstanding = 64 num_read_outstanding = 64 \
	max_write_burst_length = 64 max_read_burst_length = 64 \
	bundle = gmem0_1 port = centers

// clang-format on

#pragma HLS INTERFACE s_axilite port = data bundle = control
#pragma HLS INTERFACE s_axilite port = centers bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

    xf::data_analytics::clustering::kMeansTrain<DType, D, K, PCU, PDV>(data, centers);
}
#if !defined(__SYNTHESIS__) && XF_DATA_ANALYTICS_DEBUG == 1
void convert2array(int dim, int num, ap_uint<512>* data, DType** v) {
    const int sz = sizeof(DType) * 8;
    int p = 0;
    const int sn = 512 / sz;
    const int batch = (dim + sn - 1) / sn;
    int count = num * batch;
    int k = 0;
    int d = 0;
    while (p < count) {
        ap_uint<512> temp = data[p++];
        for (int i = 0; i < 512 / sz; ++i) {
            ap_uint<sz> ut = temp.range((i + 1) * sz - 1, i * sz);
            conv<DType, sz> nt;
            nt.ut = ut;
            v[k][d] = nt.dt;
            d++;
            if (d == dim) {
                k++;
                d = 0;
                break;
            }
        }
    } // while
}

void combineConfig(ap_uint<512>& config, int kcluster, int dim, int nsample, int maxIter, DType eps) {
    config.range(31, 0) = kcluster;
    config.range(63, 32) = dim;
    config.range(95, 64) = nsample;
    config.range(127, 96) = maxIter;
    const int sz = sizeof(DType) * 8;
    conv<DType, sz> nt;
    nt.dt = eps;
    config.range(sz + 127, 128) = nt.ut;
}
void convertVect2axi(DType** v, int dim, int num, int start, ap_uint<512>* data) {
    const int sz = sizeof(DType) * 8;
    ap_uint<1024> baseBlock = 0;
    int offset = 0;
    int q = start;
    int p = 0;
    ap_uint<512> temp = 0;
    for (int k = 0; k < num; ++k) {
        for (int d = 0; d < dim; ++d) {
            DType t = v[k][d];
            conv<DType, sz> nt;
            nt.dt = t;
            temp.range((p + 1) * sz - 1, p * sz) = nt.ut;
            p++;
            if (offset + p * sz >= 512) {
                baseBlock.range(offset + p * sz - 1, offset) = temp.range(p * sz - 1, 0);
                data[q++] = baseBlock.range(511, 0);
                offset += p * sz;
                offset -= 512;
                baseBlock >>= 512;
                p = 0;
                temp = 0;
            }
        }
    } // for
    if (offset > 0 || p > 0) {
        if (p > 0) {
            baseBlock.range(offset + p * sz - 1, offset) = temp.range(p * sz - 1, 0);

            offset += p * sz;
        }
        data[q++] = baseBlock.range(offset, 0);
    }
}
void clear(DType** x, int dim, int kc) {
    for (int ic = 0; ic < kc; ++ic) {
        for (int d = 0; d < dim; ++d) {
            x[ic][d] = 0;
        }
    }
}
void copy(DType** x, DType** y, int dim, int kc) {
    for (int ic = 0; ic < kc; ++ic) {
        for (int d = 0; d < dim; ++d) {
            y[ic][d] = x[ic][d];
        }
    }
}
bool calE(DType** c, DType** nc, int dim, int kc, DType eps) {
    bool res = true;
    for (int ic = 0; ic < kc; ++ic) {
        DType e = 0;
        for (int d = 0; d < dim; ++d) {
            DType df = nc[ic][d] - c[ic][d];
            e += df * df;
        }
        res &= e <= eps;
    }
    return res;
}
void train(DType** s, DType** c, DType** nc, int dim, int kc, int num) {
    int* cnt = (int*)malloc(sizeof(int) * kc);
    for (int ic = 0; ic < kc; ++ic) cnt[ic] = 0;

    for (int id = 0; id < num; ++id) {
        int bk = -1;
        DType md = 1e38;
        for (int ic = 0; ic < kc; ++ic) {
            DType ds = 0;
            for (int d = 0; d < dim; ++d) {
                DType df = (s[id][d] - c[ic][d]);
                ds += df * df;
            }
            if (md > ds) {
                md = ds;
                bk = ic;
            }
        }
        for (int d = 0; d < dim; ++d) {
            nc[bk][d] += s[id][d];
        }
        cnt[bk]++;
    }

    for (int ic = 0; ic < kc; ++ic) {
        int c = cnt[ic] > 0 ? cnt[ic] : 1;
        for (int d = 0; d < dim; ++d) {
            nc[ic][d] /= c;
        }
        std::cout << "cnt[" << ic << "]=" << cnt[ic] << "  " << nc[ic][0] << std::endl;
    }
    free(cnt);
}
void goldenTrain(DType** s, DType** c, DType** nc, int dim, int kc, int num, ap_uint<32>* tag, DType e, int maxIt) {
    int it = 0;
    bool stop;
    do {
        clear(nc, dim, kc);
        train(s, c, nc, dim, kc, num);
        stop = calE(nc, c, dim, kc, e);
        copy(nc, c, dim, kc);
        it++;
    } while (!stop && it < maxIt);
}

int test_all() {
    int res = 0;
    int dim = D - 5;      // D - 5;
    int kcluster = K - 3; // K - 3;
    int nsample = N;
    const int sz = sizeof(DType) * 8;
    int dsz = dim * sz;
    int numIn512 = 512 / dsz;
    const int ND = (N * D * sizeof(DType) * 8 + 511) / 512;
    const int NC = (K + PCU) * ((D * sizeof(DType) * 8 + 511) / 512);
    int numDataBlock = (nsample * dsz + 511) / 512;
    int numCenterBlock = (kcluster * dsz + 511) / 512;
    ap_uint<512> config = 0;
    int maxIter = 10; // 1000;
    DType eps = 1e-8;
    ap_uint<512>* data = (ap_uint<512>*)malloc(sizeof(ap_uint<512>) * (1 + ND + NC));
    ap_uint<512>* center = (ap_uint<512>*)malloc(sizeof(ap_uint<512>) * NC);
    ap_uint<32>* gtag = (ap_uint<32>*)malloc(sizeof(ap_uint<32>) * N);

    int cdsz = D * sizeof(DType);
    DType** x = (DType**)malloc(sizeof(DType*) * N);
    for (int i = 0; i < N; i++) {
        x[i] = (DType*)malloc(cdsz);
    }
    DType** c = (DType**)malloc(sizeof(DType*) * K);
    for (int i = 0; i < K; i++) {
        c[i] = (DType*)malloc(cdsz);
    }
    DType** nc = (DType**)malloc(sizeof(DType*) * K);
    for (int i = 0; i < K; i++) {
        nc[i] = (DType*)malloc(cdsz);
    }
    DType** gnc = (DType**)malloc(sizeof(DType*) * K);
    for (int i = 0; i < K; i++) {
        gnc[i] = (DType*)malloc(cdsz);
    }
    combineConfig(config, kcluster, dim, nsample, maxIter, eps);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < D; ++j) {
            x[i][j] = 0.5 + (i * 131 + j) % 1000;
        }
    }

    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < D; ++j) {
            DType t = 0.25 + (i * 131 + j) % 1000;
            c[i][j] = t;
        }
    }

    int kid = 0;
    std::cout << "numIn512=" << numIn512 << std::endl;
    std::cout << "numCenterBlock=" << numCenterBlock << std::endl;
    std::cout << "numDataBlock=" << numDataBlock << std::endl;
    std::cout << "K=" << K << "   N=" << N << std::endl;
    std::cout << "ND=" << ND << "   NC=" << NC << std::endl;

    for (int k = 0; k < K; ++k) {
        std::cout << "k=" << k << "   c=(";
        for (int j = 0; j < D; ++j) {
            DType cd = c[k][j];
            std::cout << cd;
            if (j < D - 1) std::cout << ",";
        }
        std::cout << ")" << std::endl;
    }
    data[0] = config;
    convertVect2axi(c, dim, kcluster, 1, data);
    int numCB = (kcluster * dsz + 511) / 512;
    convertVect2axi(x, dim, nsample, numCB + 1, data);
    kMeansCore(data, center);

    std::cout << "-------  cal golden ----" << std::endl;
    goldenTrain(x, c, gnc, dim, kcluster, nsample, gtag, eps, maxIter);

    convert2array(dim, kcluster, center, nc);
    std::cout << "K=" << K << "   N=" << N << std::endl;
    for (int k = 0; k < K; ++k) {
        std::cout << "k=" << k << "   c=(";
        for (int j = 0; j < D; ++j) {
            DType c1 = nc[k][j];
            DType c2 = gnc[k][j];
            if (c1 - c2 > 1e-8) res++;
            std::cout << std::dec << c1 << "(" << c2 << ")";
            if (j < D - 1) std::cout << ",";
        }
        std::cout << ")" << std::endl;
    }
    std::cout << "res=" << res << std::endl;
    for (int i = 0; i < N; i++) free(x[i]);

    for (int i = 0; i < K; i++) {
        free(c[i]);
        free(nc[i]);
        free(gnc[i]);
    }
    free(x);
    free(c);
    free(nc);
    free(gnc);
    free(center);
    free(gtag);

    return res;
}

int main(int argc, char** argv) {
    return test_all();
}
#endif
