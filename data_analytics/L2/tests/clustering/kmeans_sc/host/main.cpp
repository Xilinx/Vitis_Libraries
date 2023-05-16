/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include <sys/time.h>
#include <new>
#include <cstdlib>
#include "kmeans_acc.hpp"

#include <vector>

#define XF_DATA_ANALYTICS_DEBUG 0
#include "../kernel/config.hpp"
#include <ap_int.h>
#include <iostream>
#include "eval.hpp"
#include "iris.hpp"
#include <cstring>
#include <limits>
#include <stdlib.h>
#include <algorithm>

class ArgParser {
   public:
    ArgParser(int& argc, const char** argv) {
        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
    }
    bool getCmdOption(const std::string option, std::string& value) const {
        std::vector<std::string>::const_iterator itr;
        itr = std::find(this->mTokens.begin(), this->mTokens.end(), option);
        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
            value = *itr;
            return true;
        }
        return false;
    }

   private:
    std::vector<std::string> mTokens;
};

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

bool check_result(int dim,
                  int kcluster,
                  int nsample,
                  int maxIter,
                  DType eps,
                  DType** x,
                  DType** c,
                  DType** nc,
                  DType** gnc,
                  ap_uint<32>* gtag,
                  ap_uint<512>* centers) {
    int res = 0;

    std::cout << "-------  cal golden ----" << std::endl;
    goldenTrain(x, c, gnc, dim, kcluster, nsample, gtag, eps, maxIter);
    convert2array(dim, kcluster, centers, nc);
#if XF_DATA_ANALYTICS_DEBUG == 1
    std::cout << "KC=" << KC << "   NS=" << NS << std::endl;
#endif
    for (int k = 0; k < KC; ++k) {
#if XF_DATA_ANALYTICS_DEBUG == 1
        std::cout << "k=" << k << "   c=(";
#endif
        for (int j = 0; j < DIM; ++j) {
            DType c1 = nc[k][j];
            DType c2 = gnc[k][j];
            if (c1 != c2) {
                res++;
            }
#if XF_DATA_ANALYTICS_DEBUG == 1
            std::cout << std::dec << c1 << "(" << c2 << ")";
            if (j < DIM - 1) std::cout << ", ";
#endif
        }
#if XF_DATA_ANALYTICS_DEBUG == 1
        std::cout << ")" << std::endl;
#endif
    }
    if (res == 0)
        std::cout << "PASS" << std::endl;
    else
        std::cout << "FAIL" << std::endl;
    return res;
}

void kmeans_execute_fpga(int num_rep,
                         int bufferSize,
                         int NC,
                         int dim,
                         int kcluster,
                         int nsample,
                         int maxIter,
                         DType eps,
                         DType** x,
                         DType** c,
                         DType** nc,
                         DType** gnc,
                         ap_uint<32>* gtag,
                         ap_uint<512>* inputData,
                         ap_uint<512>* centers) {
    struct timeval start_time, end_time, kernel_start_time;
    gettimeofday(&start_time, 0);

    int si = 0;

    auto inputDataBP = kmeans_acc::create_bufpool(vpp::input);
    auto centersBP = kmeans_acc::create_bufpool(vpp::output);
    // sending input
    kmeans_acc::send_while([&]() -> bool {
        kmeans_acc::set_handle(si);
        kmeans_acc::user_buf(inputDataBP, inputData, bufferSize * sizeof(ap_uint<512>));
        kmeans_acc::user_buf(centersBP, centers, NC * sizeof(ap_uint<512>));

        gettimeofday(&kernel_start_time, 0);
        kmeans_acc::compute(bufferSize, NC, inputData, centers);

        return (++si < num_rep);
    });
    // receiveing output
    kmeans_acc::receive_all_out_of_order([&]() {});
    // wait for both loops to finish
    kmeans_acc::join();

    gettimeofday(&end_time, 0);
    std::cout << "Kernel has been run for " << std::dec << num_rep << " times." << std::endl;
    std::cout << "Kernel time " << tvdiff(&kernel_start_time, &end_time) / 1000.0 << "ms" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;
}

int main(int argc, const char* argv[]) {
    // cmd parser
    ArgParser parser(argc, (const char**)argv);

    // set repeat time
    int num_rep = 1;
    int spNum = NS; // 20000;
    int dim = DIM;
    int kcluster = KC;
    int nsample = spNum;

    // dim = DIM - 2;
    // kcluster = KC - 2;
    const int sz = sizeof(DType) * 8;
    int dsz = dim * sz;
    int numIn512 = 512 / dsz;
    int ND = (nsample * DIM * sizeof(DType) * 8 + 511) / 512;
    const int NC = 1 + (KC + PCU) * ((DIM * sizeof(DType) * 8 + 511) / 512);
    int bufferSize = ND + NC + 1;
    // int bufferSize = (1<<20);
    int numDataBlock = (nsample * dsz + 511) / 512;
    int numCenterBlock = (kcluster * dsz + 511) / 512;
    int maxIter = 30; // 1000;
    DType eps = 1e-8;

    ap_uint<512> config = 0;
    ap_uint<512>* inputData = aligned_alloc<ap_uint<512> >(bufferSize);
    ap_uint<512>* centers = aligned_alloc<ap_uint<512> >(NC);
    ap_uint<32>* gtag = (ap_uint<32>*)malloc(sizeof(ap_uint<32>) * nsample);

    int cdsz = DIM * sz;
    DType** x = (DType**)malloc(sizeof(DType*) * nsample);
    for (int i = 0; i < nsample; i++) {
        x[i] = (DType*)malloc(cdsz);
    }
    DType** c = (DType**)malloc(sizeof(DType*) * KC);
    for (int i = 0; i < KC; i++) {
        c[i] = (DType*)malloc(cdsz);
    }
    DType** nc = (DType**)malloc(sizeof(DType*) * KC);
    for (int i = 0; i < KC; i++) {
        nc[i] = (DType*)malloc(cdsz);
    }
    DType** gnc = (DType**)malloc(sizeof(DType*) * KC);
    for (int i = 0; i < KC; i++) {
        gnc[i] = (DType*)malloc(cdsz);
    }

    combineConfig(config, kcluster, dim, nsample, maxIter, eps);
    for (int i = 0; i < nsample; ++i) {
        for (int j = 0; j < dim; ++j) {
            // x[i][j] = 0.5 + (i * 131 + j) % 1000;
            int ii = i % 150;
            int jj = j % 4;
            x[i][j] = irisVec[ii][jj];
        }
    }

    for (int i = 0; i < KC; ++i) {
        for (int j = 0; j < dim; ++j) {
            // DType t = 0.5 + (i * 131 + j) % 1000;
            // c[i][j] = t;
            int ii = i % 150;
            int jj = j % 4;
            c[i][j] = irisVec[ii][jj];
        }
    }

    int kid = 0;
    std::cout << "numIn512=" << numIn512 << std::endl;
    std::cout << "numCenterBlock=" << numCenterBlock << std::endl;
    std::cout << "numDataBlock=" << numDataBlock << std::endl;
    std::cout << "KC=" << KC << "   NS=" << NS << std::endl;
    std::cout << "ND=" << ND << "   NC=" << NC << std::endl;
    std::cout << "nsample=" << nsample << "  kcluster=" << kcluster << "  dim=" << dim << "  bufferSize=" << bufferSize
              << std::endl;
#if !defined(__SYNTHESIS__) && XF_DATA_ANALYTICS_DEBUG == 1
    for (int k = 0; k < KC; ++k) {
        std::cout << "k=" << k << "   c=(";
        for (int j = 0; j < DIM; ++j) {
            DType cd = c[k][j];
            std::cout << cd;
            if (j < DIM - 1) std::cout << ",";
        }
        std::cout << ")" << std::endl;
    }
#endif
    inputData[0] = config;
    convertVect2axi(c, dim, kcluster, 1, inputData);
    int numCB = (kcluster * dsz + 511) / 512;
    convertVect2axi(x, dim, nsample, numCB + 1, inputData);

    std::cout << "Host map buffer has been allocated and set.\n";

    kmeans_execute_fpga(num_rep, bufferSize, NC, dim, kcluster, nsample, maxIter, eps, x, c, nc, gnc, gtag, inputData,
                        centers);
}
