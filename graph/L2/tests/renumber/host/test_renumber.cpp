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

#include "kernel_renumber.hpp"
#include "utils.hpp"
#include <map>
#include <omp.h>
#include <string>
#include "xcl2.hpp"
#include "xf_utils_sw/logger.hpp"

using namespace std;

int renumberClustersContiguously(int* C, int size) {
    printf("Within renumberClustersContiguously()\n");

    struct timeval start_time, end_time;
    gettimeofday(&start_time, 0);
    // Count the number of unique communities and internal edges
    map<int, int> clusterLocalMap; // Map each neighbor's cluster to a local number
    map<int, int>::iterator storedAlready;
    int numUniqueClusters = 0;

    for (int i = 0; i < size; i++) {
        assert(C[i] < size);
        if (C[i] >= 0) {                                  // Only if it is a valid number
            storedAlready = clusterLocalMap.find(C[i]);   // Check if it already exists
            if (storedAlready != clusterLocalMap.end()) { // Already exists
                C[i] = storedAlready->second;             // Renumber the cluster id
            } else {
                clusterLocalMap[C[i]] = numUniqueClusters; // Does not exist, add to the map
                C[i] = numUniqueClusters;                  // Renumber the cluster id
                numUniqueClusters++;                       // Increment the number
            }
        } // End of if()
    }     // End of for (i)
    gettimeofday(&end_time, 0);
    double ts = tvdiff(&start_time, &end_time) / 1000.0;
    printf("INFO: renumberClustersContiguously time %.4f ms.\n", ts);
    return numUniqueClusters; // Return the number of unique cluster ids
}

int main(int argc, const char* argv[]) {
    std::cout << "\n-----------------Renumber----------------\n";

    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;

    // cmd parser
    ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR:xclbin path is not set!\n";
        return 1;
    }

    std::string infile;
    if (!parser.getCmdOption("-i", infile)) {
        std::cout << "ERROR: input file path is not set!\n";
        return 1;
    }

    // read data
    FILE* pFile;
    pFile = fopen(infile.c_str(), "r");
    int numVertices;
    fscanf(pFile, "%d", &numVertices);
    printf("INFO: numVertices=%d \n", numVertices);
    int* C = (int*)malloc(sizeof(int) * numVertices);
    for (int j = 0; j < numVertices; j++) {
        int tmp;
        fscanf(pFile, "%d", &tmp);
        C[j] = tmp;
    }
    fclose(pFile);

    // golden
    int* CRenumber = (int*)malloc(sizeof(int) * numVertices);
    for (int i = 0; i < numVertices; i++) CRenumber[i] = C[i];

    int numClusters = renumberClustersContiguously(CRenumber, numVertices);

    int32_t configs[2];
    configs[0] = numVertices;
    configs[1] = 0;

    ap_int<DWIDTHS>* oldCids = aligned_alloc<ap_int<DWIDTHS> >(numVertices + 1);
    ap_int<DWIDTHS>* mapCid0 = aligned_alloc<ap_int<DWIDTHS> >(numVertices + 1);
    ap_int<DWIDTHS>* mapCid1 = aligned_alloc<ap_int<DWIDTHS> >(numVertices + 1);
    ap_int<DWIDTHS>* newCids = aligned_alloc<ap_int<DWIDTHS> >(numVertices + 1);

    for (int i = 0; i < numVertices; i++) {
        oldCids[i] = C[i];
    }

    // do pre-process on CPU
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
    cl::Kernel renumber;
    renumber = cl::Kernel(program, "kernel_renumber", &fail);
    logger.logCreateKernel(fail);
    std::cout << "kernel has been created" << std::endl;

    std::vector<cl_mem_ext_ptr_t> mext_o(5);
    mext_o[0] = {(unsigned int)(8) | XCL_MEM_TOPOLOGY, configs, 0};
    mext_o[1] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, oldCids, 0};
    mext_o[2] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, mapCid0, 0};
    mext_o[3] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, mapCid1, 0};
    mext_o[4] = {(unsigned int)(6) | XCL_MEM_TOPOLOGY, newCids, 0};

    // create device buffer and map dev buf to host buf
    cl::Buffer configs_buf, oldCids_buf, mapCid0_buf, mapCid1_buf, newCids_buf;

    configs_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             sizeof(int32_t) * 2, &mext_o[0]);
    oldCids_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
                             sizeof(ap_int<DWIDTHS>) * (numVertices + 1), &mext_o[1]);
    mapCid0_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             sizeof(ap_int<DWIDTHS>) * (numVertices + 1), &mext_o[2]);
    mapCid1_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             sizeof(ap_int<DWIDTHS>) * (numVertices + 1), &mext_o[3]);
    newCids_buf = cl::Buffer(context, CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
                             sizeof(ap_int<DWIDTHS>) * (numVertices + 1), &mext_o[4]);

    std::vector<cl::Memory> init;
    init.push_back(configs_buf);
    init.push_back(oldCids_buf);
    init.push_back(mapCid0_buf);
    init.push_back(mapCid1_buf);
    init.push_back(newCids_buf);

    q.enqueueMigrateMemObjects(init, CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, nullptr, nullptr);
    q.finish();

    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    ob_in.push_back(configs_buf);
    ob_in.push_back(oldCids_buf);
    q.enqueueMigrateMemObjects(ob_in, 0, nullptr, &events_write[0]);

    ob_out.push_back(configs_buf);
    ob_out.push_back(newCids_buf);

    // launch kernel and calculate kernel execution time
    std::cout << "INFO: kernel start------" << std::endl;
    gettimeofday(&start_time, 0);
    int j = 0;
    renumber.setArg(j++, configs_buf);
    renumber.setArg(j++, oldCids_buf);
    renumber.setArg(j++, mapCid0_buf);
    renumber.setArg(j++, mapCid1_buf);
    renumber.setArg(j++, newCids_buf);

    q.enqueueTask(renumber, &events_write, &events_kernel[0]);
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

    if (numClusters != configs[1])
        std::cout << "\033[0;33mWARNING: Number of usters is not equal! \033[0m" << std::endl;

    FILE* outFile;
    std::string outfile(infile);
    std::size_t found = outfile.find_last_of(".");
    outfile.insert(found, "-red");
    outFile = fopen(outfile.c_str(), "w");
    fprintf(outFile, "%d\n", numVertices);
    int err = 0;

    for (int i = 0; i < numVertices; i++) {
        int cid = newCids[i];
        fprintf(outFile, "%d\n", cid);
        if (cid != CRenumber[i]) {
            err++;
        }
    }

    fclose(outFile);

    if (err) {
        std::cerr << "INFO: Number of unique \033[31merrors " << err << "\033[0m" << std::endl;
        logger.error(xf::common::utils_sw::Logger::Message::TEST_FAIL);
    } else {
        std::cout << "INFO: Number of unique \033[32mclusters " << configs[1] << "\033[0m" << std::endl;
        logger.info(xf::common::utils_sw::Logger::Message::TEST_PASS);
    }

    free(C);
    free(CRenumber);
    free(oldCids);
    free(mapCid0);
    free(mapCid1);
    free(newCids);

    return err;
}
