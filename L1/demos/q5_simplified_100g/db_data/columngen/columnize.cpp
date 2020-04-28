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

void read_o(std::string& o_path, std::string& out_dir) {
    std::ifstream ifs;
    ifs.open(o_path, std::ios_base::in);
    if (!ifs) {
        printf("ERROR: %s cannot ben opened for read.\n", o_path.c_str());
        return;
    }
    std::string fn;
    fn = out_dir + "/o_orderkey.dat";
    FILE* f_orderkey = fopen(fn.c_str(), "wb");
    if (!f_orderkey) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        return;
    }
    fn = out_dir + "/o_orderdate.dat";
    FILE* f_orderdate = fopen(fn.c_str(), "wb");
    if (!f_orderdate) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_orderkey);
        return;
    }
    // read data line
    while (ifs) {
        orders_t t;
        ifs >> t;
        if (ifs) {
            // select columns
            TPCH_INT orderkey = t.orderkey;
            TPCH_INT orderdate = t.orderdate;
            // store to files
            fwrite(&orderkey, sizeof(TPCH_INT), 1, f_orderkey);
            fwrite(&orderdate, sizeof(TPCH_INT), 1, f_orderdate);
        }
    }
    ifs.close();
    fclose(f_orderkey);
    fclose(f_orderdate);
    printf("INFO: Loaded %s from disk.\n", o_path.c_str());
}

void read_l(std::string& l_path, std::string& out_dir) {
    std::ifstream ifs;
    ifs.open(l_path, std::ios_base::in);
    if (!ifs) {
        printf("ERROR: %s cannot ben opened for read.\n", l_path.c_str());
        return;
    }
    std::string fn = out_dir + "/l_orderkey.dat";
    FILE* f_orderkey = fopen(fn.c_str(), "wb");
    if (!f_orderkey) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        return;
    }
    fn = out_dir + "/l_extendedprice.dat";
    FILE* f_extendedprice = fopen(fn.c_str(), "wb");
    if (!f_extendedprice) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_orderkey);
        return;
    }
    fn = out_dir + "/l_discount.dat";
    FILE* f_discount = fopen(fn.c_str(), "wb");
    if (!f_discount) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_orderkey);
        fclose(f_extendedprice);
        return;
    }
    // read data line
    while (ifs) {
        lineitem_t t;
        ifs >> t;
        if (ifs) {
            // select columns
            TPCH_INT orderkey = t.orderkey;
            TPCH_INT extendedprice = t.extendedprice;
            TPCH_INT discount = t.discount;
            // store columns
            fwrite(&orderkey, sizeof(TPCH_INT), 1, f_orderkey);
            fwrite(&extendedprice, sizeof(TPCH_INT), 1, f_extendedprice);
            fwrite(&discount, sizeof(TPCH_INT), 1, f_discount);
        }
    }
    ifs.close();
    fclose(f_orderkey);
    fclose(f_extendedprice);
    fclose(f_discount);
    printf("INFO: Loaded %s from disk.\n", l_path.c_str());
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
    std::thread l_thread;
    std::thread o_thread;

    struct timeval tv0, tv1;
    int usec;

    gettimeofday(&tv0, 0);

    if (flag_o) o_thread = std::thread(read_o, std::ref(o_path), std::ref(out_dir));
    if (flag_l) l_thread = std::thread(read_l, std::ref(l_path), std::ref(out_dir));

    if (flag_o) o_thread.join();
    if (flag_l) l_thread.join();

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to load lineitem and orders table: %d usec.\n", usec);

    return 0;
}
