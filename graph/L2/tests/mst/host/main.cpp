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

#ifndef HLS_TEST
#include "xcl2.hpp"
#else
#include "mst_top.hpp"
#endif

#include "xf_utils_sw/logger.hpp"
#include "ap_int.h"
#include "utils.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <set>
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
    std::cout << "\n---------------------MST----------------\n";
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
    std::string goldenfile;
    int repInt = 1;
    if (!parser.getCmdOption("-o", offsetfile)) { // offset
        std::cout << "ERROR: offset file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-c", columnfile)) { // column
        std::cout << "ERROR: row file path is not set!\n";
        return -1;
    }

    // -------------setup k0 params---------------

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

    unsigned* offset32 = aligned_alloc<unsigned>(numVertices + 1);
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

    unsigned* column32 = aligned_alloc<unsigned>(numEdges);
    float* weight32 = aligned_alloc<float>(numEdges);
    while (columnfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> column32[index];
        data >> weight32[index];
        index++;
    }

    unsigned* mst = aligned_alloc<unsigned>(numVertices);
    for (int i = 0; i < numVertices; i++) mst[i] = -1;
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int err;
#ifndef HLS_TEST
    // do pre-process on CPU
    struct timeval start_time, end_time;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device, NULL, NULL, NULL, &err);
    logger.logCreateContext(err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    logger.logCreateCommandQueue(err);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins, NULL, &err);
    logger.logCreateProgram(err);
    cl::Kernel mst_knl;
    mst_knl = cl::Kernel(program, "mst_top");

    std::cout << "kernel has been created" << std::endl;

    std::vector<cl_mem_ext_ptr_t> mext_o(4);
#ifdef USE_HBM
    mext_o[0] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, offset32, 0};
    mext_o[1] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, column32, 0};
    mext_o[2] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, weight32, 0};
    mext_o[3] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, mst, 0};
#else
    mext_o[0] = {XCL_MEM_DDR_BANK0, offset32, 0};
    mext_o[1] = {XCL_MEM_DDR_BANK0, column32, 0};
    mext_o[2] = {XCL_MEM_DDR_BANK0, weight32, 0};
    mext_o[3] = {XCL_MEM_DDR_BANK0, mst, 0};
#endif
    // create device buffer and map dev buf to host buf
    cl::Buffer offset_buf, column_buf, weight_buf, mst_buf;
    offset_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(unsigned) * (numVertices + 1), &mext_o[0]);
    column_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(unsigned) * numEdges, &mext_o[1]);
    weight_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(float) * numEdges, &mext_o[2]);
    mst_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                         sizeof(unsigned) * numVertices, &mext_o[3]);

    std::vector<cl::Memory> init;
    init.push_back(offset_buf);
    init.push_back(column_buf);
    init.push_back(weight_buf);
    init.push_back(mst_buf);
    q.enqueueMigrateMemObjects(init, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
    q.finish();

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    ob_in.push_back(offset_buf);
    ob_in.push_back(column_buf);
    ob_in.push_back(weight_buf);
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);

    ob_out.push_back(mst_buf);
    //    q.finish();
    // launch kernel and calculate kernel execution time
    std::cout << "kernel start------" << std::endl;
    gettimeofday(&start_time, 0);
    int j = 0;
    mst_knl.setArg(j++, numVertices);
    mst_knl.setArg(j++, numEdges);
    mst_knl.setArg(j++, sourceID);
    mst_knl.setArg(j++, offset_buf);
    mst_knl.setArg(j++, column_buf);
    mst_knl.setArg(j++, weight_buf);
    mst_knl.setArg(j++, mst_buf);

    q.enqueueTask(mst_knl, &events_write, &events_kernel[0]);

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
    mst_top(numVertices, numEdges, sourceID, offset32, column32, weight32, mst);
#endif
    std::cout << "============================================================" << std::endl;

    bool* edge_color = aligned_alloc<bool>(numEdges);
    memset(edge_color, 0, numEdges);

    unsigned errs = 0;
    float total = 0.0;
    unsigned edge_cnt = 0;
    std::set<unsigned long> tree;
    for (int i = 0; i < numVertices; i++) {
        if (mst[i] == -1) {
            std::cout << "vertex " << i << " not connected with the tree." << std::endl;
            errs++;
        }
        unsigned start = offset32[mst[i]];
        unsigned end = offset32[mst[i] + 1];
        bool found = false;
        for (unsigned j = start; j < end; j++) {
            if (column32[j] == i) {
                unsigned long tmp, left, right;
                if (mst[i] != i) {
                    total = total + weight32[j];
                    left = mst[i];
                    right = i;
                    if (left >= right) {
                        tmp = 0UL | left << 32 | right;
                    } else {
                        tmp = 0UL | right << 32 | left;
                    }
                    tree.insert(tmp);
                }
                found = true;
                if (edge_color[j] == false && mst[i] != i) {
                    edge_color[j] = true;
                    edge_cnt++;
                }
            }
        }
        if (!found) {
            errs++;
            std::cout << "edge not found " << mst[mst[i]] << " " << mst[i] << std::endl;
        }
    }

    if (tree.size() != numVertices - 1) {
        std::cout << "not a tree" << std::endl;
        errs++;
    }
    std::cout << "f784c8c0 total mst weight: " << total << std::endl;

    errs ? logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL)
         : logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);

    return errs;
}
