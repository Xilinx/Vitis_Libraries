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

#include <sstream>
#include <iostream>

#include "xilinxlouvain.hpp"
#include "xf_graph_L3.hpp"
#include "common.hpp"

using namespace xilinx_apps::louvainmod;

int main(int argc, char** argv) {
    int status = 0;
    float finalQ;
    ToolOptions toolOptions(argc, argv);
    Options options;
    // LouvainMod::ComputeOptions computeOpts;
    LouvainPar::PartitionOptions partOpts;

    // set internal options fields based to commandline options
    options.modeAlveo = toolOptions.mode_alveo;
    options.xclbinPath = toolOptions.xclbinFile;
    options.kernelMode = toolOptions.kernelMode;
    options.nameProj = toolOptions.nameProj;
    options.alveoProject = toolOptions.alveoProject;
    options.numDevices = toolOptions.numDevices;
    options.deviceNames = toolOptions.deviceNames;
    options.nodeId = 0;

    std::cout << "toolOptions.modeZmq=" << toolOptions.modeZmq << " toolOptions.nodeId=" << toolOptions.nodeId
              << " toolOptions.numNodes=" << toolOptions.numNodes << std::endl;

    // create louvainMod object with internal "options". These options are common
    // between partition and load/compute operations
    LouvainPar louvainMod(options);

    switch (toolOptions.mode_alveo) {
        case ALVEOAPI_PARTITION: // 1
            partOpts.BFS_partition = false;
            partOpts.numPars = toolOptions.numPars;
            partOpts.par_prune = toolOptions.gh_par;
            louvainMod.partitionDataFile(toolOptions.opts_inFile, partOpts);
            break;
        case ALVEOAPI_PARTITION_BFS: // 2
            partOpts.BFS_partition = true;
            partOpts.numPars = toolOptions.numPars;
            partOpts.par_prune = toolOptions.gh_par;
            louvainMod.partitionDataFile(toolOptions.opts_inFile, partOpts);
            break;
        // case ALVEOAPI_LOAD: // 3
        //     std::cout << "ALVEOAPI_LOAD" << std::endl;

        //     computeOpts.outputFile = toolOptions.outputFile;
        //     computeOpts.max_iter = toolOptions.max_iter;
        //     computeOpts.max_level = toolOptions.max_level;
        //     computeOpts.tolerance = toolOptions.threshold;
        //     computeOpts.intermediateResult = false;
        //     computeOpts.final_Q = true;
        //     computeOpts.all_Q = false;

        //     finalQ = louvainMod.loadAlveoAndComputeLouvain(computeOpts);
        //     if (finalQ < -1) {
        //         std::cout << "ERROR: loadAlveoAndComputeLouvain completed with error. ErrorCode=" << finalQ <<
        //         std::endl;
        //         status = -1;
        //     } else if (toolOptions.modeZmq == ZMQ_DRIVER)  // only the driver reports the final modularity value
        //         std::cout << "INFO: loadAlveoAndComputeLouvain completed. finalQ=" << finalQ << std::endl;
        //     break;
        case ALVEOAPI_RUN: // 3
            std::cout << "ALVEOAPI_RUN" << std::endl;
            break;
        default:
            std::cout << "ERROR: Unknown tool mode " << toolOptions.mode_alveo << std::endl;
            break;
    }
    return status;
}
