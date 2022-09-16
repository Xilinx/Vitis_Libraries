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

/*
 * @file poseidon.hpp
 * @brief header file for Poseidon
 * This file part of Vitis Security Library.
 *
 */

#ifndef _XF_SECURITY_POSEIDON_HPP_
#define _XF_SECURITY_POSEIDON_HPP_

#include <ap_int.h>
#include <ap_fixed.h>
#include <hls_stream.h>
#include <limits>
#include <cmath>
#include <algorithm>
#include "modular.hpp"

#if !defined(__SYNTHESIS__)
#include <iostream>
#endif

namespace xf {
namespace security {

using namespace internal;

void quintic_sbox(ap_uint<256> prime, ap_uint<256> state, ap_uint<256>* out) {
    ap_uint<256> result1 = state;
    ap_uint<256> result2 = 0;
    ap_uint<256> result4 = 0;
    ap_uint<256> result5 = 0;
    result2 = productMod<256>(state, state, prime);
    result4 = productMod<256>(result2, result2, prime);
    result5 = productMod<256>(result1, result4, prime);

    *out = result5;
}

template <unsigned int t>
void perm_mds(ap_uint<256> prime, ap_uint<256> matrix[t][t], ap_uint<256> vector[t]) {
    ap_uint<256> internal_out[t];
#pragma HLS array_partition variable = internal_out complete
    ap_uint<256> tmp = 0;
LOOP_MDS_MATRIX_MUL_OUTER:
    for (int i = 0; i < t; i++) {
#pragma HLS loop_tripcount min = 0 max = t
#pragma HLS unroll
        internal_out[i] = 0;
    LOOP_MDS_MATRIX_MUL_INNER:
        for (int j = 0; j < t; j++) {
#pragma HLS loop_tripcount min = t max = t
#pragma HLS unroll
            tmp = productMod<256>(matrix[i][j], vector[j], prime);
            internal_out[i] = addMod<256>(internal_out[i], tmp, prime);
        }
    }
LOOP_PERM_MATRIX_MUL_UPDATE:
    for (int i = 0; i < t; i++) {
#pragma HLS unroll
        vector[i] = internal_out[i];
    }
}

template <unsigned int t>
void perm_round_function(bool isFullRound,
                         ap_uint<256> prime,
                         ap_uint<256> stateWords[t],
                         ap_uint<256> roundConstants[t],
                         ap_uint<256> mdsMatrix[t][t]) {
LOOP_ADS:
    for (int i = 0; i < t; i++) {
#pragma HLS unroll
        stateWords[i] = addMod<256>(stateWords[i], roundConstants[i], prime);
    }
    if (true == isFullRound) {
    LOOP_FULL_ROUDN_SBOX:
        for (int j = 0; j < t; j++) {
#pragma HLS unroll
            ap_uint<256> fsbox_out = 0;
            quintic_sbox(prime, stateWords[j], &fsbox_out);
            stateWords[j] = fsbox_out;
        }
    } else {
        ap_uint<256> psbox_out = 0;
        quintic_sbox(prime, stateWords[t - 1], &psbox_out);
        stateWords[t - 1] = psbox_out;
    }
    perm_mds<t>(prime, mdsMatrix, stateWords);
}

template <unsigned int t, unsigned int alph, unsigned int n, unsigned int M, unsigned int RF, unsigned int RP>
void perm_top(ap_uint<256> prime,
              ap_uint<256> stateWords[t],
              ap_uint<256> constants[t * (RF + RP)],
              ap_uint<256> mdsMatrix[t][t]) {
    ap_uint<256> roundConstants[t];
#pragma HLS array_partition variable = roundConstants complete

    unsigned int R_f = RF / 2;
    bool isFullRound = true;
LOOP_POSEIDON_STATEWORDS:
    for (int r = 0; r < (RF + RP); r++) {
#pragma HLS loop_tripcount min = 1 max = RF + RP
        for (int i = 0; i < t; i++) {
#pragma HLS unroll
            roundConstants[i] = constants[r * t + i];
        }
        if ((r < R_f) || (r >= R_f + RP)) {
            isFullRound = true;
        } else {
            isFullRound = false;
        }
        perm_round_function<t>(isFullRound, prime, stateWords, roundConstants, mdsMatrix);
    }
}

template <unsigned int t, unsigned int alph, unsigned int n, unsigned int M, unsigned int RF, unsigned int RP>
void spongHash(ap_uint<256> prime,
               int inputLen,
               hls::stream<ap_uint<256> >& inputWordsStrm,
               hls::stream<ap_uint<256> >& roundConstantsStrm,
               hls::stream<ap_uint<256> >& mdsMatrixStrm,
               ap_uint<256>* outputWords) {
    ap_uint<256> mdsMatrix[t][t];
#pragma HLS array_partition variable = mdsMatrix complete
LOOP_SPONG_HASH_LOAD_MDS_MATRIX:
    for (int i = 0; i < t; i++) {
        for (int j = 0; j < t; j++) {
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount min = t max = t
            mdsMatrix[i][j] = mdsMatrixStrm.read();
        }
    }

    ap_uint<256> roundConstants[t * (RF + RP)];
//#pragma HLS bind_storage variable = roundConstants type=ram_2p impl=bram
LOOP_SPONG_HASH_LOAD_ROUND_CONSTS:
    for (int i = 0; i < t * (RF + RP); i++) {
#pragma HLS pipeline II = 1
#pragma HLS loop_tripcount min = t * (RF + RP) max = t * (RF + RP)
        roundConstants[i] = roundConstantsStrm.read();
    }

    ap_uint<256> stateWords[t];
#pragma HLS array_partition variable = stateWords complete
LOOP_SPONG_HASH_INIT_STATEWORDS:
    for (int i = 0; i < t; i++) {
#pragma HLS unroll
        stateWords[i] = 0;
    }

    int l = inputLen / (t - 1);
    int m = inputLen % (t - 1);
    if (m != 0) {
        l += 1;
    }
LOOP_SPONG_HASH_CHUNKS_OUTER:
    for (int i = 0; i < l; i++) {
#pragma HLS loop_tripcount min = 1 max = 1000
    LOOP_SPONG_HASH_CHUNK_INNER:
        for (int j = 1; j < t; j++) {
#pragma HLS loop_tripcount min = t max = t
            if ((i == l - 1) && (m != 0)) {
                if (j < m + 1) {
                    stateWords[j] += inputWordsStrm.read();
                } else if (j == m + 1) {
                    stateWords[j] += 1;
                }
            } else {
                stateWords[j] += inputWordsStrm.read();
            }
        }

        if ((i == l - 1) && (m == 0)) {
            perm_top<t, alph, n, M, RF, RP>(prime, stateWords, roundConstants, mdsMatrix);
            stateWords[1] += 1;
        }
        perm_top<t, alph, n, M, RF, RP>(prime, stateWords, roundConstants, mdsMatrix);
    }
    *outputWords = stateWords[1];
}

template <unsigned int t,
          unsigned int alph,
          unsigned int n,
          unsigned int M,
          unsigned int RF,
          unsigned int RP,
          char field = 1,
          char sbox,
          bool security_margin = true>
void poseidon(ap_uint<256> prime,
              int inputLen,
              hls::stream<ap_uint<256> >& inputWordsStrm,
              hls::stream<ap_uint<256> >& roundConstantsStrm,
              hls::stream<ap_uint<256> >& mdsMatrixStrm,
              ap_uint<256>* outputWords) {
    spongHash<t, alph, n, M, RF, RP>(prime, inputLen, inputWordsStrm, roundConstantsStrm, mdsMatrixStrm, outputWords);
}

} // namespace security
} // namespace xf

#endif
