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
#include <iostream>
#include "utils.hpp"
#ifndef HLS_TEST
#include "xcl2.hpp"
#endif

#include <math.h>
#include "kernel_mceuropeanengine.hpp"

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
void print_result(double* out1, double* out2, double* out3, double* out4) {
    if (KN == 4)
        std::cout << "FPGA result:\n"
                  << "            Kernel 0 - " << out1[0] << "            Kernel 1 - " << out2[0]
                  << "            Kernel 2 - " << out3[0] << "            Kernel 3 - " << out4[0] << std::endl;
    else if (KN == 2) {
        std::cout << "FPGA result:\n"
                  << "            Kernel 0 - " << out1[0] << "            Kernel 1 - " << out2[0] << std::endl;
    }
}

int main(int argc, const char* argv[]) {
    std::cout << "\n----------------------MC(European) Engine-----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;

    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    struct timeval st_time, end_time;
    DtUsed* out0_a = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out1_a = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out2_a = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out3_a = aligned_alloc<DtUsed>(OUTDEP);

    DtUsed* out0_b = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out1_b = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out2_b = aligned_alloc<DtUsed>(OUTDEP);
    DtUsed* out3_b = aligned_alloc<DtUsed>(OUTDEP);
    // test data
    unsigned int timeSteps = 1;
    DtUsed requiredTolerance = 0.02;
    DtUsed underlying = 36;
    DtUsed riskFreeRate = 0.06;
    DtUsed volatility = 0.20;
    DtUsed dividendYield = 0.0;
    DtUsed strike = 40;
    bool optionType = 1;
    DtUsed timeLength = 1;

    unsigned int requiredSamples = 0; // 262144; // 48128;//0;//1024;//0;
    unsigned int maxSamples = 0;
    //
    unsigned int loop_nm = 1024;
    std::string mode_emu = "hw";
    if (std::getenv("XCL_EMULATION_MODE") != nullptr) {
        mode_emu = std::getenv("XCL_EMULATION_MODE");
    }

#ifdef HLS_TEST
    int num_rep = 20;
#else
    int num_rep = 20;
    std::string num_str;
    if (parser.getCmdOption("-rep", num_str)) {
        try {
            num_rep = std::stoi(num_str);
        } catch (...) {
            num_rep = 1;
        }
    }
    if (mode_emu.compare("hw_emu") == 0) {
        loop_nm = 1;
        num_rep = 1;
        requiredSamples = 1024;
    } else if (mode_emu.compare("sw_emu") == 0) {
        loop_nm = 1;
        num_rep = 10;
    }
    std::cout << "loop_nm = " << loop_nm << std::endl;
    std::cout << "num_rep = " << num_rep << std::endl;
    std::cout << "KN = " << KN << std::endl;

#endif

#ifdef HLS_TEST
    kernel_mc_0(loop_nm, underlying, volatility, dividendYield, riskFreeRate, timeLength, strike, optionType, out0_b,
                requiredTolerance, requiredSamples, timeSteps, maxSamples);
#if KN > 1
    kernel_mc_1(loop_nm, underlying, volatility, dividendYield, riskFreeRate, timeLength, strike, optionType, out1_b,
                requiredTolerance, requiredSamples, timeSteps, maxSamples);
#endif
#if KN > 2
    kernel_mc_2(loop_nm, underlying, volatility, dividendYield, riskFreeRate, timeLength, strike, optionType, out2_b,
                requiredTolerance, requiredSamples, timeSteps, maxSamples);
#endif
#if KN > 3
    kernel_mc_3(loop_nm, underlying, volatility, dividendYield, riskFreeRate, timeLength, strike, optionType, out3_b,
                requiredTolerance, requiredSamples, timeSteps, maxSamples);
#endif
    print_result(out0_b, out1_b, out2_b, out3_b);
#else

    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];
    cl::Context context(device);
#ifdef SW_EMU
    // hls::exp and hls::log have bug in multi-thread.
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE); // | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#else
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
#endif
    std::string devName = device.getInfo<CL_DEVICE_NAME>();

    std::cout << "Selected Device " << devName << "\n";

    cl::Program::Binaries xclbins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);

    cl::Program program(context, devices, xclbins);

    cl::Kernel kernel0[2];
    cl::Kernel kernel1[2];
    cl::Kernel kernel2[2];
    cl::Kernel kernel3[2];
    for (int i = 0; i < 2; ++i) {
        kernel0[i] = cl::Kernel(program, "kernel_mc_0");
#if KN > 1
        kernel1[i] = cl::Kernel(program, "kernel_mc_1");
#endif
#if KN > 2
        kernel2[i] = cl::Kernel(program, "kernel_mc_2");
#endif
#if KN > 3
        kernel3[i] = cl::Kernel(program, "kernel_mc_3");
#endif
    }
    std::cout << "Kernel has been created\n";

    cl_mem_ext_ptr_t mext_out_a[4];
    cl_mem_ext_ptr_t mext_out_b[4];
#ifndef USE_HBM
    mext_out_a[0] = {XCL_MEM_DDR_BANK0, out0_a, 0};
    mext_out_a[1] = {XCL_MEM_DDR_BANK1, out1_a, 0};
    mext_out_a[2] = {XCL_MEM_DDR_BANK2, out2_a, 0};
    mext_out_a[3] = {XCL_MEM_DDR_BANK3, out3_a, 0};

    mext_out_b[0] = {XCL_MEM_DDR_BANK0, out0_b, 0};
    mext_out_b[1] = {XCL_MEM_DDR_BANK1, out1_b, 0};
    mext_out_b[2] = {XCL_MEM_DDR_BANK2, out2_b, 0};
    mext_out_b[3] = {XCL_MEM_DDR_BANK3, out3_b, 0};
#else
    mext_out_a[0] = {XCL_BANK0, out0_a, 0};
    mext_out_a[1] = {XCL_BANK1, out1_a, 0};
    mext_out_a[2] = {XCL_BANK2, out2_a, 0};
    mext_out_a[3] = {XCL_BANK3, out3_a, 0};

    mext_out_b[0] = {XCL_BANK0, out0_b, 0};
    mext_out_b[1] = {XCL_BANK1, out1_b, 0};
    mext_out_b[2] = {XCL_BANK2, out2_b, 0};
    mext_out_b[3] = {XCL_BANK3, out3_b, 0};
#endif
    cl::Buffer out_buff_a[KN];
    cl::Buffer out_buff_b[KN];
    for (int i = 0; i < KN; i++) {
        out_buff_a[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   (size_t)(OUTDEP * sizeof(DtUsed)), &mext_out_a[i]);
        out_buff_b[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   (size_t)(OUTDEP * sizeof(DtUsed)), &mext_out_b[i]);
    }
    std::vector<std::vector<cl::Event> > kernel_events(num_rep);
    std::vector<std::vector<cl::Event> > read_events(num_rep);
    for (int i = 0; i < num_rep; ++i) {
        kernel_events[i].resize(KN);
        read_events[i].resize(1);
    }
    int j = 0;
    kernel0[0].setArg(j++, loop_nm);
    kernel0[0].setArg(j++, underlying);
    kernel0[0].setArg(j++, volatility);
    kernel0[0].setArg(j++, dividendYield);
    kernel0[0].setArg(j++, riskFreeRate);
    kernel0[0].setArg(j++, timeLength);
    kernel0[0].setArg(j++, strike);
    kernel0[0].setArg(j++, optionType);
    kernel0[0].setArg(j++, out_buff_a[0]);
    kernel0[0].setArg(j++, requiredTolerance);
    kernel0[0].setArg(j++, requiredSamples);
    kernel0[0].setArg(j++, timeSteps);
    kernel0[0].setArg(j++, maxSamples);
    j = 0;
    kernel0[1].setArg(j++, loop_nm);
    kernel0[1].setArg(j++, underlying);
    kernel0[1].setArg(j++, volatility);
    kernel0[1].setArg(j++, dividendYield);
    kernel0[1].setArg(j++, riskFreeRate);
    kernel0[1].setArg(j++, timeLength);
    kernel0[1].setArg(j++, strike);
    kernel0[1].setArg(j++, optionType);
    kernel0[1].setArg(j++, out_buff_b[0]);
    kernel0[1].setArg(j++, requiredTolerance);
    kernel0[1].setArg(j++, requiredSamples);
    kernel0[1].setArg(j++, timeSteps);
    kernel0[1].setArg(j++, maxSamples);
#if KN > 1
    j = 0;
    kernel1[0].setArg(j++, loop_nm);
    kernel1[0].setArg(j++, underlying);
    kernel1[0].setArg(j++, volatility);
    kernel1[0].setArg(j++, dividendYield);
    kernel1[0].setArg(j++, riskFreeRate);
    kernel1[0].setArg(j++, timeLength);
    kernel1[0].setArg(j++, strike);
    kernel1[0].setArg(j++, optionType);
    kernel1[0].setArg(j++, out_buff_a[1]);
    kernel1[0].setArg(j++, requiredTolerance);
    kernel1[0].setArg(j++, requiredSamples);
    kernel1[0].setArg(j++, timeSteps);
    kernel1[0].setArg(j++, maxSamples);
    j = 0;
    kernel1[1].setArg(j++, loop_nm);
    kernel1[1].setArg(j++, underlying);
    kernel1[1].setArg(j++, volatility);
    kernel1[1].setArg(j++, dividendYield);
    kernel1[1].setArg(j++, riskFreeRate);
    kernel1[1].setArg(j++, timeLength);
    kernel1[1].setArg(j++, strike);
    kernel1[1].setArg(j++, optionType);
    kernel1[1].setArg(j++, out_buff_b[1]);
    kernel1[1].setArg(j++, requiredTolerance);
    kernel1[1].setArg(j++, requiredSamples);
    kernel1[1].setArg(j++, timeSteps);
    kernel1[1].setArg(j++, maxSamples);
#endif
#if KN > 2
    j = 0;
    kernel2[0].setArg(j++, loop_nm);
    kernel2[0].setArg(j++, underlying);
    kernel2[0].setArg(j++, volatility);
    kernel2[0].setArg(j++, dividendYield);
    kernel2[0].setArg(j++, riskFreeRate);
    kernel2[0].setArg(j++, timeLength);
    kernel2[0].setArg(j++, strike);
    kernel2[0].setArg(j++, optionType);
    kernel2[0].setArg(j++, out_buff_a[2]);
    kernel2[0].setArg(j++, requiredTolerance);
    kernel2[0].setArg(j++, requiredSamples);
    kernel2[0].setArg(j++, timeSteps);
    kernel2[0].setArg(j++, maxSamples);
    j = 0;
    kernel2[1].setArg(j++, loop_nm);
    kernel2[1].setArg(j++, underlying);
    kernel2[1].setArg(j++, volatility);
    kernel2[1].setArg(j++, dividendYield);
    kernel2[1].setArg(j++, riskFreeRate);
    kernel2[1].setArg(j++, timeLength);
    kernel2[1].setArg(j++, strike);
    kernel2[1].setArg(j++, optionType);
    kernel2[1].setArg(j++, out_buff_b[2]);
    kernel2[1].setArg(j++, requiredTolerance);
    kernel2[1].setArg(j++, requiredSamples);
    kernel2[1].setArg(j++, timeSteps);
    kernel2[1].setArg(j++, maxSamples);
#endif
#if KN > 3
    j = 0;
    kernel3[0].setArg(j++, loop_nm);
    kernel3[0].setArg(j++, underlying);
    kernel3[0].setArg(j++, volatility);
    kernel3[0].setArg(j++, dividendYield);
    kernel3[0].setArg(j++, riskFreeRate);
    kernel3[0].setArg(j++, timeLength);
    kernel3[0].setArg(j++, strike);
    kernel3[0].setArg(j++, optionType);
    kernel3[0].setArg(j++, out_buff_a[3]);
    kernel3[0].setArg(j++, requiredTolerance);
    kernel3[0].setArg(j++, requiredSamples);
    kernel3[0].setArg(j++, timeSteps);
    kernel3[0].setArg(j++, maxSamples);
    j = 0;
    kernel3[1].setArg(j++, loop_nm);
    kernel3[1].setArg(j++, underlying);
    kernel3[1].setArg(j++, volatility);
    kernel3[1].setArg(j++, dividendYield);
    kernel3[1].setArg(j++, riskFreeRate);
    kernel3[1].setArg(j++, timeLength);
    kernel3[1].setArg(j++, strike);
    kernel3[1].setArg(j++, optionType);
    kernel3[1].setArg(j++, out_buff_b[3]);
    kernel3[1].setArg(j++, requiredTolerance);
    kernel3[1].setArg(j++, requiredSamples);
    kernel3[1].setArg(j++, timeSteps);
    kernel3[1].setArg(j++, maxSamples);
#endif

    std::vector<cl::Memory> out_vec_a; //{out_buff[0]};
    std::vector<cl::Memory> out_vec_b; //{out_buff[0]};
    for (int i = 0; i < KN; ++i) {
        out_vec_a.push_back(out_buff_a[i]);
        out_vec_b.push_back(out_buff_b[i]);
    }
    q.finish();
    gettimeofday(&st_time, 0);
    for (int i = 0; i < num_rep / KN; ++i) {
        int use_a = i & 1;
        if (use_a) {
            if (i > 1) {
                q.enqueueTask(kernel0[0], &read_events[i - 2], &kernel_events[i][0]);
#if KN > 1
                q.enqueueTask(kernel1[0], &read_events[i - 2], &kernel_events[i][1]);
#endif
#if KN > 2
                q.enqueueTask(kernel2[0], &read_events[i - 2], &kernel_events[i][2]);
#endif
#if KN > 3
                q.enqueueTask(kernel3[0], &read_events[i - 2], &kernel_events[i][3]);
#endif
            } else {
                q.enqueueTask(kernel0[0], nullptr, &kernel_events[i][0]);
#if KN > 1
                q.enqueueTask(kernel1[0], nullptr, &kernel_events[i][1]);
#endif
#if KN > 2
                q.enqueueTask(kernel2[0], nullptr, &kernel_events[i][2]);
#endif
#if KN > 3
                q.enqueueTask(kernel3[0], nullptr, &kernel_events[i][3]);
#endif
            }
        } else {
            if (i > 1) {
                q.enqueueTask(kernel0[1], &read_events[i - 2], &kernel_events[i][0]);
#if KN > 1
                q.enqueueTask(kernel1[1], &read_events[i - 2], &kernel_events[i][1]);
#endif
#if KN > 2
                q.enqueueTask(kernel2[1], &read_events[i - 2], &kernel_events[i][2]);
#endif
#if KN > 3
                q.enqueueTask(kernel3[1], &read_events[i - 2], &kernel_events[i][3]);
#endif
            } else {
                q.enqueueTask(kernel0[1], nullptr, &kernel_events[i][0]);
#if KN > 1
                q.enqueueTask(kernel1[1], nullptr, &kernel_events[i][1]);
#endif
#if KN > 2
                q.enqueueTask(kernel2[1], nullptr, &kernel_events[i][2]);
#endif
#if KN > 3
                q.enqueueTask(kernel3[1], nullptr, &kernel_events[i][3]);
#endif
            }
        }
        if (use_a) {
            q.enqueueMigrateMemObjects(out_vec_a, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
        } else {
            q.enqueueMigrateMemObjects(out_vec_b, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_events[i], &read_events[i][0]);
        }
    }

    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);
    int exec_time = tvdiff(&st_time, &end_time);
    double time_elapsed = double(exec_time) / 1000 / 1000;
    std::cout << "FPGA execution time: " << time_elapsed << " s\n"
              << "options number: " << loop_nm * num_rep << " \n"
              << "opt/sec: " << double(loop_nm * num_rep) / time_elapsed << std::endl;
    DtUsed golden = 3.834522;
    std::cout << "Expected value: " << golden << ::std::endl;
    if (num_rep > 1) {
        print_result(out0_a, out1_a, out2_a, out3_a);
    }
    print_result(out0_b, out1_b, out2_b, out3_b);

#endif

    return 0;
}
