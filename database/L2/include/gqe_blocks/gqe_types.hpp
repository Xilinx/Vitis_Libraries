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

#ifndef RKA_TYPES_H
#define RKA_TYPES_H

// XXX inline with tpch_read_2.h
#include <stdint.h>
typedef int32_t TPCH_INT;

#define TPCH_INT_SZ sizeof(TPCH_INT)

typedef TPCH_INT MONEY_T;
typedef TPCH_INT DATE_T;
typedef TPCH_INT KEY_T;

#define TPCH_INT_SZ sizeof(TPCH_INT)
#define MONEY_SZ sizeof(TPCH_INT)
#define DATE_SZ sizeof(TPCH_INT)
#define KEY_SZ sizeof(TPCH_INT)

#define VEC_LEN 16

#define FILTER_MAX_ROW 1 << 20
#define HASHJOIN_MAX_ROW 1 << 20
#define AGGREGATE_MAX_ROW 1 << 20

#define BURST_LEN 32
///
///#define S2_NM  16
///#define S3_NM  16
///#define S4_NM  16
///#define S5_NM  16
///#define S6_NM  16
///#define D_DEPTH (L_MAX_ROW / VEC_LEN + 1)
///
///#define BURST_LEN 64

#endif // TABLE_DT_H
