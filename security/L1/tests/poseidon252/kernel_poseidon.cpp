/*
 * Copyright 2021 Xilinx, Inc.
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

#include "kernel_poseidon.hpp"
#include "xf_security/poseidon.hpp"

extern void kernel_poseidon(int inputLen,
                            hls::stream<ap_uint<256> >& inputWordsStrm,
                            hls::stream<ap_uint<256> >& roundConstantsStrm,
                            hls::stream<ap_uint<256> >& mdsMatrixStrm,
                            ap_uint<256> prime,
                            ap_uint<256>* outputWords) {
    xf::security::poseidon<t, alph, n, M, RF, RP, field, sbox, security_margin>(
        prime, inputLen, inputWordsStrm, roundConstantsStrm, mdsMatrixStrm, outputWords);
}
