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

#ifndef _PARLV_HPP_
#define _PARLV_HPP_
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "xilinxlouvain.hpp"
#include "xilinxlouvainInternal.hpp"
#include "partitionLouvain.hpp"
#include "xf_graph_L3.hpp"
#include <unordered_map>
#define MAX_PARTITION (512)
#define MAX_DEVICE (64)
#define MAX_SERVER (64)

enum MOKD_PHASEFLOW { MD_CLASSIC = 0, MD_NORMAL, MD_FAST };

const long headGLVBin = 0xffff5555ffff5555;

struct TimePartition {
    double time_star;
    double time_done_par;
    double time_done_lv;
    double time_done_pre;
    double time_done_mg;
    double time_done_fnl;
    //
    double timePar[MAX_PARTITION];
    double timePar_all;
    double timePar_save;
    double timeLv[MAX_PARTITION];
    double timeLv_dev[MAX_DEVICE];
    double timeLv_all;
    double timeWrkSend[MAX_DEVICE];
    double timeWrkLoad[MAX_DEVICE];
    double timeWrkCompute[MAX_DEVICE]; // the compute time on each nodes(May have multi cards)
    double timeDriverSend;
    double timeDriverLoad;
    double timeDriverExecute;
    double timeDriverCollect;
    double timeDriverRecv;
    double timePre;
    double timeMerge;
    double timeFinal;
    double timeAll;
};

struct ParLVVar {
    bool st_Partitioned;
    bool st_ParLved;
    bool st_PreMerged;
    bool st_Merged;
    bool st_FinalLved;
    bool st_Merged_ll;
    bool st_Merged_gh;
    bool isMergeGhost;
    bool isOnlyGL;
    int kernelMode;
    int num_par;
    int num_dev;
    long NV;
    long NVl;
    long NV_gh;
    long NE;
    long NEll;
    long NElg;
    long NEgl;
    long NEgg;
    long NEself;
    long off_src[MAX_PARTITION];
    long off_lved[MAX_PARTITION];
    long max_NV;
    long max_NE;
    long max_NVl;
    long max_NElg;
    int scl_NV;
    int scl_NE;
    int scl_NVl;
    int scl_NElg;
    long NE_list_all;
    long NE_list_ll;
    long NE_list_gl;
    long NE_list_gg;
    long NV_list_all;
    long NV_list_l;
    long NV_list_g;
    bool isPrun;
    int th_prun;
};

struct bfs_selected {
    int par_idx;      // idx of the partition
    int renum_in_par; // the new number index in the partition
};

class ParLV {
   public:
    bool st_Partitioned;
    bool st_ParLved;
    bool st_PreMerged;
    bool st_Merged;
    bool st_FinalLved;
    bool st_Merged_ll;
    bool st_Merged_gh;
    bool isMergeGhost;
    bool isOnlyGL;
    int kernelMode;
    int num_par;
    int num_dev;
    long NV;
    long NVl;
    long NV_gh;
    long NE;
    long NEll;
    long NElg;
    long NEgl;
    long NEgg;
    long NEself;
    long off_src[MAX_PARTITION];
    long off_lved[MAX_PARTITION];
    long max_NV;
    long max_NE;
    long max_NVl;
    long max_NElg;
    int scl_NV;
    int scl_NE;
    int scl_NVl;
    int scl_NElg;
    long NE_list_all;
    long NE_list_ll;
    long NE_list_gl;
    long NE_list_gg;
    long NV_list_all;
    long NV_list_l;
    long NV_list_g;
    bool isPrun;
    int th_prun;
    bool use_bfs;               // if is useing bfs Low Bandwidth methods(BFS)
    bfs_selected* bfs_adjacent; // added structure and saing on disk by BFS
    int flowMode;
    ///////////////////////////////////
    int num_server; // default '1' means using concentration partition
    int numServerCard[MAX_SERVER];
    int parInServer[MAX_PARTITION];
    long parOffsets[MAX_PARTITION]; // start vertex for each partition. Currently no use for it. Just recored in
                                    // .par.proj file
    ///////////////////////////////////
    GLV* plv_src;
    GLV* par_src[MAX_PARTITION];
    GLV* par_lved[MAX_PARTITION];
    GLV* plv_merged;
    GLV* plv_final;
    long* p_v_new[MAX_PARTITION];
    SttGPar stt[MAX_PARTITION];
    TimePartition timesPar;
    unordered_map<long, long> m_v_gh;
    // list<GLV*> lv_list;

    edge* elist;
    long* M_v;

    void Init(int mode);
    void Init(int mode, GLV* src, int numpar, int numdev);
    void Init(int mode, GLV* src, int num_p, int num_d, bool isPrun, int th_prun);
    ParLV();
    ~ParLV();
    void PrintSelf();
    void UpdateTimeAll();
    void CleanList(GLV* glv_curr, GLV* glv_temp);
    GLV* MergingPar2(int&);
    // GLV* FinalLouvain(char*, int , int& , long minGraphSize, double threshold, double C_threshold, bool isParallel,
    // int numPhase);

    long MergingPar2_ll();
    long MergingPar2_gh();
    long CheckGhost();
    int partition(GLV* glv_src, int& id_glv, int num, long th_size, int th_maxGhost);
    void PreMerge();
    void Addedge(edge* edges, long head, long tail, double weight, long* M_g);
    long FindGhostInLocalC(long m);
    int FindParIdx(long e_org);
    int FindParIdxByID(int id);
    pair<long, long> FindCM_1hop(int idx, long e_org);
    pair<long, long> FindCM_1hop(long e_org);
    long FindC_nhop(long m_gh);
    pair<long, long> FindCM_1hop_bfs(int idx, long e_org, long addr_v); // for BFS premerge
    long FindC_nhop_bfs(long m_gh);                                     // for BFS premerge
    int AddGLV(GLV* plv);
    void PrintTime();
    void PrintTime2();
    void CleanTmpGlv();
    double TimeStar();
    double TimeDonePar();
    double TimeDoneLv();
    double TimeDonePre();
    double TimeDoneMerge();
    double TimeDoneFinal();
    double TimeAll_Done();
};

GLV* par_general(GLV* src, SttGPar* pstt, int& id_glv, long start, long end, bool isPrun, int th_prun);
GLV* par_general(GLV* src, int& id_glv, long start, long end, bool isPrun, int th_prun);

GLV* CreateByFile_general(char* inFile, int& id_glv);

int SaveGLVBin(char* name, GLV* glv);

double getTime();

int xai_save_partition(long* offsets_tg,
                       edge* edgelist_tg,
                       long* drglist_tg,
                       long start_vertex,     // If a vertex is smaller than star_vertex, it is a ghost
                       long end_vertex,       // If a vertex is larger than star_vertex-1, it is a ghost
                       char* path_prefix,     // For saving the partition files like <path_prefix>_xxx.par
                                              // Different server can have different path_prefix
                       int par_prune,         // Can always be set with value '1'
                       long NV_par_recommand, // Allow to partition small graphs not bigger than FPGA limitation
                       long NV_par_max        //  64*1000*1000;
                       );

// API for BFS partition
int xai_save_partition_bfs(long* offsets_tg,
                           edge* edgelist_tg,
                           long* drglist_tg,
                           long start_vertex,     // If a vertex is smaller than star_vertex, it is a ghost
                           long end_vertex,       // If a vertex is larger than star_vertex-1, it is a ghost
                           char* path_prefix,     // For saving the partition files like <path_prefix>_xxx.par
                                                  // Different server can have different path_prefix
                           int par_prune,         // Can always be set with value '1'
                           long NV_par_recommand, // Allow to partition small graphs not bigger than FPGA limitation
                           long NV_par_max        //  64*1000*1000;
                           );

void SaveParLV(char* name, ParLV* p_parlv);

void sim_getServerPar(
    // input
    graphNew* G,       // Looks like a Global Graph but here only access dataset within
    long start_vertex, // a range from start_vertex to end_vertex, which is stored locally
    long end_vertex,   // Here we assume that the vertices of a TigerGraph partition
                       // stored on a node are continuous
    // Output
    long* offsets_tg, // we can also use �degree� instead of �offsets�
    edge* edges_tg,   //
    long* dgr_tail_tg // degrees for the tail of each edge;
    );

int getNumPartitions(std::string alveoProjectFile);

const char* NameNoPath(const char* name);

void PathNoName(char* des, const char* name);

int Phaseloop_InitColorBuff(graphNew* G, int* colors, int numThreads, double& totTimeColoring);

#endif
