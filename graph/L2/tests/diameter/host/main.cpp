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
#endif

#include "xf_utils_sw/logger.hpp"
#include "ap_int.h"
#include "utils.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <algorithm>
#include <set>
#include <queue>
#include <math.h>

#ifndef INTERFACE_MEMSIZE
#define INTERFACE_MEMSIZE (100000)
#endif

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

void sssp_TB(unsigned int numVert,
             unsigned int numEdge,
             unsigned int source,
             unsigned int* offset,
             unsigned int* column,
             float* weight,
             float* distance) {
    for (int i = 0; i < numVert; i++) {
        distance[i] = std::numeric_limits<float>::infinity();
    }

    std::queue<unsigned int> q;

    q.push(source);
    distance[source] = 0;
    while (!q.empty()) {
        unsigned int tmp = q.front();
        for (int i = offset[tmp]; i < offset[tmp + 1]; i++) {
            float fromDist = distance[tmp];
            float toDist = distance[column[i]];
            float curDist = fromDist + weight[i];
            if (curDist < toDist) {
                distance[column[i]] = curDist;
                q.push(column[i]);
            }
        }
        q.pop();
    }
}

int main(int argc, const char* argv[]) {
    std::cout << "\n---------------------diameter Traversal Test----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;
#ifndef HLS_TEST
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }
#endif

    int sourceID = 30;
#ifndef HLS_TEST
    std::string offsetfile;
    std::string columnfile;

    if (!parser.getCmdOption("-o", offsetfile)) { // offset
        std::cout << "ERROR: offsetfile is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-c", columnfile)) { // column
        std::cout << "ERROR: columnfile is not set!\n";
        return -1;
    }
#else
    std::string offsetfile = "../data-csr-offset.mtx";
    std::string columnfile = "../data-csr-indicesweights.mtx";
#endif

    char line[1024] = {0};
    int index = 0;

    int numVert;
    int maxVertexId;
    int numEdges;

    std::fstream offsetfstream(offsetfile.c_str(), std::ios::in);
    if (!offsetfstream) {
        std::cout << "Error : " << offsetfile << " file doesn't exist !" << std::endl;
        exit(1);
    }

    offsetfstream.getline(line, sizeof(line));
    std::stringstream numOdata(line);
    numOdata >> numVert;
    numOdata >> maxVertexId;

    unsigned* offset32 = aligned_alloc<unsigned>(INTERFACE_MEMSIZE);

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

    // read file finish
    std::cout << "File reading finish" << std::endl;
    index = 0;

    columnfstream.getline(line, sizeof(line));
    std::stringstream numCdata(line);
    numCdata >> numEdges;
    std::cout << "Number of edges is " << numEdges << std::endl;

    unsigned* column32 = aligned_alloc<unsigned>(INTERFACE_MEMSIZE);
    float* weight32 = aligned_alloc<float>(INTERFACE_MEMSIZE);

    while (columnfstream.getline(line, sizeof(line))) {
        std::stringstream data(line);
        data >> column32[index];
        data >> weight32[index];
        index++;
    }

    std::cout << "Weight data reading finish" << std::endl;

    float* max_dist = aligned_alloc<float>(INTERFACE_MEMSIZE);
    unsigned* source = aligned_alloc<unsigned>(INTERFACE_MEMSIZE);
    unsigned* destination = aligned_alloc<unsigned>(INTERFACE_MEMSIZE);

    std::cout << "Start Kernl part" << std::endl;

    // do pre-process on CPU
    struct timeval start_time, end_time;
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);

    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    cl_int err;
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

    cl::Kernel diameter(program, "diameter_kernel", &err);
    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_o[6];
    mext_o[0] = {2, offset32, diameter()};
    mext_o[1] = {3, column32, diameter()};
    mext_o[2] = {4, weight32, diameter()};
    mext_o[3] = {5, max_dist, diameter()};
    mext_o[4] = {6, source, diameter()};
    mext_o[5] = {7, destination, diameter()};

    // create device buffer and map dev buf to host buf
    cl::Buffer offset_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                       sizeof(unsigned) * (INTERFACE_MEMSIZE), &mext_o[0]);

    cl::Buffer column_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                       sizeof(unsigned) * (INTERFACE_MEMSIZE), &mext_o[1]);

    cl::Buffer weight_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                       sizeof(float) * (INTERFACE_MEMSIZE), &mext_o[2]);

    cl::Buffer max_dist_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(unsigned) * (INTERFACE_MEMSIZE), &mext_o[3]);

    cl::Buffer src_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(unsigned) * (INTERFACE_MEMSIZE), &mext_o[4]);

    cl::Buffer des_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(unsigned) * (INTERFACE_MEMSIZE), &mext_o[5]);

    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    std::vector<cl::Memory> ob_in;
    ob_in.push_back(column_buf);
    ob_in.push_back(offset_buf);
    ob_in.push_back(weight_buf);

    std::vector<cl::Memory> ob_out;
    ob_out.push_back(max_dist_buf);
    ob_out.push_back(src_buf);
    ob_out.push_back(des_buf);

    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);

    // launch kernel and calculate kernel execution time
    std::cout << "kernel start------" << std::endl;
    std::cout << "Input: numVertex=" << numVert << ", numEdges=" << numEdges << std::endl;
    gettimeofday(&start_time, 0);
    int j = 0;
    diameter.setArg(j++, numVert);
    diameter.setArg(j++, numEdges);
    diameter.setArg(j++, offset_buf);
    diameter.setArg(j++, column_buf);
    diameter.setArg(j++, weight_buf);
    diameter.setArg(j++, max_dist_buf);
    diameter.setArg(j++, src_buf);
    diameter.setArg(j++, des_buf);
    // q.enqueueTask(diameter, &events_write, &events_kernel[0]);
    q.enqueueTask(diameter, &events_write, &events_kernel[0]);

    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();

    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;
    std::cout << "Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;

    cl_ulong ts, te;
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
    float elapsed = ((float)te - (float)ts) / 1000000.0;
    logger.info(xf::common::utils_sw::Logger::Message::TIME_H2D_MS, elapsed);

    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
    elapsed = ((float)te - (float)ts) / 1000000.0;
    logger.info(xf::common::utils_sw::Logger::Message::TIME_KERNEL_MS, elapsed);

    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &ts);
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &te);
    elapsed = ((float)te - (float)ts) / 1000000.0;
    logger.info(xf::common::utils_sw::Logger::Message::TIME_D2H_MS, elapsed);

    std::cout << "============================================================" << std::endl;
    unsigned errs = 0;

    float* distance = aligned_alloc<float>(numVert);
    sssp_TB(numVert, numEdges, source[0], offset32, column32, weight32, distance);
    if (std::fabs(distance[destination[0]] - max_dist[0]) / distance[destination[0]] > 0.0001) {
        std::cout << "source, destination, distance mismatch" << std::endl;
        err++;
    }
    if (distance[destination[0]] == std::numeric_limits<float>::infinity()) {
        std::cout << "not linked source destination found" << std::endl;
        err++;
    }
    std::cout << "source: " << source[0] << " destination: " << destination[0] << std::endl;
    std::cout << "435f8e47 calculated diameter: " << max_dist[0] << std::endl;

    errs ? logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL)
         : logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);

    return errs;
}
