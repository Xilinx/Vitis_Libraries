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

#define AP_INT_MAX_W 4096
#include <ap_int.h>

namespace xf {
namespace security {

/**
 * @brief RSA encryption/decryption class
 *
 * @tparam BlockWidth Basic multiplication width, should be picked according to cards.
 * @tparam BlockNum Number of Blocks. keyLength = BlockNum * BlockWidth.
 */
template <int BlockWidth, int BlockNum>
class rsa {
   private:
    const static int keyLength = BlockWidth * BlockNum;
    ap_uint<keyLength> nModulus;
    ap_uint<keyLength> nP;
    ap_uint<keyLength> nExponent;
    int startBit;

    void swap(ap_int<keyLength + 2>& x, ap_int<keyLength + 2>& y) {
#pragma HLS inline
        ap_int<keyLength + 2> tmp;
        tmp = x;
        x = y;
        y = tmp;
    }

    void extendEculid(ap_int<keyLength + 2> u,
                      ap_int<keyLength + 2> v,
                      ap_int<keyLength + 2>& a,
                      ap_int<keyLength + 2>& b) {
        ap_int<keyLength + 2> u1, u2, u3;
        ap_int<keyLength + 2> t1, t2, t3;

        if (u < v) {
            swap(u, v);
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
                    swap(u1, t1);
                    swap(u2, t2);
                    swap(u3, t3);
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

    void elementMulAdd(ap_uint<BlockWidth> a,
                       ap_uint<BlockWidth> b,
                       ap_uint<BlockWidth> c,
                       ap_uint<BlockWidth> d,
                       ap_uint<BlockWidth>& lower,
                       ap_uint<BlockWidth>& upper) {
#pragma HLS inline
        ap_uint<BlockWidth + 1> sumcd = c + d;
        ap_uint<BlockWidth* 2> result = a * b + sumcd;
        lower = result.range(BlockWidth - 1, 0);
        upper = result.range(BlockWidth * 2 - 1, BlockWidth);
    }

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
                elementMulAdd(opa, opb, opc, opd, lower[pos], upper[pos]);
            }
            c.range(i * BlockWidth + BlockWidth - 1, i * BlockWidth) = lower[i];
        }
    LOOP_u:
        ap_uint<1> move = 0;
        for (int i = BlockNum; i < BlockNum * 2; i++) {
#pragma HLS pipeline
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
        }
    }

    void REDC(ap_uint<BlockWidth * BlockNum * 2> t,
              ap_uint<BlockWidth * BlockNum> n,
              ap_uint<BlockWidth * BlockNum> np,
              ap_uint<BlockWidth * BlockNum>& result) {
#pragma HLS inline off
        ap_uint<BlockWidth * BlockNum> tmp1;
        ap_uint<BlockWidth * BlockNum * 2> tmp2;
        tmp1 = t.range(BlockWidth * BlockNum - 1, 0);                            // t mod R
        bigIntMul(tmp1, np, tmp2);                                               //(t mod R) * np
        tmp1 = tmp2.range(BlockWidth * BlockNum - 1, 0);                         //(t mod R) * np mod R
        bigIntMul(tmp1, n, tmp2);                                                // m * n;
        tmp2 += t;                                                               // t + m * n
        tmp1 = tmp2.range(BlockWidth * BlockNum * 2 - 1, BlockWidth * BlockNum); //(t + m * n) / R
        if (tmp1 > n) {
            tmp1 -= n;
        }
        result = tmp1;
    }

   public:
    /**
     * @brief Update key before use it to encrypt message
     *
     * @param modulus Modulus in RSA public key.
     * @param exponent Exponent in RSA public key or private key.
     */
    void updateKey(ap_uint<keyLength> modulus, ap_uint<keyLength> exponent) {
        nModulus = modulus;
        nExponent = exponent;
        startBit = keyLength - 2 - exponent.countLeadingZeros(); // bit position of bit after first 1

        ap_int<keyLength + 2> R, Ne, Rp, Np;
        R = 0;
        R[keyLength] = 1;
        Ne = modulus;
        extendEculid(R, Ne, Rp, Np);
        nP = Np.range(keyLength - 1, 0);
    }

    /**
     * @brief Encrypt message and get result
     *
     * @param message Message to be encrypted/decrypted
     * @param result Generated encrypted/decrypted result
     */
    void process(ap_uint<keyLength> message, ap_uint<keyLength>& result) {
        // Transform message to its Montegomery representation, mr
        // mr = message * (1 << KeyLength) mod N
        ap_uint<keyLength* 2> tmp = 0;
        tmp.range(2 * keyLength - 1, keyLength) = message(keyLength - 1, 0);
        ap_uint<keyLength> mr = tmp % nModulus;

        // Perform modular exponential
        ap_uint<keyLength> rr = mr;
        for (int i = startBit; i >= 0; i--) {
            bigIntMul(rr, rr, tmp);
            REDC(tmp, nModulus, nP, rr);
            if (nExponent[i] == 1) {
                bigIntMul(rr, mr, tmp);
                REDC(tmp, nModulus, nP, rr);
            }
        }

        // Transform result back to normal representation
        REDC(rr, nModulus, nP, result);
    }
};

} // namespace security
} // namespace xf

#endif
