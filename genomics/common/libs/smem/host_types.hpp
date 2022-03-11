/*
 * (c) Copyright 2022 Xilinx, Inc. All rights reserved.
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
 *
 */
#ifndef HOST_TYPES_HPP
#define HOST_TYPES_HPP
#include "smem_specs.hpp"
#include "common.h"
#include "kvec.hpp"
// requirement: (OCC_INTERVAL%16 == 0); please DO NOT change this line because some part of the code assume
// OCC_INTERVAL=0x80
typedef uint64_t bwtint_t;

typedef struct { bwtint_t x[3], info; } bwtintv_t;

typedef struct {
    size_t n, m;
    bwtintv_t* a;
} bwtintv_v;

class smem_aux_t {
   public:
    int id_read;
    bwtintv_v mem, mem1, *tmpv[2];
};

typedef struct {
    int a, b; // match score and mismatch penalty
    int o_del, e_del;
    int o_ins, e_ins;
    int pen_unpaired;         // phred-scaled penalty for unpaired reads
    int pen_clip5, pen_clip3; // clipping penalty. This score is not deducted from the DP score.
    int w;                    // band width
    int zdrop;                // Z-dropoff

    uint64_t max_mem_intv;

    int T;            // output score threshold; only affecting output
    int flag;         // see MEM_F_* macros
    int min_seed_len; // minimum seed length
    int min_chain_weight;
    int max_chain_extend;
    float split_factor; // split into a seed if MEM is longer than min_seed_len*split_factor
    int split_width;    // split into a seed if its occurence is smaller than this value
    int max_occ;        // skip a seed if its occurence is larger than this value
    int max_chain_gap;  // do not chain seed if it is max_chain_gap-bp away from the closest seed
    int n_threads;      // number of threads
    int chunk_size;     // process chunk_size-bp sequences in a batch
    float mask_level;   // regard a hit as redundant if the overlap with another better hit is over mask_level times the
                        // min length of the two hits
    float drop_ratio; // drop a chain if its seed coverage is below drop_ratio times the seed coverage of a better chain
                      // overlapping with the small chain
    float XA_drop_ratio; // when counting hits for the XA tag, ignore alignments with score < XA_drop_ratio * max_score;
                         // only effective for the XA tag
    float mask_level_redun;
    float mapQ_coef_len;
    int mapQ_coef_fac;
    int max_ins;    // when estimating insert size distribution, skip pairs with insert longer than this value
    int max_matesw; // perform maximally max_matesw rounds of mate-SW for each end
    int max_XA_hits, max_XA_hits_alt; // if there are max_hits or fewer, output them all
    int8_t mat[25];                   // scoring matrix; mat[0] == 0 if unset
} mem_opt_t;

#endif
