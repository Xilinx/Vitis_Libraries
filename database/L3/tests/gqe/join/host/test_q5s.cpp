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
#include "xf_database/gqe_input.hpp"
#include "xf_database/gqe_utils.hpp"
#include "xf_database/gqe_join_strategy.hpp"
#include "xf_database/gqe_join_config.hpp"
#include "xf_database/gqe_partjoin_config.hpp"
#include "xf_database/gqe_workshop.hpp"
#include <ap_int.h>
#include "prepare.hpp"
#include "x_utils.hpp"
#include <unordered_map>
#include <chrono>
#include <thread>

#include "xf_utils_sw/logger.hpp"

using namespace std;

#define VAL(s) #s
#define STRING(s) VAL(s)

#define VEC_LEN 8
#define USER_DEBUG 1

// load one col data into 1 buffer
template <typename T>
int load_dat(void* data, const string& name, const string& dir, const int sf, const size_t n) {
    if (!data) {
        return -1;
    }
    string fn = dir + "/dat" + to_string(sf) + "/" + name + ".dat";
    FILE* f = fopen(fn.c_str(), "rb");
    if (!f) {
        cerr << "ERROR: " << fn << " cannot be opened for binary read." << endl;
    }
    size_t cnt = fread(data, sizeof(T), n, f);
    fclose(f);
    if (cnt != n) {
        cerr << "ERROR: " << cnt << " entries read from " << fn << ", " << n << " entries required." << endl;
        return -1;
    }
    return 0;
}

struct golden_data {
    int64_t nrow;
    int64_t sum;
};
golden_data get_golden_sum(int o_row,
                           int64_t* col_o_orderkey,
                           int64_t* col_o_rowid,
                           bool valid_o,
                           char* tab_o_valid,
                           int l_row,
                           int64_t* col_l_orderkey,
                           int64_t* col_l_rowid,
                           bool valid_l,
                           char* tab_l_valid) {
    int64_t sum = 0;
    int64_t cnt = 0;

    unordered_multimap<uint64_t, uint64_t> ht1;

    for (int i = 0; i < o_row; ++i) {
        char val_8 = tab_o_valid[i / 8];
        bool valid = (val_8 >> (i % 8)) & 0x1;
        // valid_o ==0: Table O not using validation buffer
        if (valid_o == 0) valid = 1;
        if (valid) {
            int64_t k = col_o_orderkey[i];
            ht1.insert(make_pair(k, i + 1));
        }
    }

    // read t once
    for (int i = 0; i < l_row; ++i) {
        int64_t k = col_l_orderkey[i];
        // check hash table
        auto its = ht1.equal_range(k);
        for (auto it = its.first; it != its.second; ++it) {
            int64_t sum_i = (k * k) % 10000;
            sum += sum_i;
            cnt++;
        }
    }

    golden_data result;

    result.sum = sum;
    result.nrow = cnt;
    return result;
}

int main(int argc, const char* argv[]) {
    cout << "--------------- Query 5 simplified, join --------------- " << endl;

    using namespace xf::common::utils_sw;
    Logger logger(cout, cerr);

    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

    string xclbin_path;
#ifndef HLS_TEST
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }
#endif
    string in_dir;
    if (!parser.getCmdOption("-in", in_dir)) {
        in_dir = "db_data/";
    }

    string scale;
    int sim_scale = 1;
    if (parser.getCmdOption("-scale", scale)) {
        sim_scale = std::stoi(scale);
    } else
        sim_scale = 1000;
    int factor_o = 1;
    int factor_l = 1;
    string validate = "on";

    vector<string> cols_lt{"o_orderkey", "o_orderdate"};
    vector<string> cols_rt{"l_orderkey"};
    string in_dir_datl = prepare(in_dir, factor_o, cols_lt);
    string in_dir_datr = prepare(in_dir, factor_l, cols_rt);
    cout << "Read left table form " << in_dir_datl << endl;
    cout << "Read right table form " << in_dir_datr << endl;

    // XXX dat files are still 32bit.
    generate_valid(in_dir, factor_o, "o_valid", "o_orderdate", 32, 19940101UL, 19950101UL);

    size_t solution = 1;
    size_t sec_o = 1;
    size_t sec_l = 1;
    size_t sec_c = sec_l;
    size_t slice_num = 1;
    size_t log_part = 3;
    size_t part_num = 1 << log_part;

    if (solution == 1) {
        sec_c = sec_l;
    } else if (solution == 2) {
        sec_c = part_num;
    } else {
        cout << "unspported solution" << endl;
    }

    // the coefficiency of partO output buffer expansion
    float coef_exp_partO = 2;
    // the coefficiency of partL output buffer expansion
    float coef_exp_partL = 2;
    // the coefficiency of join output buffer expansion
    float coef_exp_join = 2;

    string mode = "manual";
    parser.getCmdOption("-mode", mode);

    int64_t table_o_nrow = 1500000 * factor_o;
    int64_t table_l_nrow = 6001215;

    table_o_nrow /= sim_scale;
    table_l_nrow /= sim_scale;

    cout << "Orders SF(" << factor_o << ")\t" << table_o_nrow << " rows\n"
         << "Lineitem SF(" << factor_l << ")\t" << table_l_nrow << " rows\n";

    using namespace xf::database;
    gqe::utils::MM mm;

    // 32-bit data load from tpch data
    int32_t* table_o_in_0 = mm.aligned_alloc<int32_t>(table_o_nrow);
    int32_t* table_o_in_1 = mm.aligned_alloc<int32_t>(table_o_nrow);

    // 64-bit data actually used in gqe-int64 kernel
    int64_t* tab_o_col0 = mm.aligned_alloc<int64_t>(table_o_nrow);
    int64_t* tab_o_col1 = mm.aligned_alloc<int64_t>(table_o_nrow);

    // 32-bit data load from tpch data

    int32_t* table_l_in_0 = mm.aligned_alloc<int32_t>(table_l_nrow);
    int32_t* table_l_in_1 = mm.aligned_alloc<int32_t>(table_l_nrow);

    // 64-bit data actually used in gqe-int64 kernel
    int64_t* tab_l_col0 = mm.aligned_alloc<int64_t>(table_l_nrow);
    int64_t* tab_l_col1 = mm.aligned_alloc<int64_t>(table_l_nrow);

    // define the validation buffer
    int64_t tab_o_val_len = (table_o_nrow + 7) / 8;
    char* tab_o_valid = mm.aligned_alloc<char>(tab_o_val_len);

    int64_t tab_l_val_len = (table_l_nrow + 7) / 8;
    char* tab_l_valid = mm.aligned_alloc<char>(tab_l_val_len);
    for (int i = 0; i < tab_l_val_len; i++) {
        tab_l_valid[i] = 0xff;
    }

    int64_t table_c_nrow = coef_exp_join * (float)table_l_nrow;
    int64_t table_c_nrow_depth = (table_c_nrow + VEC_LEN - 1) / VEC_LEN;

    ap_uint<512>* tab_c_col0 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow_depth);
    ap_uint<512>* tab_c_col1 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow_depth);
    ap_uint<512>* tab_c_col2 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow_depth);
    // ap_uint<512>* tab_c_col3 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow_depth);

    int err = 0;

    err += load_dat<int32_t>(table_o_in_0, "o_orderkey", in_dir, factor_o, table_o_nrow);
    err += load_dat<char>(tab_o_valid, "o_valid", in_dir, factor_o, tab_o_val_len);
    if (err) return err;
    cout << "Orders table has been read from disk" << endl;

    // convert data from 32-bit to 64-bit, for testing only
    for (int i = 0; i < table_o_nrow; ++i) {
        tab_o_col0[i] = table_o_in_0[i];
        tab_o_col1[i] = table_o_in_1[i];
    }

    err += load_dat<int32_t>(table_l_in_0, "l_orderkey", in_dir, factor_l, table_l_nrow);
    // err += load_dat<int32_t>(table_l_in_1, "l_orderkey", in_dir, factor_l, table_l_nrow);
    if (err) return err;
    cout << "LineItem table has been read from disk" << endl;

    // convert data from 32-bit to 64-bit, for testing only
    for (int i = 0; i < table_l_nrow; ++i) {
        tab_l_col0[i] = table_l_in_0[i];
        tab_l_col1[i] = table_l_in_1[i];
    }

    bool valid_o = 1;
    bool valid_l = 0;

    vector<future<size_t> > tab_o_ready;
    vector<future<size_t> > tab_l_ready;
    vector<promise<size_t> > tab_c_ready_promise;
    vector<future<size_t> > tab_c_ready;
    tab_o_ready.resize(sec_o);
    tab_l_ready.resize(sec_l);
    tab_c_ready_promise.resize(sec_c);
    tab_c_ready.resize(sec_c);
    for (size_t i = 0; i < sec_c; i++) {
        tab_c_ready[i] = tab_c_ready_promise[i].get_future();
    }

    gqe::TableSection tab_o("Table O", {"o_orderkey"}, {sizeof(int64_t)}, 1, valid_o, "o_rowid", "o_valid");
    gqe::TableSection tab_l("Table L", {"l_orderkey"}, {sizeof(int64_t)}, 1, valid_l, "l_rowid", "l_valid");
    gqe::TableSection tab_c("Table C", {"c1", "c2", "c3"}, {sizeof(int64_t), sizeof(int64_t), sizeof(int64_t)}, 0, 0,
                            "", "");

    for (size_t i = 0; i < sec_o; i++) {
        tab_o_ready[i] =
            async(&gqe::TableSection::addSec, &tab_o, vector<char*>({(char*)tab_o_col0}), tab_o_valid, table_o_nrow);
    }
    for (size_t i = 0; i < sec_l; i++) {
        size_t d_row = (table_l_nrow + (8 * sec_l) - 1) / (8 * sec_l) * 8;
        size_t d_bias = d_row * 8 * i;
        size_t v_bias = d_row / 8;
        size_t d_sec_nrow;
        if (i != (sec_l - 1)) {
            d_sec_nrow = d_row;
        } else {
            d_sec_nrow = table_l_nrow - i * d_row;
        }
        tab_l_ready[i] = async(&gqe::TableSection::addSec, &tab_l, vector<char*>({((char*)tab_l_col0) + d_bias}),
                               tab_l_valid + v_bias, d_sec_nrow);
    }
    for (size_t i = 0; i < sec_c; i++) {
        size_t d_row = (table_c_nrow + (8 * sec_c) - 1) / (8 * sec_c) * 8;
        size_t d_bias = d_row * 8 * i;
        size_t d_sec_nrow;
        if (i != (sec_c - 1)) {
            d_sec_nrow = d_row;
        } else {
            d_sec_nrow = table_c_nrow - i * d_row;
        }
        tab_c.addSec({((char*)tab_c_col0) + d_bias, ((char*)tab_c_col1) + d_bias, ((char*)tab_c_col2) + d_bias},
                     nullptr, d_sec_nrow);
    }

    gqe::TableSection tab_part_o("Part O", {"o1", "o2", "o3"}, {sizeof(int64_t), sizeof(int64_t), sizeof(int64_t)}, 0,
                                 0, "", "");
    gqe::TableSection tab_part_l("Part L", {"l1", "l2", "l3"}, {sizeof(int64_t), sizeof(int64_t), sizeof(int64_t)}, 0,
                                 0, "", "");

    size_t table_part_o_nrow = table_o_nrow * coef_exp_partO;
    size_t table_part_l_nrow = table_l_nrow * coef_exp_partL;
    for (size_t i = 0; i < part_num; i++) {
        size_t d_part_o_nrow = (table_part_o_nrow + part_num - 1) / part_num;
        int64_t* tmp_o1 = mm.aligned_alloc<int64_t>(d_part_o_nrow);
        int64_t* tmp_o2 = mm.aligned_alloc<int64_t>(d_part_o_nrow);
        int64_t* tmp_o3 = mm.aligned_alloc<int64_t>(d_part_o_nrow);
        tab_part_o.addSec({((char*)tmp_o1), ((char*)tmp_o2), ((char*)tmp_o3)}, nullptr, d_part_o_nrow);

        size_t d_part_l_nrow = (table_part_l_nrow + part_num - 1) / part_num;
        int64_t* tmp_l1 = mm.aligned_alloc<int64_t>(d_part_l_nrow);
        int64_t* tmp_l2 = mm.aligned_alloc<int64_t>(d_part_l_nrow);
        int64_t* tmp_l3 = mm.aligned_alloc<int64_t>(d_part_l_nrow);
        tab_part_l.addSec({((char*)tmp_l1), ((char*)tmp_l2), ((char*)tmp_l3)}, nullptr, d_part_l_nrow);
    }

    std::string shell_name(STRING(XDEVICE));
    gqe::Workshop wksp(shell_name, xclbin_path, gqe::WorkerFunctions::JOIN);

    auto smanual = new gqe::JoinStrategyManualSet(solution, sec_o, sec_l, slice_num, log_part, coef_exp_partO,
                                                  coef_exp_partL, coef_exp_join);

    wksp.Join(&tab_o, "o_rowid>0", &tab_o_ready, &tab_part_o, &tab_l, "", &tab_l_ready, &tab_part_l,
              "o_orderkey = l_orderkey", "c1=l_orderkey,c2=o_rowid,c3=l_rowid", &tab_c, &tab_c_ready_promise,
              gqe::INNER_JOIN, smanual);

    cout << "Aggregate in CPU" << endl;
    // calculate the aggr results: sum(l_extendedprice + orderkey)
    // col0: l_extendedprice; col1: l_orderkey
    int64_t sum = 0;
    int p_nrow = 0;

    for (size_t i = 0; i < sec_c; i++) {
        size_t res_rows = tab_c_ready[i].get();
        p_nrow += res_rows;
        cout << "GQE Join returns: " << res_rows << " rows in section[" << i << "]" << endl;
        int64_t* key = (int64_t*)tab_c.getColPointer(i, 0);
        for (size_t j = 0; j < res_rows; j++) {
            sum += key[j] * key[j] % 10000;
        }
    }
    cout << "GQE Join returns: " << p_nrow << " rows in total" << endl;

    int ret = 0;
    if (validate == "on") {
        cout << "---------------------------------Checking result---------------------------------" << endl;
        golden_data golden;
        golden = get_golden_sum(table_o_nrow, tab_o_col0, tab_o_col1, valid_o, tab_o_valid, table_l_nrow, tab_l_col0,
                                tab_l_col1, valid_l, tab_l_valid);
        cout << "Golen: rows: " << golden.nrow << ", value: " << golden.sum << endl;

        ret = (golden.sum == sum) ? 0 : 1;
        ret ? logger.error(Logger::Message::TEST_FAIL) : logger.info(Logger::Message::TEST_PASS);
    }

    // return ret;
    wksp.release();
    return 0;
}
