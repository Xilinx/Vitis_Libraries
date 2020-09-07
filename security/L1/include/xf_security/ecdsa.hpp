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

#ifndef _XF_SECURITY_ECDSA_HPP_
#define _XF_SECURITY_ECDSA_HPP_

#include <ap_int.h>
#include "xf_security/ecc.hpp"

namespace xf {
namespace security {
/**
 * @brief Elliptic Curve Digital Signature Algorithm on curve secp256k1.
 * This class provide signing and verifying functions.
 *
 * @tparam HashW Bit Width of digest that used for signting and verifying.
 */
template <int HashW>
class ecdsaSecp256k1 : public xf::security::ecc<256> {
   public:
    /// X coordinate of generation point of curve secp256k1.
    ap_uint<256> Gx;
    /// Y coordinate of generation point of curve secp256k1.
    ap_uint<256> Gy;
    /// Order of curve secp256k1.
    ap_uint<256> n;

    ecdsaSecp256k1() {
#pragma HLS inline
    }

    /**
     * @brief Setup parameters for curve y^2 = x^3 + ax + b in GF(p)
     */
    void init() {
        this->a = ap_uint<256>("0x0");
        this->b = ap_uint<256>("0x7");
        this->p = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F");
        Gx = ap_uint<256>("0x79BE667EF9DCBBAC55A06295CE870B07029BFCDB2DCE28D959F2815B16F81798");
        Gy = ap_uint<256>("0x483ADA7726A3C4655DA4FBFC0E1108A8FD17B448A68554199C47D08FFB10D4B8");
        n = ap_uint<256>("0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");
    };

    /**
     * @brief Generate Public Key point Q from private key
     *
     * @param privateKey Private Key.
     * @param Qx X coordinate of point Q.
     * @param Qy Y coordinate of point Q.
     */
    void generatePubKey(ap_uint<256> privateKey, ap_uint<256>& Qx, ap_uint<256>& Qy) {
        this->dotProduct(Gx, Gy, privateKey, Qx, Qy);
    }

    /**
     * @brief signing function.
     * It will return true if input parameters are legal, otherwise return false.
     *
     * @param hash Digest value of message to be signed.
     * @param k A random key to sign the message, should kept different each time to be used.
     * @param privateKey Private Key to sign the message
     * @param r part of signing pair {r, s}
     * @param s part of signing pair {r, s}
     */
    bool sign(ap_uint<HashW> hash, ap_uint<256> k, ap_uint<256> privateKey, ap_uint<256>& r, ap_uint<256>& s) {
        ap_uint<256> x, y;
        this->dotProduct(Gx, Gy, k, x, y); //(x, y) = k * (Gx, Gy);

        if (x >= n) {
            x -= n;
        } // x = x mod n

        if (x == 0) {
            return false;
        } else {
            r = x;

            ap_uint<256> z;
            if (HashW >= 256) {
                z = hash.range(HashW - 1, HashW - 256);
            } else {
                z = hash;
            }
            if (z >= n) {
                z -= n;
            }

            if (privateKey >= n) {
                privateKey -= n;
            }

            ap_uint<256> kInv = xf::security::internal::modularInv<256>(k, n);
            ap_uint<256> rda = xf::security::internal::productMod<256>(x, privateKey, n);
            rda = xf::security::internal::addMod<256>(rda, z, n);

            s = xf::security::internal::productMod<256>(kInv, rda, n);

            if (s == 0) {
                return false;
            } else {
                return true;
            }
        }
    }

    /**
     * @brief verifying function.
     * It will return true if verified, otherwise false.
     *
     * @param r part of signing pair {r, s}
     * @param s part of signing pair {r, s}
     * @param hash Digest value of message to be signed.
     * @param Px X coordinate of public key point P.
     * @param Py Y coordinate of public key point P.
     */
    bool verify(ap_uint<256> r, ap_uint<256> s, ap_uint<HashW> hash, ap_uint<256> Px, ap_uint<256> Py) {
        if (Px == 0 && Py == 0) {
            return false; // return false if public key is zero.
        } else {
            ap_uint<256> tx1 = xf::security::internal::productMod<256>(Px, Px, this->p);
            tx1 = xf::security::internal::productMod<256>(tx1, Px, this->p);

            ap_uint<256> tx2 = xf::security::internal::productMod<256>(Px, this->a, this->p);
            tx2 = xf::security::internal::addMod<256>(tx2, this->b, this->p);

            ap_uint<256> tx3 = xf::security::internal::addMod<256>(tx2, tx1, this->p);

            ap_uint<256> ty = xf::security::internal::productMod<256>(Py, Py, this->p);

            if (ty != tx3) { // return false if public key is not on the curve.
                return false;
            } else {
                ap_uint<256> nPx, nPy;
                this->dotProduct(Px, Py, n, nPx, nPy);

                if (nPx != 0 || nPy != 0) { // return false if public key * n is not zero.
                    return false;
                } else { // public key is valid, begin to check signature
                    if (r == 0 || r >= n || s == 0 || s >= n) {
                        return false;
                    } else {
                        ap_uint<256> z;
                        if (HashW >= 256) {
                            z = hash.range(HashW - 1, HashW - 256);
                        } else {
                            z = hash;
                        }
                        if (z >= n) {
                            z -= n;
                        }

                        ap_uint<256> sInv = xf::security::internal::modularInv<256>(s, n);

                        ap_uint<256> u1 = xf::security::internal::productMod<256>(sInv, z, n);
                        ap_uint<256> u2 = xf::security::internal::productMod<256>(sInv, r, n);

                        ap_uint<256> t1x, t1y, t2x, t2y;
                        this->dotProduct(Gx, Gy, u1, t1x, t1y);
                        this->dotProduct(Px, Py, u2, t2x, t2y);

                        ap_uint<256> x, y;
                        this->add(t1x, t1y, t2x, t2y, x, y);

                        if (x == 0 && y == 0) {
                            return false;
                        } else {
                            if (r == x) {
                                return true;
                            } else {
                                return false;
                            }
                        }
                    }
                }
            }
        }
    }
};

} // namespace security
} // namespace xf

#endif
