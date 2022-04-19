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
#include "data_engine_config.hpp"

namespace sssd_engine {
// convert the argument of filter to current supported data format
// current:
// 1: all numeric is multiply by 100 to make it as integer
// 2: all date to is formated to integer
ap_uint<64> DataEngineConfig::convert(sssd_filter_t* filter) {
    ap_uint<64> out = 0;
    switch (filter->dtype) {
        case SSSD_DTYPE_BOOL: {
            out = filter->arg_value.cmp_b;
            break;
        }
        case SSSD_DTYPE_INT: {
            out = filter->arg_value.cmp_i64;
            break;
        }
        case SSSD_DTYPE_NUMERIC: {
            out.range(55, 0) = filter->arg_value.cmp_n.significand;
            out.range(63, 56) = filter->arg_value.cmp_n.exponent;
            break;
        }
        case SSSD_DTYPE_DATE: {
            out.range(63, 32) = filter->arg_value.cmp_date.year;
            out.range(31, 16) = filter->arg_value.cmp_date.month;
            out.range(15, 0) = filter->arg_value.cmp_date.day;
            break;
        }
        default:
            fprintf(stderr, "ERROR: no filter supported data type\n");
    }
    return out;
}

// Limitation - 1:
// 1: maximum supported 16 cols
// 2: maximum projection cols is 8
// 3: Data type
//   SSSD_DTYPE_INT16, SSSD_DTYPE_INT32, SSSD_DTYPE_INT64,
//   SSSD_DTYPE_BOOL,
//   SSSD_DTYPE_STRING (partially support, len < 8B)
//   SSSD_DTYPE_DATE
//   SSSD_DTYPE_NUMERIC (partially support, exponent(-2:0), range less than );
//   SSSD_DTYPE_TIMESTAMP (not)
//   SSSD_DTYPE_INVALID (not)
//
// 4: Filter cols less than 4
// 5: Filter ops SSSD_NOT and SSSD_ISNULL not support
// 6: Filter data type only for SSSD_DTYPE_INT16, SSSD_DTYPE_INT32, SSSD_DTYPE_INT64, SSSD_DTYPE_BOOL, SSSD_DTYPE_DATE,
// SSSD_DTYPE_NUMERIC
// 7: SSSD_DTYPE_STRING: no length return
// 8: SSSD_DTYPE_NUMERIC: no exponent return
ErrorCode DataEngineConfig::genConfigBits(bool gzip, sssd_scandesc_t* sd, uint64_t* out) {
    // Comparison operator of filter.
    enum FilterOp {
        FOP_DC = 0, ///< don't care, always true.
        FOP_EQ,     ///< equal
        FOP_NE,     ///< not equal
        FOP_GT,     ///< greater than, signed.
        FOP_LT,     ///< less than, signed.
        FOP_GE,     ///< greater than or equal, signed.
        FOP_LE,     ///< less than or equal, signed.
        FOP_GTU,    ///< greater than, unsigned.
        FOP_LTU,    ///< less than, unsigned.
        FOP_GEU,    ///< greater than or equal, unsigned.
        FOP_LEU     ///< less than or equal, unsigned.
    };
    ErrorCode err = SUCCESS;
    // 0: Gzip configuration
    // 1: projection configuration
    ap_uint<64>* cfg_buf = reinterpret_cast<ap_uint<64>*>(out);
    cfg_buf[0] = gzip;
    uint32_t natt = sd->schema.natt;
    // printf("natt = %d\n", natt);
    if (natt > 16) {
        fprintf(stderr, "ERROR: maximum supported 16 cols\n");
        err = CFG_ERR;
        return err;
    }
    ap_uint<16> e_bits = 0;
    for (unsigned int i = 0; i < sd->natt; ++i) {
        e_bits[sd->att[i]] = 1;
    }
    for (unsigned int i = 0; i < sd->nfilter; ++i) {
        e_bits[sd->filter[i]->att] = 1;
    }
    for (unsigned int i = 0; i < sd->nhashatt; ++i) {
        e_bits[sd->hashatt[i]] = 1;
    }
    uint32_t prj_natt = 0;
    for (int i = 0; i < 16; ++i) {
        if (e_bits[i] == 1) prj_natt++;
    }
    // printf("project natt = %d\n", prj_natt);
    if (prj_natt > 8) {
        fprintf(stderr, "ERROR: maximum projection cols is 8\n");
        err = CFG_ERR;
        return err;
    }

    ap_uint<64> cfg = 0;
    cfg.range(31, 16) = e_bits;
    cfg.range(7, 0) = natt;
    cfg.range(15, 8) = prj_natt;
    cfg_buf[1] = cfg;
    // 2-3: data type configuration
    enum FieldType { BOOLEAN = 0, INTEGER = 1, FLOAT, DOUBLE, DATE, STRING, NUMERIC, INVALID };
    FieldType ty;
    ap_uint<64> cfg_ty = 0;
    // remove the type convert in the future;
    // optimize: data type consistent with sw
    for (unsigned int i = 0; i < natt; ++i) {
        switch (sd->schema.dtype[i]) {
            case SSSD_DTYPE_INT: {
                ty = INTEGER;
                break;
            }
            case SSSD_DTYPE_STRING: {
                ty = STRING;
                break;
            }
            case SSSD_DTYPE_BOOL: {
                ty = BOOLEAN;
                break;
            }
            case SSSD_DTYPE_DATE: {
                ty = DATE;
                break;
            }
            case SSSD_DTYPE_NUMERIC: {
                ty = NUMERIC;
                break;
            }
            default: {
                ty = INVALID;
                err = CFG_ERR;
                fprintf(stderr, "ERROR: Date type %d is not supported by now\n", sd->schema.dtype[i]);
            }
        }
        cfg_ty.range(i * 4 + 3, i * 4) = ty;
    }
    /// 2: data type
    cfg_buf[2] = cfg_ty;
    /// 3: data type number
    cfg = 0;
    int data_type_nm[8] = {0};
    for (int i = 0; i < 16; ++i) {
        if (e_bits[i] == 1) {
            switch (sd->schema.dtype[i]) {
                case SSSD_DTYPE_INT: {
                    ty = INTEGER;
                    break;
                }
                case SSSD_DTYPE_STRING: {
                    ty = STRING;
                    break;
                }
                case SSSD_DTYPE_BOOL: {
                    ty = BOOLEAN;
                    break;
                }
                case SSSD_DTYPE_DATE: {
                    ty = DATE;
                    break;
                }
                case SSSD_DTYPE_NUMERIC: {
                    ty = NUMERIC;
                    break;
                }
                default: {
                    ty = INVALID;
                    err = CFG_ERR;
                    fprintf(stderr, "ERROR: Date type %d is not supported by now\n", sd->schema.dtype[i]);
                }
            }
            data_type_nm[ty]++;
        }
    }
    for (int i = 0; i < 8; ++i) {
        cfg.range((i + 1) * 4 - 1, i * 4) = data_type_nm[i];
    }
    cfg_buf[3] = cfg;

    int8_t scan_cols[8] = {-1};

    int c = 0;
    for (int i = 0; i < 16; ++i) {
        if (e_bits[i] == 1) scan_cols[c++] = i;
    }

    // generate configuration for filter
    // 4-17
    //
    int64_t lower_bound[8] = {0};
    int64_t upper_bound[8] = {0};
    int32_t att_ord[8] = {-1, -1, -1, -1};
    unsigned int filter_cols = 0;
    FilterOp ops_r[8] = {FOP_DC};
    FilterOp ops_l[8] = {FOP_DC};
    for (int i = 0; i < sd->nfilter; ++i) {
        sssd_filter_t* t = sd->filter[i];
        FilterOp op;
        switch (t->cmp) {
            case SSSD_GT: {
                op = FOP_GT;
                break;
            }
            case SSSD_GE: {
                op = FOP_GE;
                break;
            };
            case SSSD_LT: {
                op = FOP_LT;
                break;
            };
            case SSSD_LE: {
                op = FOP_LE;
                break;
            };
            case SSSD_EQ: {
                op = FOP_EQ;
                break;
            }
            case SSSD_NEQ: {
                op = FOP_NE;
                break;
            }
            default: {
                op = FOP_DC;
                fprintf(stderr, "ERROR: SSSD_NOT and SSSD_ISNULL is not supported by now\n");
                return CFG_ERR;
            }
        }
        unsigned int ord = 0;
        for (ord = 0; ord < prj_natt; ++ord) {
            if (t->att == scan_cols[ord]) break;
        }
        if (t->cmp == SSSD_GT || t->cmp == SSSD_GE) {
            lower_bound[ord] = convert(t);
            ops_l[ord] = op;
        } else if (t->cmp == SSSD_LT || t->cmp == SSSD_LE || t->cmp == SSSD_NEQ || t->cmp == SSSD_EQ) {
            upper_bound[ord] = convert(t);
            ops_r[ord] = op;
        }
    }
    if (filter_cols > 4) {
        fprintf(stderr, "ERROR: maxinum filter columns is 4\n");
        err = CFG_ERR;
        return err;
    }
    ap_uint<64> shuffle_in = 0;
    // 4: filter column number
    for (unsigned int i = 0; i < 8; ++i) {
        if (ops_r[i] != FOP_DC || ops_l[i] != FOP_DC) {
            filter_cols++;
            shuffle_in[i] = 1;
        }
    }
    cfg_buf[4] = filter_cols;
    // 5-16
    int sz = 5;
    for (unsigned int i = 0; i < 8; ++i) {
        if (ops_r[i] != FOP_DC || ops_l[i] != FOP_DC) {
            cfg_buf[sz++] = lower_bound[i];
            cfg_buf[sz++] = upper_bound[i];
            ap_uint<64> op = 0;
            op.range(3, 0) = ops_r[i];
            op.range(7, 4) = ops_l[i];
            op[8] = 1;
            cfg_buf[sz++] = op;
        }
    }
    for (int i = filter_cols; i < 4; ++i) {
        cfg_buf[sz++] = 0;
        cfg_buf[sz++] = 0;
        ap_uint<64> op = 0;
        op.range(3, 0) = FOP_DC; // ops_r[i];
        op.range(7, 4) = FOP_DC; // ops_l[i];
        op[8] = 0;
        cfg_buf[sz++] = op;
    }
    // 17: filter field
    cfg_buf[17] = shuffle_in;
    // 18: output field
    cfg = 0;
    for (unsigned int i = 0; i < sd->natt; ++i) {
        int j = -1;
        for (; j < 8; ++j) {
            if (scan_cols[j] == sd->att[i]) break;
        }
        if (j != -1) cfg[j] = 1;
    }
    cfg_buf[18] = cfg;
    // 19: crc32
    cfg = 0;
    for (int i = 0; i < sd->nhashatt; ++i) {
        int j = 0;
        for (; j < prj_natt; ++j) {
            if (scan_cols[j] == sd->hashatt[i]) break;
        }
        if (j < prj_natt) cfg[j] = 1;
    }
    cfg_buf[19] = cfg;

    return err;
}

} // namespace sssd_engine
