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
#include <cstring>

// ------------------------------------------------------------

template <typename T>
void read_tbl(std::string _path, std::vector<T>& _vec) {
    std::ifstream ifs;
    ifs.open(_path, std::ios_base::in);
    if (!ifs) {
        printf("ERROR: %s cannot ben opened for read.\n", _path.c_str());
        return;
    }
    // read data line
    while (ifs) {
        T t;
        ifs >> t;
        if (ifs) {
            _vec.push_back(t);
        }
    }
    ifs.close();
    printf("INFO: Loaded %s from disk.\n", _path.c_str());
}

// ------------------------------------------------------------

void columnize_r(region_t* buf_r,
                 size_t nrow, //
                 TPCH_INT* col_r_regionkey,
                 std::vector<std::array<char, TPCH_READ_REGION_LEN + 1> >& col_r_name) {
    for (size_t i = 0; i < nrow; ++i) {
        col_r_regionkey[i] = buf_r[i].regionkey;
        memcpy(col_r_name[i].data(), buf_r[i].name.data, TPCH_READ_REGION_LEN + 1);
    }
}

void columnize_n(nation_t* buf_n,
                 size_t nrow, //
                 TPCH_INT* col_n_nationkey,
                 TPCH_INT* col_n_regionkey,
                 std::vector<std::array<char, TPCH_READ_NATION_LEN + 1> >& col_n_name) {
    for (size_t i = 0; i < nrow; ++i) {
        col_n_nationkey[i] = buf_n[i].nationkey;
        col_n_regionkey[i] = buf_n[i].regionkey;
        memcpy(col_n_name[i].data(), buf_n[i].name.data, TPCH_READ_NATION_LEN + 1);
    }
}

void columnize_c(customer_t* buf_c,
                 size_t nrow, //
                 TPCH_INT* col_c_custkey,
                 TPCH_INT* col_c_nationkey) {
    for (size_t i = 0; i < nrow; ++i) {
        col_c_custkey[i] = buf_c[i].custkey;
        col_c_nationkey[i] = buf_c[i].nationkey;
    }
}

void columnize_o(orders_t* buf_o,
                 size_t nrow, //
                 TPCH_INT* col_o_orderkey,
                 TPCH_INT* col_o_custkey,
                 TPCH_INT* col_o_orderdate) {
    for (size_t i = 0; i < nrow; ++i) {
        col_o_orderkey[i] = buf_o[i].orderkey;
        col_o_custkey[i] = buf_o[i].custkey;
        col_o_orderdate[i] = buf_o[i].orderdate;
    }
}

void columnize_l(lineitem_t* buf_l,
                 size_t nrow, //
                 TPCH_INT* col_l_orderkey,
                 TPCH_INT* col_l_suppkey,
                 TPCH_INT* col_l_extendedprice,
                 TPCH_INT* col_l_discount) {
    for (size_t i = 0; i < nrow; ++i) {
        col_l_orderkey[i] = buf_l[i].orderkey;
        col_l_suppkey[i] = buf_l[i].suppkey;
        col_l_extendedprice[i] = buf_l[i].extendedprice;
        col_l_discount[i] = buf_l[i].discount;
    }
}

void columnize_s(supplier_t* buf_s,
                 size_t nrow, //
                 TPCH_INT* col_s_suppkey,
                 TPCH_INT* col_s_nationkey) {
    for (size_t i = 0; i < nrow; ++i) {
        col_s_suppkey[i] = buf_s[i].suppkey;
        col_s_nationkey[i] = buf_s[i].nationkey;
    }
}

// ------------------------------------------------------------

// template <typename T, std::enable_if<std::is_integral<T>::value, int>::type = 0>
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

template <int N>
int write_to_file(const std::string& fn, const std::vector<std::array<char, N> >& d) {
    FILE* f = fopen(fn.c_str(), "wb");
    if (!f) {
        printf("ERROR: %s cannot be opened for write.\n", fn.c_str());
        return 1;
    }
    int n = 0;
    for (int i = 0; i < d.size(); ++i) {
        n += fwrite(d[i].data(), N, 1, f);
    }
    fclose(f);
    return n;
}

int main(int argc, const char* argv[]) {
    // cmd arg parser.
    ArgParser parser(argc, argv);

    int err = 0;

    std::string in_dir = ".";
    parser.getCmdOption("-in", in_dir);
    if (!is_dir(in_dir)) {
        printf("ERROR: \"%s\" is not a directory!\n", in_dir.c_str());
        ++err;
    }

    std::string r_path = in_dir + "/region.tbl";
    if (!is_file(r_path)) {
        printf("ERROR: \"%s\" is not a file!\n", r_path.c_str());
        ++err;
    }
    std::string n_path = in_dir + "/nation.tbl";
    if (!is_file(n_path)) {
        printf("ERROR: \"%s\" is not a file!\n", n_path.c_str());
        ++err;
    }
    std::string c_path = in_dir + "/customer.tbl";
    if (!is_file(c_path)) {
        printf("ERROR: \"%s\" is not a file!\n", c_path.c_str());
        ++err;
    }
    std::string o_path = in_dir + "/orders.tbl";
    if (!is_file(o_path)) {
        printf("ERROR: \"%s\" is not a file!\n", o_path.c_str());
        ++err;
    }
    std::string l_path = in_dir + "/lineitem.tbl";
    if (!is_file(l_path)) {
        printf("ERROR: \"%s\" is not a file!\n", l_path.c_str());
        ++err;
    }
    std::string s_path = in_dir + "/supplier.tbl";
    if (!is_file(s_path)) {
        printf("ERROR: \"%s\" is not a file!\n", s_path.c_str());
        ++err;
    }

    std::string out_dir = ".";
    parser.getCmdOption("-out", out_dir);
    if (!is_dir(out_dir)) {
        printf("ERROR: \"%s\" is not a directory!\n", out_dir.c_str());
        ++err;
    }

    if (err) return err;

    // set up input data and buffers

    std::vector<region_t> r_vec;
    std::vector<nation_t> n_vec;
    std::vector<customer_t> c_vec;
    std::vector<orders_t> o_vec;
    std::vector<lineitem_t> l_vec;
    std::vector<supplier_t> s_vec;

    std::thread r_thread;
    std::thread n_thread;
    std::thread c_thread;
    std::thread o_thread;
    std::thread l_thread;
    std::thread s_thread;

    struct timeval tv0, tv1;
    int usec;

    gettimeofday(&tv0, 0);

    r_thread = std::thread(read_tbl<region_t>, r_path, std::ref(r_vec));
    n_thread = std::thread(read_tbl<nation_t>, n_path, std::ref(n_vec));
    c_thread = std::thread(read_tbl<customer_t>, c_path, std::ref(c_vec));
    o_thread = std::thread(read_tbl<orders_t>, o_path, std::ref(o_vec));
    l_thread = std::thread(read_tbl<lineitem_t>, l_path, std::ref(l_vec));
    s_thread = std::thread(read_tbl<supplier_t>, s_path, std::ref(s_vec));

    r_thread.join();
    n_thread.join();
    c_thread.join();
    o_thread.join();
    l_thread.join();
    s_thread.join();

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to load table: %d usec.\n", usec);

    size_t r_nrow = r_vec.size();
    std::vector<TPCH_INT> col_r_regionkey(r_nrow);
    std::vector<std::array<char, TPCH_READ_REGION_LEN + 1> > col_r_name(r_nrow);

    size_t n_nrow = n_vec.size();
    std::vector<TPCH_INT> col_n_nationkey(n_nrow);
    std::vector<TPCH_INT> col_n_regionkey(n_nrow);
    std::vector<std::array<char, TPCH_READ_NATION_LEN + 1> > col_n_name(n_nrow);

    size_t c_nrow = c_vec.size();
    std::vector<TPCH_INT> col_c_custkey(c_nrow);
    std::vector<TPCH_INT> col_c_nationkey(c_nrow);

    size_t o_nrow = o_vec.size();
    std::vector<TPCH_INT> col_o_orderkey(o_nrow);
    std::vector<TPCH_INT> col_o_custkey(o_nrow);
    std::vector<TPCH_INT> col_o_orderdate(o_nrow);

    size_t l_nrow = l_vec.size();
    std::vector<TPCH_INT> col_l_orderkey(l_nrow);
    std::vector<TPCH_INT> col_l_suppkey(l_nrow);
    std::vector<TPCH_INT> col_l_extendedprice(l_nrow);
    std::vector<TPCH_INT> col_l_discount(l_nrow);

    size_t s_nrow = s_vec.size();
    std::vector<TPCH_INT> col_s_suppkey(s_nrow);
    std::vector<TPCH_INT> col_s_nationkey(s_nrow);

    // start timing and launch threads.
    // ************************************************************

    gettimeofday(&tv0, 0);

    r_thread = std::thread(columnize_r, r_vec.data(), r_nrow, col_r_regionkey.data(), std::ref(col_r_name));

    n_thread = std::thread(columnize_n, n_vec.data(), n_nrow, col_n_nationkey.data(), col_n_regionkey.data(),
                           std::ref(col_n_name));

    c_thread = std::thread(columnize_c, c_vec.data(), c_nrow, col_c_custkey.data(), col_c_nationkey.data());

    o_thread = std::thread(columnize_o, o_vec.data(), o_nrow, col_o_orderkey.data(), col_o_custkey.data(),
                           col_o_orderdate.data());

    l_thread = std::thread(columnize_l, l_vec.data(), l_nrow, col_l_orderkey.data(), col_l_suppkey.data(),
                           col_l_extendedprice.data(), col_l_discount.data());

    s_thread = std::thread(columnize_s, s_vec.data(), s_nrow, col_s_suppkey.data(), col_s_nationkey.data());

    r_thread.join();
    n_thread.join();
    c_thread.join();
    o_thread.join();
    l_thread.join();
    s_thread.join();

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to columnize tables: %d usec.\n", usec);

    // ************************************************************
    // done and write data to file.

    write_to_file(out_dir + "/r_regionkey.dat", col_r_regionkey);
    write_to_file(out_dir + "/r_name.dat", col_r_name);

    write_to_file(out_dir + "/n_nationkey.dat", col_n_nationkey);
    write_to_file(out_dir + "/n_regionkey.dat", col_n_regionkey);
    write_to_file(out_dir + "/n_name.dat", col_n_name);

    write_to_file(out_dir + "/c_custkey.dat", col_c_custkey);
    write_to_file(out_dir + "/c_nationkey.dat", col_c_nationkey);

    write_to_file(out_dir + "/o_orderkey.dat", col_o_orderkey);
    write_to_file(out_dir + "/o_custkey.dat", col_o_custkey);
    write_to_file(out_dir + "/o_orderdate.dat", col_o_orderdate);

    write_to_file(out_dir + "/l_orderkey.dat", col_l_orderkey);
    write_to_file(out_dir + "/l_suppkey.dat", col_l_suppkey);
    write_to_file(out_dir + "/l_extendedprice.dat", col_l_extendedprice);
    write_to_file(out_dir + "/l_discount.dat", col_l_discount);

    write_to_file(out_dir + "/s_suppkey.dat", col_s_suppkey);
    write_to_file(out_dir + "/s_nationkey.dat", col_s_nationkey);

    return 0;
}
