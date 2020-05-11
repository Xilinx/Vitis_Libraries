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

/**
 * @file cscmv.cpp
 * @brief main function for cscmv kernel host code
 *
 * This file is part of Vitis SPARSE Library.
 */
#include <cmath>
#include <cstdlib>
#include <string>
#include <iostream>
#include "L1_utils.hpp"
#include "L2_definitions.hpp"

// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"

using namespace std;
using namespace xf::sparse;
int main(int argc, char** argv) {
    if (argc < 5) {
        cout << "ERROR: passed %d arguments, expected at least 5 arguments." << endl;
        cout << "  Usage: cscmv.exe cscmv.xclbin Rows Cols NNZs [mtxFile]" << endl;
        return EXIT_FAILURE;
    }

    string l_xclbinFile = argv[1];
    unsigned int l_rows = atoi(argv[2]);
    unsigned int l_cols = atoi(argv[3]);
    unsigned int l_nnzs = atoi(argv[4]);
    string l_mxtFile = (argc == 6) ? argv[5] : "";

    CscMatType l_a;
    ColVecType l_x;
    ColVecType l_y;

    ProgramType l_program;
    GenCscMatType l_genCscMat;
    GenVecType l_genColVec;

    assert(l_cols % ColVecType::t_MemWords == 0);

    if (!l_genCscMat.genCscMatFromRnd(l_rows, l_cols, l_nnzs, 10, 3 / 2, l_program, l_a)) {
        cout << "ERROR: failed to generate sparse matrix A" << endl;
        return EXIT_FAILURE;
    }
    if (!l_genColVec.genColVecFromRnd(l_cols, 10, 3 / 2, l_program, l_x)) {
        cout << "ERROR: failed to generate input vector x" << endl;
        return EXIT_FAILURE;
    }
    if (!l_genColVec.genEmptyColVec(l_rows, l_program, l_y)) {
        cout << "ERROR: failed to generate output vector y" << endl;
        return EXIT_FAILURE;
    }

    l_rows = l_a.getRows();
    l_cols = l_a.getCols();
    l_nnzs = l_a.getNnzs();
    assert(l_cols == l_x.getEntries());
    assert(l_rows == l_y.getEntries());

    unsigned int l_colVecMemBlocks = l_cols / ColVecType::t_MemWords;
    unsigned int l_colPtrBlocks = l_cols / SPARSE_parEntries;
    unsigned int l_nnzBlocks = l_nnzs / SPARSE_parEntries;
    unsigned int l_rowCompBlocks = l_rows / SPARSE_parEntries / SPARSE_parGroups;
    unsigned int l_rowMemBlocks = l_a.getRows() / ColVecType::t_MemWords;

    // OpenCL host code begins
    cl_int l_err;
    cl::Device l_device;
    cl::Context l_context;
    cl::CommandQueue l_cmdQueue;
    cl::Program l_clProgram;
    cl::Kernel l_krnlLoad;
    cl::Kernel l_krnlXbarCol;
    cl::Kernel l_krnlCscRow;
    cl::Kernel l_krnlStore;

    auto l_devices = xcl::get_xil_devices();
    l_device = l_devices[0];

    // create context
    OCL_CHECK(l_err, l_context = cl::Context(l_device, NULL, NULL, NULL, &l_err));

    // create command queue
    OCL_CHECK(l_err,
              l_cmdQueue = cl::CommandQueue(l_context, l_device, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &l_err));

    // read_binary_file() is a utility API which will load the binaryFile
    // and will return the pointer to file buffer.
    auto l_fileBuf = xcl::read_binary_file(l_xclbinFile);

    cl::Program::Binaries l_bins{{l_fileBuf.data(), l_fileBuf.size()}};
    l_devices.resize(1);

    // Creating OpenCL Program
    OCL_CHECK(l_err, l_clProgram = cl::Program(l_context, l_devices, l_bins, NULL, &l_err));

    // Creating CUs
    OCL_CHECK(l_err, l_krnlLoad = cl::Kernel(l_clProgram, "loadColPtrValKernel", &l_err));
    OCL_CHECK(l_err, l_krnlXbarCol = cl::Kernel(l_clProgram, "xBarColKernel", &l_err));
    OCL_CHECK(l_err, l_krnlCscRow = cl::Kernel(l_clProgram, "cscRowPktKernel", &l_err));
    OCL_CHECK(l_err, l_krnlStore = cl::Kernel(l_clProgram, "storeDatPktKernel", &l_err));

    unsigned long long l_aColPtrBytes = l_program.getBufSz(l_a.getColPtrAddr());
    unsigned long long l_aNnzRowIdxBytes = l_program.getBufSz(l_a.getValRowIdxAddr());
    unsigned long long l_xBytes = l_program.getBufSz(l_x.getValAddr());
    unsigned long long l_yBytes = l_program.getBufSz(l_y.getValAddr());

    // Allocate device buffers in Global Memory
    OCL_CHECK(l_err, cl::Buffer l_aColPtrDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, l_aColPtrBytes,
                                                l_a.getColPtrAddr(), &l_err));
    OCL_CHECK(l_err, cl::Buffer l_aNnzRowIdxDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, l_aNnzRowIdxBytes,
                                                   l_a.getValRowIdxAddr(), &l_err));
    OCL_CHECK(l_err, cl::Buffer l_xDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, l_xBytes, l_x.getValAddr(),
                                          &l_err));
    OCL_CHECK(l_err, cl::Buffer l_yDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, l_yBytes,
                                          l_y.getValAddr(), &l_err));

    // Setting Kernel Arguments
    OCL_CHECK(l_err, l_err = l_krnlLoad.setArg(0, l_xDevBuf));
    OCL_CHECK(l_err, l_err = l_krnlLoad.setArg(1, l_aColPtrDevBuf));
    OCL_CHECK(l_err, l_err = l_krnlLoad.setArg(2, l_colVecMemBlocks));
    OCL_CHECK(l_err, l_err = l_krnlLoad.setArg(3, 1));

    OCL_CHECK(l_err, l_err = l_krnlXbarCol.setArg(0, l_colPtrBlocks));
    OCL_CHECK(l_err, l_err = l_krnlXbarCol.setArg(1, l_nnzBlocks));

    OCL_CHECK(l_err, l_err = l_krnlCscRow.setArg(0, l_aNnzRowIdxDevBuf));
    OCL_CHECK(l_err, l_err = l_krnlCscRow.setArg(1, l_nnzBlocks));
    OCL_CHECK(l_err, l_err = l_krnlCscRow.setArg(2, l_nnzBlocks));
    OCL_CHECK(l_err, l_err = l_krnlCscRow.setArg(3, l_rowCompBlocks));

    OCL_CHECK(l_err, l_err = l_krnlStore.setArg(1, l_yDevBuf));
    OCL_CHECK(l_err, l_err = l_krnlStore.setArg(2, l_rowMemBlocks));

    // Copy input data to device memory
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueMigrateMemObjects({l_aColPtrDevBuf, l_xDevBuf, l_aNnzRowIdxDevBuf},
                                                                 0 /* 0 means from host*/));

    // Launch the kernel
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueTask(l_krnlLoad));
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueTask(l_krnlXbarCol));
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueTask(l_krnlCscRow));
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueTask(l_krnlStore));
    l_cmdQueue.finish();

    // COpy results from device to host
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueMigrateMemObjects({l_yDevBuf}, CL_MIGRATE_MEM_OBJECT_HOST));
    l_cmdQueue.finish();

    // compute the golden reference and compare with the kernel outputs
    vector<SPARSE_dataType> l_yVecOut;
    vector<SPARSE_dataType> l_yVecRef;
    vector<SPARSE_dataType> l_xVec;
    vector<NnzUnitType> l_aMat;
    vector<SPARSE_indexType> l_aColPtr;
    l_a.loadValRowIdx(l_aMat);
    l_a.loadColPtr(l_aColPtr);
    l_x.loadVal(l_xVec);
    l_y.loadVal(l_yVecOut);

    assert(l_xVec.size() == l_cols);
    assert(l_yVecOut.size() == l_rows);
    assert(l_aColPtr.size() == l_cols);

    for (unsigned int i = 0; i < l_rows; ++i) {
        l_yVecRef.push_back(0);
    }

    SPARSE_indexType l_prePtr = 0;
    unsigned int l_nnzIdx = 0;
    for (unsigned int i = 0; i < l_cols; ++i) {
        SPARSE_indexType l_colPtr = l_aColPtr[i] - l_prePtr;
        l_prePtr = l_aColPtr[i];
        for (unsigned int j = 0; j < l_colPtr; ++j) {
            unsigned int l_rowIdx = l_aMat[l_nnzIdx].getRow();
            l_yVecRef[l_rowIdx] += l_aMat[l_nnzIdx].getVal() * l_xVec[i];
            l_nnzIdx++;
        }
    }

    unsigned int l_errs = 0;
    for (unsigned int i = 0; i < l_rows; ++i) {
        if (!compare<SPARSE_dataType>(l_yVecOut[i], l_yVecRef[i])) {
            cout << "ERROR: out[" << i << "] != ref[" << i << "] ";
            cout << "out[" << i << "] = " << l_yVecOut[i] << " ";
            cout << "ref[" << i << "] = " << l_yVecRef[i] << endl;
            l_errs++;
        }
    }

    if (l_errs == 0) {
        cout << "Test Pass!" << endl;
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
