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

#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#ifndef HLS_TEST
#include "xcl2.hpp"
#endif
#include "utils.hpp"
#include <vector>
#include <cstring>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sys/time.h>
#include "mis_kernel.hpp"
#include "xf_utils_sw/logger.hpp"

// args parser
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

int csr_graph_loading(
    std::string& offset_file_name, std::string& indices_file_name, int* offset, int* indices, int* m, int* nz) {
    std::ifstream offset_file(offset_file_name, std::ios::in);
    std::ifstream indices_file(indices_file_name, std::ios::in);

    if (!offset_file.is_open()) {
        std::cout << "Error: .mtx offset file not found" << std::endl;
        std::cout << offset_file_name << std::endl;
        return -1;
    }

    if (!indices_file.is_open()) {
        std::cout << "Error: .mtx indices file not found" << std::endl;
        std::cout << indices_file_name << std::endl;
        return -1;
    }

    int m_tmp, nz_tmp, n_tmp;

    offset_file >> m_tmp >> n_tmp; // loading vertex number info.
    indices_file >> nz_tmp;        // loading edge number info.

    float* weights = new float[M_EDGE];

    for (int i = 0; i < m_tmp + 1; i++) {
        offset_file >> offset[i];
    }

    for (int i = 0; i < nz_tmp; i++) {
        indices_file >> indices[i] >> weights[i];
    }

    *m = m_tmp;
    *nz = nz_tmp;

    delete[] weights;

    return 0;
}

unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

int main(int argc, const char* argv[]) {
    std::cout << "\n---------------------Maximal Independent Set-----------------\n";
    ArgParser parser(argc, argv);
    // cmd parser
    std::string xclbin_path;
#ifndef HLS_TEST
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }
#endif
    std::string filename_offsets;
    std::string filename_index;
    std::string filename_mis;
    if (!parser.getCmdOption("-o", filename_offsets)) { // offset
        std::cout << "ERROR: [-o] file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-i", filename_index)) { // index
        std::cout << "ERROR: [-i] file path is not set!\n";
        return -1;
    }
    if (!parser.getCmdOption("-mis", filename_mis)) { // mis
        std::cout << "ERROR: [-mis] file path is not set!\n";
        return -1;
    }

    // timing checker
    struct timeval start_time;
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    gettimeofday(&start_time, 0);

    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    cl_int err;

    // Step 1: Initialize the OpenCL environment
    // Creating Context and Command Queeu for selected Device
    cl::Context context(device, NULL, NULL, NULL, &err);
    logger.logCreateContext(err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    logger.logCreateCommandQueue(err);

    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins, NULL, &err);
    logger.logCreateProgram(err);

    cl::Kernel mis_kernel(program, "mis_kernel", &err);
    logger.logCreateProgram(err);
    std::cout << "kernel has been created" << std::endl;

    // Config paras
    int vertex_num;
    int edge_num;

    // Get device buffer
    cl::Buffer db_offset(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int) * M_VERTEX, NULL);
    cl::Buffer db_indices(context, CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, sizeof(int) * M_EDGE, NULL);
    cl::Buffer db_C_group_0(context, CL_MEM_ALLOC_HOST_PTR, sizeof(int) * M_VERTEX, NULL);
    cl::Buffer db_C_group_1(context, CL_MEM_ALLOC_HOST_PTR, sizeof(int) * M_VERTEX, NULL);
    cl::Buffer db_S_group_0(context, CL_MEM_ALLOC_HOST_PTR, sizeof(int) * M_VERTEX, NULL);
    cl::Buffer db_S_group_1(context, CL_MEM_ALLOC_HOST_PTR, sizeof(int) * M_VERTEX, NULL);
    cl::Buffer db_res_out(context, CL_MEM_ALLOC_HOST_PTR, sizeof(int) * M_VERTEX, NULL);

    // Map buffers to kernel arguments, thereby assigning them to specific device memory banks
    mis_kernel.setArg(0, vertex_num);
    mis_kernel.setArg(1, db_offset);
    mis_kernel.setArg(2, db_indices);
    mis_kernel.setArg(3, db_C_group_0);
    mis_kernel.setArg(4, db_C_group_1);
    mis_kernel.setArg(5, db_S_group_0);
    mis_kernel.setArg(6, db_S_group_1);
    mis_kernel.setArg(7, db_res_out);

    // Get host buffer
    int* hb_offset = (int*)q.enqueueMapBuffer(db_offset, CL_TRUE, CL_MAP_WRITE, 0, sizeof(int) * M_VERTEX);
    int* hb_indices = (int*)q.enqueueMapBuffer(db_indices, CL_TRUE, CL_MAP_WRITE, 0, sizeof(int) * M_EDGE);
    int* hb_C_group_0 =
        (int*)q.enqueueMapBuffer(db_C_group_0, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * M_VERTEX);
    int* hb_C_group_1 =
        (int*)q.enqueueMapBuffer(db_C_group_1, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * M_VERTEX);
    int* hb_S_group_0 =
        (int*)q.enqueueMapBuffer(db_S_group_0, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * M_VERTEX);
    int* hb_S_group_1 =
        (int*)q.enqueueMapBuffer(db_S_group_1, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * M_VERTEX);
    int* hb_res_out =
        (int*)q.enqueueMapBuffer(db_res_out, CL_TRUE, CL_MAP_WRITE | CL_MAP_READ, 0, sizeof(int) * M_VERTEX);

    // Initialize the memory used in test from host-side
    for (int i = 0; i < M_VERTEX; i++) {
        hb_C_group_0[i] = 0;
        hb_C_group_1[i] = 0;
        hb_S_group_0[i] = 0;
        hb_S_group_1[i] = 0;
        hb_res_out[i] = 0;
    }

    for (int i = 0; i < M_EDGE; i++) {
        hb_indices[i] = 0;
    }

    std::cout << "graph loading ..." << std::endl;
    csr_graph_loading(filename_offsets, filename_index, hb_offset, hb_indices, &vertex_num, &edge_num);

    // add Buffers to migrate
    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;

    ob_in.push_back(db_offset);
    ob_in.push_back(db_indices);
    ob_in.push_back(db_C_group_0);
    ob_in.push_back(db_C_group_1);
    ob_in.push_back(db_S_group_0);
    ob_in.push_back(db_S_group_1);

    ob_out.push_back(db_C_group_0);
    ob_out.push_back(db_C_group_1);
    ob_out.push_back(db_S_group_0);
    ob_out.push_back(db_S_group_1);
    ob_out.push_back(db_res_out);

    // declare events
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    // ------------------------------------------------------------------------------------
    // Step 3: Run the kernel
    // ------------------------------------------------------------------------------------
    // Set kernel arguments
    // launch kernel and calculate kernel execution time
    std::cout << "kernel start------" << std::endl;
    std::cout << "  vertex number=" << vertex_num << " edge number=" << edge_num << std::endl;

    // Set kernel arguments
    mis_kernel.setArg(0, vertex_num);
    mis_kernel.setArg(1, db_offset);
    mis_kernel.setArg(2, db_indices);
    mis_kernel.setArg(3, db_C_group_0);
    mis_kernel.setArg(4, db_C_group_1);
    mis_kernel.setArg(5, db_S_group_0);
    mis_kernel.setArg(6, db_S_group_1);
    mis_kernel.setArg(7, db_res_out);

    // Schedule transfer of inputs to device memory, execution of kernel, and transfer of outputs back to host memory
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);
    q.enqueueTask(mis_kernel, &events_write, &events_kernel[0]);
    q.enqueueMigrateMemObjects(ob_out, CL_MIGRATE_MEM_OBJECT_HOST, &events_kernel, &events_read[0]);

    // Wait for all scheduled operations to finish
    q.finish();

    struct timeval end_time;
    gettimeofday(&end_time, 0);
    std::cout << "INFO: Finish kernel execution" << std::endl;
    std::cout << "INFO: Finish E2E execution" << std::endl;

    // Profiling times
    unsigned long timeStart, timeEnd, exec_time0;
    std::cout << "-------------------------------------------------------" << std::endl;
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    events_write[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    exec_time0 = (timeEnd - timeStart) / 1000.0;
    std::cout << "INFO: Data transfer from host to device: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    events_read[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    exec_time0 = (timeEnd - timeStart) / 1000.0;
    std::cout << "INFO: Kernel1 Data transfer from device to host: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    exec_time0 = 0;
    for (int i = 0; i < 1; ++i) {
        events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
        events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
        exec_time0 += (timeEnd - timeStart) / 1000.0;

        std::cout << "INFO: Kernel" << i + 1 << " execution: " << (timeEnd - timeStart) / 1000.0 << " us\n";
        std::cout << "-------------------------------------------------------" << std::endl;
    }
    std::cout << "INFO: kernel total execution: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    unsigned long exec_timeE2E = diff(&end_time, &start_time);
    std::cout << "INFO: FPGA execution time:" << exec_timeE2E << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;

    // host-side buffer operation: Check Results and Release Allocated Resources
    int mis_num = 0;
    std::ifstream mis_file(filename_mis, std::ios::out);
    std::string Line1;
    mis_file >> Line1;
    int nerr = 0;

    for (int i = 0; i < vertex_num - 1; i++) {
        // golden mis vertex
        int vertex_golden;
        mis_file >> vertex_golden;
        if (mis_file.peek() == EOF) {
            break;
        } else {
            // kernel output
            int vertex_kernel = hb_res_out[i]; // host_res_out[i];

            // verification
            if (vertex_golden != vertex_kernel) {
                std::cout << " case error - ";
                std::cout << "  golden:" << vertex_golden;
                std::cout << "  kernel_output:" << vertex_kernel << std::endl;
                nerr++;
            }

            mis_num++; // counting mis num
        }
    }

    mis_file.close();
    std::cout << "MIS_number:" << mis_num << std::endl;

    if (!nerr) std::cout << "INFO: case pass!\n";
    nerr ? logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL)
         : logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);

    return nerr;
}