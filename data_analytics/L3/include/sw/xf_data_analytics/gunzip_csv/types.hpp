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
#ifndef DATA_ENGINE_TYPES_HPP
#define DATA_ENGINE_TYPES_HPP

#include <string>
#include <memory>
#include "sssd_api.h"

/**
 * Define data struct which contains the scan error code and pointer which points to the location of result
 */
struct RetObj {
    ErrorCode status;
    char* data;
    size_t size;
};

/**
 * Define information of file stored in memory.
 */
struct FileInfo {
    std::string file_path;
    size_t size;
    int disk;
};
struct MountInfo {
    int32_t device_id;
    std::string mount_path;
};

// Example for Q6:
/*
ScanConfig cfg;
cfg.schema = "l_quantity:DECIMAL(12,2), l_extendedprice:DECIMAL(12,2), l_discount:DECIMAL(12,2), l_shipdate:DATE";
cfg.filter =
    "l_shipdate > '19940101' && l_shipdate< '19950101' && l_discount> '0.05' && l_discount < '0.07' && "
    "l_quantity < '24'";
cfg.aggregate = "sum(l_extendedprice * l_discount)";
cfg.group_keys = "";
cfg.out_str = "sum(l_extendedprice * l_discount)";
cfg.decompression = DecompressMethod::GZIP;
*/

// Example for Q1:
/*
ScanConfig cfg;
cfg.schema = "l_returnflag:CHAR(1), l_linstatus:CHAR(1), l_quantity:DECIMAL(12,2), l_extendedprice:DECIMAL(12,2)";
cfg.filter = "l_shipdate > '19980901' && l_shipdate < '19990301'";
cfg.aggregate = "sum(l_qunatity), sum(1_extentedprice * (1 – discount))";
cfg.group_keys = "l_returnflag, l_linestatus";
cfg.out_str = "l_returnflag, l_linestatus, sum(1_quantity), sum(1_extentedprice * (1 – discount)), count(*)";
cfg.decompression = DecompressMethod::NONE;
*/
#endif
