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
#ifndef HLS_TEST
#include "xcl2.hpp"
#else
#include "shortestPath_top.hpp"
#endif
#include "ap_int.h"
#include "utils.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <limits>
#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

#define XCL_BANK0 XCL_BANK(0)
#define XCL_BANK1 XCL_BANK(1)
#define XCL_BANK2 XCL_BANK(2)
#define XCL_BANK3 XCL_BANK(3)
#define XCL_BANK4 XCL_BANK(4)
#define XCL_BANK5 XCL_BANK(5)
#define XCL_BANK6 XCL_BANK(6)
#define XCL_BANK7 XCL_BANK(7)
#define XCL_BANK8 XCL_BANK(8)
#define XCL_BANK9 XCL_BANK(9)
#define XCL_BANK10 XCL_BANK(10)
#define XCL_BANK11 XCL_BANK(11)
#define XCL_BANK12 XCL_BANK(12)
#define XCL_BANK13 XCL_BANK(13)
#define XCL_BANK14 XCL_BANK(14)
#define XCL_BANK15 XCL_BANK(15)

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

int main(int argc, const char* argv[]) {
    std::cout << "\n---------------------Shortest Path----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;
#ifndef HLS_TEST
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }
#endif
    std::string offsetfile;
    std::string columnfile;
    std::string weightfile;
    std::string goldenfile;
    std::string rep;
    int repInt = 1;
    if (!parser.getCmdOption("-o", offsetfile)) { // offset
        std::cout << "ERROR: offset file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-c", columnfile)) { // column
        std::cout << "ERROR: row file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-w", weightfile)) { // weight
        std::cout << "WARN: weight file path is not set! Use 1 as the weight\n";
        weightfile = "./tmp.csr";
    }

    if (!parser.getCmdOption("-g", goldenfile)) { // golden
        std::cout << "ERROR: row file path is not set!\n";
        return -1;
    }

    if (!parser.getCmdOption("-n", rep)) {
        std::cout << "WARN: number of run is not given, use 1." << std::endl;
    } else {
        repInt = std::stoi(rep);
    }

    // -------------setup k0 params---------------
    int err = 0;

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
    while (columnfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> column32[index];
        index++;
    }
    ap_uint<512>* column512 = reinterpret_cast<ap_uint<512>*>(column32);

    std::fstream weightfstream(weightfile.c_str(), std::ios::in);
    if (!weightfstream) {
        std::cout << "WARRN : use 1 for weight" << std::endl;
    } else {
        ap_uint<32> numWeight;
        weightfstream.getline(line, sizeof(line));
        std::stringstream numWdata(line);
        numWdata >> numWeight;
    }

    float* weight32 = aligned_alloc<float>(numEdges);
    for (int i = 0; i < numEdges; i++) {
        if (!weightfstream) {
            weight32[i] = 1;
        } else {
            weightfstream.getline(line, sizeof(line));
            std::stringstream data(line);

            data >> weight32[i];
        }
    }
    ap_uint<512>* weight512 = reinterpret_cast<ap_uint<512>*>(weight32);

    ap_uint<8>* info = aligned_alloc<ap_uint<8> >(4);
    memset(info, 0, 4 * sizeof(ap_uint<8>));
    std::vector<float*> result(repInt);
    for (int i = 0; i < repInt; i++) {
        result[i] = aligned_alloc<float>(((numVertices + 15) / 16) * 16);
    }
    std::vector<ap_uint<512>*> result512(repInt);
    for (int i = 0; i < repInt; i++) {
        result512[i] = reinterpret_cast<ap_uint<512>*>(result[i]);
    }

    ap_uint<32>* ddrQue = aligned_alloc<ap_uint<32> >(2 * 300 * 4096);
    ap_uint<512>* ddrQue512 = reinterpret_cast<ap_uint<512>*>(ddrQue);
    std::vector<ap_uint<32>*> config(repInt);
    for (int i = 0; i < repInt; i++) {
        config[i] = aligned_alloc<ap_uint<32> >(5);
        config[i][0] = sourceID + i * 10;
        config[i][1] = numVertices;
        union f_cast {
            float f;
            unsigned int i;
        };
        f_cast tmp;
        tmp.f = std::numeric_limits<float>::infinity();
        config[i][2] = tmp.i; //-1;
        config[i][3] = 2 * 300 * 4096;
        config[i][4] = 1;
    }
#ifndef HLS_TEST
    // do pre-process on CPU
    struct timeval start_time, end_time, test_time;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    std::vector<cl::Kernel> shortestPath(repInt);
    for (int i = 0; i < repInt; i++) {
        shortestPath[i] = cl::Kernel(program, "shortestPath_top");
    }
    std::cout << "kernel has been created" << std::endl;

    std::vector<cl_mem_ext_ptr_t> mext_o(2 * repInt + 5);
    mext_o[0] = {XCL_MEM_DDR_BANK0, offset512, 0};
    mext_o[1] = {XCL_MEM_DDR_BANK0, column512, 0};
    mext_o[2] = {XCL_MEM_DDR_BANK0, weight512, 0};
    mext_o[3] = {XCL_MEM_DDR_BANK0, info, 0};
    //    mext_o[4] = {XCL_MEM_DDR_BANK0, config, 0};
    mext_o[4] = {XCL_MEM_DDR_BANK0, ddrQue, 0};
    for (int i = 0; i < repInt; i++) {
        mext_o[5 + i] = {XCL_MEM_DDR_BANK0, result[i], 0};
    }
    for (int i = 0; i < repInt; i++) {
        mext_o[5 + repInt + i] = {XCL_MEM_DDR_BANK0, config[i], 0};
    }
    // create device buffer and map dev buf to host buf
    cl::Buffer offset_buf, column_buf, weight_buf, info_buf, ddrQue_buf;
    std::vector<cl::Buffer> result_buf(repInt);
    std::vector<cl::Buffer> config_buf(repInt);
    offset_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(ap_uint<32>) * (numVertices + 1), &mext_o[0]);
    column_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(ap_uint<32>) * numEdges, &mext_o[1]);
    weight_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(ap_uint<32>) * numEdges, &mext_o[2]);
    info_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                          sizeof(ap_uint<8>) * 4, &mext_o[3]);
    //    config_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
    //                            sizeof(ap_uint<32>) * 4, &mext_o[4]);
    ddrQue_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(ap_uint<32>) * 2 * 300 * 4096, &mext_o[4]);
    for (int i = 0; i < repInt; i++) {
        result_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   sizeof(ap_uint<32>) * ((numVertices + 15) / 16) * 16, &mext_o[5 + i]);
    }
    for (int i = 0; i < repInt; i++) {
        config_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   sizeof(ap_uint<32>) * 5, &mext_o[5 + repInt + i]);
    }

    std::vector<cl::Memory> init;
    for (int i = 0; i < repInt; i++) {
        init.push_back(config_buf[i]);
    }
    init.push_back(offset_buf);
    init.push_back(column_buf);
    init.push_back(weight_buf);
    init.push_back(ddrQue_buf);
    for (int i = 0; i < repInt; i++) {
        init.push_back(result_buf[i]);
    }
    init.push_back(info_buf);
    q.enqueueMigrateMemObjects(init, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
    q.finish();

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(repInt);
    std::vector<cl::Event> events_read(1);

    for (int i = 0; i < repInt; i++) {
        ob_in.push_back(config_buf[i]);
    }
    ob_in.push_back(offset_buf);
    ob_in.push_back(column_buf);
    ob_in.push_back(weight_buf);
    ob_in.push_back(info_buf);
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);

    for (int i = 0; i < repInt; i++) {
        ob_out.push_back(result_buf[i]);
    }
    ob_out.push_back(info_buf);
    //    q.finish();
    // launch kernel and calculate kernel execution time
    std::cout << "kernel start------" << std::endl;
    gettimeofday(&start_time, 0);
    for (int i = 0; i < repInt; i++) {
        int j = 0;
        shortestPath[i].setArg(j++, config_buf[i]);
        shortestPath[i].setArg(j++, offset_buf);
        shortestPath[i].setArg(j++, column_buf);
        shortestPath[i].setArg(j++, weight_buf);
        shortestPath[i].setArg(j++, ddrQue_buf);
        shortestPath[i].setArg(j++, ddrQue_buf);
        shortestPath[i].setArg(j++, result_buf[i]);
        shortestPath[i].setArg(j++, result_buf[i]);
        shortestPath[i].setArg(j++, info_buf);

        q.enqueueTask(shortestPath[i], &events_write, &events_kernel[i]);
    }

    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();

    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;

    unsigned long time1, time2, total_time;
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Write DDR Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    total_time = time2 - time1;
    for (int i = 0; i < repInt; i++) {
        events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
        events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
        std::cout << "Kernel[" << i << "] Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
        total_time += time2 - time1;
    }
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Read DDR Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    total_time += time2 - time1;
    std::cout << "Total Execution time " << total_time / 1000000.0 << "ms" << std::endl;
#else
    shortestPath_top(config[0], offset512, column512, weight512, ddrQue512, ddrQue, result512[0], result[0], info);
#endif
    std::cout << "============================================================" << std::endl;

    std::fstream goldenfstream(goldenfile.c_str(), std::ios::in);
    if (!goldenfstream) {
        std::cout << "Err : " << goldenfile << " file doesn't exist !" << std::endl;
        exit(1);
    }
    index = 0;
    while (goldenfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        std::string tmp;
        int tmpi;
        float tmpd = std::numeric_limits<float>::infinity(); // 4294967295;
        data >> tmpi;
        while (tmpi != index + 1) {
            index++;
        }
        data >> tmp;
        if (tmp.compare("Infinity")) {
            tmpd = std::stof(tmp);
            if (std::abs(result[0][index] - tmpd) > 0.000001) {
                std::cout << "Err: " << index << " " << tmpd << " " << result[0][index] << std::endl;
                err++;
            }
        } else {
            if (result[0][index] != std::numeric_limits<float>::infinity()) {
                std::cout << "Err: " << index << " Infinity " << result[0][index] << std::endl;
                err++;
            }
        }
    }

    if (index + 1 != numVertices) {
        std::cout << "Err " << std::endl;
        err++;
    }
    if (info[0] != 0) std::cout << "queue overflow" << std::endl;
    if (info[1] != 0) std::cout << "table overflow" << std::endl;

    return err;
}
