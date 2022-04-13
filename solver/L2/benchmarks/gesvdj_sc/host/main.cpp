/*
 * Copyright 2019-2021 Xilinx, Inc.
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

#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <algorithm>
#include <math.h>
#include <chrono>

#include "matrixUtility.hpp"
#include "gesvdj.hpp"

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

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) {
        throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(ptr);
}

class LTimer {
    std::chrono::time_point<std::chrono::high_resolution_clock> t;

   public:
    LTimer() { t = std::chrono::high_resolution_clock::now(); }
    float ms() {
        return (std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - t)
                    .count()) /
               1000000.0;
    }
};
inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

void user_init_data(int dataSz, int t, double* A) {
    for (int cl = 0; cl < 1; cl++) {
        double* Ac = A + cl * dataSz * dataSz;
        symMatGen<double>(dataSz, t, Ac);
    }
}

bool user_check_result(int dataAM,
                       int dataAN,
                       int in_size,
                       int out_size_V,
                       double* dataU_svd,
                       double* dataV_svd,
                       double* sigma_svd,
                       double* dataA_svd) {
    // Calculate A_out = U*sigma*VT and compare with original A matrix
    double* dataVT_svd;
    double* dataA_out;
    dataA_out = new double[in_size];
    dataVT_svd = new double[out_size_V];
    transposeMat<double>(dataAN, dataV_svd, dataVT_svd);
    MulMat(dataAM, dataAM, dataAN, dataAN, dataU_svd, sigma_svd, dataVT_svd, dataA_out);

    // Calculate err between dataA_svd and dataA_out
    double errA = 0;
    for (int i = 0; i < dataAM; i++) {
        for (int j = 0; j < dataAN; j++) {
            errA += (dataA_svd[i * dataAN + j] - dataA_out[i * dataAN + j]) *
                    (dataA_svd[i * dataAN + j] - dataA_out[i * dataAN + j]);
        }
    }
    errA = std::sqrt(errA);

    // Delete created buffers
    delete[] dataVT_svd;
    delete[] dataA_out;

    std::cout << "-------------- " << std::endl;
    if (errA > 0.0001) {
        std::cout << "ERROR: Result false" << std::endl;
        std::cout << "-------------- " << std::endl;
        return false;
    } else {
        std::cout << "INFO: Result correct" << std::endl;
        std::cout << "-------------- " << std::endl;
        return true;
    }
}

static bool verify_result = true;

static bool run_hw(const int S, int seed, int dataAM = 16, int dataAN = 16) {
    // dbg_vpp_acc = 3;

    std::cout << "Running " << S << " hw iterations"
              << "\n";
    int s = 0;
    // LTimer ltm;
    int out_size_U = dataAM * dataAM * sizeof(double);
    int out_size_V = dataAN * dataAN * sizeof(double);
    int out_size_sigma = dataAM * dataAN * sizeof(double);
    int in_size = dataAM * dataAN * sizeof(double);
    double* dataA = aligned_alloc<double>(in_size);
    double* sigma = aligned_alloc<double>(out_size_sigma);
    double* dataU = aligned_alloc<double>(out_size_U);
    double* dataV = aligned_alloc<double>(out_size_V);

    // user_init_data(dataAN, 0, dataA, );
    user_init_data(dataAN, seed, dataA);
    struct timeval tstart, tend, kernel_time;

    // Initialization of host buffers

    struct timeval start_time, end_time, kernel_start_time;
    gettimeofday(&start_time, 0);

    auto dataA_bp = xgesvdj::create_bufpool(vpp::input);
    auto sigma_bp = xgesvdj::create_bufpool(vpp::output);
    auto dataU_bp = xgesvdj::create_bufpool(vpp::output);
    auto dataV_bp = xgesvdj::create_bufpool(vpp::output);

    xgesvdj::send_while([&]() -> bool {
        xgesvdj::set_handle(s);

        // dynamically allocate and prepare data
        double* dataA = (double*)xgesvdj::alloc_buf(dataA_bp, in_size);
        double* sigma = (double*)xgesvdj::alloc_buf(sigma_bp, out_size_sigma);
        double* dataU = (double*)xgesvdj::alloc_buf(dataU_bp, out_size_U);
        double* dataV = (double*)xgesvdj::alloc_buf(dataV_bp, out_size_V);

        // iterate over all clustered transactions
        xgesvdj::compute(dataA, sigma, dataU, dataV, dataAN);
        if (++s == S) return 0;
        return 1;
    });
    // receiving output
    bool verify = verify_result;
    bool ok = true;
    xgesvdj::receive_all_in_order([&]() {
        int r = xgesvdj::get_handle();
        double* sigma = (double*)xgesvdj::get_buf(sigma_bp);
        double* dataU = (double*)xgesvdj::get_buf(dataU_bp);
        double* dataV = (double*)xgesvdj::get_buf(dataV_bp);
        if (verify) {
            // let's check the result
            double* dataA = (double*)xgesvdj::get_buf(dataA_bp);
            ok = user_check_result(dataAM, dataAN, in_size, out_size_V, dataU, dataV, sigma, dataA);
        }
    });
    xgesvdj::join();

    return !ok;
}

//! Core function of SVD benchmark
int main(int argc, const char* argv[]) {
    // Initialize parser
    ArgParser parser(argc, argv);

    // Initialize paths addresses
    std::string num_str;
    int num_runs, csz, dataAM, dataAN, seed;

    // verify_result = false;

    if (!parser.getCmdOption("-runs", num_str)) {
        num_runs = 1;
        std::cout << "INFO:number runs is not set!\n";
    } else {
        num_runs = std::stoi(num_str);
    }
    if (!parser.getCmdOption("-", num_str)) {
        csz = 1;
        std::cout << "INFO:number clusterSize is not set!\n";
    } else {
        csz = std::stoi(num_str);
    }
    if (!parser.getCmdOption("-M", num_str)) {
        dataAM = 16;
        std::cout << "INFO:row size M is not set!\n";
    } else {
        dataAM = std::stoi(num_str);
    }
    if (!parser.getCmdOption("-N", num_str)) {
        dataAN = 16;
        std::cout << "INFO:column size N is not set!\n";
    } else {
        dataAN = std::stoi(num_str);
    }
    if (!parser.getCmdOption("-seed", num_str)) {
        seed = 12;
        std::cout << "INFO:seed is not set!\n";
    } else {
        seed = std::stoi(num_str);
    }

    // dataAM = dataAN is valid only for symmetric matrix
    dataAM = (dataAM > dataAN) ? dataAN : dataAM;
    dataAN = dataAM;

    return run_hw(num_runs, seed, dataAM, dataAN);
}
