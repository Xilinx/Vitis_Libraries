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
#include "shortestPath_acc.hpp"
#include "ap_int.h"
#include "utils.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <limits>
#include <stdlib.h>

//#define USER_BUF_ON

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

bool check_result(const int numVertices, ap_uint<8>* info, float* result, std::string goldenfile) {
    int err = 0;
    char line[1024] = {0};
    int index = 0;

    if (info[0] != 0) {
        std::cout << "queue overflow" << std::endl;
        // exit(1);
    }
    if (info[1] != 0) {
        std::cout << "table overflow" << std::endl;
        // exit(1);
    }

    bool* connect;
    connect = aligned_alloc<bool>(((numVertices + 1023) / 1024) * 1024);
    for (int i = 0; i < numVertices; i++) {
        connect[i] = false;
    }

    std::fstream goldenfstream(goldenfile.c_str(), std::ios::in);
    if (!goldenfstream) {
        std::cout << "Err : " << goldenfile << " file doesn't exist !" << std::endl;
        exit(1);
    }
    goldenfstream.getline(line, sizeof(line));

    index = 0;
    while (goldenfstream.getline(line, sizeof(line))) {
        std::string str(line);
        std::replace(str.begin(), str.end(), ',', ' ');
        std::stringstream data(str.c_str());
        int vertex;
        float distance;
        int pred_golden;
        data >> vertex;
        data >> distance;
        data >> pred_golden;
        if (std::abs(result[vertex - 1] - distance) / distance > 0.00001) {
            std::cout << "Err distance: " << vertex - 1 << " " << distance << " " << result[vertex - 1] << std::endl;
            err++;
        }
        connect[vertex - 1] = true;
    }

    for (int i = 0; i < numVertices; i++) {
        if (connect[i] == false && result[i] != std::numeric_limits<float>::infinity()) {
            std::cout << "Err distance: " << i << " " << std::numeric_limits<float>::infinity() << " " << result[i]
                      << std::endl;
            err++;
            break;
        }
    }

    return err;
}

bool shortestPath_execute_fpag(const int num_rep,
                               const int numVertices,
                               const int numEdges,
                               ap_uint<32>* config32,
                               ap_uint<512>* offset512,
                               ap_uint<512>* column512,
                               ap_uint<512>* weight512,
                               ap_uint<32>* ddrQue32,
                               float* result32,
                               ap_uint<8>* info8,
                               std::string goldenfile) {
    struct timeval start_time, kernel_start_time, end_time;
    gettimeofday(&start_time, 0);

    int si = 0;
    auto configBP = shortestPath_acc::create_bufpool(vpp::input);
    auto offsetBP = shortestPath_acc::create_bufpool(vpp::input);
    auto columnBP = shortestPath_acc::create_bufpool(vpp::input);
    auto weightBP = shortestPath_acc::create_bufpool(vpp::input);
    auto ddrQueBP = shortestPath_acc::create_bufpool(vpp::output);
    auto resultBP = shortestPath_acc::create_bufpool(vpp::output);
    auto infoBP = shortestPath_acc::create_bufpool(vpp::output);

    shortestPath_acc::send_while([&]() -> bool {
#ifdef USER_BUF_ON
        shortestPath_acc::set_handle(si);

        shortestPath_acc::user_buf(configBP, config32, sizeof(ap_uint<32>) * 6);
        shortestPath_acc::user_buf(offsetBP, offset512, sizeof(ap_uint<512>) * ((numVertices + 1 + 15) / 16));
        shortestPath_acc::user_buf(columnBP, column512, sizeof(ap_uint<512>) * ((numEdges + 15) / 16));
        shortestPath_acc::user_buf(weightBP, weight512, sizeof(ap_uint<512>) * ((numEdges + 15) / 16));
        shortestPath_acc::user_buf(ddrQueBP, ddrQue32, sizeof(ap_uint<32>) * (10 * 300 * 4096));
        shortestPath_acc::user_buf(resultBP, result32, sizeof(float) * ((numVertices + 1023) / 1024) * 1024);
        shortestPath_acc::user_buf(infoBP, info8, sizeof(ap_uint<8>) * 4);

        gettimeofday(&kernel_start_time, 0);
        shortestPath_acc::compute(numVertices, numEdges, config32, offset512, column512, weight512,
                                  (ap_uint<512>*)ddrQue32, ddrQue32, (ap_uint<512>*)result32, (ap_uint<32>*)result32,
                                  info8);
#else
        ap_uint<32>* acc_config32 = (ap_uint<32>*)shortestPath_acc::alloc_buf(configBP, sizeof(ap_uint<32>) * 6);
        ap_uint<512>* acc_offset512 =
            (ap_uint<512>*)shortestPath_acc::alloc_buf(offsetBP, sizeof(ap_uint<512>) * ((numVertices + 1 + 15) / 16));
        ap_uint<512>* acc_column512 =
            (ap_uint<512>*)shortestPath_acc::alloc_buf(columnBP, sizeof(ap_uint<512>) * ((numEdges + 15) / 16));
        ap_uint<512>* acc_weight512 =
            (ap_uint<512>*)shortestPath_acc::alloc_buf(weightBP, sizeof(ap_uint<512>) * ((numEdges + 15) / 16));
        ap_uint<32>* acc_ddrQue32 =
            (ap_uint<32>*)shortestPath_acc::alloc_buf(ddrQueBP, sizeof(ap_uint<32>) * (10 * 300 * 4096));
        float* acc_result32 =
            (float*)shortestPath_acc::alloc_buf(resultBP, sizeof(float) * ((numVertices + 1023) / 1024) * 1024);
        ap_uint<8>* acc_info8 = (ap_uint<8>*)shortestPath_acc::alloc_buf(infoBP, sizeof(ap_uint<8>) * 4);

        memcpy(acc_config32, config32, sizeof(ap_uint<32>) * 6);
        memcpy(acc_offset512, offset512, sizeof(ap_uint<512>) * ((numVertices + 1 + 15) / 16));
        memcpy(acc_column512, column512, sizeof(ap_uint<512>) * ((numEdges + 15) / 16));
        memcpy(acc_weight512, weight512, sizeof(ap_uint<512>) * ((numEdges + 15) / 16));
        memcpy(acc_info8, info8, sizeof(ap_uint<8>) * 4);

        gettimeofday(&kernel_start_time, 0);
        shortestPath_acc::compute(numVertices, numEdges, acc_config32, acc_offset512, acc_column512, acc_weight512,
                                  (ap_uint<512>*)acc_ddrQue32, acc_ddrQue32, (ap_uint<512>*)acc_result32,
                                  (ap_uint<32>*)acc_result32, acc_info8);
#endif
        return (++si < num_rep);
    });

    shortestPath_acc::receive_all_in_order([&]() {
#ifndef USER_BUF_ON
        float* acc_result32 = (float*)shortestPath_acc::get_buf(resultBP);
        ap_uint<8>* acc_info8 = (ap_uint<8>*)shortestPath_acc::get_buf(infoBP);

        memcpy(result32, acc_result32, sizeof(float) * ((numVertices + 1023) / 1024) * 1024);
        memcpy(info8, acc_info8, sizeof(ap_uint<8>) * 4);
#endif
    });
    shortestPath_acc::join();

    gettimeofday(&end_time, 0);

    bool verify = true;
    bool err = check_result(numVertices, info8, result32, goldenfile);
    if (err) {
        verify = false;
        std::cout << "ERROR: MISMATCH on result" << std::endl;
    }
    std::cout << (verify ? "PASSED" : "ERROR: FAILED") << ": kernel end------" << std::endl;
    std::cout << "Kernel time " << tvdiff(&kernel_start_time, &end_time) / 1000.0 << "ms" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;
    return err;
}

int main(int argc, const char* argv[]) {
    std::cout << "\n---------------------Shortest Path----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    int repInt = 1;

#ifndef USE_SMALL_DATASET
    std::string offsetfile;
    std::string columnfile;
    std::string goldenfile;
    if (!parser.getCmdOption("-o", offsetfile)) { // offset
        std::cout << "ERROR: offset file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-c", columnfile)) { // column
        std::cout << "ERROR: row file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-g", goldenfile)) { // golden
        std::cout << "ERROR: row file path is not set!\n";
        return -1;
    }
#else
    std::string offsetfile = "data/data-csr-offset.mtx";
    std::string columnfile = "data/data-csr-indicesweights.mtx";
    std::string goldenfile = "data/data-golden.sssp.mtx";
#endif
    // std::string offsetfile = "data/data-csr-offset.mtx";
    // std::string columnfile = "data/data-csr-indicesweights.mtx";
    // std::string goldenfile = "data/data-golden.sssp.mtx";

    // -------------setup k0 params---------------
    // int err = 0;

    char line[1024] = {0};
    int index = 0;

    int numVertices;
    int numEdges;
    unsigned int sourceID = 30;

    std::fstream offsetfstream(offsetfile.c_str(), std::ios::in);
    if (!offsetfstream) {
        std::cout << "Error : " << offsetfile << " file doesn't exist !" << std::endl;
        exit(1);
    }

    offsetfstream.getline(line, sizeof(line));
    std::stringstream numOdata(line);
    numOdata >> numVertices;
    numOdata >> numVertices;

    ap_uint<32>* offset32 = aligned_alloc<ap_uint<32> >(numVertices + 1);
    while (offsetfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> offset32[index];
        index++;
    }

    ap_uint<512>* offset512 = reinterpret_cast<ap_uint<512>*>(offset32);
    int max = 0;
    int id = 0;
    for (int i = 0; i < numVertices; i++) {
        if (offset32[i + 1] - offset32[i] > max) {
            max = offset32[i + 1] - offset32[i];
            id = i;
        }
    }
    std::cout << "id: " << id << " max out: " << max << std::endl;
    sourceID = id;

    std::fstream columnfstream(columnfile.c_str(), std::ios::in);
    if (!columnfstream) {
        std::cout << "Error : " << columnfile << " file doesn't exist !" << std::endl;
        exit(1);
    }

    index = 0;

    columnfstream.getline(line, sizeof(line));
    std::stringstream numCdata(line);
    numCdata >> numEdges;

    ap_uint<32>* column32 = aligned_alloc<ap_uint<32> >(numEdges);
    float* weight32 = aligned_alloc<float>(numEdges);
    while (columnfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> column32[index];
        data >> weight32[index];
        index++;
    }
    ap_uint<512>* column512 = reinterpret_cast<ap_uint<512>*>(column32);
    ap_uint<512>* weight512 = reinterpret_cast<ap_uint<512>*>(weight32);

    ap_uint<8>* info = aligned_alloc<ap_uint<8> >(4);
    memset(info, 0, 4 * sizeof(ap_uint<8>));
    float* result;
    result = aligned_alloc<float>(((numVertices + 1023) / 1024) * 1024);

    ap_uint<32>* ddrQue = aligned_alloc<ap_uint<32> >(10 * 300 * 4096);

    ap_uint<32>* config;
    config = aligned_alloc<ap_uint<32> >(6);
    config[0] = numVertices;
    union f_cast {
        float f;
        unsigned int i;
    };
    f_cast tmp;
    tmp.f = std::numeric_limits<float>::infinity();
    config[1] = tmp.i; //-1;
    config[2] = 0;
    config[3] = 10 * 300 * 4096;
    ap_uint<32> cmd;
    cmd.set_bit(0, 1); // enable weight?
    cmd.set_bit(1, 0); // enable predecessor?
    cmd.set_bit(2, 0); // float or fixed? 0 for float, 1 for fixed
    config[4] = cmd;
    config[5] = sourceID;

    return shortestPath_execute_fpag(repInt, numVertices, numEdges, config, offset512, column512, weight512, ddrQue,
                                     result, info, goldenfile);

    std::cout << "============================================================" << std::endl;
}
