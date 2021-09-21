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
//#define USER_BUF_ON
#include "ap_int.h"
#include "bfs_acc.hpp"
#include "utils.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>

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

int bfs_execute_fpag(const int numVertices,
                     const int srcNodeID,
                     const int numEdges,
                     ap_uint<32>* queue32,
                     ap_uint<32>* column32,
                     ap_uint<32>* offset32,
                     ap_uint<32>* result32_dt,
                     ap_uint<32>* result32_ft,
                     ap_uint<32>* result32_pt,
                     ap_uint<32>* result32_lv) {
    struct timeval kernel_start_time, start_time, end_time;
    gettimeofday(&start_time, 0);

    int si = 0, num_rep = 1;
    auto columnBP = bfs_acc::create_bufpool(vpp::input);
    auto offsetBP = bfs_acc::create_bufpool(vpp::input);

    auto queueBP = bfs_acc::create_bufpool(vpp::output);
    auto resultDTBP = bfs_acc::create_bufpool(vpp::output);
    auto resultFTBP = bfs_acc::create_bufpool(vpp::output);
    auto resultPTBP = bfs_acc::create_bufpool(vpp::output);
    auto resultLVBP = bfs_acc::create_bufpool(vpp::output);

    bfs_acc::send_while([&]() -> bool {
        bfs_acc::set_handle(si);
#ifdef USER_BUF_ON
        bfs_acc::user_buf(queueBP, queue32, sizeof(ap_uint<32>) * numVertices);
        bfs_acc::user_buf(columnBP, column32, sizeof(ap_uint<32>) * numEdges);
        bfs_acc::user_buf(offsetBP, offset32, sizeof(ap_uint<32>) * (numVertices + 1));
        bfs_acc::user_buf(resultDTBP, result32_dt, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        bfs_acc::user_buf(resultFTBP, result32_ft, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        bfs_acc::user_buf(resultPTBP, result32_pt, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        bfs_acc::user_buf(resultLVBP, result32_lv, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);

        gettimeofday(&kernel_start_time, 0);
        bfs_acc::compute(srcNodeID, numEdges, numVertices, (ap_uint<512>*)column32, (ap_uint<512>*)offset32,
                         (ap_uint<512>*)queue32, queue32, (ap_uint<512>*)result32_dt, result32_dt, result32_ft,
                         result32_pt, result32_lv);
#else
        ap_uint<32>* column = (ap_uint<32>*)bfs_acc::alloc_buf(columnBP, sizeof(ap_uint<32>) * numEdges);
        ap_uint<32>* offset = (ap_uint<32>*)bfs_acc::alloc_buf(offsetBP, sizeof(ap_uint<32>) * (numVertices + 1));
        ap_uint<32>* queue = (ap_uint<32>*)bfs_acc::alloc_buf(queueBP, sizeof(ap_uint<32>) * numVertices);
        ap_uint<32>* result32_dt =
            (ap_uint<32>*)bfs_acc::alloc_buf(resultDTBP, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        ap_uint<32>* result32_ft =
            (ap_uint<32>*)bfs_acc::alloc_buf(resultFTBP, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        ap_uint<32>* result32_pt =
            (ap_uint<32>*)bfs_acc::alloc_buf(resultPTBP, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        ap_uint<32>* result32_lv =
            (ap_uint<32>*)bfs_acc::alloc_buf(resultLVBP, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);

        memcpy(column, column32, sizeof(ap_uint<32>) * numEdges);
        memcpy(offset, offset32, sizeof(ap_uint<32>) * (numVertices + 1));

        bfs_acc::set_handle(si);
        bfs_acc::compute(srcNodeID, numEdges, numVertices, (ap_uint<512>*)column, (ap_uint<512>*)offset,
                         (ap_uint<512>*)queue, queue, (ap_uint<512>*)result32_dt, result32_dt, result32_ft, result32_pt,
                         result32_lv);

#endif
        return (++si < num_rep);
    });

    bfs_acc::receive_all_in_order([&]() {
#ifndef USER_BUF_ON
        ap_uint<32>* result32_DT = (ap_uint<32>*)bfs_acc::get_buf(resultDTBP);
        ap_uint<32>* result32_FT = (ap_uint<32>*)bfs_acc::get_buf(resultFTBP);
        ap_uint<32>* result32_PT = (ap_uint<32>*)bfs_acc::get_buf(resultPTBP);
        ap_uint<32>* result32_LV = (ap_uint<32>*)bfs_acc::get_buf(resultLVBP);

        int ri = bfs_acc::get_handle();
        memcpy(result32_dt, result32_DT, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        memcpy(result32_ft, result32_FT, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        memcpy(result32_pt, result32_PT, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);
        memcpy(result32_lv, result32_LV, sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16);

#endif
    });
    bfs_acc::join();

    gettimeofday(&end_time, 0);
    std::cout << "Kernel time " << tvdiff(&kernel_start_time, &end_time) / 1000.0 << "ms" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;
    return 0;
}

int main(int argc, const char* argv[]) {
    std::cout << "\n---------------------BFS Traversal Test----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    int srcNodeID = 0;

#ifndef HLS_TEST
    std::string offsetfile;
    std::string columnfile;
    std::string goldenfile;
    std::string nodeIDStr;
    if (!parser.getCmdOption("-o", offsetfile)) { // offset
        std::cout << "ERROR: offsetfile is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-c", columnfile)) { // column
        std::cout << "ERROR: columnfile is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-g", goldenfile)) { // row
        std::cout << "ERROR: goldenfile is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-i", nodeIDStr)) { // source node
        std::cout << "ERROR: source node is not set!\n";
        return -1;
    } else {
        srcNodeID = std::stoi(nodeIDStr);
        std::cout << "Source node ID:" << srcNodeID << std::endl;
    }
#else
    std::string offsetfile = "data/test_offset.csr";
    std::string columnfile = "data/test_column.csr";
    std::string goldenfile = "data/test_golden.mtx";
    srcNodeID = 0;
#endif
    /*
        std::string offsetfile = "data/test_offset.csr";
        std::string columnfile = "data/test_column.csr";
        std::string goldenfile = "data/test_golden.mtx";
        srcNodeID = 0;
    */

    char line[1024] = {0};
    int index = 0;

    int numVertices;
    int maxVertexId;
    int numEdges;

    std::fstream offsetfstream(offsetfile.c_str(), std::ios::in);
    if (!offsetfstream) {
        std::cout << "Error : " << offsetfile << " file doesn't exist !" << std::endl;
        exit(1);
    }

    offsetfstream.getline(line, sizeof(line));
    std::stringstream numOdata(line);
    numOdata >> numVertices;
    numOdata >> maxVertexId;

    ap_uint<32>* offset32 = aligned_alloc<ap_uint<32> >(numVertices + 1);
    while (offsetfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> offset32[index];
        index++;
    }

    std::fstream columnfstream(columnfile.c_str(), std::ios::in);
    if (!columnfstream) {
        std::cout << "Error : " << columnfile << " file doesn't exist !" << std::endl;
        exit(1);
    }

    index = 0;

    columnfstream.getline(line, sizeof(line));
    std::stringstream numCdata(line);
    numCdata >> numEdges;

    ap_uint<32>* queue32 = aligned_alloc<ap_uint<32> >(numVertices);
    ap_uint<32>* column32 = aligned_alloc<ap_uint<32> >(numEdges);
    while (columnfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> column32[index];
        index++;
    }

    ap_uint<32>* result32_dt = aligned_alloc<ap_uint<32> >(((numVertices + 15) / 16) * 16);
    ap_uint<32>* result32_ft = aligned_alloc<ap_uint<32> >(((numVertices + 15) / 16) * 16);
    ap_uint<32>* result32_pt = aligned_alloc<ap_uint<32> >(((numVertices + 15) / 16) * 16);
    ap_uint<32>* result32_lv = aligned_alloc<ap_uint<32> >(((numVertices + 15) / 16) * 16);

    bfs_execute_fpag(numVertices, srcNodeID, numEdges, queue32, column32, offset32, result32_dt, result32_ft,
                     result32_pt, result32_lv);

    std::cout << "============================================================" << std::endl;
    int err = 0;

    std::vector<std::vector<int> > gold_result(numVertices, std::vector<int>(4, -1));
    gold_result.resize(numVertices);

    std::fstream goldenfstream(goldenfile.c_str(), std::ios::in);
    if (!goldenfstream) {
        std::cout << "Error : " << goldenfile << " file doesn't exist !" << std::endl;
        exit(1);
    }
    index = 0;
    while (goldenfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        for (int i = 0; i < 4; i++) {
            std::string tmp;
            data >> tmp;

            int tmpi = std::stoi(tmp);
            gold_result[index][i] = tmpi;
        }

        index++;
    }

    if (index != numVertices) {
        std::cout << "Error : Mismatch has been found in the golden file !" << std::endl;
        return -1;
    }

    for (int i = 0; i < numVertices; i++) {
        if ((result32_dt[i] != (ap_uint<32>)-1 && result32_dt[i].to_int() != gold_result[i][0]) ||
            result32_ft[i].to_int() != gold_result[i][1] || result32_pt[i].to_int() != gold_result[i][2] ||
            result32_lv[i].to_int() != gold_result[i][3]) {
            std::cout << "Mismatch-" << i << ":\tsw: " << gold_result[i][0] << " " << gold_result[i][1] << " "
                      << gold_result[i][2] << " " << gold_result[i][3] << " <-> "
                      << "hw: " << result32_dt[i] << " " << result32_ft[i] << " " << result32_pt[i] << " "
                      << result32_lv[i];

            std::cout << "\t\t\t***\t";
            if (result32_dt[i] != (ap_uint<32>)-1 && result32_dt[i].to_int() != gold_result[i][0]) std::cout << "D";
            if (result32_ft[i].to_int() != gold_result[i][1]) std::cout << "F";
            if (result32_pt[i].to_int() != gold_result[i][2]) std::cout << "P";
            if (result32_lv[i].to_int() != gold_result[i][3]) std::cout << "L";

            std::cout << std::endl;
            err++;
            break; //
        }
    }

    if (err == 0)
        std::cout << "Check Passed.\n\n";
    else
        std::cout << "Check failed.\n\n";
    return err;
}
