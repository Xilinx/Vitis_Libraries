#ifndef _XF_SECURITY_ECDSA_SECP256K1_LOW_RESOURCE_HPP_
#define _XF_SECURITY_ECDSA_SECP256K1_LOW_RESOURCE_HPP_

#include <ap_int.h>
#include <cmath>
#ifndef __SYNTHESIS__
#include <iostream>
#else

#endif

const ap_uint<256> a = ap_uint<256>("0x0");
const ap_uint<256> b = ap_uint<256>("0x7");
const ap_uint<256> p = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
const ap_uint<256> Gx = ap_uint<256>("0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
const ap_uint<256> Gy = ap_uint<256>("0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8");
const ap_uint<256> n = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");

namespace xf {
namespace security {

template <int WDATA, int WPORT, int WPTR, int HashW>
class ecdsaSecp256k1 {
   public:
    using DType = ap_uint<WDATA>;     // raw data type
    using EType = ap_uint<WPORT + 2>; // element type
    using IType = ap_uint<WPTR>;      // index type
    using RType = ap_uint<WDATA + 2>; // intermediate result type
    static const int DEP = 512;

    ecdsaSecp256k1() {
#pragma HLS inline
#pragma HLS bind_storage variable = NA type = RAM_S2P impl = BRAM_ECC
    }

   private:
    // st 0-14
    EType NA[DEP] = {
        EType("0x0000000000000000"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // a, ptr = 0
        EType("0x0000000000000007"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // b, ptr = 1
        EType("0xFFFFFFFEFFFFFC2F"), EType("0xFFFFFFFFFFFFFFFF"), EType("0xFFFFFFFFFFFFFFFF"),
        EType("0xFFFFFFFFFFFFFFFF"), // p, ptr = 2
        EType("0x59F2815B16F81798"), EType("0x029BFCDB2DCE28D9"), EType("0x55A06295CE870B07"),
        EType("0x79BE667EF9DCBBAC"), // Gx, ptr = 3
        EType("0x9C47D08FFB10D4B8"), EType("0xFD17B448A6855419"), EType("0x5DA4FBFC0E1108A8"),
        EType("0x483ADA7726A3C465"), // Gy, ptr = 4
        EType("0xBFD25E8CD0364141"), EType("0xBAAEDCE6AF48A03B"), EType("0xFFFFFFFFFFFFFFFE"),
        EType("0xFFFFFFFFFFFFFFFF"), // n, ptr = 5
        EType("0x0000000000000000"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 0, ptr = 6
        EType("0x0000000000000001"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 1, ptr = 7
        EType("0x00000001000003D1"), EType("0x0000000000000000"), EType("0x0000000000000000"),
        EType("0x0000000000000000"), // 2^256 - p, ptr = 8
        EType("0x402DA1732FC9BEBF"), EType("0x4551231950B75FC4"), EType("0x0000000000000001"),
        EType("0x0000000000000000"), // 2^256 - n, ptr = 9
                                     // const params, [0] ~ [9]
                                     // [10], undefined and reserved for projective coordinate lamda = lamda * Z
                                     // [11], undefined and reserved for projective coordinate lamda^2
                                     // [12], undefined and reserved for projective coordinate lamda^3
                                     // [13], undefined and reserved for projective coordinate lamda^2 * X
                                     // [14], undefined and reserved for projective coordinate lamda^3 * Y
    };

    void prt(IType ptr) {
        RType tmp;
        load_array(tmp, ptr);
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

    // st 94-98
    void modularInv(IType pa, IType pb, IType pr) {
#pragma HLS inline
        const int st = 94; // 5

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

    /**
     * @brief: aop--all operations
     * type: 000-addMod;    001-subMod;     010-eq;         011-le;
     *       100-leftShift; 101-rightShift; 110-ProductMod; 111-lt;
    */
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
#pragma HLS bind_op variable = sum op = add impl = dsp
#pragma HLS bind_op variable = sum op = sub impl = dsp
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
                    if (pm == 5) {  // productMod_n
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
                    } else { // productMod_p
                        ap_uint<128> aH = opA.range(255, 128);
                        ap_uint<128> aL = opA.range(127, 0);
                        ap_uint<128> bH = opB.range(255, 128);
                        ap_uint<128> bL = opB.range(127, 0);

                        ap_uint<256> aLbH = aL * bH;
                        ap_uint<256> aHbL = aH * bL;
                        ap_uint<512> aHbH = aH * bH;
                        ap_uint<256> aLbL = aL * bL;
                        ap_uint<512> mid = aLbH + aHbL;

                        ap_uint<512> mul = (aHbH << 256) + (mid << 128) + aLbL;
                        ap_uint<256> c0 = mul.range(255, 0);
                        ap_uint<256> c1 = mul.range(511, 256);
                        ap_uint<256> w1 = 0;
                        ap_uint<256> w2 = 0;
                        ap_uint<256> w3 = 0;
                        ap_uint<256> w4 = 0;
                        ap_uint<256> w5 = 0;
                        ap_uint<256> w6 = 0;

                        w1.range(255, 32) = c1.range(223, 0);
                        w2.range(255, 9) = c1.range(246, 0);
                        w3.range(255, 8) = c1.range(247, 0);
                        w4.range(255, 7) = c1.range(248, 0);
                        w5.range(255, 6) = c1.range(249, 0);
                        w6.range(255, 4) = c1.range(251, 0);

                        ap_uint<256> s1 = c1.range(255, 252) + c1.range(255, 250) + c1.range(255, 249) +
                                          c1.range(255, 248) + c1.range(255, 247) + c1.range(255, 224);
                        ap_uint<256> k11 = (s1 << 2) + (s1 << 1) + s1;
                        ap_uint<256> k = (s1 << 32) + (k11 << 7) + (s1 << 6) + (s1 << 4) + s1;

                        ap_uint<257> sum = 0;
                        sum = k + c0;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += w1;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += w2;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += w3;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += w4;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += w5;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += w6;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        sum += c1;
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        if (sum >= opM) {
                            sum -= opM;
                        }
                        opR = sum;
                    }
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

    // st 16-18
    void fromJacobian(IType pX, IType pY, IType pZ, IType rX, IType rY) {
#pragma HLS inline
        const int st = 16; // 16

        if (eq(pZ, 6)) {
            store_array(DType(0), rX);
            store_array(DType(0), rY);
        } else {
            modularInv(pZ, 2, st);             // ZInv, 16
            productMod(st, st, 2, st + 1);     // ZInv_2, 17
            productMod(st + 1, st, 2, st + 2); // ZInv_3, 18
            productMod(pX, st + 1, 2, rX);     // x = X * ZInv_2
            productMod(pY, st + 2, 2, rY);     // y = Y * ZInv_3
        }
    }

    // st 19-39
    void addJacobian(
        IType pX1, IType pY1, IType pZ1, IType pX2, IType pY2, IType pZ2, IType pX3, IType pY3, IType pZ3) {
#pragma HLS inline
        const int st = 19; // 19

        productMod(pZ1, pZ1, 2, st);        // I1, 19
        productMod(pZ2, pZ2, 2, st + 1);    // I2, 20
        productMod(st, pZ1, 2, st + 2);     // J1, 21
        productMod(st + 1, pZ2, 2, st + 3); // J2, 22
        productMod(pX1, st + 1, 2, st + 4); // U1, 23
        productMod(pX2, st, 2, st + 5);     // U2, 24
        subMod(st + 4, st + 5, 2, st + 6);  // H, 25
        addMod(st + 6, st + 6, 2, st + 7);  // F, 26
        productMod(st + 7, st + 7, 2, st + 7);
        productMod(pY1, st + 3, 2, st + 8);     // K1, 27
        productMod(pY2, st + 2, 2, st + 9);     // K2, 28
        productMod(st + 4, st + 7, 2, st + 10); // V, 29
        productMod(st + 7, st + 6, 2, st + 11); // G, 30
        subMod(st + 8, st + 9, 2, st + 12);     // R, 31
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
            addMod(st + 8, st + 9, 2, st + 13); // K1 + K2, 32
            if (eq(st + 13, 6)) {
                store_array(DType(1), pX3);
                store_array(DType(1), pY3);
                store_array(DType(0), pZ3);
            } else {
                productMod(st + 12, st + 12, 2, st + 14); // tmpX, 33
                addMod(st + 10, st + 10, 2, st + 15);     // tmp2V, 34
                addMod(st + 14, st + 11, 2, st + 14);
                subMod(st + 14, st + 15, 2, pX3);

                subMod(st + 10, pX3, 2, st + 16); // tmp2, 35
                productMod(st + 16, st + 12, 2, st + 16);
                productMod(st + 8, st + 11, 2, st + 17); // tmp4, 36
                addMod(st + 17, st + 17, 2, st + 17);
                subMod(st + 16, st + 17, 2, pY3);

                addMod(pZ1, pZ2, 2, st + 18); // tmp5, 37
                productMod(st + 18, st + 18, 2, st + 18);
                addMod(st, st + 1, 2, st + 19);       // tmp6, 38
                subMod(st + 18, st + 19, 2, st + 20); // tmp7, 39
                productMod(st + 20, st + 6, 2, pZ3);
            }
        }
    }

    // st 40-66
    void doubleJacobian(IType pX1, IType pY1, IType pZ1, IType pX2, IType pY2, IType pZ2) {
#pragma HLS inline
        const int st = 40;                     // 40
        productMod(pZ1, pZ1, 2, st);           // N 40
        productMod(pY1, pY1, 2, st + 1);       // E  41
        productMod(pX1, pX1, 2, st + 2);       // B 42
        productMod(st + 1, st + 1, 2, st + 3); // L 43

        addMod(pX1, st + 1, 2, st + 4);        // tmp1  44
        productMod(st + 4, st + 4, 2, st + 5); // tmp1 =^2 45
        addMod(st + 2, st + 3, 2, st + 6);     // tmp2  46
        subMod(st + 5, st + 6, 2, st + 7);     // tmp3 47
        addMod(st + 7, st + 7, 2, st + 8);     // S  48

        productMod(st, st, 2, st + 9);        // tmp4 49
        productMod(st + 9, 0, 2, st + 10);    // tmp4 50
        addMod(st + 2, st + 2, 2, st + 11);   // tmp5 51
        addMod(st + 11, st + 2, 2, st + 12);  // tmp5 52
        addMod(st + 12, st + 10, 2, st + 13); // M 53

        addMod(st + 8, st + 8, 2, st + 14);       // tmp6 54
        productMod(st + 13, st + 13, 2, st + 15); // tmp7 55
        subMod(st + 15, st + 14, 2, st + 16);     // X2 56

        subMod(st + 8, st + 16, 2, st + 17);      // tmp8 57
        productMod(st + 17, st + 13, 2, st + 18); // tmp8 58
        addMod(st + 3, st + 3, 2, st + 19);       // tmp9 59
        addMod(st + 19, st + 19, 2, st + 20);     // tmp9 60
        addMod(st + 20, st + 20, 2, st + 21);     // tmp9 61
        subMod(st + 18, st + 21, 2, st + 22);     // Y2 62

        addMod(pY1, pZ1, 2, st + 23);             // tmp10 63
        productMod(st + 23, st + 23, 2, st + 24); // tmp10 64
        addMod(st + 1, st, 2, st + 25);           // tmp11 65

        subMod(st + 24, st + 25, 2, st + 26); // Z2 66
        copy_array(st + 16, pX2);
        copy_array(st + 22, pY2);
        copy_array(st + 26, pZ2);
    }

    void dotProductJacobianAffine(IType px, IType py, IType pk, IType rx, IType ry, IType rz) {
#pragma HLS inline
        store_array(DType(1), rx);
        store_array(DType(1), ry);
        store_array(DType(0), rz);

        productMod(px, 11, 2, 13);
        productMod(py, 12, 2, 14);

        for (int i = 256 - 1; i >= 0; i--) {
            doubleJacobian(rx, ry, rz, rx, ry, rz);
            if (bit(pk, i) == ap_uint<1>(1)) {
                // addJacobian(rx, ry, rz, px, py, 7, rx, ry, rz);
                addJacobian(rx, ry, rz, 13, 14, 10, rx, ry, rz);
            }
        }
    }

    // st 67-77
    bool sign(ap_uint<1> isGenPubKey, DType hash, DType k, DType privateKey, DType& r, DType& s) {
#pragma HLS inline
        const int st = 67; // 67

        store_array(hash, st);           // hash(z), 67
        store_array(k, st + 1);          // k, 68
        store_array(privateKey, st + 2); // privateKey, 69
        // x1, 70
        // y1, 71
        // z1, 72
        // x, 73
        // y, 74
        dotProductJacobianAffine(3, 4, st + 1, st + 3, st + 4, st + 5);
        fromJacobian(st + 3, st + 4, st + 5, st + 6, st + 7);

        if (isGenPubKey == 0) { // sign
            if (le(st + 6, 5)) {
                subMod(st + 6, 5, 5, st + 6); //(xp - n ) mod n
            }

            if (eq(st + 6, 6)) { // r=0
                return false;
            } else {
                load_array(r, st + 6);

                if (le(st, 5)) {
                    subMod(st, 5, 5, st); //(hash -n) mod n
                }

                if (le(st + 2, 5)) {
                    subMod(st + 2, 5, 5, st + 2); //(key - n) mod n
                }

                modularInv(st + 1, 5, st + 8);         // kInv, 75
                productMod(st + 6, st + 2, 5, st + 9); // rda, 76

                addMod(st + 9, st, 5, st + 9);

                productMod(st + 8, st + 9, 5, st + 10); // tmp_s, 77

                load_array(s, st + 10);

                if (eq(st + 10, 6)) {
                    return false;
                } else {
                    return true;
                }
            }
        } else { // generate public key
            load_array(r, st + 6);
            load_array(s, st + 7);
            return true;
        }
    }

    // st 78-93
    bool verify(DType r, DType s, DType hash, DType Px, DType Py) {
#pragma HLS inline
        const int st = 78; // 78

        store_array(r, st);        // r, 78
        store_array(s, st + 1);    // s, 79
        store_array(hash, st + 2); // hash, 80
        store_array(Px, st + 3);   // Px, 81
        store_array(Py, st + 4);   // Py, 82

        if (eq(st, 6) || le(st, 5) || eq(st + 1, 6) || le(st + 1, 5)) {
            return false;
        } else {
            if (le(st + 2, 5)) {
                subMod(st + 2, 5, 5, st + 2);
            }

            modularInv(st + 1, 5, st + 5);         // sInv, 83
            productMod(st + 5, st + 2, 5, st + 6); // u1, 84
            productMod(st + 5, st, 5, st + 7);     // u2, 85

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
        store_array(lamda, 10);    // lamda
        productMod(10, 10, 2, 11); // lamda^2
        productMod(10, 11, 2, 12); // lamda^3

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
