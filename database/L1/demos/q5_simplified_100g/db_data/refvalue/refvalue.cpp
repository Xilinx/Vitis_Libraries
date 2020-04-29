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

FILE* fo(std::string fn) {
    FILE* f = fopen(fn.c_str(), "rb");
    if (!f) {
        printf("ERROR: %s cannot be opened for binary read.\n", fn.c_str());
    }
    return f;
}

void join(std::string& in_dir) {
    long long refvalue = 0;
    long jrow = 0;

    FILE* f_l_orderkey = fo(in_dir + "/l_orderkey.dat");
    FILE* f_l_extendedprice = fo(in_dir + "/l_extendedprice.dat");
    FILE* f_l_discount = fo(in_dir + "/l_discount.dat");
    FILE* f_o_orderkey = fo(in_dir + "/o_orderkey.dat");
    FILE* f_o_orderdate = fo(in_dir + "/o_orderdate.dat");

    FILE* flog = fopen("join.log", "w");
    if (!flog) {
        printf("ERROR: join.log cannot ben opened for write.\n");
    }

    TPCH_INT l_orderkey;
    TPCH_INT l_extendedprice;
    TPCH_INT l_discount;

    TPCH_INT o_orderkey;
    TPCH_INT o_orderdate;

    int nl = 1;
    nl &= fread(&l_orderkey, sizeof(TPCH_INT), 1, f_l_orderkey);
    nl &= fread(&l_extendedprice, sizeof(TPCH_INT), 1, f_l_extendedprice);
    nl &= fread(&l_discount, sizeof(TPCH_INT), 1, f_l_discount);

    // read data line
    int no = 1;
    while (no == 1) {
        no = 1;
        no &= fread(&o_orderkey, sizeof(TPCH_INT), 1, f_o_orderkey);
        no &= fread(&o_orderdate, sizeof(TPCH_INT), 1, f_o_orderdate);
        if (no == 1) {
            if (o_orderdate >= 19940101 && o_orderdate < 19950101) {
                while (l_orderkey != o_orderkey) {
                    nl = 1;
                    nl &= fread(&l_orderkey, sizeof(TPCH_INT), 1, f_l_orderkey);
                    nl &= fread(&l_extendedprice, sizeof(TPCH_INT), 1, f_l_extendedprice);
                    nl &= fread(&l_discount, sizeof(TPCH_INT), 1, f_l_discount);
                    if (nl != 1) break;
                }
                while (l_orderkey == o_orderkey) {
                    long long v = l_extendedprice * (100 - l_discount);
                    refvalue += v;
                    fprintf(flog, "%d|%d|%d|%d\n", o_orderkey, o_orderdate, l_extendedprice, l_discount);
                    jrow++;
                    // next
                    nl = 1;
                    nl &= fread(&l_orderkey, sizeof(TPCH_INT), 1, f_l_orderkey);
                    nl &= fread(&l_extendedprice, sizeof(TPCH_INT), 1, f_l_extendedprice);
                    nl &= fread(&l_discount, sizeof(TPCH_INT), 1, f_l_discount);
                    if (nl != 1) break;
                }
            }
        }
    }
    fclose(f_l_orderkey);
    fclose(f_l_extendedprice);
    fclose(f_l_discount);
    fclose(f_o_orderkey);
    fclose(f_o_orderdate);

    fclose(flog);
    printf("joined row number: %ld\nref value: %lld.%lld\n", jrow, refvalue / 10000, refvalue % 10000);
}

int main(int argc, const char* argv[]) {
    // cmd arg parser.
    ArgParser parser(argc, argv);

    std::string in_dir;
    if (!parser.getCmdOption("-in", in_dir) || !is_dir(in_dir)) {
        printf("ERROR: input dir is not specified or not valid.\n");
        return 1;
    }

    int err = 0;

    struct timeval tv0, tv1;
    int usec;
    gettimeofday(&tv0, 0);

    join(in_dir);

    gettimeofday(&tv1, 0);
    usec = tvdiff(&tv0, &tv1);
    printf("Time to load lineitem and orders table: %d usec.\n", usec);

    return 0;
}
