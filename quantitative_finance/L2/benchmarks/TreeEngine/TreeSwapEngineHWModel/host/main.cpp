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
#endif
#include <cstring>
#include <vector>
#include <fstream>
#include <iostream>
#include <sys/time.h>
#include "ap_int.h"
#include "utils.hpp"
#include "tree_engine_kernel.hpp"

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
    std::cout << "\n----------------------Tree Swap (HW) Engine-----------------\n";
    // cmd parser
    std::string run_mode = "hw";
#ifndef HLS_TEST
    ArgParser parser(argc, argv);
    std::string mode;
    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    if (std::getenv("XCL_EMULATION_MODE") != nullptr) {
        run_mode = std::getenv("XCL_EMULATION_MODE");
    }
    std::cout << "[INFO]Running in " << run_mode << " mode" << std::endl;
#endif

    // Allocate Memory in Host Memory
    DT* output[4];
    for (int i = 0; i < 4; i++) {
        output[i] = aligned_alloc<DT>(N * K);
    }

    ScanInputParam0* inputParam0_alloc = aligned_alloc<ScanInputParam0>(1);
    ScanInputParam1* inputParam1_alloc = aligned_alloc<ScanInputParam1>(1);

    // -------------setup params---------------
    int err = 0;
    DT minErr = 10e-10;
    int timestep = 10;
    int len = K;
    if (run_mode == "hw_emu") {
        timestep = 10;
    }

    std::cout << "timestep=" << timestep << std::endl;

    double golden;

    if (timestep == 10) golden = -0.00020198789915012378;
    if (timestep == 50) golden = -0.0002019878994616189;
    if (timestep == 100) golden = -0.0002019878995414788;
    if (timestep == 500) golden = -0.00020198789953431;
    if (timestep == 1000) golden = -0.0002019878991688788;

    double fixedRate = 0.049995924285639641;
    double initTime[12] = {0,
                           1,
                           1.4958904109589042,
                           2,
                           2.4986301369863013,
                           3.0027397260273974,
                           3.4986301369863013,
                           4.0027397260273974,
                           4.4986301369863018,
                           5.0027397260273974,
                           5.4986301369863018,
                           6.0027397260273974};

    int initSize = 12;
    int exerciseCnt[5] = {0, 2, 4, 6, 8};
    int fixedCnt[5] = {0, 2, 4, 6, 8};
    int floatingCnt[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    for (int i = 0; i < 1; i++) {
        inputParam1_alloc[i].index = i;
        inputParam1_alloc[i].type = 0;
        inputParam1_alloc[i].fixedRate = fixedRate;
        inputParam1_alloc[i].timestep = timestep;
        inputParam1_alloc[i].initSize = initSize;
        inputParam1_alloc[i].a = 0.055228873373796609;
        inputParam1_alloc[i].sigma = 0.0061062754654949824;
        inputParam1_alloc[i].flatRate = 0.04875825;
        inputParam0_alloc[i].x0 = 0.0;
        inputParam0_alloc[i].nominal = 1000.0;
        inputParam0_alloc[i].spread = 0.0;
        for (int j = 0; j < initSize; j++) {
            inputParam0_alloc[i].initTime[j] = initTime[j];
        }
        for (int j = 0; j < ExerciseLen; j++) {
            inputParam1_alloc[i].exerciseCnt[j] = exerciseCnt[j];
        }
        for (int j = 0; j < FloatingLen; j++) {
            inputParam1_alloc[i].floatingCnt[j] = floatingCnt[j];
        }
        for (int j = 0; j < FixedLen; j++) {
            inputParam1_alloc[i].fixedCnt[j] = fixedCnt[j];
        }
    }

#ifndef HLS_TEST
    // do pre-process on CPU
    struct timeval start_time, end_time, test_time;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
#ifdef SW_EMU_TEST
    cl::CommandQueue q(context, device); //, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#else
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#endif
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    // load xclbin
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    cl::Kernel kernel1_TreeEngine(program, "scanTreeKernel1");
    cl::Kernel kernel2_TreeEngine(program, "scanTreeKernel2");
#if KN > 2
    cl::Kernel kernel3_TreeEngine(program, "scanTreeKernel3");
#endif
#if KN > 3
    cl::Kernel kernel4_TreeEngine(program, "scanTreeKernel4");
#endif
    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_in0[4];
    cl_mem_ext_ptr_t mext_in1[4];
    cl_mem_ext_ptr_t mext_out[4];

#ifdef USE_HBM
    mext_in0[0] = {XCL_BANK0, inputParam0_alloc, 0};
    mext_in0[1] = {XCL_BANK1, inputParam0_alloc, 0};

    mext_in1[0] = {XCL_BANK0, inputParam1_alloc, 0};
    mext_in1[1] = {XCL_BANK1, inputParam1_alloc, 0};

    mext_out[0] = {XCL_BANK0, output[0], 0};
    mext_out[1] = {XCL_BANK1, output[1], 0};
#else
    mext_in0[0] = {XCL_MEM_DDR_BANK0, inputParam0_alloc, 0};
    mext_in0[1] = {XCL_MEM_DDR_BANK1, inputParam0_alloc, 0};
    mext_in0[2] = {XCL_MEM_DDR_BANK2, inputParam0_alloc, 0};

    mext_in1[0] = {XCL_MEM_DDR_BANK0, inputParam1_alloc, 0};
    mext_in1[1] = {XCL_MEM_DDR_BANK1, inputParam1_alloc, 0};
    mext_in1[2] = {XCL_MEM_DDR_BANK2, inputParam1_alloc, 0};

    mext_out[0] = {XCL_MEM_DDR_BANK0, output[0], 0};
    mext_out[1] = {XCL_MEM_DDR_BANK1, output[1], 0};
    mext_out[2] = {XCL_MEM_DDR_BANK2, output[2], 0};
#if KN == 4
    mext_in0[3] = {XCL_MEM_DDR_BANK3, inputParam0_alloc, 0};
    mext_in1[3] = {XCL_MEM_DDR_BANK3, inputParam1_alloc, 0};
    mext_out[3] = {XCL_MEM_DDR_BANK3, output[3], 0};
#endif
#endif

    // create device buffer and map dev buf to host buf
    cl::Buffer output_buf[4];
    cl::Buffer inputParam0_buf[4], inputParam1_buf[4];
    for (int i = 0; i < KN; i++) {
        inputParam0_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(ScanInputParam0), &mext_in0[i]);
        inputParam1_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(ScanInputParam1), &mext_in1[i]);
        output_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   sizeof(DT) * N * K, &mext_out[i]);
    }

    std::vector<cl::Memory> ob_in;
    for (int i = 0; i < KN; i++) {
        ob_in.push_back(inputParam0_buf[i]);
        ob_in.push_back(inputParam1_buf[i]);
    }

    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, nullptr);

    std::vector<cl::Memory> ob_out;
    for (int i = 0; i < KN; i++) {
        ob_out.push_back(output_buf[i]);
    }

    q.finish();

    // launch kernel and calculate kernel execution time
    std::cout << "kernel start------" << std::endl;
    std::vector<cl::Event> events_kernel(4);
    gettimeofday(&start_time, 0);

    int j = 0;
    kernel1_TreeEngine.setArg(j++, len);
    kernel1_TreeEngine.setArg(j++, inputParam0_buf[0]);
    kernel1_TreeEngine.setArg(j++, inputParam1_buf[0]);
    kernel1_TreeEngine.setArg(j++, output_buf[0]);

    j = 0;
    kernel2_TreeEngine.setArg(j++, len);
    kernel2_TreeEngine.setArg(j++, inputParam0_buf[1]);
    kernel2_TreeEngine.setArg(j++, inputParam1_buf[1]);
    kernel2_TreeEngine.setArg(j++, output_buf[1]);
#if KN > 2
    j = 0;
    kernel3_TreeEngine.setArg(j++, len);
    kernel3_TreeEngine.setArg(j++, inputParam0_buf[2]);
    kernel3_TreeEngine.setArg(j++, inputParam1_buf[2]);
    kernel3_TreeEngine.setArg(j++, output_buf[2]);
#endif
#if KN > 3
    j = 0;
    kernel4_TreeEngine.setArg(j++, len);
    kernel4_TreeEngine.setArg(j++, inputParam0_buf[3]);
    kernel4_TreeEngine.setArg(j++, inputParam1_buf[3]);
    kernel4_TreeEngine.setArg(j++, output_buf[3]);
#endif
    for (int i = 0; i < 1; ++i) {
        q.enqueueTask(kernel1_TreeEngine, nullptr, &events_kernel[0]);
        q.enqueueTask(kernel2_TreeEngine, nullptr, &events_kernel[1]);
#if KN > 2
        q.enqueueTask(kernel3_TreeEngine, nullptr, &events_kernel[2]);
#endif
#if KN > 3
        q.enqueueTask(kernel4_TreeEngine, nullptr, &events_kernel[3]);
#endif
    }

    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;
    unsigned long time_start, time_end;
    unsigned long time1, time2;
    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_kernel[0].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Kernel-1 Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    time_start = time1;
    time_end = time2;
    events_kernel[1].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_kernel[1].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Kernel-2 Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    if (time_start > time1) time_start = time1;
    if (time_end < time2) time_end = time2;
#if KN > 2
    events_kernel[2].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_kernel[2].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Kernel-3 Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    if (time_start > time1) time_start = time1;
    if (time_end < time2) time_end = time2;
#endif
#if KN > 3
    events_kernel[3].getProfilingInfo(CL_PROFILING_COMMAND_START, &time1);
    events_kernel[3].getProfilingInfo(CL_PROFILING_COMMAND_END, &time2);
    std::cout << "Kernel-4 Execution time " << (time2 - time1) / 1000000.0 << "ms" << std::endl;
    if (time_start > time1) time_start = time1;
    if (time_end < time2) time_end = time2;
#endif

    std::cout << "FPGA Execution time " << (time_end - time_start) / 1000000.0 << "ms" << std::endl;

    std::cout << "CPU Execution time " << tvdiff(&start_time, &end_time) / 1000.0 << "ms" << std::endl;
    q.enqueueMigrateMemObjects(ob_out, 1, nullptr, nullptr);
    q.finish();
#else
    scanTreeKernel1(len, inputParam0_alloc, inputParam1_alloc, output[0]);
    scanTreeKernel2(len, inputParam0_alloc, inputParam1_alloc, output[1]);
#endif
    for (int i = 0; i < KN; i++) {
        for (int j = 0; j < len; j++) {
            DT out = output[i][j];
            if (std::fabs(out - golden) > minErr) {
                err++;
                std::cout << "[ERROR] Kernel-" << i + 1 << ": NPV[" << j << "]= " << std::setprecision(15) << out
                          << " ,diff/NPV= " << (out - golden) / golden << std::endl;
            }
        }
    }
    std::cout << "NPV[" << 0 << "]= " << std::setprecision(15) << output[0][0]
              << " ,diff/NPV= " << (output[0][0] - golden) / golden << std::endl;
    return err;
}
