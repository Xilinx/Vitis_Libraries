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
#ifndef DATA_ENGINE_SW_API_H
#define DATA_ENGINE_SW_API_H

#include <memory>
#include <unordered_map>
#include <string.h>
#include <vector>
#include <thread>
#include <future>
#include <fstream>
#include "sssd_api.h"
#include "types.hpp"
#include "data_engine_sc.hpp"

namespace sssd_engine {
/**
 *@brief class to handle a specific SSD device
 */
class SmartSSDCache {
   private:
    /**
     * Objects of DataEngines
     */
    data_engine_sc::DataEngine* engine[16];
    VPP_BP csvInBufPool;
    VPP_BP cfgInBufPool;
    VPP_BP outBufPool;
    VPP_BP metaBufPool;
    VPP_CC* cuCluster;

    /**
     * Object of DataEngineConfig
     */
    DataEngineConfig dg_cfg;

    /**
     * prefix of file path
     */
    std::vector<MountInfo> mount_info;

    /**
     * number of card
     */
    int card_nm;

    FILE* log_ptr;

    /**
     * @brief get the file size
     * @param file_path file path
     */
    inline size_t getFileSize(std::string file_path) {
        std::ifstream fs;
        fs.open(file_path.c_str(), std::ios::binary);
        fs.seekg(0, std::ios_base::end);
        size_t size = fs.tellg();
        fs.close();
        return size;
    }

    /**
     * @brief Check basename and get the disk id based on mount info
     * @param file_path file path
     */
    int32_t getDiskID(const std::string& file_path);

   public:
    /**
     * @brief get number of cards
     *
     * @return number of cards
     */
    int getCardNum() { return card_nm; };

    /**
     * @brief default constructor, instantiate data engine
     *
     * @param xclbin_path path of xclbin
     * @param card_num number of availabe SmartSSD cards
     * @param disks information of the SSDs
     * @param log file handler for dumping the logs
     *
     */
    SmartSSDCache(const char* xclbin_path, int card_num, sssd_info_t* disks, FILE* log);

    /**
     * @brief default deconstructor
     *
     */
    ~SmartSSDCache();

    /**
     * @brief store file to managed SmartSSD cards. Copy data from CPU memory to SSD cards
     *
     * @param data pointer to file content in memory. Before return, the block of storage will be released
     * @param size file size
     * @param filename file to store
     * @return success or fail status
     *
     */
    ErrorCode addFile(std::unique_ptr<char[]> data, size_t size, const std::string& filename);

    /**
     * @brief scan files from managed SmartSSD cards.
     *
     * @param fname file name;
     * @param sd pointer to configuration including schema, projection and filter conditions
    filenames.
     * @param err return the status of exectution.
     * @return pointer the selected row. It is stored in format defined by data type in schema.
     *
     */
    char* scanFile(const char* fname, sssd_scandesc_t* sd, ErrorCode& err);

    /**
     * @brief list all the file name that matche the path_pattern
     * @param path_pattern specify the path_pattern for regex
     * @param file_list store the file name that matched the path_pattern
     * @return fail/success
     *
     */
    ErrorCode listFiles(const char* path_pattern, std::vector<std::string>& file_list);

    /**
     * @brief print input information
     *
     * @param sd the input schema
     *
     */
    void print_input(const sssd_scandesc_t* sd);

    /**
     * @brief print output information
     *
     * @param value values
     * @param isnull is null flags
     * @param hash hash value
     * @param sd the schema
     *
     */
    void print_output(const int64_t value[], const bool isnull[], int32_t hash, const sssd_scandesc_t* sd);

    /**
     * @brief release specified buffer
     *
     * @param file_path path to the file
     * @param buf_ptr buffer to be released
     *
     */
    void release(const char* file_path, char* buf_ptr);
};
} // namespace sssd_engine
#endif
