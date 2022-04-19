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
#pragma once

#include <ap_int.h>
#include <hls_stream.h>
#include "xf_database/compaction_core/utils.hpp"
#include "xf_database/compaction_core/comparison.hpp"
#include "xf_database/compaction_core/config.hpp"

namespace xf {
namespace database {
namespace lsm {

template <unsigned int BufWidth_>
class CUCore {
   private:
    // One key decoder / stream write / stream read for data IO
    FPGA_utils<BufWidth_> decoder_;
    FPGA_utils<BufWidth_> decoder_base_;
    // Two comparison pipelines feeding each other
    KeyStreamCompareV2<BufWidth_> comparator_;

   public:
    void CUCoreLoop(
        // ap_uint<8> in_streamID_tag,
        unsigned char in_streamID_tag_,
        ap_uint<BufWidth_>* in_dataBuff,
        ap_uint<32>* in_metaInfo,
        unsigned int in_dataBurst_len,
        unsigned int in_metaBurst_len,

        // ap_uint<8> base_streamID_tag,
        unsigned char base_streamID_tag_,
        ap_uint<BufWidth_>* base_dataBuff,
        ap_uint<32>* base_metaInfo,
        unsigned int base_dataBurst_len,
        unsigned int base_metaBurst_len,

        hls::burst_maxi<ap_uint<BufWidth_> >& out_dataBuf,
        hls::burst_maxi<ap_uint<32> >& out_metaInfo) {
#ifndef __SYNTHESIS__
        printf("Inside Core Loop\n");
#endif
#pragma HLS DATAFLOW

        // Input / Output / Buffer streams
        hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_> _keyStream_new("keyStream_new");
        hls::stream<ap_uint<32>, StreamDepth_> _keyCnt_new("keyCnt_new");
        hls::stream<ap_uint<8>, StreamDepth_> _streamID_new("streamID_new");
#pragma HLS aggregate variable = _keyStream_new

        hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_> _keyStream_base("keyStream_base");
        hls::stream<ap_uint<32>, StreamDepth_> _keyCnt_base("keyCnt_base");
        hls::stream<ap_uint<8>, StreamDepth_> _streamID_base("streamID_base");
#pragma HLS aggregate variable = _keyStream_base

        hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_> _keyStream_next("keyStream_next");
        hls::stream<ap_uint<32>, StreamDepth_> _keyCnt_next("keyCnt_next");
        hls::stream<ap_uint<32>, StreamDepth_> _streamID_next("streamID_next");
#pragma HLS aggregate variable = _keyStream_next

        hls::stream<ap_uint<BufWidth_>, StreamDepth_> _new_dataBuff_stream("new_dataBuff_stream");
        hls::stream<ap_uint<BufWidth_>, StreamDepth_> _base_dataBuff_stream("base_dataBuff_stream");
        hls::stream<ap_uint<32>, StreamDepth_> _new_metaInfo_stream("new_metaInfo_stream");
        hls::stream<ap_uint<32>, StreamDepth_> _base_metaInfo_stream("base_metaInfo_stream");
        hls::stream<ap_uint<BufWidth_>, BufDepth_ * 2> _out_dataStream("out_dataStream");
        hls::stream<ap_uint<7>, 2> _out_dataStream_status("out_dataStream_status");
        hls::stream<ap_uint<32>, MetaDepth_ * 2> _out_metaStream("out_metaStream");
        hls::stream<ap_uint<7>, 2> _out_metaStream_status("out_metaStream_status");

        // assigning from wider to narrower takes MSB
        ap_uint<8> in_streamID_tag = in_streamID_tag_;
        ap_uint<8> base_streamID_tag = base_streamID_tag_;

        decoder_.DataBurstRead(in_dataBurst_len, in_dataBuff, _new_dataBuff_stream);
        decoder_.MetaBurstRead(in_metaBurst_len, in_metaInfo, _new_metaInfo_stream);

        decoder_.KeyDecode(in_streamID_tag, _new_dataBuff_stream, _new_metaInfo_stream, _keyStream_new, _keyCnt_new,
                           _streamID_new, in_dataBurst_len, in_metaBurst_len);

        decoder_base_.DataBurstRead(base_dataBurst_len, base_dataBuff, _base_dataBuff_stream);
        decoder_base_.MetaBurstRead(base_metaBurst_len, base_metaInfo, _base_metaInfo_stream);

        decoder_base_.KeyDecode(base_streamID_tag, _base_dataBuff_stream, _base_metaInfo_stream, _keyStream_base,
                                _keyCnt_base, _streamID_base, base_dataBurst_len, base_metaBurst_len);

        comparator_.KeyCompare(_keyStream_new, _keyCnt_new, _streamID_new,

                               _keyStream_base, _keyCnt_base, _streamID_base,

                               _keyStream_next, _keyCnt_next, _streamID_next);

        decoder_.StreamWrite(_keyStream_next, _keyCnt_next, _streamID_next, _out_dataStream, _out_dataStream_status,
                             _out_metaStream, _out_metaStream_status);

        Writer<BufWidth_Set_>(out_dataBuf, _out_dataStream, _out_dataStream_status);
        Writer<32>(out_metaInfo, _out_metaStream, _out_metaStream_status);
    }
};

} // namespace lsm
} // namespace database
} // namespace xf
