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
#include "xf_database/gqe_input.hpp"
#include "xf_database/gqe_utils.hpp"
#include "xf_database/gqe_workshop.hpp"
#include "xf_database/gqe_worker.hpp"
#include "xf_database/gqe_bloomfilter_config.hpp"
#include <ap_int.h>
#include "prepare.hpp"
#include "x_utils.hpp"
#include <unordered_map>
#include <vector>
#include <future>

#include "xf_utils_sw/logger.hpp"
using namespace std;

#define VEC_LEN 8
// 1 / BUILD_FACTOR of L table rows will be built into bloom-filter
#define BUILD_FACTOR 10

// load one col data into 1 buffer
template <typename T>
int load_dat(void* data, const std::string& name, const std::string& dir, const int sf, const size_t n) {
    if (!data) {
        return -1;
    }
    std::string fn = dir + "/dat" + std::to_string(sf) + "/" + name + ".dat";
    FILE* f = fopen(fn.c_str(), "rb");
    if (!f) {
        std::cerr << "ERROR: " << fn << " cannot be opened for binary read." << std::endl;
    }
    size_t cnt = fread(data, sizeof(T), n, f);
    fclose(f);
    if (cnt != n) {
        std::cerr << "ERROR: " << cnt << " entries read from " << fn << ", " << n << " entries required." << std::endl;
        return -1;
    }
    return 0;
}

void checkRes(int64_t* oCol, int64_t* lCol, int64_t* resCol, size_t& f_num, size_t count) {
    size_t bd = count / BUILD_FACTOR;
    size_t ptrO = 0;
    size_t ptrR = 0;
    for (ptrO = 0; ptrO < bd; ptrO++) {
        for (size_t j = ptrR; j < count; j++) {
            if (*(oCol + ptrO) != *(lCol + j)) {
                ptrR = j;
                break;
            } else {
                continue;
            }
        }
    }
    f_num = bd - ptrR;
}

int main(int argc, const char* argv[]) {
    std::cout << "--------------- Query 5 simplified, filter --------------- " << std::endl;

    using namespace xf::common::utils_sw;
    Logger logger(std::cout, std::cerr);

    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir)) {
        std::cout << "Please provide the path to the input tables\n";
        return 1;
    }

    std::string scale;
    int factor_o = 1;
    int factor_l = 1;
    if (parser.getCmdOption("-O", scale)) {
        try {
            factor_o = std::stoi(scale);
        } catch (...) {
            factor_o = 1;
        }
    }
    if (parser.getCmdOption("-L", scale)) {
        try {
            factor_l = std::stoi(scale);
        } catch (...) {
            factor_l = 1;
        }
    }

    std::string section;
    int sec_l = 1;
    if (parser.getCmdOption("-sec", section)) {
        try {
            sec_l = std::stoi(section);
        } catch (...) {
            sec_l = 1;
        }
    }

    std::vector<std::string> cols_rt;
    cols_rt.push_back("l_orderkey");
    cols_rt.push_back("l_extendedprice");
    cols_rt.push_back("l_discount");
    std::string in_dir_datr = prepare(in_dir, factor_l, cols_rt);
    std::cout << "Read right table form " << in_dir_datr << std::endl;

    int64_t table_o_nrow = 1500000 * factor_o;
    int64_t table_l_nrow = 6001215;
    switch (factor_l) {
        case 1:
            table_l_nrow = 6001215;
            break;
        case 2:
            table_l_nrow = 11997941;
            break;
        case 4:
            table_l_nrow = 23996604;
            break;
        case 8:
            table_l_nrow = 47989007;
            break;
        case 10:
            table_l_nrow = 59986052;
            break;
        case 12:
            table_l_nrow = 71985077;
            break;
        case 20:
            table_l_nrow = 119994608;
            break;
        case 30:
            table_l_nrow = 179998372;
            break;
        case 32:
            table_l_nrow = 192000000;
            break;
        case 33:
            table_l_nrow = 198000000;
            break;
        case 34:
            table_l_nrow = 204000000;
            break;
        case 35:
            table_l_nrow = 210000000;
            break;
        case 36:
            table_l_nrow = 216000000;
            break;
        case 37:
            table_l_nrow = 222000000;
            break;
        case 38:
            table_l_nrow = 228000000;
            break;
        case 39:
            table_l_nrow = 234000000;
            break;
        case 40:
            table_l_nrow = 240012290;
            break;
        case 60:
            table_l_nrow = 360011594;
            break;
        case 80:
            table_l_nrow = 480025129;
            break;
        case 100:
            table_l_nrow = 600037902;
            break;
        case 150:
            table_l_nrow = 900035147;
            break;
        case 200:
            table_l_nrow = 1200018434;
            break;
        case 250:
            table_l_nrow = 1500000714;
            break;
        default:
            table_l_nrow = 6001215;
            std::cerr << "L factor not supported, using SF1" << std::endl;
    }
    if (factor_l > 30 && factor_l < 40) {
        factor_l = 40;
    }

    int sim_scale = 10000;
    if (parser.getCmdOption("-scale", scale)) {
        try {
            sim_scale = std::stoi(scale);
        } catch (...) {
            sim_scale = 10000;
        }
    }

    table_o_nrow /= sim_scale;
    table_l_nrow /= sim_scale;

    std::cout << "Orders SF(" << factor_o << ")\t" << table_o_nrow << " rows\n"
              << "Lineitem SF(" << factor_l << ")\t" << table_l_nrow << " rows\n";

    using namespace xf::database;
    gqe::utils::MM mm;

    // 32-bit data load from tpch data
    int32_t* table_l_in_0 = mm.aligned_alloc<int32_t>(table_l_nrow);
    int32_t* table_l_in_1 = mm.aligned_alloc<int32_t>(table_l_nrow);

    // 64-bit data actually used in gqe-int64 kernel
    int64_t* tab_l_col0 = mm.aligned_alloc<int64_t>(table_l_nrow);
    int64_t* tab_l_col1 = mm.aligned_alloc<int64_t>(table_l_nrow);
    int64_t tab_l_val_len = (table_l_nrow + 7) / 8;
    char* tab_l_valid = mm.aligned_alloc<char>(tab_l_val_len);

    int64_t* tab_o_col0 = mm.aligned_alloc<int64_t>(table_l_nrow / BUILD_FACTOR);

    int64_t table_c_nrow = table_l_nrow;
    int64_t table_c_nrow_depth = (table_c_nrow + VEC_LEN - 1) / VEC_LEN;

    ap_uint<512>* tab_c_col0 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow_depth);
    memset(tab_c_col0, 0, table_c_nrow_depth * sizeof(ap_uint<512>));
    ap_uint<512>* tab_c_col1 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow_depth);
    memset(tab_c_col1, 0, table_c_nrow_depth * sizeof(ap_uint<512>));

    int err = 0;
    err += load_dat<int32_t>(table_l_in_0, "l_orderkey", in_dir, factor_l, table_l_nrow);
    err += load_dat<int32_t>(table_l_in_1, "l_extendedprice", in_dir, factor_l, table_l_nrow);
    if (err) return err;
    std::cout << "LineItem table has been read from disk" << std::endl;

    // diable even rows of table L
    for (int i = 0; i < tab_l_val_len; i++) {
        tab_l_valid[i] = 0x55;
    }

    // convert data from 32-bit to 64-bit, for testing only
    for (int i = 0; i < table_l_nrow; ++i) {
        tab_l_col0[i] = table_l_in_0[i];
        tab_l_col1[i] = table_l_in_1[i];
    }
    // build 0 - 1/BUILD_FACTOR of table L into bf1
    for (int i = 0; i < table_l_nrow / BUILD_FACTOR; i++) {
        tab_o_col0[i] = table_l_in_0[i];
    }

    gqe::TableSection tab_o("Table O", {"o_orderkey"}, {sizeof(int64_t)}, false, false, "o_rowid", "");
    gqe::TableSection tab_l("Table L", {"l_orderkey", "l_extendedprice"}, {sizeof(int64_t), sizeof(int64_t)}, false,
                            true, "l_rowid", "l_valid");
    gqe::TableSection tab_c("Table C", {"c1", "c2"}, {sizeof(int64_t), sizeof(int64_t)}, 0, 0, "", "");

    vector<future<size_t> > tab_o_ready(1);
    vector<future<size_t> > tab_l_ready(1);
    vector<promise<size_t> > tab_c_ready_promise(1);
    vector<future<size_t> > tab_c_ready(1);

    tab_o_ready[0] = async(&gqe::TableSection::addSec, &tab_o, vector<char*>({(char*)tab_o_col0}), nullptr,
                           table_l_nrow / BUILD_FACTOR);
    tab_l_ready[0] = async(&gqe::TableSection::addSec, &tab_l, vector<char*>({(char*)tab_l_col0, (char*)tab_l_col1}),
                           tab_l_valid, table_l_nrow);
    tab_c.addSec(vector<char*>({(char*)tab_c_col0, (char*)tab_c_col1}), nullptr, table_c_nrow);
    tab_c_ready[0] = tab_c_ready_promise[0].get_future();

    gqe::Workshop wksp("xilinx_u50_gen3x16_xdma_5_202210_1", xclbin_path, gqe::WorkerFunctions::JOIN);
    wksp.Bloomfilter(&tab_o, "", &tab_o_ready, &tab_l, "", &tab_l_ready, "o_orderkey=l_orderkey",
                     "c1=l_orderkey,c2=l_extendedprice", &tab_c, &tab_c_ready_promise);

    size_t res_rows = tab_c_ready[0].get();
    std::cout << "res_rows= " << res_rows << std::endl;
    // save filtered key/payload to for checking

    // test each added key in the filtered key list
    size_t nerror = 0;
    checkRes(tab_o_col0, tab_l_col0, (int64_t*)tab_c_col0, res_rows, nerror);
    wksp.release();
    nerror ? logger.error(Logger::Message::TEST_FAIL) : logger.info(Logger::Message::TEST_PASS);
    return nerror;
}
