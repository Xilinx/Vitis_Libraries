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

#ifndef _XF_GRAPH_L3_OP_LOUVAINMODULARITY_HPP_
#define _XF_GRAPH_L3_OP_LOUVAINMODULARITY_HPP_

#include "graph.hpp"
#include "op_base.hpp"
#include "openclHandle.hpp"
#include "xilinxlouvainInternal.hpp"
#include <time.h>

namespace xf {
namespace graph {
namespace L3 {

class opLouvainModularity : public opBase {
   public:
    static uint32_t cuPerBoardLouvainModularity;

    static uint32_t dupNmLouvainModularity;

    std::thread louvainModularityThread;

    std::vector<event<int> > eventQueue;

    class clHandle* handles;

    // KMemorys_host* buff_hosts;

    KMemorys_host_prune* buff_hosts_prune;

    opLouvainModularity() : opBase(){};

    void setHWInfo(uint32_t numDev, uint32_t CUmax);

    void freeLouvainModularity(xrmContext* ctx);

    void createHandle(class openXRM* xrm,
                      clHandle& handle,
                      std::string kernelName,
                      std::string kernelAlias,
                      std::string xclbinFile,
                      int32_t IDDevice,
                      unsigned int requestLoad);

    void init(class openXRM* xrm,
              std::string kernelName,
              std::string kernelAlias,
              std::string xclbinFile,
              uint32_t* deviceIDs,
              uint32_t* cuIDs,
              unsigned int requestLoad);

    void mapHostToClBuffers(
        graphNew* G, int flowMode, bool opts_coloring, long opts_minGraphSize, double opts_C_thresh, int numThreads);

    static int compute(unsigned int deviceID,
                       unsigned int cuID,
                       unsigned int channelID,
                       xrmContext* ctx,
                       xrmCuResource* resR,
                       std::string instanceName,
                       clHandle* handles,
                       int flowMode,
                       uint32_t numBuffers,
                       GLV* pglv_iter,
                       double opts_C_thresh,
                       /*KMemorys_host* buff_host,*/
                       KMemorys_host_prune* buff_host_prune,
                       int* eachItrs,
                       double* currMod,
                       long* numClusters,
                       double* eachTimeInitBuff,
                       double* eachTimeReadBuff);

    void demo_par_core(int id_dev, int flowMode, GLV* pglv_orig, GLV* pglv_iter, LouvainPara* para_lv);

    event<int> addwork(GLV* glv,
                       int flowMode,
                       double opts_C_thresh,
                       int* eachItrs,
                       double* currMod,
                       long* numClusters,
                       double* eachTimeInitBuff,
                       double* eachTimeReadBuff);

   private:
    std::vector<int> deviceOffset;

    uint32_t deviceNm;

    uint32_t maxCU;

    uint32_t numBuffers_ = 23;

    // static void bufferInit(clHandle* hds, long NV, long NE_mem_1, long NE_mem_2, KMemorys_host* buff_host);
    static double PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune(int numColors,
                                                                graphNew* G,
                                                                long* M,
                                                                double opts_C_thresh,
                                                                double* currMod,
                                                                // Updated variables
                                                                int* colors,
                                                                KMemorys_host_prune* buff_host);
    static double PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune_renumber_2cu(int numColors,
                                                                             long NVl,
                                                                             graphNew* G,
                                                                             long* M,
                                                                             double opts_C_thresh,
                                                                             double* currMod,
                                                                             // Updated variables
                                                                             int* colors,
                                                                             KMemorys_host_prune* buff_host);

    static void UsingFPGA_MapHostClBuff_prune(
        clHandle* hds, long NV, long NE_mem_1, long NE_mem_2, KMemorys_host_prune* buff_hosts_prune);

    static int cuExecute(
        clHandle* hds, cl::Kernel& kernel0, unsigned int num_runs, std::vector<cl::Event>* evIn, cl::Event* evOut);

    static void migrateMemObj(clHandle* hds,
                              bool type,
                              unsigned int num_runs,
                              std::vector<cl::Memory>& ob,
                              std::vector<cl::Event>* evIn,
                              cl::Event* evOut);

    static void releaseMemObjects(clHandle* hds, uint32_t numBuffer);

    static void postProcess();

    // static void PhaseLoop_UsingFPGA_1_KernelSetup(bool isLargeEdge,
    //                                               cl::Kernel& kernel_louvain,
    //                                               std::vector<cl::Memory>& ob_in,
    //                                               std::vector<cl::Memory>& ob_out,
    //                                               clHandle* hds);
    static void PhaseLoop_UsingFPGA_1_KernelSetup_prune(bool isLargeEdge,
                                                        cl::Kernel& kernel_louvain,
                                                        std::vector<cl::Memory>& ob_in,
                                                        std::vector<cl::Memory>& ob_out,
                                                        clHandle* hds);

    static void UsingFPGA_MapHostClBuff_prune_2cu(
        clHandle* hds, long NV, long NE_mem_1, long NE_mem_2, KMemorys_host_prune* buff_hosts_prune);

    static void PhaseLoop_UsingFPGA_1_KernelSetup_prune_2cu(bool isLargeEdge,
                                                            cl::Kernel& kernel_louvain,
                                                            std::vector<cl::Memory>& ob_in,
                                                            std::vector<cl::Memory>& ob_out,
                                                            clHandle* hds);
    static double PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune(long vertexNum,
                                                                KMemorys_host_prune* buff_host,
                                                                int* eachItrs,
                                                                // output
                                                                long* C,
                                                                int* eachItr,
                                                                double* currMod);

    static double PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune_renumber(long vertexNum,
                                                                         KMemorys_host_prune* buff_host,
                                                                         int* eachItrs,
                                                                         // output
                                                                         long* C,
                                                                         int* eachItr,
                                                                         double* currMod,
                                                                         long* numClusters);

    static void PhaseLoop_UpdatingC_org(int phase, long NV, long NV_G, long* C, long* C_orig);

    long* CreateM(long NV_new, long NV_orig, long* C_orig, long* M_orig);

    double PhaseLoop_CommPostProcessing_par(GLV* pglv_orig,
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
                                            double& time_set);

    double PhaseLoop_CommPostProcessing_par_renumber(GLV* pglv_orig,
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
                                                     double& time_set);
};

} // L3
} // graph
} // xf

class myCmd {
   public:
    int argc;
    char argv[64][256];
    // char line_last[1024];
    int cmd_last;
    string line_last;
    int cnt_his;
    list<string> his_cmd;
    myCmd(); //{cnt_his=0; argc=0; cmd_last=-1;};
    const char* cmd_SkipSpace(const char* str);
    int cmd_CpyWord(char* des, const char* src);
    int cmd_Getline();
    int cmd_Getline(const char* str);
    // int cmd_GetCmd();
    // int cmd_GetCmd(const char* str);
    void cmd_resetArg();
    int cmd_findPara(const char* para);
    // int PrintHistory();
};

//#define NUM_CMD (28)

float loadAlveoAndComputeLouvain(char* xclbinPath,
                                 int kernelMode,
                                 unsigned int numDevices,
                                 std::string deviceNames,
                                 char* alveoProject,
                                 unsigned mode_zmq,
                                 unsigned numPureWorker,
                                 char* nameWorkers[128],
                                 unsigned int nodeID,
                                 char* opts_outputFile,
                                 unsigned int max_iter,
                                 unsigned int max_level,
                                 float tolerance,
                                 bool intermediateResult,
                                 bool verbose,
                                 bool final_Q,
                                 bool all_Q);

#endif
