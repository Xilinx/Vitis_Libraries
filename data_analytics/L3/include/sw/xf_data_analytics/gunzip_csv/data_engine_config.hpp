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
#ifndef _DATA_ENGINE_CONFIG_H_
#define _DATA_ENGINE_CONFIG_H_

#include <vector>
#include <string>
#include <stdint.h>
#include "ap_int.h"
#include "types.hpp"

#define DDR_SIZE_META_LWORD 64
#define DDR_SIZE_OUT_BYTE (1024 * 1024 * 1024)
#define DDR_SIZE_CFG_LWORD 128

#define N 12

namespace sssd_engine {
/**
 * @brief generating configuration bits for CSV scanner kernel
 */
class DataEngineConfig {
   private:
    ap_uint<64> convert(sssd_filter_t* filter);

   public:
    /**
     * @brief default constructor
     *
     */
    DataEngineConfig(){};
    /**
     * @brief generate configuration bits for kernel
     *
     * @param gzip file under gzip or not
     * @param sd schema for describing the table
     * @param cfg kernel configurations
     * @return error code
     *
     */
    ErrorCode genConfigBits(bool gzip, sssd_scandesc_t* sd, uint64_t* cfg);
};
} // namespace sssd_engine
#endif
