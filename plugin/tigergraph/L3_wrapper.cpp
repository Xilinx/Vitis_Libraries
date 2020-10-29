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

#ifndef _L3_WRAPPER_CPP_
#define _L3_WRAPPER_CPP_

#include "L3_wrapper.hpp"

namespace xf {
namespace graph {
namespace L3 {

int runMultiEvents(uint32_t number, std::vector<xf::graph::L3::event<int> >& f) {
    int ret = 0;
    for (int i = 0; i < number; ++i) {
        ret += f[i].wait();
    }
    return ret;
}

event<int> pageRankWeight(std::shared_ptr<xf::graph::L3::Handle>& handle,
                          float alpha,
                          float tolerance,
                          int maxIter,
                          xf::graph::Graph<uint32_t, float> g,
                          float* pagerank) {
    return (handle->oppg)->addwork(alpha, tolerance, maxIter, g, pagerank);
};

event<int> shortestPath(std::shared_ptr<xf::graph::L3::Handle>& handle,
                        uint32_t nSource,
                        uint32_t* sourceID,
                        bool weighted,
                        xf::graph::Graph<uint32_t, float> g,
                        float** result,
                        uint32_t** pred) {
    for (int i = 0; i < nSource; ++i) {
        (handle->opsp)
            ->eventQueue.push_back((handle->opsp)->addwork(1, &sourceID[i], weighted, g, &result[i][0], &pred[i][0]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsp)->msspThread = std::thread(std::move(t), nSource, std::ref((handle->opsp)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> triangleCount(std::shared_ptr<xf::graph::L3::Handle>& handle,
                         xf::graph::Graph<uint32_t, uint32_t> g,
                         uint64_t& nTriangle) {
    return (handle->optcount)->addwork(g, nTriangle);
};

event<int> labelPropagation(std::shared_ptr<xf::graph::L3::Handle>& handle,
                            uint32_t maxIter,
                            xf::graph::Graph<uint32_t, uint32_t> g,
                            uint32_t* labels) {
    return (handle->oplprop)->addwork(maxIter, g, labels);
};

event<int> bfs(std::shared_ptr<xf::graph::L3::Handle>& handle,
               uint32_t sourceID,
               xf::graph::Graph<uint32_t, uint32_t> g,
               uint32_t* predecent,
               uint32_t* distance) {
    return (handle->opbfs)->addwork(sourceID, g, predecent, distance);
};

event<int> wcc(std::shared_ptr<xf::graph::L3::Handle>& handle,
               xf::graph::Graph<uint32_t, uint32_t> g,
               uint32_t* result) {
    return (handle->opwcc)->addwork(g, result);
};

event<int> scc(std::shared_ptr<xf::graph::L3::Handle>& handle,
               xf::graph::Graph<uint32_t, uint32_t> g,
               uint32_t* result) {
    return (handle->opscc)->addwork(g, result);
};

event<int> convertCsrCsc(std::shared_ptr<xf::graph::L3::Handle>& handle,
                         xf::graph::Graph<uint32_t, uint32_t> g,
                         xf::graph::Graph<uint32_t, uint32_t> g2) {
    return (handle->opconvertcsrcsc)->addwork(g, g2);
};

event<int> cosineSimilaritySSSparse(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                    uint32_t sourceNUM,
                                    uint32_t* sourceIndice,
                                    uint32_t* sourceWeights,
                                    uint32_t topK,
                                    xf::graph::Graph<uint32_t, float> g,
                                    uint32_t* resultID,
                                    float* similarity) {
    (handle->opsimsparse)
        ->eventQueue.push_back(
            (handle->opsimsparse)
                ->addwork(1, 1, sourceNUM, sourceIndice, sourceWeights, topK, g, resultID, similarity));
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimsparse)->simSparseThread = std::thread(std::move(t), 1, std::ref((handle->opsimsparse)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> cosineSimilarityAPSparse(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                    uint32_t topK,
                                    xf::graph::Graph<uint32_t, float> g,
                                    uint32_t** resultID,
                                    float** similarity) {
    uint32_t numVertices = g.nodeNum;
    for (int i = 0; i < numVertices; ++i) {
        (handle->opsimsparse)
            ->eventQueue.push_back(
                (handle->opsimsparse)->addworkAP(1, 1, i, topK, g, &resultID[i][0], &similarity[i][0]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimsparse)->simSparseThread =
        std::thread(std::move(t), numVertices, std::ref((handle->opsimsparse)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> jaccardSimilaritySSSparse(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                     uint32_t sourceNUM,
                                     uint32_t* sourceIndice,
                                     uint32_t* sourceWeights,
                                     uint32_t topK,
                                     xf::graph::Graph<uint32_t, float> g,
                                     uint32_t* resultID,
                                     float* similarity) {
    (handle->opsimsparse)
        ->eventQueue.push_back(
            (handle->opsimsparse)
                ->addwork(0, 1, sourceNUM, sourceIndice, sourceWeights, topK, g, resultID, similarity));
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimsparse)->simSparseThread = std::thread(std::move(t), 1, std::ref((handle->opsimsparse)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> jaccardSimilarityAPSparse(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                     uint32_t topK,
                                     xf::graph::Graph<uint32_t, float> g,
                                     uint32_t** resultID,
                                     float** similarity) {
    uint32_t numVertices = g.nodeNum;
    for (int i = 0; i < numVertices; ++i) {
        (handle->opsimsparse)
            ->eventQueue.push_back(
                (handle->opsimsparse)->addworkAP(0, 1, i, topK, g, &resultID[i][0], &similarity[i][0]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimsparse)->simSparseThread =
        std::thread(std::move(t), numVertices, std::ref((handle->opsimsparse)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> cosineSimilaritySSDense(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                   uint32_t sourceNUM,
                                   uint32_t* sourceWeights,
                                   uint32_t topK,
                                   xf::graph::Graph<uint32_t, float> g,
                                   uint32_t* resultID,
                                   float* similarity) {
    (handle->opsimdense)
        ->eventQueue.push_back(
            (handle->opsimdense)->addwork(1, 1, sourceNUM, sourceWeights, topK, g, resultID, similarity));
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimdense)->simDenseThread = std::thread(std::move(t), 1, std::ref((handle->opsimdense)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> jaccardSimilaritySSDense(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                    uint32_t sourceNUM,
                                    uint32_t* sourceWeights,
                                    uint32_t topK,
                                    xf::graph::Graph<uint32_t, float> g,
                                    uint32_t* resultID,
                                    float* similarity) {
    (handle->opsimdense)
        ->eventQueue.push_back(
            (handle->opsimdense)->addwork(0, 1, sourceNUM, sourceWeights, topK, g, resultID, similarity));
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimdense)->simDenseThread = std::thread(std::move(t), 1, std::ref((handle->opsimdense)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> cosineSimilarityAPDense(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                   uint32_t topK,
                                   xf::graph::Graph<uint32_t, float> g,
                                   uint32_t** resultID,
                                   float** similarity) {
    uint32_t numVertices = g.nodeNum;
    for (int i = 0; i < numVertices; ++i) {
        (handle->opsimdense)
            ->eventQueue.push_back(
                (handle->opsimdense)->addworkAP(1, 1, i, topK, g, &resultID[i][0], &similarity[i][0]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimdense)->simDenseThread =
        std::thread(std::move(t), numVertices, std::ref((handle->opsimdense)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> jaccardSimilarityAPDense(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                    uint32_t topK,
                                    xf::graph::Graph<uint32_t, float> g,
                                    uint32_t** resultID,
                                    float** similarity) {
    uint32_t numVertices = g.nodeNum;
    for (int i = 0; i < numVertices; ++i) {
        (handle->opsimdense)
            ->eventQueue.push_back(
                (handle->opsimdense)->addworkAP(0, 1, i, topK, g, &resultID[i][0], &similarity[i][0]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimdense)->simDenseThread =
        std::thread(std::move(t), numVertices, std::ref((handle->opsimdense)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> knnSimilaritySSSparse(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                 uint32_t sourceNUM,
                                 uint32_t* sourceIndice,
                                 uint32_t* sourceWeights,
                                 uint32_t topK,
                                 xf::graph::Graph<uint32_t, float> g,
                                 std::string* knownLabels,
                                 std::string& label) {
    (handle->opsimsparse)
        ->eventQueue.push_back(
            (handle->opsimsparse)
                ->addworkKNN(1, 1, sourceNUM, sourceIndice, sourceWeights, topK, g, knownLabels, label));
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimsparse)->simSparseThread = std::thread(std::move(t), 1, std::ref((handle->opsimsparse)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> knnSimilaritySSDense(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                uint32_t sourceNUM,
                                uint32_t* sourceWeights,
                                uint32_t topK,
                                xf::graph::Graph<uint32_t, float> g,
                                std::string* knownLabels,
                                std::string& label) {
    (handle->opsimdense)
        ->eventQueue.push_back(
            (handle->opsimdense)->addworkKNN(1, 1, sourceNUM, sourceWeights, topK, g, knownLabels, label));
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimdense)->simDenseThread = std::thread(std::move(t), 1, std::ref((handle->opsimdense)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> knnSimilarityAPSparse(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                 uint32_t topK,
                                 xf::graph::Graph<uint32_t, float> g,
                                 std::string* knownLabels,
                                 std::string* label) {
    uint32_t numVertices = g.nodeNum;
    for (int i = 0; i < numVertices; ++i) {
        (handle->opsimsparse)
            ->eventQueue.push_back((handle->opsimsparse)->addworkAPKNN(1, 1, i, topK, g, knownLabels, label[i]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimsparse)->simSparseThread =
        std::thread(std::move(t), numVertices, std::ref((handle->opsimsparse)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

event<int> knnSimilarityAPDense(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                uint32_t topK,
                                xf::graph::Graph<uint32_t, float> g,
                                std::string* knownLabels,
                                std::string* label) {
    uint32_t numVertices = g.nodeNum;
    for (int i = 0; i < numVertices; ++i) {
        (handle->opsimdense)
            ->eventQueue.push_back((handle->opsimdense)->addworkAPKNN(1, 1, i, topK, g, knownLabels, label[i]));
    }
    std::packaged_task<int(uint32_t, std::vector<event<int> >&)> t(runMultiEvents);
    std::future<int> f0 = t.get_future();
    (handle->opsimdense)->simDenseThread =
        std::thread(std::move(t), numVertices, std::ref((handle->opsimdense)->eventQueue));
    return event<int>(std::forward<std::future<int> >(f0));
};

int cosineSimilaritySSDenseMultiCard(std::shared_ptr<xf::graph::L3::Handle>& handle,
                                     int32_t deviceNm,
                                     int32_t sourceNUM,
                                     int32_t* sourceWeights,
                                     int32_t topK,
                                     xf::graph::Graph<int32_t, int32_t>** g,
                                     int32_t* resultID,
                                     float* similarity) {
    std::vector<event<int> > eventQueue;
    float** similarity0 = new float*[deviceNm];
    int32_t** resultID0 = new int32_t*[deviceNm];
    int counter[deviceNm];
    for (int i = 0; i < deviceNm; ++i) {
        counter[i] = 0;
        similarity0[i] = aligned_alloc<float>(topK);
        resultID0[i] = aligned_alloc<int32_t>(topK);
        memset(resultID0[i], 0, topK * sizeof(int32_t));
        memset(similarity0[i], 0, topK * sizeof(float));
    }
    for (int i = 0; i < deviceNm; ++i) {
        eventQueue.push_back(
            (handle->opsimdense)
                ->addworkInt(1, 0, sourceNUM, sourceWeights, topK, g[i][0], resultID0[i], similarity0[i]));
    }
    int ret = 0;
    for (int i = 0; i < eventQueue.size(); ++i) {
        ret += eventQueue[i].wait();
    }
    for (int i = 0; i < topK; ++i) {
        for (int j = 0; j < deviceNm; ++j) {
            if (similarity[i] < similarity0[j][counter[j]]) {
                similarity[i] = similarity0[j][counter[j]];
                resultID[i] = resultID0[j][counter[j]];
                counter[j]++;
            }
        }
    }
    for (int i = 0; i < deviceNm; ++i) {
        free(similarity0[i]);
        free(resultID0[i]);
    }
    delete[] similarity0;
    delete[] resultID0;
    return ret;
};
} // L3
} // graph
} // xf

class sharedHandles {
   public:
    std::unordered_map<unsigned int, std::shared_ptr<xf::graph::L3::Handle> > handlesMap;
    static sharedHandles& instance() {
        static sharedHandles theInstance;
        return theInstance;
    }
};
class sharedHandlesSSSP {
   public:
    std::unordered_map<unsigned int, std::shared_ptr<xf::graph::L3::Handle> > handlesMap;
    static sharedHandlesSSSP& instance() {
        static sharedHandlesSSSP theInstance;
        return theInstance;
    }
};
class sharedHandlesCosSimDense {
   public:
    std::unordered_map<unsigned int, std::shared_ptr<xf::graph::L3::Handle> > handlesMap;
    static sharedHandlesCosSimDense& instance() {
        static sharedHandlesCosSimDense theInstance;
        return theInstance;
    }
};

extern "C" void bfs_fpga(uint32_t numVertices,
                         uint32_t numEdges,
                         uint32_t sourceID,
                         xf::graph::Graph<uint32_t, uint32_t> g,
                         uint32_t* predecent,
                         uint32_t* distance) {
    //----------------- Text Parser --------------------------
    std::string opName;
    std::string kernelName;
    int requestLoad;
    std::string xclbinPath;
    int deviceNeeded;

    std::fstream userInput("/home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/config.json", std::ios::in);
    if (!userInput) {
        std::cout << "Error : config file doesn't exist !" << std::endl;
        exit(1);
    }
    char line[1024] = {0};
    char* token;
    while (userInput.getline(line, sizeof(line))) {
        token = strtok(line, "\"\t ,}:{\n");
        while (token != NULL) {
            if (!std::strcmp(token, "operationName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                opName = token;
            } else if (!std::strcmp(token, "kernelName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                kernelName = token;
            } else if (!std::strcmp(token, "requestLoad")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                requestLoad = std::atoi(token);
            } else if (!std::strcmp(token, "xclbinPath")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                xclbinPath = token;
            } else if (!std::strcmp(token, "deviceNeeded")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                deviceNeeded = std::atoi(token);
            }
            token = strtok(NULL, "\"\t ,}:{\n");
        }
    }
    userInput.close();

    //----------------- Setup shortestPathFloat thread ---------
    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinFile = (char*)xclbinPath.c_str();
    op0.deviceNeeded = deviceNeeded;

    std::shared_ptr<xf::graph::L3::Handle> handleInstance(new xf::graph::L3::Handle);
    sharedHandles::instance().handlesMap[0] = handleInstance;
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];

    handle0->addOp(op0);
    handle0->setUp();

    //---------------- Run L3 API -----------------------------------
    auto ev = xf::graph::L3::bfs(handle0, sourceID, g, predecent, distance);
    int ret = ev.wait();

    //--------------- Free and delete -----------------------------------
    (handle0->opbfs)->join();
    handle0->free();
    g.freeBuffers();
}

extern "C" void load_xgraph_fpga(uint32_t numVertices, uint32_t numEdges, xf::graph::Graph<uint32_t, float> g) {
    //----------------- Text Parser --------------------------
    std::string opName;
    std::string kernelName;
    int requestLoad;
    std::string xclbinPath;
    int deviceNeeded;

    std::fstream userInput("/home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/config_shortest_path.json",
                           std::ios::in);
    if (!userInput) {
        std::cout << "Error : config file doesn't exist !" << std::endl;
        exit(1);
    }
    char line[1024] = {0};
    char* token;
    while (userInput.getline(line, sizeof(line))) {
        token = strtok(line, "\"\t ,}:{\n");
        while (token != NULL) {
            if (!std::strcmp(token, "operationName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                opName = token;
            } else if (!std::strcmp(token, "kernelName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                kernelName = token;
            } else if (!std::strcmp(token, "requestLoad")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                requestLoad = std::atoi(token);
            } else if (!std::strcmp(token, "xclbinPath")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                xclbinPath = token;
            } else if (!std::strcmp(token, "deviceNeeded")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                deviceNeeded = std::atoi(token);
            }
            token = strtok(NULL, "\"\t ,}:{\n");
        }
    }
    userInput.close();

    //----------------- Setup shortestPathFloat thread ---------
    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinFile = (char*)xclbinPath.c_str();
    op0.deviceNeeded = deviceNeeded;

    std::shared_ptr<xf::graph::L3::Handle> handleInstance(new xf::graph::L3::Handle);
    sharedHandles::instance().handlesMap[0] = handleInstance;
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];

    handle0->addOp(op0);
    handle0->setUp();

    //---------------- Run Load Graph -----------------------------------
    (handle0->opsp)->loadGraph(g);
    std::cout << "INFO: Load Graph Done " << std::endl;

    //--------------- Free and delete -----------------------------------
    g.freeBuffers();
}

extern "C" void shortest_ss_pos_wt_fpga(uint32_t numVertices,
                                        uint32_t sourceID,
                                        bool weighted,
                                        xf::graph::Graph<uint32_t, float> g,
                                        float** result,
                                        uint32_t** pred) {
    //---------------- Run Load Graph -----------------------------------
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];
    auto ev = xf::graph::L3::shortestPath(handle0, 1, &sourceID, weighted, g, result, pred);
    int ret = ev.wait();

    (handle0->opsp)->join();
    handle0->free();
    //--------------- Free and delete -----------------------------------
    // g.freeBuffers();
}

extern "C" void load_xgraph_pageRank_wt_fpga(uint32_t numVertices,
                                             uint32_t numEdges,
                                             xf::graph::Graph<uint32_t, float> g) {
    //----------------- Text Parser --------------------------
    std::string opName;
    std::string kernelName;
    int requestLoad;
    std::string xclbinPath;
    int deviceNeeded;

    std::fstream userInput("/home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/config_pagerank.json", std::ios::in);
    if (!userInput) {
        std::cout << "Error : config file doesn't exist !" << std::endl;
        exit(1);
    }
    char line[1024] = {0};
    char* token;
    while (userInput.getline(line, sizeof(line))) {
        token = strtok(line, "\"\t ,}:{\n");
        while (token != NULL) {
            if (!std::strcmp(token, "operationName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                opName = token;
            } else if (!std::strcmp(token, "kernelName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                kernelName = token;
            } else if (!std::strcmp(token, "requestLoad")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                requestLoad = std::atoi(token);
            } else if (!std::strcmp(token, "xclbinPath")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                xclbinPath = token;
            } else if (!std::strcmp(token, "deviceNeeded")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                deviceNeeded = std::atoi(token);
            }
            token = strtok(NULL, "\"\t ,}:{\n");
        }
    }
    userInput.close();

    //----------------- Setup shortestPathFloat thread ---------
    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinFile = (char*)xclbinPath.c_str();
    op0.deviceNeeded = deviceNeeded;

    std::shared_ptr<xf::graph::L3::Handle> handleInstance(new xf::graph::L3::Handle);
    sharedHandles::instance().handlesMap[0] = handleInstance;
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];

    handle0->addOp(op0);
    handle0->setUp();

    //---------------- Run Load Graph -----------------------------------
    (handle0->oppg)->loadGraph(g);
    std::cout << "INFO: Load Graph Done " << std::endl;

    //--------------- Free and delete -----------------------------------
    g.freeBuffers();
}

extern "C" void pageRank_wt_fpga(
    float alpha, float tolerance, uint32_t maxIter, xf::graph::Graph<uint32_t, float> g, float* rank) {
    // std::cout<<"alpja = "<<alpha<<"\t tol = "<<tolerance<<"\t iter = "<<maxIter<<std::endl;
    //---------------- Run Load Graph -----------------------------------
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];
    auto ev = xf::graph::L3::pageRankWeight(handle0, alpha, tolerance, maxIter, g, rank);
    int ret = ev.wait();

    (handle0->oppg)->join();
    handle0->free();
    //--------------- Free and delete -----------------------------------
    // g.freeBuffers();
}

extern "C" void load_xgraph_cosine_nbor_ss_fpga(uint32_t numVertices,
                                                uint32_t numEdges,
                                                xf::graph::Graph<uint32_t, float> g) {
    //----------------- Text Parser --------------------------
    std::string opName;
    std::string kernelName;
    int requestLoad;
    std::string xclbinPath;
    int deviceNeeded;

    std::fstream userInput("/home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/config_cosine_nbor_ss.json",
                           std::ios::in);
    if (!userInput) {
        std::cout << "Error : config file doesn't exist !" << std::endl;
        exit(1);
    }
    char line[1024] = {0};
    char* token;
    while (userInput.getline(line, sizeof(line))) {
        token = strtok(line, "\"\t ,}:{\n");
        while (token != NULL) {
            if (!std::strcmp(token, "operationName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                opName = token;
            } else if (!std::strcmp(token, "kernelName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                kernelName = token;
            } else if (!std::strcmp(token, "requestLoad")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                requestLoad = std::atoi(token);
            } else if (!std::strcmp(token, "xclbinPath")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                xclbinPath = token;
            } else if (!std::strcmp(token, "deviceNeeded")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                deviceNeeded = std::atoi(token);
            }
            token = strtok(NULL, "\"\t ,}:{\n");
        }
    }
    userInput.close();

    //----------------- Setup shortestPathFloat thread ---------
    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinFile = (char*)xclbinPath.c_str();
    op0.deviceNeeded = deviceNeeded;

    std::shared_ptr<xf::graph::L3::Handle> handleInstance(new xf::graph::L3::Handle);
    sharedHandles::instance().handlesMap[0] = handleInstance;
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];

    handle0->addOp(op0);
    handle0->setUp();

    //---------------- Run Load Graph -----------------------------------
    (handle0->opsimsparse)->loadGraph(g);
    std::cout << "INFO: Load Graph Done " << std::endl;

    //--------------- Free and delete -----------------------------------
    g.freeBuffers();
}

extern "C" void cosine_nbor_ss_fpga(uint32_t topK,
                                    uint32_t sourceLen,
                                    uint32_t* sourceIndice,
                                    uint32_t* sourceWeight,
                                    xf::graph::Graph<uint32_t, float> g,
                                    uint32_t* resultID,
                                    float* similarity) {
    //---------------- Run Load Graph -----------------------------------
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandles::instance().handlesMap[0];
    auto ev = xf::graph::L3::cosineSimilaritySSSparse(handle0, sourceLen, sourceIndice, sourceWeight, topK, g, resultID,
                                                      similarity);
    int ret = ev.wait();

    (handle0->opsimsparse)->join();
    handle0->free();
    //--------------- Free and delete -----------------------------------
    // g.freeBuffers();
}

extern "C" void loadgraph_cosinesim_ss_dense_fpga(uint32_t deviceNeeded, xf::graph::Graph<int32_t, int32_t>** g) {
    //----------------- Text Parser --------------------------
    std::string opName;
    std::string kernelName;
    int requestLoad;
    std::string xclbinPath;

    std::fstream userInput("/home/tigergraph/tigergraph/dev/gdk/gsql/src/QueryUdf/config_cosinesim_ss_dense_fpga.json",
                           std::ios::in);
    if (!userInput) {
        std::cout << "Error : config file doesn't exist !" << std::endl;
        exit(1);
    }
    char line[1024] = {0};
    char* token;
    while (userInput.getline(line, sizeof(line))) {
        token = strtok(line, "\"\t ,}:{\n");
        while (token != NULL) {
            if (!std::strcmp(token, "operationName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                opName = token;
            } else if (!std::strcmp(token, "kernelName")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                kernelName = token;
            } else if (!std::strcmp(token, "requestLoad")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                requestLoad = std::atoi(token);
            } else if (!std::strcmp(token, "xclbinPath")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                xclbinPath = token;
            } else if (!std::strcmp(token, "deviceNeeded")) {
                token = strtok(NULL, "\"\t ,}:{\n");
                //             deviceNeeded = std::atoi(token);
            }
            token = strtok(NULL, "\"\t ,}:{\n");
        }
    }
    userInput.close();

    //----------------- Setup shortestPathFloat thread ---------
    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinFile = (char*)xclbinPath.c_str();
    op0.deviceNeeded = deviceNeeded;

    std::shared_ptr<xf::graph::L3::Handle> handleInstance(new xf::graph::L3::Handle);
    sharedHandlesCosSimDense::instance().handlesMap[0] = handleInstance;
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesCosSimDense::instance().handlesMap[0];

    handle0->addOp(op0);
    handle0->setUp();

    //---------------- Run Load Graph -----------------------------------
    for (int i = 0; i < deviceNeeded; ++i) {
        (handle0->opsimdense)->loadGraphMultiCardNonBlocking(i, g[i][0]);
    }

    //--------------- Free and delete -----------------------------------

    for (int i = 0; i < deviceNeeded; ++i) {
        g[i]->freeBuffers();
        delete[] g[i]->numEdgesPU;
        delete[] g[i]->numVerticesPU;
    }
    delete[] g;
}

extern "C" void cosinesim_ss_dense_fpga(uint32_t deviceNeeded,
                                        int32_t sourceLen,
                                        int32_t* sourceWeight,
                                        int32_t topK,
                                        xf::graph::Graph<int32_t, int32_t>** g,
                                        int32_t* resultID,
                                        float* similarity) {
    //---------------- Run Load Graph -----------------------------------
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesCosSimDense::instance().handlesMap[0];
    int ret = xf::graph::L3::cosineSimilaritySSDenseMultiCard(handle0, deviceNeeded, sourceLen, sourceWeight, topK, g,
                                                              resultID, similarity);

    // (handle0->opsimdense)->join();
    // handle0->free();
}

extern "C" void close_fpga() {
    //---------------- Run Load Graph -----------------------------------
    std::shared_ptr<xf::graph::L3::Handle> handle0 = sharedHandlesCosSimDense::instance().handlesMap[0];
    handle0->free();
}
#endif
