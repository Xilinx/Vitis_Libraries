/*
 * Copyright 2022 Xilinx, Inc.
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
#ifndef XLNX_DATA_ENGINE_SC_HPP
#define XLNX_DATA_ENGINE_SC_HPP

#include <iostream>
#include <thread>
#include <atomic>
#include <queue>
#include <future>
#include <condition_variable>
#include <mutex>
#include "types.hpp"
#include "data_engine_config.hpp"
#include "data_engine_acc.hpp"
#include "vpp_queue.hpp"

namespace sssd_engine {
namespace data_engine_sc {
struct QueueRequest {
    std::string file_path;
    size_t size; // file_size
    uint64_t* cfg_buf;
    bool valid;
    uint32_t file_nm;
    std::promise<RetObj> p;
};

struct FileDescriptors {
    int* fd;
};

/**
 * @brief class to manage kernels on single device
 */
class DataEngine {
   private:
    /**
     * @brief execute kernel
     * @param file_path file path.
     * @param size size of input file in bytes
     * @param cfg pointer to configuration buffer
     * @return object that contains the scan error code and poninter to result buffer
     */
    RetObj run_all(std::string file_path, size_t size, uint64_t* cfg);

    // queue to store request

    std::list<QueueRequest> run_q;
    std::mutex p_mutex;
    std::condition_variable p_cond;

    // queue to store request
    std::list<QueueRequest> ex_q;
    std::mutex e_mutex;
    std::condition_variable e_cond;

    // flag to indicate thread is running
    std::atomic<bool> is_running;

    uint64_t* dummy_cfg;
    /**
     * @brief execute data engine.
     */
    void run();

    std::string last_file;

    VPP_BP csvInBufPool;
    VPP_BP cfgInBufPool;
    VPP_BP outBufPool;
    VPP_BP metaBufPool;
    VPP_CC* cuCluster;

    FILE* log_ptr;
    int card_id;
    double pass_ratio;

    vpp::squeue<char*> tokens;

   public:
    /**
     * @brief constructor of data engine.
     * context, program, command queue are created and ready after FPGA init
     *
     * @param t_id targeted device id.
     * @param _csvInBufPool input CSV buffer pool
     * @param _cfgInBufPool input configuration buffer pool
     * @param _outBufPool output result buffer pool
     * @param _metaBufPool output meta buffer pool
     * @param _cuCluster CU clusters
     *
     */
    DataEngine(int t_id,
               VPP_BP& _csvInBufPool,
               VPP_BP& _cfgInBufPool,
               VPP_BP& _outBufPool,
               VPP_BP& _metaBufPool,
               VPP_CC* _cuCluster);

    /**
     * @brief default deconstructor
     *
     */
    ~DataEngine();

    /**
     * @brief thread for run data engine;
     *
     */
    std::thread run_t;

    /**
     * @brief push request to queue
     *
     * @param prom promise to synchronize status of execution
     * @param file_path file path
     * @param size file size
     * @param cfg pointer to configuration buffer
     *
     */
    void pushRequest(std::promise<RetObj> prom, std::string file_path, size_t size, uint64_t* cfg);
    /**
     * @brief release buffer
     *
     * @param buf buffer to be released
     *
     */
    inline void release(char* buf) { tokens.put(buf); }
};
} // namespace data_engine_sc
} // namespace sssd_engine
#endif
