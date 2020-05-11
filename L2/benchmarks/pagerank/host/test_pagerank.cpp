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

#ifndef _HLS_TEST_
#ifndef _GENDATA_
#include "xcl2.hpp"
#endif
#endif
#ifndef __SYNTHESIS__
#include <algorithm>
#include <iostream>
#include <limits>
#include <string.h>
#include <sys/time.h>
#include "graph.hpp"
#endif

#include "xf_graph_L2.hpp"
#ifdef _HLS_TEST_
#ifndef _GENDATA_
#include "kernel_pagerank.hpp"
#endif
#endif

#define DT double
typedef ap_uint<512> buffType;

#ifndef __SYNTHESIS__

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) {
        throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(ptr);
}

// Compute time difference
unsigned long diff(const struct timeval* newTime, const struct timeval* oldTime) {
    return (newTime->tv_sec - oldTime->tv_sec) * 1000000 + (newTime->tv_usec - oldTime->tv_usec);
}

// Arguments parser
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
    // Initialize parserl
    ArgParser parser(argc, argv);

    // Initialize paths addresses
    std::string xclbin_path;
    std::string num_str;

    int num_runs;
    int nrows;
    int nnz;

    // Read In paths addresses
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "INFO: input path is not set!\n";
    }
    if (!parser.getCmdOption("-runs", num_str)) {
        num_runs = 1;
        std::cout << "INFO: number runs is not set!\n";
    } else {
        num_runs = std::stoi(num_str);
    }
    if (!parser.getCmdOption("-nnz", num_str)) {
        nnz = 7;
        std::cout << "INFO: number of non-zero is not set!\n";
    } else {
        nnz = std::stoi(num_str);
    }
    if (!parser.getCmdOption("-nrows", num_str)) {
        nrows = 5;
        std::cout << "INFO: number of rows/column is not set!\n";
    } else {
        nrows = std::stoi(num_str);
    }
    std::string files;
    std::string filename, tmp, filename2_1, filename2_2;
    std::string dataSetDir;
    std::string refDir;
    if (!parser.getCmdOption("-files", num_str)) {
        files = "";
        std::cout << "INFO: dataSet name is not set!\n";
    } else {
        files = num_str;
    }
    if (!parser.getCmdOption("-dataSetDir", num_str)) {
        dataSetDir = "./data/";
        std::cout << "INFO: dataSet dir is not set!\n";
    } else {
        dataSetDir = num_str;
    }
    if (!parser.getCmdOption("-refDir", num_str)) {
        refDir = "./data/";
        std::cout << "INFO: reference dir is not set!\n";
    } else {
        refDir = num_str;
    }
    filename = dataSetDir + files + ".txt";
    filename2_1 = dataSetDir + files + "csr_offsets.txt";
    filename2_2 = dataSetDir + files + "csr_columns.txt";
    std::cout << "INFO: dataSet path is " << filename << std::endl;
    std::cout << "INFO: dataSet offset path is " << filename2_1 << std::endl;
    std::cout << "INFO: dataSet indice path is " << filename2_2 << std::endl;

    std::string fileRef;
    fileRef = refDir + "pagerank_ref.txt";
    std::cout << "INFO: reference data path is " << fileRef << std::endl;

    // Variables to measure time
    struct timeval tstart, tend;
    struct timeval tstart1, tend1;
    struct timeval tstart2, tend2;
    struct timeval tstart0, tend0;
    struct timeval tstartE2E, tendE2E;

    gettimeofday(&tstartE2E, 0);

    CscMatrix<int, DT> cscMat;
    readInWeightedDirectedGraphCV<int, DT>(filename2_2, cscMat, nnz);
    std::cout << "INFO: ReadIn succeed" << std::endl;
    readInWeightedDirectedGraphRow<int, DT>(filename2_1, cscMat, nnz, nrows);

    // Output the inputs information
    std::cout << "INFO: Number of kernel runs: " << num_runs << std::endl;
    std::cout << "INFO: Number of edges: " << nnz << std::endl;
    std::cout << "INFO: Number of nrows: " << nrows << std::endl;

    DT alpha = 0.85;
    DT tolerance = 1e-3f;
    int maxIter = 500;

    ///// declaration
    ap_uint<32>* offsetArr = aligned_alloc<ap_uint<32> >(nrows + 1);
    ap_uint<32>* indiceArr = aligned_alloc<ap_uint<32> >(nnz);
    for (int i = 0; i < nnz; ++i) {
        if (i < nrows + 1) {
            offsetArr[i] = cscMat.columnOffset.data()[i];
        }
        indiceArr[i] = cscMat.row.data()[i];
    }
    int iteration = (sizeof(DT) == 8) ? (nrows + 7) / 8 : (nrows + 16 - 1) / 16;
    int unrollNm2 = (sizeof(DT) == 4) ? 16 : 8;
    int iteration2 = (nrows + unrollNm2 - 1) / unrollNm2;
    buffType* cntValFull = aligned_alloc<buffType>(iteration2);
    buffType* buffPing = aligned_alloc<buffType>(iteration2);
    buffType* buffPong = aligned_alloc<buffType>(iteration2);
    DT* pagerank = aligned_alloc<DT>(nrows);
    ap_uint<64>* pagerank2 = aligned_alloc<ap_uint<64> >(nrows);
    ap_uint<32>* degreeCSR = aligned_alloc<ap_uint<32> >(nrows + 16);
    ap_uint<32>* orderUnroll = aligned_alloc<ap_uint<32> >(nrows + 16);

    DT* golden = new DT[nrows];

    for (int i = 0; i < nrows; ++i) {
        golden[i] = 0;
        degreeCSR[i] = 0;
        pagerank[i] = 0;
    }
    readInRef<int, DT>(fileRef, golden, nrows);

#ifdef _HLS_TEST_
    ap_uint<512>* pagerank1 = aligned_alloc<ap_uint<512> >(iteration2);
    ap_uint<512>* degree = reinterpret_cast<ap_uint<512>*>(degreeCSR);
    ap_uint<512>* offsetCSC = reinterpret_cast<ap_uint<512>*>(offsetArr);
    ap_uint<512>* indiceCSC = reinterpret_cast<ap_uint<512>*>(indiceArr);
    const int widthOR = (sizeof(DT) == 8) ? 256 : 512;
    ap_uint<widthOR>* orderUnroll2 = reinterpret_cast<ap_uint<widthOR>*>(orderUnroll);
    kernel_pagerank_0(nrows, nnz, alpha, tolerance, maxIter, pagerank1, degree, offsetCSC, indiceCSC, cntValFull,
                      buffPing, buffPong, orderUnroll2);
    int cnt = 0;
    int size0 = sizeof(DT) * 8;
    for (int i = 0; i < iteration2; ++i) {
        ap_ufixed<64, 32> tt;
        ap_uint<512> tmp11 = pagerank1[i];
        for (int k = 0; k < unrollNm2; ++k) {
            if (cnt < nrows) {
                tt.range(size0 - 1, 0) = tmp11.range(size0 * (k + 1) - 1, size0 * k);
                pagerank[cnt] = (DT)(tt.to_double());
                cnt++;
            }
        }
    }
    free(pagerank1);
#else
    // Platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    cl::Device device = devices[0];

    // Creating Context and Command Queue for selected Device
    cl::Context context(device);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE);
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
    printf("INFO: Found Device=%s\n", devName.c_str());

    cl::Program::Binaries xclBins = xcl::import_binary_file(xclbin_path);
    devices.resize(1);
    cl::Program program(context, devices, xclBins);
    cl::Kernel kernel_pagerank(program, "kernel_pagerank_0");
    std::cout << "INFO: Kernel has been created" << std::endl;

    // DDR Settings
    std::vector<cl_mem_ext_ptr_t> mext_in(8);
    mext_in[0].flags = XCL_MEM_DDR_BANK0;
    mext_in[0].obj = pagerank2;
    mext_in[0].param = 0;
    mext_in[1].flags = XCL_MEM_DDR_BANK0;
    mext_in[1].obj = indiceArr;
    mext_in[1].param = 0;
    mext_in[2].flags = XCL_MEM_DDR_BANK0;
    mext_in[2].obj = cntValFull;
    mext_in[2].param = 0;
    mext_in[3].flags = XCL_MEM_DDR_BANK0;
    mext_in[3].obj = buffPing;
    mext_in[3].param = 0;
    mext_in[4].flags = XCL_MEM_DDR_BANK0;
    mext_in[4].obj = buffPong;
    mext_in[4].param = 0;
    mext_in[5].flags = XCL_MEM_DDR_BANK0;
    mext_in[5].obj = degreeCSR;
    mext_in[5].param = 0;
    mext_in[6].flags = XCL_MEM_DDR_BANK0;
    mext_in[6].obj = offsetArr;
    mext_in[6].param = 0;
    mext_in[7].flags = XCL_MEM_DDR_BANK0;
    mext_in[7].obj = orderUnroll;
    mext_in[7].param = 0;

    // Create device buffer and map dev buf to host buf
    std::vector<cl::Buffer> buffer(8);

    buffer[0] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(DT) * (nrows), &mext_in[0]);
    buffer[1] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                           sizeof(ap_uint<32>) * nnz, &mext_in[1]);
    buffer[2] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                           sizeof(buffType) * iteration2, &mext_in[2]);
    buffer[3] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(buffType) * iteration2, &mext_in[3]);
    buffer[4] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(buffType) * iteration2, &mext_in[4]);
    buffer[5] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(ap_uint<32>) * nrows, &mext_in[5]);
    buffer[6] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                           sizeof(ap_uint<32>) * (nrows + 1), &mext_in[6]);
    buffer[7] = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                           sizeof(ap_uint<32>) * (nrows + 16), &mext_in[7]);

    // Data transfer from host buffer to device buffer

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    ob_in.push_back(buffer[1]);
    ob_in.push_back(buffer[2]);
    ob_in.push_back(buffer[3]);
    ob_in.push_back(buffer[4]);
    ob_in.push_back(buffer[5]);
    ob_in.push_back(buffer[6]);
    ob_in.push_back(buffer[7]);
    ob_out.push_back(buffer[0]);

    kernel_pagerank.setArg(0, nrows);
    kernel_pagerank.setArg(1, nnz);
    kernel_pagerank.setArg(2, alpha);
    kernel_pagerank.setArg(3, tolerance);
    kernel_pagerank.setArg(4, maxIter);
    kernel_pagerank.setArg(5, buffer[0]);
    kernel_pagerank.setArg(6, buffer[5]);
    kernel_pagerank.setArg(7, buffer[6]);
    kernel_pagerank.setArg(8, buffer[1]);
    kernel_pagerank.setArg(9, buffer[2]);
    kernel_pagerank.setArg(10, buffer[3]);
    kernel_pagerank.setArg(11, buffer[4]);
    kernel_pagerank.setArg(12, buffer[7]);

    // Setup kernel
    std::cout << "INFO: Finish kernel setup" << std::endl;
    std::vector<std::vector<cl::Event> > kernel_evt(1);
    std::vector<std::vector<cl::Event> > kernel_evt1(1);
    std::vector<std::vector<cl::Event> > kernel_evt2(1);
    std::vector<std::vector<cl::Event> > kernel_evt3(1);
    kernel_evt[0].resize(1);
    kernel_evt1[0].resize(1);
    kernel_evt2[0].resize(1);
    kernel_evt3[0].resize(1);

    for (int i = 0; i < num_runs; ++i) {
        q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &kernel_evt[i][0]); // 0 : migrate from host to dev
    }
    // Launch kernel and compute kernel execution time
    for (int i = 0; i < num_runs; ++i) {
        q.enqueueTask(kernel_pagerank, &kernel_evt[i], &kernel_evt1[i][0]);
    }

    // Data transfer from device buffer to host buffer
    for (int i = 0; i < num_runs; ++i) {
        q.enqueueMigrateMemObjects(ob_out, 1, &kernel_evt1[i], nullptr); // 1 : migrate from dev to host
    }
    q.finish();

    gettimeofday(&tendE2E, 0);
    unsigned long timeStart, timeEnd;
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "INFO: Finish kernel0 execution" << std::endl;
    kernel_evt1[0][0].getProfilingInfo(CL_PROFILING_COMMAND_START, &timeStart);
    kernel_evt1[0][0].getProfilingInfo(CL_PROFILING_COMMAND_END, &timeEnd);
    unsigned long exec_time0 = (timeEnd - timeStart) / 1000.0;
    std::cout << "INFO: Average execution per run: " << exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    std::cout << "INFO: Finish E2E execution" << std::endl;
    int exec_timeE2E = diff(&tendE2E, &tstartE2E);
    std::cout << "INFO: FPGA execution time of " << num_runs << " runs:" << exec_timeE2E << " us\n"
              << "INFO: Average execution per run: " << exec_timeE2E - exec_time0 * num_runs + exec_time0 << " us\n";
    std::cout << "-------------------------------------------------------" << std::endl;
    for (int k = 0; k < nrows; ++k) {
        ap_uint<64> tmp = pagerank2[k];
        ap_ufixed<64, 32> tmp1;
        tmp1.range(63, 0) = tmp.range(63, 0);
        pagerank[k] = tmp1.to_double();
    }
#endif
    DT sum2 = 0.0;
    for (int i = 0; i < nrows; ++i) {
        sum2 += golden[i];
    }
    for (int i = 0; i < nrows; ++i) {
        golden[i] /= sum2;
    }

    // Calculate err
    DT err = 0.0;
    int accurate = 0;
    for (int i = 0; i < nrows; ++i) {
        err += (golden[i] - pagerank[i]) * (golden[i] - pagerank[i]);
        if (std::abs(pagerank[i] - golden[i]) < tolerance) {
            accurate += 1;
        }
        std::cout << "pagerank i = " << i << "\t our = " << pagerank[i] << "\t golden = " << golden[i] << std::endl;
    }
    DT accRate = accurate * 1.0 / nrows;
    err = std::sqrt(err);
    std::cout << "INFO: Accurate Rate = " << accRate << std::endl;
    std::cout << "INFO: Err Geomean = " << err << std::endl;

    free(offsetArr);
    free(indiceArr);
    free(orderUnroll);
    free(cntValFull);
    free(buffPing);
    free(buffPong);
    free(pagerank);
    free(pagerank2);
    free(degreeCSR);
    delete[] golden;

    if (err < 1e-2) {
        std::cout << "INFO: Result is correct" << std::endl;
        return 0;
    } else {
        std::cout << "INFO: Result is wrong" << std::endl;
        return 1;
    }
}
#endif
