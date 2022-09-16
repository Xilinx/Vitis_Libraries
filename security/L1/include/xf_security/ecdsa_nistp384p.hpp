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

/**
 * @file ecdsa.hpp
 * @brief header file for Elliptic Curve Digital Signature Algorithm
 * related function. Now it support curve secp256k1.
 * This file is part of Vitis Security Library.
 */

#ifndef _XF_SECURITY_NISTP384_HPP_
#define _XF_SECURITY_NISTP384_HPP_

#include <ap_int.h>
#include <cmath>
#ifndef __SYNTHESIS__
#include <iostream>
#else

#endif

const ap_uint<384> a =
    ap_uint<384>("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000fffffffc");
const ap_uint<384> b =
    ap_uint<384>("0xb3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef");
const ap_uint<384> p =
    ap_uint<384>("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff");
const ap_uint<384> Gx =
    ap_uint<384>("0xaa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7");
const ap_uint<384> Gy =
    ap_uint<384>("0x3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f");
const ap_uint<384> n =
    ap_uint<384>("0xffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973");

namespace xf {
namespace security {

template <int WDATA, int WPORT, int WPTR, int HashW>
class nist384p_array {
   public:
    using DType = ap_uint<WDATA>;     // raw data type
    using EType = ap_uint<WPORT + 2>; // element type
    using IType = ap_uint<WPTR>;      // index type
    using RType = ap_uint<WDATA + 2>; // intermediate result type
    static const int DEP = 1024;

    nist384p_array() {
#pragma HLS inline
#pragma HLS bind_storage variable = NA type = RAM_S2P impl = BRAM_ECC
    }

   private:
    EType NA[DEP] = {
        EType("0x00000000fffffffc"), EType("0xffffffff00000000"), EType("0xfffffffffffffffe"),
        EType("0xffffffffffffffff"), EType("0xffffffffffffffff"),
        EType("0xffffffffffffffff"), // a, ptr = 0
        EType("0x2a85c8edd3ec2aef"), EType("0xc656398d8a2ed19d"), EType("0x0314088f5013875a"),
        EType("0x181d9c6efe814112"), EType("0x988e056be3f82d19"),
        EType("0xb3312fa7e23ee7e4"), // b, ptr = 1
        EType("0x00000000ffffffff"), EType("0xffffffff00000000"), EType("0xfffffffffffffffe"),
        EType("0xffffffffffffffff"), EType("0xffffffffffffffff"),
        EType("0xffffffffffffffff"), // p, ptr = 2
        EType("0x3a545e3872760ab7"), EType("0x5502f25dbf55296c"), EType("0x59f741e082542a38"),
        EType("0x6e1d3b628ba79b98"), EType("0x8eb1c71ef320ad74"),
        EType("0xaa87ca22be8b0537"), // Gx, ptr = 3
        EType("0x7a431d7c90ea0e5f"), EType("0x0a60b1ce1d7e819d"), EType("0xe9da3113b5f0b8c0"),
        EType("0xf8f41dbd289a147c"), EType("0x5d9e98bf9292dc29"),
        EType("0x3617de4a96262c6f"), // Gy, ptr = 4
        EType("0xecec196accc52973"), EType("0x581a0db248b0a77a"), EType("0xc7634d81f4372ddf"),
        EType("0xffffffffffffffff"), EType("0xffffffffffffffff"),
        EType("0xffffffffffffffff"), // n, ptr = 5
        EType("0x0000000000000000"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 0, ptr = 6
        EType("0x0000000000000001"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 1, ptr = 7
        EType("0xffffffff00000001"), EType("0x00000000ffffffff"), EType("0x0000000000000001"),
        EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 2^384 - p, ptr = 8
        EType("0x1313e695333ad68d"), EType("0xa7e5f24db74f5885"), EType("0x389cb27e0bc8d220"),
        EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 2^384 - n, ptr = 9
                                     // const params, [0] ~ [9]
                                     // [10], undefined and reserved for projective coordinate lamda = lamda * Z
                                     // [11], undefined and reserved for projective coordinate lamda^2
                                     // [12], undefined and reserved for projective coordinate lamda^3
                                     // [13], undefined and reserved for projective coordinate lamda^2 * X
                                     // [14], undefined and reserved for projective coordinate lamda^3 * Y
    };

    void prt(IType a) {
        RType tmp;
        load_array(tmp, a);
#ifndef __SYNTHESIS__
        std::cout << "new ecdsa value = " << std::hex << tmp << std::endl;
#endif
    }

    void load_array(DType& op, IType ptr) {
#pragma HLS inline
        for (int i = 0; i < WDATA / WPORT; i++) {
#pragma HLS unroll
            op.range(i * WPORT + WPORT - 1, i * WPORT) = NA[ptr * WDATA / WPORT + i];
        }
    }

    void load_array(RType& op, IType ptr) {
#pragma HLS inline
        for (int i = 0; i < WDATA / WPORT - 1; i++) {
#pragma HLS unroll
            op.range(i * WPORT + WPORT - 1, i * WPORT) = NA[ptr * WDATA / WPORT + i];
        }
        op.range(WDATA + 1, WDATA - WPORT) = NA[ptr * WDATA / WPORT + WDATA / WPORT - 1];
    }

    void store_array(DType op, IType ptr) {
#pragma HLS inline
        for (int i = 0; i < WDATA / WPORT; i++) {
#pragma HLS unroll
            NA[ptr * WDATA / WPORT + i] = op.range(i * WPORT + WPORT - 1, i * WPORT);
        }
    }

    void store_array(RType op, IType ptr) {
#pragma HLS inline
        for (int i = 0; i < WDATA / WPORT - 1; i++) {
#pragma HLS unroll
            NA[ptr * WDATA / WPORT + i] = op.range(i * WPORT + WPORT - 1, i * WPORT);
        }
        NA[ptr * WDATA / WPORT + WDATA / WPORT - 1] = op.range(WDATA + 1, WDATA - WPORT);
    }

    void copy_array(DType src, DType des) {
#pragma HLS inline off
        for (int i = 0; i < WDATA / WPORT; i++) {
#pragma HLS unroll
            NA[des * WDATA / WPORT + i] = NA[src * WDATA / WPORT + i];
        }
    }

    ap_uint<1> bit(IType ptr, int k) { return NA[ptr * WDATA / WPORT + k / WPORT][k % WPORT]; }

    void modularInv(IType pa, IType pb, IType pr) {
#pragma HLS inline
        const int st = 82; // 5

        copy_array(pb, st);            // u
        copy_array(pa, st + 1);        // v
        store_array(RType(1), st + 2); // s
        store_array(RType(0), st + 3); // r
        ap_uint<32> k = 0;

        while (lt(st + 1, 6)) {
            if (bit(st, 0) == 0) {
                rightShift(st);
                leftShift(st + 2);
            } else if (bit(st + 1, 0) == 0) {
                rightShift(st + 1);
                leftShift(st + 3);
            } else if (le(st + 1, st)) {
                subMod(st + 1, st, 6, st + 1);
                rightShift(st + 1);
                addMod(st + 2, st + 3, 6, st + 2);
                leftShift(st + 3);
            } else {
                subMod(st, st + 1, 6, st);
                rightShift(st);
                addMod(st + 3, st + 2, 6, st + 3);
                leftShift(st + 2);
            }
            k++;
        }

        if (le(st + 3, pb)) {
            subMod(st + 3, pb, 6, st + 3);
        }
        subMod(pb, st + 3, 6, st + 4); // r2
        k -= WDATA;

        for (int i = 0; i < k; i++) {
            if (bit(st + 4, 0) == 1) {
                addMod(st + 4, pb, 6, st + 4);
            }
            rightShift(st + 4);
        }
        monProduct(st + 4, 7, pb, pr);
    }

    void monProduct(IType pa, IType pb, IType pm, IType pr) {
#pragma HLS inline
        store_array(RType(0), pr);
        ap_uint<1> a0 = bit(pa, 0);
        for (int i = 0; i < WDATA; i++) {
            ap_uint<1> qa = bit(pb, i);
            ap_uint<1> qm = bit(pr, 0) ^ (qa & a0);
            if (qa == 1) {
                addMod(pr, pa, 6, pr);
            }
            if (qm == 1) {
                addMod(pr, pm, 6, pr);
            }
            rightShift(pr);
        }
        if (lt(pr, pm)) {
            subMod(pr, pm, 6, pr);
        }
    }

    void addMod(IType pa, IType pb, IType pc, IType pr) {
#pragma HLS inline
        aop(0, pa, pb, pc, pr);
    }

    void subMod(IType pa, IType pb, IType pc, IType pr) {
#pragma HLS inline
        aop(1, pa, pb, pc, pr);
    }

    bool eq(IType pa, IType pb) {
#pragma HLS inline
        return aop(2, pa, pb, 0, 0);
    }

    bool le(IType pa, IType pb) {
#pragma HLS inline
        return aop(3, pa, pb, 0, 0);
    }

    void leftShift(IType pa) {
#pragma HLS inline
        aop(4, pa, 0, 0, pa);
    }

    void rightShift(IType pa) {
#pragma HLS inline
        aop(5, pa, 0, 0, pa);
    }

    void productMod(IType pa, IType pb, IType pc, IType pr) {
#pragma HLS inline
        aop(6, pa, pb, pc, pr);
    }

    bool lt(IType pa, IType pb) {
#pragma HLS inline
        return aop(7, pa, pb, 0, 0);
    }

    bool aop(ap_uint<3> type, IType pa, IType pb, IType pm, IType pr) {
#pragma HLS inline off
        RType opA, opB, opM, opR;
        bool res = true;
        bool ifstore = true;
        load_array(opA, pa);
        load_array(opB, pb);
        load_array(opM, pm);
        if (type[2] == 0) {
            if (type[1] == 0) {
                ap_uint<WDATA + 1> sum;
#pragma HLS bind_op variable = sum oppp = add impl = dsp
#pragma HLS bind_op variable = sum oppp = sub impl = dsp
                if (type[0] == 0) { // 0, addMod
                    // opR = xf::security::internal::addMod<WDATA + 2>(opA, opB, opM);
                    sum = opA + opB;
                    if (sum >= opM) {
                        sum -= opM;
                    }
                } else { // 1, subMod
                    // opR = xf::security::internal::subMod<WDATA + 2>(opA, opB, opM);
                    if (opA >= opB) {
                        sum = opA - opB;
                    } else {
                        sum = opA + opM;
                        sum -= opB;
                    }
                }
                opR = sum;
            } else {
                if (type[0] == 0) { // 2, eq
                    res = (opA == opB);
                } else { // 3, le
                    res = (opA >= opB);
                }
                ifstore = false;
            }
        } else {
            if (type[1] == 0) {
                if (type[0] == 0) { // 4, left shift
                    opR = opA << 1;
                } else { // 5, right shift
                    opR = opA >> 1;
                }
            } else {
                if (type[0] == 0) { // 6, productMod
                    // opR = xf::security::internal::productMod<WDATA>(opA, opB, opM);
                    ap_uint<WDATA + 1> tmp = 0;
#pragma HLS bind_op variable = tmp op = add impl = dsp
#pragma HLS bind_op variable = tmp op = sub impl = dsp
                    for (int i = WDATA - 1; i >= 0; i--) {
                        tmp <<= 1;
                        if (tmp >= opM) {
                            tmp -= opM;
                        }
                        if (opB[i] == 1) {
                            tmp += opA;
                            if (tmp >= opM) {
                                tmp -= opM;
                            }
                        }
                    }
                    opR = tmp;
                } else { // 7, large than
                    res = (opA > opB);
                    ifstore = false;
                }
            }
        }
        if (ifstore) {
            store_array(opR, pr);
        }
        return res;
    }

    void fromJacobian(IType pX, IType pY, IType pZ, IType rX, IType rY) {
#pragma HLS inline
        const int st = 16; // 3

        if (eq(pZ, 6)) {
            store_array(DType(0), rX);
            store_array(DType(0), rY);
        } else {
            modularInv(pZ, 2, st);             // ZInv, 8
            productMod(st, st, 2, st + 1);     // ZInv_2, 9
            productMod(st + 1, st, 2, st + 2); // ZInv_3, 10
            productMod(pX, st + 1, 2, rX);     // x = X * ZInv_2
            productMod(pY, st + 2, 2, rY);     // y = Y * ZInv_3
        }
    }

    void addJacobian(
        IType pX1, IType pY1, IType pZ1, IType pX2, IType pY2, IType pZ2, IType pX3, IType pY3, IType pZ3) {
#pragma HLS inline
        const int st = 19; // 21

        productMod(pZ1, pZ1, 2, st);        // I1, 11
        productMod(pZ2, pZ2, 2, st + 1);    // I2, 12
        productMod(st, pZ1, 2, st + 2);     // J1, 13
        productMod(st + 1, pZ2, 2, st + 3); // J2, 14
        productMod(pX1, st + 1, 2, st + 4); // U1, 15
        productMod(pX2, st, 2, st + 5);     // U2, 16
        subMod(st + 4, st + 5, 2, st + 6);  // H, 17
        addMod(st + 6, st + 6, 2, st + 7);  // F, 18
        productMod(st + 7, st + 7, 2, st + 7);
        productMod(pY1, st + 3, 2, st + 8);     // K1, 19
        productMod(pY2, st + 2, 2, st + 9);     // K2, 20
        productMod(st + 4, st + 7, 2, st + 10); // V, 21
        productMod(st + 7, st + 6, 2, st + 11); // G, 22
        subMod(st + 8, st + 9, 2, st + 12);     // R, 23
        addMod(st + 12, st + 12, 2, st + 12);

        if (eq(pZ2, 6)) {
            copy_array(pX1, pX3);
            copy_array(pY1, pY3);
            copy_array(pZ1, pZ3);
        } else if (eq(pZ1, 6)) {
            copy_array(pX2, pX3);
            copy_array(pY2, pY3);
            copy_array(pZ2, pZ3);
        } else {
            addMod(st + 8, st + 9, 2, st + 13); // K1 + K2, 24
            if (eq(st + 13, 6)) {
                store_array(DType(1), pX3);
                store_array(DType(1), pY3);
                store_array(DType(0), pZ3);
            } else {
                productMod(st + 12, st + 12, 2, st + 14); // tmpX, 25
                addMod(st + 10, st + 10, 2, st + 15);     // tmp2V, 26
                addMod(st + 14, st + 11, 2, st + 14);
                subMod(st + 14, st + 15, 2, pX3);

                subMod(st + 10, pX3, 2, st + 16); // tmp2, 27
                productMod(st + 16, st + 12, 2, st + 16);
                productMod(st + 8, st + 11, 2, st + 17); // tmp4, 28
                addMod(st + 17, st + 17, 2, st + 17);
                subMod(st + 16, st + 17, 2, pY3);

                addMod(pZ1, pZ2, 2, st + 18); // tmp5, 29
                productMod(st + 18, st + 18, 2, st + 18);
                addMod(st, st + 1, 2, st + 19);       // tmp6, 30
                subMod(st + 18, st + 19, 2, st + 20); // tmp7, 31
                productMod(st + 20, st + 6, 2, pZ3);
            }
        }
    }

    void doubleJacobian(IType pX1, IType pY1, IType pZ1, IType pX2, IType pY2, IType pZ2) {
#pragma HLS inline
        const int st = 40; // 15
        if (eq(pY1, 6)) {
            store_array(DType(1), pX2);
            store_array(DType(1), pY2);
            store_array(DType(0), pZ2);
        } else {
            productMod(pY1, pY1, 2, st);           // ySq, 32
            addMod(st, st, 2, st + 1);             // ySq_2, 33
            addMod(st + 1, st + 1, 2, st + 2);     // ySq_4, 34
            productMod(st + 1, st + 1, 2, st + 3); // yQu_4, 35
            addMod(st + 3, st + 3, 2, st + 4);     // yQu_8, 36

            productMod(st + 2, pX1, 2, st + 5); // S, 37

            productMod(pZ1, pZ1, 2, st + 6);       // zSq, 38
            subMod(pX1, st + 6, 2, st + 7);        // M_op_1, 39
            addMod(pX1, st + 6, 2, st + 8);        // M_op_2, 40
            productMod(st + 7, st + 8, 2, st + 9); // M_base, 41
            addMod(st + 9, st + 9, 2, st + 10);    // M, 42
            addMod(st + 10, st + 9, 2, st + 10);

            productMod(st + 10, st + 10, 2, st + 11); // MSq, 43
            addMod(st + 5, st + 5, 2, st + 12);       // S_2, 44
            subMod(st + 11, st + 12, 2, pX2);

            subMod(st + 5, pX2, 2, st + 13); // tmp, 45;
            productMod(st + 13, st + 10, 2, st + 13);

            productMod(pY1, pZ1, 2, st + 14); // tmp_1, 46
            addMod(st + 14, st + 14, 2, pZ2);
            subMod(st + 13, st + 4, 2, pY2);
        }
    }

    void dotProductJacobianAffine(IType px, IType py, IType pk, IType rx, IType ry, IType rz) {
#pragma HLS inline
        store_array(DType(1), rx);
        store_array(DType(1), ry);
        store_array(DType(0), rz);

        productMod(px, 11, 2, 13);
        productMod(py, 12, 2, 14);

        for (int i = 384 - 1; i >= 0; i--) {
            doubleJacobian(rx, ry, rz, rx, ry, rz);
            if (bit(pk, i) == ap_uint<1>(1)) {
                // addJacobian(rx, ry, rz, px, py, 7, rx, ry, rz);
                addJacobian(rx, ry, rz, 13, 14, 10, rx, ry, rz);
            }
        }
    }

    bool sign(ap_uint<1> isGenPubKey, DType hash, DType k, DType privateKey, DType& r, DType& s) {
#pragma HLS inline
        const int st = 55; // 11

        store_array(hash, st);           // hash(z), 47
        store_array(k, st + 1);          // k, 48
        store_array(privateKey, st + 2); // privateKey, 49
        // x1, 50
        // y1, 51
        // z1, 52
        // x, 53
        // y, 54
        dotProductJacobianAffine(3, 4, st + 1, st + 3, st + 4, st + 5);
        fromJacobian(st + 3, st + 4, st + 5, st + 6, st + 7);

        if (isGenPubKey == 0) {
            if (le(st + 6, 5)) {
                subMod(st + 6, 5, 5, st + 6);
            }

            if (eq(st + 6, 6)) {
                return false;
            } else {
                load_array(r, st + 6);

                if (le(st, 5)) {
                    subMod(st, 5, 5, st);
                }

                if (le(st + 2, 5)) {
                    subMod(st + 2, 5, 5, st + 2);
                }

                modularInv(st + 1, 5, st + 8);         // kInv, 55
                productMod(st + 6, st + 2, 5, st + 9); // rda, 56
                addMod(st + 9, st, 5, st + 9);

                productMod(st + 8, st + 9, 5, st + 10); // tmp_s, 57

                load_array(s, st + 10);

                if (eq(st + 10, 6)) {
                    return false;
                } else {
                    return true;
                }
            }
        } else {
            load_array(r, st + 6);
            load_array(s, st + 7);
            return true;
        }
    }

    bool verify(DType r, DType s, DType hash, DType Px, DType Py) {
#pragma HLS inline
        const int st = 66; // 16

        store_array(r, st);        // r, 58
        store_array(s, st + 1);    // s, 59
        store_array(hash, st + 2); // hash, 60
        store_array(Px, st + 3);   // Px, 61
        store_array(Py, st + 4);   // Py, 62

        if (eq(st, 6) || le(st, 5) || eq(st + 1, 6) || le(st + 1, 5)) {
            return false;
        } else {
            if (le(st + 2, 5)) {
                subMod(st + 2, 5, 5, st + 2);
            }

            modularInv(st + 1, 5, st + 5);         // sInv, 63
            productMod(st + 5, st + 2, 5, st + 6); // u1, 64
            productMod(st + 5, st, 5, st + 7);     // u2, 65

            dotProductJacobianAffine(3, 4, st + 6, st + 8, st + 9, st + 10);
            dotProductJacobianAffine(st + 3, st + 4, st + 7, st + 11, st + 12, st + 13);
            addJacobian(st + 8, st + 9, st + 10, st + 11, st + 12, st + 13, st + 8, st + 9, st + 10);

            fromJacobian(st + 8, st + 9, st + 10, st + 14, st + 15);
            if (eq(st + 14, 6) && eq(st + 15, 6)) {
                return false;
            } else {
                if (eq(st, st + 14)) {
                    return true;
                } else {
                    return false;
                }
            }
        }
    }

   public:
    /*
     * opType: true, process sign; false, verify.
     * hash  : hash of message.
     * op1   : k, for sign; Px, for verify.
     * op2   : privateKey, for sign; Py, for verify.
     * r     : r, of signature.
     * s     : s, of signature.
     */
    /*
     * opType: 0, sign;                 1, verify;               2, generate public key.
     * hash  : hash of message;         hash of message;         not defined
     * op1   : k for sign;              Px for verify;           privateKey for generate pubkey
     * op2   : privateKey for sign;     Py for verify;           not defined
     * r     : r of signature;          r of signature;          Px of pub key
     * s     : s of signature;          s of signature;          Px of pub key
     * lamda : lamda to randomize project coordinate
     */
    bool process(ap_uint<2> opType, DType hash, DType op1, DType op2, DType& r, DType& s, DType lamda) {
        store_array(lamda, 10);
        productMod(10, 10, 2, 11);
        productMod(10, 11, 2, 12);

        if (opType == 1) {
            return verify(r, s, hash, op1, op2);
        } else {
            return sign(opType[1], hash, op1, op2, r, s);
        }
    }
};

} // namespace security
} // namespace xf

#endif
