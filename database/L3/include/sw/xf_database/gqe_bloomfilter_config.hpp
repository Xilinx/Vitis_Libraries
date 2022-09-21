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

#ifndef _GQE_BLOOMFILTER_CONFIG_L3_
#define _GQE_BLOOMFILTER_CONFIG_L3_
// commmon
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <iomanip>
// HLS
#include <ap_int.h>
// L2
#include "xf_database/gqe_utils.hpp"
// L3
#include "xf_database/gqe_table.hpp"
#include "xf_database/gqe_input.hpp"
#include "xf_database/gqe_base_config.hpp"

namespace xf {
namespace database {
namespace gqe {

class BloomFilterConfig : protected BaseConfig {
   private:
    // sw-shuffle for scan, determins the column order while scanning data in
    std::vector<std::vector<int8_t> > sw_shuffle_scan_bf;
    // sw-shuffle for write out, determins the column order while writing data out
    std::vector<int8_t> sw_shuffle_wr_bf;

    // the kernel config passed to gqeJoin kernel
    ap_uint<512>* table_bf_cfg;

    // setup kernel config (table_bf_cfg) for gqeJoin
    void SetupKernelConfig(bool build_probe,
                           std::string filter_a,
                           std::string filter_b,
                           bool gen_rowID_en[2],
                           bool valid_en[2],
                           std::vector<std::vector<std::string> > bf_keys,
                           std::vector<std::vector<int8_t> > sw_shuffle_scan_bf,
                           std::vector<int8_t> sw_shuffle_wr_bf);

   public:
    /**
    * @brief constructor of BloomFilterConfig.
    *
    * The class generates bloom filter configure bits by parsing the bloom filter run arguments,
    *
    * @param a left table
    * @param filter_a filter condition of left table
    * @param b right table
    * @param filter_b filter condition of right table
    * @param bf_str bloomfilter condition(s)
    * @param c result table
    * @param output_str output column mapping
    * @param build_probe true for probe, false for build
    *
    */
    BloomFilterConfig(TableSection a,
                      std::string filter_a,
                      TableSection b,
                      std::string filter_b,
                      std::string bf_str, // comma separated
                      TableSection c,
                      std::string output_str, // comma separated
                      bool build_probe);

    /**
     * @brief get the gqeJoin kernel config
     *
     * @return bloom filter config bits
     */
    ap_uint<512>* getBloomFilterConfigBits() const;

    /**
     * @brief get the sw-shuffle config for scan.
     *
     * @return return the scan sw_shuffle cfg
    */
    std::vector<std::vector<int8_t> > getShuffleScan() const;

    /**
     * @brief get the sw-shuffle config for write out.
     *
     * @return return the write out sw_shuffle cfg
    */
    std::vector<int8_t> getShuffleWrite() const;
};
}
}
}
#endif
