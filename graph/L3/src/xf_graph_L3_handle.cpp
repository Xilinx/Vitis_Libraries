/*
* Copyright (C) 2019-2022, Xilinx, Inc.
* Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#ifndef _XF_GRAPH_L3_HANDLE_CPP_
#define _XF_GRAPH_L3_HANDLE_CPP_

#include "xf_graph_L3_handle.hpp"

namespace xf {
namespace graph {
namespace L3 {

int Handle::initOpLouvainModularity(std::string xclbinPath,
                                    std::string kernelName,
                                    std::string kernelAlias,
                                    unsigned int requestLoad,
                                    unsigned int deviceNeeded,
                                    unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    // fetchCuInfo will scan all available devices/CUs.
    std::cout << "INFO: Init Louvain" << std::endl;
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    oplouvainmod->setHWInfo(deviceNm, maxCU);
    oplouvainmod->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    oplouvainmod->initRoundRobinThread(xrm, oplouvainmod->handles, kernelName, kernelAlias, requestLoad, deviceNeeded,
                                       cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpTwoHop(std::string kernelName,
                         std::string xclbinPath,
                         std::string kernelAlias,
                         unsigned int requestLoad,
                         unsigned int deviceNeeded,
                         unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init twoHop" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    optwohop->setHWInfo(deviceNm, maxCU);
    optwohop->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    optwohop->initThread(xrm, optwohop->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpSP(std::string kernelName,
                     std::string xclbinPath,
                     std::string kernelAlias,
                     unsigned int requestLoad,
                     unsigned int deviceNeeded,
                     unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init SP" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opsp->setHWInfo(deviceNm, maxCU);
    opsp->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opsp->initThread(xrm, opsp->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpTriangleCount(std::string kernelName,
                                std::string xclbinPath,
                                std::string kernelAlias,
                                unsigned int requestLoad,
                                unsigned int deviceNeeded,
                                unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init triangleCount" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    optcount->setHWInfo(deviceNm, maxCU);
    optcount->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    optcount->initThread(xrm, optcount->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpLabelPropagation(std::string kernelName,
                                   std::string xclbinPath,
                                   std::string kernelAlias,
                                   unsigned int requestLoad,
                                   unsigned int deviceNeeded,
                                   unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init labelPropagation" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    oplprop->setHWInfo(deviceNm, maxCU);
    oplprop->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    oplprop->initThread(xrm, oplprop->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpBFS(std::string kernelName,
                      std::string xclbinPath,
                      std::string kernelAlias,
                      unsigned int requestLoad,
                      unsigned int deviceNeeded,
                      unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init BFS" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opbfs->setHWInfo(deviceNm, maxCU);
    opbfs->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opbfs->initThread(xrm, opbfs->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpWCC(std::string kernelName,
                      std::string xclbinPath,
                      std::string kernelAlias,
                      unsigned int requestLoad,
                      unsigned int deviceNeeded,
                      unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init WCC" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opwcc->setHWInfo(deviceNm, maxCU);
    opwcc->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opwcc->initThread(xrm, opwcc->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpSCC(std::string kernelName,
                      std::string xclbinPath,
                      std::string kernelAlias,
                      unsigned int requestLoad,
                      unsigned int deviceNeeded,
                      unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init SCC" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opscc->setHWInfo(deviceNm, maxCU);
    opscc->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opscc->initThread(xrm, opscc->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpConvertCsrCsc(std::string kernelName,
                                std::string xclbinPath,
                                std::string kernelAlias,
                                unsigned int requestLoad,
                                unsigned int deviceNeeded,
                                unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init convertCsrCsc" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opconvertcsrcsc->setHWInfo(deviceNm, maxCU);
    opconvertcsrcsc->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opconvertcsrcsc->initThread(xrm, opconvertcsrcsc->handles, kernelName, kernelAlias, requestLoad, deviceNeeded,
                                cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpPageRank(std::string kernelName,
                           std::string xclbinPath,
                           std::string kernelAlias,
                           unsigned int requestLoad,
                           unsigned int deviceNeeded,
                           unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init pageRank" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    oppg->setHWInfo(deviceNm, maxCU);
    oppg->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    oppg->initThread(xrm, oppg->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpSimDense(std::string kernelName,
                           std::string xclbinPath,
                           std::string kernelAlias,
                           unsigned int requestLoad,
                           unsigned int deviceNeeded,
                           unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init similarityDense" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opsimdense->setHWInfo(deviceNm, maxCU);
    opsimdense->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opsimdense->initThread(xrm, opsimdense->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpSimDenseInt(std::string kernelName,
                              std::string xclbinPath,
                              std::string kernelAlias,
                              unsigned int requestLoad,
                              unsigned int deviceNeeded,
                              unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init similarityDenseInt" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;
    opsimdense->setHWInfo(deviceNm, maxCU);
    opsimdense->initInt(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opsimdense->initRoundRobinThread(xrm, opsimdense->handles, kernelName, kernelAlias, requestLoad, deviceNeeded,
                                     cuPerBoard);

    delete[] cuID;
    delete[] deviceID;
    return 0;
};

int Handle::initOpSimSparse(std::string kernelName,
                            std::string xclbinPath,
                            std::string kernelAlias,
                            unsigned int requestLoad,
                            unsigned int deviceNeeded,
                            unsigned int cuPerBoard) {
    uint32_t* deviceID;
    uint32_t* cuID;
    int32_t status = 0;

    std::cout << "INFO: Init similaritySparse" << std::endl;
    // fetchCuInfo will scan all available devices/CUs.
    deviceNm = deviceNeeded; // set initial requested number of devices
    status = xrm->fetchCuInfo(kernelName.c_str(), kernelAlias.c_str(), requestLoad, deviceNm, cuPerBoard,
                              maxChannelSize, maxCU, &deviceID, &cuID);
    if (status < 0) return status;

    opsimsparse->setHWInfo(deviceNm, maxCU);
    opsimsparse->init(xrm, kernelName, kernelAlias, xclbinPath, deviceID, cuID, requestLoad);
    opsimsparse->initThread(xrm, opsimsparse->handles, kernelName, kernelAlias, requestLoad, deviceNeeded, cuPerBoard);
    delete[] cuID;
    delete[] deviceID;
    return 0;
};

void Handle::addOp(singleOP op) {
    ops.push_back(op);
}

int Handle::setUp(std::string deviceNames) {
    const std::string delimiters(" ");
    for (int i = deviceNames.find_first_not_of(delimiters, 0); i != std::string::npos;
         i = deviceNames.find_first_not_of(delimiters, i)) {
        auto tokenEnd = deviceNames.find_first_of(delimiters, i);
        if (tokenEnd == std::string::npos) tokenEnd = deviceNames.size();
        const std::string token = deviceNames.substr(i, tokenEnd - i);
        supportedDeviceNames.push_back(token);
        std::cout << token << std::endl;
        i = tokenEnd;
    }
    getEnvMultiBoards();

    unsigned int opNm = ops.size();
    unsigned int deviceCounter = 0;
    int32_t status = 0;

    for (int i = 0; i < opNm; ++i) {
        if (ops[i].operationName == "louvainModularity") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            // Unload existing xclbin first if present
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
#ifndef NDEBUG
                std::cout << "DEBUG: "
                          << "xrm->unloadXclbinNonBlock devId=" << supportedDeviceIds[j] << std::endl;
#endif
                thUn[j] = xrm->unloadXclbinNonBlock(supportedDeviceIds[j]);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }

            // load xclbin asynchronously (i.e. non-blocking) using thread
            std::future<int> th[boardNm];

            for (int j = 0; j < boardNm; ++j) {
#ifndef NDEBUG
                std::cout << "DEBUG: " << __FUNCTION__ << ": xrm->loadXclbinAsync "
                          << "\n    devId=" << supportedDeviceIds[j] << "\n    ops[i].xclbinFile=" << ops[i].xclbinPath
                          << std::endl;
#endif
                th[j] = loadXclbinAsync(supportedDeviceIds[j], ops[i].xclbinPath);
            }

            // wait for thread to finish
            for (int j = 0; j < boardNm; ++j) {
                auto loadedDevId = th[j].get();
                if (loadedDevId < 0) {
                    std::cout << "ERROR: Failed to load " << ops[i].xclbinPath << "(Status=" << loadedDevId
                              << "). Please check if it is "
                              << "created for the Xilinx Acceleration card installed on "
                              << "the server." << std::endl;
                    return loadedDevId;
                }
            }
#endif
            deviceCounter += boardNm;
            status = initOpLouvainModularity(ops[i].xclbinPath, ops[i].kernelName, ops[i].kernelAlias,
                                             ops[i].requestLoad, ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "twoHop") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpTwoHop(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                  ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "pagerank") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpPageRank(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                    ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "shortestPathFloat") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpSP(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                              ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "similarityDense") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::future<int> th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinAsync(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                auto loadedDevId = th[j].get();
                if (loadedDevId < 0) {
                    std::cout << "ERROR: failed to load " << ops[i].xclbinPath << "(Status=" << loadedDevId
                              << "). Please check if it is "
                              << "created for the Xilinx Acceleration card installed on "
                              << "the server." << std::endl;
                    return loadedDevId;
                }
            }
#endif
            deviceCounter += boardNm;
            status = initOpSimDense(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                    ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "similarityDenseInt") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::future<int> th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinAsync(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                auto loadedDevId = th[j].get();
                if (loadedDevId < 0) {
                    std::cout << "ERROR: failed to load " << ops[i].xclbinPath << "(Status=" << loadedDevId
                              << "). Please check if it is "
                              << "created for the Xilinx Acceleration card installed on "
                              << "the server." << std::endl;
                    return loadedDevId;
                }
            }
#endif
            deviceCounter += boardNm;
            status = initOpSimDenseInt(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                       ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "similaritySparse") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpSimSparse(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                     ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "triangleCount") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpTriangleCount(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                         ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "labelPropagation") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpLabelPropagation(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias,
                                            ops[i].requestLoad, ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "BFS") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpBFS(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                               ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "WCC") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpWCC(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                               ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "SCC") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpSCC(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                               ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else if (ops[i].operationName == "convertCsrCsc") {
            unsigned int boardNm = ops[i].deviceNeeded;
            if (deviceCounter + boardNm > totalSupportedDevices) {
                std::cout << "Error: Need more devices" << std::endl;
                exit(1);
            }
#if !(SW_EMU_TEST || HW_EMU_TEST)
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            std::thread th[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                th[j] = loadXclbinNonBlock(deviceCounter + j, ops[i].xclbinPath);
            }
            for (int j = 0; j < boardNm; ++j) {
                th[j].join();
            }
#endif
            deviceCounter += boardNm;
            status = initOpConvertCsrCsc(ops[i].kernelName, ops[i].xclbinPath, ops[i].kernelAlias, ops[i].requestLoad,
                                         ops[i].deviceNeeded, ops[i].cuPerBoard);
        } else {
            std::cout << "Error: the operation " << ops[i].operationName << " is not supported" << std::endl;
            exit(1);
        }
        if (status < 0) return XF_GRAPH_L3_ERROR_ALLOC_CU;
    }
    return 0;
}

int Handle::setUp() {
    // std::string deviceNames = "xilinx_u50_gen3x16_xdma_201920_3";
    std::string deviceNames =
        "xilinx_u50_gen3x16_xdma_201920_3 xilinx_u50_gen3x16_xdma_5_202210_1 xilinx_u55c_gen3x16_xdma_3_202210_1";
    return setUp(deviceNames);
}

void Handle::getEnvMultiBoards() {
    cl_uint platformID = 0;
    cl_platform_id* platforms = NULL;
    char vendor_name[128] = {0};
    cl_uint num_platforms = 0;
    cl_int err2 = clGetPlatformIDs(0, NULL, &num_platforms);
    if (CL_SUCCESS != err2) {
        std::cout << "INFO: get platform failed1" << std::endl;
    }
    platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);
    if (NULL == platforms) {
        std::cout << "INFO: allocate platform failed" << std::endl;
    }
    err2 = clGetPlatformIDs(num_platforms, platforms, NULL);
    if (CL_SUCCESS != err2) {
        std::cout << "INFO: get platform failed2" << std::endl;
    }
    for (cl_uint ui = 0; ui < num_platforms; ++ui) {
        err2 = clGetPlatformInfo(platforms[ui], CL_PLATFORM_VENDOR, 128 * sizeof(char), vendor_name, NULL);
        if (CL_SUCCESS != err2) {
            std::cout << "INFO: get platform failed3" << std::endl;
        } else if (!std::strcmp(vendor_name, "Xilinx")) {
            platformID = ui;
        }
    }
    cl_device_id* devices;
    std::vector<cl::Device> devices0 = xcl::get_xil_devices();
    uint32_t totalXilinxDevices = devices0.size();
    totalSupportedDevices = 0;
    devices = (cl_device_id*)malloc(sizeof(cl_device_id) * totalXilinxDevices);
    err2 = clGetDeviceIDs(platforms[platformID], CL_DEVICE_TYPE_ALL, totalXilinxDevices, devices, NULL);
    std::cout << "INFO: total xilinx devices = " << totalXilinxDevices << std::endl;
    size_t valueSize;
    char* value;

    for (int i = 0; i < totalXilinxDevices; ++i) {
        // print device name
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 0, NULL, &valueSize);
        value = new char[valueSize];
        clGetDeviceInfo(devices[i], CL_DEVICE_NAME, valueSize, value, NULL);
        std::cout << "INFO: " << __FUNCTION__ << ": Scanned device " << i << ":" << value << std::endl;
        std::string strValue = value;
        std::size_t found50 = strValue.rfind("u50");
        std::size_t found55 = strValue.rfind("u55c");
        if (found50 != std::string::npos) {
            value = "u50";
        }
        if (found55 != std::string::npos) {
            value = "u55c";
        }
        for (int j = 0; j < supportedDeviceNames.size(); j++) {
            if (supportedDeviceNames[j].rfind(value) != std::string::npos) {
                std::cout << "    Supported device found:" << value << std::endl;
                supportedDeviceIds[totalSupportedDevices++] = i; // save curret supported supported devices
                break;
            }
        }
        // delete value;
    }

    std::cout << "INFO: Total matching devices: " << totalSupportedDevices << std::endl;
}

void Handle::showHandleInfo() {
#ifndef NDEBUG
    std::cout << "INFO: " << __FUNCTION__ << " deviceNm_=" << deviceNm << " maxCU=" << maxCU << std::endl;
    unsigned int opNm = ops.size();
    for (unsigned int i = 0; i < opNm; ++i) {
        std::cout << "INFO: " << __FUNCTION__ << " operationName=" << ops[i].operationName
                  << " kernelname=" << ops[i].kernelName << " requestLoad=" << ops[i].requestLoad
                  << " xclbinFile=" << ops[i].xclbinPath << std::endl;
    }
#endif
}

void Handle::free() {
    unsigned int opNm = ops.size();
    unsigned int deviceCounter = 0;
    for (int i = 0; i < opNm; ++i) {
        if (ops[i].operationName == "louvainModularity") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            oplouvainmod->freeLouvainModularity(xrm->ctx);
        } else if (ops[i].operationName == "twoHop") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            optwohop->freeTwoHop(xrm->ctx);
        } else if (ops[i].operationName == "pagerank") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            oppg->freePG(xrm->ctx);
        } else if (ops[i].operationName == "shortestPathFloat") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opsp->freeSP(xrm->ctx);
        } else if (ops[i].operationName == "similarityDense") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opsimdense->freeSimDense(xrm->ctx);
        } else if (ops[i].operationName == "similarityDenseInt") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opsimdense->freeSimDense(xrm->ctx);
        } else if (ops[i].operationName == "similaritySparse") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opsimsparse->freeSimSparse(xrm->ctx);
        } else if (ops[i].operationName == "triangleCount") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            optcount->freeTriangleCount(xrm->ctx);
        } else if (ops[i].operationName == "labelPropagation") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            oplprop->freeLabelPropagation(xrm->ctx);
        } else if (ops[i].operationName == "BFS") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opbfs->freeBFS(xrm->ctx);
        } else if (ops[i].operationName == "WCC") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opwcc->freeWCC(xrm->ctx);
        } else if (ops[i].operationName == "SCC") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opscc->freeSCC(xrm->ctx);
        } else if (ops[i].operationName == "convertCsrCsc") {
            unsigned int boardNm = ops[i].deviceNeeded;
            std::thread thUn[boardNm];
            for (int j = 0; j < boardNm; ++j) {
                thUn[j] = xrm->unloadXclbinNonBlock(deviceCounter + j);
            }
            for (int j = 0; j < boardNm; ++j) {
                thUn[j].join();
            }
            deviceCounter += boardNm;
            opconvertcsrcsc->freeConvertCsrCsc(xrm->ctx);
        }
    }
    xrm->freeXRM();
};

void Handle::loadXclbin(unsigned int deviceId, std::string& xclbinName) {
    xrm->loadXclbin(deviceId, xclbinName);
};

std::thread Handle::loadXclbinNonBlock(unsigned int deviceId, std::string& xclbinName) {
    return xrm->loadXclbinNonBlock(deviceId, xclbinName);
};

std::future<int> Handle::loadXclbinAsync(unsigned int deviceId, std::string& xclbinName) {
    return xrm->loadXclbinAsync(deviceId, xclbinName);
};

} // L3
} // graph
} // xf
#endif
