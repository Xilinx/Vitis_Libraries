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

#include "utils.hpp"
#include "xf_graph_L3.hpp"

#define DT float

int main(int argc, const char* argv[]) {
    //--------------- cmd parser -------------------------------

    // cmd parser
    ArgParser parser(argc, argv);

    std::string filenameWeight;
    std::string goldenFile;
    std::string tmpStr;

    const int splitNm = 4; // kernel has 4 PUs, the input data should be splitted into 4 parts

    int numVertices = 1580; // total number of vertex read from file
    int numEdges = 200;     // total number of edge read from file
    int general = ((numVertices + splitNm * 4 - 1) / (splitNm * 4)) * 4;
    int rest = numVertices - general * (splitNm - 1);
    int32_t* numVerticesPU = new int32_t[splitNm]; // vertex numbers in each PU
    int32_t* numEdgesPU = new int32_t[splitNm];    // edge numbers in each PU
    int32_t topK;
    std::cout << "INFO: use dense graph" << std::endl;

    if (!parser.getCmdOption("-weight", tmpStr)) { // weight
        filenameWeight = "./data/cosine_dense_weight.csr";
        std::cout << "INFO: indices file path is not set, use default " << filenameWeight << "\n";
    } else {
        filenameWeight = tmpStr;
        std::cout << "INFO: indices file path is " << filenameWeight << std::endl;
    }

    if (!parser.getCmdOption("-golden", tmpStr)) { // golden
        goldenFile = "./data/cosine_sparse.mtx";
        std::cout << "INFO: golden file path is not set!\n";
    } else {
        goldenFile = tmpStr;
        std::cout << "INFO: golden file path is " << goldenFile << std::endl;
    }

    if (!parser.getCmdOption("-topK", tmpStr)) { // topK
        topK = 100;
        std::cout << "INFO: topK is not set, use 32 by default" << std::endl;
    } else {
        topK = std::stoi(tmpStr);
    }

    //----------------- Text Parser ----------------------------------
    std::string opName;
    std::string kernelName;
    int requestLoad = 100;
    std::string xclbinPath;
    int deviceNeeded = 1;

    std::fstream userInput("./config.json", std::ios::in);
    if (!userInput) {
        std::cout << "Error : file doesn't exist !" << std::endl;
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

    //----------------- Setup similarity thread ---------
    xf::graph::L3::Handle::singleOP op0;
    op0.operationName = (char*)opName.c_str();
    op0.setKernelName((char*)kernelName.c_str());
    op0.requestLoad = requestLoad;
    op0.xclbinFile = (char*)xclbinPath.c_str();
    op0.deviceNeeded = deviceNeeded;

    xf::graph::L3::Handle handle0;
    handle0.addOp(op0);
    handle0.setUp();

    //---------------- setup number of vertices in each PU ---------
    for (int i = 0; i < splitNm; ++i) {
        numVerticesPU[i] = general;
    }
    numVerticesPU[splitNm - 1] = rest;

    int sourceID = 3; // source ID

    int32_t* numElementsPU;
    numElementsPU = new int32_t[splitNm];
    for (int i = 0; i < splitNm; i++) {
        numEdgesPU[i] = numEdges;
        numElementsPU[i] = numEdges * numVerticesPU[i];
    }

    xf::graph::Graph<int32_t, int32_t>** g = new xf::graph::Graph<int32_t, int32_t>*[deviceNeeded];
    int fpgaNodeNm = 0;
    for (int i = 0; i < deviceNeeded; ++i) {
        g[i] = new xf::graph::Graph<int32_t, int32_t>("Dense", 4 * splitNm, numEdges, numVerticesPU);
        g[i][0].numEdgesPU = new int32_t[splitNm];
        g[i][0].numVerticesPU = new int32_t[splitNm];
        g[i][0].edgeNum = numEdges;
        g[i][0].nodeNum = numVertices;
        g[i][0].splitNum = splitNm;
        g[i][0].refID = fpgaNodeNm;
        for (int j = 0; j < splitNm; ++j) {
            fpgaNodeNm += numVerticesPU[j];
            g[i][0].numVerticesPU[j] = numVerticesPU[j];
            g[i][0].numEdgesPU[j] = numEdgesPU[j] * numVerticesPU[j];
        }
    }
    std::fstream weightfstream(filenameWeight.c_str(), std::ios::in);
    if (!weightfstream) {
        std::cout << "Error: " << filenameWeight << "weight file doesn't exist !" << std::endl;
        exit(1);
    }

    int sumVertex = 0;
    for (int i = 0; i < splitNm; i++) { // offset32 buffers allocation
        sumVertex += numVerticesPU[i];
    }
    if (sumVertex != numVertices) { // vertex numbers between file input and numVerticesPU should match
        std::cout << "Error : sum of PU vertex numbers doesn't match file input vertex number!" << std::endl;
        exit(1);
    }

    readInWeight<splitNm>(weightfstream, numEdges, numVerticesPU, g[0][0].weightsDense);

    std::cout << "INFO: numVertice=" << numVertices << std::endl;
    std::cout << "INFO: numEdges=" << numEdges << std::endl;

    //---------------- Generate Source Indice and Weight Array -------
    int sourceLen;         // sourceIndice array length
    int32_t* sourceWeight; // weights of source vertex's out members
    generateSourceParams<splitNm>(numVerticesPU, numEdges, sourceID, g[0][0].weightsDense, sourceLen, &sourceWeight);

    //---------------- Load Graph -----------------------------------
    for (int i = 0; i < deviceNeeded; ++i) {
        (handle0.opsimdense)->loadGraphMultiCardNonBlocking(i, g[i][0]);
    }
    float* similarity = xf::graph::L3::aligned_alloc<float>(topK);
    int32_t* resultID = xf::graph::L3::aligned_alloc<int32_t>(topK);
    memset(resultID, 0, topK * sizeof(int32_t));
    memset(similarity, 0, topK * sizeof(float));

    //---------------- Run L3 API -----------------------------------
    int ret = xf::graph::L3::cosineSimilaritySSDenseMultiCard(handle0, deviceNeeded, sourceLen, sourceWeight, topK, g,
                                                              resultID, similarity);

    (handle0.opsimdense)->join();
    handle0.free();
    for (int i = 0; i < deviceNeeded; ++i) {
        g[i]->freeBuffers();
        delete[] g[i]->numEdgesPU;
        delete[] g[i]->numVerticesPU;
    }
    delete[] g;
    //---------------- Check Result ---------------------------------
    int err = checkData<splitNm>(goldenFile, resultID, similarity);
    delete[] numElementsPU;
    delete[] numVerticesPU;
    delete[] numEdgesPU;
    free(similarity);
    free(resultID);

    if (err == 0) {
        std::cout << "INFO: Results are correct" << std::endl;
        return 0;
    } else {
        std::cout << "Error: Results are false" << std::endl;
        return 1;
    }
}
