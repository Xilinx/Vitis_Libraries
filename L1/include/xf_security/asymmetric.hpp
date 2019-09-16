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

#ifndef _XF_SECURITY_ASYMMETRIC_CRYPTOGRAPHY_HPP_
#define _XF_SECURITY_ASYMMETRIC_CRYPTOGRAPHY_HPP_

#define AP_INT_MAX_W 8192
#include <ap_int.h>

namespace xf {
namespace security {
namespace details {

template <int BitWidth>
void swap(ap_int<BitWidth>& x, ap_int<BitWidth>& y) {
#pragma HLS inline
    ap_int<BitWidth> tmp;
    tmp = x;
    x = y;
    y = tmp;
}

// a * u  - b * v = gcd. only applied when gcd = 1
template <int BitWidth>
void extendEculid(ap_int<BitWidth + 2> u, ap_int<BitWidth + 2> v, ap_int<BitWidth + 2>& a, ap_int<BitWidth + 2>& b) {
    ap_int<BitWidth + 2> u1, u2, u3;
    ap_int<BitWidth + 2> t1, t2, t3;

    if (u < v) {
        swap<BitWidth + 2>(u, v);
    }

    u1 = 1;
    u2 = 0;
    u3 = u;
    t1 = v;
    t2 = u - 1;
    t3 = v;

    do {
        do {
            if (u3[0] == 0) {
                if (u1[0] == 1 || u2[0] == 1) {
                    u1 += v;
                    u2 += u;
                }
                u1 >>= 1;
                u2 >>= 1;
                u3 >>= 1;
            }
            if (t3[0] == 0 || u3 < t3) {
                swap<BitWidth + 2>(u1, t1);
                swap<BitWidth + 2>(u2, t2);
                swap<BitWidth + 2>(u3, t3);
            }
        } while (u3[0] == 0);
        while (u1 < t1 || u2 < t2) {
            u1 += v;
            u2 += u;
        }
        u1 -= t1;
        u2 -= t2;
        u3 -= t3;
    } while (t3 > 0);
    while (u1 >= v && u2 >= u) {
        u1 -= v;
        u2 -= u;
    }
    a = u1;
    b = u2;
}

template <int Width>
void elementMulAdd(ap_uint<Width> a,
                   ap_uint<Width> b,
                   ap_uint<Width> c,
                   ap_uint<Width> d,
                   ap_uint<Width>& lower,
                   ap_uint<Width>& upper) {
#pragma HLS inline
    ap_uint<Width + 1> sumcd = c + d;
    ap_uint<Width* 2> result = a * b + sumcd;
    lower = result.range(Width - 1, 0);
    upper = result.range(Width * 2 - 1, Width);
}

template <int BlockWidth, int BlockNum>
void bigIntMul(ap_uint<BlockWidth * BlockNum> a,
               ap_uint<BlockWidth * BlockNum> b,
               ap_uint<BlockWidth * BlockNum * 2>& c) {
#pragma HLS inline off
    ap_uint<BlockWidth> lower[BlockNum * 2];
    ap_uint<BlockWidth> upper[BlockNum * 2];
#pragma HLS ARRAY_PARTITION variable = lower dim = 0
#pragma HLS ARRAY_PARTITION variable = upper dim = 0

LOOP_i:
    for (int i = 0; i < BlockNum; i++) {
#pragma HLS pipeline
    LOOP_j:
        for (int j = BlockNum - 1; j >= 0; j--) {
#pragma HLS unroll
#pragma HLS dependence variable = lower inter false
#pragma HLS dependence variable = upper inter false
            ap_uint<BlockWidth> opa, opb, opc, opd;
            opa = a.range(i * BlockWidth + BlockWidth - 1, i * BlockWidth);
            opb = b.range(j * BlockWidth + BlockWidth - 1, j * BlockWidth);
            const int pos = i + j;
            const int pre = pos - 1;
            if (i == 0 || j == BlockNum - 1) {
                opc = 0;
            } else {
                opc = lower[pos];
            }
            if (i == 0) {
                opd = 0;
            } else {
                opd = upper[pre];
            }
            elementMulAdd<BlockWidth>(opa, opb, opc, opd, lower[pos], upper[pos]);
        }
        c.range(i * BlockWidth + BlockWidth - 1, i * BlockWidth) = lower[i];
    }
LOOP_u:
    ap_uint<1> move = 0;
    for (int i = BlockNum; i < BlockNum * 2; i++) {
#pragma HLS pipeline
        /*
if(i < BlockNum) {
    c.range(i * BlockWidth + BlockWidth - 1, i * BlockWidth) = lower[i];
} else {
                */
        ap_uint<BlockWidth> opl, opu;
        if (i != BlockNum * 2 - 1) {
            opl = lower[i];
        } else {
            opl = 0;
        }
        opu = upper[i - 1];
        ap_uint<BlockWidth + 1> tmp = opl + opu + move;
        c.range(i * BlockWidth + BlockWidth - 1, i * BlockWidth) = tmp.range(BlockWidth - 1, 0);
        move = tmp[BlockWidth];
        //}
    }
}

template <int BlockWidth, int BlockNum>
void REDC(ap_uint<BlockWidth * BlockNum * 2> t,
          ap_uint<BlockWidth * BlockNum> n,
          ap_uint<BlockWidth * BlockNum> np,
          ap_uint<BlockWidth * BlockNum>& result) {
#pragma HLS inline
    ap_uint<BlockWidth * BlockNum> tmp1;
    ap_uint<BlockWidth * BlockNum * 2> tmp2;
    tmp1 = t.range(BlockWidth * BlockNum - 1, 0);                            // t mod R
    bigIntMul<BlockWidth, BlockNum>(tmp1, np, tmp2);                         //(t mod R) * np
    tmp1 = tmp2.range(BlockWidth * BlockNum - 1, 0);                         //(t mod R) * np mod R
    bigIntMul<BlockWidth, BlockNum>(tmp1, n, tmp2);                          // m * n;
    tmp2 += t;                                                               // t + m * n
    tmp1 = tmp2.range(BlockWidth * BlockNum * 2 - 1, BlockWidth * BlockNum); //(t + m * n) / R
    if (tmp1 > n) {
        tmp1 -= n;
    }
    result = tmp1;
}

} // namespace details

/**
 * @brief RSA encryption/decryption function
 *
 * @tparam KeyLength Length of key, usually 1024/2048
 * @tparam BlockWidth Basic multiplication width, should be picked according to cards.
 * Also KeyLength shoud be divisible by BlockWidth.
 *
 * @param message Message to be encrypted or ciphertxt to be decrypted.
 * @param N Served as modulus in modular operation.
 * @param key Encryption/decryption key used.
 * @param result encrypiton/decryption result.
 */
template <int KeyLength, int BlockWidth>
void rsa(ap_uint<KeyLength> message, ap_uint<KeyLength> N, ap_uint<KeyLength> key, ap_uint<KeyLength>& result) {
    const int BlockNum = KeyLength / BlockWidth;
    // Transform message to its Montegomery representation, mr
    // mr = message * (1 << KeyLength) mod N
    ap_uint<KeyLength* 2> tmp = message;
    tmp <<= KeyLength;
    ap_uint<KeyLength> mr = tmp % N;

    // Calculate Np and Rp s.t. R * Rp - N * Np = 1
    ap_int<KeyLength + 2> R = 0;
    R[KeyLength] = 1;
    ap_int<KeyLength + 2> Ne, Rp, Np;
    Ne = N;
    details::extendEculid<KeyLength>(R, Ne, Rp, Np);

    // Perform modular exponential
    bool jump = true;
    ap_uint<KeyLength> rr = 1;
    for (int i = KeyLength - 1; i >= 0; i--) {
        if (jump == true) {
            if (key[i] == 1) {
                jump = false;
                rr = mr;
            }
        } else {
            details::bigIntMul<BlockWidth, BlockNum>(rr, rr, tmp);
            details::REDC<BlockWidth, BlockNum>(tmp, N, Np, rr);
            if (key[i] == 1) {
                details::bigIntMul<BlockWidth, BlockNum>(rr, mr, tmp);
                details::REDC<BlockWidth, BlockNum>(tmp, N, Np, rr);
            }
        }
    }

    // Transform result back to normal representation
    details::REDC<BlockWidth, BlockNum>(rr, N, Np, result);
}
} // namespace security
} // namespace xf

#endif
