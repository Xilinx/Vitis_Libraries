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

#include "xilinxlouvain.hpp"
#include "ParLV.hpp"
#include "op_louvainmodularity.hpp"
//#include "ctrlLV.h"
#include <vector>
#include <memory>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

namespace {

using namespace xilinx_apps::louvainmod;

// Max number of vertices that fit on one Alveo card
long NV_par_max = 64 * 1000 * 1000;
// Only use 80% NV_max to save 20% space for ghosts
long NV_par_max_margin = NV_par_max * 8 / 10;

// Keeps all state for a partition run
//
class PartitionRun {
   public:
    const Options& globalOpts_;
    // const ComputedSettings &settings_;
    LouvainPar::PartitionOptions partOpts_;
    bool isVerbose_ = false;
    ParLV parlv_;
    std::string projName_;
    std::string projPath_;
    std::vector<int> parInServer_;          // number of partitions for each server
    std::string inputFileName_ = "no-file"; // file name for the source file for the graph, or a dummy string if no file
    const int numServers = 1;

    PartitionRun(const Options& globalOpts, // const ComputedSettings &settings,
                 const LouvainPar::PartitionOptions& partOpts)
        : globalOpts_(globalOpts), partOpts_(partOpts), isVerbose_(globalOpts.verbose) {
        int kernelMode = globalOpts.kernelMode;

        if (kernelMode < 2 || kernelMode > 4) {
            std::ostringstream oss;
            oss << "Invalid kernelMode value " << kernelMode << ".  The supported values are 2, 3 and 4.";
            throw Exception(oss.str());
        }

        parlv_.Init(kernelMode, nullptr, partOpts.numPars, globalOpts.numDevices, true, partOpts.par_prune);
        parlv_.num_par = partOpts.numPars;
        parlv_.th_prun = partOpts.par_prune;
        parlv_.num_server = numServers;
        parlv_.use_bfs = partOpts.BFS_partition;

        if (globalOpts.nameProj.empty()) {
            std::ostringstream oss1;
            oss1 << "ERROR: Alveo project name is empty";
            throw Exception(oss1.str());
        }
        //////////////////////////// Set the name for partition project////////////////////////////
        char path_proj[1024];
        char name_proj[256];
        std::strcpy(name_proj, NameNoPath(globalOpts.nameProj));
        PathNoName(path_proj, globalOpts.nameProj);
        projName_ = name_proj;
        projPath_ = path_proj;

#ifdef PRINTINFO
        printf("\033[1;37;40mINFO\033[0m:Partition Project: path = %s name = %s\n", path_proj, name_proj);
#endif

        parlv_.timesPar.timePar_all = getTime();
        parlv_.timesPar.timePar_save = 0.0; // Initialize in case we end up not writing to a file
    }

    ~PartitionRun() {}

    void setFileName(const char* inFileName) { inputFileName_ = inFileName; }

    int addPartitionData(const LouvainPar::PartitionData& partitionData) {
        // Determine the prefix string for each partition (.par) file
        // For compatibility, when num_server is 1, no 'srv<n>' surfix used
        char pathName_proj_svr[1024];
        int serverNum = (partitionData.nodeId >= 0) ? partitionData.nodeId : globalOpts_.nodeId;
        // Assign partitions to the current node.
        // Node 0 (aka driver) gets the last partitions.
        // All other nodes gets numPartitions/numNode partitions.
        // Below is an example of partition assignment for 10 partitions on a 3-node cluster
        // node 1 (aka worker 1): par_svr0_000.par par_svr0_001.par par_svr0_002.par
        // node 2 (aka worker 2): par_svr1_000.par par_svr1_001.par par_svr1_002.par
        // node 0 (aka driver)  : par_svr2_000.par par_svr2_001.par par_svr2_002.par par_svr2_003.par
        // if (globalOpts_.partitionNameMode == PartitionNameMode::Server
        //     || (globalOpts_.partitionNameMode == PartitionNameMode::Server && settings_.numServers > 1))
        // {
        //     int serverIndexAssigned = (serverNum == 0) ? (settings_.numServers-1) : (settings_.serverIndex-1);
        //     std::sprintf(pathName_proj_svr, "%s_svr%d", globalOpts_.nameProj.c_str(), serverIndexAssigned);
        // }
        // else
        std::strcpy(pathName_proj_svr, globalOpts_.nameProj); // louvain_partitions_000.par

        // User requested NV per partition
        long NV_par_requested = partitionData.NV_par_requested;
        // No supplied desired partition size: calculate it
        if (NV_par_requested == 0) {
            int totalNumDevices = numServers * globalOpts_.numDevices;

            // Assume that we want one partition per Alveo card unless overridden by num_par or isWholeGraph option
            int numPartitionsThisServer = (partitionData.isWholeGraph) ? totalNumDevices : globalOpts_.numDevices;

            // If num_par specifies more partitions than we have total number of devices in the cluster,
            // create num_par devices instead.  This feature is for testing partitioning of smaller graphs,
            // but can also be used to pre-calculate the partitions for graphs that are so large that each
            // Alveo card needs to process more than its maximum number of vertices.
            if (partOpts_.numPars > totalNumDevices) {
                // If we're in whole-graph mode, we'll create numPars partitions for this "server partition"
                if (partitionData.isWholeGraph) numPartitionsThisServer = partOpts_.numPars;

                // Not whole-graph mode: this server partition represents a fraction of the total number of alveo
                // partitions desired.  Calculate the number of alveo partitions for this server partition by
                // dividing the desired total number of alveo partitions (numPars) by the total number of server
                // partitions expected, distributing any remainder among the servers.
                else {
                    // Distribute partitions evenly among servers
                    numPartitionsThisServer = partOpts_.numPars / numServers;

                    // Distribute the L leftover partitions (where L = servers % partitions) among the first
                    // L servers
                    int extraPartitions = partOpts_.numPars % numServers;
                    if (extraPartitions > serverNum) ++numPartitionsThisServer;
                }
            }

            // Determine the number of vertices for each partition on this server, which is the lesser of
            // (a) 80% Alveo card capacity and (b) the number of vertices for this server divided by the number of
            // partitions on this server.
            NV_par_requested = NV_par_max_margin;
            const long numVerticesThisServer = partitionData.end_vertex - partitionData.start_vertex;
            const long numPartitionVertices =
                (numVerticesThisServer + numPartitionsThisServer - 1) / numPartitionsThisServer;
            if (numPartitionVertices < NV_par_requested) NV_par_requested = numPartitionVertices;

            std::cout << "INFO: automatically computed NV_par_requested=" << NV_par_requested << std::endl;
        } else if (NV_par_requested > NV_par_max_margin) {
            // making sure NV_par_requested is within NV_par_max_margin
            NV_par_requested = NV_par_max_margin;
            std::cout << "INFO: NV_par_requested is adjusted to NV_par_max_margin " << NV_par_requested << std::endl;
        } else {
            std::cout << "INFO: NV_par_requested is set to " << NV_par_requested << std::endl;
        }

        int numPartitionsCreated = 0;
        if (!partOpts_.BFS_partition) {
            numPartitionsCreated = xai_save_partition(
                const_cast<long*>(partitionData.offsets_tg), const_cast<Edge*>(partitionData.edgelist_tg),
                const_cast<long*>(partitionData.drglist_tg), partitionData.start_vertex, partitionData.end_vertex,
                pathName_proj_svr,   // num_server==1? <dir>/louvain_partitions_ : louvain_partitions_svr<num_server>
                partOpts_.par_prune, // always be '1'
                NV_par_requested,    // Allow to partition small graphs not bigger than FPGA limitation
                NV_par_max);
        } else {
            numPartitionsCreated = xai_save_partition_bfs(
                const_cast<long*>(partitionData.offsets_tg), const_cast<Edge*>(partitionData.edgelist_tg),
                const_cast<long*>(partitionData.drglist_tg), partitionData.start_vertex, partitionData.end_vertex,
                pathName_proj_svr,   // num_server==1? <dir>/louvain_partitions_ : louvain_partitions_svr<num_server>
                partOpts_.par_prune, // always be '1'
                NV_par_requested,    // Allow to partition small graphs not bigger than FPGA limitation
                NV_par_max);
        }

        if (numPartitionsCreated < 0) {
            std::ostringstream oss;
            oss << "ERROR: Failed to create Alveo partition #" << parInServer_.size() << " for server partition "
                << "start_vertex=" << partitionData.start_vertex << ", end_vertex=" << partitionData.end_vertex << ".";
            throw Exception(oss.str());
        }
        parInServer_.push_back(numPartitionsCreated);
        return numPartitionsCreated;
    }

    void finishPartitioning(int numAlveoPartitions[]) {
        parlv_.st_Partitioned = true;
        parlv_.timesPar.timePar_all = getTime() - parlv_.timesPar.timePar_all;

        //////////////////////////// save <metadata>.par file for loading //////////////////
        // Format: -create_alveo_partitions <inFile> -num_pars <par_num> -par_prune <par_prun> -name <ProjectFile>
        char* meta = (char*)malloc(4096);
        char pathName_tmp[1024];
        int numPartitions = 0;
        int numServers = 1; //(globalOpts_.partitionNameMode == PartitionNameMode::Flat) ? 1 : numServers;
        for (int i = 0; i < numServers; i++) numPartitions += numAlveoPartitions[i];

        std::sprintf(pathName_tmp, "%s%s.par.proj", projPath_.c_str(), projName_.c_str());
        if (parlv_.use_bfs)
            std::sprintf(
                meta, "-create_alveo_BFS_partitions %s -num_pars %d -par_prune %d -name %s -time_par %f -time_save %f ",
                inputFileName_.c_str(), numPartitions, partOpts_.par_prune, globalOpts_.nameProj.c_str(),
                parlv_.timesPar.timePar_all, parlv_.timesPar.timePar_save);
        else
            std::sprintf(meta,
                         "-create_alveo_partitions %s -num_pars %d -par_prune %d -name %s -time_par %f -time_save %f ",
                         inputFileName_.c_str(), numPartitions, partOpts_.par_prune, globalOpts_.nameProj.c_str(),
                         parlv_.timesPar.timePar_all, parlv_.timesPar.timePar_save);
        parlv_.num_par = numPartitions;
        // adding: -server_par <num_server> <num_par on server0> ... <num_par on server N>
        // example: -server_par 3 1 1 1
        char tmp_str[128];
        std::sprintf(tmp_str, "-server_par %d ", numServers);
        std::strcat(meta, tmp_str);
        for (int i_svr = 0, end = numServers; i_svr < end; i_svr++) {
            std::sprintf(tmp_str, "%d ", numAlveoPartitions[i_svr]);
            std::strcat(meta, tmp_str);
        }
        std::strcat(meta, "\n");

        FILE* fp = fopen(pathName_tmp, "w");
        fwrite(meta, sizeof(char), strlen(meta), fp);
        fclose(fp);
        printf("INFO: Partition project metadata saved in file %s\n", pathName_tmp);
        printf("INFO: Metadata in file is: %s\n", meta);
        std::sprintf(pathName_tmp, "%s%s.par.parlv", projPath_.c_str(), projName_.c_str());
        SaveParLV(pathName_tmp, &parlv_);
        std::sprintf(pathName_tmp, "%s%s.par.src", projPath_.c_str(), projName_.c_str());
        GLVHead dummyGlvHead;
        dummyGlvHead.NV = (parlv_.plv_src == nullptr) ? partOpts_.totalNumVertices : parlv_.plv_src->NV;
        if (dummyGlvHead.NV < 1)
            throw Exception(
                "ERROR: Number of vertices appears not to be set."
                "  Ensure that your graph source file has at least one vertex or that you have set"
                " PartitionOptions::totalNumVertices.");
        SaveHead<GLVHead>(pathName_tmp, &dummyGlvHead);

        if (isVerbose_) {
            std::cout
                << "************************************************************************************************"
                << std::endl;
            std::cout
                << "***********************  Louvain Partition Summary   *******************************************"
                << std::endl;
            std::cout
                << "************************************************************************************************"
                << std::endl;
            std::cout << "Number of servers                  : " << numServers << std::endl;
            std::cout << "Output Alveo partition project     : " << globalOpts_.nameProj << std::endl;
            std::cout << "Number of partitions created       : " << parlv_.num_par << std::endl;
            std::cout << "Time for partitioning the graph    : "
                      << (parlv_.timesPar.timePar_all + parlv_.timesPar.timePar_save) << std::endl;
            std::cout << " partitioning +  saving \n" << std::endl;
            std::cout << "    Time for partition             : " << parlv_.timesPar.timePar_all << std::endl;
            std::cout << "    Time for saving                : " << parlv_.timesPar.timePar_save << std::endl;
            std::cout
                << "************************************************************************************************"
                << std::endl;
        }
    }
};

} // anonymous namespace

//#####################################################################################################################

namespace xilinx_apps {
namespace louvainmod {

// Values determined from the global options
//
struct ComputedSettings {
    std::vector<std::string> hostIps;
    int numServers = 1;
    int modeZmq = ZMQ_NONE;
    int numPureWorker = 0;
    int serverIndex = -1; // this is the index of hostIp in hostIpAddresses. It's used
                          // to construct partition file name, sets driver/worker mode
                          // Start with -1 to indicate it's not assigned.
    std::vector<std::string> nameWorkers;

    ComputedSettings(const Options& options) {
        const std::string delimiters(" ");
        const std::string hostIpStr = options.clusterIpAddresses;
        const std::string hostIpAddress = options.hostIpAddress;
        /*modeZmq = (options.nodeId == 0) ? ZMQ_DRIVER : ZMQ_WORKER;
        int idx = 0;
        for (int i = hostIpStr.find_first_not_of(delimiters, 0); i != std::string::npos;
            i = hostIpStr.find_first_not_of(delimiters, i))
        {
            auto tokenEnd = hostIpStr.find_first_of(delimiters, i);
            if (tokenEnd == std::string::npos)
                tokenEnd = hostIpStr.size();
            const std::string token = hostIpStr.substr(i, tokenEnd - i);
            hostIps.push_back(token);
            if (hostIpAddress == token)
                serverIndex = idx;

            if ((serverIndex == 0) && (idx > 0)) // The first IP in the list is the driver. Others are workers.
                nameWorkers.push_back(std::string("tcp://" + token + ":5555"));
            idx++;
            i = tokenEnd;
        }

        numServers = hostIps.size();
        numPureWorker = nameWorkers.size();
        modeZmq = (serverIndex == 0) ? ZMQ_DRIVER : ZMQ_WORKER;
        */
        modeZmq = ZMQ_DRIVER;
#ifndef NDEBUG
        std::cout << "DEBUG: " << __FUNCTION__ << " serverIndex=" << serverIndex << std::endl;
#endif
    }
};

//#####################################################################################################################
class LouvainModImpl {
   public:
    Options options_; // copy of options passed to LouvainPar constructor
    ComputedSettings settings_;
    std::unique_ptr<PartitionRun> partitionRun_; // the active or most recent partition run
    // void loadComputeUnitsToFPGAs();
    // bool computeUnitsLoaded;
    LouvainModImpl(const Options& options) : options_(options), settings_(options) {}
};

//#####################################################################################################################
//
// LouvainPar Members
//
#ifndef LouvainPar

void LouvainPar::partitionDataFile(const char* fileName, const PartitionOptions& partOptions) {
#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << "\n    fileName=" << fileName << std::endl;
#endif
    assert(fileName);
    int id_glv = 0;
    GLV* glv_src = CreateByFile_general(const_cast<char*>(fileName), id_glv); // Louvain code not const correct
    if (glv_src == NULL) throw Exception("Unable to read data file");         // TODO: better error message
    const long NV = glv_src->NV;

    startPartitioning(partOptions);
    pImpl_->partitionRun_->setFileName(fileName);
    ParLV& parlv = pImpl_->partitionRun_->parlv_;
    parlv.plv_src = glv_src;

    //////////////////////////// partition ////////////////////////////
    // numServers is computed based on clusterIpAddresses fields from toolOptions
    const int num_server = pImpl_->partitionRun_->numServers;
    std::vector<long> start_vertex(num_server);
    std::vector<long> end_vertex(num_server);
    std::vector<long> vInServer(num_server);
    for (int i_svr = 0; i_svr < num_server; i_svr++) {
        start_vertex[i_svr] = i_svr * (NV / num_server);
        if (i_svr != num_server - 1)
            end_vertex[i_svr] = start_vertex[i_svr] + NV / num_server;
        else
            end_vertex[i_svr] = NV;
        vInServer[i_svr] = end_vertex[i_svr] - start_vertex[i_svr];
    }

    int num_partition = partOptions.numPars;
    int start_par[MAX_PARTITION];       // eg. {0, 3, 6} when par_num == 9
    int end_par[MAX_PARTITION];         // eg. {3, 6, 9} when par_num == 9
    int numParsInServer[MAX_PARTITION]; // eg. {3, 3, 3} when par_num == 9 and num_server==3
    for (int i_svr = 0; i_svr < num_server; i_svr++) {
        start_par[i_svr] = i_svr * (num_partition / num_server);
        if (i_svr != num_server - 1)
            end_par[i_svr] = start_par[i_svr] + (num_partition / num_server);
        else
            end_par[i_svr] = num_partition - 1;
        numParsInServer[i_svr] = end_par[i_svr] - start_par[i_svr];
    }

    long* offsets_glb = glv_src->G->edgeListPtrs;
    num_partition = 0;

    for (int i_svr = 0; i_svr < num_server; i_svr++) {
        long* offsets_tg = (long*)malloc(sizeof(long) * (vInServer[i_svr] + 1));
        edge* edgelist_tg =
            (edge*)malloc(sizeof(edge) * (offsets_glb[end_vertex[i_svr]] - offsets_glb[start_vertex[i_svr]]));
        long* drglist_tg =
            (long*)malloc(sizeof(long) * (offsets_glb[end_vertex[i_svr]] - offsets_glb[start_vertex[i_svr]]));

        sim_getServerPar( // This function should be repleased by GSQL
            glv_src->G, start_vertex[i_svr], end_vertex[i_svr], offsets_tg, edgelist_tg, drglist_tg);
#ifndef NDEBUG
        std::cout << "DEBUG: server id=" << i_svr << "\n       start_vertex=" << start_vertex[i_svr]
                  << "\n       end_vertex=" << end_vertex[i_svr] << "\n       NV_tg=" << vInServer[i_svr]
                  << "\n       start_par=" << start_par[i_svr] << "\n       parInServer=" << numParsInServer[i_svr]
                  << "\n       pathName=" << fileName << std::endl;
#endif

        long NV_par_requested = 0;
        if (partOptions.numPars > 1)
            NV_par_requested =
                (NV + partOptions.numPars - 1) / partOptions.numPars; // allow to partition small graph with -par_num

        LouvainPar::PartitionData partitionData;
        partitionData.offsets_tg = offsets_tg;
        partitionData.edgelist_tg = edgelist_tg;
        partitionData.drglist_tg = drglist_tg;
        partitionData.start_vertex = start_vertex[i_svr];
        partitionData.end_vertex = end_vertex[i_svr];
        partitionData.NV_par_requested = NV_par_requested;
        partitionData.nodeId = i_svr;
        numParsInServer[i_svr] = addPartitionData(partitionData);

        num_partition += numParsInServer[i_svr];
        free(offsets_tg);
        free(edgelist_tg);
        free(drglist_tg);
    }

    finishPartitioning(numParsInServer);
}

void LouvainPar::startPartitioning(const PartitionOptions& partOpts) {
    pImpl_->partitionRun_.reset(new PartitionRun(pImpl_->options_, partOpts));
}

int LouvainPar::addPartitionData(const PartitionData& partitionData) {
    return pImpl_->partitionRun_->addPartitionData(partitionData);
}

void LouvainPar::finishPartitioning(int numAlveoPartitions[]) {
    pImpl_->partitionRun_->finishPartitioning(numAlveoPartitions);
}

#endif // Louvain Partition

//#####################################################################################################################
//
// LouvainRun Members
//
#ifndef LouvainRun

void LouvainRun::setAlveoProject(const char* alveoProject) {
    pImpl_->options_.alveoProject = alveoProject;
}

void LouvainRun::loadAlveo() {}
void LouvainRun::computeLouvain(const ComputeOptions& computeOpts) {}

float LouvainRun::loadAlveoAndComputeLouvain(const ComputeOptions& computeOpts) {
    float finalQ;
    char* nameWorkers[128];

#ifndef NDEBUG
    std::cout << "DEBUG: " << __FILE__ << "::" << __FUNCTION__ << "\n    xclbinPath=" << pImpl_->options_.xclbinPath
              << "\n    alveoProject=" << pImpl_->options_.alveoProject
              << "\n    deviceNames=" << pImpl_->options_.deviceNames << "\n    nodeId=" << pImpl_->options_.nodeId
              << "\n    modeZmq=" << pImpl_->settings_.modeZmq
              << "\n    numPureWorker=" << pImpl_->settings_.numPureWorker;
#endif

    int i = 0;
    for (auto it = pImpl_->settings_.nameWorkers.begin(); it != pImpl_->settings_.nameWorkers.end(); ++it) {
        nameWorkers[i++] = (char*)it->c_str();
        std::cout << "\n    nameWorker " << i << "=" << nameWorkers[i - 1];
    }
    std::cout << std::endl;

    finalQ = ::loadAlveoAndComputeLouvain(
        (char*)(pImpl_->options_.xclbinPath.c_str()), pImpl_->options_.kernelMode, pImpl_->options_.numDevices,
        pImpl_->options_.deviceNames, (char*)(pImpl_->options_.alveoProject.c_str()), pImpl_->settings_.modeZmq,
        pImpl_->settings_.numPureWorker, nameWorkers, pImpl_->options_.nodeId, (char*)(computeOpts.outputFile.c_str()),
        computeOpts.max_iter, computeOpts.max_level, computeOpts.tolerance, computeOpts.intermediateResult,
        pImpl_->options_.verbose, computeOpts.final_Q, computeOpts.all_Q);
//,computeOpts.BFS_partition);

#ifndef NDEBUG
    std::cout << "DEBUG: " << __FUNCTION__ << " finalQ=" << finalQ << std::endl;
#endif
    return finalQ;
}

#endif

//#####################################################################################################################
//
// LouvainModImpl Members
//
// void LouvainModImpl::loadComputeUnitsToFPGAs()

} // namespace louvainmod
} // namespace xilinx_apps

//#####################################################################################################################

//
// Shared Library Entry Points
//

extern "C" {

xilinx_apps::louvainmod::LouvainModImpl* xilinx_louvainmod_createImpl(const xilinx_apps::louvainmod::Options& options) {
    return new xilinx_apps::louvainmod::LouvainModImpl(options);
}

void xilinx_louvainmod_destroyImpl(xilinx_apps::louvainmod::LouvainModImpl* pImpl) {
    delete pImpl;
}

} // extern "C"
