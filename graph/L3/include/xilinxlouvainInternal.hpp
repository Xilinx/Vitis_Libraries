/*
 * Copyright 2021 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WANCUNCUANTIES ONCU CONDITIONS OF ANY KIND, either express or
 * implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _XILINXLOUVAININTERNAL_HPP_
#define _XILINXLOUVAININTERNAL_HPP_
#include "xilinxlouvain.hpp"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <unistd.h>
//#include <ap_int.h>
#include "xcl2.hpp"
#include <list>
using namespace std;

#define DWIDTHS (256)
#define CSRWIDTHS (256)
#define COLORWIDTHS (32)
#define NUM (DWIDTHS / 32)
// design for kernel, but only host used now
//#ifndef USE_U55C
//#define MAXNV (1ul << 26)  //  67,108,864
//#define MAXNE (1 << 27)  // 134,217.728
//#define MAXNV_M (64000000)
//#else
//#define MAXNV (1ul << 27)
//#define MAXNE (1 << 28)
//#define MAXNV_M (128000000)
//#endif
//#define VERTEXS (MAXNV / NUM)
//#define EDGES (MAXNE / NUM)

// glb will be init in ParLV.cpp:host_ParserParameters
extern long glb_MAXNV;
extern long glb_MAXNE;
extern long glb_MAXNV_M;

#define DEGREES (1 << 17)
#define COLORS (4096)
typedef double DWEIGHT;

#ifndef MULTITHREAD /* Enable multi-thread mode for using OpenMP for host code */
#define NUMTHREAD (1)
#else
#define NUMTHREAD (16) /* For number of thread, usually it is not the more the best, 16 is based on expericece*/
#endif
// For enlarge size of HBM space
#define WKARND_HBM /* Enable 2-HBM for storing large graphNew, to be as a default \
                      configuration */
#define NUM_PORT_KERNEL (16)
#define MAX_NUM_PHASE (200)
#define MAX_NUM_TOTITR (10000)
#define MAX_NUM_DEV (64)
#define MAX_NUM_PARTITIONS (1024)

using edge = xilinx_apps::louvainmod::Edge;

class graphNew {
   public:
    long numVertices;   /* Number of columns                                */
    long sVertices;     /* Number of rows: Bipartite graph: number of S vertices; T = N - S */
    long numEdges;      /* Each edge stored twice, but counted once        */
    long* edgeListPtrs; /* start vertex of edge, sorted, primary key        */
    edge* edgeList;     /* end   vertex of edge, sorted, secondary key      */
};

struct TimeLv {
    int parNo;
    int phase;
    int totItr;
    unsigned int deviceID[MAX_NUM_PHASE];
    unsigned int cuID[MAX_NUM_PHASE];
    unsigned int channelID[MAX_NUM_PHASE];

    double totTimeAll;
    double totTimeInitBuff;             // = 0;
    double totTimeReadBuff;             // = 0;
    double totTimeReGraph;              // = 0;
    double totTimeE2E_2;                // = 0;
    double totTimeE2E;                  // = 0;
    double totTimeE2E_DEV[MAX_NUM_DEV]; // = 0;
    double totTimeBuildingPhase;
    double totTimeClustering;
    double totTimeColoring;
    double totTimeFeature; // = 0;

    double timePrePre;
    double timePrePre_dev;
    double timePrePre_xclbin;
    double timePrePre_buff;

    double eachTimeInitBuff[MAX_NUM_PHASE];
    double eachTimeReadBuff[MAX_NUM_PHASE];
    double eachTimeReGraph[MAX_NUM_PHASE];
    double eachTimeE2E_2[MAX_NUM_PHASE];
    double eachTimeE2E[MAX_NUM_PHASE];
    double eachTimePhase[MAX_NUM_PHASE];
    double eachTimeFeature[MAX_NUM_PHASE];
    double eachMod[MAX_NUM_PHASE];
    int eachItrs[MAX_NUM_PHASE];
    long eachClusters[MAX_NUM_PHASE];
    double eachNum[MAX_NUM_PHASE];
    double eachC[MAX_NUM_PHASE];
    double eachM[MAX_NUM_PHASE];
    double eachBuild[MAX_NUM_PHASE];
    double eachSet[MAX_NUM_PHASE];
};
struct GLVHead {
    long NV;
    long NVl;
    long NE;
    long NElg;
    long NC;
    double Q;
    int numColors;
    int numThreads;
    char name[256];
    int ID;
};
template <class T_HEAD>
void SaveHead(char* name, T_HEAD* ptr) {
    assert(name);
    FILE* fp = fopen(name, "wb");
    fwrite(ptr, sizeof(T_HEAD), 1, fp);
    fclose(fp);
}
template <class T_HEAD>
void LoadHead(char* name, T_HEAD* ptr) {
    assert(name);
    FILE* fp = fopen(name, "rb");
    fread(ptr, sizeof(T_HEAD), 1, fp);
    fclose(fp);
}
// loading an other file on disk by BFS
template <class T_HEAD>
void Loadselected(char* name, T_HEAD** ptr) {
    assert(name);
    FILE* fp = fopen(name, "rb");
    long nv = 0;
    fread(&nv, sizeof(long), 1, fp);
    if (nv < 0) {
        printf("ERROR: value(%ld) of NV is not right!!!  \n", nv);
        fclose(fp);
    }
    *ptr = (T_HEAD*)malloc(sizeof(T_HEAD) * nv);
    fread(*ptr, sizeof(T_HEAD), nv, fp);
    fclose(fp);
}
struct FeatureLV;
class GLV {
   public:
    long NV;
    long NVl;
    long NE;
    long NElg;
    long NC;
    double Q;
    int numColors;
    int numThreads;
    char name[256];
    int ID;
    graphNew* G;
    long* C;
    long* M;
    int* colors;
    list<FeatureLV> com_list;
    TimeLv times;

   public:
    GLV();
    GLV(int& id);
    ~GLV();
    void InitVar();
    void FreeMem();
    void CleanCurrentG();
    void InitByFile(char*);
    void InitByOhterG(graphNew*);
    void SetByOhterG(graphNew*);
    void SetByOhterG(graphNew* G_src, long* M_src);
    void InitG(graphNew* G_src);
    void InitC(long* C_src);
    void InitM(long* M_src);
    void InitC();
    void InitM();
    void InitColor();
    void SetM(long* M_src);
    void SetM();
    void SetC(long* C_src);
    void SetC();
    void ResetColor();
    void ResetC();
    void SyncWithG();
    void print();
    void printSimple();
    void SetName(char* nm); //{strcpy(name, nm);};
    void SetName_par(int, int, long, long, int);
    void SetName_loadg(int ID_curr, char* path);
    void SetName_ParLvMrg(int num_par, int ID_src);
    void SetName_lv(int, int);
    void SetName_cat(int ID_src1, int ID_src2);
    void PushFeature(int ph, int iter, double time, bool FPGA);
    void printFeature();
    GLV* CloneSelf(int& id_glv);
    GLV* CloneSelf_lite(int& id_glv);
    GLV* RstColorWithGhost();
    void RstNVlByM();
    void RstNVElg();
};

struct FeatureLV {
    double totalTot;
    double totalIn;
    double m;
    double Q;
    long NV;
    long NE;
    long NC; // number of community/number of clusters
    int No_phase;
    int Num_iter;
    double time;
    bool isFPGA;
    void init();
    FeatureLV();
    FeatureLV(GLV* glv);
    double ComputeQ(GLV* glv);
    double ComputeQ2(GLV* glv);
    void PrintFeature();
};

struct KMemorys_clBuff {
    cl::Buffer db_config0;
    cl::Buffer db_config1;
    //
    cl::Buffer db_offsets;
    cl::Buffer db_indices;
    cl::Buffer db_weights;
    cl::Buffer db_indices2;
    cl::Buffer db_weights2;
    cl::Buffer db_colorAxi;
    cl::Buffer db_colorInx;
    //
    cl::Buffer db_cidPrev;
    cl::Buffer db_cidCurr;
    //
    cl::Buffer db_cidSizePrev;
    cl::Buffer db_cidSizeUpdate;
    cl::Buffer db_cidSizeCurr;
    //
    cl::Buffer db_totPrev;
    cl::Buffer db_totUpdate;
    cl::Buffer db_totCurr;
    //
    cl::Buffer db_cWeight;
};

struct KMemorys_host_prune {
    int64_t* config0;
    DWEIGHT* config1;
    // Graph data
    int* offsets;
    int* indices;
    float* weights;
    int* indices2;
    float* weights2;
    int* colorAxi;
    int* colorInx;
    // Updated Community info
    int* cidPrev;
    int* cidCurr;
    // Iterated size of communities
    int* cidSizePrev;
    int* cidSizeUpdate;
    int* cidSizeCurr;
    // Iterated tot of communities
    float* totPrev;
    float* totUpdate;
    float* totCurr;
    //
    float* cWeight;
    //
    int* offsetsdup;
    int* indicesdup;
    int* indicesdup2;
    uint8_t* flag;
    uint8_t* flagUpdate;
    KMemorys_host_prune() { memset((void*)this, 0, sizeof(KMemorys_host_prune)); }
    void freeMem() {
        free(config0);
        free(config1);
        //
        free(offsets);
        free(indices);
        free(weights);
        if (indices2) free(indices2);
        if (weights2) free(weights2);
        free(colorAxi);
        free(colorInx);
        //
        free(cidPrev);
        free(cidCurr);
        //
        free(totPrev);
        free(totUpdate);
        free(totCurr);
        //
        free(cidSizeCurr);
        free(cidSizeUpdate);
        free(cidSizePrev);
        //
        free(cWeight);
        // free
        free(offsetsdup);
        free(indicesdup);
        free(indicesdup2);
        free(flag);
        free(flagUpdate);
    }
};

struct KMemorys_clBuff_prune {
    cl::Buffer db_config0;
    cl::Buffer db_config1;
    //
    cl::Buffer db_offsets;
    cl::Buffer db_indices;
    cl::Buffer db_weights;
    cl::Buffer db_indices2;
    cl::Buffer db_weights2;
    cl::Buffer db_colorAxi;
    cl::Buffer db_colorInx;
    //
    cl::Buffer db_cidPrev;
    cl::Buffer db_cidCurr;
    //
    cl::Buffer db_cidSizePrev;
    cl::Buffer db_cidSizeUpdate;
    cl::Buffer db_cidSizeCurr;
    //
    cl::Buffer db_totPrev;
    cl::Buffer db_totUpdate;
    cl::Buffer db_totCurr;
    //
    cl::Buffer db_cWeight;
    //
    cl::Buffer db_offsetsdup;
    cl::Buffer db_indicesdup;
    cl::Buffer db_indicesdup2;
    cl::Buffer db_flag;
    cl::Buffer db_flagUpdate;
};

int host_ParserParameters(int argc,
                          char** argv,
                          double& opts_C_thresh,   //; //Threshold with coloring on
                          long& opts_minGraphSize, //; //Min |V| to enable coloring
                          double& opts_threshold,  //; //Value of threshold
                          int& opts_ftype,         //; //File type
                          char* opts_inFile,       //;
                          bool& opts_coloring,     //
                          bool& opts_output,       //;
                          bool& opts_VF,           //;
                          char* opts_xclbinPath);

graphNew* host_PrepareGraph(int opts_ftype, char* opts_inFile, bool opts_VF);

int host_writeOut(char* opts_inFile, long NV_begin, long* C_orig);

void runLouvainWithFPGA(graphNew* G,  // Input graph, undirectioned
                        long* C_orig, // Output
                        char* opts_xclbinPath,
                        bool opts_coloring,
                        long opts_minGraphSize,
                        double opts_threshold,
                        double opts_C_thresh,
                        int numThreads);

long renumberClustersContiguously_ghost(long* C, long size, long NV_l);

// Only used by L3

bool PhaseLoop_CommPostProcessing(long NV,
                                  int numThreads,
                                  double opts_threshold,
                                  bool opts_coloring,
                                  double prevMod,
                                  double currMod,
                                  // modified:
                                  graphNew*& G,
                                  long*& C,
                                  long*& C_orig,
                                  bool& nonColor,
                                  int& phase,
                                  int& totItr,
                                  long& numClusters,
                                  double& totTimeBuildingPhase);

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

double PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune(int numColors,
                                                     graphNew* G,
                                                     long* M,
                                                     double opts_C_thresh,
                                                     double* currMod,
                                                     // Updated variables
                                                     int* colors,
                                                     KMemorys_host_prune* buff_host_prune);
double PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune_renumber(int numColors,
                                                              long NVl,
                                                              graphNew* G,
                                                              long* M,
                                                              double opts_C_thresh,
                                                              double* currMod,
                                                              // Updated variables
                                                              int* colors,
                                                              KMemorys_host_prune* buff_host_prune);
double PhaseLoop_UsingFPGA_Prep_Init_buff_host_prune_renumber_2cu(int numColors,
                                                                  long NVl,
                                                                  graphNew* G,
                                                                  long* M,
                                                                  double opts_C_thresh,
                                                                  double* currMod,
                                                                  // Updated variables
                                                                  int* colors,
                                                                  KMemorys_host_prune* buff_host);
double PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune(long vertexNum,
                                                     KMemorys_host_prune* buff_host_prune,
                                                     int* eachItrs,
                                                     // output
                                                     long* C,
                                                     int* eachItr,
                                                     double* currMod);
double PhaseLoop_UsingFPGA_Prep_Read_buff_host_prune_renumber(long vertexNum,
                                                              KMemorys_host_prune* buff_host_prune,
                                                              int* eachItrs,
                                                              // output
                                                              long* C,
                                                              int* eachItr,
                                                              double* currMod,
                                                              long* numClusters);

struct LouvainPara {
    bool opts_coloring;     // whether use coloring; It always be true for FPGA flow
    long opts_minGraphSize; // Minimal number of community for stopping Louvain phase
    double opts_threshold;  // dQ threshold for Non-coloring flow
    double opts_C_thresh;   // dQ threshold for coloring flow
    int numThreads;         // Number of threads
    int max_num_level;
    int max_num_iter;
};

#endif
