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

#ifndef _XF_SECURITY_ECC_HPP_
#define _XF_SECURITY_ECC_HPP_

#include <ap_int.h>
#include "xf_security/modular.hpp"

namespace xf {
namespace security {

template <int W>
class ecc {
   public:
    void add(ap_uint<W> Px, ap_uint<W> Py, ap_uint<W> Qx, ap_uint<W> Qy, ap_uint<W>& Rx, ap_uint<W>& Ry) {
        if (Qx == 0 && Qy == 0) { // Q is zero
            Rx = Px;
            Ry = Py;
        } else if (Px == Qx && (Py + Qy) == p) {
            Rx = 0;
            Ry = 0;
        } else {
            ap_uint<W> lamda, lamda_d;
            if (Px == Qx && Py == Qy) {
                lamda = xf::security::internal::productMod<W>(Px, Px, p);
                lamda = xf::security::internal::productMod<W>(lamda, 3, p);
                lamda = xf::security::internal::addMod<W>(lamda, a, p);
                lamda_d = xf::security::internal::productMod<W>(Py, 2, p);
            } else {
                lamda = xf::security::internal::subMod<W>(Qy, Py, p);
                lamda_d = xf::security::internal::subMod<W>(Qx, Px, p);
            }
            lamda_d = xf::security::internal::modularInv<W>(lamda_d, p);
            lamda = xf::security::internal::productMod<W>(lamda, lamda_d, p);

            ap_uint<W> lamda_sqr = xf::security::internal::productMod<W>(lamda, lamda, p);

            ap_uint<W> resX, resY;
            resX = xf::security::internal::subMod<W>(lamda_sqr, Px, p);
            resX = xf::security::internal::subMod<W>(resX, Qx, p);

            resY = xf::security::internal::subMod<W>(Px, resX, p);
            resY = xf::security::internal::productMod<W>(lamda, resY, p);
            resY = xf::security::internal::subMod<W>(resY, Py, p);

            Rx = resX;
            Ry = resY;
        }
    }

    void dotProduct(ap_uint<W> Px, ap_uint<W> Py, ap_uint<W> k, ap_uint<W>& Rx, ap_uint<W>& Ry) {
        ap_uint<W> resX = 0;
        ap_uint<W> resY = 0;

        for (int i = 0; i < W; i++) {
            if (k[i] == 1) {
                add(Px, Py, resX, resY, resX, resY);
            }
            add(Px, Py, Px, Py, Px, Py);
        }

        Rx = resX;
        Ry = resY;
    }

    void generatePublicKey(ap_uint<W> Gx, ap_uint<W> Gy, ap_uint<W> privateKey, ap_uint<W>& Px, ap_uint<W>& Py) {
        dotProduct(Gx, Gy, privateKey, Px, Py);
    }

    ap_uint<W> a;
    ap_uint<W> b;
    ap_uint<W> p;

    ecc() {
#pragma HLS inline
    }

    void init(ap_uint<W> inputA, ap_uint<W> inputB, ap_uint<W> inputP) {
        a = inputA;
        b = inputB;
        p = inputP;
    }

    void encrypt(ap_uint<W> Gx,
                 ap_uint<W> Gy,
                 ap_uint<W> Px,
                 ap_uint<W> Py,
                 ap_uint<W> randomKey,
                 ap_uint<W> PMx,
                 ap_uint<W> PMy,
                 ap_uint<W>& C1x,
                 ap_uint<W>& C1y,
                 ap_uint<W>& C2x,
                 ap_uint<W>& C2y) {
        dotProduct(Gx, Gy, randomKey, C1x, C1y);
        ap_uint<W> Tx, Ty;
        dotProduct(Px, Py, randomKey, Tx, Ty);
        add(Tx, Ty, PMx, PMy, C2x, C2y);
    }

    void decrypt(ap_uint<W> C1x,
                 ap_uint<W> C1y,
                 ap_uint<W> C2x,
                 ap_uint<W> C2y,
                 ap_uint<W> privateKey,
                 ap_uint<W>& PMx,
                 ap_uint<W>& PMy) {
        ap_uint<W> Tx, Ty;
        dotProduct(C1x, C1y, privateKey, Tx, Ty);
        Ty = p - Ty;
        add(Tx, Ty, C2x, C2y, PMx, PMy);
    }
};
} // namespace security
} // namespace xf

#endif
