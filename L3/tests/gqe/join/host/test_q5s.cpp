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
#include "xf_database/gqe_join.hpp"
#include "prepare.hpp"
#include "x_utils.hpp"
#include <unordered_map>

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
int64_t get_golden_sum(int l_row,
                       int32_t* col_l_orderkey,
                       int32_t* col_l_extendedprice,
                       int32_t* col_l_discount,
                       int o_row,
                       int32_t* col_o_orderkey,
                       int32_t* col_o_orderdate) {
    ap_uint<64> sum = 0;
    int cnt = 0;

    std::unordered_multimap<uint32_t, uint32_t> ht1;

    {
        for (int i = 0; i < o_row; ++i) {
            uint32_t k = col_o_orderkey[i];
            uint32_t date = col_o_orderdate[i];
            if (date >= 19940101 && date < 19950101) ht1.insert(std::make_pair(k, date));
        }
    }
    // read t once
    for (int i = 0; i < l_row; ++i) {
        uint32_t k = col_l_orderkey[i];
        uint32_t p = col_l_extendedprice[i];
        uint32_t d = col_l_discount[i];
        // check hash table
        auto its = ht1.equal_range(k);
        for (auto it = its.first; it != its.second; ++it) {
            // std::cout << p << ", " << d << std::endl;
            sum += (p * (100 - d));
            ++cnt;
        }
    }

    std::cout << "INFO: CPU ref matched " << cnt << " rows, sum = " << sum << std::endl;
    return sum;
}

int main(int argc, const char* argv[]) {
    std::cout << "--------------- Query 5 simplified, join --------------- " << std::endl;

    // cmd arg parser.
    x_utils::ArgParser parser(argc, argv);

    std::string xclbin_path;
    if (!parser.getCmdOption("-xclbin", xclbin_path)) {
        std::cout << "ERROR: xclbin path is not set!\n";
        return 1;
    }
    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir)) {
        in_dir = "db_data/";
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
    int sim_scale = 1;
    if (parser.getCmdOption("-scale", scale)) {
        try {
            sim_scale = std::stoi(scale);
        } catch (...) {
            sim_scale = 10000;
        }
    }
    std::string validate = "on";
    parser.getCmdOption("-validate", validate);
    if (validate != "on" && validate != "off") validate = "on";

    std::vector<std::string> cols_lt;
    cols_lt.push_back("o_orderkey");
    cols_lt.push_back("o_orderdate");
    std::vector<std::string> cols_rt;
    cols_rt.push_back("l_orderkey");
    cols_rt.push_back("l_extendedprice");
    cols_rt.push_back("l_discount");
    std::string in_dir_datl = prepare(in_dir, factor_o, cols_lt);
    std::string in_dir_datr = prepare(in_dir, factor_l, cols_rt);
    std::cout << "Read left table form " << in_dir_datl << std::endl;
    std::cout << "Read right table form " << in_dir_datr << std::endl;

    std::string mode = "v1";
    size_t solution = 2;
    size_t sec_o = 2; // set 0, use user's input section
    size_t sec_l = 2; // set 0, use user's input section
    size_t slice_num = 2;
    size_t log_part = 2;
    parser.getCmdOption("-mode", mode);
    if (mode == "manual") {
        std::cout << "Using StrategyManualSet" << std::endl;

        if (parser.getCmdOption("-solution", scale)) {
            try {
                solution = std::stoi(scale);
            } catch (...) {
                solution = 2;
            }
        }
        std::cout << "Select solution:" << solution << std::endl;

        if (solution > 2) {
            std::cout << "No supported Strategy" << std::endl;
            return 1;
        }

        if (parser.getCmdOption("-sec_o", scale)) {
            try {
                sec_o = std::stoi(scale);
            } catch (...) {
                sec_o = 1;
            }
        }
        if (parser.getCmdOption("-sec_l", scale)) {
            try {
                sec_l = std::stoi(scale);
            } catch (...) {
                sec_l = 1;
            }
        }
        if (parser.getCmdOption("-log_part", scale)) {
            try {
                log_part = std::stoi(scale);
            } catch (...) {
                log_part = 2;
            }
        }
        if (solution == 2 && log_part < 2) {
            std::cout << "ERROR: partition number only supports >= 4 for now!!" << std::endl;
            return -1;
        }
        if (parser.getCmdOption("-slice_num", scale)) {
            try {
                slice_num = std::stoi(scale);
            } catch (...) {
                slice_num = 1;
            }
        }
    } else if (mode == "v1") {
        std::cout << "Using StrategyV1" << std::endl;
    } else {
        std::cout << "No supported Strategy" << std::endl;
        return 1;
    }
    int32_t table_o_nrow = 1500000 * factor_o;
    int32_t table_l_nrow = 6001215;
    switch (factor_l) {
        case 1:
            table_l_nrow = 6001215;
            break;
        case 2:
            table_l_nrow = 11997996;
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
    table_o_nrow /= sim_scale;
    table_l_nrow /= sim_scale;

    std::cout << "Orders SF(" << factor_o << ")\t" << table_o_nrow << " rows\n"
              << "Lineitem SF(" << factor_l << ")\t" << table_l_nrow << " rows\n";

    using namespace xf::database;
    gqe::utils::MM mm;
    // set the proper size of output table nrow by different SF
    int32_t table_c_nrow = 1 << 25;

    int32_t* tab_o_col0 = mm.aligned_alloc<int32_t>(table_o_nrow);
    int32_t* tab_o_col1 = mm.aligned_alloc<int32_t>(table_o_nrow);

    int32_t* tab_l_col0 = mm.aligned_alloc<int32_t>(table_l_nrow);
    int32_t* tab_l_col1 = mm.aligned_alloc<int32_t>(table_l_nrow);
    int32_t* tab_l_col2 = mm.aligned_alloc<int32_t>(table_l_nrow);

    ap_uint<512>* tab_c_col0 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow);
    ap_uint<512>* tab_c_col1 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow);
    ap_uint<512>* tab_c_col2 = mm.aligned_alloc<ap_uint<512> >(table_c_nrow);

    int err = 0;

    err += load_dat<int32_t>(tab_o_col0, "o_orderkey", in_dir, factor_o, table_o_nrow);
    err += load_dat<int32_t>(tab_o_col1, "o_orderdate", in_dir, factor_o, table_o_nrow);
    if (err) return err;
    std::cout << "Orders table has been read from disk" << std::endl;

    err += load_dat<int32_t>(tab_l_col0, "l_orderkey", in_dir, factor_l, table_l_nrow);
    err += load_dat<int32_t>(tab_l_col1, "l_extendedprice", in_dir, factor_l, table_l_nrow);
    err += load_dat<int32_t>(tab_l_col2, "l_discount", in_dir, factor_l, table_l_nrow);
    if (err) return err;
    std::cout << "LineItem table has been read from disk" << std::endl;

    gqe::Table tab_o;
    tab_o.addCol("o_orderdate", gqe::TypeEnum::TypeInt32, tab_o_col1, table_o_nrow);
    tab_o.addCol("o_orderkey", gqe::TypeEnum::TypeInt32, tab_o_col0, table_o_nrow);

    gqe::Table tab_l;
    tab_l.addCol("l_extendedprice", gqe::TypeEnum::TypeInt32, tab_l_col1, table_l_nrow);
    tab_l.addCol("l_discount", gqe::TypeEnum::TypeInt32, tab_l_col2, table_l_nrow);
    tab_l.addCol("l_orderkey", gqe::TypeEnum::TypeInt32, tab_l_col0, table_l_nrow);

    gqe::Table tab_c;
    tab_c.addCol("c1", gqe::TypeEnum::TypeInt32, tab_c_col0, table_c_nrow);
    tab_c.addCol("c2", gqe::TypeEnum::TypeInt32, tab_c_col1, table_c_nrow);
    tab_c.addCol("c3", gqe::TypeEnum::TypeInt32, tab_c_col2, table_c_nrow);

    tab_o.info();
    tab_l.info();

    // constructor
    gqe::Joiner bigjoin(xclbin_path);

    gqe::ErrCode err_code;
    if (mode == "v1") {
        // use JoinStrategyV1
        auto sv1 = new gqe::JoinStrategyV1();
        err_code =
            bigjoin.join(tab_o, "19940101<=o_orderdate && o_orderdate<19950101", tab_l, "", "o_orderkey = l_orderkey",
                         tab_c, "c1=l_extendedprice, c2=l_discount, c3=l_orderkey", gqe::INNER_JOIN, sv1);
        if (!err_code) {
            delete sv1;
            std::cout << "Join done" << std::endl;
        } else {
            return err_code;
        }
    } else if (mode == "manual") {
        // use JoinStrategyManualSet
        auto smanual = new gqe::JoinStrategyManualSet(solution, sec_o, sec_l, slice_num, log_part);
        err_code =
            bigjoin.join(tab_o, "19940101<=o_orderdate && o_orderdate<19950101", tab_l, "", "o_orderkey = l_orderkey",
                         tab_c, "c1=l_extendedprice, c2=l_discount, c3=l_orderkey", gqe::INNER_JOIN, smanual);

        if (!err_code) {
            delete smanual;
        } else {
            return err_code;
        }
    }
    std::cout << "Aggregate in CPU" << std::endl;
    // calculate the aggr results:
    // sum(l_extendedprice * (1-discount))
    // col0:extendedprice*100 col1: discount*100 col2: orderdate col3: keyid
    long long sum = 0;
    int p_nrow = tab_c.getRowNum();
    std::cout << "Join result rows: " << p_nrow << std::endl;
    for (int n = 0; n < p_nrow / 16; n++) {
        for (int i = 0; i < 16; i++) {
            uint32_t extendedprice = tab_c_col0[n](31 + 32 * i, 32 * i);
            // uint32_t extendedprice = (tab_c.getColPointer(0))[n](31 + 32 * i, 32 * i);
            uint32_t discount = tab_c_col1[n](31 + 32 * i, 32 * i);
            // uint32_t discount = (tab_c.getColPointer(1))[n](31 + 32 * i, 32 * i);
            sum += extendedprice * (100 - discount);
        }
    }
    for (int n = 0; n < p_nrow % 16; n++) {
        uint32_t extendedprice = tab_c_col0[p_nrow / 16](31 + 32 * n, 32 * n);
        uint32_t discount = tab_c_col1[p_nrow / 16](31 + 32 * n, 32 * n);
        sum += extendedprice * (100 - discount);
    }
    std::cout << "Q5S Join done, result: " << sum << std::endl;

    if (validate == "on") {
        std::cout << "---------------------------------Checking result---------------------------------" << std::endl;
        long long golden =
            get_golden_sum(table_l_nrow, tab_l_col0, tab_l_col1, tab_l_col2, table_o_nrow, tab_o_col0, tab_o_col1);
        std::cout << "Golen value: " << golden << ", FPGA value: " << sum << std::endl;
        if (sum == golden) {
            std::cout << "Validate Pass" << std::endl;
        } else {
            std::cout << "Validate Failed" << std::endl;
            return 1;
        }
    }
    return 0;
}
