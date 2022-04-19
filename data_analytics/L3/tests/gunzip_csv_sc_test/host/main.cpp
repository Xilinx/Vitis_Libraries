/*
 * Copyright 2022 Xilinx, Inc.
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

// Example: scan multiple files on 1 SmartSSD for Q1 and Q6
// main thread
#include "sssd_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <sys/time.h>
struct list_out_t {
    int32_t fnm;
    char** list_out;
};
struct scan_out_t {
    // input config
    sssd_scandesc_t* sd;
    // output row number of scan
    int32_t row_nm;
    // int64_t* scan_out;
};
typedef struct list_out_t list_out_t;
typedef struct scan_out_t scan_out_t;

inline int tvdiff(struct timeval* tv0, struct timeval* tv1) {
    return (tv1->tv_sec - tv0->tv_sec) * 1000000 + (tv1->tv_usec - tv0->tv_usec);
}

int sssd_scanfn(const int64_t value[], const bool isnull[], int32_t hash, void* context) {
    // How to know size??
    scan_out_t* out_p = (scan_out_t*)context;
    out_p->row_nm++;
    sssd_scandesc_t* sd = out_p->sd;
    if (out_p->row_nm < 5) {
        for (int i = 0; i < sd->natt; ++i) {
            int nm = sd->att[i];
            switch (sd->schema.dtype[nm]) {
                case (SSSD_DTYPE_INT):
                case (SSSD_DTYPE_BOOL): {
                    printf("%d, ", value[i]);
                    break;
                }
                case (SSSD_DTYPE_DATE): {
                    sssd_date_t dt;
                    memcpy(&dt, &value[i], sizeof(int64_t));
                    printf("%d-%d-%d, ", dt.year, dt.month, dt.day);
                    break;
                }
                case (SSSD_DTYPE_NUMERIC): {
                    sssd_numeric_t dt;
                    memcpy(&dt, &value[i], sizeof(int64_t));
                    printf("%de%d, ", dt.significand, dt.exponent);
                    break;
                }
                case (SSSD_DTYPE_STRING): {
                    sssd_string_t* dt = reinterpret_cast<sssd_string_t*>(value[i]);
                    printf("%d chars %.*s, ", dt->len, dt->len, dt->byte);
                    break;
                }
                default:
                    printf("Error: no supported output data type\n");
            }
        }
        printf("hash: %d\n", hash);
    }
    return 0;
}

int sssd_listfn(const char* path, void* context) {
    list_out_t* out_p = (list_out_t*)context;
    strcpy(out_p->list_out[out_p->fnm], path);
    printf("path is %s\n", out_p->list_out[out_p->fnm]);
    out_p->fnm++;
    return 0;
}
int main(int argc, char* argv[]) {
    printf("argc %d\n", argc);
    if (argc < 2) {
        printf("xclbin path is not set\n");
        return -1;
    }
    if (argc < 3) {
        printf("path pattern is not set\n");
        return -1;
    };
    char* xclbin_path = argv[1];
    char* path_pattern = argv[2];
    int card_num = 1;
    if (argc < 4) {
        printf("please set mount info first\n");
        return -1;
    }
    char* mount_point = argv[3];
    sssd_info_t disk0 = {0, mount_point};
    sssd_t sssd = sssd_init(xclbin_path, card_num, &disk0);

    // set the schema
    sssd_schema_t schema;
    schema.natt = 16;
    sssd_dtype_t* dtype = (sssd_dtype_t*)malloc(sizeof(sssd_dtype_t) * schema.natt);
    dtype[0] = SSSD_DTYPE_INT;     // l_orderkey
    dtype[1] = SSSD_DTYPE_INT;     // l_partkey
    dtype[2] = SSSD_DTYPE_INT;     // l_suppkey
    dtype[3] = SSSD_DTYPE_INT;     // l_linenumber
    dtype[4] = SSSD_DTYPE_NUMERIC; // l_quantity
    dtype[5] = SSSD_DTYPE_NUMERIC; // l_extendedprice
    dtype[6] = SSSD_DTYPE_NUMERIC; // l_discount
    dtype[7] = SSSD_DTYPE_NUMERIC; // l_tax
    dtype[8] = SSSD_DTYPE_STRING;  // l_returnflag
    dtype[9] = SSSD_DTYPE_STRING;  // l_linestatus
    dtype[10] = SSSD_DTYPE_DATE;   // l_shipdate
    dtype[11] = SSSD_DTYPE_DATE;   // l_commitdate
    dtype[12] = SSSD_DTYPE_DATE;   // l_receiptdate
    dtype[13] = SSSD_DTYPE_STRING; // l_shipinstruct
    dtype[14] = SSSD_DTYPE_STRING; // l_shipmode
    dtype[15] = SSSD_DTYPE_STRING; // l_comment
    schema.dtype = dtype;
    schema.ftype = "csv";
    schema.u.csv.header = 0;
    schema.u.csv.delim = 0;
    schema.u.csv.quote = 0;

    // For Q1
    sssd_scandesc_t sd_q1;

    sd_q1.schema = schema;
    sd_q1.nhashatt = 2;
    sd_q1.hashatt = (int32_t*)malloc(sizeof(int32_t) * sd_q1.nhashatt);
    sd_q1.hashatt[0] = 8;
    sd_q1.hashatt[1] = 9;
    // projection
    sd_q1.natt = 7;
    sd_q1.att = (int32_t*)malloc(sizeof(int32_t) * sd_q1.natt);
    sd_q1.att[0] = 4;  // l_quantity
    sd_q1.att[1] = 5;  // l_extendedprice
    sd_q1.att[2] = 6;  // l_discount;
    sd_q1.att[3] = 7;  // l_tax;
    sd_q1.att[4] = 8;  // l_returnflag
    sd_q1.att[5] = 9;  // l_linestatus;
    sd_q1.att[6] = 10; // l_shipdate;
    // filter
    sd_q1.nfilter = 1;
    sssd_filter_t** filter = (sssd_filter_t**)malloc(sizeof(sssd_filter_t*) * sd_q1.nfilter);
    for (int i = 0; i < sd_q1.nfilter; ++i) {
        filter[i] = (sssd_filter_t*)malloc(sizeof(sssd_filter_t));
    }
    // l_shipdate <= 19980902
    filter[0]->att = 10; // l_shipdate
    filter[0]->dtype = SSSD_DTYPE_DATE;
    filter[0]->cmp = SSSD_LE;
    filter[0]->arg_value.cmp_date.year = 1998;
    filter[0]->arg_value.cmp_date.month = 9;
    filter[0]->arg_value.cmp_date.day = 2;

    sd_q1.filter = filter;

    // set callback
    sssd_listfn_t fl = sssd_listfn;
    sssd_scanfn_t fn = sssd_scanfn;
    list_out_t list_ctxt = {0, 0};
    list_ctxt.list_out = (char**)malloc(sizeof(char*) * 1024);
    for (int i = 0; i < 1024; ++i) {
        list_ctxt.list_out[i] = (char*)malloc(sizeof(char) * 1024);
    }
    //// Multiple thread test
    std::thread t1(
        [&sssd, &fl, &list_ctxt](const char* pattern) {
            int ret = sssd_list(sssd, pattern, fl, &list_ctxt);
            if (ret == -1) printf("list failed\n");
        },
        path_pattern);
    t1.join();
    printf("fnm = %d\n", list_ctxt.fnm);

    std::thread t_pool[list_ctxt.fnm];
    scan_out_t* scan_ctxt = (scan_out_t*)malloc(sizeof(scan_out_t) * list_ctxt.fnm);
    int t_nm = 36;
    if (list_ctxt.fnm < t_nm) t_nm = list_ctxt.fnm;
    for (int i = 0; i < t_nm; ++i) {
        // int ret = sssd_scan(sssd, list_ctxt.list_out[i], &sd_q1, fn, &scan_ctxt[i]);
        // if(i < list_ctxt.fnm) {
        t_pool[i] = std::thread(
            [&sssd, &sd_q1, &fn, &list_ctxt, &scan_ctxt](const int nm, const int id) {
                for (int j = 0; j < (list_ctxt.fnm + nm - 1) / nm; ++j) {
                    int idx = j * nm + id;
                    if (idx < list_ctxt.fnm) {
                        scan_ctxt[idx].row_nm = 0;
                        scan_ctxt[idx].sd = &sd_q1;
                        char* file_name = list_ctxt.list_out[idx];
                        scan_out_t* ctxt = &scan_ctxt[idx];
                        int ret = sssd_scan(sssd, file_name, &sd_q1, fn, ctxt);
                        if (ret == -1) printf("scan failed\n");
                    }
                }
            },
            t_nm, i);
    }
    for (int i = 0; i < t_nm; ++i) {
        t_pool[i].join();
        printf("output rows %d\n", scan_ctxt[i].row_nm);
    }
    free(sd_q1.att);
    for (int i = 0; i < sd_q1.nfilter; ++i) {
        free(filter[i]);
    }
    free(filter);

    free(sd_q1.hashatt);

    sssd_scandesc_t sd_q6;

    // For Q6
    sd_q6.schema = schema;

    // projection
    sd_q6.natt = 4;
    sd_q6.att = (int32_t*)malloc(sizeof(int32_t) * sd_q6.natt);
    sd_q6.att[0] = 4;  // l_quantity
    sd_q6.att[1] = 5;  // l_extendedprice
    sd_q6.att[2] = 6;  // l_discount;
    sd_q6.att[3] = 10; // l_shipdate;
    // filter
    sd_q6.nfilter = 5;

    sssd_filter_t** filter_q6 = (sssd_filter_t**)malloc(sizeof(sssd_filter_t*) * sd_q6.nfilter);
    for (int i = 0; i < sd_q6.nfilter; ++i) {
        filter_q6[i] = (sssd_filter_t*)malloc(sizeof(sssd_filter_t));
    }
    // l_shipdate >= 19940101
    filter_q6[0]->att = 10; // l_shipdate
    filter_q6[0]->dtype = SSSD_DTYPE_DATE;
    filter_q6[0]->cmp = SSSD_GE;
    filter_q6[0]->arg_value.cmp_date.year = 1994;
    filter_q6[0]->arg_value.cmp_date.month = 1;
    filter_q6[0]->arg_value.cmp_date.day = 1;
    // l_shipdate < 19950101
    filter_q6[1]->att = 10; // l_shipdate
    filter_q6[1]->dtype = SSSD_DTYPE_DATE;
    filter_q6[1]->cmp = SSSD_LT;
    filter_q6[1]->arg_value.cmp_date.year = 1995;
    filter_q6[1]->arg_value.cmp_date.month = 1;
    filter_q6[1]->arg_value.cmp_date.day = 1;
    // l_discout >= 0.05
    filter_q6[2]->att = 6;
    filter_q6[2]->dtype = SSSD_DTYPE_NUMERIC;
    filter_q6[2]->cmp = SSSD_GE;
    filter_q6[2]->arg_value.cmp_n.significand = 5;
    filter_q6[2]->arg_value.cmp_n.exponent = -2;
    // l_discout <= 0.07
    filter_q6[3]->att = 6;
    filter_q6[3]->dtype = SSSD_DTYPE_NUMERIC;
    filter_q6[3]->cmp = SSSD_LE;
    filter_q6[3]->arg_value.cmp_n.significand = 7;
    filter_q6[3]->arg_value.cmp_n.exponent = -2;

    // l_quantity < 24
    filter_q6[4]->att = 4;
    filter_q6[4]->dtype = SSSD_DTYPE_NUMERIC;
    filter_q6[4]->cmp = SSSD_LT;
    filter_q6[4]->arg_value.cmp_n.significand = 24;
    filter_q6[4]->arg_value.cmp_n.exponent = 0;
    sd_q6.filter = filter_q6;
    sd_q6.nhashatt = 0;

    struct timeval tr0, tr1;
    gettimeofday(&tr0, 0);
    for (int i = 0; i < list_ctxt.fnm; ++i) {
        scan_ctxt[i].row_nm = 0;
        scan_ctxt[i].sd = &sd_q6;

        // int ret = sssd_scan(sssd, list_ctxt.list_out[i], &sd_q1, fn, &scan_ctxt[i]);
        t_pool[i] = std::thread(
            [&sssd, &sd_q6, &fn](const char* file_name, scan_out_t* ctxt) {
                int ret = sssd_scan(sssd, file_name, &sd_q6, fn, ctxt);
                if (ret == -1) printf("scan failed\n");
            },
            list_ctxt.list_out[i], &scan_ctxt[i]);
        // path_pattern, &scan_ctxt[i]);
    }
    for (int i = 0; i < list_ctxt.fnm; ++i) {
        t_pool[i].join();
        printf("output rows %d\n", scan_ctxt[i].row_nm);
    }
    gettimeofday(&tr1, 0);
    std::cout << "e2e execution time " << tvdiff(&tr0, &tr1) / 1000.0 << "ms" << std::endl;
    sssd_final(sssd);
    free(dtype);
    free(sd_q6.att);
    for (int i = 0; i < sd_q6.nfilter; ++i) {
        free(filter_q6[i]);
    }
    free(filter_q6);
    free(scan_ctxt);

    for (int i = 0; i < 1024; ++i) {
        free(list_ctxt.list_out[i]);
    }
    free(list_ctxt.list_out);
}
