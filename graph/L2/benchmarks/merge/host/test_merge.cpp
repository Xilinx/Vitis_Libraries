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
#include "common.hpp"
#include "xcl2.hpp"
#include "xf_utils_sw/logger.hpp"
#include "utils.hpp"
#include "ap_int.h"
#include <hls_stream.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <limits>
#include <algorithm>
#include <map>
#include <iterator>
using namespace std;
#define COMP_GLOBAL

void readfile(string filename, int* ptr) {
    int i = 0;
    std::string line;
    std::ifstream myfile(filename.c_str());
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            ptr[i] = std::stoi(line);
            i++;
        }
        myfile.close();
    } else {
        std::cout << "Unable to open file: " << filename << std::endl;
    }
}
void readfile(string filename, float* ptr) {
    int i = 0;
    std::string line;
    std::ifstream myfile(filename.c_str());
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            ptr[i] = atof(line.c_str());
            i++;
        }
        myfile.close();
    } else {
        std::cout << "Unable to open file: " << filename << std::endl;
    }
}

template <class T>
int write_file(int num, T* ptr, std::string name) {
    int i;
    ofstream ofile;
    ofile.open(name);
    for (i = 0; i < num; i++) ofile << ptr[i] << endl;
    ofile.close();
    return 0;
}

// class ArgParser {
//   public:
//    ArgParser(int& argc, const char** argv) {
//        for (int i = 1; i < argc; ++i) mTokens.push_back(std::string(argv[i]));
//    }
//    bool getCmdOption(std::string option, std::string& value) const {
//        std::vector<std::string>::const_iterator itr
//            = std::find(this->mTokens.begin(), this->mTokens.end(), option);
//        if (itr != this->mTokens.end() && ++itr != this->mTokens.end()) {
//            value = *itr;
//            return true;
//        }
//        return false;
//    }
//
//   private:
//    std::vector<std::string> mTokens;
//};

void data_init(int num_v, DF_V_T* count_c_single, DF_V_T* jump, DF_V_T* count_c, DF_V_T* index_c) {
    for (int i = 0; i < num_v; i++) {
        count_c_single[i] = 0;
        jump[i] = -1;
        count_c[i] = 0;
        index_c[i] = 0;
    }
}

void sort_by_offset(
    int num_c_out, int num_e_out, int* offset_out, int* edges_out, float* weights_out, std::pair<int, float>* pair_ew) {
    for (int i = 0; i < num_e_out; i++) {
        pair_ew[i].first = edges_out[i];
        pair_ew[i].second = weights_out[i];
    }
    for (int i = 0; i < num_c_out; i++) {
        int start = offset_out[i];
        int end = offset_out[i + 1];
        std::sort(pair_ew + start, pair_ew + end);
    }
    for (int i = 0; i < num_e_out; i++) {
        edges_out[i] = pair_ew[i].first;
        weights_out[i] = pair_ew[i].second;
    }
}

bool arg_checking(ArgParser parser, std::string option, std::string& value, bool check_exist) {
    if (!parser.getCmdOption(option, value)) { // offset
        std::cout << "ERROR: " << option << " is not set!\n";
        return 0;
    } else {
        if (check_exist) {
            ifstream f(value.c_str());
            if (!f.good()) {
                std::cout << "ERROR: " << value << " not exist" << std::endl;
                return 0;
            }
        }
    }
    return 1;
}

int main(int argc, const char* argv[]) {
    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return -1;
    }
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;
    std::string offsetfile;
    std::string edgefile;
    std::string weightfile;
    std::string cfile;
    std::string out_offsetfile;
    std::string out_edgefile;
    std::string out_weightfile;
    std::string golden_offsetfile;
    std::string golden_edgefile;
    std::string golden_weightfile;

    if (!arg_checking(parser, "-io", offsetfile, 1)) return -1;
    if (!arg_checking(parser, "-ie", edgefile, 1)) return -1;
    if (!arg_checking(parser, "-iw", weightfile, 1)) return -1;
    if (!arg_checking(parser, "-ic", cfile, 1)) return -1;
    if (!arg_checking(parser, "-oo", out_offsetfile, 0)) return -1;
    if (!arg_checking(parser, "-oe", out_edgefile, 0)) return -1;
    if (!arg_checking(parser, "-ow", out_weightfile, 0)) return -1;
    if (!arg_checking(parser, "-go", golden_offsetfile, 1)) return -1;
    if (!arg_checking(parser, "-ge", golden_edgefile, 1)) return -1;
    if (!arg_checking(parser, "-gw", golden_weightfile, 1)) return -1;

    int numVertices = 0;
    int numEdges = 0;
    int num = 0;
    int max = 0;
    std::string line;
    std::ifstream myfile(offsetfile.c_str());
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            num++;
            numEdges = std::stoi(line);
        }
        myfile.close();
    } else {
        std::cout << "Unable to open file";
    }
    std::ifstream myfile2(cfile.c_str());
    if (myfile2.is_open()) {
        while (getline(myfile2, line)) {
            max = max > std::stoi(line) ? max : std::stoi(line);
        }
        myfile2.close();
    } else {
        std::cout << "Unable to open file";
    }

    numVertices = num - 1;
    int num_c_out = max + 1;
    int buffer_size1 = numEdges;
    int* offset_in = aligned_alloc<int>(num);
    int* edges_in = aligned_alloc<int>(buffer_size1);
    float* weights_in = aligned_alloc<float>(buffer_size1);
    int* c = aligned_alloc<int>(num - 1);
    int* offset_out = aligned_alloc<int>(num);
    int* edges_out = aligned_alloc<int>(buffer_size1);
    float* weights_out = aligned_alloc<float>(buffer_size1);
    int* count_c_single = aligned_alloc<int>(num);
    int* jump = aligned_alloc<int>(num);
    int* count_c = aligned_alloc<int>(num);
    int* index_c = aligned_alloc<int>(num);
#ifdef COMP_GLOBAL
    std::pair<int, float>* pair_ew = (std::pair<int, float>*)malloc((numEdges) * sizeof(std::pair<int, float>));
    int* offset_golden = aligned_alloc<int>(num);
    int* edges_golden = aligned_alloc<int>(numEdges);
    float* weights_golden = aligned_alloc<float>(numEdges);
#endif

    readfile(offsetfile, offset_in);
    readfile(edgefile, edges_in);
    readfile(weightfile, weights_in);
    readfile(cfile, c);

    int* num_e_out = aligned_alloc<int>(1);
    data_init(num - 1, count_c_single, jump, count_c, index_c);

    struct timeval start_time, end_time;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device, NULL, NULL, NULL, &fail);
    logger.logCreateContext(fail);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
    logger.logCreateCommandQueue(fail);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    devices[0] = device;
    cl::Program program(context, devices, xclBins, NULL, &fail);
    logger.logCreateProgram(fail);
    cl::Kernel merge;
    merge = cl::Kernel(program, "merge_kernel", &fail);
    logger.logCreateKernel(fail);
    std::cout << "kernel has been created" << std::endl;

    // create device buffer and map dev buf to host buf
    cl::Buffer offset_in_buf, edges_in_buf, weights_in_buf, c_buf;
    cl::Buffer edges_in2_buf, weights_in2_buf;
    cl::Buffer offset_out_buf, edges_out_buf, weights_out_buf;
    cl::Buffer count_c_single_buf, jump_buf, num_e_out_buf, count_c_buf, index_c_buf;

    cl_int err;
    num_e_out_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, 1 * sizeof(int), num_e_out, &err);
    offset_in_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, num * sizeof(int), offset_in, &err);
    edges_in_buf =
        cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, buffer_size1 * sizeof(int), edges_in, &err);
    weights_in_buf =
        cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, buffer_size1 * sizeof(float), weights_in, &err);
    c_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, (num - 1) * sizeof(int), c, &err);
    offset_out_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, num * sizeof(int), offset_out, &err);
    edges_out_buf =
        cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, buffer_size1 * sizeof(int), edges_out, &err);
    weights_out_buf =
        cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, buffer_size1 * sizeof(float), weights_out, &err);
    count_c_single_buf =
        cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, num * sizeof(int), count_c_single, &err);
    jump_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, num * sizeof(int), jump, &err);
    count_c_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, num * sizeof(int), count_c, &err);
    index_c_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE, num * sizeof(int), index_c, &err);

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    ob_in.push_back(offset_in_buf);
    ob_in.push_back(edges_in_buf);
    ob_in.push_back(weights_in_buf);
    ob_in.push_back(c_buf);
    ob_in.push_back(count_c_single_buf);
    ob_in.push_back(jump_buf);
    ob_in.push_back(count_c_buf);
    ob_in.push_back(index_c_buf);

    ob_out.push_back(num_e_out_buf);
    ob_out.push_back(offset_out_buf);
    ob_out.push_back(edges_out_buf);
    ob_out.push_back(weights_out_buf);

    // launch kernel and calculate kernel execution time
    std::cout << "INFO: kernel start------" << std::endl;
    gettimeofday(&start_time, 0);
    int j = 0;
    merge.setArg(j++, num - 1);
    merge.setArg(j++, numEdges);
    merge.setArg(j++, num_c_out);
    merge.setArg(j++, num_e_out_buf);
    merge.setArg(j++, offset_in_buf);
    merge.setArg(j++, edges_in_buf);
    merge.setArg(j++, weights_in_buf);
    merge.setArg(j++, c_buf);
    merge.setArg(j++, offset_out_buf);
    merge.setArg(j++, edges_out_buf);
    merge.setArg(j++, weights_out_buf);
    merge.setArg(j++, count_c_single_buf);
    merge.setArg(j++, jump_buf);
    merge.setArg(j++, count_c_buf);
    merge.setArg(j++, index_c_buf);
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);
    q.enqueueTask(merge, &events_write, &events_kernel[0]);
    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();
    gettimeofday(&end_time, 0);

    std::cout << "INFO: kernel end------" << std::endl;
    std::cout << "INFO: Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;

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

#ifdef COMP_GLOBAL
    sort_by_offset(num_c_out, num_e_out[0], offset_out, edges_out, weights_out, pair_ew);
    offset_out[0] = 0;
    write_file<int>(num_c_out + 1, offset_out, out_offsetfile);
    write_file<int>(num_e_out[0], edges_out, out_edgefile);
    write_file<float>(num_e_out[0], weights_out, out_weightfile);

    std::string diff_o = "diff --brief " + golden_offsetfile + " " + out_offsetfile;
    int ro = system(diff_o.c_str());
    if (ro) {
        printf("Test offset failed\n");
        return 1;
    }
    std::string diff_e = "diff --brief " + golden_edgefile + " " + out_edgefile;
    int re = system(diff_e.c_str());
    if (re) {
        printf("Test edge failed\n");
        return 1;
    }
    std::string diff_w = "diff --brief " + golden_weightfile + " " + out_weightfile;
    int rw = system(diff_w.c_str());
    if (rw) {
        printf("Test weight failed\n");
        return 1;
    }
    printf("Test passed\n");
    return 0;

#endif

    free(offset_in);
    free(edges_in);
    free(weights_in);
    free(c);
    free(offset_out);
    free(edges_out);
    free(weights_out);
    free(count_c_single);
    free(jump);
#ifdef COMP_GLOBAL
    free(offset_golden);
    free(edges_golden);
    free(weights_golden);
    free(pair_ew);
#endif
    return 0;
}
