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
#include <string>

namespace xf {
namespace database {
namespace lsm {

template <unsigned int BufWidth_>
class KeyStreamCompareV2 {
   private:
    enum ingress_state : unsigned char { SRC_STREAM, SRC_TMP, SRC_EMPTY };

    enum compare_state : unsigned char {
        ST_IDLE,
        ST_NEW,    // comparator consuming remaining new stream token
        ST_BASE,   // comparator consuming remaining base stream token
        ST_COMPARE // comparator actively doing comparison
    };

#ifndef __SYNTHESIS__
#if KernelDebug_
    void printID(ap_uint<8> in) { printf("StreamID %d\n", in); }
#endif
#endif

    void MetaUpdate(ap_uint<8>& curr_streamID_base,
                    ap_uint<8>& curr_token_cnt,
                    hls::stream<ap_uint<32>, StreamDepth_>& out_streamID_next,
                    hls::stream<ap_uint<8>, StreamDepth_>& in_streamID_base,
                    unsigned int& keyRead_base,
                    bool keyReadBase_UpdateVld) {
#pragma HLS inline
        ap_uint<32> curr_ID;
        curr_ID.range(7, 0) = curr_streamID_base;
        curr_ID.range(23, 16) = curr_token_cnt;
        curr_token_cnt = 0;
        curr_ID.range(31, 24) = 0;
        curr_ID.range(15, 8) = 0;
        out_streamID_next.write(curr_ID);
        if (keyReadBase_UpdateVld) curr_streamID_base = in_streamID_base.read();
        keyRead_base++;
    }

    void CompareUpdate(compare_state& comparator_, bool& keyReadNew_vld, bool& keyReadBase_vld) {
#pragma HLS inline
        if (keyReadNew_vld && keyReadBase_vld)
            comparator_ = ST_COMPARE;
        else if (!keyReadBase_vld && keyReadNew_vld) {
            comparator_ = ST_NEW;
        } else if (keyReadBase_vld && !keyReadNew_vld) {
            comparator_ = ST_BASE;
        } else {
            comparator_ = ST_IDLE;
        }
    }

    void LastTokenCheck(ingress_state& new_state, bool keyReadNew_vld) {
#pragma HLS inline
        new_state = (keyReadNew_vld) ? SRC_STREAM : SRC_EMPTY;
    }

    void TmpTokenCheck(ingress_state& new_state, unsigned int rd_ptr, bool rd_ptr_UpdateVld) {
#pragma HLS inline
        new_state = rd_ptr_UpdateVld ? SRC_STREAM : SRC_TMP;
        rd_ptr++;
    }

   public:
    void KeyCompare(hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_>& in_keyStream_new,
                    hls::stream<ap_uint<32>, StreamDepth_>& in_keyCnt_new,
                    hls::stream<ap_uint<8>, StreamDepth_>& in_streamID_new,

                    hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_>& in_keyStream_base,
                    hls::stream<ap_uint<32>, StreamDepth_>& in_keyCnt_base,
                    hls::stream<ap_uint<8>, StreamDepth_>& in_streamID_base,

                    hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_>& out_keyStream_next,
                    hls::stream<ap_uint<32>, StreamDepth_>& out_keyCnt_next,
                    hls::stream<ap_uint<32>, StreamDepth_>& out_streamID_next) {
#ifndef __SYNTHESIS__
        std::unique_ptr<keyDecodeWord<BufWidth_>[]> tmp_uni_ptr(new keyDecodeWord<BufWidth_>[ 16 ]);
        keyDecodeWord<BufWidth_>(&tmp_)[16] = *reinterpret_cast<keyDecodeWord<BufWidth_>(*)[16]>(tmp_uni_ptr.get());
#else
        keyDecodeWord<BufWidth_> tmp_[16];
#pragma HLS bind_storage variable = tmp_ type = RAM_2P
#endif
        unsigned char wr_ptr = 0;
        unsigned char wr_ptr_next = 0;
        unsigned char rd_ptr = 0;

        unsigned int keyCnt_new = in_keyCnt_new.read();
        unsigned int keyCnt_base = in_keyCnt_base.read();
        out_keyCnt_next.write(keyCnt_new + keyCnt_base);

        unsigned int keyRead_new = 0;
        unsigned int keyRead_base = 0;
        ingress_state new_state = (keyCnt_new != 0) ? SRC_STREAM : SRC_EMPTY;
        ingress_state base_state = (keyCnt_base != 0) ? SRC_STREAM : SRC_EMPTY;

        keyDecodeWord<BufWidth_> new_token;
        ap_uint<8> curr_streamID_new;
        if (new_state == SRC_STREAM) {
            new_token = in_keyStream_new.read();
            curr_streamID_new = in_streamID_new.read();
        } else {
        }

        keyDecodeWord<BufWidth_> base_token;
        ap_uint<8> curr_streamID_base;
        if (base_state == SRC_STREAM) {
            base_token = in_keyStream_base.read();
            curr_streamID_base = in_streamID_base.read();
        } else {
        }

        compare_state comparator_;
        if (keyCnt_new != 0 && keyCnt_base != 0)
            comparator_ = ST_COMPARE;
        else if (keyCnt_new == 0 && keyCnt_base != 0)
            comparator_ = ST_BASE;
        else if (keyCnt_new != 0 && keyCnt_base == 0)
            comparator_ = ST_NEW;
        else
            comparator_ = ST_IDLE;

        ap_uint<8> curr_token_cnt = 0;
    compareMain:
        while ((keyRead_new < keyCnt_new) || (keyRead_base < keyCnt_base)) {
#pragma HLS dependence variable = tmp_ intra false
#pragma HLS pipeline II = 1

            bool comparator_iscompare = comparator_ == ST_COMPARE;
            bool comparator_isnew = comparator_ == ST_NEW;
            bool comparator_isbase = comparator_ == ST_BASE;
            bool keyReadNew_Vld = keyRead_new < keyCnt_new;
            bool keyReadBase_Vld = keyRead_base < keyCnt_base;
            bool keyReadNew_UpdateVld = keyRead_new < (keyCnt_new - 1);
            bool keyReadBase_UpdateVld = keyRead_base < (keyCnt_base - 1);
            bool newState_isStream = new_state == SRC_STREAM;
            bool newState_isTmp = new_state == SRC_TMP;
            bool newState_isEmpty = new_state == SRC_EMPTY;
            bool baseState_isStream = base_state == SRC_STREAM;
            bool baseState_isTmp = base_state == SRC_TMP;
            bool baseState_isEmpty = base_state == SRC_EMPTY;
            bool newToken_small = new_token.keyWord < base_token.keyWord;
            bool baseToken_small = base_token.keyWord < new_token.keyWord;
            bool rd_ptr_UpdateVld = rd_ptr == (wr_ptr - 1);
            bool wr_ptr_zero = wr_ptr == 0;
            bool wr_ptr_gt_one = wr_ptr > 1;

            curr_token_cnt++;
            if (comparator_iscompare) {
                if (newToken_small ||
                    (new_token.keyEnd && !base_token.keyEnd && new_token.keyWord == base_token.keyWord)) {
                    out_keyStream_next.write(new_token);
                    new_token.printDetail();
                    if (newState_isStream && baseState_isStream) {
                        wr_ptr_next = wr_ptr + 1;
                        tmp_[wr_ptr] = (base_token);
                        if (new_token.keyEnd) {
                            CompareUpdate(comparator_, keyReadNew_UpdateVld, keyReadBase_Vld);
                            LastTokenCheck(new_state, keyReadNew_UpdateVld);
                            MetaUpdate(curr_streamID_new, curr_token_cnt, out_streamID_next, in_streamID_new,
                                       keyRead_new, keyReadNew_UpdateVld);
                            base_state = wr_ptr_zero ? SRC_EMPTY : SRC_TMP;
                        } else {
                            comparator_ = ST_NEW;
                            base_state = SRC_EMPTY;
                        }
                        wr_ptr++;
                    } else if ((newState_isStream && baseState_isTmp) || (newState_isStream && baseState_isEmpty)) {
                        rd_ptr = 0; // reset tmp read ptr
                        if (new_token.keyEnd) {
                            CompareUpdate(comparator_, keyReadNew_UpdateVld, keyReadBase_Vld);
                            LastTokenCheck(new_state, keyReadNew_UpdateVld);
                            MetaUpdate(curr_streamID_new, curr_token_cnt, out_streamID_next, in_streamID_new,
                                       keyRead_new, keyReadNew_UpdateVld);
                        } else {
                            comparator_ = ST_NEW;
                            base_state = SRC_EMPTY;
                        }
                    } else { // new_state == SRC_TMP && base_state == SRC_STREAM
                             // new_state == SRC_EMPTY && base_state == SRC_STREAM
                        tmp_[rd_ptr] = base_token;
                        wr_ptr_next = rd_ptr + 1; // write address for the next token
                        if (new_token.keyEnd) {   // last element on tmp array.
                            CompareUpdate(comparator_, keyReadNew_UpdateVld, keyReadBase_Vld);
                            LastTokenCheck(new_state, keyReadNew_UpdateVld);
                            MetaUpdate(curr_streamID_new, curr_token_cnt, out_streamID_next, in_streamID_new,
                                       keyRead_new, keyReadNew_UpdateVld);
                            base_state = (wr_ptr_gt_one) ? SRC_TMP : SRC_EMPTY;
                            wr_ptr = rd_ptr + 1;
                            rd_ptr = 0;
                        } else {
                            comparator_ = ST_NEW;
                            base_state = SRC_EMPTY;
                            TmpTokenCheck(new_state, rd_ptr, rd_ptr_UpdateVld);
                        }
                    }
                } else if (baseToken_small ||
                           (base_token.keyEnd && !new_token.keyEnd && new_token.keyWord == base_token.keyWord)) {
                    out_keyStream_next.write(base_token);
                    base_token.printDetail();
                    if (newState_isStream && baseState_isStream) {
                        wr_ptr_next = wr_ptr + 1;
                        tmp_[wr_ptr] = (new_token);
                        if (base_token.keyEnd) {
                            CompareUpdate(comparator_, keyReadNew_Vld, keyReadBase_UpdateVld);
                            LastTokenCheck(base_state, keyReadBase_UpdateVld);
                            MetaUpdate(curr_streamID_base, curr_token_cnt, out_streamID_next, in_streamID_base,
                                       keyRead_base, keyReadBase_UpdateVld);
                            new_state = (wr_ptr_zero) ? SRC_EMPTY : SRC_TMP;
                        } else {
                            comparator_ = ST_BASE;
                            new_state = SRC_EMPTY;
                        }
                        wr_ptr++;
                    } else if ((newState_isTmp && baseState_isStream) || (newState_isEmpty && baseState_isStream)) {
                        rd_ptr = 0;
                        if (base_token.keyEnd) {
                            CompareUpdate(comparator_, keyReadNew_Vld, keyReadBase_UpdateVld);
                            LastTokenCheck(base_state, keyReadBase_UpdateVld);
                            MetaUpdate(curr_streamID_base, curr_token_cnt, out_streamID_next, in_streamID_base,
                                       keyRead_base, keyReadBase_UpdateVld);
                        } else {
                            comparator_ = ST_BASE;
                            new_state = SRC_EMPTY;
                        }
                    } else { // new_state == SRC_STREAM && base_state == SRC_TMP
                             // new_state == SRC_STREAM && base_state == SRC_EMPTY
                        tmp_[rd_ptr] = new_token;
                        wr_ptr_next = rd_ptr + 1;
                        if (base_token.keyEnd) {
                            CompareUpdate(comparator_, keyReadNew_Vld, keyReadBase_UpdateVld);
                            LastTokenCheck(base_state, keyReadBase_UpdateVld);
                            MetaUpdate(curr_streamID_base, curr_token_cnt, out_streamID_next, in_streamID_base,
                                       keyRead_base, keyReadBase_UpdateVld);
                            new_state = (wr_ptr_gt_one) ? SRC_TMP : SRC_EMPTY;
                            wr_ptr = rd_ptr + 1;
                            rd_ptr = 0;
                        } else {
                            comparator_ = ST_BASE;
                            new_state = SRC_EMPTY;
                            TmpTokenCheck(base_state, rd_ptr, rd_ptr_UpdateVld);
                        }
                    }
                } else { // need further comparison. Both input stream read in new token
                    comparator_ = ST_COMPARE;
                    out_keyStream_next.write(base_token);
                    base_token.printDetail();
                    if (newState_isStream && baseState_isStream) {
                        wr_ptr_next = wr_ptr + 1;
                        tmp_[wr_ptr] = (base_token);
                        new_state = SRC_STREAM;
                        base_state = SRC_STREAM;
                        wr_ptr++;
                    } else if ((newState_isStream && baseState_isTmp) || (newState_isStream && baseState_isEmpty)) {
                        new_state = SRC_STREAM;
                        TmpTokenCheck(base_state, rd_ptr, rd_ptr_UpdateVld);
                    } else { // new_state == SRC_TMP && base_state == SRC_STREAM
                             // new_state == SRC_EMPTY && base_state == SRC_STREAM
                        TmpTokenCheck(new_state, rd_ptr, rd_ptr_UpdateVld);
                        base_state = SRC_STREAM;
                    }
                }
            } else if (comparator_isnew) {
                out_keyStream_next.write(new_token);
                new_token.printDetail();
                if (!new_token.keyEnd) { // need to read next token
                    comparator_ = ST_NEW;
                    if (newState_isTmp) {
                        rd_ptr++;
                        if (rd_ptr_UpdateVld) {
                            new_state = SRC_STREAM;
                            rd_ptr = 0;
                            wr_ptr = wr_ptr_next;
                        }
                    } else if (newState_isEmpty)
                        new_state = SRC_STREAM;
                } else {
                    CompareUpdate(comparator_, keyReadNew_UpdateVld, keyReadBase_Vld);
                    LastTokenCheck(new_state, keyReadNew_UpdateVld);
                    MetaUpdate(curr_streamID_new, curr_token_cnt, out_streamID_next, in_streamID_new, keyRead_new,
                               keyReadNew_UpdateVld);

                    rd_ptr = 0;
                    wr_ptr = wr_ptr_next;
                    base_state = (keyReadBase_UpdateVld) ? SRC_TMP : SRC_EMPTY;
                }
            } else if (comparator_isbase) {
                out_keyStream_next.write(base_token);
                base_token.printDetail();
                if (!base_token.keyEnd) { // need to read next token
                    comparator_ = ST_BASE;
                    if (baseState_isTmp) {
                        rd_ptr++;
                        if (rd_ptr_UpdateVld) {
                            base_state = SRC_STREAM;
                            rd_ptr = 0;
                            wr_ptr = wr_ptr_next;
                        }
                    } else if (baseState_isEmpty)
                        base_state = SRC_STREAM;
                } else {
                    CompareUpdate(comparator_, keyReadNew_Vld, keyReadBase_UpdateVld);
                    LastTokenCheck(base_state, keyReadBase_UpdateVld);
                    MetaUpdate(curr_streamID_base, curr_token_cnt, out_streamID_next, in_streamID_base, keyRead_base,
                               keyReadBase_UpdateVld);

                    rd_ptr = 0;
                    wr_ptr = wr_ptr_next;
                    new_state = (keyReadNew_UpdateVld) ? SRC_TMP : SRC_EMPTY;
                }
            } else { // idle state, should have consumed all tokens from both streams
                comparator_ = ST_IDLE;
            }

            if (new_state == SRC_STREAM) {
                new_token = in_keyStream_new.read();
                if (base_state == SRC_STREAM)
                    base_token = in_keyStream_base.read();
                else if (base_state == SRC_TMP)
                    base_token = tmp_[rd_ptr];
            } else if (new_state == SRC_TMP) {
                new_token = tmp_[rd_ptr];
                if (base_state == SRC_STREAM)
                    base_token = in_keyStream_base.read();
                else if (base_state == SRC_TMP)
                    base_token = tmp_[rd_ptr];
            } else {
                if (base_state == SRC_STREAM)
                    base_token = in_keyStream_base.read();
                else if (base_state == SRC_TMP)
                    base_token = tmp_[rd_ptr];
            }
        }
    }
};

} // namespace lsm
} // namespace database
} // namespace xf
