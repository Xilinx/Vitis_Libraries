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
#include "uut_top.hpp"

void uut_top(const unsigned int t_chId,
             hls::stream<ap_uint<32> >& p_paramStr,
             hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> >& p_datStr,
             hls::stream<ap_uint<32> >& p_paramFwdStr,
             hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> >& p_datFwdStr,
             hls::stream<ap_uint<32> >& p_paramOutStr,
             hls::stream<ap_uint<SPARSE_dataBits * SPARSE_parEntries> >& p_datOutStr) {
    xf::sparse::dispColVec<SPARSE_maxColParBlocks, SPARSE_hbmChannels, SPARSE_parEntries, SPARSE_dataBits>(
        t_chId, p_paramStr, p_datStr, p_paramFwdStr, p_datFwdStr, p_paramOutStr, p_datOutStr);
}
