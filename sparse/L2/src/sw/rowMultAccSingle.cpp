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
 * @file rowMultAcc.cpp
 * @brief main function for rowMultAcc kernel host code
 *
 * This file is part of Vitis SPARSE Library.
 */

#include <cmath>
#include <cstdlib>
#include <string>
#include <iostream>
#include "L2_definitions.hpp"

#if VITIS_TEST
// This extension file is required for stream APIs
#include "CL/cl_ext_xilinx.h"
// This file is required for OpenCL C++ wrapper APIs
#include "xcl2.hpp"
#else
#include "cscKernel.hpp"
void rowMultAccKernel(const ap_uint<SPARSE_ddrMemBits>* p_memRdPtr,
                      const unsigned int p_memRdBlocks,
                      const ap_uint<SPARSE_hbmMemBits>* p_aNnzIdx,
                      const unsigned int p_nnzBlocks,
                      const unsigned int p_rowBlocks,
                      const unsigned int p_memWrBlocks,
                      ap_uint<SPARSE_ddrMemBits>* p_memWrPtr);
#endif

using namespace std;
using namespace xf::sparse;

template <typename T>
bool isClose(float p_tolRel, float p_tolAbs, T p_vRef, T p_v, bool& p_exactMatch) {
    float l_diffAbs = abs(p_v - p_vRef);
    p_exactMatch = (p_vRef == p_v);
    bool l_status = (l_diffAbs <= (p_tolAbs + p_tolRel * abs(p_vRef)));
    return (l_status);
}
template <typename T>
bool compare(T x, T ref) {
    return x == ref;
}

template <>
bool compare<double>(double x, double ref) {
    bool l_exactMatch;
    return isClose<float>(1e-3, 3e-6, x, ref, l_exactMatch);
}
template <>
bool compare<float>(float x, float ref) {
    bool l_exactMatch;
    return isClose<float>(1e-3, 3e-6, x, ref, l_exactMatch);
}

int main(int argc, char** argv) {
    if (argc < 5) {
        cout << "ERROR: passed %d arguments, expected at least 5 arguments." << endl;
        cout << "  Usage: rowMultAccSingle.exe rowMultAcc.xclbin M N NNZs [mtxFile]" << endl;
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

    if (!l_genCscMat.genCscMatFromRnd(l_rows, l_cols, l_nnzs, 10, 3 / 2, l_program, l_a)) {
        cout << "ERROR: failed to generate sparse matrix A" << endl;
        return EXIT_FAILURE;
    }
    if (!l_genColVec.genColVecFromRnd(l_nnzs, 10, 3 / 2, l_program, l_x)) {
        cout << "ERROR: failed to generate input vector x" << endl;
        return EXIT_FAILURE;
    }
    if (!l_genColVec.genEmptyColVec(l_rows, l_program, l_y)) {
        cout << "ERROR: failed to generate output vector y" << endl;
        return EXIT_FAILURE;
    }

    unsigned int l_colVecMemBlocks = l_x.getEntries() / ColVecType::t_MemWords;
    unsigned int l_nnzBlocks = l_a.getNnzs() / SPARSE_parEntries;
    unsigned int l_rowCompBlocks = l_a.getRows() / SPARSE_parEntries / SPARSE_parGroups;
    unsigned int l_rowMemBlocks = l_a.getRows() / ColVecType::t_MemWords;

#if VITIS_TEST
    // OpenCL host code begins
    cl_int l_err;
    cl::Device l_device;
    cl::Context l_context;
    cl::CommandQueue l_cmdQueue;
    cl::Program l_clProgram;
    cl::Kernel l_krnlRowMultAcc;

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
    OCL_CHECK(l_err, l_krnlRowMultAcc = cl::Kernel(l_clProgram, "rowMultAccKernel", &l_err));

    unsigned long long l_aBytes = l_program.getBufSz(l_a.getValRowIdxAddr());
    unsigned long long l_xBytes = l_program.getBufSz(l_x.getValAddr());
    unsigned long long l_yBytes = l_program.getBufSz(l_y.getValAddr());

    // Allocate device buffers in Global Memory
    OCL_CHECK(l_err, cl::Buffer l_aDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, l_aBytes,
                                          l_a.getValRowIdxAddr(), &l_err));
    OCL_CHECK(l_err, cl::Buffer l_xDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, l_xBytes, l_x.getValAddr(),
                                          &l_err));
    OCL_CHECK(l_err, cl::Buffer l_yDevBuf(l_context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, l_yBytes,
                                          l_y.getValAddr(), &l_err));

    // Setting Kernel Arguments
    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(0, l_xDevBuf));
    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(1, l_colVecMemBlocks));

    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(2, l_aDevBuf));
    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(3, l_nnzBlocks));
    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(4, l_rowCompBlocks));

    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(5, l_rowMemBlocks));
    OCL_CHECK(l_err, l_err = l_krnlRowMultAcc.setArg(6, l_yDevBuf));

    // Copy input data to device memory
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueMigrateMemObjects({l_aDevBuf, l_xDevBuf}, 0 /* 0 means from host*/));

    // Launch the kernel
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueTask(l_krnlRowMultAcc));
    l_cmdQueue.finish();

    // COpy results from device to host
    OCL_CHECK(l_err, l_err = l_cmdQueue.enqueueMigrateMemObjects({l_yDevBuf}, CL_MIGRATE_MEM_OBJECT_HOST));
    l_cmdQueue.finish();

#else

    rowMultAccKernel(reinterpret_cast<ap_uint<SPARSE_ddrMemBits>*>(l_x.getValAddr()), l_colVecMemBlocks,
                     reinterpret_cast<ap_uint<SPARSE_hbmMemBits>*>(l_a.getValRowIdxAddr()), l_nnzBlocks,
                     l_rowCompBlocks, l_rowMemBlocks, reinterpret_cast<ap_uint<SPARSE_ddrMemBits>*>(l_y.getValAddr()));

#endif

    // compute the golden reference and compare with the kernel outputs
    vector<SPARSE_dataType> l_yVecOut;
    vector<SPARSE_dataType> l_yVecRef;
    vector<SPARSE_dataType> l_xVec;
    vector<NnzUnitType> l_aMat;
    l_a.loadValRowIdx(l_aMat);
    l_x.loadVal(l_xVec);
    l_y.loadVal(l_yVecOut);
    assert(l_xVec.size() == l_nnzs);
    assert(l_yVecOut.size() == l_rows);
    assert(l_aMat.size() == l_nnzs);

    for (unsigned int i = 0; i < l_rows; ++i) {
        l_yVecRef.push_back(0);
    }
    for (unsigned int i = 0; i < l_nnzs; ++i) {
        SPARSE_indexType l_rowIdx = l_aMat[i].getRow();
        l_yVecRef[l_rowIdx] += l_aMat[i].getVal() * l_xVec[i];
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
