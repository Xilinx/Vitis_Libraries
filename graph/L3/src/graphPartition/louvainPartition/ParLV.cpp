/*
 * Copyright 2019-2021 Xilinx, Inc.
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
#include <thread>
#include <chrono>

#include "defs.hpp"
#include "ParLV.hpp"
#include "partitionLouvain.hpp"

// set default global max values for U50
long glb_MAXNV = (1ul << 26);
long glb_MAXNE = (1 << 27);
long glb_MAXNV_M = (1 << 27);

using namespace std;

// time functions

typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePointType;

double getTime() {
    TimePointType t1 = chrono::high_resolution_clock::now();
    chrono::duration<double> l_durationSec = chrono::duration<double>(t1.time_since_epoch());
    return l_durationSec.count();
}

// time functions end

// load/send GLV through data files
int SaveGLVBin(char* name, GLV* glv) {
    assert(name);
    assert(glv);
    graphNew* g = glv->G;
    long nv = g->numVertices;
    long ne = g->numEdges;
    long ne_undir = g->edgeListPtrs[nv];
    long nc = glv->NC;
    double Q = glv->Q;
    long nvl = glv->NVl;
    long nelg = glv->NElg;
    FILE* fp = fopen(name, "wb");
    if (fp == NULL) {
        printf("ERROR: SaveGLVBin failed to open %s \n", name);
        return -1;
    }
    fwrite(&headGLVBin, sizeof(long), 1, fp);
    fwrite(&nv, sizeof(long), 1, fp);
    fwrite(&ne, sizeof(long), 1, fp);
    fwrite(&ne_undir, sizeof(long), 1, fp);
    fwrite(&nc, sizeof(long), 1, fp);
    fwrite(&Q, sizeof(double), 1, fp);
    fwrite(&nvl, sizeof(long), 1, fp);
    fwrite(&nelg, sizeof(long), 1, fp);
    fwrite(g->edgeListPtrs, sizeof(long), nv + 1, fp);
    fwrite(g->edgeList, sizeof(edge), ne_undir, fp);
    fwrite(glv->M, sizeof(long), nv, fp);
    fwrite(glv->C, sizeof(long), nv, fp);
    fclose(fp);
#ifdef PRINTINFO
    printf("INFO: SaveGLVBin %s Successfully nv=%ld ne=%ld undir ne=%ld nc=%ld Q=%lf \n", name, nv, ne, ne_undir, nc,
           Q);
#endif

    return 0;
}

long UseInt(long nv, long* src, FILE* fp) {
    int* tmp = (int*)malloc(sizeof(int) * nv);
    for (int i = 0; i < nv; i++) tmp[i] = src[i];
    long ret = fwrite(tmp, sizeof(int), nv, fp);
    free(tmp);

    return ret;
}

int SaveGLVBin(char* name, GLV* glv, bool useInt) {
#ifndef NDEBUG
    std::cout << "DEBUG:" << __FUNCTION__ << " name=" << name << std::endl;
#endif
    assert(name);
    assert(glv);
    graphNew* g = glv->G;
    long nv = g->numVertices;
    long ne = g->numEdges;
    long ne_undir = g->edgeListPtrs[nv];
    long nc = glv->NC;
    double Q = glv->Q;
    long nvl = glv->NVl;
    long nelg = glv->NElg;
    FILE* fp = fopen(name, "wb");
    if (fp == NULL) {
        printf("ERROR: SaveGLVBin failed to open %s \n", name);
        return -1;
    }
    fwrite(&headGLVBin, sizeof(long), 1, fp);
    fwrite(&nv, sizeof(long), 1, fp);
    fwrite(&ne, sizeof(long), 1, fp);
    fwrite(&ne_undir, sizeof(long), 1, fp);
    fwrite(&nc, sizeof(long), 1, fp);
    fwrite(&Q, sizeof(double), 1, fp);
    fwrite(&nvl, sizeof(long), 1, fp);
    fwrite(&nelg, sizeof(long), 1, fp);
    fwrite(g->edgeListPtrs, sizeof(long), nv + 1, fp);
    fwrite(g->edgeList, sizeof(edge), ne_undir, fp);
    fwrite(glv->M, sizeof(long), nv, fp);
    fwrite(glv->C, sizeof(long), nv, fp);
    fclose(fp);
#ifdef PRINTINFO
    printf("INFO: SaveGLVBin %s Successfully nv=%ld ne=%ld undir ne=%ld nc=%ld Q=%lf \n", name, nv, ne, ne_undir, nc,
           Q);
#endif
    return 0;
}
int SaveGLVBin_OnlyC(char* name, GLV* glv, bool useInt) {
    assert(name);
    assert(glv);
    graphNew* g = glv->G;
    long nv = g->numVertices;
    long nc = glv->NC;
    double Q = glv->Q;
    long nvl = glv->NVl;
    long nelg = glv->NElg;
    FILE* fp = fopen(name, "wb");
    if (fp == NULL) {
        printf("ERROR: SaveGLVBin_OnlyC failed for open %s \n", name);
        return -1;
    }
    fwrite(&headGLVBin, sizeof(long), 1, fp);
    fwrite(&nv, sizeof(long), 1, fp);
    fwrite(&nc, sizeof(long), 1, fp);
    fwrite(&Q, sizeof(double), 1, fp);
    fwrite(&nvl, sizeof(long), 1, fp);
    fwrite(&nelg, sizeof(long), 1, fp);
    if (useInt) {
        int* tmp = (int*)malloc(sizeof(int) * nv);
        for (int i = 0; i < nv; i++) tmp[i] = glv->C[i];
        fwrite(tmp, sizeof(int), nv, fp);
        free(tmp);
    } else
        fwrite(glv->C, sizeof(long), nv, fp);
    fclose(fp);
#ifdef PRINTINFO
    printf("INFO: SaveGLVBin_OnlyC %s nv=%ld nc=%ld Q=%lf Successfully \n", name, nv, nc, Q);
#endif
    return 0;
}
int SaveGLVBin_OnlyC(char* name, GLV* glv) {
    assert(name);
    assert(glv);
    graphNew* g = glv->G;
    long nv = g->numVertices;
    long nc = glv->NC;
    double Q = glv->Q;
    long nvl = glv->NVl;
    long nelg = glv->NElg;
    FILE* fp = fopen(name, "wb");
    if (fp == NULL) {
        printf("ERROR: SaveGLVBin_OnlyC failed for open %s \n", name);
        return -1;
    }
    fwrite(&headGLVBin, sizeof(long), 1, fp);
    fwrite(&nv, sizeof(long), 1, fp);
    fwrite(&nc, sizeof(long), 1, fp);
    fwrite(&Q, sizeof(double), 1, fp);
    fwrite(&nvl, sizeof(long), 1, fp);
    fwrite(&nelg, sizeof(long), 1, fp);
    fwrite(glv->C, sizeof(long), nv, fp);
    fclose(fp);
#ifdef PRINTINFO
    printf("INFO: SaveGLVBin_OnlyC %s nv=%ld nc=%ld Q=%lf Successfully \n", name, nv, nc, Q);
#endif
    return 0;
}

int SaveGLVBinBatch(GLV* glv[], int num_par, const char* path, bool useInt) {
    assert(glv);
    assert(num_par < MAX_PARTITION);
    int ret = 0;
    for (int i = 0; i < num_par; i++) {
        char pathName[1024];
        if (strlen(path)) {
            strcpy(pathName, path);
            strcat(pathName, "/");
        } else
            strcpy(pathName, "./");
        strcat(pathName, glv[i]->name);
        ret += SaveGLVBin(pathName, glv[i], useInt);
    }
    return ret;
}

int SaveGLVBinBatch_OnlyC(GLV* glv[], int num_par, const char* path) {
    assert(glv);
    assert(num_par < MAX_PARTITION);
    int ret = 0;
    for (int i = 0; i < num_par; i++) {
        char pathName[1024];
        if (strlen(path)) {
            strcpy(pathName, path);
            strcat(pathName, "/");
        } else
            strcpy(pathName, "./");
        strcat(pathName, glv[i]->name);
        ret += SaveGLVBin_OnlyC(pathName, glv[i]);
    }
    return ret;
}

// functions for printing info or read command lines
int general_findPara(int argc, char** argv, const char* para) {
    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], para)) return i;
    }
    return -1;
}
void ParameterError(const char* msg) {
    printf("\033[1;31;40mPARAMETER ERROR\033[0m: %s \n", msg);
    exit(1);
}

int host_ParserParameters(int argc,
                          char** argv,
                          double& opts_C_thresh,   // Threshold with coloring on
                          long& opts_minGraphSize, // Min |V| to enable coloring
                          double& opts_threshold,  // Value of threshold
                          int& opts_ftype,         // File type
                          char opts_inFile[4096],  //
                          bool& opts_coloring,     //
                          bool& opts_output,       //
                          std::string& opts_outputFile,
                          bool& opts_VF,            //;
                          std::string& xclbinFile,  // Full path including filename to the xclbin files to be loaded
                          std::string& deviceNames, // Target device names
                          int& numThread,
                          int& numPars,
                          // bool& BFS_partition,
                          int& gh_par,
                          int& kernelMode,
                          int& numDevices,
                          int& mode_zmq,
                          char* path_zmq,
                          bool& useCmd,
                          int& mode_alveo,
                          char* nameProj,
                          std::string& nameMetaFile,
                          int& numPureWorker,
                          char* nameWorkers[128],
                          int& nodeID,
                          int& numNodes,
                          int& max_num_level,
                          int& max_num_iter) {
    const int max_parameter = 100;
    bool rec[max_parameter];
    for (int i = 1; i < argc; i++) rec[i] = false;
    int has_opts_C_thresh = general_findPara(argc, argv, "-d");
    int has_opts_minGraphSize = general_findPara(argc, argv, "-m");
    int has_opts_threshold = general_findPara(argc, argv, "-t");
    int has_opts_ftype = general_findPara(argc, argv, "-f");
    int has_opts_inFile; //= general_findPara(argc, argv, "-thread");
    int has_opts_coloring = general_findPara(argc, argv, "-c");
    int has_opts_output = general_findPara(argc, argv, "-o");
    int has_opts_VF = general_findPara(argc, argv, "-v");
    int hasXclbinPath = general_findPara(argc, argv, "-x");
    int hasDeviceNames = general_findPara(argc, argv, "-devices");
    int has_numThread = general_findPara(argc, argv, "-thread");
    int hasNumPars = general_findPara(argc, argv, "-num_pars");
    int has_gh_par = general_findPara(argc, argv, "-par_prune");
    int hasKernelMode = general_findPara(argc, argv, "-kernel_mode");
    int hasNumDevices = general_findPara(argc, argv, "-num_devices");
    int hasNumCu = general_findPara(argc, argv, "-num_cu");
    int has_driver = general_findPara(argc, argv, "-driver");
    int has_worker = general_findPara(argc, argv, "-worker");
    int has_driverAlone = general_findPara(argc, argv, "-driverAlone");
    int has_workerAlone = general_findPara(argc, argv, "-workerAlone");
    int has_cmd = general_findPara(argc, argv, "-cmd");
    int hasNumNodes = general_findPara(argc, argv, "-num_nodes");
    int has_max_num_level = general_findPara(argc, argv, "-num_level");
    int has_max_num_iter = general_findPara(argc, argv, "-num_iter");

    if (general_findPara(argc, argv, "-create_alveo_partitions") != -1) {
        mode_alveo = ALVEOAPI_PARTITION;
        // BFS_partition = false;
        int indx = general_findPara(argc, argv, "-name");
        if (argc > indx && indx != -1) strcpy(nameProj, argv[indx + 1]);
    } else if (general_findPara(argc, argv, "-create_alveo_BFS_partitions") != -1) {
        mode_alveo = ALVEOAPI_PARTITION_BFS;
        // BFS_partition = true;
        int indx = general_findPara(argc, argv, "-name");
        if (argc > indx && indx != -1) strcpy(nameProj, argv[indx + 1]);
    } else if (general_findPara(argc, argv, "-load_alveo_partitions") != -1) {
        int indx = general_findPara(argc, argv, "-load_alveo_partitions") + 1;
        mode_alveo = ALVEOAPI_LOAD;
        if (argc > indx)
            nameMetaFile = argv[indx];
        else {
            printf("\033[1;31;40mPARAMETER ERROR\033[0m: -load_alveo_partitions <Project Metadata file> \n");
            return -1;
        }
        if (general_findPara(argc, argv, "-setwkr") == -1) {
            numPureWorker = 0;
        } else { //[-setwkr <numWorkers> <worker name> [<worker name>] ]
            int indx2 = general_findPara(argc, argv, "-setwkr") + 1;
            if (argc <= indx2) ParameterError("-setwkr <numWorkers> <worker name> [<worker name>] ]");
            numPureWorker = atoi(argv[indx2++]);
            if (argc < indx2 + numPureWorker) ParameterError("-setwkr <numWorkers> <worker name> [<worker name>] ]");
            for (int i = 0; i < numPureWorker; i++) {
                nameWorkers[i] = argv[indx2 + i];
            }
        }

        if ((has_driverAlone == -1) && (has_workerAlone == -1)) {
            mode_zmq = ZMQ_NONE;
        } else {
            if (has_driverAlone != -1) {
                mode_zmq = ZMQ_DRIVER;
            } else {
                mode_zmq = ZMQ_WORKER;
                if (argc > has_workerAlone + 1)
                    nodeID = atoi(argv[has_workerAlone + 1]);
                else {
                    printf(
                        "\033[1;31;40mPARAMETER ERROR\033[0m: -load_alveo_partitions -worker "
                        "\033[1;31;40m<nodeID>\033[0m missed \n");
                    exit(1);
                }
            }
        }
    } else if (general_findPara(argc, argv, "-louvain_modularity_alveo") != -1)
        mode_alveo = ALVEOAPI_RUN;
    else
        mode_alveo = ALVEOAPI_NONE;

    if (has_cmd != -1)
        useCmd = true;
    else
        useCmd = false;

    if (mode_alveo == ALVEOAPI_NONE) {
        if ((has_driver == -1) && (has_worker == -1)) {
            mode_zmq = ZMQ_NONE;
        } else {
            if (has_driver != -1) {
                mode_zmq = ZMQ_DRIVER;
                if (argc > has_driver + 1)
                    strcpy(path_zmq, argv[has_driver + 1]);
                else
                    strcpy(path_zmq, "./");
            } else {
                mode_zmq = ZMQ_WORKER;
                if (argc > has_worker + 1)
                    strcpy(path_zmq, argv[has_driver + 1]);
                else
                    strcpy(path_zmq, "./");
            }
        }
    }

    if (has_opts_C_thresh != -1 && has_opts_C_thresh < (argc - 1)) {
        rec[has_opts_C_thresh] = true;
        rec[has_opts_C_thresh + 1] = true;
        opts_C_thresh = atof(argv[has_opts_C_thresh + 1]);
    } else
        opts_C_thresh = 0.0002;
#ifdef PRINTINFO
    printf("PARAMETER  opts_C_thresh = %f\n", opts_C_thresh);
#endif
    if (has_opts_minGraphSize != -1 && has_opts_minGraphSize < (argc - 1)) {
        rec[has_opts_minGraphSize] = true;
        rec[has_opts_minGraphSize + 1] = true;
        opts_minGraphSize = atoi(argv[has_opts_minGraphSize + 1]);
    } else
        opts_minGraphSize = 10;
#ifdef PRINTINFO
    printf("PARAMETER  has_opts_minGraphSize= %ld\n", opts_minGraphSize);
#endif
    if (has_opts_threshold != -1 && has_opts_threshold < (argc - 1)) {
        rec[has_opts_threshold] = true;
        rec[has_opts_threshold + 1] = true;
        opts_threshold = atof(argv[has_opts_threshold + 1]);
    } else
        opts_threshold = 0.000001;
#ifdef PRINTINFO
    printf("PARAMETER  opts_C_thresh= %f\n", opts_C_thresh);
#endif
    if (has_opts_ftype != -1 && has_opts_ftype < (argc - 1)) {
        rec[has_opts_ftype] = true;
        rec[has_opts_ftype + 1] = true;
        opts_ftype = atof(argv[has_opts_ftype + 1]);
    } else
        opts_ftype = 3;
#ifdef PRINTINFO
    printf("PARAMETER  opts_ftype = %i\n", opts_ftype);
#endif
    if (has_opts_coloring != -1) {
        rec[has_opts_coloring] = true;
        opts_coloring = true;
    }
#ifdef PRINTINFO
    printf("PARAMETER  opts_coloring = %d\n", opts_coloring);
#endif
    opts_output = false;
    if (has_opts_VF != -1) {
        rec[has_opts_VF] = true;
        opts_VF = true;
    }
#ifdef PRINTINFO
    printf("PARAMETER  opts_VF = %d\n", opts_VF);
#endif

    if (hasXclbinPath != -1 && hasXclbinPath < (argc - 1)) {
        rec[hasXclbinPath] = true;
        rec[hasXclbinPath + 1] = true;
        xclbinFile = argv[hasXclbinPath + 1];
    }

    if (hasDeviceNames != -1 && hasDeviceNames < (argc - 1)) {
        rec[hasDeviceNames] = true;
        rec[hasDeviceNames + 1] = true;
        deviceNames = argv[hasDeviceNames + 1];
#ifndef NDEBUG
        std::cout << "INFO: deviceNames=" << deviceNames << std::endl;
#endif
    } else {
        deviceNames = "xilinx_u50_gen3x16_xdma_201920_3";
#ifdef PRINTINFO
        printf("Using defalut device xilinx_u50_gen3x16_xdma_201920_3, because of the missing deviceNames.\n");
#endif
    }

    if (has_numThread != -1 && has_numThread < (argc - 1)) {
        rec[has_numThread] = true;
        rec[has_numThread + 1] = true;
        numThread = atoi(argv[has_numThread + 1]);
    } else
        numThread = 16;

    if (hasNumPars != -1 && hasNumPars < (argc - 1)) {
        rec[hasNumPars] = true;
        rec[hasNumPars + 1] = true;
        numPars = atoi(argv[hasNumPars + 1]);
    } else
        numPars = 2;

    if (hasNumDevices != -1 && hasNumDevices < (argc - 1)) {
        rec[hasNumDevices] = true;
        rec[hasNumDevices + 1] = true;
        numDevices = atoi(argv[hasNumDevices + 1]);
    } else
        numDevices = 1;

    int numCu = 1;
    if (hasNumCu != -1 && hasNumCu < (argc - 1)) {
        rec[hasNumCu] = true;
        rec[hasNumCu + 1] = true;
        numCu = atoi(argv[hasNumCu + 1]);
#ifdef PRINTINFO
        printf("PARAMETER  numCu = %i\n", numCu);
#endif
    } else
        numCu = 1;

    if (has_gh_par != -1 && has_gh_par < (argc - 1)) {
        rec[has_gh_par] = true;
        rec[has_gh_par + 1] = true;
        gh_par = atoi(argv[has_gh_par + 1]);
    } else
        gh_par = 1;

// Kernel mode handling
#ifdef PRINTINFO
    printf("PARAMETER  gh_par = %i\n", gh_par);
#endif
    if (hasKernelMode != -1 && hasKernelMode < (argc - 1)) {
        rec[hasNumDevices] = true;
        rec[hasNumDevices + 1] = true;
        kernelMode = atoi(argv[hasKernelMode + 1]);
    } else
        kernelMode = 1;

#ifndef NDEBUG
    printf("PARAMETER  kernelMode=%d\n", kernelMode);
#endif
    if (has_opts_output != -1 && has_opts_output < (argc - 1)) {
        opts_output = true;
        rec[has_opts_output] = true;
        rec[has_opts_output + 1] = true;

        opts_outputFile = argv[has_opts_output + 1];
    }

    if (hasNumNodes != -1 && hasNumNodes < (argc - 1)) {
        rec[hasNumNodes] = true;
        rec[hasNumNodes + 1] = true;
        numNodes = atoi(argv[hasNumNodes + 1]);
    } else
        numNodes = 1;

    if (has_max_num_level != -1 && has_max_num_level < (argc - 1)) {
        rec[has_max_num_level] = true;
        rec[has_max_num_level + 1] = true;
        max_num_level = atoi(argv[has_max_num_level + 1]);
    } else
        max_num_level = MAX_NUM_PHASE;
#ifdef PRINTINFO
    printf("PARAMETER max_num_level = %i\n", max_num_level);
#endif

    if (has_max_num_iter != -1 && has_max_num_iter < (argc - 1)) {
        rec[has_max_num_iter] = true;
        rec[has_max_num_iter + 1] = true;
        max_num_iter = atoi(argv[has_max_num_iter + 1]);
    } else
        max_num_iter = MAX_NUM_TOTITR;
#ifdef PRINTINFO
    printf("PARAMETER max_num_iter = %i\n", max_num_iter);
#endif

    if (mode_alveo == ALVEOAPI_LOAD) return 0; // No need to set input matrix file if

    for (int i = 1; i < argc; i++) {
        // printf("i= %d rec[i]=%d\n", i , rec[i]);
        if (rec[i] == false) {
            has_opts_inFile = i;
            strcpy(opts_inFile, argv[has_opts_inFile]);
#ifdef PRINTINFO
            printf("PARAMETER opts_inFile = %s\n", opts_inFile);
#endif
            FILE* file = fopen(opts_inFile, "r");
            if (file == NULL) {
                printf("\033[1;31;40mPARAMETER ERROR\033[0m: Cannot open the batch file: %s\n", opts_inFile);
                exit(1);
            } else
                fclose(file);
            break;
        } else {
            if (i == argc - 1) {
                printf("\033[1;31;40mPARAMETER ERROR\033[0m: opts_inFile NOT set!!!\n");
                exit(1);
            }
        }
    }
#ifndef NDEBUG
    std::cout << "DEBUG: host_ParserParameters "
              << "\n    numPars=" << numPars << "\n    numNodes=" << numNodes << std::endl;
#endif
    return 0;
}

ToolOptions::ToolOptions(int argcIn, char** argvIn) {
    argc = argcIn;
    argv = argvIn;
    host_ParserParameters(argc, argv, opts_C_thresh, opts_minGraphSize, threshold, opts_ftype, opts_inFile,
                          opts_coloring, opts_output, outputFile, opts_VF, xclbinFile, deviceNames, numThreads, numPars,
                          gh_par, kernelMode, numDevices, modeZmq, path_zmq, useCmd, mode_alveo, nameProj, alveoProject,
                          numPureWorker, nameWorkers, nodeId, numNodes, max_level, max_iter);
}

void PrintTimeRpt(GLV* glv, int num_dev, bool isHead) {
    int num_phase = 6;
    if (isHead) {
        printf("==========");
        printf("==========");
        for (int d = 0; d < num_dev; d++) {
            printf("==Dev_%-2d==", d);
        }
        printf("=");

        for (int phs = 0; phs < num_phase; phs++) printf("==phase%-2d=", phs);
    } else {
        for (int d = 0; d < (num_dev + num_phase); d++) printf("----------");
    }
    printf("\n");

    for (int d = 0; d < num_dev; d++) {
        // 1: Get E2E time on the device
        glv->times.totTimeE2E_DEV[d] = 0;
        for (int i = 0; i < glv->times.phase; i++)
            if (glv->times.deviceID[i] == d) glv->times.totTimeE2E_DEV[d] += glv->times.eachTimeE2E[i];
        // 2-1: Print left column
        if (d == 0) {
            if (glv->times.parNo == -1)
                printf("Final Louv ");
            else
                printf("Par:%2d    ", glv->times.parNo);
        } else
            printf("          ");
        printf("Dev_%-2d:   ", d);
        // 2-2:
        for (int i = 0; i < num_dev; i++) {
            if (i == d) {
                printf(" %3.4f ", glv->times.totTimeE2E_DEV[d]);
            } else
                printf("          ");
        }
        printf(" = ");
        // 2-3
        for (int i = 0; i < glv->times.phase; i++) {
            if (glv->times.deviceID[i] != d)
                printf("          ");
            else
                printf(" + %2.3f ", glv->times.eachTimeE2E[i]);
        }
        printf("\n");
    }
}

void PrintTimeRpt(ParLV& parlv, int num_dev) {
    printf(
        "===============\033[1;35;40mE2E time Matrix for each partition's very phases on each device "
        "\033[0m====================\n");
    double totTimeOnDev[num_dev];
    for (int d = 0; d < num_dev; d++) totTimeOnDev[d] = 0;
    for (int p = 0; p < parlv.num_par; p++) {
        PrintTimeRpt(parlv.par_src[p], num_dev, p == 0);
        for (int d = 0; d < num_dev; d++) {
            totTimeOnDev[d] += parlv.par_src[p]->times.totTimeE2E_DEV[d];
        }
    }
    printf("--------------------------------------------------------------------------------\n");
    // printf("Total Par time   :   ", num_dev);//?
    for (int d = 0; d < num_dev; d++) printf(" %3.4f   ", totTimeOnDev[d]);
    printf("\n");
    // printf("============================================================\n");
    PrintTimeRpt(parlv.plv_merged, num_dev, false);
    printf("====================================================================================================\n");
}

void PrintRptParameters(double opts_C_thresh,   // Threshold with coloring on
                        long opts_minGraphSize, // Min |V| to enable coloring
                        double opts_threshold,  // Value of threshold
                        int opts_ftype,         // File type
                        char* opts_inFile,
                        bool opts_coloring,
                        bool opts_output,
                        char* opts_outputFile,
                        bool opts_VF,
                        char* opts_xclbinPath,
                        int numThreads,
                        int num_par,
                        int par_prune,
                        bool kernelMode,
                        int devNeed_cmd,
                        int mode_zmq,
                        char* path_zmq,
                        bool useCmd,
                        int mode_alveo,
                        xf::graph::L3::Handle::singleOP& op0) {
    printf("************************************************************************************************\n");
    printf(
        "********************************  \033[1;35;40mParameters Report \033[0m   "
        "*********************|********************\n");
    printf("************************************************************************************************\n");
    // numDevices
    printf(
        "FPGA Parameter \033[1;37;40mnumDevices    \033[0m: %-8d  \t\t\t Default=        1,       by "
        "command-line: \" \033[1;37;40m-num_devices\033[0m     <v> \"",
        op0.deviceNeeded);
    printf(" or by config.json\n");
    printf(
        "FPGA Parameter \033[1;37;40mrequestLoad     \033[0m: %-8d  \t\t\t Default=      100,       by config.json     "
        "                  \n",
        op0.requestLoad);
    printf(
        "FPGA Parameter \033[1;37;40moperationName   \033[0m: %s    \t\t Default=louvainModularity, by config.json     "
        "                  \n",
        op0.operationName);
    printf(
        "FPGA Parameter \033[1;37;40mkernelName      \033[0m: %s      \t\t Default=kernel_louvain,  by config.json     "
        "                  \n",
        op0.kernelName);
    if (opts_xclbinPath[0] == 0)
        printf(
            "FPGA Parameter \033[1;37;40mxclbinFile      \033[0m: %s    \t  by config.json           or by \" "
            "\033[1;37;40m-x <path>\033[0m\"                    \n",
            op0.xclbinPath);
    else
        printf(
            "FPGA Parameter \033[1;37;40mxclbinFile      \033[0m: %s    \t  by command-line: \" \033[1;37;40m-x "
            "<path>\033[0m\" or by config.json                \n",
            op0.xclbinPath);
    printf(
        "FPGA Parameter \033[1;37;40mtype of xclbin  \033[0m: %d    \t\t\t Default=  normal ,       by command-line: "
        "\" \033[1;37;40m-fast\033[0m \"         \n",
        kernelMode);
    printf(
        "Louv Parameter \033[1;37;40mLouvain_inFile  \033[0m: %s    \t\t\t Required -f  3   ,       by command-line: "
        "\" \033[1;37;40m<name>\033[0m \"        \n",
        opts_inFile);
    printf(
        "Louv Parameter \033[1;37;40mLouvain_Output  \033[0m: %s    \t\t\t Default=  No Out ,       by command-line: "
        "\" \033[1;37;40m-o\033[0m \"            \n",
        opts_output ? "true" : "false");
    if (opts_output)
        printf(
            "Louv Parameter \033[1;37;40mhas Output File \033[0m: %s      \t\t Default=    false,       by "
            "command-line: \" \033[1;37;40m-o     <path>\033[0m \" \n",
            opts_outputFile);
    printf(
        "Louv Parameter \033[1;37;40mCPU numThreads  \033[0m: %-8d  \t\t\t Default=       16,       by command-line: "
        "\" \033[1;37;40m-thread   <v>\033[0m \" \n",
        numThreads);
    printf(
        "Louv Parameter \033[1;37;40mcoloring thrhd  \033[0m: %1.7f \t\t\t Default=   0.0002,       by command-line: "
        "\" \033[1;37;40m-d        <v>\033[0m \" \n",
        opts_C_thresh);
    printf(
        "Louv Parameter \033[1;37;40mparallel thrhd  \033[0m: %1.7f \t\t\t Default= 0.000001,       by command-line: "
        "\" \033[1;37;40m-t        <v>\033[0m \" \n",
        opts_threshold);
    printf(
        "Louv Parameter \033[1;37;40mminGraphSize    \033[0m: %-8ld  \t\t\t Default=       10,       by command-line: "
        "\" \033[1;37;40m-m        <v>\033[0m \" \n",
        opts_minGraphSize);
    printf(
        "Part Parameter \033[1;37;40mNumber of shares\033[0m: %-8d  \t\t\t Default=        2,       by command-line: "
        "\" \033[1;37;40m-num_pars  <v>\033[0m \" \n",
        num_par);
    printf(
        "Part Parameter \033[1;37;40mpruning thrhd   \033[0m: %-8d  \t\t\t Default=        1,       by command-line: "
        "\" \033[1;37;40m-par_prune<v>\033[0m \" \n",
        par_prune);

    printf("************************************************************************************************\n");
}

// functions for printing info or read command lines end
void ParLV::Init(int mode) {
    st_Partitioned = false;
    st_ParLved = false;
    st_PreMerged = false;
    st_Merged = false;
    st_FinalLved = false;
    //
    st_Merged_ll = false;
    st_Merged_gh = false;
    isMergeGhost = false;
    isOnlyGL = false;
    isPrun = true;
    th_prun = 1;
    plv_src = NULL;
    plv_merged = NULL;
    plv_final = NULL;
    num_par = NV = NVl = NE = NElg = NEll = NEgl = NEgg = NEself = NV_gh = 0;
    elist = NULL;
    M_v = NULL;
    NE_list_all = 0;
    NE_list_ll = 0;
    NE_list_gl = 0;
    NE_list_gg = 0;
    NV_list_all = 0;
    NV_list_l = 0;
    NV_list_g = 0;
    num_dev = 1;
    kernelMode = mode;
    num_server = 1;
}

void ParLV::Init(int mode, GLV* src, int nump, int numd) {
    Init(mode);
    plv_src = src;
    num_par = nump;
    num_dev = numd;
    st_Partitioned = true;
}

void ParLV::Init(int mode, GLV* src, int num_p, int num_d, bool isPrun, int th_prun) {
    Init(mode, src, num_p, num_d);
    this->isPrun = isPrun;
    this->th_prun = th_prun;
}

ParLV::ParLV() {
    Init(MD_FAST);
}

ParLV::~ParLV() {
    num_par = 0;
    if (elist) free(elist);
    if (M_v) free(M_v);
}

void ParLV::PrintSelf() {
    printf(
        "=========================================================[\033[1;35;40m LIST Begin "
        "\033[0m]======================================================================================\n");
    printf(
        "= Uniq ID ==| Numbers for  C / V     ( V_ghost ) Edge Number  (ghost edges  "
        ")=================================================================================\n");
    printf(
        "=========================================================[\033[1;35;40m Partitioned sub-graphs "
        "\033[0m]============================================================================\n");

    for (int p = 0; p < num_par; p++)
        if (st_Partitioned == false)
            break;
        else
            this->par_src[p]->printSimple();
    printf(
        "=========================================================[\033[1;35;40m Louvained sub-graphs "
        "\033[0m]==================================================================================\n");

    for (int p = 0; p < num_par; p++)
        if (st_ParLved == false)
            break;
        else
            this->par_lved[p]->printSimple();

    if (st_FinalLved) {
        this->plv_src->printSimple();
        printf(
            "=========================================================[\033[1;35;40m Merged sub-graphs together "
            "\033[0m]============================================================================\n");
        this->plv_merged->printSimple();
        printf(
            "=========================================================[\033[1;35;40m Original graph with Updated "
            "Communities \033[0m]===============================================================\n");
        this->plv_final->printSimple();
    } else if (st_Merged) {
        this->plv_merged->printSimple();
        printf(
            "=========================================================[\033[1;35;40m Original graph with Updated "
            "Communities \033[0m]===============================================================\n");
        this->plv_src->printSimple();
    } else if (st_PreMerged) {
        this->plv_merged->printSimple();
    }
    printf(
        "==========================================================[\033[1;35;40m LIST   END "
        "\033[0m]===========================================================================================\n");
}

void ParLV::UpdateTimeAll() {
    timesPar.timeAll =
        +timesPar.timePar_all + timesPar.timeLv_all + timesPar.timePre + timesPar.timeMerge + timesPar.timeFinal;
};

int ParLV::partition(GLV* glv_src, int& id_glv, int num, long th_size, int th_maxGhost) {
    assert(glv_src);
    assert(glv_src->G);
    num_par = num;
    if (num_par >= MAX_PARTITION) {
        printf("\033[1;31;40mERROR\033[0m: exe_LV_SETM wrong number of partition %d which should be small than %d!\n",
               num_par, MAX_PARTITION);
        return -1;
    }
    long vsize = glv_src->NV / num_par;
    long start = 0;
    long end = start + vsize;
    off_src[0] = 0;
    for (int i = 0; i < num_par; i++) {
        if (th_maxGhost > 0)
            par_src[i] = stt[i].ParNewGlv_Prun(glv_src->G, start, end, id_glv, th_maxGhost);
        else
            par_src[i] = stt[i].ParNewGlv(glv_src->G, start, end, id_glv);
        // par_list.push_back(pt_par[i]);
        start = end;
        end = start + vsize;
        off_src[i + 1] = start;
    }
    return 0;
}

int GetScl(long v) {
    int ret = 0;
    while (v > 0) {
        v = v >> 1;
        ret++;
    }
    return ret;
}

void ParLV::PreMerge() {
    if (st_PreMerged == true) return;
    assert(num_par > 0);
    // assert(st_ParLved==true);
    if (st_ParLved == false) return;
    off_lved[0] = off_src[0] = 0;
    NV = NVl = NE = NElg = 0;
    max_NV = max_NVl = max_NE = max_NElg = 0;
    for (int p = 0; p < num_par; p++) {
        NV += par_lved[p]->NV;
        NVl += par_lved[p]->NVl;
        NE += par_lved[p]->NE;
        NElg += par_lved[p]->NElg;
        max_NV = max_NV > par_lved[p]->NV ? max_NV : par_lved[p]->NV;
        max_NVl = max_NVl > par_lved[p]->NVl ? max_NVl : par_lved[p]->NVl;
        max_NE = max_NE > par_lved[p]->NE ? max_NE : par_lved[p]->NE;
        max_NElg = max_NElg > par_lved[p]->NElg ? max_NElg : par_lved[p]->NElg;
        off_lved[p + 1] = NVl;
        off_src[p + 1] = off_src[p] + par_src[p]->NVl;
    }
    scl_NV = GetScl(max_NV);
    scl_NE = GetScl(max_NE);
    scl_NVl = GetScl(max_NVl);
    scl_NElg = GetScl(max_NElg);
    NV_gh = CheckGhost(); // + NVl;;
    NV = NV_gh + NVl;
    elist = (edge*)malloc(sizeof(edge) * (NE));
    M_v = (long*)malloc(sizeof(long) * (NV));
    assert(M_v);
    assert(elist);
    memset(M_v, 0, sizeof(long) * (NV));
    NE_list_all = 0;
    NE_list_ll = 0;
    NE_list_gl = 0;
    NE_list_gg = 0;
    NV_list_all = 0;
    NV_list_l = 0;
    NV_list_g = 0;
    for (int p = 0; p < num_par; p++) {
        GLV* G_src = par_src[p];
        for (int v = 0; v < G_src->NVl; v++) {
            long base_src = off_src[p];
            long base_lved = off_lved[p];
            long c_src = G_src->C[v];
            long c_mg;
            if (c_src < par_lved[p]->NVl)
                c_mg = c_src + base_lved;
            else
                c_mg = p_v_new[p][c_src];
            G_src->C[v] = c_mg;
// M_v[v+base_lved] = c_mg;
#ifdef DBG_PAR_PRINT
            printf("DBGPREMG:p=%d  v=%d base_src=%d  base_lved=%d C1=%d, isLocal%d, c_mg=%d\n", p, v, base_src,
                   base_lved, G_src->C[v], c_src < par_lved[p]->NVl, c_mg);
#endif
        }
    }
    st_PreMerged = true;
}

int ParLV::AddGLV(GLV* plv) {
    assert(plv);
    par_src[num_par] = plv;
    num_par++;
    return num_par;
}

long ParLV::FindGhostInLocalC(long me) {
    long e_org = -me - 1;
    int idx = 0;
    // 1. find #p
    for (int p = 0; p < num_par; p++) {
        if (off_src[p] <= e_org && e_org < off_src[p + 1]) {
            idx = p;
            break;
        }
    }
    // 2.
    long address = e_org - off_src[idx];
    long m_src = par_src[idx]->M[address];
    // assert(m_org ==  m_src);
    long c_src = par_src[idx]->C[address];
    long c_src_m = c_src + off_lved[idx];
#ifdef PRINTINFO
    printf("e_org=%-4ld - %-4ld = address:%-4ld; c_src:%-4ld+off%-4ld=c_src_m%-4ld\n", e_org, off_src[idx], address,
           c_src, off_lved[idx], c_src_m);
#endif
    return c_src_m;
}

int ParLV::FindParIdx(long e_org) {
    int idx = 0;
    // 1. find #p
    for (int p = 0; p < num_par; p++) {
        if (off_src[p] <= e_org && e_org < off_src[p + 1]) {
            idx = p;
            break;
        }
    }
    return idx;
}

int ParLV::FindParIdxByID(int id) {
    if (!this->st_Partitioned) return -1;
    for (int p = 0; p < num_par; p++)
        if (this->par_lved[p]->ID == id) return p;
    if (!this->st_ParLved) return -1;
    for (int p = 0; p < num_par; p++)
        if (this->par_src[p]->ID == id) return p;
    return -1;
}

pair<long, long> ParLV::FindCM_1hop(int idx, long e_org) {
    // 2.
    pair<long, long> ret;
    long addr_v = e_org - off_src[idx];
    long c_src_sync = par_src[idx]->C[addr_v];
    long c_lved_new = c_src_sync; // key logic
    long m_lved_new = par_lved[idx]->M[c_lved_new];
    ret.first = c_lved_new;
    ret.second = m_lved_new;
    return ret;
}

pair<long, long> ParLV::FindCM_1hop(long e_org) {
    // 2.
    int idx = FindParIdx(e_org);
    pair<long, long> ret;
    long addr_v = e_org - off_src[idx];
    long c_src_sync = par_src[idx]->C[addr_v];
    long c_lved_new = c_src_sync; // key logic
    long m_lved_new = par_lved[idx]->M[c_lved_new];
    ret.first = c_lved_new;
    ret.second = m_lved_new;
    return ret;
}

long ParLV::FindC_nhop(long m_g) {
    assert(m_g < 0);
    long m_next = m_g;
    int cnt = 0;

    do {
        long e_org = -m_next - 1;
        int idx = FindParIdx(e_org);
        long v_src = e_org - off_src[idx]; // dbg
        pair<long, long> cm = FindCM_1hop(idx, e_org);
        long c_lved_new = cm.first;
        long m_lved_new = cm.second;
        cnt++;

        if (m_lved_new >= 0)
            return c_lved_new + off_lved[idx];
        else if (m_lved_new == m_g) {
            return m_g;
        } else { // m_lved_new<0;
            m_next = m_lved_new;
        }

    } while (cnt < 2 * num_par);
    return m_g; // no local community for the ghost which should be add as a new community
}

//#define DBG_PAR_PRINT
long FindOldOrAddNew(unordered_map<long, long>& map_v, long& NV, long v) {
    unordered_map<long, long>::iterator iter;
    int ret;
    iter = map_v.find(v);
    if (iter == map_v.end()) {
        ret = NV++; // add new
#ifdef DBG_PAR_PRINT
        printf("DBG_PAR_PRINT, new:%d ", ret);
#endif
    } else {
        ret = iter->second; // find old
#ifdef DBG_PAR_PRINT
        printf("DBG_PAR_PRINT, old:%d ", ret);
#endif
    }
    return ret;
}

long ParLV::CheckGhost() {
    long NV_gh_new = 0;
    for (int p = 0; p < num_par; p++) {
        GLV* G_src = par_src[p];
        GLV* G_lved = par_lved[p];
        long* vtxPtr = G_lved->G->edgeListPtrs;
        edge* vtxInd = G_lved->G->edgeList;
        p_v_new[p] = (long*)malloc(sizeof(long) * (G_lved->NV));
        assert(p_v_new[p]);
        for (int v = G_lved->NVl; v < G_lved->NV; v++) {
            long mv = G_lved->M[v];
            long v_new = 0;
            if (use_bfs)
                v_new = FindC_nhop_bfs(mv); // find the bfs-hop subgraph //BFS
            else
                v_new = FindC_nhop(mv); // find the directly cat subgraph
            if (v_new == mv) {
                p_v_new[p][v] = FindOldOrAddNew(m_v_gh, NV_gh_new, v_new) + this->NVl;
#ifdef DBG_PAR_PRINT
                printf("CheckGhost: p=%-2d  v=%-6d mv=%-6d  v_new=%-6d NV_gh=%d\n", p, v, mv, p_v_new[p][v], NV_gh_new);
#endif
            } else {
                p_v_new[p][v] = v_new;
#ifdef DBG_PAR_PRINT
                printf("CheckGhost: p=%-2d  v=%-6d mv=%-6d  v_new=%-6d  isNVL%d\n", p, v, mv, v_new, v_new < this->NVl);
#endif
            }
        }
    }
    return NV_gh_new;
}

double ParLV::TimeStar() {
    return timesPar.time_star = getTime();
}
double ParLV::TimeDonePar() {
    return timesPar.time_done_par = getTime();
}
double ParLV::TimeDoneLv() {
    return timesPar.time_done_lv = getTime();
}
double ParLV::TimeDonePre() {
    return timesPar.time_done_pre = getTime();
}
double ParLV::TimeDoneMerge() {
    return timesPar.time_done_mg = getTime();
}
double ParLV::TimeDoneFinal() {
    timesPar.time_done_fnl = getTime();
    return timesPar.time_done_fnl;
}

double ParLV::TimeAll_Done() {
    timesPar.timePar_all = timesPar.time_done_par - timesPar.time_star;
    timesPar.timeLv_all = timesPar.time_done_lv - timesPar.time_done_par;
    timesPar.timePre = timesPar.time_done_pre - timesPar.time_done_lv;
    timesPar.timeMerge = timesPar.time_done_mg - timesPar.time_done_pre;
    timesPar.timeFinal = timesPar.time_done_fnl - timesPar.time_done_mg;
    timesPar.timeAll = timesPar.time_done_fnl - timesPar.time_star;
    return timesPar.timeAll;
}

void ParLV::PrintTime() {
    printf("\033[1;37;40mINFO\033[0m: Total time for partition orignal       : %lf\n", timesPar.timePar_all);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition Louvain subs  : %lf\n", timesPar.timeLv_all);
    for (int d = 0; d < num_dev; d++) { // for parlv.timeLv_dev[d]
        printf("\033[1;37;40m    \033[0m: Total time for Louvain on dev-%1d        : %lf\t = ", d,
               timesPar.timeLv_dev[d]);
        for (int p = d; p < num_par; p += num_dev) printf("+ %3.4f ", timesPar.timeLv[p]);
        printf("\n");
    }
    printf("\033[1;37;40mINFO\033[0m: Total time for partition pre-Merge     : %lf\n", timesPar.timePre);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition Merge         : %lf\n", timesPar.timeMerge);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition Final Louvain : %lf\n", timesPar.timeFinal);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition All flow      : %lf\n", timesPar.timeAll);
}

void ParLV::PrintTime2() {
    // Final number of clusters       : 225
    // Final modularity               :
    printf("\033[1;37;40mINFO\033[0m: Final number of clusters               : %ld\n", plv_src->com_list.back().NC);
    printf("\033[1;37;40mINFO\033[0m: Final modularity                       : %lf\n", plv_src->com_list.back().Q);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition + Louvain     : %lf\n", timesPar.timePar_all);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition pre-Merge     : %lf\n", timesPar.timePre);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition Merge         : %lf\n", timesPar.timeMerge);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition Final Louvain : %lf\n", timesPar.timeFinal);
    printf("\033[1;37;40mINFO\033[0m: Total time for partition All flow      : %lf\n", timesPar.timeAll);
}

void ParLV::CleanTmpGlv() {
    for (int p = 0; p < num_par; p++) {
        delete (par_src[p]);
        delete (par_lved[p]);
    }
    delete (plv_merged);
}
////////////////////////////////////////////////////////////////////////////////////////////////

void sim_getServerPar(
    // inputs
    graphNew* G,       // Looks like a Global Graph but here only access dataset within
    long start_vertex, // a range from start_vertex to end_vertex, which is stored locally
    long end_vertex,   // Here we assume that the vertices of a TigerGraph partition
                       // stored on a node are continuous
    // Outputs
    long* offsets_tg, // we can also use �degree� instead of �offsets�
    edge* edges_tg,   //
    long* dgr_tail_tg // degrees for the tail of each edge;
    ) {
    // printf("DBG_TGPAR: NV=%d NE=%d \n", G->numVertices, G->numEdges);
    long* off_glb = G->edgeListPtrs; // in GSQL, maybe �degree� can be much easier.
    edge* edges_glb = G->edgeList;
    long cnt_e = 0;
    long cnt_v = 0;
    offsets_tg[0] = off_glb[0 + start_vertex] - off_glb[start_vertex]; // 0;
    for (long v_glb = start_vertex; v_glb < end_vertex; v_glb++) {     // Scanning nodes within a range
        offsets_tg[cnt_v + 1] = off_glb[v_glb + 1] - off_glb[start_vertex];
        long degree = off_glb[v_glb + 1] - off_glb[v_glb];
        for (long e = 0; e < degree; e++) {
            edges_tg[cnt_e].head = edges_glb[off_glb[v_glb] + e].head;
            edges_tg[cnt_e].tail = edges_glb[off_glb[v_glb] + e].tail;
            edges_tg[cnt_e].weight = edges_glb[off_glb[v_glb] + e].weight;
            dgr_tail_tg[cnt_e] = off_glb[edges_tg[cnt_e].tail + 1] - off_glb[edges_tg[cnt_e].tail];
            // printf("INFO:edges_tg[cnt_e].tail = %ld,dgr_tail_tg[cnt_e]=%ld\n",edges_tg[cnt_e].tail,
            // dgr_tail_tg[cnt_e]);
            cnt_e++;
        }
        cnt_v++;
    }
}

// start bfs partition (lOW_BANGWIDTH_MOTHED) method
#define lOW_BANGWIDTH_METHOD

#ifdef lOW_BANGWIDTH_METHOD

struct HopV {
    long v;
    int hop;
};
typedef int t_sel;
long FindStartVertexlastround(graphNew* G, t_sel V_selected[], long laststart) {
    long NV = G->numVertices;
    long* offsets = G->edgeListPtrs;
    edge* indices = G->edgeList;
    long v_start = -1;
    int degree_max = 1;

    // omp is not fast than the directly write coding style
    for (long v = laststart; v < NV; v++) {
        if (V_selected[v]) continue;
        v_start = v;
        break;
    }
    return v_start;
}

long FindStartVertex(graphNew* G, t_sel V_selected[]) {
    long NV = G->numVertices;
    long* offsets = G->edgeListPtrs;
    edge* indices = G->edgeList;
    long v_start = -1;
    int degree_max = 1;

    for (long v = 0; v < NV; v++) {
        if (V_selected[v]) continue;
        long adj1 = offsets[v];
        long adj2 = offsets[v + 1];
        int degree = adj2 - adj1;
        if (degree < degree_max) continue;
        v_start = v;
        degree_max = degree;
    }
    return v_start;
}

long BFSPar_AddNeighbors(
    graphNew* G,
    int p,              //
    t_sel V_selected[], // inout, for recording whether a vertex is selected
    long v,
    int hop,
    // long* drglist_tg,
    // output
    edge* elist_par[],
    long num_e_dir[],
    long num_e_dir_lg[],
    long num_v_l[],
    long num_v_g[],
    unordered_map<long, long> map_v_l[], // std:map for local vertices, will be used for renumbering and creating M
    unordered_map<long, long> map_v_g[], // std:map for ghost vertices, will be used for renumbering and creating M
    std::queue<HopV> q_par[],
    unordered_map<long, long> map_v_l_scaned[],
    int max_hop[]) {
    long* offsets = G->edgeListPtrs;
    edge* indices = G->edgeList;
#ifdef DEBUGPAR
    printf("BFS, v=%ld\n", v);
#endif
    long adj1 = offsets[v];
    long adj2 = offsets[v + 1];
    int degree = adj2 - adj1;
    bool hasAGhost = false;        // th_prun = 1 ,so this flag turn ture just in first ghost add to edge
    int e_mindgr = 0;              // the min degree of the edge
    int e_min = 0;                 // the min degree of the edge's global ID
    long num_e_min = num_e_dir[p]; // the min degree of the edge's sub-graph ID

    for (int d = 0; d < degree; d++) {
        unordered_map<long, long>::iterator itr;
        bool notSelected = false;
        bool isTailLocal = false;
        bool isTailGhost = false;
        bool BeenScaned = false;
        long e = indices[adj1 + d].tail;

        if (V_selected[e] == 0) {
            notSelected = true;
            V_selected[e] = p + 1;
        } // atom action done

        itr = map_v_l[p].find(e);
        if (itr != map_v_l[p].end()) isTailLocal = true;
        itr = map_v_l_scaned[p].find(e);
        if (itr != map_v_l_scaned[p].end()) BeenScaned = true;
        itr = map_v_g[p].find(e);
        if (itr != map_v_g[p].end()) isTailGhost = true;
        if (notSelected) {
            map_v_l[p][e] = num_v_l[p];
            num_v_l[p]++;
            HopV he;
            he.hop = hop + 1;
            he.v = e;
            q_par[p].push(he);
#ifdef DEBUGPAR
            printf("push vertex'e %ld\n", e);
#endif
            // if(he.hop>max_hop[p])
            //	max_hop[p] = he.hop;
        }
        // add to edge ghost;
        if (BeenScaned) // && v!=e)
            continue;
        else {
            double w = indices[adj1 + d].weight;
#ifdef NO_PRUNING
            // if no define the th_maxGhost=1 in the top function:BFS_par_general_4TG, but that is impossible
            elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
            elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
            elist_par[p][num_e_dir[p]].weight = w;
#ifdef DEBUGPAR
            printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
            num_e_dir[p]++;
            if (!notSelected && !isTailLocal) {
                if (!isTailGhost) { // new found ghost
                    map_v_g[p][e] = num_v_g[p];
                    num_v_g[p]++;
                    num_e_dir_lg[p]++;
                } else {
                    num_e_dir_lg[p]++;
                }
            }
#else
            // so the th_maxGhost=1 and should do the ghost edge pruning by 1
            if (!notSelected && !isTailLocal) {
                if (!isTailGhost) { // new found ghost
                    long drglist_tg = offsets[e + 1] - offsets[e];
                    if (!hasAGhost) { // add new edge
                        elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
                        elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
                        elist_par[p][num_e_dir[p]].weight = w;
                        num_e_min = num_e_dir[p];
#ifdef DEBUGPAR
                        printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
                        num_e_dir[p]++;

                        map_v_g[p][e] = num_v_g[p];
                        e_mindgr = drglist_tg;
                        e_min = e;
                        num_v_g[p]++;
                        num_e_dir_lg[p]++;
                        hasAGhost = true;
                    } else { // switch the min  // because the bfs, this branch access lightly
                        // printf("b emin=%ld, e=%ld, num_v_g[p]=%d \n",e_min, e,num_v_g[p]);
                        if (drglist_tg < e_mindgr || (drglist_tg == e_mindgr && e < e_min)) {
                            map_v_g[p].erase(e_min);
                            map_v_g[p][e] = num_v_g[p] - 1;
                            e_mindgr = drglist_tg;
                            e_min = e;

                            elist_par[p][num_e_min].head = v <= e ? v : e;
                            elist_par[p][num_e_min].tail = v <= e ? e : v;
                            elist_par[p][num_e_min].weight = w;
                        }
                    }
                } else { // add new edge
                    elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
                    elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
                    elist_par[p][num_e_dir[p]].weight = w;
#ifdef DEBUGPAR
                    printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
                    num_e_dir[p]++;
                }
            } else { // add new edge
                elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
                elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
                elist_par[p][num_e_dir[p]].weight = w;
#ifdef DEBUGPAR
                printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
                num_e_dir[p]++;
            }
#endif
        } // add to edge ghost;
    }     // for eatch e
    map_v_l_scaned[p][v] = v;
    return 0;
}

long addGhostAfterPartition(
    graphNew* G,
    int p,              //
    t_sel V_selected[], // inout, for recording whether a vertex is selected
    long v,
    int hop,
    // long* drglist_tg,
    // output
    edge* elist_par[],
    long num_e_dir[],
    long num_e_dir_lg[],
    long num_v_l[],
    long num_v_g[],
    unordered_map<long, long> map_v_l[], // std:map for local vertices, will be used for renumbering and creating M
    unordered_map<long, long> map_v_g[], // std:map for ghost vertices, will be used for renumbering and creating M
    std::queue<HopV> q_par[],
    unordered_map<long, long> map_v_l_scaned[],
    int max_hop[]) {
    long* offsets = G->edgeListPtrs;
    edge* indices = G->edgeList;
#ifdef DEBUGPAR
    printf(" addghost BFS, v=%ld\n", v);
#endif
    long adj1 = offsets[v];
    long adj2 = offsets[v + 1];
    int degree = adj2 - adj1;
    bool hasAGhost = false;        // th_prun = 1 ,so this flag turn ture just in first ghost add to edge
    int e_mindgr = 0;              // the min degree of the edge
    int e_min = 0;                 // the min degree of the edge's global ID
    long num_e_min = num_e_dir[p]; // the min degree of the edge's sub-graph ID

    for (int d = 0; d < degree; d++) {
        unordered_map<long, long>::iterator itr;
        bool notSelected = false;
        bool isTailLocal = false;
        bool isTailGhost = false;
        bool BeenScaned = false;
        long e = indices[adj1 + d].tail;

        // if(V_selected[e]==0){
        // 	notSelected = true;
        // 	//V_selected[e] = p+1;
        // } //atom action done

        itr = map_v_l[p].find(e);
        if (itr != map_v_l[p].end()) isTailLocal = true;
        itr = map_v_l_scaned[p].find(e);
        if (itr != map_v_l_scaned[p].end()) BeenScaned = true;
        itr = map_v_g[p].find(e);
        if (itr != map_v_g[p].end()) isTailGhost = true;
        // if( notSelected){
        // 	map_v_l[p][e] = num_v_l[p];
        // 	num_v_l[p]++;
        // 	HopV he;
        // 	he.hop=hop+1;
        // 	he.v = e;
        // 	q_par[p].push(he);
        // }
        // add to edge ghost;
        if (BeenScaned) // && v!=e)
            continue;
        else {
            double w = indices[adj1 + d].weight;
#ifdef NO_PRUNING
            // if no define the th_maxGhost=1 in the top function:BFS_par_general_4TG, but that is impossible
            elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
            elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
            elist_par[p][num_e_dir[p]].weight = w;
#ifdef DEBUGPAR
            printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
            num_e_dir[p]++;
            if (!notSelected && !isTailLocal) {
                if (!isTailGhost) { // new found ghost
                    map_v_g[p][e] = num_v_g[p];
                    num_v_g[p]++;
                    num_e_dir_lg[p]++;
                } else {
                    num_e_dir_lg[p]++;
                }
            }
#else
            // so the th_maxGhost=1 and should do the ghost edge pruning by 1
            if (!notSelected && !isTailLocal) {
                if (!isTailGhost) { // new found ghost
                    long drglist_tg = offsets[e + 1] - offsets[e];
                    if (!hasAGhost) { // add new edge
                        elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
                        elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
                        elist_par[p][num_e_dir[p]].weight = w;
                        num_e_min = num_e_dir[p];
#ifdef DEBUGPAR
                        printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
                        num_e_dir[p]++;

                        map_v_g[p][e] = num_v_g[p];
                        e_mindgr = drglist_tg;
                        e_min = e;
                        num_v_g[p]++;
                        num_e_dir_lg[p]++;
                        hasAGhost = true;
                    } else { // switch the min  // because the bfs, this branch access lightly
                        // printf("b emin=%ld, e=%ld, num_v_g[p]=%d \n",e_min, e,num_v_g[p]);
                        if (drglist_tg < e_mindgr || (drglist_tg == e_mindgr && e < e_min)) {
                            map_v_g[p].erase(e_min);
                            map_v_g[p][e] = num_v_g[p] - 1;
                            e_mindgr = drglist_tg;
                            e_min = e;

                            elist_par[p][num_e_min].head = v <= e ? v : e;
                            elist_par[p][num_e_min].tail = v <= e ? e : v;
                            elist_par[p][num_e_min].weight = w;
                        }
                    }
                } else { // add new edge
                    elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
                    elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
                    elist_par[p][num_e_dir[p]].weight = w;
#ifdef DEBUGPAR
                    printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
                    num_e_dir[p]++;
                }
            } else { // add new edge
                elist_par[p][num_e_dir[p]].head = v <= e ? v : e;
                elist_par[p][num_e_dir[p]].tail = v <= e ? e : v;
                elist_par[p][num_e_dir[p]].weight = w;
#ifdef DEBUGPAR
                printf("edge (%ld %ld)\n", elist_par[p][num_e_dir[p]].head, elist_par[p][num_e_dir[p]].tail);
#endif
                num_e_dir[p]++;
            }
#endif
        } // add to edge ghost;
    }     // for eatch e
    map_v_l_scaned[p][v] = v;
    return 0;
}

bool isHopDone(std::queue<HopV>& q_par_p, int num_hop_p) {
    bool hop_done;
    if (!q_par_p.empty()) {
        HopV hv = q_par_p.front();
        if (hv.hop == num_hop_p)
            hop_done = false;
        else
            hop_done = true;
    } else
        hop_done = true;
    return hop_done;
}

void BFSPar_creatingEdgeLists_fixed_prune(
    int mode_start, // 0: find vertices with max degrees, 1: vertices at average partition -m 1
    int mode_hop,   //"-h 0": just one vertex will be scanned; "-h 1": full hop; "-h 2"(default): full hop with limited
                    // MAX_PAR_VERTEX
    graphNew* G,
    int num_par,        // which is fixed for convenience, in future num_par maybe modified
    t_sel V_selected[], // inout, for recording whether a vertex is selected
    // long* drglist_tg,//input, the degree of all the vertext
    // output
    edge* elist_par[],
    long num_e_dir[],
    long num_e_dir_lg[],
    long num_v_l[],
    long num_v_g[],
    unordered_map<long, long> map_v_l[], // std:map for local vertices, will be used for renumbering and creating M
    unordered_map<long, long> map_v_g[]  // std:map for ghost vertices, will be used for renumbering and creating M
    ) {
    const int MAX_PAR = num_par;
    std::queue<HopV> q_par[MAX_PAR];
    int num_hop[MAX_PAR];
    // long v_start[MAX_PAR];
    unordered_map<long, long> map_v_l_scaned[MAX_PAR];

    long NV_all = G->numVertices;
    long* offsets = G->edgeListPtrs;
    edge* indices = G->edgeList;
    int idx_par = 0;
    long laststart = 0; // for the last round fast sourch

#ifdef DEBUGPAR
    for (int e = 0; e < G->numEdges; e++) {
        printf("e=%ld, indices[e].tail=%ld, dgr=%ld\n", e, indices[e].tail, drglist_tg[e]);
    }
#endif
    // find the max vertices of the partition graph by NV/max_par
    const long MAX_PAR_VERTEX = (NV_all + MAX_PAR - 1) / MAX_PAR;

    long NE_all = G->numEdges;
    /************************************/
    // Step-1: finding num_par start vertices from global G,
    // and initializing queues for each partition: q_par[p].push(v_start[p]);
    /************************************/
    for (int p = 0; p < num_par; p++) {
        num_v_l[p] = 0;
        num_v_g[p] = 0;
        num_e_dir[p] = 0;
        num_e_dir_lg[p] = 0;
        num_hop[p] = 0;
    }

    // find the first startvertex for partition0
    for (int p = 0; p < 1; p++) {
        num_v_l[p] = 0;

        long v_start;

        if (mode_start == 0) {
            // v_start = FindStartVertexlastround(G, V_selected, laststart);
            // laststart = v_start;
            v_start = FindStartVertex(G, V_selected);
        } else
            v_start = p * (NV_all / num_par);
        V_selected[v_start] = p + 1; // true;
        HopV hv_start;
        hv_start.hop = 0;
        hv_start.v = v_start;
        q_par[p].push(hv_start);
        map_v_l[p][v_start] = num_v_l[p];
        num_v_l[p]++;
        printf("par=%d_push_v_start=%ld, num_v_l[%d]=%ld\n", p, v_start, p, num_v_l[p]);

        num_v_g[p] = 0;
        num_e_dir[p] = 0;
        num_e_dir_lg[p] = 0;
        num_hop[p] = 0;
    }

    bool notAllQueuesEmpty = true;
    unordered_map<long, long>::iterator itr;
    /************************************/
    // Step-2: based on start vertices, doing hop-search:
    /************************************/
    int cnt_hop_round = 0;
    long num_v_all_l = 0;

    for (int p = 0; p < num_par; p++) { // loop for each partition
        bool notQueueEmpty = false;

        // 2-1 growing until MAX_PAR_VERTEX
        while (num_v_l[p] < MAX_PAR_VERTEX && notAllQueuesEmpty) { // for hop
            notAllQueuesEmpty = (num_v_all_l < NV_all);
            bool hop_done = true;
            // 2-1-1. growing many hops
            // 2-1-2. if empty, add a new start vertex then continue growing
            do {
                if (q_par[p].empty()) {
                    printf("empty_par=%d num_v_all_l=%ld  %ld\n", p, num_v_all_l, NV_all);
                    hop_done = true;
                    continue;
                }
                HopV hv = q_par[p].front();
                q_par[p].pop();

                if (hv.hop > num_hop[p]) num_hop[p] = hv.hop;

                BFSPar_AddNeighbors(G,
                                    p,          //
                                    V_selected, // inout, for recording whether a vertex is selected
                                    hv.v, hv.hop,
                                    // output
                                    elist_par, num_e_dir, num_e_dir_lg, num_v_l, num_v_g,
                                    map_v_l, // std:map for local vertices, will be used for renumbering and creating M
                                    map_v_g, // std:map for ghost vertices, will be used for renumbering and creating M
                                    q_par, map_v_l_scaned, num_hop);
                notQueueEmpty = !q_par[p].empty();
                // notAllQueuesEmpty|= notQueueEmpty;

                if (num_v_l[p] >= MAX_PAR_VERTEX) {
                    printf("!!!! break on the max vertex!!!%ld\n", MAX_PAR_VERTEX);
                    break;
                }
                if (mode_hop == 1 || mode_hop == 2)
                    hop_done = isHopDone(q_par[p], num_hop[p]);
                else
                    hop_done = true; // Always true if checking one vertext
            } while (!hop_done);
            // printf("limit_v=%d, num_v_all_l=%d, num_v_l[%d] = %d\n", MAX_PAR_VERTEX, num_v_all_l, p , num_v_l[p]);
            // printf("queueEMP=%d, notall=%d\n", !notQueueEmpty , notAllQueuesEmpty );
            if (!notQueueEmpty && notAllQueuesEmpty && (num_v_l[p] < MAX_PAR_VERTEX)) {
                // printf("2-1-2. add a new start for the empty par-queue to continue growing the partition graph\n ");
                long v_start;
                if (mode_start == 0)
                    if (p < num_par - 1) { // to make the last round
                        // v_start = FindStartVertex(G, V_selected);
                        v_start = FindStartVertexlastround(G, V_selected, laststart);
                        laststart = v_start;
                    } else {
                        v_start = FindStartVertexlastround(G, V_selected, laststart);
                        laststart = v_start;
                    }
                else
                    v_start = p * (NV_all / num_par);
                if (v_start < 0) {
                    printf(" ----all vertex selected, go to add ghost\n");
                    break;
                }
                V_selected[v_start] = p + 1; // true;
                HopV hv_start;
                hv_start.hop = 0;
                hv_start.v = v_start;
                q_par[p].push(hv_start);
                map_v_l[p][v_start] = num_v_l[p];
                num_v_l[p]++;
                notQueueEmpty = true;
                // printf(" ==== Empty case par=%d_push_v_start=%d, num_v_l[%d]=%d\n", p, v_start, p, num_v_l[p]);
            }
#ifdef DEBUGPAR
            printf("cnt_hop_round=%d, notAllQueuesEmpty = %d\n", cnt_hop_round++, notAllQueuesEmpty);
#endif
        } // while one partition

        // 2-2. add all queue vertex to the partition and ghost
        while (!q_par[p].empty()) {
            HopV hv = q_par[p].front();
            q_par[p].pop();

            addGhostAfterPartition(G,
                                   p,          //
                                   V_selected, // inout, for recording whether a vertex is selected
                                   hv.v, hv.hop,
                                   // output
                                   elist_par, num_e_dir, num_e_dir_lg, num_v_l, num_v_g,
                                   map_v_l, // std:map for local vertices, will be used for renumbering and creating M
                                   map_v_g, // std:map for ghost vertices, will be used for renumbering and creating M
                                   q_par, map_v_l_scaned, num_hop);
        }

        num_v_all_l += num_v_l[p];
        // printf("add ghost: limit_v=%d, num_v_all_l=%d, num_v_l[%d] = %d, num_hop[p]=%d\n", limit_v, num_v_all_l, p ,
        // num_v_l[p],  num_hop[p] );

        // 2-3.find the next partition's start vertex
        if (p < num_par - 1) {
            num_v_l[p + 1] = 0;
            long v_start;

            if (mode_start == 0)
                v_start = FindStartVertex(G, V_selected);
            else
                v_start = (p + 1) * (NV_all / num_par);
            // assert(v_start>0);
            if (v_start < 0) {
                // all vertex selected, but not edges
                break;
            }
            V_selected[v_start] = p + 1 + 1; // true;
            HopV hv_start;
            hv_start.hop = 0;
            hv_start.v = v_start;
            q_par[p + 1].push(hv_start);
            map_v_l[p + 1][v_start] = num_v_l[p + 1];
            num_v_l[p + 1]++;
            printf("par=%d, _push_v_start=%ld, num_v_l[%d]=%ld ------------\n", p + 1, v_start, p + 1, num_v_l[p + 1]);

            num_v_g[p + 1] = 0;
            num_e_dir[p + 1] = 0;
            num_e_dir_lg[p + 1] = 0;
            num_hop[p + 1] = 0;
        }
    } // for all partition
}

// find the v in the map and renum use the second value of map
bool MapRenumber(unordered_map<long, long>& map_v, long v, long& renum) {
    unordered_map<long, long>::iterator storedAlready = map_v.find(v);
    if (storedAlready != map_v.end()) {
        renum = storedAlready->second;
        // printf("renum %ld to %ld\n", v, renum);
        return true;
    } else {
        return false;
    }
}

// step-3: renumbering the head and tail in edge lists
void BFSPar_renumberingEdgeLists( // return: real number of partition
    int num_par,
    edge* elist_par[],
    long num_e_dir[],
    long num_e_dir_lg[],
    long num_v_l[],
    long num_v_g[],
    unordered_map<long, long> map_v_l[], // std:map for local vertices, will be used for renumbering and creating M
    unordered_map<long, long> map_v_g[], // std:map for ghost vertices, will be used for renumbering and creating M
    long* M[]) {
// map_v_l first value is v_global(key), second value is renum (value++)
#pragma omp parallel for
    for (int p = 0; p < num_par; p++) {
        for (int i = 0; i < num_e_dir[p]; i++) {
            edge e = elist_par[p][i];
            long head = e.head;
            long tail = e.tail;
            long renum_h_l = 0;
            long renum_t_l = 0;
            long renum_h_g = 0;
            long renum_t_g = 0;
            bool isIn_h_l = MapRenumber(map_v_l[p], head, renum_h_l);
            bool isIn_t_l = MapRenumber(map_v_l[p], tail, renum_t_l);
            // bool isIn_h_g = isInMap(map_v_g,  head);
            // bool isIn_t_g = isInMap(map_v_g,  tail);
            if (isIn_h_l && isIn_t_l) { // is local
                elist_par[p][i].head = renum_h_l;
                elist_par[p][i].tail = renum_t_l;
                M[p][renum_h_l] = head;
                M[p][renum_t_l] = tail;
            } else if (isIn_h_l && (!isIn_t_l)) { // tail is ghost
                MapRenumber(map_v_g[p], tail, renum_t_g);
                // printf("tail %ld, %ld \n",tail, renum_t_g+num_v_l[p]);
                elist_par[p][i].head = renum_h_l;
                elist_par[p][i].tail = renum_t_g + num_v_l[p];
                M[p][renum_h_l] = head;
                M[p][renum_t_g + num_v_l[p]] = (-1) * tail - 1;
            } else if (!isIn_h_l && isIn_t_l) { // head is ghost and swap
                MapRenumber(map_v_g[p], head, renum_h_g);
                // printf("h  %ld, %ld \n",head, renum_h_g+num_v_l[p]);
                elist_par[p][i].head = renum_h_g + num_v_l[p];
                elist_par[p][i].tail = renum_t_l;
                M[p][renum_h_g + num_v_l[p]] = (-1) * head - 1;
                M[p][renum_t_l] = tail;
            } else {
                printf("ERROR: in %s\n", __FUNCTION__);
            }
        } // end edgelist per partition

        printf("check edgelist: \n");
        // for(int i = 0; i < num_e_dir[p]; i++){
        // printf("check_par:%3d, \t edge_%d_%d\n", p, elist_par[p][i].head, elist_par[p][i].tail);
        //}

        // convert to M
        // for(int i=0; i < num_v_l[p]; i++){
        //     M[p][map_v_l[p][i]] = i;
        // }

        printf("check M: \n");
        // for(int i = 0; i < num_v_l[p]+num_v_g[p]; i++){
        // printf("check_M:%3d, \t M[%d]=%lld\n", p, i, M[p][i]);
        //}

    } // end all partition
}

void BFS_par_general_4TG(
    // Input data from TG
    long numVertices,
    long numEdges,
    long* offsets_tg,
    edge* edgelist_tg,
    long* drglist_tg, // no use in bfs
    char* path_prefix,
    // no use new
    long start_parInGlb, // Start vertex for each partition
    long stride_par,     // Total local vertex. Number of ghost vertex is not available until partition finish
    //
    int& id_glv,     // ID counter's reference for GLV obejects created. Any integer with any value is OK here;
    int th_maxGhost, // Ghost prunning parameter, always be '1'. It can be set by command-lin
    int num_par,
    GLV* parlv_par_src[MAX_PARTITION]) {
    // input
    const int mode_start = 0;
    const int mode_hop = 2;
    graphNew* G = (graphNew*)malloc(sizeof(graphNew));
    G->edgeListPtrs = offsets_tg;
    G->edgeList = edgelist_tg;
    G->numVertices = numVertices;
    G->numEdges = numEdges;
    t_sel* V_selected;        //, //inout, for recording whether a vertex is selected
                              // output
    edge* elist_par[num_par]; //,
    long* tmp_M_v[num_par];
    long num_e_dir[num_par];    //
    long num_e_dir_lg[num_par]; //,
    long num_v_l[num_par];      //,
    long num_v_g[num_par];      //,
    unordered_map<long, long>
        map_v_l[num_par]; //,//std:map for local vertices, will be used for renumbering and creating M
    unordered_map<long, long>
        map_v_g[num_par]; ////std:map for ghost vertices, will be used for renumbering and creating M
    for (int p = 0; p < num_par; p++) {
        elist_par[p] = (edge*)malloc(sizeof(edge) * (G->numEdges));
        tmp_M_v[p] = (long*)malloc(sizeof(long) * (G->numVertices));
    }

    V_selected = (t_sel*)malloc(sizeof(t_sel) * (G->numVertices));
    memset(V_selected, 0, sizeof(t_sel) * (G->numVertices));
    BFSPar_creatingEdgeLists_fixed_prune(
        mode_start, mode_hop, G, num_par,
        V_selected, // inout, for recording whether a vertex is selected
        // drglist_tg,
        // output
        elist_par, num_e_dir, num_e_dir_lg, num_v_l, num_v_g,
        map_v_l, // std:map for local vertices, will be used for renumbering and creating M
        map_v_g  // std:map for ghost vertices, will be used for renumbering and creating M
        );

    BFSPar_renumberingEdgeLists(num_par, elist_par, num_e_dir, num_e_dir_lg, num_v_l, num_v_g, map_v_l, map_v_g,
                                tmp_M_v);

    // 2-3. generate GLV
    for (int p = 0; p < num_par; p++) {
        // int id_glv = p+1;
        graphNew* Gnew = (graphNew*)malloc(sizeof(graphNew));
        GLV* glv = new GLV(id_glv);
        printf("par%d num_v = %ld,num_e_dir = %ld  \n", p, (num_v_l[p] + num_v_g[p]), num_e_dir[p]);
        GetGFromEdge(Gnew, elist_par[p], (num_v_l[p] + num_v_g[p]), num_e_dir[p]);
        glv->SetByOhterG(Gnew);
        glv->SetM(tmp_M_v[p]);
        glv->SetName_par(glv->ID, p, p, p, 0);

        parlv_par_src[p] = glv;
    }
    // check
    // for(int i=0; i<G->numVertices; i++){
    // 	printf(" V_selected[%d] = %d \t\n", i, V_selected[i]);
    // }

    // connect V_selected with map_v_l+map_v_l to bfs_adjacent

    bfs_selected* bfs_adjacent = (bfs_selected*)malloc(sizeof(bfs_selected) * (G->numVertices));
    for (int p = 0; p < num_par; p++) {
        for (unordered_map<long, long>::iterator itr = map_v_l[p].begin(); itr != map_v_l[p].end(); ++itr) {
            int v = itr->first;
            bfs_adjacent[v].par_idx = V_selected[v] - 1;
            bfs_adjacent[v].renum_in_par = itr->second;
        }
    }

    // check
    // for(int v=0; v<G->numVertices; v++)
    //     printf(" v= %d, par= %d, renum= %d\n", v, bfs_adjacent[v].par_idx, bfs_adjacent[v].renum_in_par);
    // save adjacent

    char tailName[125] = ".bfs.adj";
    char pathName[1024];
    strcpy(pathName, path_prefix);
    strcat(pathName, tailName);
    // sprintf(fullName, "_proj.bfs.adj\0", wfileName.c_str());
    string fn = pathName;
    FILE* f = fopen(fn.c_str(), "wb");
    std::cout << "WARNING: " << fn << " will be opened for binary write." << std::endl;
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary write." << std::endl;
    }
    long nv = G->numVertices;
    // fprintf(f, "*Vertices %ld\n", G->numVertices);
    // fprintf(f, "v\t par\t renum\t\n");
    // for(int v=0; v<G->numVertices; v++)
    // fprintf(f, " %d\t %d\t %d\t\n", v, bfs_adjacent[v].par_idx, bfs_adjacent[v].renum_in_par);
    fwrite(&nv, sizeof(long), 1, f);
    fwrite(bfs_adjacent, sizeof(bfs_adjacent), nv, f);
    fclose(f);

    free(V_selected);
    for (int p = 0; p < num_par; p++) {
        free(elist_par[p]);
        free(tmp_M_v[p]);
    }
}

pair<long, long> ParLV::FindCM_1hop_bfs(int idx, long e_org, long addr_v) {
    pair<long, long> ret;
    // long addr_v = e_org - off_src[idx];
    long c_src_sync = par_src[idx]->C[addr_v];
    long c_lved_new = c_src_sync; // key logic
    long m_lved_new = par_lved[idx]->M[c_lved_new];
    ret.first = c_lved_new;
    ret.second = m_lved_new;
    return ret;
}

long ParLV::FindC_nhop_bfs(long m_g) {
    assert(m_g < 0);
    long m_next = m_g;
    int cnt = 0;

    do {
        long e_org = -m_next - 1;
        int idx = bfs_adjacent[e_org].par_idx; // FindParIdx(e_org);
        long addr_v = bfs_adjacent[e_org].renum_in_par;
        // dbg
        // long v_src = e_org - off_src[idx]; // dbg
        // dbg
        pair<long, long> cm = FindCM_1hop_bfs(idx, e_org, addr_v);
        long c_lved_new = cm.first;
        long m_lved_new = cm.second;

        // debug begin
        // printf("DBG:FindC:cnt=%d, m:%-4d --> e_org:%-4d, idx:%-2d, --> v_src:%-4d, c_src&lved:%-4d, m_lved:%-4d -->
        // c_new:%d",
        //                   cnt,     m_next,   e_org,      idx,          addr_v,     c_lved_new,    m_lved_new,
        //                   c_lved_new + off_lved[idx]);
        // if(m_lved_new>=0)
        //         printf("-> c_new:%d\n",c_lved_new + off_lved[idx]);
        // else
        //         printf("\n");
        // debug end

        cnt++;

        if (m_lved_new >= 0)
            return c_lved_new + off_lved[idx];
        else if (m_lved_new == m_g) {
            return m_g;
        } else { // m_lved_new<0;
            m_next = m_lved_new;
        }

    } while (cnt < 2 * num_par);
    return m_g; // no local community for the ghost which should be add as a new community
}

#endif

// end bfs partition method to get BFS subgraph

int xai_save_partition_bfs(
    // long numVertices,
    // long numEdges,
    long* offsets_tg,
    edge* edgelist_tg,
    long* drglist_tg,
    long start_vertex,     // If a vertex is smaller than star_vertex, it is a ghost
    long end_vertex,       // If a vertex is larger than end_vertex-1, it is a ghost
    char* path_prefix,     // For saving the partition files like <path_prefix>_xxx.par
                           // Different server can have different path_prefix
    int par_prune,         // Can always be set with value '1'
    long NV_par_requested, // NV requested per partition
    long NV_par_max        //  64*1000*1000;
    ) {
    long NV_server = end_vertex - start_vertex;
    long NV_par = (NV_par_requested <= NV_par_max) ? NV_par_requested : NV_par_max;
    int num_par = (NV_server + NV_par - 1) / NV_par;
    long numEdges = offsets_tg[NV_server];
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n    start_vertex=" << start_vertex << "\n    end_vertex=" << end_vertex
              << "\n    path_prefix=" << path_prefix << "\n    NV_par_requested=" << NV_par_requested
              << "\n    NV_par_max=" << NV_par_max << "\n    NV_server=" << NV_server << "\n    numEdges=" << numEdges
              << "\n    NV_par=" << NV_par << "\n    num_par=" << num_par << std::endl;
#endif
    int status = 0;

    GLV* parlv_par_src[MAX_PARTITION];
    int id_glv = 0;
    long average_stride = 0;    // NV_par_requested;
    long start_vertext_par = 0; // start_vertex;
    int p = 0;
    // while (start_vertext_par != end_vertex) {
    // long NV_par = 0;//(end_vertex - start_vertext_par ) > average_stride ? average_stride: end_vertex -
    // start_vertext_par;

    do {
        BFS_par_general_4TG(
            // Input data from TG
            NV_server, // numVertices,
            numEdges, offsets_tg, edgelist_tg, drglist_tg, path_prefix,
            // Partition parameters
            start_vertext_par, // Start vertex for each partition
            NV_par,            // Total local vertex. Number of ghost vertex is not available until partition finish
            id_glv,    // ID counter's reference for GLV obejects created. Any integer with any value is OK here;
            par_prune, // Ghost prunning parameter, always be '1'. It can be set by command-lin
            num_par,
            // output
            parlv_par_src);

        bool deletePar = false;
        for (int p = 0; p < num_par; p++) {
            long diff_NV = NV_par_max - parlv_par_src[p]->NV;
            // std::cout << "DEBUG: after par_general_4TG "
            //          << "\n    NV_par=" << NV_par
            //          << "\n    diff_NV=" << diff_NV
            //          << "\n    parlv_par_src[p]->NV=" << parlv_par_src[p]->NV
            //          << std::endl;
            if (diff_NV < 0) {
                deletePar = true;
                break;
            }
        }

        for (int p = 0; p < num_par; p++) {
            if (deletePar) {
                delete (parlv_par_src[p]);
                parlv_par_src[p] = NULL;
                std::cout << "WARNING: partition NV_par > NV_par_MAX caused num_par++ " << std::endl;
                num_par++;
            }
        }

    } while (parlv_par_src[0] ==
             NULL); // If the partition is too big to load on FPGA, reduce the NV_par until partition is small enough

    for (int p = 0; p < num_par; p++) {
        char nm[1024];
        sprintf(nm, "_%03d.par", p);
        parlv_par_src[p]->SetName(nm);
        char pathName[1024];
        strcpy(pathName, path_prefix);
        strcat(pathName, nm);
        status = SaveGLVBin(pathName, parlv_par_src[p], false);
        if (status < 0) return status;
    }
    // start_vertext_par += NV_par;
    // p++;
    //}
    std::cout << "INFO: Total number of partitions created: " << num_par << std::endl;
    return num_par;
}

GLV* par_general_4TG(long start_vertexInGlb,
                     long* offsets_tg,
                     edge* edgelist_tg,
                     long* drglist_tg,
                     long start_parInGlb,
                     long stride_par,
                     int& id_glv,
                     int th_maxGhost) {
    SttGPar stt;
    GLV* ret = stt.ParNewGlv_Prun(start_vertexInGlb, offsets_tg, edgelist_tg, drglist_tg, start_parInGlb, stride_par,
                                  id_glv, th_maxGhost);
    return ret;
}

int xai_save_partition(long* offsets_tg,
                       edge* edgelist_tg,
                       long* drglist_tg,
                       long start_vertex,     // If a vertex is smaller than star_vertex, it is a ghost
                       long end_vertex,       // If a vertex is larger than end_vertex-1, it is a ghost
                       char* path_prefix,     // For saving the partition files like <path_prefix>_xxx.par
                                              // Different server can have different path_prefix
                       int par_prune,         // Can always be set with value '1'
                       long NV_par_requested, // NV requested per partition
                       long NV_par_max        //  64*1000*1000;
                       ) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n    start_vertex=" << start_vertex << "\n    end_vertex=" << end_vertex
              << "\n    path_prefix=" << path_prefix << "\n    NV_par_requested=" << NV_par_requested
              << "\n    NV_par_max=" << NV_par_max << std::endl;
#endif
    int status = 0;

    long NV_server = end_vertex - start_vertex;
    GLV* parlv_par_src[MAX_PARTITION];
    int id_glv = 0;
    long average_stride = NV_par_requested;
    long start_vertext_par = start_vertex;
    int p = 0;
    while (start_vertext_par != end_vertex) {
        long NV_par =
            (end_vertex - start_vertext_par) > average_stride ? average_stride : end_vertex - start_vertext_par;

        do {
            parlv_par_src[p] = par_general_4TG(
                // Input data from TG
                start_vertex, offsets_tg, edgelist_tg, drglist_tg,
                // Partition parameters
                start_vertext_par, // Start vertex for each partition
                NV_par,            // Total local vertex. Number of ghost vertex is not available until partition finish
                id_glv,   // ID counter's reference for GLV obejects created. Any integer with any value is OK here;
                par_prune // Ghost prunning parameter, always be '1'. It can be set by command-lin
                );
            long diff_NV = NV_par_max - parlv_par_src[p]->NV;
            // std::cout << "DEBUG: after par_general_4TG "
            //          << "\n    NV_par=" << NV_par
            //          << "\n    diff_NV=" << diff_NV
            //          << "\n    parlv_par_src[p]->NV=" << parlv_par_src[p]->NV
            //          << std::endl;
            if (diff_NV < 0) {
                NV_par -= diff_NV;
                delete (parlv_par_src[p]);
                parlv_par_src[p] = NULL;
            }
        } while (
            parlv_par_src[p] ==
            NULL); // If the partition is too big to load on FPGA, reduce the NV_par until partition is small enough
        char nm[1024];
        sprintf(nm, "_%03d.par", p);
        parlv_par_src[p]->SetName(nm);
        char pathName[1024];
        strcpy(pathName, path_prefix);
        strcat(pathName, nm);
        status = SaveGLVBin(pathName, parlv_par_src[p], false);
        if (status < 0) return status;
        start_vertext_par += NV_par;
        p++;
    }
    std::cout << "INFO: Total number of partitions created: " << p << std::endl;
    return p;
}

void SaveParLV(char* name, ParLV* p_parlv) {
    assert(name);
    FILE* fp = fopen(name, "wb");
    char* ptr = (char*)p_parlv;
    fwrite(ptr, sizeof(ParLVVar), 1, fp);
    fclose(fp);
}

const char* NameNoPath(const char* name) {
    assert(name);
    int len = strlen(name) - 1;
    char c = name[len];
    while ((c != '/' && c != '\\' && c != '\n' && c != 0) && len > 0) {
        c = name[--len];
    }
    if (len == 0)
        return name + len;
    else
        return name + len + 1;
}

void PathNoName(char* des, const char* name) {
    assert(name);
    int len = strlen(name) - 1;
    char c = name[len];
    while ((c != '/' && c != '\\' && c != '\n' && c != 0) && len > 0) {
        c = name[--len];
    }
    for (int i = 0; i < len; i++) des[i] = name[i];
    if (len > 0)
        des[len++] = '/';
    else if (len == 0) {
        des[len++] = '.';
        des[len++] = '/';
    }
    des[len] = '\0';
}

int Phaseloop_InitColorBuff(graphNew* G, int* colors, int numThreads, double& totTimeColoring) {
#pragma omp parallel for
    for (long i = 0; i < G->numVertices; i++) {
        colors[i] = -1;
    }
    double tmpTime;
    int numColors = algoDistanceOneVertexColoringOpt(G, colors, numThreads, &tmpTime) + 1;
    totTimeColoring += tmpTime;
    return numColors;
}

long ParLV::MergingPar2_ll() {
    // 1.create new edge list;
    long num_e_dir = 0;
    NEll = 0;
    NEself = 0;
    // long num_c_g   = 0;
    for (int p = 0; p < num_par; p++) {
        GLV* G_src = par_src[p];
        GLV* G_lved = par_lved[p];
        long* vtxPtr = G_lved->G->edgeListPtrs;
        edge* vtxInd = G_lved->G->edgeList;
        for (int v = 0; v < G_lved->NVl; v++) {
            assert(G_lved->M[v] >= 0);
            long adj1 = vtxPtr[v];
            long adj2 = vtxPtr[v + 1];
            int degree = adj2 - adj1;
            long off_local = off_lved[p];
            long v_new = v + off_local;
            long e_new;
            p_v_new[p][v] = v_new;
            // M_v[v_new] = v_new;
            for (int d = 0; d < degree; d++) {
                long e = vtxInd[adj1 + d].tail;
                long me = G_lved->M[e];
                bool isGhost = me < 0;
                if (v < e || isGhost) continue;
                e_new = e + off_local;
                double w = vtxInd[adj1 + d].weight;
                elist[num_e_dir].head = v_new;
                elist[num_e_dir].tail = e_new;
                elist[num_e_dir].weight = w;
                assert(v_new < this->NVl);
                assert(e_new < this->NVl);
                num_e_dir++;
                NEll++;
                if (v_new == e_new) NEself++;
#ifdef DBG_PAR_PRINT
                printf(
                    "LOCAL: p=%-2d v=%-8ld mv=%-8ld v_new=%-8ld, e=%-8ld me=%-8ld e_new=%-8ld w=%-3.0f NE=%-8ld "
                    "NEself=%-8ld NEll=%-8ld \n",
                    p, v, 0, v_new, e, me, e_new, w, num_e_dir, NEself, NEll);
#endif
                // AddEdge(elist, v, e, w, M_v);
            } // for d
        }     // for v
    }
    st_Merged_ll = true;
    return num_e_dir;
}

long ParLV::MergingPar2_gh() {
    long num_e_dir = 0;

    for (int p = 0; p < num_par; p++) {
        GLV* G_src = par_src[p];
        GLV* G_lved = par_lved[p];
        long* vtxPtr = G_lved->G->edgeListPtrs;
        edge* vtxInd = G_lved->G->edgeList;

        for (int v = G_lved->NVl; v < G_lved->NV; v++) {
            long mv = G_lved->M[v]; // should be uniqe in the all sub-graph
            assert(mv < 0);
            // combination of possible connection:
            // 1.1   C(head_gh)==Normal C and tail is local;
            // <local,
            // local>
            // 1.2   C(head_gh)==Normal C and tail is head_gh itself;
            // <local,
            // local>
            // 1.3.1 C(head_gh)==Normal C,and tail is other ghost in current sub graph and its C is normal
            // <local,
            // local>
            // 1.3.2 C(head_gh)==Normal C,and tail is other ghost in current sub graph and its C is other ghost <local,
            // m(tail_ghost)>,
            // 2     C(head_gh) is other ghost

            long v_new = p_v_new[p][v]; /*
         if(v_new <0 ){
                 if(isOnlyGL)
                         continue;
                 v_new = FindOldOrAddNew(m_v_gh, this->NV_gh, v_new);
                 v_new += this->NVl;// Allocated a new community for local
         }*/

            long adj1 = vtxPtr[v];
            long adj2 = vtxPtr[v + 1];
            int degree = adj2 - adj1;
            long off_local = off_lved[p];
            // trace v

            for (int d = 0; d < degree; d++) {
                double w = vtxInd[adj1 + d].weight;
                long e = vtxInd[adj1 + d].tail;
                long me = G_lved->M[e];
                bool isGhost = me < 0;
                if (v < e) continue;
                long e_new;
                if (me >= 0)
                    e_new = e + off_local;
                else if (me == mv) {
                    // assert (me == mv);
                    e_new = v_new;
                    NEself++;
                    NEgg++;
                } else {
                    e_new = p_v_new[p][e];
                }
                elist[NEll + num_e_dir].head = v_new;
                elist[NEll + num_e_dir].tail = e_new;
                elist[NEll + num_e_dir].weight = w;
                // M_v[v_new] = v_new;
                num_e_dir++;
#ifdef DBG_PAR_PRINT
                printf(
                    "GHOST: p=%-2d v=%-8ld mv=%-8ld v_new=%-8ld, e=%-8ld me=%-8ld e_new=%-8ld w=%-3.0f NE=%-8ld "
                    "NEself=%-8ld NEgg=%-8ld \n",
                    p, v, mv, v_new, e, me, e_new, w, num_e_dir, NEself, NEgg);
#endif
            }
            // 1 findDesC;
        }
    } // for sub graph ;

    st_Merged_gh = true;
    return num_e_dir;
}

GLV* ParLV::MergingPar2(int& id_glv) {
    long num_e_dir = 0;
    // CheckGhost();
    num_e_dir += MergingPar2_ll();
    num_e_dir += MergingPar2_gh();
    NE = num_e_dir;

    graphNew* Gnew = (graphNew*)malloc(sizeof(graphNew));
    // std::cout << "------------" << __FUNCTION__ << " id_glv=" << id_glv << std::endl;
    GLV* glv = new GLV(id_glv);
    glv->SetName_ParLvMrg(num_par, plv_src->ID);
#ifdef PRINTINFO
    printf("\033[1;37;40mINFO: PAR\033[0m: NV( %-8d) = NVl ( %-8d) + NV_gh( %-8d) \n", NV, NVl, NV_gh);
    printf(
        "\033[1;37;40mINFO: PAR\033[0m: NE( %-8d) = NEll( %-8d) + NElg ( %-8d) + NEgl( %-8d) + NEgg( %-8d) + NEself( "
        "%-8d) \n",
        NE, NEll, NElg, NEgl, NEgg, NEself);
#endif
    GetGFromEdge_selfloop(Gnew, elist, this->NVl + this->NV_gh, num_e_dir);
    glv->SetByOhterG(Gnew);
    // glv->SetM(M_v);
    st_Merged = true;
    plv_merged = glv;
    return glv;
}
