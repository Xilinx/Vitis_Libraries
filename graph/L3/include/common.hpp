/*
 * Copyright 2020 Xilinx, Inc.
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

#pragma once

#ifndef _XF_GRAPH_L3_COMMON_HPP_
#define _XF_GRAPH_L3_COMMON_HPP_

#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>

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
#define XCL_BANK16 XCL_BANK(16)
#define XCL_BANK17 XCL_BANK(17)
#define XCL_BANK18 XCL_BANK(18)
#define XCL_BANK19 XCL_BANK(19)
#define XCL_BANK20 XCL_BANK(20)
#define XCL_BANK21 XCL_BANK(21)
#define XCL_BANK22 XCL_BANK(22)
#define XCL_BANK23 XCL_BANK(23)
#define XCL_BANK24 XCL_BANK(24)
#define XCL_BANK25 XCL_BANK(25)
#define XCL_BANK26 XCL_BANK(26)
#define XCL_BANK27 XCL_BANK(27)
#define XCL_BANK28 XCL_BANK(28)
#define XCL_BANK29 XCL_BANK(29)
#define XCL_BANK30 XCL_BANK(30)
#define XCL_BANK31 XCL_BANK(31)

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePointType;

struct ToolOptions {
    int argc;
    char** argv; // strings not owned!

    double opts_C_thresh;   //; //Threshold with coloring on
    long opts_minGraphSize; //; //Min |V| to enable coloring
    double threshold;       //; //Value of threshold
    int opts_ftype;         //; //File type
    char opts_inFile[4096]; //;
    bool opts_coloring;     //
    bool opts_output;       //;
    std::string outputFile;
    bool opts_VF; //;
    std::string xclbinFile;
    std::string deviceNames;
    int numNodes;
    int nodeId;
    int numThreads;
    int numPars;
    int gh_par; // same as par_prune
    int kernelMode;
    int numDevices;
    int modeZmq;
    char path_zmq[4096];
    bool useCmd;
    int mode_alveo;
    char nameProj[4096];      // used for create partitions
    std::string alveoProject; // used for load/compute TODO: consolidate with nameProj
    int numPureWorker;
    char* nameWorkers[128];
    int max_level;
    int max_iter;

    ToolOptions(int argc, char** argv);
};

enum { ZMQ_NONE = 0, ZMQ_DRIVER = 1, ZMQ_WORKER = 2 };

namespace xf {
namespace graph {
namespace L3 {

template <typename T>
T* aligned_alloc(std::size_t num) {
    void* ptr = nullptr;
#if _WIN32
    ptr = (T*)malloc(num * sizeof(T));
    if (num == 0) {
#else
    if (posix_memalign(&ptr, 4096, num * sizeof(T))) {
#endif
        throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(ptr);
}

template <typename MType>
union f_cast;

template <>
union f_cast<uint32_t> {
    uint32_t f;
    uint32_t i;
};

template <>
union f_cast<double> {
    double f;
    uint64_t i;
};

template <>
union f_cast<float> {
    float f;
    uint32_t i;
};

double showTimeData(std::string p_Task, TimePointType& t1, TimePointType& t2);

} // L3
} // graph
} // xf

#endif
