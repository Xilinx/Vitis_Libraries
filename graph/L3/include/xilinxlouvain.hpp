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

#ifndef _XILINXLOUVAIN_HPP_
#define _XILINXLOUVAIN_HPP_

#include <exception>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <utility>

#include "xilinx_apps_common.hpp"

namespace xilinx_apps {
namespace louvainmod {

class LouvainModImpl;
class Options;
}
}

extern "C" {

// XILINX_LOUVAINMOD_IMPL_DECL
// int create_and_load_alveo_partitions(int argc, char *argv[]);

// XILINX_LOUVAINMOD_IMPL_DECL
// float loadAlveoAndComputeLouvain(
//     char* xclbinPath, int kernelMode, unsigned numDevices, std::string deviceNames,
//     char* alveoProject, unsigned mode_zmq, unsigned numPureWorker,
//     char* nameWorkers[128], unsigned int nodeID,  char* opts_outputFile,
//     unsigned int max_iter, unsigned int max_level, float tolerance,
//     bool intermediateResult, bool verbose, bool final_Q, bool all_Q);

// XILINX_LOUVAINMOD_IMPL_DECL
xilinx_apps::louvainmod::LouvainModImpl* xilinx_louvainmod_createImpl(const xilinx_apps::louvainmod::Options& options);

// XILINX_LOUVAINMOD_IMPL_DECL
void xilinx_louvainmod_destroyImpl(xilinx_apps::louvainmod::LouvainModImpl* pImpl);
}

// int loadComputeUnitsToFPGAs(char* xclbinPath, int kernelMode,
//                             unsigned numDevices, std::string deviceNames);
// float loadAlveoAndComputeLouvainWrapper(int argc, char *argv[]);
// float louvain_modularity_alveo(int argc, char *argv[]);
// int compute_modularity(char* inFile, char* clusterInfoFile, int offset);

namespace xilinx_apps {
namespace louvainmod {

/**
 * @brief %Exception class for Louvain modularity run-time errors
 *
 * This exception class is derived from `std::exception` and provides the standard @ref what() member function.
 * An object of this class is constructed with an error message string, which is stored internally and
 * retrieved with the @ref what() member function.
 */
class Exception : public std::exception {
    std::string message;

   public:
    /**
     * Constructs an Exception object.
     *
     * @param msg an error message string, which is copied and stored internal to the object
     */
    Exception(const std::string& msg) : message(msg) {}

    /**
     * Returns the error message string passed to the constructor.
     *
     * @return the error message string
     */
    virtual const char* what() const noexcept override { return message.c_str(); }
};

struct Edge {
    long head;
    long tail;
    double weight;
    Edge(long head_ = 0, long tail_ = 0, double weight_ = 1.0) {
        head = head_;
        tail = tail_;
        weight = weight_;
    }
};

enum class PartitionNameMode {
    Auto = 0, // Name style depends on number of servers listed in clusterIpAddresses
    Flat,     // partitions are numbered consecutively with no regard for server cluster
    Server    // partitions are numbered with both a server number and partition number within the server
};

struct Options {
    bool verbose = true;
    XString xclbinPath;
    XString nameProj;     // -name option: location of "partition project" created by partitioning, read by load/compute
    XString alveoProject; // Alveo project file .par.proj TODO: to be combined with nameProj
    int modeAlveo;
    int kernelMode = LOUVAINMOD_PRUNING_KERNEL; // Kernel mode
    unsigned numDevices = 1;                    // number of devices
    XString deviceNames;                        // space-separated list of target device names
    unsigned nodeId = 0;                        // node ID 0 will be the driver, all others will be workers
    unsigned serverIndex = 0;                   // node ID 0 will be the driver, all others will be workers
    XString hostName;                           // optional host name of this server for debugging purposes
    XString clusterIpAddresses; // space-separated list of server IP addresses in the cluster, or empty for 1 server
    XString hostIpAddress;      // IP address of this server, or empty for 1 server
    PartitionNameMode partitionNameMode = PartitionNameMode::Auto; // format of partition names
};

/// partition class for louvain partition
class LouvainPar {
   public:
    /// Index for a vertex, which should be between 0 and the total number of vertices in the whole graph.
    using VertexIndex = std::uint64_t;

    struct PartitionOptions {
        bool BFS_partition = true; // use bfs partition mothed to get low-bandwidth subgraph
        int numPars = 1;           // total desired number of partitions across all servers
        int par_prune = 1; // ghost pruning technique (1 means single ghost node of smallest degree per local node)
                           // >1 means keep that many ghost nodes per local node (TODO: verify description for >1)
                           //        int numServers = 1;  // number of servers, probably should be made obsolete
        long totalNumVertices = 0; // TODO: figure this out internally during compute, eliminate need for .par.src file

        PartitionOptions() = default;
        PartitionOptions(const PartitionOptions& opt) = default;
        PartitionOptions& operator=(const PartitionOptions& opt) = default;
    };

    struct PartitionData {
        const long* offsets_tg = nullptr;  // values are indexes into edgelist_tg (not into some global edge list)
        const Edge* edgelist_tg = nullptr; // head and tail are global vertex ids
        const long* drglist_tg = nullptr;
        long start_vertex = 0;
        long end_vertex = 0; // one vertex ID BEYOND the last vertex of the server partition
        long NV_par_requested =
            0;           // Requested NV per partition.  Leave as 0 to calculate from num Alveo cards on each server
        int nodeId = -1; // Node ID for the server, or -1 to use Options::nodeId
        bool isWholeGraph = false; // set to true if this server partition is the whole graph
    };

    LouvainPar(const Options& options) : pImpl_(xilinx_louvainmod_createImpl(options)) {}

    ~LouvainPar() { xilinx_louvainmod_destroyImpl(pImpl_); }

    /**
     * Reads an input data file and partitions it in preparation for computing Louvain modularity.
     *
     * @param fileName the name of the file containing the graph to partition
     */
    void partitionDataFile(const char* fileName, const PartitionOptions& options);

    void startPartitioning(const PartitionOptions& options);
    int addPartitionData(const PartitionData&); // Returns actual number of partitions created
    void finishPartitioning(int numAlveoPartitions[]);

   private:
    LouvainModImpl* pImpl_ = nullptr;
};

/// louvain class for louvain modularity algorithm
class LouvainRun {
   public:
    struct ComputeOptions {
        XString outputFile;
        unsigned max_iter;
        unsigned max_level;
        float tolerance;
        bool intermediateResult;
        bool final_Q;
        bool all_Q;
    };

    LouvainRun(const Options& options) : pImpl_(xilinx_louvainmod_createImpl(options)) {}

    ~LouvainRun() { xilinx_louvainmod_destroyImpl(pImpl_); }

    void setAlveoProject(const char* alveoProject);
    void loadAlveo(); // Loads .par files into CPU memory.  Can we load first .par per card into HBM here?
    void computeLouvain(const ComputeOptions& computeOpts);
    float loadAlveoAndComputeLouvain(const ComputeOptions& computeOpts);

   private:
    LouvainModImpl* pImpl_ = nullptr;
};

} // namespace louvainmod
} // namespace xilinx_apps

#endif
