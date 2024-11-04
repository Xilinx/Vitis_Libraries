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

#include <ap_int.h>
#include "xf_database/compaction_core/core.hpp"
#include "xf_database/compaction_core/config.hpp"

namespace xf {
namespace database {
namespace lsm {

extern "C" void CUCoreLoopTop(
    // ap_uint<8> in_streamID_tag,
    unsigned char in_streamID_tag,
    ap_uint<BufWidth_Set_>* in_dataBuff,
    ap_uint<32>* in_metaInfo,

    // ap_uint<8> base_streamID_tag,
    unsigned char base_streamID_tag,
    ap_uint<BufWidth_Set_>* base_dataBuff,
    ap_uint<32>* base_metaInfo,

    hls::burst_maxi<ap_uint<BufWidth_Set_> > out_dataBuff,
    hls::burst_maxi<ap_uint<32> > out_metaInfo,

    unsigned int in_dataBurst_len,
    unsigned int in_metaBurst_len,
    unsigned int base_dataBurst_len,
    unsigned int base_metaBurst_len) {
    enum { bufDepth_ = 1024 * 64 * IncreaseFactor_, metaDepth_ = 1024 * 64 * IncreaseFactor_ };

#pragma HLS INTERFACE s_axilite port = in_streamID_tag bundle = control
#pragma HLS INTERFACE m_axi port = in_dataBuff depth = bufDepth_ bundle = gmem0_0 num_read_outstanding = \
    16 max_read_burst_length = 64
#pragma HLS INTERFACE m_axi port = in_metaInfo depth = metaDepth_ bundle = gmem0_1 num_read_outstanding = \
    16 max_read_burst_length = 64
#pragma HLS INTERFACE s_axilite port = base_streamID_tag bundle = control
#pragma HLS INTERFACE m_axi port = base_dataBuff depth = bufDepth_ bundle = gmem0_2 num_read_outstanding = \
    16 max_read_burst_length = 64
#pragma HLS INTERFACE m_axi port = base_metaInfo depth = metaDepth_ bundle = gmem0_3 num_read_outstanding = \
    16 max_read_burst_length = 64
#pragma HLS INTERFACE m_axi port = out_dataBuff depth = bufDepth_* 2 bundle = gmem1_0 num_write_outstanding = \
    16 max_write_burst_length = 64
#pragma HLS INTERFACE m_axi port = out_metaInfo depth = metaDepth_* 2 bundle = gmem1_1 num_write_outstanding = \
    16 max_write_burst_length = 64
#pragma HLS INTERFACE s_axilite port = in_dataBurst_len bundle = control
#pragma HLS INTERFACE s_axilite port = in_metaBurst_len bundle = control
#pragma HLS INTERFACE s_axilite port = base_dataBurst_len bundle = control
#pragma HLS INTERFACE s_axilite port = base_metaBurst_len bundle = control
#pragma HLS INTERFACE s_axilite port = return bundle = control

#ifndef __SYNTHESIS__
    printf("Kernerl start\n");
#endif

    xf::database::lsm::CUCore<BufWidth_Set_> core;

#ifndef __SYNTHESIS__
    printf("Core Loop Called %d %d %d %d %d %d\n", in_streamID_tag, base_streamID_tag, in_dataBurst_len,
           in_metaBurst_len, base_dataBurst_len, base_metaBurst_len);
#endif

#ifndef __SYNTHESIS__
    printf("Core Loop Called2 %x %x %x %x %x %x\n", in_dataBuff, in_metaInfo, base_dataBuff, base_metaInfo,
           out_dataBuff, out_metaInfo);
#endif

    core.CUCoreLoop(in_streamID_tag, in_dataBuff, in_metaInfo, in_dataBurst_len, in_metaBurst_len, base_streamID_tag,
                    base_dataBuff, base_metaInfo, base_dataBurst_len, base_metaBurst_len, out_dataBuff, out_metaInfo);
}

} // namespace lsm
} // namespace database
} // namespace xf
