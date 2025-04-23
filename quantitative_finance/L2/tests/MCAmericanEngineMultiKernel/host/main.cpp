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
#include <sys/time.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include "MCAE_kernel.hpp"
#include "ap_int.h"
#include "utils.hpp"
#include "xf_utils_sw/logger.hpp"

#define XCL_BANK(n) (((unsigned int)(n)) | XCL_MEM_TOPOLOGY)

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
    std::cout << "\n----------------------MC(American) Engine-----------------\n";
    // cmd parser
    ArgParser parser(argc, argv);
    std::string xclbin_path;
#ifndef HLS_TEST
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }
#endif
    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    int err = 0;
    std::string mode = "hw";
    if (std::getenv("XCL_EMULATION_MODE") != nullptr) {
        mode = std::getenv("XCL_EMULATION_MODE");
    }
    std::cout << "[INFO]Running in " << mode << " mode" << std::endl;
    // AXI depth
    int data_size = depthP;            // 20480;//= depthP = 1024(calibrate
                                       // samples)*10(steps) *2(iter), width: 64*UN
    int matdata_size = depthM;         ////180;//=depthM = 9*10(steps)*2(iter), width: 64
    int coefdata_size = TIMESTEPS - 1; // 9;//=(steps-1), width: 4*64
    std::cout << "data_size is " << data_size << std::endl;

    ap_uint<64 * UN_K1>* output_price[2];
    ap_uint<64>* output_mat[2];  // = aligned_alloc<ap_uint<64> >(matdata_size);
    ap_uint<64 * COEF>* coef[2]; // = aligned_alloc<ap_uint<64 * COEF> >(coefdata_size);
    TEST_DT* output[2];          // = aligned_alloc<TEST_DT>(1);

    for (int i = 0; i < 2; ++i) {
        output_price[i] = aligned_alloc<ap_uint<64 * UN_K1> >(data_size); // 64*UN
        output_mat[i] = aligned_alloc<ap_uint<64> >(matdata_size);
        coef[i] = aligned_alloc<ap_uint<64 * COEF> >(coefdata_size);
        output[i] = aligned_alloc<TEST_DT>(1);
    }

    // -------------setup params---------------

    int timeSteps = 100;
    TEST_DT underlying = 36;
    TEST_DT strike = 40.0;
    int optionType = 1;
    TEST_DT volatility = 0.20;
    TEST_DT riskFreeRate = 0.06;
    TEST_DT dividendYield = 0.0;
    TEST_DT timeLength = 1;
    TEST_DT requiredTolerance = 0.02;

    unsigned int requiredSamples = 24576;
    int calibSamples = 4096;
    int maxsamples = 0;
    double golden_output = 3.978;
    std::string num_str;
    int loop_nm = 100;
    if (parser.getCmdOption("-cal", num_str)) {
        try {
            calibSamples = std::stoi(num_str);
        } catch (...) {
            calibSamples = 4096;
        }
    }
    if (parser.getCmdOption("-s", num_str)) {
        try {
            timeSteps = std::stoi(num_str);
        } catch (...) {
            timeSteps = 100;
        }
    }
    if (parser.getCmdOption("-p", num_str)) {
        try {
            requiredSamples = std::stoi(num_str);
        } catch (...) {
            requiredSamples = 24576;
        }
    }

    if (mode.compare("hw_emu") == 0) {
        timeSteps = UN_K2_STEP;
        golden_output = 4.18;
        loop_nm = 1;
    }
    std::cout << "loop_nm: " << loop_nm << std::endl;
    std::cout << "paths: " << requiredSamples << std::endl;
#ifndef HLS_TEST
    struct timeval start_time, end_time;
    cl_int cl_err;
    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device, NULL, NULL, NULL, &cl_err);
    logger.logCreateContext(cl_err);
#ifdef SW_EMU_TEST
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE, &cl_err);
#else
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &cl_err);
#endif
    logger.logCreateCommandQueue(cl_err);

    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("Found Device=%s\n", devName.c_str());

    // cl::Program::Binaries xclBins =
    // xcl::import_binary_file("../xclbin/MCAE_u250_hw.xclbin");
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins, NULL, &cl_err);
    logger.logCreateProgram(cl_err);
    cl::Kernel kernel_MCAE_k0[2];
    cl::Kernel kernel_MCAE_k2[2];
    cl::Kernel kernel_MCAE_k1[2];
    for (int i = 0; i < 2; ++i) {
        kernel_MCAE_k0[i] = cl::Kernel(program, "MCAE_k0", &cl_err);
        logger.logCreateKernel(cl_err);
        kernel_MCAE_k1[i] = cl::Kernel(program, "MCAE_k1", &cl_err);
        logger.logCreateKernel(cl_err);
        kernel_MCAE_k2[i] = cl::Kernel(program, "MCAE_k2", &cl_err);
        logger.logCreateKernel(cl_err);
    }

    std::cout << "kernel has been created" << std::endl;

    cl_mem_ext_ptr_t mext_o[2][4];
    for (int c = 0; c < 2; ++c) {
        mext_o[c][0] = {7, output_price[c], kernel_MCAE_k0[c]()};
        mext_o[c][1] = {8, output_mat[c], kernel_MCAE_k0[c]()};
        mext_o[c][2] = {6, coef[c], kernel_MCAE_k1[c]()};
        mext_o[c][3] = {8, output[c], kernel_MCAE_k2[c]()};
    }

    // create device buffer and map dev buf to host buf
    cl::Buffer output_price_buf[2];
    cl::Buffer output_mat_buf[2];
    cl::Buffer coef_buf[2];
    cl::Buffer output_buf[2];
    for (int i = 0; i < 2; ++i) {
        output_price_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(ap_uint<64 * UN_K1>) * data_size, &mext_o[i][0]);
        output_mat_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                       sizeof(ap_uint<64>) * matdata_size, &mext_o[i][1]);
        coef_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(ap_uint<64 * COEF>) * coefdata_size, &mext_o[i][2]);
        output_buf[i] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                   sizeof(TEST_DT), &mext_o[i][3]);
    }

    std::vector<cl::Memory> ob_out;
    ob_out.push_back(output_buf[0]);

    std::vector<cl::Memory> ob_out_b;
    ob_out_b.push_back(output_buf[1]);

    for (int c = 0; c < 2; ++c) {
        kernel_MCAE_k0[c].setArg(0, underlying);
        kernel_MCAE_k0[c].setArg(1, volatility);
        kernel_MCAE_k0[c].setArg(2, riskFreeRate);
        kernel_MCAE_k0[c].setArg(3, dividendYield);
        kernel_MCAE_k0[c].setArg(4, timeLength);
        kernel_MCAE_k0[c].setArg(5, strike);
        kernel_MCAE_k0[c].setArg(6, optionType);
        kernel_MCAE_k0[c].setArg(7, output_price_buf[c]);
        kernel_MCAE_k0[c].setArg(8, output_mat_buf[c]);
        kernel_MCAE_k0[c].setArg(9, calibSamples);
        kernel_MCAE_k0[c].setArg(10, timeSteps);

        kernel_MCAE_k1[c].setArg(0, timeLength);
        kernel_MCAE_k1[c].setArg(1, riskFreeRate);
        kernel_MCAE_k1[c].setArg(2, strike);
        kernel_MCAE_k1[c].setArg(3, optionType);
        kernel_MCAE_k1[c].setArg(4, output_price_buf[c]);
        kernel_MCAE_k1[c].setArg(5, output_mat_buf[c]);
        kernel_MCAE_k1[c].setArg(6, coef_buf[c]);
        kernel_MCAE_k1[c].setArg(7, calibSamples);
        kernel_MCAE_k1[c].setArg(8, timeSteps);

        kernel_MCAE_k2[c].setArg(0, underlying);
        kernel_MCAE_k2[c].setArg(1, volatility);
        kernel_MCAE_k2[c].setArg(2, dividendYield);
        kernel_MCAE_k2[c].setArg(3, riskFreeRate);
        kernel_MCAE_k2[c].setArg(4, timeLength);
        kernel_MCAE_k2[c].setArg(5, strike);
        kernel_MCAE_k2[c].setArg(6, optionType);
        kernel_MCAE_k2[c].setArg(7, coef_buf[c]);
        kernel_MCAE_k2[c].setArg(8, output_buf[c]);
        kernel_MCAE_k2[c].setArg(9, requiredTolerance);
        kernel_MCAE_k2[c].setArg(10, requiredSamples);
        kernel_MCAE_k2[c].setArg(11, timeSteps);
    }

    // number of call for kernel
    std::vector<std::vector<cl::Event> > evt0(loop_nm);
    std::vector<std::vector<cl::Event> > evt1(loop_nm);
    std::vector<std::vector<cl::Event> > evt2(loop_nm);
    std::vector<std::vector<cl::Event> > evt3(loop_nm);
    for (int i = 0; i < loop_nm; i++) {
        evt0[i].resize(1);
        evt1[i].resize(1);
        evt2[i].resize(1);
        evt3[i].resize(1);
    }

    std::cout << "kernel start------" << std::endl;

    q.finish();
    gettimeofday(&start_time, 0);
    for (int i = 0; i < loop_nm; ++i) {
        // launch kernel and calculate kernel execution time
        int use_a = i & 1;
        if (use_a) {
            if (i < 2) {
                q.enqueueTask(kernel_MCAE_k0[0], nullptr, &evt0[i][0]);
            } else {
                q.enqueueTask(kernel_MCAE_k0[0], &evt3[i - 2], &evt0[i][0]);
            }
            q.enqueueTask(kernel_MCAE_k1[0], &evt0[i], &evt1[i][0]);
            q.enqueueTask(kernel_MCAE_k2[0], &evt1[i], &evt2[i][0]);
            q.enqueueMigrateMemObjects(ob_out, 1, &evt2[i], &evt3[i][0]);
        } else {
            if (i < 2) {
                q.enqueueTask(kernel_MCAE_k0[1], nullptr, &evt0[i][0]);
            } else {
                q.enqueueTask(kernel_MCAE_k0[1], &evt3[i - 2], &evt0[i][0]);
            }
            q.enqueueTask(kernel_MCAE_k1[1], &evt0[i], &evt1[i][0]);
            q.enqueueTask(kernel_MCAE_k2[1], &evt1[i], &evt2[i][0]);
            q.enqueueMigrateMemObjects(ob_out_b, 1, &evt2[i], &evt3[i][0]);
        }
    }
    q.flush();
    q.finish();
    gettimeofday(&end_time, 0);
    std::cout << "kernel end------" << std::endl;

    TEST_DT out_price;
    if (loop_nm == 1) {
        out_price = output[1][0];
    } else {
        out_price = (output[0][0] + output[1][0]) / 2;
    }
    std::cout << "out_price = " << out_price << std::endl;

    int exec_time = tvdiff(&start_time, &end_time);

    double time_elapsed = double(exec_time) / 1000 / 1000;
    std::cout << "FPGA execution time: " << time_elapsed << " s\n"
              << "options number: " << loop_nm << " \n"
              << "opt/sec: " << double(loop_nm) / time_elapsed << std::endl;

    double diff = std::fabs(out_price - golden_output);
    if (diff > requiredTolerance) {
        std::cout << "diff = " << diff << ", requiredTolerance=" << requiredTolerance << std::endl;
        std::cout << "Output is wrong!" << std::endl;
        err += 1;
    }
#endif

    err ? logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL)
        : logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);
    return err;
}
