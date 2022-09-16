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

#ifndef HOST_TOKINIT_HISTOGRAM_CPP
#define HOST_TOKINIT_HISTOGRAM_CPP

#include <iostream>
#include <sys/time.h>
#include "ap_int.h"

#ifndef HLS_TEST
#include "xf_utils_sw/logger.hpp"
#include "xcl2.hpp"
const int PIXEL_W = 2048;
const int PIXEL_H = 2048;
const int FRAME_DIM = 3;
const int ALL_PIXEL = PIXEL_W * PIXEL_H * FRAME_DIM;
const int MAX_NUM_BLK88_W = PIXEL_W / 8;
const int MAX_NUM_BLK88_H = PIXEL_H / 8;
const int MAX_NUM_BLK88 = MAX_NUM_BLK88_W * MAX_NUM_BLK88_H;
const int MAX_ORDERS_SIZE = (3 * 64 + 3 * 64 + 3 * 256 + 3 * 1024);
const int MAX_QF_THRESH_SIZE = 256;
const int MAX_CTX_MAP_SIZE = 256;
const int MAX_AC_TOKEN_SIZE = ALL_PIXEL;
#else
#include "hls_init_histogram.hpp"
#endif

#define MAX_NUM_CONFIG 32

unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) throw std::bad_alloc();
    return reinterpret_cast<T*>(ptr);
}

void hls_ANSinitHistogram_wrapper(std::string xclbinPath,
                                  int config[32],
                                  //====================
                                  int32_t* ac_coeff_ordered_ddr,
                                  int32_t* strategy_ddr,
                                  int32_t* qf_ddr,
                                  uint8_t* qdc_ddr,
                                  uint8_t* ctx_map,
                                  uint32_t* qf_thresholds,
                                  uint64_t* ac_tokens_ddr,
                                  //====================
                                  uint64_t* tokens0_ptr,
                                  uint64_t* tokens1_ptr,
                                  uint64_t* tokens2_ptr,
                                  uint64_t* tokens3_ptr,
                                  //====================
                                  int32_t* histograms0_ptr,
                                  uint32_t* histograms_size0_ptr,
                                  uint32_t* total_count0_ptr,
                                  uint32_t* nonempty0_ptr,
                                  //======================
                                  int32_t* histograms1_ptr,
                                  uint32_t* histograms_size1_ptr,
                                  uint32_t* total_count1_ptr,
                                  uint32_t* nonempty1_ptr,
                                  //======================
                                  int32_t* histograms2_ptr,
                                  uint32_t* histograms_size2_ptr,
                                  uint32_t* total_count2_ptr,
                                  uint32_t* nonempty2_ptr,
                                  //======================
                                  int32_t* histograms3_ptr,
                                  uint32_t* histograms_size3_ptr,
                                  uint32_t* total_count3_ptr,
                                  uint32_t* nonempty3_ptr,
                                  //======================
                                  int32_t* histograms4_ptr,
                                  uint32_t* histograms_size4_ptr,
                                  uint32_t* total_count4_ptr,
                                  uint32_t* nonempty4_ptr) {
#ifndef HLS_TEST

    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;

    struct timeval start_time; // End to end time clock start
    gettimeofday(&start_time, 0);

    // platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device, NULL, NULL, NULL, &fail);
    logger.logCreateContext(fail);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
    logger.logCreateCommandQueue(fail);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("INFO: Found Device=%s\n", devName.c_str());
    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbinPath);

    devices.resize(1);
    cl::Program program(context, devices, xclBins, NULL, &fail);
    logger.logCreateProgram(fail);

    int repInt = 1;
    // create kernels
    // std::vector<cl::Kernel> cluster_kernel(repInt);
    std::vector<cl::Kernel> initHist_kernel(repInt);
    for (int i = 0; i < repInt; i++) {
        initHist_kernel[i] = cl::Kernel(program, "JxlEnc_ans_initHistogram", &fail);
        logger.logCreateKernel(fail);
    }
    std::cout << "INFO: kernel has been created" << std::endl;

    // declare map of host Buffers
    std::cout << "kernel config size:" << 26 << std::endl;
    std::cout << "group_dim: " << config[4] << std::endl;
    std::cout << "do_once: " << config[12] << "," << config[13] << "," << config[14] << "," << config[15] << ","
              << config[16] << std::endl;

    // add code for hls_ANSinitTop
    // 1. create all I/O Buffer
    uint32_t* hb_config = aligned_alloc<uint32_t>(MAX_NUM_CONFIG);

    int32_t* hb_ac_coeff_ordered_ddr = aligned_alloc<int32_t>(ALL_PIXEL);
    int32_t* hb_strategy_ddr = aligned_alloc<int32_t>(MAX_NUM_BLK88);
    int32_t* hb_qf_ddr = aligned_alloc<int32_t>(MAX_NUM_BLK88);
    uint8_t* hb_qdc_ddr = aligned_alloc<uint8_t>(MAX_NUM_BLK88);
    uint8_t* hb_ctx_map = aligned_alloc<uint8_t>(MAX_CTX_MAP_SIZE);
    uint32_t* hb_qf_thresholds = aligned_alloc<uint32_t>(MAX_QF_THRESH_SIZE);
    uint64_t* hb_ac_tokens_ddr = aligned_alloc<uint64_t>(MAX_AC_TOKEN_SIZE);

    ap_uint<64>* hb_token0_ptr = aligned_alloc<ap_uint<64> >(MAX_AC_TOKEN_SIZE);
    ap_uint<64>* hb_token1_ptr = aligned_alloc<ap_uint<64> >(MAX_AC_TOKEN_SIZE);
    ap_uint<64>* hb_token2_ptr = aligned_alloc<ap_uint<64> >(MAX_AC_TOKEN_SIZE);
    ap_uint<64>* hb_token3_ptr = aligned_alloc<ap_uint<64> >(MAX_AC_TOKEN_SIZE);

    int32_t* hb_histograms0_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms1_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms2_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms3_ptr = aligned_alloc<int32_t>(163840);
    int32_t* hb_histograms4_ptr = aligned_alloc<int32_t>(163840);

    uint32_t* hb_histograms_size0_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histograms_size1_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histograms_size2_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histograms_size3_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_histograms_size4_ptr = aligned_alloc<uint32_t>(4096);

    uint32_t* hb_total_count0_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_total_count1_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_total_count2_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_total_count3_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_total_count4_ptr = aligned_alloc<uint32_t>(4096);

    uint32_t* hb_nonempty0_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty1_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty2_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty3_ptr = aligned_alloc<uint32_t>(4096);
    uint32_t* hb_nonempty4_ptr = aligned_alloc<uint32_t>(4096);

    //==================================================
    // 2. init all the host Buffers
    //==================================================
    for (int j = 0; j < MAX_NUM_CONFIG; j++) {
        hb_config[j] = config[j];
    }

    for (int j = 0; j < ALL_PIXEL; j++) {
        hb_ac_coeff_ordered_ddr[j] = ac_coeff_ordered_ddr[j];
    }

    for (int j = 0; j < MAX_NUM_BLK88; j++) {
        hb_strategy_ddr[j] = strategy_ddr[j];
    }

    for (int j = 0; j < MAX_NUM_BLK88; j++) {
        hb_qdc_ddr[j] = qdc_ddr[j];
    }

    for (int j = 0; j < MAX_NUM_BLK88; j++) {
        hb_qf_ddr[j] = qf_ddr[j];
    }

    for (int j = 0; j < MAX_CTX_MAP_SIZE; j++) {
        hb_ctx_map[j] = ctx_map[j];
    }

    for (int j = 0; j < MAX_QF_THRESH_SIZE; j++) {
        hb_qf_thresholds[j] = qf_thresholds[j];
    }

    for (int j = 0; j < MAX_AC_TOKEN_SIZE; j++) {
        hb_ac_tokens_ddr[j] = ac_tokens_ddr[j];
    }

    for (int j = 0; j < MAX_AC_TOKEN_SIZE; j++) {
        hb_token0_ptr[j] = (ap_uint<64>)tokens0_ptr[j];
        hb_token1_ptr[j] = (ap_uint<64>)tokens1_ptr[j];
        hb_token2_ptr[j] = (ap_uint<64>)tokens2_ptr[j];
        hb_token3_ptr[j] = (ap_uint<64>)tokens3_ptr[j];
    }

    for (int j = 0; j < 163840; j++) {
        hb_histograms0_ptr[j] = 0;
        hb_histograms1_ptr[j] = 0;
        hb_histograms2_ptr[j] = 0;
        hb_histograms3_ptr[j] = 0;
        hb_histograms4_ptr[j] = 0;
    }

    for (int j = 0; j < 4096; j++) {
        hb_histograms_size0_ptr[j] = 0;
        hb_histograms_size1_ptr[j] = 0;
        hb_histograms_size2_ptr[j] = 0;
        hb_histograms_size3_ptr[j] = 0;
        hb_histograms_size4_ptr[j] = 0;
    }

    for (int j = 0; j < 4096; j++) {
        hb_total_count0_ptr[j] = 0;
        hb_total_count1_ptr[j] = 0;
        hb_total_count2_ptr[j] = 0;
        hb_total_count3_ptr[j] = 0;
        hb_total_count4_ptr[j] = 0;
    }

    for (int j = 0; j < 4096; j++) {
        hb_nonempty0_ptr[j] = 0;
        hb_nonempty1_ptr[j] = 0;
        hb_nonempty2_ptr[j] = 0;
        hb_nonempty3_ptr[j] = 0;
        hb_nonempty4_ptr[j] = 0;
    }

    // mapping to HBM banks
    std::vector<cl_mem_ext_ptr_t> mext_o(33);
    mext_o[0] = {(((unsigned int)(7)) | XCL_MEM_TOPOLOGY), hb_config, 0};

    mext_o[1] = {(((unsigned int)(2)) | XCL_MEM_TOPOLOGY), hb_ac_coeff_ordered_ddr, 0};
    mext_o[2] = {(((unsigned int)(3)) | XCL_MEM_TOPOLOGY), hb_strategy_ddr, 0};
    mext_o[3] = {(((unsigned int)(4)) | XCL_MEM_TOPOLOGY), hb_qf_ddr, 0};
    mext_o[4] = {(((unsigned int)(5)) | XCL_MEM_TOPOLOGY), hb_qdc_ddr, 0};
    mext_o[5] = {(((unsigned int)(6)) | XCL_MEM_TOPOLOGY), hb_ctx_map, 0};
    mext_o[6] = {(((unsigned int)(6)) | XCL_MEM_TOPOLOGY), hb_qf_thresholds, 0};
    mext_o[7] = {(((unsigned int)(8)) | XCL_MEM_TOPOLOGY), hb_ac_tokens_ddr, 0};

    mext_o[8] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_token0_ptr, 0};
    mext_o[9] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_token1_ptr, 0};
    mext_o[10] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_token2_ptr, 0};
    mext_o[11] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_token3_ptr, 0};

    mext_o[12] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_nonempty0_ptr, 0};
    mext_o[13] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_nonempty1_ptr, 0};
    mext_o[14] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_nonempty2_ptr, 0};
    mext_o[15] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_nonempty3_ptr, 0};
    mext_o[16] = {(((unsigned int)(9)) | XCL_MEM_TOPOLOGY), hb_nonempty4_ptr, 0};

    mext_o[17] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_histograms0_ptr, 0};
    mext_o[18] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_histograms1_ptr, 0};
    mext_o[19] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_histograms2_ptr, 0};
    mext_o[20] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_histograms3_ptr, 0};
    mext_o[21] = {(((unsigned int)(10)) | XCL_MEM_TOPOLOGY), hb_histograms4_ptr, 0};

    mext_o[22] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_histograms_size0_ptr, 0};
    mext_o[23] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_histograms_size1_ptr, 0};
    mext_o[24] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_histograms_size2_ptr, 0};
    mext_o[25] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_histograms_size3_ptr, 0};
    mext_o[26] = {(((unsigned int)(11)) | XCL_MEM_TOPOLOGY), hb_histograms_size4_ptr, 0};

    mext_o[27] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_total_count0_ptr, 0};
    mext_o[28] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_total_count1_ptr, 0};
    mext_o[29] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_total_count2_ptr, 0};
    mext_o[30] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_total_count3_ptr, 0};
    mext_o[31] = {(((unsigned int)(12)) | XCL_MEM_TOPOLOGY), hb_total_count4_ptr, 0};

    //===================================================
    // 3. create device Buffer and map dev buf to host buf,
    //===================================================
    cl::Buffer db_config;

    cl::Buffer db_ac_coef_ordered_ddr;
    cl::Buffer db_strategy_ddr;
    cl::Buffer db_qf_ddr;
    cl::Buffer db_qdc_ddr;
    cl::Buffer db_ctx_map;
    cl::Buffer db_qf_thresholds;
    cl::Buffer db_ac_tokens_ddr;

    cl::Buffer db_token0_ptr;
    cl::Buffer db_token1_ptr;
    cl::Buffer db_token2_ptr;
    cl::Buffer db_token3_ptr;

    cl::Buffer db_histograms0_ptr;
    cl::Buffer db_histograms1_ptr;
    cl::Buffer db_histograms2_ptr;
    cl::Buffer db_histograms3_ptr;
    cl::Buffer db_histograms4_ptr;

    cl::Buffer db_histograms_size0_ptr;
    cl::Buffer db_histograms_size1_ptr;
    cl::Buffer db_histograms_size2_ptr;
    cl::Buffer db_histograms_size3_ptr;
    cl::Buffer db_histograms_size4_ptr;

    cl::Buffer db_total_count0_ptr;
    cl::Buffer db_total_count1_ptr;
    cl::Buffer db_total_count2_ptr;
    cl::Buffer db_total_count3_ptr;
    cl::Buffer db_total_count4_ptr;

    cl::Buffer db_nonempty0_ptr;
    cl::Buffer db_nonempty1_ptr;
    cl::Buffer db_nonempty2_ptr;
    cl::Buffer db_nonempty3_ptr;
    cl::Buffer db_nonempty4_ptr;

    // init cl Buffer
    db_config = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(int32_t) * MAX_NUM_CONFIG, &mext_o[0]);

    db_ac_coef_ordered_ddr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                        sizeof(int32_t) * ALL_PIXEL, &mext_o[1]);

    db_strategy_ddr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                 sizeof(int32_t) * MAX_NUM_BLK88, &mext_o[2]);

    db_qf_ddr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(int32_t) * MAX_NUM_BLK88, &mext_o[3]);

    db_qdc_ddr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(int32_t) * MAX_NUM_BLK88, &mext_o[4]);

    db_ctx_map = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                            sizeof(int32_t) * MAX_CTX_MAP_SIZE, &mext_o[5]);

    db_qf_thresholds = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(int32_t) * MAX_QF_THRESH_SIZE, &mext_o[6]);
    db_ac_tokens_ddr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(int32_t) * MAX_AC_TOKEN_SIZE, &mext_o[7]);
    //=================================
    db_token0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               sizeof(ap_uint<64>) * MAX_AC_TOKEN_SIZE, &mext_o[8]);

    db_token1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               sizeof(ap_uint<64>) * MAX_AC_TOKEN_SIZE, &mext_o[9]);

    db_token2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               sizeof(ap_uint<64>) * MAX_AC_TOKEN_SIZE, &mext_o[10]);

    db_token3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                               sizeof(ap_uint<64>) * MAX_AC_TOKEN_SIZE, &mext_o[11]);
    //===================================
    db_nonempty0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(uint32_t) * 4096, &mext_o[12]);
    db_nonempty1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(uint32_t) * 4096, &mext_o[13]);
    db_nonempty2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(uint32_t) * 4096, &mext_o[14]);
    db_nonempty3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(uint32_t) * 4096, &mext_o[15]);
    db_nonempty4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                  sizeof(uint32_t) * 4096, &mext_o[16]);
    //=================================
    db_histograms0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[17]);
    db_histograms1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[18]);
    db_histograms2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[19]);
    db_histograms3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[20]);
    db_histograms4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                    sizeof(int32_t) * 163840, &mext_o[21]);
    //=================================
    db_histograms_size0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(uint32_t) * 4096, &mext_o[22]);

    db_histograms_size1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(uint32_t) * 4096, &mext_o[23]);

    db_histograms_size2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(uint32_t) * 4096, &mext_o[24]);

    db_histograms_size3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(uint32_t) * 4096, &mext_o[25]);

    db_histograms_size4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                         sizeof(uint32_t) * 4096, &mext_o[26]);

    //==================================
    db_total_count0_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(uint32_t) * 4096, &mext_o[27]);

    db_total_count1_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(uint32_t) * 4096, &mext_o[28]);

    db_total_count2_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(uint32_t) * 4096, &mext_o[29]);

    db_total_count3_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(uint32_t) * 4096, &mext_o[30]);

    db_total_count4_ptr = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                                     sizeof(uint32_t) * 4096, &mext_o[31]);

    //==================================
    // add Buffers to migrate
    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;

    ob_in.push_back(db_config);
    ob_in.push_back(db_ac_coef_ordered_ddr);
    ob_in.push_back(db_strategy_ddr);
    ob_in.push_back(db_qf_ddr);
    ob_in.push_back(db_qdc_ddr);
    ob_in.push_back(db_ctx_map);
    ob_in.push_back(db_qf_thresholds);
    ob_in.push_back(db_token0_ptr);
    ob_in.push_back(db_token1_ptr);
    ob_in.push_back(db_token2_ptr);
    ob_in.push_back(db_token3_ptr);

    ob_out.push_back(db_config);
    ob_out.push_back(db_ac_tokens_ddr);
    ob_out.push_back(db_histograms0_ptr);
    ob_out.push_back(db_histograms1_ptr);
    ob_out.push_back(db_histograms2_ptr);
    ob_out.push_back(db_histograms3_ptr);
    ob_out.push_back(db_histograms4_ptr);
    ob_out.push_back(db_histograms_size0_ptr);
    ob_out.push_back(db_histograms_size1_ptr);
    ob_out.push_back(db_histograms_size2_ptr);
    ob_out.push_back(db_histograms_size3_ptr);
    ob_out.push_back(db_histograms_size4_ptr);
    ob_out.push_back(db_total_count0_ptr);
    ob_out.push_back(db_total_count1_ptr);
    ob_out.push_back(db_total_count2_ptr);
    ob_out.push_back(db_total_count3_ptr);
    ob_out.push_back(db_total_count4_ptr);
    ob_out.push_back(db_nonempty0_ptr);
    ob_out.push_back(db_nonempty1_ptr);
    ob_out.push_back(db_nonempty2_ptr);
    ob_out.push_back(db_nonempty3_ptr);
    ob_out.push_back(db_nonempty4_ptr);

    // set kernel args
    for (int i = 0; i < repInt; i++) {
        initHist_kernel[i].setArg(0, db_config);
        initHist_kernel[i].setArg(1, db_ac_coef_ordered_ddr);
        initHist_kernel[i].setArg(2, db_strategy_ddr);
        initHist_kernel[i].setArg(3, db_qf_ddr);
        initHist_kernel[i].setArg(4, db_qdc_ddr);
        initHist_kernel[i].setArg(5, db_ctx_map);
        initHist_kernel[i].setArg(6, db_qf_thresholds);
        initHist_kernel[i].setArg(7, db_ac_tokens_ddr);
        initHist_kernel[i].setArg(8, db_token0_ptr);
        initHist_kernel[i].setArg(9, db_token1_ptr);
        initHist_kernel[i].setArg(10, db_token2_ptr);
        initHist_kernel[i].setArg(11, db_token3_ptr);
        initHist_kernel[i].setArg(12, db_histograms0_ptr);
        initHist_kernel[i].setArg(13, db_histograms_size0_ptr);
        initHist_kernel[i].setArg(14, db_total_count0_ptr);
        initHist_kernel[i].setArg(15, db_nonempty0_ptr);
        initHist_kernel[i].setArg(16, db_histograms1_ptr);
        initHist_kernel[i].setArg(17, db_histograms_size1_ptr);
        initHist_kernel[i].setArg(18, db_total_count1_ptr);
        initHist_kernel[i].setArg(19, db_nonempty1_ptr);
        initHist_kernel[i].setArg(20, db_histograms2_ptr);
        initHist_kernel[i].setArg(21, db_histograms_size2_ptr);
        initHist_kernel[i].setArg(22, db_total_count2_ptr);
        initHist_kernel[i].setArg(23, db_nonempty2_ptr);
        initHist_kernel[i].setArg(24, db_histograms3_ptr);
        initHist_kernel[i].setArg(25, db_histograms_size3_ptr);
        initHist_kernel[i].setArg(26, db_total_count3_ptr);
        initHist_kernel[i].setArg(27, db_nonempty3_ptr);
        initHist_kernel[i].setArg(28, db_histograms4_ptr);
        initHist_kernel[i].setArg(29, db_histograms_size4_ptr);
        initHist_kernel[i].setArg(30, db_total_count4_ptr);
        initHist_kernel[i].setArg(31, db_nonempty4_ptr);
    }

    // launch kernel and calculate kernel execution time
    std::cout << "INFO: Kernel Start" << std::endl;
    // declare events
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    // migrate,
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);
    q.enqueueTask(initHist_kernel[0], &events_write, &events_kernel[0]);
    q.enqueueMigrateMemObjects(ob_out, 1, &events_kernel, &events_read[0]);
    q.finish();

    struct timeval end_time;
    gettimeofday(&end_time, 0);
    std::cout << "INFO: Finish kernel execution" << std::endl;
    std::cout << "INFO: Finish E2E execution" << std::endl;

    // print related times
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

    for (int j = 0; j < MAX_NUM_CONFIG; j++) {
        config[j] = hb_config[j];
    }

    for (int j = 0; j < ALL_PIXEL; j++) {
        ac_tokens_ddr[j] = hb_ac_tokens_ddr[j];
    }

    // output
    std::cout << "histograms_ptr:" << std::endl;
    for (int j = 0; j < 163840; j++) {
        histograms0_ptr[j] = hb_histograms0_ptr[j];
        histograms1_ptr[j] = hb_histograms1_ptr[j];
        histograms2_ptr[j] = hb_histograms2_ptr[j];
        histograms3_ptr[j] = hb_histograms3_ptr[j];
        histograms4_ptr[j] = hb_histograms4_ptr[j];
    }

    std::cout << "histograms_size:" << std::endl;
    for (int j = 0; j < 4096; j++) {
        histograms_size0_ptr[j] = hb_histograms_size0_ptr[j];
        histograms_size1_ptr[j] = hb_histograms_size1_ptr[j];
        histograms_size2_ptr[j] = hb_histograms_size2_ptr[j];
        histograms_size3_ptr[j] = hb_histograms_size3_ptr[j];
        histograms_size4_ptr[j] = hb_histograms_size4_ptr[j];
    }

    std::cout << "total_count_ptr:" << std::endl;
    for (int j = 0; j < 4096; j++) {
        total_count0_ptr[j] = hb_total_count0_ptr[j];
        total_count1_ptr[j] = hb_total_count1_ptr[j];
        total_count2_ptr[j] = hb_total_count2_ptr[j];
        total_count3_ptr[j] = hb_total_count3_ptr[j];
        total_count4_ptr[j] = hb_total_count4_ptr[j];
    }

    for (int j = 0; j < 4096; j++) {
        nonempty0_ptr[j] = hb_nonempty0_ptr[j];
        nonempty1_ptr[j] = hb_nonempty1_ptr[j];
        nonempty2_ptr[j] = hb_nonempty2_ptr[j];
        nonempty3_ptr[j] = hb_nonempty3_ptr[j];
        nonempty4_ptr[j] = hb_nonempty4_ptr[j];
    }

    free(hb_config);
    free(hb_ac_coeff_ordered_ddr);
    free(hb_strategy_ddr);
    free(hb_qf_ddr);
    free(hb_qdc_ddr);
    free(hb_ctx_map);
    free(hb_qf_thresholds);
    free(hb_ac_tokens_ddr);
    free(hb_token0_ptr);
    free(hb_token1_ptr);
    free(hb_token2_ptr);
    free(hb_token3_ptr);
    free(hb_histograms0_ptr);
    free(hb_histograms1_ptr);
    free(hb_histograms2_ptr);
    free(hb_histograms3_ptr);
    free(hb_histograms4_ptr);
    free(hb_histograms_size0_ptr);
    free(hb_histograms_size1_ptr);
    free(hb_histograms_size2_ptr);
    free(hb_histograms_size3_ptr);
    free(hb_histograms_size4_ptr);
    free(hb_total_count0_ptr);
    free(hb_total_count1_ptr);
    free(hb_total_count2_ptr);
    free(hb_total_count3_ptr);
    free(hb_total_count4_ptr);
    free(hb_nonempty0_ptr);
    free(hb_nonempty1_ptr);
    free(hb_nonempty2_ptr);
    free(hb_nonempty3_ptr);
    free(hb_nonempty4_ptr);

    std::cout << "finished opencl host" << std::endl;
#else
    ap_uint<64>* hls_tokens0_ptr = reinterpret_cast<ap_uint<64>*>(tokens0_ptr);
    ap_uint<64>* hls_tokens1_ptr = reinterpret_cast<ap_uint<64>*>(tokens1_ptr);
    ap_uint<64>* hls_tokens2_ptr = reinterpret_cast<ap_uint<64>*>(tokens2_ptr);
    ap_uint<64>* hls_tokens3_ptr = reinterpret_cast<ap_uint<64>*>(tokens3_ptr);

    hls_ANSinitHistogram(config, ac_coeff_ordered_ddr, strategy_ddr, qf_ddr, qdc_ddr, ctx_map, qf_thresholds,
                         ac_tokens_ddr, hls_tokens0_ptr, hls_tokens1_ptr, hls_tokens2_ptr, hls_tokens3_ptr,
                         histograms0_ptr, histograms_size0_ptr, total_count0_ptr, nonempty0_ptr, histograms1_ptr,
                         histograms_size1_ptr, total_count1_ptr, nonempty1_ptr, histograms2_ptr, histograms_size2_ptr,
                         total_count2_ptr, nonempty2_ptr, histograms3_ptr, histograms_size3_ptr, total_count3_ptr,
                         nonempty3_ptr, histograms4_ptr, histograms_size4_ptr, total_count4_ptr, nonempty4_ptr);
#endif
}

#endif
