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
#ifndef SSSD_API_H
#define SSSD_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
// Why not add char type to defined fixed size string???
enum sssd_dtype_t {
    SSSD_DTYPE_INVALID = 0,
    SSSD_DTYPE_BOOL = 1,    /* 8-bit int*/
    SSSD_DTYPE_INT = 2,     /* 64-bit int */
    SSSD_DTYPE_FLOAT = 6,   /*64-bit double */
    SSSD_DTYPE_NUMERIC = 7, /* 64-bit struct */
    SSSD_DTYPE_STRING = 8,
    SSSD_DTYPE_DATE = 9,      // YYYY-MM-DD
    SSSD_DTYPE_TIMESTAMP = 10 // YYYY-MM-DDThhmmss.usec+hh:mm
};
typedef enum sssd_dtype_t sssd_dtype_t;

typedef struct sssd_date_t sssd_date_t;
struct sssd_date_t {
    int32_t year;
    int16_t month;
    int16_t day;
};

typedef struct sssd_timestamp_t sssd_timestamp_t;
struct sssd_timestamp_t {
    int32_t year;
    int16_t month;
    int16_t day;
    int32_t sec; /* covers hour/minute/second */
    int32_t usec;
    int16_t zsec; /* covers offset hour:minute */
};

typedef struct sssd_numeric_t sssd_numeric_t;
struct sssd_numeric_t {
    int64_t significand : 56;
    int exponent : 8;
};

typedef struct sssd_string_t sssd_string_t;
struct sssd_string_t {
    int32_t len;
    char byte[];
};

enum sssd_cmp_t {
    SSSD_GT,
    SSSD_GE,
    SSSD_LT,
    SSSD_LE,
    SSSD_EQ,
    SSSD_NEQ,
    //??? How to use
    SSSD_NOT,
    //??? How to use
    SSSD_ISNULL
};
typedef enum sssd_cmp_t sssd_cmp_t;

typedef struct sssd_schema_t sssd_schema_t;
struct sssd_schema_t {
    int32_t natt;
    char** aname;
    sssd_dtype_t* dtype;

    char* ftype; // "csv"
    //??need to check
    union {
        struct {
            char header;   // boolean; 0 defaults to false
            char delim;    // 0 defaults to comma
            char quote;    // 0 defaults to double-quote
            char escape;   // 0 defaults to double-quote
            char* nullstr; // 0 defaults to empty string, i.e. ""
        } csv;

        // future formats and their options
        // ...
    } u;
};

typedef struct sssd_filter_t sssd_filter_t;
struct sssd_filter_t {
    int att;
    sssd_dtype_t dtype;
    sssd_cmp_t cmp;
    //??? how to use
    bool arg_isnull;
    union {
        bool cmp_b;
        int64_t cmp_i64;
        double cmp_d;
        sssd_numeric_t cmp_n;
        sssd_string_t cmp_s;
        sssd_date_t cmp_date;
        sssd_timestamp_t cmp_tm;
    } arg_value;
};

// need add attributes to specify which cols is output??
typedef struct sssd_scandesc_t sssd_scandesc_t;
struct sssd_scandesc_t {
    sssd_schema_t schema;

    // project these attributes
    int32_t natt;
    int32_t* att;

    // filters
    // Note: we should make this into a tree for more sophisticated
    // filters in the future. Keeping it simple for now.
    int32_t nfilter;
    sssd_filter_t** filter;

    // hash these attributes
    int32_t nhashatt;
    int32_t* hashatt;
};
typedef struct sssd_info_t {
    int32_t device_id;
    const char* mount_path;
} sssd_info_t;
/**
 * Function signature of a callback from sssd_scan().  The param
 * att[] and isnull[] contains value of the row.  This function
 * should return 0 to continue, or -1 to stop the scan.
 * @param size size of one row data in bytes
 * @param value[] contains all the projection columns.
 * all the columns data are compacted together in-order.
 * For Q1
 *  value[0]  ----------------------
 *            |l_quantity(8B)      |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[1]  ----------------------
 *            |l_extendedprice(8B) |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[2] ----------------------
 *            |l_discount(8B)      |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[3] ----------------------
 *            |l_tax(8B)           |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[4] ----------------------
 *            |l_returnflag(5B)    |                      ---------------
 *            |    - pointer(8b)   |--------------------->|len(4B)      |
 *  value[5] ----------------------                      ---------------
 *            |l_linestatus(5B)    |                      |char(1B)     |
 *            |    - pointer(8B)   |----------            ---------------
 *  value[6] ----------------------         |
 *            |l_shipdate(8B)      |         |            ---------------
 *            |    - year(4B)      |         ------------>|len(4B)      |
 *            |    - month(2B)     |                      ---------------
 *            |    - day(2B)       |                      |char(1B)     |
 *            ----------------------                      ---------------
 * For Q6, size = 32
 *  value[0]  ----------------------
 *            |l_quantity(8B)      |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[1]  ----------------------
 *            |l_extendedprice(8B) |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[2] ----------------------
 *            |l_discount(8B)      |
 *            |   -significand(56b)|
 *            |   -exponent(8b)    |
 *  value[3] ----------------------
 *            |l_shipdate(8B)      |
 *            |    - year(4B)      |
 *            |    - month(2B)     |
 *            |    - day(2B)       |
 *            ----------------------
 *
 */

typedef int (*sssd_scanfn_t)(const int64_t value[], const bool isnull[], int32_t hash, void* context);
/**
 *   Function signature of a callback from sssd_list().
 *   Returns 0 to continue, or -1 to stop the list.
 */
typedef int (*sssd_listfn_t)(const char* path, void* context);
// all functions return 0 on success, -1 on failure.

/**
 *   Startup and shutdown.
 */
typedef void* sssd_t;
/**
 * Define types of error.
 */
enum ErrorCode { SUCCESS = 0, CFG_ERR = 1, DEV_ERR = 2, MEM_ERR = 3 };

// TODO:
// In the future, need to povide api for user to get the mount info
sssd_t sssd_init(const char* xclbinpath, int ndisk, sssd_info_t* disks); /* returns  */

void sssd_final(sssd_t handle);

/* --- ERRORS ---------------------------- */

/**
 *   Return the errnum or errmsg of last failure
 */

int sssd_errnum(sssd_t handle);

const char* sssd_errmsg(sssd_t handle);

/**
 *   Starts a scan and blocks until the scan finishes. During the
 *   scan, the callback function fn will be invoked for each row that
 *   passed the filters. The param context points to a user defined
 *   memory region, which will be passed untouched along to fn.
 *
 *   If fname ends with ".gz", gunzip will be automatically done
 *   before scan.
 */

// disk information can be infered from fname, maybe deleted???
int sssd_scan(sssd_t sssd, const char* fname, sssd_scandesc_t* sd, sssd_scanfn_t fn, void* context);

/**
 *   List the all files on disk that match the path_pattern, and
 *   invoke the callback function fn for each path that qualifies.
 */

int sssd_list(sssd_t sssd, const char* path_pattern, sssd_listfn_t fn, void* context);
#ifdef __cplusplus
}
#endif
#endif
