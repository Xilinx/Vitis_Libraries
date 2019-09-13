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

void read_l(std::string& l_path, std::string& out_dir) {
    std::ifstream ifs;
    ifs.open(l_path, std::ios_base::in);
    if (!ifs) {
        printf("ERROR: %s cannot ben opened for read.\n", l_path.c_str());
        return;
    }
    std::string fn = out_dir + "/l_shipdate.dat";
    FILE* f_shipdate = fopen(fn.c_str(), "wb");
    if (!f_shipdate) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        return;
    }
    fn = out_dir + "/l_discount.dat";
    FILE* f_discount = fopen(fn.c_str(), "wb");
    if (!f_discount) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_shipdate);
        return;
    }
    fn = out_dir + "/l_quantity.dat";
    FILE* f_quantity = fopen(fn.c_str(), "wb");
    if (!f_quantity) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_shipdate);
        fclose(f_discount);
        return;
    }
    fn = out_dir + "/l_commitdate.dat";
    FILE* f_commitdate = fopen(fn.c_str(), "wb");
    if (!f_commitdate) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_shipdate);
        fclose(f_discount);
        fclose(f_quantity);
        return;
    }
    fn = out_dir + "/l_extendedprice.dat";
    FILE* f_extendedprice = fopen(fn.c_str(), "wb");
    if (!f_extendedprice) {
        printf("ERROR: %s cannot ben opened for write.\n", fn.c_str());
        ifs.close();
        fclose(f_shipdate);
        fclose(f_discount);
        fclose(f_quantity);
        fclose(f_commitdate);
        return;
    }
    // read data line
    while (ifs) {
        lineitem_t t;
        ifs >> t;
        if (ifs) {
            // select columns
            TPCH_INT shipdate = t.shipdate;
            TPCH_INT discount = t.discount;
            TPCH_INT quantity = t.quantity;
            TPCH_INT commitdate = t.commitdate;
            TPCH_INT extendedprice = t.extendedprice;
            // store columns
            fwrite(&shipdate, sizeof(TPCH_INT), 1, f_shipdate);
            fwrite(&discount, sizeof(TPCH_INT), 1, f_discount);
            fwrite(&quantity, sizeof(TPCH_INT), 1, f_quantity);
            fwrite(&commitdate, sizeof(TPCH_INT), 1, f_commitdate);
            fwrite(&extendedprice, sizeof(TPCH_INT), 1, f_extendedprice);
        }
    }
    ifs.close();
    fclose(f_shipdate);
    fclose(f_discount);
    fclose(f_quantity);
    fclose(f_commitdate);
    fclose(f_extendedprice);
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

    std::string out_dir = ".";
    parser.getCmdOption("-d", out_dir);
    if (!is_dir(out_dir)) {
        printf("ERROR: \"%s\" is not a directory!\n", out_dir.c_str());
        ++err;
    }

    if (err) return err;

    // set up input data and buffers
    std::thread l_thread;

    struct timeval tv0, tv1;
    int usec;

    gettimeofday(&tv0, 0);

    read_l(l_path, out_dir);

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to load lineitem table: %d usec.\n", usec);

    return 0;
}
