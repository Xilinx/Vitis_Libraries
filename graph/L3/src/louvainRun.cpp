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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xf_graph_L3.hpp"
#include "op_louvainmodularity.hpp"
#include <unordered_map>
#include "ParLV.hpp"

//#####################################################################################################################
//
// LouvainRun myCmd
//
#if 1
myCmd::myCmd() {
    cnt_his = 0;
    argc = 0;
    cmd_last = -1;
};
const char* myCmd::cmd_SkipSpace(const char* str) {
    const char* pch = str;
    while (*pch != 0) {
        if (*pch == ' ' || *pch == '\t') {
            pch++;
        } else
            break;
    }
    return pch;
}
bool isChNormal(char ch) {
    return (ch != '\0') && (ch != '\n') && (ch != ' ') && (ch != '\t' && (ch != '#'));
}
int myCmd::cmd_CpyWord(char* des, const char* src) {
    int cnt = 0;
    assert(des);
    assert(src);
    while (isChNormal(*src)) {
        *des = *src;
        src++;
        des++;
        cnt++;
    }
    *des = '\0';
    return cnt;
}
void myCmd::cmd_resetArg() {
    if (argc == 0) return;
    for (int i = 0; i < argc; i++) argv[i][0] = '\0';
    argc = 0;
}

int myCmd::cmd_findPara(const char* para) {
    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], para)) return i;
    }
    return -1;
}

int Getline(char* str) {
    char ch;
    int cnt = 0;
    do {
        ch = getchar();
        str[cnt++] = ch;
    } while (ch != '\n');
    return cnt;
}

int myCmd::cmd_Getline() { //( char** argv){
    // int argc;
    // assert(argv!=0);
    char str[4096];

    cmd_resetArg();
    // printf("$LVCMD:\/");
    printf("\033[1;31;40m$\033[0m\033[1;34;40m[LVCMD]$: \033[0m");
    // scanf("%[^\n]", str);
    Getline(str);
    line_last = str;
    // his_cmd.push_back(line_last);
    const char* pch = str;
    do {
        pch = cmd_SkipSpace(pch);
        if (isChNormal(*pch)) {
            pch += cmd_CpyWord(argv[argc++], pch);
        }
    } while (*pch != '\n' && *pch != '\0' && *pch != '#');
    return argc;
}
int myCmd::cmd_Getline(const char* str) { //( char** argv){
    cmd_resetArg();
    line_last = str;
    // his_cmd.push_back(line_last);
    const char* pch = str;
    do {
        pch = cmd_SkipSpace(pch);
        if (isChNormal(*pch)) {
            pch += cmd_CpyWord(argv[argc++], pch);
        }
    } while (*pch != '\n' && *pch != '\0' && *pch != '#');
    return argc;
}

#endif

//#####################################################################################################################
//
// LouvainRun
//
#if 1

void getDiffTime(TimePointType& t1, TimePointType& t2, double& time) {
    t2 = chrono::high_resolution_clock::now();
    chrono::duration<double> l_durationSec = t2 - t1;
    time = l_durationSec.count();
}

/*
    Return values:
    -1:
    Other positive numbers: number of partitions saved in the Alveo project file
*/
int getNumPartitions(std::string alveoProjectFile) {
    int numPartitions = 0;
    int numServers;
    FILE* fp = fopen(alveoProjectFile.c_str(), "r");
    if (fp == NULL) {
        std::cout << "ERROR: Failed to open Alveo project file " << alveoProjectFile << std::endl;
        return ERRORCODE_GETNUMPARTITIONS_FAILED_OPEN_ALVEOPRJ;
    }
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* fdata = (char*)malloc(fsize * sizeof(char));
    assert(fdata);
    fread(fdata, fsize, sizeof(char), fp);
    fclose(fp);

    myCmd ps;
    ps.cmd_Getline(fdata);
    if (ps.argc < 8) {
        std::cout << "ERROR: Invalid Alveo project settings in " << alveoProjectFile << std::endl;
        free(fdata);
        return ERRORCODE_GETNUMPARTITIONS_INVALID_ARGC;
    }

    //////////////////////////////////////////////////////////////////////////
    // for multi-server partition [-server_par <num_server> <num_par on server0> <num_par on server?>]
    int idx_server = ps.cmd_findPara("-server_par");
    if (idx_server > -1) {
        numPartitions = 0;
        numServers = atoi(ps.argv[idx_server + 1]);
        for (int i_svr = 0; i_svr < numServers; i_svr++) {
            numPartitions += atoi(ps.argv[idx_server + 2 + i_svr]);
        }
    }

    // make sure numPartitions is reasonable
    if (numPartitions > MAX_NUM_PARTITIONS) {
        std::cout << "ERROR: number of partitions in Alveo project file " << alveoProjectFile << " exceeds the maximum "
                  << MAX_NUM_PARTITIONS << std::endl;
        std::cout << "    Please check that the project file is on a shared drive accessible by all nodes and is not "
                     "corrupted."
                  << std::endl;
        return ERRORCODE_GETNUMPARTITIONS_INVALID_NUMPARS;
    } else
        return numPartitions;
}

// persistent storge for L3::Handle that is shared by L3 functions
class sharedHandlesLouvainMod {
   public:
    std::unordered_map<unsigned int, std::shared_ptr<xf::graph::L3::Handle> > handlesMap;
    static sharedHandlesLouvainMod& instance() {
        static sharedHandlesLouvainMod theInstance;
        return theInstance;
    }
};

void freeSharedHandle() {
    if (!sharedHandlesLouvainMod::instance().handlesMap.empty()) {
        // free the object stored in handlsMap[0] and erase handlsMap[0]
        std::cout << "INFO: " << __FUNCTION__ << std::endl;
        std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesLouvainMod::instance().handlesMap[0];
        handle0->free();
        sharedHandlesLouvainMod::instance().handlesMap.erase(0);
    }
}

int loadComputeUnitsToFPGAs(char* xclbinPath, int kernelMode, unsigned int numDevices, std::string deviceNames) {
    int status = 0;

    std::string opName = "louvainModularity";
    std::string kernelName = "kernel_louvain";
    int requestLoad = 100;

    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesLouvainMod::instance().handlesMap[0];

    xf::graph::L3::Handle::singleOP* op0 = new (xf::graph::L3::Handle::singleOP);
    //----------------- Set parameters of op0 again some of those will be covered by command-line
    op0->operationName = (char*)opName.c_str();
    op0->setKernelName((char*)kernelName.c_str());
    if (kernelMode == LOUVAINMOD_PRUNING_KERNEL)
        op0->setKernelAlias("kernel_louvain_pruning_u50");
    else if (kernelMode == LOUVAINMOD_2CU_U55C_KERNEL)
        op0->setKernelAlias("kernel_louvain_2cu_u55");

    op0->requestLoad = requestLoad;
    op0->xclbinPath = xclbinPath;
    op0->deviceNeeded = numDevices;
    op0->cuPerBoard = (kernelMode == LOUVAINMOD_2CU_U55C_KERNEL) ? 2 : 1;
#ifndef NDEBUG
    std::cout << "DEBUG: loadComputeUnitsToFPGAs"
              << "\n     xclbinPath=" << xclbinPath << "\n     kernelMode=" << kernelMode
              << "\n     numDevices=" << numDevices << "\n     deviceNames=" << deviceNames
              << "\n     cuPerBoard=" << op0->cuPerBoard << std::endl;
#endif
    //----------------- enable handle0--------
    handle0->addOp(*op0);
    status = handle0->setUp(deviceNames);

    return status;
}

// create a new shared handle if
// 1. The shared handle does not exist or
// 2. The numDevices option changes
int createSharedHandle(char* xclbinPath,
                       int kernelMode,
                       unsigned int numDevices,
                       std::string deviceNames,
                       bool opts_coloring,
                       long opts_minGraphSize,
                       double opts_C_thresh,
                       int numThreads) {
    int status = 0;

    // return right away if the handle has already been created
    if (!sharedHandlesLouvainMod::instance().handlesMap.empty()) {
        std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesLouvainMod::instance().handlesMap[0];

        std::cout << "DEBUG: " << __FUNCTION__ << " numDevices=" << handle0->getNumDevices() << std::endl;
        handle0->showHandleInfo();
        if (numDevices != handle0->getNumDevices()) {
            std::cout << "INFO: " << __FUNCTION__ << "numDevices changed. Creating a new handle."
                      << " numDevices loaded=" << handle0->getNumDevices() << " numDevices requested=" << numDevices
                      << std::endl;
            freeSharedHandle();
        } else {
            std::cout << "INFO: " << __FUNCTION__ << " Using exsiting handle with " << handle0->getNumDevices()
                      << " devices loaded." << std::endl;
            return 0;
        }
    }
    std::cout << "INFO: " << __FUNCTION__ << std::endl;
    std::shared_ptr<xf::graph::L3::Handle> handleInstance(new xf::graph::L3::Handle);
    sharedHandlesLouvainMod::instance().handlesMap[0] = handleInstance;
    status = loadComputeUnitsToFPGAs(xclbinPath, kernelMode, numDevices, deviceNames);
    if (status < 0) return ERRORCODE_CREATESHAREDHANDLE;

    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesLouvainMod::instance().handlesMap[0];
    (handle0->oplouvainmod)
        ->mapHostToClBuffers(NULL, kernelMode, opts_coloring, opts_minGraphSize, opts_C_thresh, numThreads);

    return status;
}

int LoadParLV(char* name, ParLV* p_parlv) {
    assert(name);
    FILE* fp = fopen(name, "rb");

    if (fp == NULL) {
        printf("ERROR: Failed to open ParLV file %s\n", name);
        return -1;
    }

    char* ptr = (char*)p_parlv;
    int kernelMode = p_parlv->kernelMode;
    int num_dev = p_parlv->num_dev;
    fread(ptr, sizeof(ParLVVar), 1, fp);
    p_parlv->kernelMode = kernelMode;
    p_parlv->num_dev = num_dev;
    fclose(fp);

    return 0;
}

// Parser_ParProjFile
// Return value:
// -2: ParLV file cannot be opened
//
int Parser_ParProjFile(std::string projFile, ParLV& parlv, char* path, char* name, char* name_inFile) {
#ifndef NDEBUG
    printf("DEBUG:\n    projFile=%s\n    path=%s\n    name=%s\n    name_inFile=%s\n", projFile.c_str(), path, name,
           name_inFile);
#endif
    // Format: -create_alveo_partitions <inFile> -num_pars <num_pars> -par_prune <par_prune> -name <ProjectFile>
    assert(path);
    assert(name);
    assert(name_inFile);
    FILE* fp = fopen(projFile.c_str(), "r");
    if (fp == NULL) {
        printf("\033[1;31;40mERROR\033[0m: Project Metadata file %s failed for open \n", projFile.c_str());
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* fdata = (char*)malloc(fsize * sizeof(char));
    assert(fdata);
    fread(fdata, fsize, sizeof(char), fp);
    fclose(fp);
    myCmd ps;
    ps.cmd_Getline(fdata);
    if (ps.argc < 8) {
        printf("\033[1;31;40mERROR\033[0m: Partition Project Meta wrong\n");
        free(fdata);
        return -1;
    }

    if (strcmp("-create_alveo_partitions", ps.argv[0]) == 0 &&
        (strcmp("-create_alveo_BFS_partitions", ps.argv[0]) != 0)) {
        parlv.use_bfs = false;
    } else if (strcmp("-create_alveo_partitions", ps.argv[0]) != 0 &&
               (strcmp("-create_alveo_BFS_partitions", ps.argv[0]) == 0)) {
        parlv.use_bfs = true;
    } else {
        printf("\033[1;31;40mERROR\033[0m: MessageParser_D2W: Unknow head%s. -create_alveo_partitions is required\n",
               ps.argv[0]);
        free(fdata);
        return -1;
    }
    strcpy(name_inFile, ps.argv[1]);
    char* str_par_num = ps.argv[3];
    char* str_par_prune = ps.argv[5];
    char* nameProj = ps.argv[7];
#ifdef PRINTINFO_2
    printf("\033[1;37;40mINFO: The graph matrix is   : %s \033[0m\n", name_inFile);
#endif
    if (ps.argc > 9) {
        parlv.timesPar.timePar_all = atof(ps.argv[9]);
        parlv.timesPar.timePar_save = atof(ps.argv[11]);
    } else {
        parlv.timesPar.timePar_all = 0;
        parlv.timesPar.timePar_save = 0;
    }
    //////////////////////////////////////////////////////////////////////////
    // for multi-server partition [-server_par num_server num_par_on_server0 ... num_par on serverN-1]
    int idx_server = ps.cmd_findPara("-server_par");
    if (idx_server > -1) {
        parlv.num_par = 0;
        parlv.num_server = atoi(ps.argv[idx_server + 1]);
        for (int i_svr = 0; i_svr < parlv.num_server; i_svr++) {
            parlv.parInServer[i_svr] = atoi(ps.argv[idx_server + 2 + i_svr]);
            parlv.num_par += parlv.parInServer[i_svr];
        }
    }
    ///////////////////////////////////////////////////////////////////////////
    if (nameProj[0] == '/')
        PathNoName(path, nameProj); // Absolute name used in project file, so abstract the path use project name
    else
        PathNoName(path, projFile.c_str()); // Use project file path for abstraction the path
    strcpy(name, NameNoPath(nameProj));
    int id_glv = 0;
    char namePath_tmp[1024];
    sprintf(namePath_tmp, "%s/%s.par.parlv", path, name);
#ifndef NDEBUG
    printf("DEBUG: start LoadParLV, use_bfs=%d\n", parlv.use_bfs);
#endif
    if (LoadParLV(namePath_tmp, &parlv) < 0) return -2;

    sprintf(namePath_tmp, "%s/%s.par.src", path, name);
    // std::cout << "------------" << __FUNCTION__ << " id_glv=" << id_glv << std::endl;
    parlv.plv_src = new GLV(id_glv);
    LoadHead<GLVHead>(namePath_tmp, (GLVHead*)parlv.plv_src);
#ifndef NDEBUG
    printf(
        "DEBUG: LoadHead results\n    NV=%ld\n    NVl=%ld\n    NE=%ld\n    NElg=%ld\n    NC=%ld\n    Q=%f\n    "
        "numColors=%d\n    numThreads=%d\n    name=%s\n    ID=%d\n",
        parlv.plv_src->NV, parlv.plv_src->NVl, parlv.plv_src->NE, parlv.plv_src->NElg, parlv.plv_src->NC,
        parlv.plv_src->Q, parlv.plv_src->numColors, parlv.plv_src->numThreads, parlv.plv_src->name, parlv.plv_src->ID);
#endif

    // add to parser BFS partition's global ID
    if (parlv.use_bfs) {
        sprintf(namePath_tmp, "%s/%s.bfs.adj", path, name);
        // std::cout << "------------" << __FUNCTION__ << " id_glv=" << id_glv << std::endl;
        Loadselected<bfs_selected>(namePath_tmp, &(parlv.bfs_adjacent));
#ifndef NDEBUG
        printf("DEBUG: Loadselected results\n %d--> %d--> %d\t\n", 0, parlv.bfs_adjacent[0].par_idx,
               parlv.bfs_adjacent[0].renum_in_par);
#endif
    }

#ifdef PRINTINFO
    printf("\033[1;37;40mINFO\033[0m:Partition Project: path = %s name = %s\n", path, name);
#endif
    if (parlv.num_par != atoi(str_par_num)) {
        printf("\033[1;31;40mWARNING\033[0m: parlv.num_par(%d) != %d; Value in file will be used\n", parlv.num_par,
               atoi(str_par_num));
    }
    parlv.num_par = atoi(str_par_num);
    parlv.th_prun = atoi(str_par_prune);

    int cnt_file = 0;

    int p = 0;
    int i_svr = 0;
    int p_svr = 0;
    while (p < parlv.num_par) {
        char nm[1024];
        if (parlv.num_server == 1)
            sprintf(nm, "%s_%03d.par", name, p);
        else {
            sprintf(nm, "%s_svr%d_%03d.par", name, i_svr, p_svr);
        }
#ifndef NDEBUG
        printf("DEBUG:     par_src->name[%3d]=%s\n", p, nm);
#endif
        parlv.par_src[p]->SetName(nm);
        cnt_file++;
        if (p_svr == parlv.parInServer[i_svr] - 1) {
            i_svr++;
            p_svr = 0;
        } else
            p_svr++;
        p++;
    }
    free(fdata);
    if (cnt_file != parlv.num_par)
        return -1;
    else
        return 0;
}

GLV* LoadGLVBin(char* name, int& id_glv) {
    assert(name);

    graphNew* g = (graphNew*)malloc(sizeof(graphNew));
    long nv = 0;
    long ne = 0;
    long ne_undir = 0;
    long nc = 0;
    double Q = -1;
    long head = 0;
    long nvl;  //= glv->NVl;
    long nelg; //= glv->NElg;

    FILE* fp = fopen(name, "rb");
    if (fp == NULL) {
        printf("ERROR: LoadGLVBin failed to open %s \n", name);
        // fclose(fp);
        return NULL;
    }
    fread(&head, sizeof(long), 1, fp);
    if (head != headGLVBin) {
        printf("ERROR: head(%ld)!=headGLVBin(%ld) \n", head, headGLVBin);
        fclose(fp);
        free(g);
        return NULL;
    }
    fread(&nv, sizeof(long), 1, fp);
    if (nv <= 0) {
        printf("ERROR: value(%ld) of NV is not right!!!  \n", nv);
        fclose(fp);
        free(g);
        return NULL;
    }
    fread(&ne, sizeof(long), 1, fp);
    if (ne <= 0) {
        printf("ERROR: value(%ld) of NE is not right!!!  \n", ne);
        fclose(fp);
        free(g);
        return NULL;
    }
    fread(&ne_undir, sizeof(long), 1, fp);
    if (ne_undir <= 0) {
        printf("ERROR: value(%ld) of ne_undir is not right!!!  \n", ne_undir);
        fclose(fp);
        free(g);
        return NULL;
    }
    fread(&nc, sizeof(long), 1, fp);
    if (nc < 0) {
        printf("ERROR: value(%ld) of NC is not right!!!  \n", nc);
        fclose(fp);
        free(g);
        return NULL;
    }
    fread(&Q, sizeof(double), 1, fp);
    if (Q > 1) {
        printf("ERROR: value(%lf) of Q is not right!!!  \n", Q);
        fclose(fp);
        free(g);
        return NULL;
    }
    fread(&nvl, sizeof(long), 1, fp);
    fread(&nelg, sizeof(long), 1, fp);

    g->numEdges = ne;
    g->numVertices = nv;
#ifdef PRINTINFO
    printf("INFO: LoadGLVBin %s Successfully: nv=%ld ne=%ld undir ne=%ld nc=%ld Q=%lf \n", name, nv, ne, ne_undir, nc,
           Q);
#endif
    g->edgeListPtrs = (long*)malloc(sizeof(long) * nv + 1);
    g->edgeList = (edge*)malloc(sizeof(edge) * ne_undir);
    long* M = (long*)malloc(sizeof(long) * nv);
    assert(g->edgeListPtrs);
    assert(g->edgeList);
    assert(M);
    fread(g->edgeListPtrs, sizeof(long), nv + 1, fp);
    fread(g->edgeList, sizeof(edge), ne_undir, fp);
    fread(M, sizeof(long), nv, fp);
    // std::cout << "------------" << __FUNCTION__ << " id_glv=" << id_glv << std::endl;
    GLV* glv = new GLV(id_glv);
    glv->SetByOhterG(g, M);
    fread(glv->C, sizeof(long), nv, fp);
    glv->NC = nc;
    glv->Q = Q;
    glv->NVl = nvl;   //= glv->NVl;
    glv->NElg = nelg; //= glv->NElg;
    fclose(fp);
    return glv;
}

GLV* LoadGLVBin(char* path, char* file, int& id_glv) {
    assert(path);
    assert(file);
    char pathName[2048];
    strcpy(pathName, path);
    strcat(pathName, file);
    return LoadGLVBin(pathName, id_glv);
}

// Return values
//  0: Success
// -2: Fail to load partition file
int LouvainProcess_part1(int& nodeID, ParLV& parlv, char* path_driver, ParLV& parlv_wkr) {
#ifndef NDEBUG
    std::cout << "DEBUG:" << __FILE__ << "::" << __FUNCTION__
              //<< "\n    msg_d2w=" << tmp_msg_d2w
              << "\n    parlv_wkr num_par=" << parlv_wkr.num_par << std::endl;
#endif

    int status = 0;

    // this will be initialized by message again
    // parlv_wkr.Init(parlv.kernelMode, NULL, parlv.num_par, parlv.num_dev, parlv.isPrun, parlv.th_prun);
    // char path_driver[1024];
    // char names[32][256];

    // MessageParser_D2W(tmp_msg_d2w, parlv_wkr, path_driver, names, nodeID);
    // parlv_wkr.num_par = parlv.num_par;

    // parlv.PrintSelf();
    // worker: load file////////////////
    TimePointType l_load_start = chrono::high_resolution_clock::now();
    TimePointType l_load_end;

    int id_glv_wkr = 0;
    char* path_worker = (char*)"./";
    // Worker: Loading from disk
    for (int p = 0; p < parlv_wkr.num_par; p++) {
#ifndef NDEBUG
        printf("INFO: Loading partition file %s%s \n", path_driver, parlv.par_src[p]->name);
#endif
        parlv_wkr.par_src[p] = LoadGLVBin(path_driver, parlv.par_src[p]->name, id_glv_wkr);
        if (parlv_wkr.par_src[p] == NULL) return -2;
        parlv_wkr.par_src[p]->SetName(parlv.par_src[p]->name);
        parlv_wkr.st_Partitioned = true;
#ifdef PRINTINFO
        parlv_wkr.par_src[p]->printSimple();
#endif
    }

    getDiffTime(l_load_start, l_load_end, parlv_wkr.timesPar.timeWrkLoad[0]);

    return status;
}

/*
    For both driver; no zmq communications occur
    Return values:
    0: Success
    -1: Error in Parser_ParProjFile
    -2: Error in LouvainProcess_part1
*/
int load_alveo_partitions_DriverSelf( // Driver* drivers,
    std::string projFile,
    int numNode,
    int numPureWorker,
    // output
    ParLV& parlv_drv,
    ParLV& parlv_wkr,
    char* name_inFile) {
    int status = 0;
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FILE__ << "::" << __FUNCTION__ << " numNode=" << numNode
              << " numPureWorker=" << numPureWorker << std::endl;
#endif

    // ParLV parlv;
    char path_proj[1024];
    char name_proj[1024];
    if (Parser_ParProjFile(projFile, parlv_drv, path_proj, name_proj, name_inFile) != 0) {
        printf("\033[1;31;40mERROR\033[0m: load_alveo_partitions Failed\n");
        return -1;
    }

    /*
    //int id_glv = 0;

    // Assign partitions to the driver and all workers.
    // The driver gets the last partitions.
    // Each worker gets numPartitions/numNode partitions.
    // Below is an example of partition assignment for 10 partitions on 3 servers (a.k.a nodes)
    // worker1: par_svr0_000.par par_svr0_001.par par_svr0_002.par
    // worker2: par_svr1_000.par par_svr1_001.par par_svr1_002.par
    // driver : par_svr2_000.par par_svr2_001.par par_svr2_002.par par_svr2_003.par
    //char msg_d2w[numPureWorker][MAX_LEN_MESSAGE];
    //char msg_driver[MAX_LEN_MESSAGE];
    {
        //////////////////////////// CreateSendMessage for driver itself ////////////////////////////
        //TimePointType l_send_start = chrono::high_resolution_clock::now();
        //TimePointType l_send_end;
        //int stride = parlv_drv.num_par / numNode;
        // Generate message to driver itself, which always gets the last partition files
       // MessageGen_D2W(msg_driver, parlv_drv, path_proj, stride * (numNode - 1), parlv_drv.num_par, 0);

//         //////////////////////////// CreateSendMessage for worker to be sent ////////////////////////////
//         for(int nodeID = 1; nodeID<=numPureWorker; nodeID++){
//         	MessageGen_D2W(msg_d2w[nodeID-1], parlv_drv, path_proj, stride * (nodeID - 1), stride * nodeID, nodeID);
// #ifndef NDEBUG
//         	printf("DEBUG: after MessageGen_D2W msg_d2w=%s\n", msg_d2w[nodeID-1]);
// #endif
//         	drivers[nodeID-1].send(msg_d2w[nodeID-1], MAX_LEN_MESSAGE, 0);
//         }

//        getDiffTime(l_send_start, l_send_end, parlv_drv.timesPar.timeDriverSend);
    }*/
    int nodeID = 0;
    // char msg_w2d[numNode][MAX_LEN_MESSAGE];
    {
        // status = LouvainProcess_part1(nodeID, parlv_drv, msg_driver, parlv_wkr);
        status = LouvainProcess_part1(nodeID, parlv_drv, path_proj, parlv_drv);
    }

    return status;
}

/*
 Return values:
  0: Success
 -1:
 -2:
 -3:
*/
int compute_louvain_alveo_seperated_load(int kernelMode,
                                         unsigned int numDevices,
                                         unsigned int numPartitions,
                                         char* alveoProject,
                                         int mode_zmq,
                                         int numPureWorker,
                                         char* nameWorkers[128],
                                         unsigned int nodeId,
                                         float tolerance,
                                         bool verbose,
                                         std::shared_ptr<xf::graph::L3::Handle>& handle0,
                                         ParLV* p_parlv_dvr,
                                         ParLV* p_parlv_wkr) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n     kernelMode=" << kernelMode << "\n     numDevices=" << numDevices
              << "\n     numPartitions=" << numPartitions << "\n     alveoProject=" << alveoProject
              << "\n     mode_zmq=" << mode_zmq << "\n     numPureWorker=" << numPureWorker
              << "\n     nodeId=" << nodeId << "\n     tolerance=" << tolerance << "\n     verbose=" << verbose
              << std::endl;

    for (int i = 0; i < numPureWorker; i++)
        std::cout << "DEBUG: nameWorker " << i << "=" << nameWorkers[i] << std::endl;

#endif
    int status = 0;
    bool isPrun = true;

    double opts_threshold = 0.000001; // Value of threshold, no impect on for FPGA related flows.
    int par_prune = 1;
    int numNode = numPureWorker + 1;

    if (status < 0) return status;

    if (mode_zmq == ZMQ_DRIVER) {
        //-----------------------------------------------------------------
        // DRIVER
        //-----------------------------------------------------------------
        char inFile[1024] = "";

        p_parlv_dvr->Init(kernelMode, NULL, numPartitions, numDevices, isPrun, par_prune);
        p_parlv_wkr->Init(kernelMode, NULL, numPartitions, numDevices, isPrun, par_prune);

        // API FOR LOADING
        {
            // Driver* drivers = new Driver[numPureWorker];
            // ConnectWorkers(drivers, numPureWorker, nameWorkers);
            TimePointType l_load_start = chrono::high_resolution_clock::now();
            TimePointType l_load_end;
            status = load_alveo_partitions_DriverSelf(/*drivers,*/ alveoProject, numNode, numPureWorker, (*p_parlv_dvr),
                                                      (*p_parlv_wkr), inFile);
            if (status < 0) return status;

            getDiffTime(l_load_start, l_load_end, p_parlv_dvr->timesPar.timeDriverLoad);
            /*
                        const char* char_tmp = "shake hands from Driver";
                        for (int i = 0; i < numPureWorker; i++) {
            #ifdef PRINTINFO
                            printf("Driver shake hands with worker %d\n", (i + 1));
            #endif
                            drivers[i].send(char_tmp, MAX_LEN_MESSAGE, 0);
                        }

                        isAllWorkerLoadingDone(drivers, numPureWorker);
                        delete[] drivers;
                        */
        }
#ifdef PRINTINFO
        printf("\n\n\033[1;31;40mDriver's LOADING DONE \033[0m\n");
        printf("\n\n\033[1;31;40mPlease Wait for Workers' Done\033[0m\n");
        printf("\n\033[1;31;40mTO START LOUVAIN PUT ANY KEY: \033[0m");
        // getchar();

        // TODO Add synchronization here
        printf("\n\033[1;31;40mTO RUNNING LOUVAIN \033[0m\n");
#endif
        // API FOR RUNNING LOUVAIN
    }
    // else if (mode_zmq == ZMQ_WORKER) {
    //     //-----------------------------------------------------------------
    //     // WORKER
    //     //-----------------------------------------------------------------
    // 	p_parlv_wkr->Init(kernelMode, NULL, numPartitions, numDevices, isPrun, par_prune);
    // 	p_parlv_wkr->timesPar.timeAll = getTime();
    //     char LoadCommand[MAX_LEN_MESSAGE];
    //     {
    //         char inFile[1024];
    //         ParLV parlv_tmp;
    //         parlv_tmp.Init(kernelMode, NULL, numPartitions, numDevices, isPrun, par_prune);

    //         status = LouvainGLV_general_top_zmq_worker_new_part1(parlv_tmp, nodeId, (*p_parlv_wkr), NULL);
    //     }
    // }

    return status;
}

void Louvain_thread_core(
    std::shared_ptr<xf::graph::L3::Handle> handle0, int kernelMode, GLV* glv_src, GLV* glv, LouvainPara* para_lv) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FILE__ << "::" << __FUNCTION__ << "\n    kernelMode=" << kernelMode
              << "\n    NVl=" << glv->NVl << std::endl;
#endif
    xf::graph::L3::louvainModularity(handle0, kernelMode, glv_src, glv, para_lv);
}

GLV* L3_LouvainGLV_general(
    int& id_glv, std::shared_ptr<xf::graph::L3::Handle>& handle0, int kernelMode, GLV* glv_src, LouvainPara* para_lv) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << std::endl;
#endif
    GLV* glv_iter;
    std::thread td;

    glv_iter = glv_src->CloneSelf(id_glv);
    assert(glv_iter);
    glv_iter->SetName_lv(glv_iter->ID, glv_src->ID);

    td = std::thread(Louvain_thread_core, handle0, kernelMode, glv_src, glv_iter, para_lv);
    td.join();
    return glv_iter;
}

/*

*/
void Server_SubLouvain(std::shared_ptr<xf::graph::L3::Handle>& handle0,
                       ParLV& parlv,
                       int& id_glv,
                       LouvainPara* para_lv) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FILE__ << "::" << __FUNCTION__ << "\n    handle0=" << handle0
              << " parlv.num_par=" << parlv.num_par << "\n    parlv.num_dev=" << parlv.num_dev
              << "\n    id_glv=" << id_glv << std::endl;
#endif
    parlv.timesPar.timeLv_all = getTime();
    GLV* glv[parlv.num_par];
    std::thread td[parlv.num_par];
    for (int p = 0; p < parlv.num_par; p++) {
        glv[p] = parlv.par_src[p]->CloneSelf(id_glv);
    }
    int cuPerBoard = handle0->oplouvainmod->cuPerBoardLouvainModularity; // cuPerBoard
    int parCnt = 0;
    while (parCnt < parlv.num_par) {
        // Can only launch as many threads as numDevices
        for (int cu = 0; cu < parlv.num_dev * cuPerBoard; cu++) {
            // also need to handle when num_par is not divisible by num_dev
            if (parCnt + cu >= parlv.num_par) break;
            printf("INFO:     start Louvain_thread_core thread %d\n", parCnt + cu);
            parlv.timesPar.timeLv[parCnt + cu] = getTime();
            assert(glv[parCnt + cu]);
            td[parCnt + cu] = std::thread(Louvain_thread_core, handle0, parlv.kernelMode, parlv.par_src[parCnt + cu],
                                          glv[parCnt + cu], para_lv);
            parlv.par_lved[parCnt + cu] = glv[parCnt + cu];
            char tmp_name[1024];
            strcpy(tmp_name, parlv.par_src[parCnt + cu]->name);
            parlv.par_lved[parCnt + cu]->SetName(strcat(tmp_name, "_wrk_lv"));
        }

        for (int cu = 0; cu < parlv.num_dev * cuPerBoard; cu++) {
            if (parCnt + cu >= parlv.num_par) break;
            td[parCnt + cu].join();
            parlv.timesPar.timeLv[parCnt + cu] = getTime() - parlv.timesPar.timeLv[parCnt + cu];
        }
        parCnt += parlv.num_dev * cuPerBoard;
    }

    parlv.st_ParLved = true;
    parlv.timesPar.timeLv_all = getTime() - parlv.timesPar.timeLv_all;
}

void BackAnnotateC(int num_par,
                   GLV* par_src[],
                   long* off_src,
                   long* C_mg_lved,
                   // OUTPUT
                   GLV* plv_src) {
    for (int p = 0; p < num_par; p++) {
        for (long v_sub = 0; v_sub < par_src[p]->NVl; v_sub++) {
            long v_orig = v_sub + off_src[p];
            long v_merged = par_src[p]->C[v_sub];
            plv_src->C[v_orig] = C_mg_lved[v_merged];
        }
    }
}

void BackAnnotateC(ParLV& parlv,
                   // OUTPUT
                   GLV* plv_src) {
    BackAnnotateC(parlv.num_par, parlv.par_src, parlv.off_src, parlv.plv_merged->C, plv_src);
}
void BackAnnotateC(ParLV& parlv) {
    BackAnnotateC(parlv.num_par, parlv.par_src, parlv.off_src, parlv.plv_merged->C, parlv.plv_src);
}

GLV* Driver_Merge_Final_LoacalPar(std::shared_ptr<xf::graph::L3::Handle>& handle0,
                                  ParLV& parlv,
                                  int& id_glv,
                                  LouvainPara* para_lv) {
    parlv.timesPar.timePre = getTime();
#ifdef PRINTINFO_2
    printf("\033[1;37;40mINFO: Now doing PreMerging... \033[0m\n");
#endif
    parlv.PreMerge();
    parlv.timesPar.timePre = getTime() - parlv.timesPar.timePre;
#ifdef PRINTINFO
    printf("\033[1;37;40mINFO\033[0m: Total time for partition pre-Merge : %lf\n", parlv.timesPar.timePre);
#endif
#ifdef PRINTINFO_2
    printf("\033[1;37;40mINFO: Now doing Merging... \033[0m\n");
#endif
    parlv.timesPar.timeMerge = getTime();
    parlv.MergingPar2(id_glv);
    parlv.timesPar.timeMerge = getTime() - parlv.timesPar.timeMerge;
#ifdef PRINTINFO
    printf("\033[1;37;40mINFO\033[0m: Total time for partition Merge : %lf\n", parlv.timesPar.timeMerge);
#endif
    parlv.timesPar.timeFinal = getTime();

// parlv.PrintSelf();
#ifdef PRINTINFO
    parlv.plv_merged->printSimple();
#endif
    if (parlv.st_Merged == false) return NULL;
    assert(parlv.plv_merged);
#ifdef PRINTINFO_2
    printf("\033[1;37;40mINFO: Now doing Final Louvain... \033[0m\n");
#endif

    GLV* glv_final = parlv.plv_merged; //->CloneSelf(id_glv);
#ifdef PRINTINFO_2
    printf("\033[1;37;40mINFO: Now doing BackAnnotationg... \033[0m\n");
#endif

    BackAnnotateC(parlv);
    // printf( "Total time for final : %lf \n" , parlv.timesPar.timeFinal);
    // parlv.plv_src->PushFeature(0, 0, 0.0, true);

    parlv.timesPar.timeFinal = getTime() - parlv.timesPar.timeFinal;

    return glv_final;
}

GLV* louvain_modularity_alveo(std::shared_ptr<xf::graph::L3::Handle>& handle0,
                              ParLV& parlv,     // To collect time and necessary data
                              ParLV& parlv_wkr, // Driver's self data for sub-louvain
                              LouvainPara* para_lv,
                              int numNode,
                              int numPureWorker,
                              char** nameWorkers // To enalbe all workers for Louvain
                              ) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << " handle0=" << handle0 << std::endl;
#endif
    int id_glv = 0;
    int nodeID = 0;
    GLV* glv_final;

    TimePointType l_start = chrono::high_resolution_clock::now();
    TimePointType l_end;

    // Driver* drivers = new Driver[numPureWorker];
    // ConnectWorkers(drivers, numPureWorker, nameWorkers);

    // enalbeAllWorkerLouvain(drivers, numPureWorker);

    TimePointType l_driver_collect_start = chrono::high_resolution_clock::now();
    TimePointType l_driver_collect_end;
    // char msg_w2d[numNode][MAX_LEN_MESSAGE];
    { /////////////// Louvain process on both driver and worker ///////////////
        TimePointType l_par_start = chrono::high_resolution_clock::now();
        TimePointType l_par_end;
        int id_glv = 0;
        bool opts_coloring = true;
        // Louvain
        TimePointType l_compute_start = chrono::high_resolution_clock::now();
        TimePointType l_compute_end;
        Server_SubLouvain(handle0, parlv, id_glv, para_lv);
        getDiffTime(l_compute_start, l_compute_end, parlv_wkr.timesPar.timeWrkCompute[0]);
        /*
            worker: send(save) file////////////////
            parlv_wkr.timesPar.timeWrkSend[0] = 0;

            MessageGen_W2D(msg_w2d[numNode - 1], parlv_wkr, nodeID);

            parlv_wkr.st_ParLved = true;
    #ifdef PRINTINFO
            parlv_wkr.PrintSelf();
    #endif
            getDiffTime(l_par_start, l_par_end, parlv.timesPar.timeLv_all);
        }

        { /////////////// Receive messages and Load file for final Louvain in driver ///////////////
            TimePointType l_receive_start = chrono::high_resolution_clock::now();
            TimePointType l_receive_end;
            // Receive and Parse  messages from worker and driver
            int num_par_sub = 0;
            int num_par_per_worker = parlv.num_par / numNode;

    #pragma omp parallel for
            for (int i = 0; i < numPureWorker; i++) {
                for (int j = 0; j < num_par_per_worker; j++) {
                    receiveGLV_OnlyC(&drivers[i], parlv.par_src[j + i * num_par_per_worker]);
                    parlv.par_lved[j + i * num_par_per_worker] = receiveGLV(&drivers[i], id_glv);
    #ifdef PRINTINFO
                    printf("INFO: Received ID : %d\n", j + i * num_par_per_worker);
    #endif
                }
                drivers[i].receive(msg_w2d[i], MAX_LEN_MESSAGE);
    #ifdef PRINTINFO_2
                printf("INFO: Received from worker:%s (requester%d): %s\n", nameWorkers[i], i, msg_w2d[i]);
    #endif
            }

            for (int i = 0; i < numNode; i++) {
                num_par_sub += MessageParser_W2D(i, msg_w2d[i], parlv);
            }

            for (int p = numPureWorker * num_par_per_worker; p < num_par_sub; p++) {
                parlv.par_src[p] = parlv_wkr.par_src[p - numPureWorker * num_par_per_worker];
                parlv.par_src[p]->ID = p + 1;

                parlv.par_lved[p] = parlv_wkr.par_lved[p - numPureWorker * num_par_per_worker];
                parlv.par_lved[p]->ID = p;
            }
            parlv.num_server = numNode;

            parlv.st_ParLved = true;
    #ifdef PRINTINFO
            parlv.PrintSelf();
    #endif
            delete[] drivers;
            getDiffTime(l_receive_start, l_receive_end, parlv.timesPar.timeDriverRecv);
        }
        getDiffTime(l_driver_collect_start, l_driver_collect_end, parlv.timesPar.timeDriverCollect);
        */
    }
    { /////////////// Merge and Final louvain on driver ///////////////
        TimePointType l_merge_start = chrono::high_resolution_clock::now();
        TimePointType l_merge_end;
        if (parlv.plv_src->C != NULL) free(parlv.plv_src->C);
        parlv.plv_src->C = (long*)malloc(sizeof(long) * parlv.plv_src->NV);
        printf("before merge, org id_glv = %d\n", id_glv);
        glv_final = Driver_Merge_Final_LoacalPar(handle0, parlv, id_glv, para_lv);

        getDiffTime(l_merge_start, l_merge_end, parlv.timesPar.time_done_mg);
    }

    getDiffTime(l_start, l_end, parlv.timesPar.timeAll);

// parlv.plv_src->PushFeature(0, 0, 0.0, true);// calculate Q of results, no need to be included in total time
#ifdef PRINTINFO
    printf("Total time for all flow : %lf \n", parlv.timesPar.timeAll);
#endif
    return glv_final;
    // delete( glv_final);
}

int host_writeOut(const char* opts_inFile, long NV_begin, long* C_orig) {
    if ((opts_inFile == 0) || (C_orig == 0)) {
        printf(
            "\033[1;31;40mERROR: Function host_writeOut got invalid input buff; Writing out "
            "results failed!\n\033[0m");
        return -1;
    }
    char outFile[4096];
    sprintf(outFile, "%s.clustInfo", opts_inFile);
#ifdef PRINTINFO_2
    printf("\033[1;37;40mINFO: Cluster information will be stored in file: %s\n\033[0m", outFile);
#endif
    FILE* out = fopen(outFile, "w");
    for (long i = 0; i < NV_begin; i++) {
        fprintf(out, "%ld %ld\n", i, C_orig[i]);
    }
    fclose(out);
    return 0;
}

//#####################################################################################################################
//
// compute_louvain_alveo_seperated_compute

template <class T>
void PrintArrayByNum(int num, T* array) {
    for (int i = 0; i < num; i++) {
        if (i == 0)
            printf("[");
        else
            printf(", ");
        if (std::is_same<T, int>::value)
            printf("%d", (int)(array[i]));
        else if (std::is_same<T, long>::value)
            printf("%ld", (long)(array[i]));
        else if (std::is_same<T, float>::value)
            printf("%f", (float)(array[i]));
        else if (std::is_same<T, double>::value)
            printf("%lf", (double)(array[i]));
        if (i == num - 1) printf("]\n");
    }
}
template <class T>
T SummArrayByNum(int num, T* array) {
    T ret = 0;
    for (int i = 0; i < num; i++) ret += array[i];
    return ret;
}

int FindMinLevel(ParLV& parlv) {
    int ret = MAX_NUM_PHASE;
    for (int i = 0; i < parlv.num_par; i++)
        ret = ret > parlv.par_src[i]->times.phase ? parlv.par_src[i]->times.phase : ret;
    return ret;
}

int FindMaxLevel(ParLV& parlv) {
    int ret = 0;
    for (int i = 0; i < parlv.num_par; i++)
        ret = ret < parlv.par_src[i]->times.phase ? parlv.par_src[i]->times.phase : ret;
    return ret;
}

void PrintRptPartition(int mode_zmq, ParLV& parlv, int op0_numDevices, int numNode, int numPureWorker) {
    if (mode_zmq != ZMQ_WORKER) {
        printf("************************************************************************************************\n");
        printf(
            "******************************  \033[1;35;40mPartition Louvain Performance\033[0m   "
            "********************************\n");
        printf("************************************************************************************************\n");
        printf("\033[1;37;40mINFO\033[0m: Original number of vertices            : %ld\n", parlv.plv_src->NV);
        printf("\033[1;37;40mINFO\033[0m: Original number of un-direct edges     : %ld\n", parlv.plv_src->NE);
        printf("\033[1;37;40mINFO\033[0m: number of partition                    : %d \n", parlv.num_par);
        printf("\033[1;37;40mINFO\033[0m: number of device used                  : %d \n", op0_numDevices);
        printf("\033[1;37;40mINFO\033[0m: Final number of communities            : %ld\n", parlv.plv_src->NC);
        printf("\033[1;37;40mINFO\033[0m: Final modularity                       : %lf\n",
               parlv.plv_src->Q); // com_lit.back().
        printf("************************************************************************************************\n");

        if (mode_zmq == ZMQ_NONE) {
            double totTimeOnDev[parlv.num_dev];
            double totTimeFPGA_pure = 0;
            double totTimeFPGA_wait = 0;
            double totTimeCPU = 0;
            for (int d = 0; d < op0_numDevices; d++) totTimeOnDev[d] = 0;
            for (int p = 0; p < parlv.num_par; p++) {
                for (int d = 0; d < op0_numDevices; d++) {
                    totTimeOnDev[d] += parlv.par_src[p]->times.totTimeE2E_DEV[d];
                    totTimeFPGA_pure += parlv.par_src[p]->times.totTimeE2E_DEV[d];
                }
                totTimeFPGA_wait += parlv.par_src[p]->times.totTimeE2E_2;
                totTimeCPU += parlv.par_src[p]->times.totTimeAll - parlv.par_src[p]->times.totTimeE2E;
            }
            totTimeFPGA_pure += parlv.plv_merged->times.totTimeE2E;
            totTimeFPGA_wait += parlv.plv_merged->times.totTimeE2E_2;
            totTimeCPU += parlv.plv_merged->times.totTimeAll - parlv.plv_merged->times.totTimeE2E;
            printf("\t--- Total for All CPU time (s)       : %lf\t=\t", totTimeCPU);
            for (int p = 0; p < parlv.num_par; p++)
                printf(" +%lf (par-%d) ", parlv.par_src[p]->times.totTimeAll - parlv.par_src[p]->times.totTimeE2E, p);
            printf(" +%lf (Final) \n", parlv.plv_merged->times.totTimeAll - parlv.plv_merged->times.totTimeE2E);
            printf("\t--- Total for All FPGA time (s)      : %lf\t=\t", totTimeFPGA_pure);
            for (int p = 0; p < parlv.num_par; p++) printf(" +%lf (par-%d) ", parlv.par_src[p]->times.totTimeE2E, p);
            printf(" +%lf (Final) \n", parlv.plv_merged->times.totTimeE2E);

            for (int d = 0; d < op0_numDevices; d++)
                printf("\t--- Sub-Louvain on Dev-%d             : %lf\n", d, totTimeOnDev[d]);
            printf("\t--- Fnl-Louvain on Dev-0             : %lf\n", parlv.plv_merged->times.totTimeE2E);
            printf(
                "\t--- FPGA efficiency with %d device(s) : %2.2f\n", op0_numDevices,
                (totTimeFPGA_pure * 100.0) / (op0_numDevices * (parlv.timesPar.timeAll - parlv.timesPar.timePar_all)));
            printf(
                "************************************************************************************************\n");
        } // end ZMQ_NONE

        printf("************************************************************************************************\n");
        printf("***********************************  Louvain Summary   *****************************************\n");
        printf("************************************************************************************************\n");
        printf(
            "\033[1;31;40m    1.   Time for loading partitions \033[0m                               : "
            "\033[1;31;40m%lf\033[0m (s)\n",
            parlv.timesPar.timeDriverLoad);
        printf(
            "\033[1;31;40m    2.   Time for computing distributed Louvain:  \033[0m                  : "
            "\033[1;31;40m%lf\033[0m (s) =",
            parlv.timesPar.timeDriverExecute);
        printf(" Time for sub-Louvain&transmission + Driver Time for Merge&Final-Louvain\n");
        printf("    2.1   Time for sub-Louvain&transmission                          : %lf (s) = ",
               parlv.timesPar.timeDriverCollect);
        printf(" max( driver, worker-1, worker-2)\n");

        printf("    2.1.1 Driver Time for sub-Louvain and results receiving          : %lf (s) = ",
               parlv.timesPar.timeDriverCollect);
        printf(" %lf (sub-lv) + %lf (waiting & recv) \n", parlv.timesPar.timeWrkCompute[numNode - 1],
               parlv.timesPar.timeDriverCollect - parlv.timesPar.timeWrkCompute[numNode - 1]);
        printf("    2.1.2 Worker Time for sub-Louvain and results sending            : %lf (s) = ",
               parlv.timesPar.timeWrkCompute[0] + parlv.timesPar.timeWrkSend[0]);
        printf(" %lf (sub-lv) + %lf (send) \n", parlv.timesPar.timeWrkCompute[0], parlv.timesPar.timeWrkSend[0]);
        printf("    2.1.3 Worker Time for sub-Louvain and results sending            : %lf (s) = ",
               parlv.timesPar.timeWrkCompute[1] + parlv.timesPar.timeWrkSend[1]);
        printf(" %lf (sub-lv) + %lf (send) \n", parlv.timesPar.timeWrkCompute[1], parlv.timesPar.timeWrkSend[1]);

        printf("    2.2   Driver Time for Merge&Final-Louavin                        : %lf (s) = ",
               parlv.timesPar.time_done_mg);
        printf(" pre-Merge + Merge + Final-Louvain \n");
        printf("    2.2.1 Driver: pre-Merge                                          : %lf (s)\n",
               parlv.timesPar.timePre);
        printf("    2.2.2 Driver: Merge                                              : %lf (s)\n",
               parlv.timesPar.timeMerge);
        printf("    2.2.3 Driver: Final-Louvain                                      : %lf (s)\n",
               parlv.timesPar.timeFinal);
    }
    printf("************************************************************************************************\n");
}

// Summary
// Number of vertices           : 30
// Number of edges              : 15
// Number of partitions         : 10
// Partition size               : 3
// Number of nodes (machines)   : 3
// Number of Xilinx Alveo cards : 9
// Number of levels             : 3
// Delta Q tolerance            : 0.0001
// Number of iterations         : 20 [15, 10, 5]
// Number of communities        : 12 [7, 3, 2]
// Modularity                   : 0.84 [0.64, 0.77, 0.84]
// Time                         : 9 sec (Partition Compute: 7 sec, Merge: 1 sec, Merge Compute: 1 sec)
void PrintRptPartition_Summary(ParLV& parlv,
                               // int numNode, int* card_Node,
                               double opts_C_thresh) {
    int num_par = parlv.num_par;
    int numNode = parlv.num_server;
    int* card_Node = parlv.numServerCard;

    printf("****************************************Summary*************************************************\n");
    printf("Number of vertices           : %ld\n", parlv.plv_src->NV);
    printf("Number of edges              : %ld\n", parlv.plv_src->NE);
    printf("Number of partitions         : %d\n", parlv.num_par);
    printf("Partition size               : < %ld\n", glb_MAXNV_M);
    printf("Number of nodes (machines)   : %d\n", numNode);
    printf("Number of Xilinx Alveo cards : %d", SummArrayByNum<int>(numNode, card_Node));
    PrintArrayByNum<int>(numNode, card_Node); //[3, 3,3]
    printf("Number of level [Min, Max]   : [ %d, %d ]\n", FindMinLevel(parlv), FindMaxLevel(parlv));
    printf(
        "Number of iterations         : Each partition has its own iteration number for each level, so please check "
        "the Partition Summary\n");
    printf("Delta Q tolerance            : %lf\n", opts_C_thresh);
    printf("Number of communities        : %ld\n", parlv.plv_src->NC);
    printf("Modularity                   : %lf\n", parlv.plv_src->Q);

    printf("Time                         : %4.3f sec ", parlv.timesPar.timeDriverExecute);
    printf(" (Partition Compute: %4.3f sec", parlv.timesPar.timeDriverCollect);
    printf(", Merge: %4.3f sec", parlv.timesPar.timePre + parlv.timesPar.timeMerge);
    printf(", Merge Compute: %4.3f sec)", parlv.timesPar.timeFinal);
    printf("\n");
    printf("********************************Partition Summary***********************************************\n");
    for (int p = 0; p < num_par; p++) {
        printf("\tPARTITION-%d :\n ", p);
        int numLevel = parlv.par_src[p]->times.phase;
        printf("\t\tNumber of levels             : %d\n", numLevel);

        printf("\t\tNumber of iterations         : %d ", parlv.par_src[p]->times.totItr);
        PrintArrayByNum<int>(numLevel, parlv.par_src[p]->times.eachItrs); //[15, 10, 5]

        printf("\t\tNumber of communities        : %ld ", parlv.par_src[p]->NC);
        PrintArrayByNum<long>(numLevel, parlv.par_src[p]->times.eachClusters); //[7, 3, 2]

        printf("\t\tModularity                   : %lf ", parlv.par_src[p]->Q);
        PrintArrayByNum<double>(numLevel, parlv.par_src[p]->times.eachMod); //[7, 3, 2]//[0.64, 0.77, 0.84]
    }
    printf("************************************************************************************************\n");
}

float compute_louvain_alveo_seperated_compute(int mode_zmq,
                                              int numPureWorker,
                                              char* nameWorkers[128],
                                              unsigned int nodeID,
                                              char* opts_outputFile,
                                              unsigned int max_iter,
                                              unsigned int max_level,
                                              float tolerance,
                                              bool intermediateResult,
                                              bool verbose,
                                              bool final_Q,
                                              bool all_Q,
                                              std::shared_ptr<xf::graph::L3::Handle>& handle0,
                                              ParLV* p_parlv_dvr,
                                              ParLV* p_parlv_wkr) {
#ifndef NDEBUG
    printf("DEBUG:  compute_louvain_alveo_seperated_compute");
    printf("\n    mode_zmq=%d", mode_zmq);
    printf("\n    numPureWorker=%d", numPureWorker);
    printf("\n    nodeID=%d", nodeID);
    printf("\n    opts_outputFile=%s", opts_outputFile);
    printf("\n    max_iter=%d", max_iter);
    printf("\n    max_level=%d", max_level);
    printf("\n    tolerance=%f", tolerance);
    printf("\n    intermediateResult=%d", intermediateResult);
    printf("\n    verbose=%d", verbose);
    printf("\n    final_Q=%d", final_Q);
    printf("\n    all_Q=%d\n", all_Q);

    for (int i = 0; i < numPureWorker; i++)
        std::cout << "DEBUG: nameWorker " << i << "=" << nameWorkers[i] << std::endl;

#endif
    // TODO: We should remove globals as this is quite confusing
    int mode_alveo = ALVEOAPI_RUN;
    double opts_C_thresh = tolerance; // Threshold with coloring on
    long opts_minGraphSize = 100;     // Min |V| to enable coloring
    double opts_threshold = 0.000001; // Value of threshold
    int numThreads = 16;
    int numNode = numPureWorker + 1;

    LouvainPara* para_lv = new (LouvainPara);
    para_lv->opts_coloring = true;
    para_lv->opts_minGraphSize = opts_minGraphSize;
    para_lv->opts_threshold = opts_threshold;
    para_lv->opts_C_thresh = opts_C_thresh;
    para_lv->numThreads = numThreads;
    para_lv->max_num_level = max_level;
    para_lv->max_num_iter = max_iter;

    if (mode_alveo == ALVEOAPI_RUN) {
        if (mode_zmq == ZMQ_DRIVER) {
            // API FOR RUNNING LOUVAIN
            GLV* glv_final;
            TimePointType l_execute_start = chrono::high_resolution_clock::now();
            TimePointType l_execute_end;

            glv_final = louvain_modularity_alveo(handle0, *p_parlv_dvr, *p_parlv_wkr, para_lv, numNode, numPureWorker,
                                                 nameWorkers);

            getDiffTime(l_execute_start, l_execute_end, p_parlv_dvr->timesPar.timeDriverExecute);
            glv_final->PushFeature(0, 0, 0.0, true);
            p_parlv_dvr->plv_src->Q = glv_final->Q;
            p_parlv_dvr->plv_src->NC = glv_final->NC;
            PrintRptPartition(mode_zmq, *p_parlv_dvr, p_parlv_dvr->num_dev, numNode, numPureWorker);
            PrintRptPartition_Summary(*p_parlv_dvr, opts_C_thresh);
            std::string outputFileName(opts_outputFile);
            if (!outputFileName.empty()) {
                host_writeOut(outputFileName.c_str(), p_parlv_dvr->plv_src->NV, p_parlv_dvr->plv_src->C);
            } else {
#ifdef PRINTINFO_2
                printf("\033[1;37;40mINFO: Please use -o <output file> to store Cluster information\033[0m\n");
#endif
            }
#ifdef PRINTINFO
            printf("Deleting orignal graph... \n");
#endif
#ifdef PRINTINFO
            p_parlv_dvr->plv_src->printSimple();
#endif
            glv_final->printSimple();
            double ret = glv_final->Q;
            return ret;
        }
        //  else if (mode_zmq == ZMQ_WORKER) {
        //     LouvainGLV_general_top_zmq_worker_new_part2(handle0, para_lv, nodeID, *p_parlv_wkr);
        // }
    }

    return 0;
}

/*
loadAlveoAndComputeLouvain
Return values:
    -1 to 1: Modularity value
    -2: Error in getNumPartitions
    -3: Error in compute_louvain_alveo_seperated_load
    -4: Error in createSharedHandle
*/
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
                                 bool all_Q) {
    ParLV parlv_drv, parlv_wkr;
    float ret = 0;

    bool opts_coloring = false;
    long opts_minGraphSize = 10;      // Min |V| to enable coloring
    double opts_C_thresh = tolerance; // Threshold with coloring on
    int numThreads = 16;

    int numPartitions = getNumPartitions(alveoProject);

    if (numPartitions < 0) // return error code directly
        return numPartitions;

    // Allocating memory for load
    if (mode_zmq == ZMQ_DRIVER) {
        int id_glv = 0;
        for (int i = 0; i < numPartitions; i++) {
            parlv_drv.par_src[i] = new GLV(id_glv);
        }
    }

    int status = createSharedHandle(xclbinPath, kernelMode, numDevices, deviceNames, opts_coloring, opts_minGraphSize,
                                    opts_C_thresh, numThreads);
    if (status < 0) return status;

    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesLouvainMod::instance().handlesMap[0];
    ret = compute_louvain_alveo_seperated_load(kernelMode, numDevices, numPartitions, alveoProject, mode_zmq,
                                               numPureWorker, nameWorkers, nodeID, tolerance, verbose, handle0,
                                               &parlv_drv, &parlv_wkr);

    // return right away if load returns an error code
    if (ret < 0) return ERRORCODE_COMPUTE_LOUVAIN_ALVEO_SEPERATED_LOAD;

    ret = compute_louvain_alveo_seperated_compute(mode_zmq, numPureWorker, nameWorkers, nodeID, opts_outputFile,
                                                  max_iter, max_level, tolerance, intermediateResult, verbose, final_Q,
                                                  all_Q, handle0, &parlv_drv, &parlv_wkr);

    return ret;
}

#endif
