/*
 * Copyright 2020-2021 Xilinx, Inc.
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

#include "op_louvainmodularity.hpp"
#include "xf_utils_sw/logger.hpp"
#include "defs.hpp"
#include <unordered_map>

namespace xf {
namespace graph {
namespace L3 {

#define MAX_LOUVAINMOD_CU 128
std::mutex louvainmodComputeMutex[MAX_LOUVAINMOD_CU];

uint32_t opLouvainModularity::cuPerBoardLouvainModularity;

uint32_t opLouvainModularity::dupNmLouvainModularity;

void opLouvainModularity::createHandle(class openXRM* xrm,
                                       clHandle& handle,
                                       std::string kernelName,
                                       std::string kernelAlias,
                                       std::string xclbinFile,
                                       int32_t IDDevice,
                                       unsigned int requestLoad) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n    IDDevice=" << IDDevice << "\n    handle=" << &handle << std::endl;
#endif

    xf::common::utils_sw::Logger logger(std::cout, std::cerr);
    cl_int fail;

    // Platform related operations
    std::vector<cl::Device> devices = xcl::get_xil_devices();
    handle.device = devices[IDDevice];
    handle.context = cl::Context(handle.device);
    handle.q = cl::CommandQueue(handle.context, handle.device,
                                CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &fail);
    logger.logCreateCommandQueue(fail);
    std::string devName = handle.device.getInfo<CL_DEVICE_NAME>();
    // set up global values based on device selected
    // TODO: this needs to be revisited to support mixed cards.
    std::string key50("u50");
    std::string key55("u55c");
    std::size_t found50 = devName.rfind(key50);
    std::size_t found55 = devName.rfind(key55);
    if (found50 != std::string::npos) {
        glb_MAXNV = (1ul << 26);
        glb_MAXNE = (1ul << 27);
        glb_MAXNV_M = (64000000);
        std::cout << "INFO : Find u50 shell!" << std::endl;
    } else if (found55 != std::string::npos) {
        glb_MAXNV = (1ul << 27);
        glb_MAXNE = (1ul << 28);
        glb_MAXNV_M = (128000000);
        std::cout << "INFO : Find u55 shell!" << std::endl;
    } else {
        std::cout << "ERROR: Please use U55C for this case!" << std::endl;
    }
/*
if (devName == "xilinx_u50_gen3x16_xdma_201920_3") {
    glb_MAXNV = (1ul << 26);
    glb_MAXNE = (1ul << 27);
    glb_MAXNV_M = (64000000);
} else if (devName == "xilinx_u55c_gen3x16_xdma_3_202110_1") {
    glb_MAXNV = (1ul << 27);
    glb_MAXNE = (1ul << 28);
    glb_MAXNV_M = (128000000);
} else if (devName == "xilinx_u55c_gen3x16_xdma_3_202210_1") {
    glb_MAXNV = (1ul << 27);
    glb_MAXNE = (1ul << 28);
    glb_MAXNV_M = (128000000);
} else if (devName == "xilinx_u55c_gen3x16_xdma_3_202210_1") {
    glb_MAXNV = (1ul << 27);
    glb_MAXNE = (1ul << 28);
    glb_MAXNV_M = (128000000);
} else {
    std::cout << "ERROR: Please use U55C for this case!" << std::endl;
}
*/
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n    device:" << devName << "\n    glb_MAXNV:" << glb_MAXNV
              << "\n    glb_MAXNE:" << glb_MAXNE << "\n    glb_MAXNV_M:" << glb_MAXNV_M << std::endl;
#endif
    handle.xclBins = xcl::import_binary_file(xclbinFile);
    std::vector<cl::Device> devices2;
    devices2.push_back(handle.device);
    // TODO: handle execption from cl::Program
    devices2.resize(1); // must be here in U55C
    handle.program = cl::Program(handle.context, devices2, handle.xclBins, NULL, &fail);
    logger.logCreateProgram(fail);

    handle.resR = (xrmCuResource*)malloc(sizeof(xrmCuResource));
    memset(handle.resR, 0, sizeof(xrmCuResource));
    int ret = xrm->allocCU(handle.resR, kernelName.c_str(), kernelAlias.c_str(), requestLoad);
    std::string instanceName0;
    if (ret == 0) {
        handle.resR->instanceName; // "kernel_louvain_0";//
        if (cuPerBoardLouvainModularity >= 2) instanceName0 = "kernel_louvain:{" + instanceName0 + "}";
    } else {
        instanceName0 = "kernel_louvain";
        std::string cuID = std::to_string(handle.cuID);
        if (cuPerBoardLouvainModularity >= 2) {
            std::string instanceName1 = instanceName0 + "_" + cuID;
            strcpy(handle.resR->instanceName, instanceName1.c_str());
            instanceName0 = "kernel_louvain:{" + instanceName0 + "_" + cuID + "}";
        }
    }
    handle.isBusy = false;
    const char* instanceName = instanceName0.c_str();
    handle.kernel = cl::Kernel(handle.program, instanceName);
    logger.logCreateKernel(fail);

#ifndef NDEBUG
    std::cout << "DEBUG:" << __FUNCTION__ << " IDDevice=" << IDDevice << "=" << devName << " CommandQueue=" << &handle.q
              << " kernel=" << &handle.kernel << " kernelName=" << kernelName << " kernelAlias=" << kernelAlias
              << " resR.deviceId=" << handle.resR->deviceId << " resR.cuId=" << handle.resR->cuId
              << " resR.channelID=" << handle.resR->channelId << " resR.instanceName=" << handle.resR->instanceName
              << " instanceName0=" << instanceName0 << " created" << std::endl;

#endif
};

void opLouvainModularity::setHWInfo(uint32_t numDevices, uint32_t CUmax) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << " numDevices=" << numDevices << " maxCU=" << maxCU << std::endl;
#endif
    maxCU = CUmax;
    deviceNm = numDevices;
    cuPerBoardLouvainModularity = maxCU / deviceNm;

    handles = new clHandle[CUmax];
    // buff_hosts = new KMemorys_host[deviceNm];
    buff_hosts_prune = new KMemorys_host_prune[CUmax];
};

void opLouvainModularity::freeLouvainModularity(xrmContext* ctx) {
    std::cout << "INFO: " << __FUNCTION__ << std::endl;

    for (int i = 0; i < maxCU; ++i) {
        delete[] handles[i].buffer;
        int deviceId = handles[i].resR->deviceId;
        int cuId = handles[i].resR->cuId;
#ifndef NDEBUG
        std::cout << "DEBUG:" << __FUNCTION__ << " resR.deviceId=" << handles[i].resR->deviceId
                  << " resR.cuId=" << handles[i].resR->cuId << " resR.channelID=" << handles[i].resR->channelId
                  << " resR.instanceName=" << handles[i].resR->instanceName << std::endl;
#endif
        if (!xrmCuRelease(ctx, handles[i].resR)) {
            std::cout << "ERROR:" << __FUNCTION__ << " xrmCuRelease failed: deviceId=" << deviceId << " cuId=" << cuId
                      << std::endl;
        };
    }
    delete[] handles;
};

void opLouvainModularity::init(class openXRM* xrm,
                               std::string kernelName,
                               std::string kernelAlias,
                               std::string xclbinFile,
                               uint32_t* deviceIDs,
                               uint32_t* cuIDs,
                               unsigned int requestLoad) {
    dupNmLouvainModularity = 100 / requestLoad;
    cuPerBoardLouvainModularity /= dupNmLouvainModularity;
    unsigned int cnt = 0;
    unsigned int cntCU = 0;
    unsigned int* handleID = new unsigned int[maxCU];
    handleID[0] = cnt;
    handles[0].deviceID = deviceIDs[0];
    handles[0].cuID = cuIDs[0];
    handles[0].dupID = 0;
    std::thread th[maxCU];

    createHandle(xrm, handles[cnt], kernelName, kernelAlias, xclbinFile, deviceIDs[cnt], requestLoad);
    handles[cnt].buffer = new cl::Buffer[numBuffers_];
    unsigned int prev = deviceIDs[0];
    deviceOffset.push_back(0);
    for (int i = 1; i < maxCU; ++i) {
        handles[i].deviceID = deviceIDs[i];
        handles[i].cuID = cuIDs[i];
        handles[i].dupID = i % dupNmLouvainModularity;
        createHandle(xrm, handles[i], kernelName, kernelAlias, xclbinFile, deviceIDs[i], requestLoad);
        handles[i].buffer = new cl::Buffer[numBuffers_];
        if (deviceIDs[i] != prev) {
            prev = deviceIDs[i];
            deviceOffset.push_back(i);
        }
    }
    delete[] handleID;
}

void opLouvainModularity::migrateMemObj(clHandle* hds,
                                        bool type,
                                        unsigned int num_runs,
                                        std::vector<cl::Memory>& ob,
                                        std::vector<cl::Event>* evIn,
                                        cl::Event* evOut) {
    for (int i = 0; i < num_runs; ++i) {
        hds[0].q.enqueueMigrateMemObjects(ob, type, evIn, evOut); // 0 : migrate from host to dev
    }
};

void opLouvainModularity::releaseMemObjects(clHandle* hds, uint32_t numBuffers) {
#ifndef NDEBUG
    std::cout << "DEBUG: releaseMemObjects numBuffers=" << numBuffers << std::endl;
    delete[] hds[0].buffer;
#endif
};

// mapHostToClBuffers runs only once for each handle since all buffers are mapped
// to FPGA with fixed sizes.
void opLouvainModularity::mapHostToClBuffers(
    graphNew* Graph, int kernelMode, bool opts_coloring, long opts_minGraphSize, double opts_C_thresh, int numThreads) {
    unsigned long NV_orig; // = G->numVertices;
    unsigned long NE_orig; // = G->numEdges;

    NV_orig = glb_MAXNV_M;   // Now using fixed size for L3
    NE_orig = (glb_MAXNV_M); // Now using fixed size for L3
    if (NV_orig >= glb_MAXNV - 1) {
        printf("WARNING: G->numVertices(%lx) is more than glb_MAXNV(%lx), partition should be used\n", NV_orig,
               glb_MAXNV);
        NV_orig = glb_MAXNV - 2;
    }

    long NE_mem = NE_orig * 2; // number for real edge to be stored in memory
    long NE_mem_1 = NE_mem < (glb_MAXNV) ? NE_mem : (glb_MAXNV);
    long NE_mem_2 = NE_mem - NE_mem_1;
#ifndef NDEBUG
    printf("INFO: in mapHostToClBuffers kernelMode = %d\n\n", kernelMode);
#endif
    // for(int i=0; i < deviceNm; i++) {
    for (int i = 0; i < maxCU; i++) {
        if (kernelMode == LOUVAINMOD_PRUNING_KERNEL) {
#ifndef NDEBUG
            std::cout << "DEBUG: Start LOUVAINMOD_PRUNING_KERNEL UsingFPGA_MapHostClBuff_prune for host buffer[" << i
                      << "]" << std::endl;
#endif
            UsingFPGA_MapHostClBuff_prune(&handles[i], NV_orig, NE_mem_1, NE_mem_2, &buff_hosts_prune[i]);
        } else if (kernelMode == LOUVAINMOD_2CU_U55C_KERNEL) {
#ifndef NDEBUG
            std::cout << "INFO: Start LOUVAINMOD_2CU_U55C_KERNEL Create host buffer[" << i << "] for 2-cu "
                      << std::endl;
#endif
            unsigned long NV_MAX = 85000000; // limited max vertexes for 2cu verion
            UsingFPGA_MapHostClBuff_prune_2cu(&handles[i], NV_MAX, NE_mem_1, NE_mem_2, &buff_hosts_prune[i]);
        }
    }
};

int opLouvainModularity::compute(unsigned int deviceID,
                                 unsigned int cuID,
                                 unsigned int channelID,
                                 xrmContext* ctx,
                                 xrmCuResource* resR,
                                 std::string instanceName0,
                                 clHandle* handles,
                                 int kernelMode,
                                 uint32_t numBuffers,
                                 GLV* pglv_iter,
                                 double opts_C_thresh,
                                 // KMemorys_host* buff_host,
                                 KMemorys_host_prune* buff_host_prune,
                                 int* eachItrs,
                                 double* currMod,
                                 long* numClusters,
                                 double* eachTimeInitBuff,
                                 double* eachTimeReadBuff) {
    uint32_t which =
        channelID + cuID * dupNmLouvainModularity + deviceID * dupNmLouvainModularity * cuPerBoardLouvainModularity;
#ifdef PRINTINFO
    printf(
        "INFO: compute channelID=%d, cuID=%d dupNmLouvainModularity=%d deviceID=%d cuPerBoardLouvainModularity=%d "
        "which=%d \n",
        channelID, cuID, dupNmLouvainModularity, deviceID, cuPerBoardLouvainModularity, which);
    printf("    kernelMode = %d\n", kernelMode);
#endif

    std::lock_guard<std::mutex> lockMutex(louvainmodComputeMutex[which]);

    clHandle* hds = &handles[which];
#ifdef PRINTINFO
    std::cout << "DEBUG: " << __FUNCTION__ << " IDDevice=" << deviceID << " handle=" << &handles[which] << std::endl;
#endif
    pglv_iter->times.deviceID[0] = deviceID;
    pglv_iter->times.cuID[0] = cuID;
    pglv_iter->times.channelID[0] = channelID;
    pglv_iter->times.eachTimeE2E[0] = omp_get_wtime();
    std::vector<cl::Memory> ob_in;
    std::vector<cl::Memory> ob_out;
    std::vector<std::vector<cl::Event> > kernel_evt0(1);
    std::vector<std::vector<cl::Event> > kernel_evt1(1);
    kernel_evt0[0].resize(1);
    kernel_evt1[0].resize(1);
    std::vector<cl::Event> events_write(1);
    std::vector<cl::Event> events_kernel(1);
    std::vector<cl::Event> events_read(1);

    cl::Device device = hds[0].device;
    // const char* instanceName = instanceName0.c_str();
    // Creating Context and Command Queue for selected Device
    cl::Context context = hds[0].context;
    cl::CommandQueue q = hds[0].q;
    std::string devName = device.getInfo<CL_DEVICE_NAME>();
#ifdef PRINTINFO
    printf("INFO: Found Device=%s\n", devName.c_str());
#endif
    cl::Kernel kernel_louvain = hds[0].kernel;

#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << " kernel=" << &kernel_louvain << " which=" << which << std::endl;
#endif

    bool isLargeEdge = pglv_iter->G->numEdges > (glb_MAXNV / 2);

    if (kernelMode == LOUVAINMOD_PRUNING_KERNEL) {
#ifdef PRINTINFO
        std::cout << "INFO: inside flow LOUVAINMOD_PRUNING_KERNEL " << std::endl;
#endif
        KMemorys_host_prune* buf_host_prune = &buff_host_prune[which];
#ifdef PRINTINFO
        std::cout << "INFO: buf_host_prune has been created" << std::endl;
#endif

        eachTimeInitBuff[0] =
            PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune(pglv_iter->numColors, pglv_iter->G, pglv_iter->M,
                                                          opts_C_thresh, currMod, pglv_iter->colors, buf_host_prune);
#ifdef PRINTINFO
        std::cout << "INFO: PhaseLoop_UsingFPGA_Prep_Init_buff_host #prune# done" << std::endl;
#endif
        PhaseLoop_UsingFPGA_1_KernelSetup_prune(isLargeEdge, kernel_louvain, ob_in, ob_out, hds);
#ifdef PRINTINFO
        std::cout << "\twhich=" << which << " PhaseLoop_UsingFPGA_1_KernelSetup Device Available: "
                  << std::endl; // << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
#endif
        // PhaseLoop_UsingFPGA_2_DataWriteTo (q, kernel_evt0, ob_in);
        migrateMemObj(hds, 0, 1, ob_in, nullptr, &events_write[0]);

        // PhaseLoop_UsingFPGA_3_KernelRun   (q, kernel_evt0, kernel_evt1, kernel_louvain);
        int ret = cuExecute(hds, kernel_louvain, 1, &events_write, &events_kernel[0]);

        // PhaseLoop_UsingFPGA_4_DataReadBack(q, kernel_evt1, ob_out);
        migrateMemObj(hds, 1, 1, ob_out, &events_kernel, &events_read[0]);
#ifdef PRINTINFO
        std::cout << "\twhich=" << which << " PhaseLoop_UsingFPGA_4_DataReadBack Device Available: "
                  << std::endl; // << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
#endif
        // PhaseLoop_UsingFPGA_5_KernelFinish(q);
        q.finish();
#ifdef PRINTINFO
        std::cout << "\twhich=" << which << " PhaseLoop_UsingFPGA_5_KernelFinish Device Available: "
                  << std::endl; // << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
#endif
        eachTimeReadBuff[0] = PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune(pglv_iter->NV, buf_host_prune, eachItrs,
                                                                            pglv_iter->C, eachItrs, currMod);
    } else {
#ifdef PRINTINFO
        std::cout << "INFO: inside flow 4 for renum+2cu opt version " << std::string(hds[0].resR->instanceName)
                  << std::endl;
#endif
        KMemorys_host_prune* buf_host_prune = &buff_host_prune[which]; //
#ifdef PRINTINFO
        std::cout << "INFO: buf_host_prune has been created" << std::endl;
#endif

        eachTimeInitBuff[0] = PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune_renumber_2cu(
            pglv_iter->numColors, pglv_iter->NVl, pglv_iter->G, pglv_iter->M, opts_C_thresh, currMod, pglv_iter->colors,
            buf_host_prune);
#ifdef PRINTINFO
        std::cout << "INFO: PhaseLoop_UsingFPGA_Prep_Init_buff_host #prune# done" << std::endl;
#endif
        PhaseLoop_UsingFPGA_1_KernelSetup_prune_2cu(isLargeEdge, kernel_louvain, ob_in, ob_out, hds);
#ifdef PRINTINFO
        std::cout << "\tPhaseLoop_UsingFPGA_1_KernelSetup Device Available: "
                  << std::endl; // << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
#endif
        // PhaseLoop_UsingFPGA_2_DataWriteTo (q, kernel_evt0, ob_in);
        migrateMemObj(hds, 0, 1, ob_in, nullptr, &events_write[0]);

        // PhaseLoop_UsingFPGA_3_KernelRun   (q, kernel_evt0, kernel_evt1, kernel_louvain);
        int ret = cuExecute(hds, kernel_louvain, 1, &events_write, &events_kernel[0]);

        // PhaseLoop_UsingFPGA_4_DataReadBack(q, kernel_evt1, ob_out);
        migrateMemObj(hds, 1, 1, ob_out, &events_kernel, &events_read[0]);
#ifdef PRINTINFO
        std::cout << "\tPhaseLoop_UsingFPGA_4_DataReadBack Device Available: "
                  << std::endl; // << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
#endif
        // PhaseLoop_UsingFPGA_5_KernelFinish(q);
        q.finish();
#ifdef PRINTINFO
        std::cout << "\tPhaseLoop_UsingFPGA_5_KernelFinish Device Available: "
                  << std::endl; // << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
#endif
        eachTimeReadBuff[0] = PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune_renumber(
            pglv_iter->NV, buf_host_prune, eachItrs, pglv_iter->C, eachItrs, currMod, numClusters);
    }
    pglv_iter->times.eachTimeE2E[0] = omp_get_wtime() - pglv_iter->times.eachTimeE2E[0];
    hds->isBusy = false;
    return 0;
}

double opLouvainModularity::PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune(int numColors,
                                                                          graphNew* G,
                                                                          long* M,
                                                                          double opts_C_thresh,
                                                                          double* currMod,
                                                                          // Updated variables
                                                                          int* colors,
                                                                          KMemorys_host_prune* buff_host) {
    long edgeNum;
    double time1 = omp_get_wtime();
    assert(numColors < COLORS);
    long vertexNum = G->numVertices;
    long* vtxPtr = G->edgeListPtrs;
    edge* vtxInd = G->edgeList;
    long NE = G->numEdges;
    long NEx2 = NE << 1;
    long NE1 = NEx2 < (glb_MAXNV) ? NEx2 : (glb_MAXNV); // 256MB/sizeof(int/float)=64M

    long cnt_e = 0;
    for (int i = 0; i < vertexNum + 1; i++) {
        buff_host[0].offsets[i] = (int)vtxPtr[i];
        if (i != vertexNum) {
            if (M[i] < 0) buff_host[0].offsets[i] = (int)(0x80000000 | (unsigned int)vtxPtr[i]);
        } else
            buff_host[0].offsets[i] = (int)(vtxPtr[i]);
    }
    edgeNum = buff_host[0].offsets[vertexNum];
    for (int i = 0; i < vertexNum + 1; i++) {
        buff_host[0].offsets[i] = (int)vtxPtr[i];
        buff_host[0].offsetsdup[i] = buff_host[0].offsets[i]; //
        if (i != vertexNum) {
            if (M[i] < 0) buff_host[0].offsets[i] = (int)(0x80000000 | (unsigned int)vtxPtr[i]);
        } else
            buff_host[0].offsets[i] = (int)(vtxPtr[i]);
    }
    edgeNum = buff_host[0].offsets[vertexNum];

    for (int i = 0; i < vertexNum; i++) {
        int adj1 = vtxPtr[i];
        int adj2 = vtxPtr[i + 1];
        buff_host[0].flag[i] = 0;       //
        buff_host[0].flagUpdate[i] = 0; //
        for (int j = adj1; j < adj2; j++) {
            if (cnt_e < NE1) {
                buff_host[0].indices[j] = (int)vtxInd[j].tail;
                buff_host[0].indicesdup[j] = (int)vtxInd[j].tail;
                buff_host[0].weights[j] = vtxInd[j].weight;
            } else {
                buff_host[0].indices2[j - NE1] = (int)vtxInd[j].tail;
                buff_host[0].indicesdup2[j - NE1] = (int)vtxInd[j].tail;
                buff_host[0].weights2[j - NE1] = vtxInd[j].weight;
            }
            cnt_e++;
        }
    }
    for (int i = 0; i < vertexNum; i++) {
        buff_host[0].colorAxi[i] = colors[i];
    }

    buff_host[0].config0[0] = vertexNum;
    buff_host[0].config0[1] = numColors;
    buff_host[0].config0[2] = 0;
    buff_host[0].config0[3] = edgeNum;
    buff_host[0].config0[4] = 0; // totItr= 0

    buff_host[0].config1[0] = opts_C_thresh;
    buff_host[0].config1[1] = currMod[0];
    time1 = omp_get_wtime() - time1;
    // printf("maxv:%ld, NEx2:%ld, vertexNum: %ld, numColors:%d, edgeNum:%ld, opts_C_thresh:%f, currMod:%f\n",
    //        glb_MAXNV, NEx2, vertexNum, numColors, edgeNum, opts_C_thresh, currMod[0]);
    return time1;
#ifdef PRINTINFO_LVPHASE
    std::cout << "INFO: eachItrs" << buff_host[0].config0[2] << ", "
              << "eachItr[0] = " << buff_host[0].config0[2] << ", "
              << "currMod[0] = " << buff_host[0].config1[1] << std::endl;
#endif
}

double opLouvainModularity::PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune_renumber_2cu(int numColors,
                                                                                       long NVl,
                                                                                       graphNew* G,
                                                                                       long* M,
                                                                                       double opts_C_thresh,
                                                                                       double* currMod,
                                                                                       // Updated variables
                                                                                       int* colors,
                                                                                       KMemorys_host_prune* buff_host) {
    long edgeNum;
    double time1 = omp_get_wtime();
    assert(numColors < COLORS);
    long vertexNum = G->numVertices;
    long* vtxPtr = G->edgeListPtrs;
    edge* vtxInd = G->edgeList;
    long NE = G->numEdges;
    long NEx2 = NE << 1;
    long NE1 = NEx2 < (glb_MAXNV) ? NEx2 : (glb_MAXNV); // 256MB/sizeof(int/float)=64M

    long cnt_e = 0;
    for (int i = 0; i < vertexNum + 1; i++) {
        buff_host[0].offsets[i] = (int)vtxPtr[i];
        if (i != vertexNum) {
            if (M[i] < 0) buff_host[0].offsets[i] = (int)(0x80000000 | (unsigned int)vtxPtr[i]);
        } else
            buff_host[0].offsets[i] = (int)(vtxPtr[i]);
    }
    edgeNum = buff_host[0].offsets[vertexNum];
    for (int i = 0; i < vertexNum + 1; i++) {
        buff_host[0].offsets[i] = (int)vtxPtr[i];
        // buff_host[0].offsetsdup[i] = buff_host[0].offsets[i];//
        if (i != vertexNum) {
            if (M[i] < 0) buff_host[0].offsets[i] = (int)(0x80000000 | (unsigned int)vtxPtr[i]);
        } else
            buff_host[0].offsets[i] = (int)(vtxPtr[i]);
    }
    edgeNum = buff_host[0].offsets[vertexNum];

    for (int i = 0; i < vertexNum; i++) {
        int adj1 = vtxPtr[i];
        int adj2 = vtxPtr[i + 1];
        buff_host[0].flag[i] = 0;       //
        buff_host[0].flagUpdate[i] = 0; //
        for (int j = adj1; j < adj2; j++) {
            if (cnt_e < NE1) {
                buff_host[0].indices[j] = (int)vtxInd[j].tail;
                // buff_host[0].indicesdup[j] = (int)vtxInd[j].tail;
                buff_host[0].weights[j] = vtxInd[j].weight;
            } else {
                buff_host[0].indices2[j - NE1] = (int)vtxInd[j].tail;
                // buff_host[0].indicesdup2[j-NE1] = (int)vtxInd[j].tail;
                buff_host[0].weights2[j - NE1] = vtxInd[j].weight;
            }
            cnt_e++;
        }
    }
    for (int i = 0; i < vertexNum; i++) {
        buff_host[0].colorAxi[i] = colors[i];
    }

    buff_host[0].config0[0] = vertexNum;
    buff_host[0].config0[1] = numColors;
    buff_host[0].config0[2] = 0;
    buff_host[0].config0[3] = edgeNum;
    buff_host[0].config0[4] = 0; // zyx renumber
    buff_host[0].config0[5] = NVl;

    buff_host[0].config1[0] = opts_C_thresh;
    buff_host[0].config1[1] = currMod[0];
    time1 = omp_get_wtime() - time1;

    // printf("INFO: glb_MAXNV:%ld, vertexNum: %ld, NE: %ld , edgeNum:%ld, NVl:%ld, buff_host[0].config0[4]=%ld \n",
    // glb_MAXNV, vertexNum, NE, edgeNum,NVl,buff_host[0].config0[4]);
    // std::cout << "INFO: numColors" <<buff_host[0].config0[1] <<", "<<"eachItr[0] = "<<buff_host[0].config0[2]<<",
    // "<<"currMod[0] = "<<buff_host[0].config1[1]<< std::endl;

    return time1;

#ifdef PRINTINFO_LVPHASE
    std::cout << "INFO: numColors" << buff_host[0].config0[1] << ", "
              << "eachItr[0] = " << buff_host[0].config0[2] << ", "
              << "currMod[0] = " << buff_host[0].config1[1] << std::endl;
#endif
}

void opLouvainModularity::UsingFPGA_MapHostClBuff_prune(
    clHandle* hds, long NV, long NE_mem_1, long NE_mem_2, KMemorys_host_prune* buff_host_prune) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n    NV=" << NV << "\n    NE_mem_1=" << NE_mem_1
              << "\n    NE_mem_2=" << NE_mem_2 << std::endl;
#endif
    std::vector<cl_mem_ext_ptr_t> mext_in(NUM_PORT_KERNEL + 7);
    buff_host_prune[0].config0 = aligned_alloc<int64_t>(6); //
    buff_host_prune[0].config1 = aligned_alloc<DWEIGHT>(4);
    buff_host_prune[0].offsets = aligned_alloc<int>(NV + 1);
    buff_host_prune[0].indices = aligned_alloc<int>(NE_mem_1);
    buff_host_prune[0].offsetsdup = aligned_alloc<int>(NV + 1);   //
    buff_host_prune[0].indicesdup = aligned_alloc<int>(NE_mem_1); //
    buff_host_prune[0].weights = aligned_alloc<float>(NE_mem_1);
    buff_host_prune[0].flag = aligned_alloc<uint8_t>(NV);
    buff_host_prune[0].flagUpdate = aligned_alloc<uint8_t>(NV);
    if (NE_mem_2 > 0) {
        buff_host_prune[0].indices2 = aligned_alloc<int>(NE_mem_2);
        buff_host_prune[0].indicesdup2 = aligned_alloc<int>(NE_mem_2);
    }
    if (NE_mem_2 > 0) {
        buff_host_prune[0].weights2 = aligned_alloc<float>(NE_mem_2);
    }
    buff_host_prune[0].cidPrev = aligned_alloc<int>(NV);
    buff_host_prune[0].cidCurr = aligned_alloc<int>(NV);
    buff_host_prune[0].cidSizePrev = aligned_alloc<int>(NV);
    buff_host_prune[0].totPrev = aligned_alloc<float>(NV);
    buff_host_prune[0].cidSizeCurr = aligned_alloc<int>(NV);
    buff_host_prune[0].totCurr = aligned_alloc<float>(NV);
    buff_host_prune[0].cidSizeUpdate = aligned_alloc<int>(NV);
    buff_host_prune[0].totUpdate = aligned_alloc<float>(NV);
    buff_host_prune[0].cWeight = aligned_alloc<float>(NV);
    buff_host_prune[0].colorAxi = aligned_alloc<int>(NV);
    buff_host_prune[0].colorInx = aligned_alloc<int>(NV);

// ap_uint<CSRWIDTHS>* axi_offsets = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].offsets);
// ap_uint<CSRWIDTHS>* axi_indices = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indices);
// ap_uint<CSRWIDTHS>* axi_offsetsdup = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].offsetsdup);
// ap_uint<CSRWIDTHS>* axi_indicesdup = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indicesdup);
// ap_uint<CSRWIDTHS>* axi_weights = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].weights);
// ap_uint<CSRWIDTHS>* axi_indices2;
// ap_uint<CSRWIDTHS>* axi_indicesdup2 = 0;
// ap_uint<CSRWIDTHS>* axi_weights2;
// if (NE_mem_2>0){
// 	axi_indices2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indices2);
// 	axi_indicesdup2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indicesdup2);
// 	axi_weights2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].weights2);
// }
// ap_uint<DWIDTHS>*     axi_cidPrev       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidPrev);
// ap_uint<DWIDTHS>*     axi_cidCurr       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidCurr);
// ap_uint<DWIDTHS>*     axi_cidSizePrev   = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizePrev);
// ap_uint<DWIDTHS>*     axi_totPrev       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totPrev);
// ap_uint<DWIDTHS>*     axi_cidSizeCurr   = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizeCurr);
// ap_uint<DWIDTHS>*     axi_totCurr       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totCurr);
// ap_uint<DWIDTHS>*     axi_cidSizeUpdate = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizeUpdate);
// ap_uint<DWIDTHS>*     axi_totUpdate     = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totUpdate);
// ap_uint<DWIDTHS>*     axi_cWeight       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cWeight);
// ap_uint<COLORWIDTHS>* axi_colorAxi      = reinterpret_cast<ap_uint<COLORWIDTHS>*>(buff_host_prune[0].colorAxi);
// ap_uint<COLORWIDTHS>* axi_colorInx      = reinterpret_cast<ap_uint<COLORWIDTHS>*>(buff_host_prune[0].colorInx);
// uint8_t* axi_flag 					= reinterpret_cast<uint8_t*>(buff_host_prune[0].flag);
// uint8_t* axi_flagUpdate 				= reinterpret_cast<uint8_t*>(buff_host_prune[0].flagUpdate);
#ifndef NDEBUG
    std::cout << "INFO: start mext_in " << std::endl;
#endif
    // DDR Settings
    mext_in[0] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, buff_host_prune[0].config0, 0};
    mext_in[1] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, buff_host_prune[0].config1, 0};
    mext_in[2] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, buff_host_prune[0].offsets, 0};
    mext_in[3] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indices, 0};
    mext_in[4] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, buff_host_prune[0].weights, 0};
    if (NE_mem_2 > 0) {
        mext_in[3 + 18] = {(unsigned int)(1) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indices2, 0};
        mext_in[4 + 18] = {(unsigned int)(3) | XCL_MEM_TOPOLOGY, buff_host_prune[0].weights2, 0};
    }
    mext_in[5] = {(unsigned int)(6) | XCL_MEM_TOPOLOGY, buff_host_prune[0].colorAxi, 0};
    mext_in[6] = {(unsigned int)(8) | XCL_MEM_TOPOLOGY, buff_host_prune[0].colorInx, 0};
    mext_in[7] = {(unsigned int)(10) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidPrev, 0};
    mext_in[8] = {(unsigned int)(12) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizePrev, 0};
    mext_in[9] = {(unsigned int)(14) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totPrev, 0};
    mext_in[10] = {(unsigned int)(16) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidCurr, 0};
    mext_in[11] = {(unsigned int)(18) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizeCurr, 0};
    mext_in[12] = {(unsigned int)(20) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totCurr, 0};
    mext_in[13] = {(unsigned int)(22) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizeUpdate, 0};
    mext_in[14] = {(unsigned int)(24) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totUpdate, 0};
    mext_in[15] = {(unsigned int)(26) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cWeight, 0};

    mext_in[16] = {(unsigned int)(27) | XCL_MEM_TOPOLOGY, buff_host_prune[0].offsetsdup, 0};
    mext_in[17] = {(unsigned int)(28) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indicesdup, 0}; //| (unsigned int)(29)
    mext_in[20] = {(unsigned int)(29) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indicesdup2, 0};

    mext_in[18] = {(unsigned int)(7) | XCL_MEM_TOPOLOGY, buff_host_prune[0].flag, 0};
    mext_in[19] = {(unsigned int)(9) | XCL_MEM_TOPOLOGY, buff_host_prune[0].flagUpdate, 0};

    int flag_RW = CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE;
    int flag_RD = CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY;

    hds[0].buffer[0] = cl::Buffer(hds[0].context, flag_RW, sizeof(int64_t) * (6), &mext_in[0]);
    hds[0].buffer[1] = cl::Buffer(hds[0].context, flag_RW, sizeof(DWEIGHT) * (4), &mext_in[1]);
    hds[0].buffer[2] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV + 1), &mext_in[2]);
    hds[0].buffer[3] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_1, &mext_in[3]);
    hds[0].buffer[4] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_1, &mext_in[4]);
    hds[0].buffer[5] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV), &mext_in[5]);
    hds[0].buffer[6] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[6]);
    hds[0].buffer[7] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[7]);
    hds[0].buffer[8] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[8]);
    hds[0].buffer[9] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[9]);
    hds[0].buffer[10] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[10]);
    hds[0].buffer[11] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[11]);
    hds[0].buffer[12] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[12]);
    hds[0].buffer[13] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[13]);
    hds[0].buffer[14] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[14]);
    hds[0].buffer[15] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[15]);
    hds[0].buffer[16] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV + 1), &mext_in[16]); // offset
    hds[0].buffer[17] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NE_mem_1), &mext_in[17]);
    hds[0].buffer[18] = cl::Buffer(hds[0].context, flag_RW, sizeof(uint8_t) * (NV), &mext_in[18]);
    hds[0].buffer[19] = cl::Buffer(hds[0].context, flag_RW, sizeof(uint8_t) * (NV), &mext_in[19]);

    if (NE_mem_2 > 0) {
#ifndef NDEBUG
        std::cout << "INFO: NE_mem_2 =  " << NE_mem_2 << std::endl;
#endif
        hds[0].buffer[20] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[20]);
        hds[0].buffer[21] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[21]);
        hds[0].buffer[22] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[22]);
    }
#ifndef NDEBUG
    std::cout << "INFO: init buffer done!  " << std::endl;
#endif
}

int opLouvainModularity::cuExecute(
    clHandle* hds, cl::Kernel& kernel0, unsigned int num_runs, std::vector<cl::Event>* evIn, cl::Event* evOut)

{
    for (int i = 0; i < num_runs; ++i) {
        hds[0].q.enqueueTask(kernel0, evIn, evOut);
    }
    return 0;
}

void opLouvainModularity::PhaseLoop_UsingFPGA_1_KernelSetup_prune(bool isLargeEdge,
                                                                  cl::Kernel& kernel_louvain,
                                                                  std::vector<cl::Memory>& ob_in,
                                                                  std::vector<cl::Memory>& ob_out,
                                                                  // KMemorys_clBuff_prune   &buff_cl
                                                                  clHandle* hds) {
    // Data transfer from host buffer to device buffer
    ob_in.push_back(hds[0].buffer[0]);
    ob_in.push_back(hds[0].buffer[1]);
    ob_in.push_back(hds[0].buffer[2]);
    ob_in.push_back(hds[0].buffer[3]);
    if (isLargeEdge) ob_in.push_back(hds[0].buffer[21]);
    ob_in.push_back(hds[0].buffer[4]);
    if (isLargeEdge) ob_in.push_back(hds[0].buffer[22]);
    ob_in.push_back(hds[0].buffer[5]);
    ob_in.push_back(hds[0].buffer[6]);
    ob_in.push_back(hds[0].buffer[8]);
    ob_in.push_back(hds[0].buffer[9]);
    ob_in.push_back(hds[0].buffer[10]);
    ob_in.push_back(hds[0].buffer[11]);
    ob_in.push_back(hds[0].buffer[12]);
    ob_in.push_back(hds[0].buffer[13]);
    ob_in.push_back(hds[0].buffer[14]);
    ob_in.push_back(hds[0].buffer[15]);
    ob_out.push_back(hds[0].buffer[0]);
    ob_out.push_back(hds[0].buffer[1]);
    ob_out.push_back(hds[0].buffer[7]);
    ob_in.push_back(hds[0].buffer[16]);
    ob_in.push_back(hds[0].buffer[17]);
    ob_in.push_back(hds[0].buffer[18]);
    ob_in.push_back(hds[0].buffer[19]);
    if (isLargeEdge) ob_in.push_back(hds[0].buffer[20]);

    kernel_louvain.setArg(0, hds[0].buffer[0]);   // config0
    kernel_louvain.setArg(1, hds[0].buffer[1]);   // config1
    kernel_louvain.setArg(2, hds[0].buffer[2]);   // offsets
    kernel_louvain.setArg(3, hds[0].buffer[3]);   // indices
    kernel_louvain.setArg(4, hds[0].buffer[4]);   // weights
    kernel_louvain.setArg(5, hds[0].buffer[5]);   // colorAxi
    kernel_louvain.setArg(6, hds[0].buffer[6]);   // colorInx
    kernel_louvain.setArg(7, hds[0].buffer[7]);   // cidPrev
    kernel_louvain.setArg(8, hds[0].buffer[8]);   // cidSizePrev
    kernel_louvain.setArg(9, hds[0].buffer[9]);   // totPrev
    kernel_louvain.setArg(10, hds[0].buffer[10]); // cidCurr
    kernel_louvain.setArg(11, hds[0].buffer[11]); // cidSizeCurr
    kernel_louvain.setArg(12, hds[0].buffer[12]); // totCurr
    kernel_louvain.setArg(13, hds[0].buffer[13]); // cUpdate
    kernel_louvain.setArg(14, hds[0].buffer[14]); // totCurr
    kernel_louvain.setArg(15, hds[0].buffer[15]); // cWeight
    kernel_louvain.setArg(16, hds[0].buffer[16]); // offsets
    kernel_louvain.setArg(17, hds[0].buffer[17]); // indices
    kernel_louvain.setArg(18, hds[0].buffer[18]); // flag
    kernel_louvain.setArg(19, hds[0].buffer[19]); // db_flagUpdate
#ifdef PRINTINFO
    std::cout << "INFO: Finish kernel setup" << std::endl;
#endif
}

void opLouvainModularity::UsingFPGA_MapHostClBuff_prune_2cu(
    clHandle* hds, long NV, long NE_mem_1, long NE_mem_2, KMemorys_host_prune* buff_host_prune) {
    if (std::string(hds[0].resR->instanceName) == "kernel_louvain_0") {
#ifdef PRINTINFO
        std::cout << "INFO: start MapHostClBuff kernel_louvain_0" << std::endl;
#endif
        std::vector<cl_mem_ext_ptr_t> mext_in(NUM_PORT_KERNEL + 7);
        buff_host_prune[0].config0 = aligned_alloc<int64_t>(6);
        buff_host_prune[0].config1 = aligned_alloc<DWEIGHT>(4);
        buff_host_prune[0].offsets = aligned_alloc<int>(NV + 1);
        buff_host_prune[0].indices = aligned_alloc<int>(NE_mem_1);
        //
        //
        buff_host_prune[0].weights = aligned_alloc<float>(NE_mem_1);
        buff_host_prune[0].flag = aligned_alloc<uint8_t>(NV);
        buff_host_prune[0].flagUpdate = aligned_alloc<uint8_t>(NV);
        if (NE_mem_2 > 0) {
            buff_host_prune[0].indices2 = aligned_alloc<int>(NE_mem_2);
            //
        }
        if (NE_mem_2 > 0) {
            buff_host_prune[0].weights2 = aligned_alloc<float>(NE_mem_2);
        }
        buff_host_prune[0].cidPrev = aligned_alloc<int>(NV);
        buff_host_prune[0].cidCurr = aligned_alloc<int>(NV);
        buff_host_prune[0].cidSizePrev = aligned_alloc<int>(NV);
        buff_host_prune[0].totPrev = aligned_alloc<float>(NV);
        buff_host_prune[0].cidSizeCurr = aligned_alloc<int>(NV);
        buff_host_prune[0].totCurr = aligned_alloc<float>(NV);
        buff_host_prune[0].cidSizeUpdate = aligned_alloc<int>(NV);
        buff_host_prune[0].totUpdate = aligned_alloc<float>(NV);
        buff_host_prune[0].cWeight = aligned_alloc<float>(NV);
        buff_host_prune[0].colorAxi = aligned_alloc<int>(NV);
        buff_host_prune[0].colorInx = aligned_alloc<int>(NV);

// ap_uint<CSRWIDTHS>* axi_offsets = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].offsets);
// ap_uint<CSRWIDTHS>* axi_indices = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indices);
// //
// //
// ap_uint<CSRWIDTHS>* axi_weights = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].weights);
// ap_uint<CSRWIDTHS>* axi_indices2;
// //
// ap_uint<CSRWIDTHS>* axi_weights2;
// if(NE_mem_2>0){
// 	axi_indices2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indices2);
// 	//
// 	axi_weights2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].weights2);
// }
// ap_uint<DWIDTHS>*     axi_cidPrev       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidPrev);
// ap_uint<DWIDTHS>*     axi_cidCurr       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidCurr);
// ap_uint<DWIDTHS>*     axi_cidSizePrev   = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizePrev);
// ap_uint<DWIDTHS>*     axi_totPrev       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totPrev);
// ap_uint<DWIDTHS>*     axi_cidSizeCurr   = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizeCurr);
// ap_uint<DWIDTHS>*     axi_totCurr       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totCurr);
// ap_uint<DWIDTHS>*     axi_cidSizeUpdate = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizeUpdate);
// ap_uint<DWIDTHS>*     axi_totUpdate     = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totUpdate);
// ap_uint<DWIDTHS>*     axi_cWeight       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cWeight);
// ap_uint<COLORWIDTHS>* axi_colorAxi      = reinterpret_cast<ap_uint<COLORWIDTHS>*>(buff_host_prune[0].colorAxi);
// ap_uint<COLORWIDTHS>* axi_colorInx      = reinterpret_cast<ap_uint<COLORWIDTHS>*>(buff_host_prune[0].colorInx);
// uint8_t* axi_flag 					= reinterpret_cast<uint8_t*>(buff_host_prune[0].flag);
// uint8_t* axi_flagUpdate 				= reinterpret_cast<uint8_t*>(buff_host_prune[0].flagUpdate);
#ifdef PRINTINFO
        std::cout << "INFO: start mext_in " << std::endl;
#endif
        // DDR Settings
        mext_in[0] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, buff_host_prune[0].config0, 0};
        mext_in[1] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, buff_host_prune[0].config1, 0};
        mext_in[2] = {(unsigned int)(4) | XCL_MEM_TOPOLOGY, buff_host_prune[0].offsets, 0};
        mext_in[3] = {(unsigned int)(0) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indices, 0};
        mext_in[4] = {(unsigned int)(2) | XCL_MEM_TOPOLOGY, buff_host_prune[0].weights, 0};
        if (NE_mem_2 > 0) {
            mext_in[3 + 18] = {(unsigned int)(1) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indices2, 0};
            mext_in[4 + 18] = {(unsigned int)(3) | XCL_MEM_TOPOLOGY, buff_host_prune[0].weights2, 0};
        }
        mext_in[5] = {(unsigned int)(5) | XCL_MEM_TOPOLOGY, buff_host_prune[0].colorAxi, 0};
        mext_in[6] = {(unsigned int)(6) | XCL_MEM_TOPOLOGY, buff_host_prune[0].colorInx, 0};
        mext_in[7] = {(unsigned int)(7) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidPrev, 0};
        mext_in[8] = {(unsigned int)(8) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizePrev, 0};
        mext_in[9] = {(unsigned int)(9) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totPrev, 0};
        mext_in[10] = {(unsigned int)(10) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidCurr, 0};
        mext_in[11] = {(unsigned int)(11) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizeCurr, 0};
        mext_in[12] = {(unsigned int)(12) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totCurr, 0};
        mext_in[13] = {(unsigned int)(13) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizeUpdate, 0};
        mext_in[14] = {(unsigned int)(14) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totUpdate, 0};
        mext_in[15] = {(unsigned int)(15) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cWeight, 0};

        mext_in[18] = {(unsigned int)(5) | XCL_MEM_TOPOLOGY, buff_host_prune[0].flag, 0};
        mext_in[19] = {(unsigned int)(5) | XCL_MEM_TOPOLOGY, buff_host_prune[0].flagUpdate, 0};
        int flag_RW = CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE;
        int flag_RD = CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY;

        hds[0].buffer[0] = cl::Buffer(hds[0].context, flag_RW, sizeof(int64_t) * (6), &mext_in[0]);
        hds[0].buffer[1] = cl::Buffer(hds[0].context, flag_RW, sizeof(DWEIGHT) * (4), &mext_in[1]);
        hds[0].buffer[2] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV + 1), &mext_in[2]);
        hds[0].buffer[3] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_1, &mext_in[3]);
        hds[0].buffer[4] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_1, &mext_in[4]);
        hds[0].buffer[5] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV), &mext_in[5]);
        hds[0].buffer[6] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[6]);
        hds[0].buffer[7] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[7]);
        hds[0].buffer[8] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[8]);
        hds[0].buffer[9] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[9]);
        hds[0].buffer[10] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[10]);
        hds[0].buffer[11] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[11]);
        hds[0].buffer[12] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[12]);
        hds[0].buffer[13] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[13]);
        hds[0].buffer[14] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[14]);
        hds[0].buffer[15] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[15]);
        ////offset
        //
        hds[0].buffer[18] = cl::Buffer(hds[0].context, flag_RW, sizeof(uint8_t) * (NV), &mext_in[18]);
        std::cout << "INFO: init buffer  " << std::endl;
        hds[0].buffer[19] = cl::Buffer(hds[0].context, flag_RW, sizeof(uint8_t) * (NV), &mext_in[19]);
        std::cout << "INFO: init buffer 1  " << std::endl;
        if (NE_mem_2 > 0) {
#ifdef PRINTINFO
            std::cout << "INFO: NE_mem_2 =  " << NE_mem_2 << std::endl;
#endif
            //
            hds[0].buffer[21] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[21]);
            hds[0].buffer[22] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[22]);
        }
#ifdef PRINTINFO
        std::cout << "INFO: init buffer done!  " << std::endl;
#endif

    } else {
#ifdef PRINTINFO
        std::cout << "INFO: start MapHostClBuff kernel_louvain_1" << std::endl;
#endif
        std::vector<cl_mem_ext_ptr_t> mext_in(NUM_PORT_KERNEL + 7);
        buff_host_prune[0].config0 = aligned_alloc<int64_t>(6);
        buff_host_prune[0].config1 = aligned_alloc<DWEIGHT>(4);
        buff_host_prune[0].offsets = aligned_alloc<int>(NV + 1);
        buff_host_prune[0].indices = aligned_alloc<int>(NE_mem_1);
        //
        //
        buff_host_prune[0].weights = aligned_alloc<float>(NE_mem_1);
        buff_host_prune[0].flag = aligned_alloc<uint8_t>(NV);
        buff_host_prune[0].flagUpdate = aligned_alloc<uint8_t>(NV);
        if (NE_mem_2 > 0) {
            buff_host_prune[0].indices2 = aligned_alloc<int>(NE_mem_2);
            //
        }
        if (NE_mem_2 > 0) {
            buff_host_prune[0].weights2 = aligned_alloc<float>(NE_mem_2);
        }
        buff_host_prune[0].cidPrev = aligned_alloc<int>(NV);
        buff_host_prune[0].cidCurr = aligned_alloc<int>(NV);
        buff_host_prune[0].cidSizePrev = aligned_alloc<int>(NV);
        buff_host_prune[0].totPrev = aligned_alloc<float>(NV);
        buff_host_prune[0].cidSizeCurr = aligned_alloc<int>(NV);
        buff_host_prune[0].totCurr = aligned_alloc<float>(NV);
        buff_host_prune[0].cidSizeUpdate = aligned_alloc<int>(NV);
        buff_host_prune[0].totUpdate = aligned_alloc<float>(NV);
        buff_host_prune[0].cWeight = aligned_alloc<float>(NV);
        buff_host_prune[0].colorAxi = aligned_alloc<int>(NV);
        buff_host_prune[0].colorInx = aligned_alloc<int>(NV);

// ap_uint<CSRWIDTHS>* axi_offsets = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].offsets);
// ap_uint<CSRWIDTHS>* axi_indices = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indices);
// //
// //
// ap_uint<CSRWIDTHS>* axi_weights = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].weights);
// ap_uint<CSRWIDTHS>* axi_indices2;
// //
// ap_uint<CSRWIDTHS>* axi_weights2;
// if(NE_mem_2>0){
// 	axi_indices2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].indices2);
// 	//
// 	axi_weights2 = reinterpret_cast<ap_uint<CSRWIDTHS>*>(buff_host_prune[0].weights2);
// }
// ap_uint<DWIDTHS>*     axi_cidPrev       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidPrev);
// ap_uint<DWIDTHS>*     axi_cidCurr       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidCurr);
// ap_uint<DWIDTHS>*     axi_cidSizePrev   = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizePrev);
// ap_uint<DWIDTHS>*     axi_totPrev       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totPrev);
// ap_uint<DWIDTHS>*     axi_cidSizeCurr   = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizeCurr);
// ap_uint<DWIDTHS>*     axi_totCurr       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totCurr);
// ap_uint<DWIDTHS>*     axi_cidSizeUpdate = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cidSizeUpdate);
// ap_uint<DWIDTHS>*     axi_totUpdate     = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].totUpdate);
// ap_uint<DWIDTHS>*     axi_cWeight       = reinterpret_cast<ap_uint<DWIDTHS>*>(buff_host_prune[0].cWeight);
// ap_uint<COLORWIDTHS>* axi_colorAxi      = reinterpret_cast<ap_uint<COLORWIDTHS>*>(buff_host_prune[0].colorAxi);
// ap_uint<COLORWIDTHS>* axi_colorInx      = reinterpret_cast<ap_uint<COLORWIDTHS>*>(buff_host_prune[0].colorInx);
// uint8_t* axi_flag 					= reinterpret_cast<uint8_t*>(buff_host_prune[0].flag);
// uint8_t* axi_flagUpdate 				= reinterpret_cast<uint8_t*>(buff_host_prune[0].flagUpdate);
#ifdef PRINTINFO
        std::cout << "INFO: start mext_in " << std::endl;
#endif
        // DDR Settings
        mext_in[0] = {(unsigned int)(20) | XCL_MEM_TOPOLOGY, buff_host_prune[0].config0, 0};
        mext_in[1] = {(unsigned int)(20) | XCL_MEM_TOPOLOGY, buff_host_prune[0].config1, 0};
        mext_in[2] = {(unsigned int)(20) | XCL_MEM_TOPOLOGY, buff_host_prune[0].offsets, 0};
        mext_in[3] = {(unsigned int)(16) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indices, 0};
        mext_in[4] = {(unsigned int)(18) | XCL_MEM_TOPOLOGY, buff_host_prune[0].weights, 0};
        if (NE_mem_2 > 0) {
            mext_in[3 + 18] = {(unsigned int)(17) | XCL_MEM_TOPOLOGY, buff_host_prune[0].indices2, 0};
            mext_in[4 + 18] = {(unsigned int)(19) | XCL_MEM_TOPOLOGY, buff_host_prune[0].weights2, 0};
        }
        mext_in[5] = {(unsigned int)(21) | XCL_MEM_TOPOLOGY, buff_host_prune[0].colorAxi, 0};
        mext_in[6] = {(unsigned int)(22) | XCL_MEM_TOPOLOGY, buff_host_prune[0].colorInx, 0};
        mext_in[7] = {(unsigned int)(23) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidPrev, 0};
        mext_in[8] = {(unsigned int)(24) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizePrev, 0};
        mext_in[9] = {(unsigned int)(25) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totPrev, 0};
        mext_in[10] = {(unsigned int)(26) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidCurr, 0};
        mext_in[11] = {(unsigned int)(27) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizeCurr, 0};
        mext_in[12] = {(unsigned int)(28) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totCurr, 0};
        mext_in[13] = {(unsigned int)(29) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cidSizeUpdate, 0};
        mext_in[14] = {(unsigned int)(30) | XCL_MEM_TOPOLOGY, buff_host_prune[0].totUpdate, 0};
        mext_in[15] = {(unsigned int)(31) | XCL_MEM_TOPOLOGY, buff_host_prune[0].cWeight, 0};

        mext_in[18] = {(unsigned int)(21) | XCL_MEM_TOPOLOGY, buff_host_prune[0].flag, 0};
        mext_in[19] = {(unsigned int)(21) | XCL_MEM_TOPOLOGY, buff_host_prune[0].flagUpdate, 0};

        int flag_RW = CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE;
        int flag_RD = CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY;

        hds[0].buffer[0] = cl::Buffer(hds[0].context, flag_RW, sizeof(int64_t) * (6), &mext_in[0]);
        hds[0].buffer[1] = cl::Buffer(hds[0].context, flag_RW, sizeof(DWEIGHT) * (4), &mext_in[1]);
        hds[0].buffer[2] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV + 1), &mext_in[2]);
        hds[0].buffer[3] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_1, &mext_in[3]);
        hds[0].buffer[4] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_1, &mext_in[4]);
        hds[0].buffer[5] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * (NV), &mext_in[5]);
        hds[0].buffer[6] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[6]);
        hds[0].buffer[7] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[7]);
        hds[0].buffer[8] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[8]);
        hds[0].buffer[9] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[9]);
        hds[0].buffer[10] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[10]);
        hds[0].buffer[11] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[11]);
        hds[0].buffer[12] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[12]);
        hds[0].buffer[13] = cl::Buffer(hds[0].context, flag_RW, sizeof(int) * (NV), &mext_in[13]);
        hds[0].buffer[14] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[14]);
        hds[0].buffer[15] = cl::Buffer(hds[0].context, flag_RW, sizeof(float) * (NV), &mext_in[15]);
        ////offset
        //
        hds[0].buffer[18] = cl::Buffer(hds[0].context, flag_RW, sizeof(uint8_t) * (NV), &mext_in[18]);
        hds[0].buffer[19] = cl::Buffer(hds[0].context, flag_RW, sizeof(uint8_t) * (NV), &mext_in[19]);
        if (NE_mem_2 > 0) {
#ifdef PRINTINFO
            std::cout << "INFO: NE_mem_2 =  " << NE_mem_2 << std::endl;
#endif
            hds[0].buffer[21] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[21]);
            hds[0].buffer[22] = cl::Buffer(hds[0].context, flag_RD, sizeof(int) * NE_mem_2, &mext_in[22]);
        }
#ifdef PRINTINFO
        std::cout << "INFO: init buffer done!  " << std::endl;
#endif
    }
}

void opLouvainModularity::PhaseLoop_UsingFPGA_1_KernelSetup_prune_2cu(bool isLargeEdge,
                                                                      cl::Kernel& kernel_run,
                                                                      std::vector<cl::Memory>& ob_in,
                                                                      std::vector<cl::Memory>& ob_out,
                                                                      clHandle* hds) {
#ifdef PRINTINFO
    std::cout << "INFO: start 2cu kernel setup " << std::endl;
#endif

    cl::Kernel kernel_louvain = hds[0].kernel;

    // Data transfer from host buffer to device buffer
    ob_in.push_back(hds[0].buffer[0]);
    ob_in.push_back(hds[0].buffer[1]);
    ob_in.push_back(hds[0].buffer[2]);
    ob_in.push_back(hds[0].buffer[3]);
    if (isLargeEdge) ob_in.push_back(hds[0].buffer[21]);
    ob_in.push_back(hds[0].buffer[4]);
    if (isLargeEdge) ob_in.push_back(hds[0].buffer[22]);
    ob_in.push_back(hds[0].buffer[5]); // axi_colorAxi
    ob_in.push_back(hds[0].buffer[6]); // axi_cidPrev
    ob_in.push_back(hds[0].buffer[7]);
    ob_in.push_back(hds[0].buffer[8]);
    ob_in.push_back(hds[0].buffer[9]);
    ob_in.push_back(hds[0].buffer[10]);
    ob_in.push_back(hds[0].buffer[11]);
    ob_in.push_back(hds[0].buffer[12]);
    ob_in.push_back(hds[0].buffer[13]);
    ob_in.push_back(hds[0].buffer[14]);
    ob_in.push_back(hds[0].buffer[15]);
    // ob_in.push_back(hds[0].buffer[16]);
    // ob_in.push_back(hds[0].buffer[17]);
    ob_in.push_back(hds[0].buffer[18]);
    ob_in.push_back(hds[0].buffer[19]);

    ob_out.push_back(hds[0].buffer[0]);
    ob_out.push_back(hds[0].buffer[1]);
    ob_out.push_back(hds[0].buffer[7]);

    kernel_louvain.setArg(0, hds[0].buffer[0]);   // config0
    kernel_louvain.setArg(1, hds[0].buffer[1]);   // config1
    kernel_louvain.setArg(2, hds[0].buffer[2]);   // offsets
    kernel_louvain.setArg(3, hds[0].buffer[3]);   // indices
    kernel_louvain.setArg(4, hds[0].buffer[4]);   // weights
    kernel_louvain.setArg(5, hds[0].buffer[5]);   // colorAxi
    kernel_louvain.setArg(6, hds[0].buffer[6]);   // colorInx
    kernel_louvain.setArg(7, hds[0].buffer[7]);   // cidPrev
    kernel_louvain.setArg(8, hds[0].buffer[8]);   // cidSizePrev
    kernel_louvain.setArg(9, hds[0].buffer[9]);   // totPrev
    kernel_louvain.setArg(10, hds[0].buffer[10]); // cidCurr
    kernel_louvain.setArg(11, hds[0].buffer[11]); // cidSizeCurr
    kernel_louvain.setArg(12, hds[0].buffer[12]); // totCurr
    kernel_louvain.setArg(13, hds[0].buffer[13]); // cUpdate
    kernel_louvain.setArg(14, hds[0].buffer[14]); // totCurr
    kernel_louvain.setArg(15, hds[0].buffer[15]); // cWeight
    kernel_louvain.setArg(16, hds[0].buffer[2]);  // offsets
    kernel_louvain.setArg(17, hds[0].buffer[3]);  // indices
    kernel_louvain.setArg(18, hds[0].buffer[18]); // flag
    kernel_louvain.setArg(19, hds[0].buffer[19]); // db_flagUpdate
#ifdef PRINTINFO
    std::cout << "INFO: Finish kernel setup" << std::endl;
#endif
}

double opLouvainModularity::PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune(long vertexNum,
                                                                          KMemorys_host_prune* buff_host,
                                                                          int* eachItrs,
                                                                          // output
                                                                          long* C,
                                                                          int* eachItr,
                                                                          double* currMod) {
    double time1 = omp_get_wtime();
    // updating
    eachItrs[0] = buff_host[0].config0[2];
    eachItr[0] = buff_host[0].config0[2];
    currMod[0] = buff_host[0].config1[1];

    for (int i = 0; i < vertexNum; i++) {
        C[i] = (long)buff_host[0].cidPrev[i];
    }
#ifdef PRINTINFO_LVPHASE
    std::cout << " read INFO: eachItrs" << eachItrs[0] << ", "
              << "eachItr[0] = " << eachItr[0] << ", "
              << "currMod[0] = " << currMod[0] << std::endl;
#endif
    time1 = omp_get_wtime() - time1;
    return time1;
}

double opLouvainModularity::PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune_renumber(long vertexNum,
                                                                                   KMemorys_host_prune* buff_host,
                                                                                   int* eachItrs,
                                                                                   // output
                                                                                   long* C,
                                                                                   int* eachItr,
                                                                                   double* currMod,
                                                                                   long* numClusters) {
    double time1 = omp_get_wtime();
    // updating
    eachItrs[0] = buff_host[0].config0[2];
    eachItr[0] = buff_host[0].config0[2];
    currMod[0] = buff_host[0].config1[1];
    numClusters[0] = buff_host[0].config0[4];

    for (int i = 0; i < vertexNum; i++) {
        C[i] = (long)buff_host[0].cidPrev[i];
    }
#ifdef PRINTINFO_LVPHASE
    std::cout << " read INFO: eachItrs" << eachItrs[0] << ", "
              << "eachItr[0] = " << eachItr[0] << ", "
              << "currMod[0] = " << currMod[0] << std::endl;
#endif
    time1 = omp_get_wtime() - time1;
    return time1;
}

void opLouvainModularity::postProcess(){};

void opLouvainModularity::PhaseLoop_UpdatingC_org(int phase, long NV, long NV_G, long* C, long* C_orig) {
    if (phase == 1) {
#pragma omp parallel for
        for (long i = 0; i < NV; i++) {
            C_orig[i] = C[i]; // After the first phase
        }
    } else {
#pragma omp parallel for
        for (long i = 0; i < NV; i++) {
            assert(C_orig[i] < NV_G);
            if (C_orig[i] >= 0) C_orig[i] = C[C_orig[i]]; // Each cluster in a previous phase becomes a vertex
        }
    }
}

long* opLouvainModularity::CreateM(long NV_new, long NV_orig, long* C_orig, long* M_orig) {
    long* M = (long*)malloc(NV_new * sizeof(long));
    assert(M);
    memset(M, 0, NV_new * sizeof(long));
    for (int i = 0; i < NV_orig; i++) {
        if (M_orig[i] < 0) M[C_orig[i]] = M_orig[i];
    }
    return M;
}

double opLouvainModularity::PhaseLoop_CommPostProcessing_par(GLV* pglv_orig,
                                                             GLV* pglv_iter,
                                                             int numThreads,
                                                             double opts_threshold,
                                                             bool opts_coloring,
                                                             // modified:
                                                             bool& nonColor,
                                                             int& phase,
                                                             int& totItr,
                                                             long& numClusters,
                                                             double& totTimeBuildingPhase,
                                                             double& time_renum,
                                                             double& time_C,
                                                             double& time_M,
                                                             double& time_buid,
                                                             double& time_set) {
    double time1 = 0;
    time1 = omp_get_wtime();
    time_renum = time1;
    graphNew* Gnew;
    numClusters = renumberClustersContiguously_ghost(pglv_iter->C, pglv_iter->G->numVertices, pglv_iter->NVl);
#ifdef PRINTINFO_LVPHASE
    printf("Number of unique clusters: %ld\n", numClusters);
#endif
    time_renum = omp_get_wtime() - time_renum;

    time_C = omp_get_wtime();
    PhaseLoop_UpdatingC_org(phase, pglv_orig->NV, pglv_iter->NV, pglv_iter->C, pglv_orig->C);
    time_C = omp_get_wtime() - time_C;

    time_M = omp_get_wtime();
    long* M_new = CreateM(numClusters, pglv_orig->NV, pglv_orig->C, pglv_orig->M);
    time_M = omp_get_wtime() - time_M;

    Gnew = (graphNew*)malloc(sizeof(graphNew));
    assert(Gnew != 0);
    double tmpTime = buildNextLevelGraphOpt(pglv_iter->G, Gnew, pglv_iter->C, numClusters, 8);
    totTimeBuildingPhase += tmpTime;
    time_buid = tmpTime;

    time_set = omp_get_wtime();
    pglv_iter->SetByOhterG(Gnew, M_new);
    time_set = omp_get_wtime() - time_set;

    time1 = omp_get_wtime() - time1;

    return time1;
}
double opLouvainModularity::PhaseLoop_CommPostProcessing_par_renumber(GLV* pglv_orig,
                                                                      GLV* pglv_iter,
                                                                      int numThreads,
                                                                      double opts_threshold,
                                                                      bool opts_coloring,
                                                                      // modified:
                                                                      bool& nonColor,
                                                                      int& phase,
                                                                      int& totItr,
                                                                      long& numClusters,
                                                                      double& totTimeBuildingPhase,
                                                                      double& time_renum,
                                                                      double& time_C,
                                                                      double& time_M,
                                                                      double& time_buid,
                                                                      double& time_set) {
    double time1 = 0;
    time1 = omp_get_wtime();
    time_renum = time1;
    graphNew* Gnew;
// numClusters = renumberClustersContiguously_ghost(pglv_iter->C, pglv_iter->G->numVertices, pglv_iter->NVl);
#ifdef PRINTINFO_LVPHASE
    printf("Number of unique clusters renumber: %ld\n", numClusters);
#endif
    time_renum = omp_get_wtime() - time_renum;

    time_C = omp_get_wtime();
    PhaseLoop_UpdatingC_org(phase, pglv_orig->NV, pglv_iter->NV, pglv_iter->C, pglv_orig->C);
    time_C = omp_get_wtime() - time_C;

    time_M = omp_get_wtime();
    long* M_new = CreateM(numClusters, pglv_orig->NV, pglv_orig->C, pglv_orig->M);
    time_M = omp_get_wtime() - time_M;

    Gnew = (graphNew*)malloc(sizeof(graphNew));
    assert(Gnew != 0);
    double tmpTime = buildNextLevelGraphOpt(pglv_iter->G, Gnew, pglv_iter->C, numClusters, 8);
    totTimeBuildingPhase += tmpTime;
    time_buid = tmpTime;

    time_set = omp_get_wtime();
    pglv_iter->SetByOhterG(Gnew, M_new);
    time_set = omp_get_wtime() - time_set;

    time1 = omp_get_wtime() - time1;

    return time1;
}

void opLouvainModularity::demo_par_core(
    int id_dev, int kernelMode, GLV* pglv_orig, GLV* pglv_iter, LouvainPara* para_lv) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << " id_dev=" << id_dev << std::endl;
#endif
    bool opts_coloring = para_lv->opts_coloring;
    long opts_minGraphSize = para_lv->opts_minGraphSize;
    double opts_threshold = para_lv->opts_threshold;
    double opts_C_thresh = para_lv->opts_C_thresh;
    int numThreads = para_lv->numThreads;
    int max_num_level = para_lv->max_num_level;
    int max_num_iter = para_lv->max_num_iter;
    pglv_orig->times.totTimeAll = omp_get_wtime();

    pglv_orig->times.phase = 1;  // Total phase counter
    pglv_orig->times.totItr = 0; // Total iteration counter

    pglv_orig->times.totTimeInitBuff = 0;
    pglv_orig->times.totTimeReadBuff = 0;
    pglv_orig->times.totTimeReGraph = 0;
    pglv_orig->times.totTimeE2E_2 = 0;
    pglv_orig->times.totTimeE2E = 0;

    pglv_orig->times.totTimeBuildingPhase = 0;
    pglv_orig->times.totTimeClustering = 0;
    pglv_orig->times.totTimeColoring = 0;
    pglv_orig->times.totTimeFeature = 0;

    pglv_orig->times.timePrePre_dev = 0;
    pglv_orig->times.timePrePre_xclbin = 0;
    pglv_orig->times.timePrePre_buff = 0;
    pglv_orig->times.timePrePre = omp_get_wtime();
    // double time2  = omp_get_wtime();
    long NV_orig = pglv_orig->G->numVertices;
    long NE_orig = pglv_orig->G->numEdges;
    long numClusters;

    assert(NV_orig < glb_MAXNV);
    assert(NE_orig < glb_MAXNE);

    long NE_mem = NE_orig * 2; // number for real edge to be stored in memory
    long NE_mem_1 = NE_mem < (glb_MAXNV) ? NE_mem : (glb_MAXNV);
    long NE_mem_2 = NE_mem - NE_mem_1;

    double prevMod = -1;       // Last-phase modularity
    double mod = pglv_orig->Q; // Current modularity
    double* currMod = &mod;

    bool nonColor = false; // Make sure that at least one phase with lower opts_threshold runs
    bool isItrStop = false;

    pglv_orig->times.timePrePre = omp_get_wtime() - pglv_orig->times.timePrePre;

    while (!isItrStop) {
        int phase_tmp = pglv_orig->times.phase;
        pglv_orig->times.eachTimePhase[pglv_orig->times.phase - 1] = omp_get_wtime();
#ifdef PRINTINFO
        printf("===============================\n");
        printf("Phase Begin%d\n", pglv_orig->times.phase);
        printf("===============================\n");
#endif

        {
            pglv_orig->times.eachTimeE2E_2[pglv_orig->times.phase - 1] = omp_get_wtime();

#ifdef PRINTINFO
            printf("DEBUG: addwork to FPGA task queue\n");
#endif
            auto ev =
                addwork(pglv_iter, kernelMode, opts_C_thresh, &pglv_orig->times.eachItrs[pglv_orig->times.phase - 1],
                        currMod, &numClusters, &pglv_orig->times.eachTimeInitBuff[pglv_orig->times.phase - 1],
                        &pglv_orig->times.eachTimeReadBuff[pglv_orig->times.phase - 1]);
            ev.wait();

            pglv_orig->times.eachTimeE2E[pglv_orig->times.phase - 1] = pglv_iter->times.eachTimeE2E[0];
            pglv_orig->times.deviceID[pglv_orig->times.phase - 1] = pglv_iter->times.deviceID[0];
            pglv_orig->times.cuID[pglv_orig->times.phase - 1] = pglv_iter->times.cuID[0];
            pglv_orig->times.channelID[pglv_orig->times.phase - 1] = pglv_iter->times.channelID[0];
            pglv_orig->times.eachTimeE2E_2[pglv_orig->times.phase - 1] =
                omp_get_wtime() - pglv_orig->times.eachTimeE2E_2[pglv_orig->times.phase - 1];

            pglv_orig->times.totTimeInitBuff += pglv_orig->times.eachTimeInitBuff[pglv_orig->times.phase - 1];
            pglv_orig->times.totTimeReadBuff += pglv_orig->times.eachTimeReadBuff[pglv_orig->times.phase - 1];
            pglv_orig->times.totTimeE2E_2 += pglv_orig->times.eachTimeE2E_2[pglv_orig->times.phase - 1];
            pglv_orig->times.totTimeE2E += pglv_orig->times.eachTimeE2E[pglv_orig->times.phase - 1];
            pglv_orig->times.totItr += pglv_orig->times.eachItrs[pglv_orig->times.phase - 1];
        }
        if (kernelMode == LOUVAINMOD_PRUNING_KERNEL) {
            pglv_orig->times.eachTimeReGraph[pglv_orig->times.phase - 1] = PhaseLoop_CommPostProcessing_par(
                pglv_orig, pglv_iter, numThreads, opts_threshold, opts_coloring, nonColor, pglv_orig->times.phase,
                pglv_orig->times.totItr, numClusters, pglv_orig->times.totTimeBuildingPhase,
                pglv_orig->times.eachNum[pglv_orig->times.phase - 1],
                pglv_orig->times.eachC[pglv_orig->times.phase - 1], pglv_orig->times.eachM[pglv_orig->times.phase - 1],
                pglv_orig->times.eachBuild[pglv_orig->times.phase - 1],
                pglv_orig->times.eachSet[pglv_orig->times.phase - 1]);
        } else {
            pglv_orig->times.eachTimeReGraph[pglv_orig->times.phase - 1] = PhaseLoop_CommPostProcessing_par_renumber(
                pglv_orig, pglv_iter, numThreads, opts_threshold, opts_coloring, nonColor, pglv_orig->times.phase,
                pglv_orig->times.totItr, numClusters, pglv_orig->times.totTimeBuildingPhase,
                pglv_orig->times.eachNum[pglv_orig->times.phase - 1],
                pglv_orig->times.eachC[pglv_orig->times.phase - 1], pglv_orig->times.eachM[pglv_orig->times.phase - 1],
                pglv_orig->times.eachBuild[pglv_orig->times.phase - 1],
                pglv_orig->times.eachSet[pglv_orig->times.phase - 1]);
        }

        pglv_orig->times.eachClusters[pglv_orig->times.phase - 1] = numClusters;
        pglv_orig->times.eachMod[pglv_orig->times.phase - 1] = currMod[0];
        pglv_orig->times.totTimeReGraph += pglv_orig->times.eachTimeReGraph[pglv_orig->times.phase - 1];
        pglv_orig->NC = numClusters;
        pglv_iter->NC = numClusters;
#ifdef PRINTINFO
        printf("INFO: numClusters=%ld \n", numClusters);
#endif
        pglv_orig->Q = currMod[0];
        pglv_iter->Q = currMod[0];
        pglv_orig->times.eachTimeFeature[pglv_orig->times.phase - 1] = omp_get_wtime();
        pglv_orig->times.eachTimeFeature[pglv_orig->times.phase - 1] =
            omp_get_wtime() - pglv_orig->times.eachTimeFeature[pglv_orig->times.phase - 1];
        pglv_orig->times.totTimeFeature += pglv_orig->times.eachTimeFeature[pglv_orig->times.phase - 1];

        if ((pglv_orig->times.phase >= max_num_level) || (pglv_orig->times.totItr >= max_num_iter)) {
            isItrStop = true; // Break if too many phases or iterations
        } else if ((currMod[0] - prevMod) <= opts_threshold) {
            isItrStop = true;
        } else if (pglv_iter->NV <= opts_minGraphSize) {
            isItrStop = true;
        } else {
            isItrStop = false;
            pglv_orig->times.phase++;
        }
        prevMod = currMod[0];
        /*if (isItrStop == false)
                pglv_orig->times.eachTimePhase[pglv_orig->times.phase - 2] = omp_get_wtime() -
        pglv_orig->times.eachTimePhase[pglv_orig->times.phase - 2];
        else
                pglv_orig->times.eachTimePhase[pglv_orig->times.phase - 1] = omp_get_wtime() -
        pglv_orig->times.eachTimePhase[pglv_orig->times.phase - 1];*/
        pglv_orig->times.eachTimePhase[phase_tmp - 1] = omp_get_wtime() - pglv_orig->times.eachTimePhase[phase_tmp - 1];
#ifdef PRINTINFO
        printf("=======================================================\n");
        printf("Par:%d     Phase:%d  Done! at Device:%d       Phase time =%lf      E2E_wait=%lf     E2E_pure=%lf \n",
               pglv_orig->times.parNo, phase_tmp, pglv_orig->times.deviceID[phase_tmp - 1],
               pglv_orig->times.eachTimePhase[phase_tmp - 1], pglv_orig->times.eachTimeE2E_2[phase_tmp - 1],
               pglv_orig->times.eachTimeE2E[phase_tmp - 1]);
        printf("=====================================================\n");
#endif
    } // End of while(1) = End of Louvain
    pglv_orig->times.totTimeAll = omp_get_wtime() - pglv_orig->times.totTimeAll;
    double timePostPost;
    double timePostPost_feature;
    timePostPost_feature = omp_get_wtime();
    timePostPost = omp_get_wtime();
#ifdef PRINTINFO
    printf("********************************************\n");
    printf("*********    Compact Summary   *************\n");
    printf("********************************************\n");
    printf("Number of threads              : %d\n", numThreads);
    printf("Total number of phases         : %d\n", pglv_orig->times.phase);
    printf("Total number of iterations     : %d\t\t = ", pglv_orig->times.totItr);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf(" + %8d  ", pglv_orig->times.eachItrs[i]);
    printf("\n");
    printf("Final number of clusters       : %ld\t\t : ", numClusters);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("   %8d  ", pglv_orig->times.eachClusters[i]);
    printf("\n");
    printf("Final modularity               : %lf : \t", prevMod);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("  %2.6f  ", pglv_orig->times.eachMod[i]);
    printf("\n");
    printf("Total time for clustering      : %lf\n", pglv_orig->times.totTimeClustering);
    printf("Total time for building phases : %lf\n", pglv_orig->times.totTimeBuildingPhase);
    printf("Total E2E time(s)              : %lf = \t", pglv_orig->times.totTimeE2E);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimeE2E_2[i]);
    printf("\n");
    if (opts_coloring) {
        printf("Total time for coloring        : %lf\n", pglv_orig->times.totTimeColoring);
    }
    printf("********************************************\n");
    printf("TOTAL TIME                     : %lf\n",
           pglv_orig->times.totTimeE2E_2 + pglv_orig->times.totTimeClustering + pglv_orig->times.totTimeBuildingPhase +
               pglv_orig->times.totTimeColoring);
    printf("********************************************\n");
    printf("Total time for Init buff_host  : %lf = \t", pglv_orig->times.totTimeInitBuff);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimeInitBuff[i]);
    printf("\n");
    printf("Total time for Read buff_host  : %lf = \t", pglv_orig->times.totTimeReadBuff);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimeReadBuff[i]);
    printf("\n");
    printf("Total time for totTimeE2E_2    : %lf = \t", pglv_orig->times.totTimeE2E_2);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimeE2E_2[i]);
    printf("\n");

    printf("Total time for totTimeReGraph  : %lf = \t", pglv_orig->times.totTimeReGraph);
    for (int i = 0; i < pglv_orig->times.phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimeReGraph[i]);
    printf("\n");
#endif

    double ToTeachNum = 0;   //        [MAX_NUM_PHASE];
    double ToTeachC = 0;     //        [MAX_NUM_PHASE];
    double ToTeachM = 0;     //        [MAX_NUM_PHASE];
    double ToTeachBuild = 0; //        [MAX_NUM_PHASE];
    double ToTeachSet = 0;   //         [MAX_NUM_PHASE];

    for (int i = 0; i < pglv_orig->times.phase; i++) {
        ToTeachNum += pglv_orig->times.eachNum[i];     //        [MAX_NUM_PHASE];
        ToTeachC += pglv_orig->times.eachC[i];         //        [MAX_NUM_PHASE];
        ToTeachM += pglv_orig->times.eachM[i];         //        [MAX_NUM_PHASE];
        ToTeachBuild += pglv_orig->times.eachBuild[i]; //        [MAX_NUM_PHASE];
        ToTeachSet += pglv_orig->times.eachSet[i];
    }
    int phase = pglv_orig->times.phase;
#ifdef PRINTINFO
    printf("----- time for ToTeachNum      : %lf = \t", ToTeachNum);
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachNum[i]);
    printf("\n");
    printf("----- time for ToTeachC        : %lf = \t", ToTeachC);
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachC[i]);
    printf("\n");
    printf("----- time for ToTeachM        : %lf = \t", ToTeachM);
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachM[i]);
    printf("\n");
    printf("----- time for ToTeachBuild    : %lf = \t", ToTeachBuild);
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachBuild[i]);
    printf("\n");
    printf("----- time for ToTeachSet      : %lf = \t", ToTeachSet);
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachSet[i]);
    printf("\n");

    printf("Total time for totTimeFeature  : %lf = \t", pglv_orig->times.totTimeFeature);
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimeFeature[i]);
    printf("\n");
    printf("********************************************\n");
    printf("TOTAL TIME2                    : %lf = \t", pglv_orig->times.totTimeAll); // eachTimePhase
    for (int i = 0; i < phase; i++) printf("+ %2.6f  ", pglv_orig->times.eachTimePhase[i]);
    printf("\n");
    printf("********************************************\n");
#endif
    // buff_host.freeMem();
    // pglv_orig->PushFeature(phase, pglv_orig->times.totItr, pglv_orig->times.totTimeE2E_2, true); // isUsingFPGA);
    // pglv_iter->PushFeature(phase, pglv_orig->times.totItr, pglv_orig->times.totTimeE2E_2, true); // isUsingFPGA);
    timePostPost = omp_get_wtime() - timePostPost;
    timePostPost_feature = omp_get_wtime() - timePostPost_feature;
#ifdef PRINTINFO
    printf("TOTAL PrePre                   : %lf = %f(dev) + %lf(bin) + %lf(buff) +%lf\n", pglv_orig->times.timePrePre,
           pglv_orig->times.timePrePre_dev, pglv_orig->times.timePrePre_xclbin, pglv_orig->times.timePrePre_buff,
           pglv_orig->times.timePrePre - pglv_orig->times.timePrePre_dev - pglv_orig->times.timePrePre_xclbin -
               pglv_orig->times.timePrePre_buff); // eachTimePhase
    printf("TOTAL PostPost                 : %lf = %lf + %lf\n", timePostPost, timePostPost_feature,
           timePostPost - timePostPost_feature); // eachTimePhase
#endif
}

event<int> opLouvainModularity::addwork(GLV* glv,
                                        int kernelMode,
                                        double opts_C_thresh,
                                        int* eachItrs,
                                        double* currMod,
                                        long* numClusters,
                                        double* eachTimeInitBuff,
                                        double* eachTimeReadBuff) {
    return createL3(task_queue[0], &(compute), handles, kernelMode, numBuffers_, glv, opts_C_thresh,
                    /*buff_hosts,*/ buff_hosts_prune, eachItrs, currMod, numClusters, eachTimeInitBuff,
                    eachTimeReadBuff);
};

} // L3
} // graph
} // xf
