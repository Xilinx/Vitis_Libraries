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
#include "xf_database/gqe_bloomfilter_config.hpp"
#include "xf_database/kernel_command.hpp"
#define USER_DEBUG 1
namespace xf {
namespace database {
namespace gqe {

// init to setup the join config, which includes:
//- sw_cfg: sw_shuffle cfg for scan and wr
//- krn_cfg: gqe kernel used config
BloomFilterConfig::BloomFilterConfig(TableSection a,
                                     std::string filter_a,
                                     TableSection b,
                                     std::string filter_b,
                                     std::string bf_str, // comma separated
                                     TableSection c,
                                     std::string output_str,
                                     bool build_probe) {
    // 0) process gen-rowid_en  and val_en
    bool gen_rowID_en[2];
    bool valid_en[2];

    gen_rowID_en[0] = a.getRowIDEnableFlag();
    gen_rowID_en[1] = b.getRowIDEnableFlag();
    valid_en[0] = a.getValidEnableFlag();
    valid_en[1] = b.getValidEnableFlag();

    // 1) read in col names of table a, b, and c
    std::vector<std::string> a_col_names = a.getColNames();
    std::vector<std::string> b_col_names = b.getColNames();
    std::vector<std::string> c_col_names = c.getColNames();
    int table_b_valid_col_num = b_col_names.size();
    // verify the column num in table A and B is no more than 3
    CHECK_0(a_col_names, 3, "A");
    CHECK_0(b_col_names, 3, "B");
    // remove the space in str
    xf::database::internals::filter_config::trim(bf_str);
    xf::database::internals::filter_config::trim(output_str);

#ifdef USER_DEBUG
    std::cout << "1. Get cols from table" << std::endl;
    std::cout << "Table A: ";
    for (size_t i = 0; i < a_col_names.size(); i++) {
        std::cout << a_col_names[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Table B: ";
    for (size_t i = 0; i < b_col_names.size(); i++) {
        std::cout << b_col_names[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "Table C: ";
    for (size_t i = 0; i < c_col_names.size(); i++) {
        std::cout << c_col_names[i] << " ";
    }
    std::cout << std::endl << "------------------" << std::endl;
#endif

    // 2) extract join key col and write out col from input info
    //  join keys extracted from two input tables
    std::vector<std::vector<std::string> > bf_keys;
    bf_keys = extractKeys(bf_str);

    // write out cols name extracted from table out
    std::vector<std::string> write_out_cols;
    write_out_cols = extractWcols(bf_keys, c_col_names, output_str);

    // 3) calc the sw_shuffle_scan cfg
    sw_shuffle_scan_bf.resize(2);
    // sw scan-shuffle, puts key cols to 1st and 2nd col
    shuffleScan(filter_a, bf_keys[0], write_out_cols, a_col_names, sw_shuffle_scan_bf[0]);
    shuffleScan(filter_b, bf_keys[1], write_out_cols, b_col_names, sw_shuffle_scan_bf[1]);

#ifdef USER_DEBUG
    std::cout << "3.1. sw_shuffle_scan_bf_a: ";
    for (size_t i = 0; i < sw_shuffle_scan_bf[0].size(); i++) {
        std::cout << (int)sw_shuffle_scan_bf[0][i] << " ";
    }
    std::cout << std::endl;
    std::cout << "after sw_shuffle scan column names: " << std::endl;
    for (size_t i = 0; i < a_col_names.size(); i++) {
        std::cout << a_col_names[i] << " ";
    }
    std::cout << std::endl;

    std::cout << "3.2. sw_shuffle_scan_bf_b: ";
    for (size_t i = 0; i < sw_shuffle_scan_bf[1].size(); i++) {
        std::cout << (int)sw_shuffle_scan_bf[1][i] << " ";
    }
    std::cout << std::endl;
    std::cout << "after sw_shuffle scan column names: " << std::endl;
    for (size_t i = 0; i < b_col_names.size(); i++) {
        std::cout << b_col_names[i] << " ";
    }
    std::cout << std::endl;

#endif
    // updating row-id col name
    std::cout << "before updateRowIDCol, filter_a = " << filter_a << " filter_b = " << filter_b << std::endl;
    updateRowIDCol(filter_a, a_col_names, a.getRowIDColName());
    updateRowIDCol(filter_b, b_col_names, b.getRowIDColName());

#ifdef USER_DEBUG
    std::cout << "3.3 after add row-id, column names: " << std::endl;
    // tab a
    for (size_t i = 0; i < a_col_names.size(); i++) {
        std::cout << a_col_names[i] << " ";
    }
    // tab b
    for (size_t i = 0; i < b_col_names.size(); i++) {
        std::cout << b_col_names[i] << " ";
    }

    std::cout << std::endl << "------------------" << std::endl;
#endif

    // 4) calc the shuffle wr cfg by "mimic kernel out col ==> compare with write out col"
    // define the bloom filtered cols that mimic the hardware shuffle Join Keys
    int bf_keys_num = 1;
    std::vector<std::string> col_out_names;
    if (bf_str != "") {
        bf_keys_num = bf_keys[0].size();

        // l table rowid col name write to col_out_names
        col_out_names.push_back(b.getRowIDColName());
        col_out_names.push_back(a.getRowIDColName());
        // insert bf keys
        col_out_names.insert(col_out_names.end(), a_col_names.begin(), a_col_names.begin() + bf_keys_num);
        if (bf_keys_num == 1) {
            col_out_names.push_back("unused");
        }
    } else {
        if (table_b_valid_col_num != 0) {
            std::cout << "WARNING: Bypass data and ignore table b." << std::endl;
        }
        std::copy(a_col_names.begin(), a_col_names.end(), std::back_inserter(col_out_names));
    }
#ifdef USER_DEBUG
    std::cout << "4.1. After BF, column names: " << std::endl;
    for (size_t i = 0; i < col_out_names.size(); i++) {
        std::cout << col_out_names[i] << " ";
    }
    std::cout << std::endl;
#endif

    // get the shuffle_wr config
    // the cols before shuffle_wr: col_out_names
    // the cols after shuffle_wr: extractWCols result: write_out_cols
    sw_shuffle_wr_bf = shuffleWrite(col_out_names, write_out_cols);

#ifdef USER_DEBUG
    for (size_t i = 0; i < sw_shuffle_wr_bf.size(); i++) {
        std::cout << "sw_shuffle_wr_bf: " << (int)sw_shuffle_wr_bf[i] << std::endl;
    }
    for (size_t i = 0; i < write_out_cols.size(); i++) {
        std::cout << "write_out_cols: " << write_out_cols[i] << std::endl;
    }
    std::cout << "----------cfg setup done---------" << std::endl;
#endif

    // 5) setup kernel cfg, which includes bloomfilter setup, input_col_en, wr_col_en
    // input_col_en and wr_col_en are taken from sw_shuffle_scan/wr cfg
    SetupKernelConfig(build_probe, filter_a, filter_b, gen_rowID_en, valid_en, bf_keys, sw_shuffle_scan_bf,
                      sw_shuffle_wr_bf);
}

/**
 * @brief setup gqe kernel used config
 *
 * kernel config is consist of 3 parts:
 * - krn bloomfilter setup
 * - scan input col enable, get from sw_shuffle_scan
 * - write out col enable, get from sw_shuffle_wr
**/
void BloomFilterConfig::SetupKernelConfig(bool build_probe,
                                          std::string filter_a,
                                          std::string filter_b,
                                          bool gen_rowID_en[2],
                                          bool valid_en[2],
                                          std::vector<std::vector<std::string> > bf_keys,
                                          std::vector<std::vector<int8_t> > sw_shuffle_scan_bf,
                                          std::vector<int8_t> sw_shuffle_wr_bf) {
    using krncmdclass = xf::database::gqe::KernelCommand;
    krncmdclass krncmd = krncmdclass();

    krncmd.setScanColEnable(1, sw_shuffle_scan_bf[build_probe ? 1 : 0]); // XXX
    krncmd.setBloomfilterOn(1);
    krncmd.setBloomfilterSize(1 << 28);
    krncmd.setBloomfilterBuildProbe(build_probe);
    krncmd.setRowIDValidEnable(1, gen_rowID_en[build_probe ? 1 : 0], valid_en[build_probe ? 1 : 0]); // XXX
    if (build_probe) {
        krncmd.setWriteColEnable(2, 1, sw_shuffle_wr_bf); // XXX
    }
    krncmd.setFilter(1, build_probe ? filter_b : filter_a);

    table_bf_cfg = mm.aligned_alloc<ap_uint<512> >(14);
    ap_uint<512>* tmp_cfg = krncmd.getConfigBits();
    memcpy(table_bf_cfg, tmp_cfg, sizeof(ap_uint<512>) * 14);
}

ap_uint<512>* BloomFilterConfig::getBloomFilterConfigBits() const {
    return table_bf_cfg;
}

// get the sw scan-shuffle config which shuffles key col to the front
std::vector<std::vector<int8_t> > BloomFilterConfig::getShuffleScan() const {
    return sw_shuffle_scan_bf;
}

std::vector<int8_t> BloomFilterConfig::getShuffleWrite() const {
    return sw_shuffle_wr_bf;
}

} // database
} // gqe
} // xf
