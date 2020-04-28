/*
 * Copyright 2019 Xilinx, Inc.
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

#include "tpch_read_2.hpp"
#include "utils.hpp"

#include <cstdio>
#include <fstream>
// C++11 thread
#include <thread>

// ------------------------------------------------------------

void read_o(std::string o_path, std::vector<orders_t>& o_vec) {
    std::ifstream ifs;
    ifs.open(o_path, std::ios_base::in);
    if (!ifs) {
        printf("ERROR: %s cannot ben opened for read.\n", o_path.c_str());
        return;
    }
    // read data line
    while (ifs) {
        orders_t t;
        ifs >> t;
        if (ifs) {
            o_vec.push_back(t);
        }
    }
    ifs.close();
    printf("INFO: Loaded %s from disk.\n", o_path.c_str());
}

void read_l(std::string l_path, std::vector<lineitem_t>& l_vec) {
    std::ifstream ifs;
    ifs.open(l_path, std::ios_base::in);
    if (!ifs) {
        printf("ERROR: %s cannot ben opened for read.\n", l_path.c_str());
        return;
    }
    // read data line
    while (ifs) {
        lineitem_t t;
        ifs >> t;
        if (ifs) {
            l_vec.push_back(t);
        }
    }
    ifs.close();
    printf("INFO: Loaded %s from disk.\n", l_path.c_str());
}

void columnize_l(lineitem_t* buf_l,
                 size_t nrow, //
                 TPCH_INT* col_l_orderkey,
                 TPCH_INT* col_l_extendedprice,
                 TPCH_INT* col_l_discount) {
    for (size_t i = 0; i < nrow; ++i) {
        col_l_orderkey[i] = buf_l[i].orderkey;
        col_l_extendedprice[i] = buf_l[i].extendedprice;
        col_l_discount[i] = buf_l[i].discount;
    }
}

void columnize_o(orders_t* buf_o,
                 size_t nrow, //
                 TPCH_INT* col_o_orderkey,
                 TPCH_INT* col_o_orderdate) {
    for (size_t i = 0; i < nrow; ++i) {
        col_o_orderkey[i] = buf_o[i].orderkey;
        col_o_orderdate[i] = buf_o[i].orderdate;
    }
}

template <typename T>
int write_to_file(const std::string& fn, const std::vector<T>& d) {
    FILE* f = fopen(fn.c_str(), "wb");
    if (!f) {
        printf("ERROR: %s cannot be opened for write.\n", fn.c_str());
        return 1;
    }
    int n = fwrite(d.data(), sizeof(T), d.size(), f);
    fclose(f);
    return n;
}

int main(int argc, const char* argv[]) {
    // cmd arg parser.
    ArgParser parser(argc, argv);

    int err = 0;
    bool flag_l = false;
    std::string l_path;
    if (parser.getCmdOption("-l", l_path)) {
        if (!has_end(l_path, ".tbl") && !has_end(l_path, ".TBL")) {
            printf("WARNING: %s is not a .tbl, ignored!\n", l_path.c_str());
        } else {
            flag_l = true;
        }
    } else {
        printf("ERROR: -l <path/to/lineitem.tbl> is required!\n");
        ++err;
    }

    std::string l_odir = ".";
    parser.getCmdOption("-ldir", l_odir);
    if (!is_dir(l_odir)) {
        printf("ERROR: \"%s\" is not a directory!\n", l_odir.c_str());
        ++err;
    }

    bool flag_o = false;
    std::string o_path;
    if (parser.getCmdOption("-o", o_path)) {
        if (!has_end(o_path, ".tbl") && !has_end(o_path, ".TBL")) {
            printf("WARNING: %s is not a .tbl, ignored!\n", o_path.c_str());
        } else {
            flag_o = true;
        }
    } else {
        printf("ERROR: -o <path/to/orders.tbl> is required!\n");
        ++err;
    }

    std::string out_dir = ".";
    parser.getCmdOption("-d", out_dir);
    if (!is_dir(out_dir)) {
        printf("ERROR: \"%s\" is not a directory!\n", out_dir.c_str());
        ++err;
    }

    if (err) return err;

    // set up input data and buffers

    std::vector<orders_t> o_vec;
    std::vector<lineitem_t> l_vec;

    std::thread l_thread;
    std::thread o_thread;

    struct timeval tv0, tv1;
    int usec;

    gettimeofday(&tv0, 0);

    if (flag_o) o_thread = std::thread(read_o, o_path, std::ref(o_vec));
    if (flag_l) l_thread = std::thread(read_l, l_path, std::ref(l_vec));

    if (flag_o) o_thread.join();
    if (flag_l) l_thread.join();

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to load lineitem and orders table: %d usec.\n", usec);

    size_t o_nrow = o_vec.size();
    std::vector<TPCH_INT> col_o_orderkey(o_nrow);
    std::vector<TPCH_INT> col_o_orderdate(o_nrow);

    size_t l_nrow = l_vec.size();
    std::vector<TPCH_INT> col_l_orderkey(l_nrow);
    std::vector<TPCH_INT> col_l_extendedprice(l_nrow);
    std::vector<TPCH_INT> col_l_discount(l_nrow);

    // start timing and launch threads.
    // ************************************************************

    gettimeofday(&tv0, 0);

    if (flag_l) {
        l_thread = std::thread(columnize_l, l_vec.data(), l_nrow, col_l_orderkey.data(), col_l_extendedprice.data(),
                               col_l_discount.data());
    }
    if (flag_o) {
        o_thread = std::thread(columnize_o, o_vec.data(), o_nrow, col_o_orderkey.data(), col_o_orderdate.data());
    }

    if (flag_l) l_thread.join();
    if (flag_o) o_thread.join();

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to columnize lineitem and orders table: %d usec.\n", usec);

    // ************************************************************
    // done and write data to file.

    std::string fn;
    if (flag_l) {
        fn = out_dir + "/l_orderkey.dat";
        write_to_file(fn, col_l_orderkey);
        fn = out_dir + "/l_extendedprice.dat";
        write_to_file(fn, col_l_extendedprice);
        fn = out_dir + "/l_discount.dat";
        write_to_file(fn, col_l_discount);
    }

    if (flag_o) {
        fn = out_dir + "/o_orderkey.dat";
        write_to_file(fn, col_o_orderkey);
        fn = out_dir + "/o_orderdate.dat";
        write_to_file(fn, col_o_orderdate);
    }

    return 0;
}
