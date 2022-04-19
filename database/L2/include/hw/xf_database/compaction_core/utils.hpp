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

#include <stdio.h>
#include <ap_int.h>
#include <hls_stream.h>
#include "hls_burst_maxi.h"
#include "xf_database/compaction_core/config.hpp"
#include <string>

namespace xf {
namespace database {
namespace lsm {

template <unsigned int BufWidth_>
struct keyDecodeWord {
    ap_uint<BufWidth_> keyWord;
    bool keyEnd;

    void printDetail() {
#ifndef __SYNTHESIS__
#if KernelDebug_
        printf("keyEnd: %d | keyWord: ", keyEnd);
        for (int i = BufWidth_ / 8 - 1; i >= 0; i--) {
            unsigned char curr = keyWord.range(i * 8 + 8 - 1, i * 8);
            if (curr >= 48)
                printf("-- %c ", curr);
            else
                printf("-- %x ", curr);
        }
        printf("\n");
#endif
#endif
    }

    keyDecodeWord() {
        keyWord = ap_uint<BufWidth_>(0);
        keyEnd = false;
    };

    keyDecodeWord(ap_uint<BufWidth_ + 1> bits) { (keyWord, keyEnd) = bits; }

    ap_uint<BufWidth_ + 1> toBits() {
        ap_uint<BufWidth_ + 1> result = (keyWord, keyEnd);
        return result;
    }

    ap_uint<BufWidth_ + 1> toBitsSimple() {
        ap_uint<BufWidth_> result = (keyWord);
        return result;
    }
};

template <unsigned int BufWidth_>
class FPGA_utils {
   private:
    enum states : unsigned char { ST_IDLE, ST_PROCESS, ST_LAST };

   public:
    FPGA_utils(){};

    void DataBurstRead(unsigned int burst_len,
                       ap_uint<BufWidth_>* in_dataBuff,
                       hls::stream<ap_uint<BufWidth_>, StreamDepth_>& in_dataBuff_stream) {
#ifndef __SYNTHESIS__
        printf("Data burst read start. Burst len %d\n", burst_len);
#endif
    dataBurstRead:
        for (int i = 0; i < burst_len; i++) {
            in_dataBuff_stream.write(in_dataBuff[i]);
        }
#ifndef __SYNTHESIS__
        printf("Data burst read end\n");
#endif
    }

    void DataBurstWrite(hls::stream<ap_uint<8>, 2>& out_dataStream_status,
                        hls::stream<ap_uint<BufWidth_>, BufDepth_ * 2>& out_dataStream,
                        ap_uint<BufWidth_>* out_dataBuf) {
        int total = 0;
        int n = out_dataStream_status.read();
        ap_uint<BufWidth_> tmp;
    dataBurstWrite:
        while (n) {
            for (int i = 0; i < n; i++) {
#pragma HLS pipeline II = 1
                tmp = out_dataStream.read();
                out_dataBuf[total + i] = tmp;
            }
            total += n;
            n = out_dataStream_status.read();
        }
    }

    void MetaBurstRead(unsigned int burst_len,
                       ap_uint<32>* in_metaInfo,
                       hls::stream<ap_uint<32>, StreamDepth_>& in_metaInfo_stream) {
#ifndef __SYNTHESIS__
        printf("Meta burst read start. Burst len %d\n", burst_len);
#endif
    metaBurstRead:
        for (int i = 0; i < burst_len; i++) {
            in_metaInfo_stream.write(in_metaInfo[i]);
        }
#ifndef __SYNTHESIS__
        printf("Meta burst read end\n");
#endif
    }

    void MetaBurstWrite(hls::stream<ap_uint<8>, 2>& out_metaStream_status,
                        hls::stream<ap_uint<32>, MetaDepth_ * 2>& out_metaStream,
                        ap_uint<32>* out_metaInfo) {
        int total = 0;
        int n = out_metaStream_status.read();
        ap_uint<32> tmp;
    metaBurstWrite:
        while (n) {
            for (int i = 0; i < n; i++) {
#pragma HLS pipeline II = 1
                tmp = out_metaStream.read();
                out_metaInfo[total + i] = tmp;
            }
            total += n;
            n = out_metaStream_status.read();
        }
    }

    void KeyDecode(ap_uint<8> in_streamID_tag,
                   hls::stream<ap_uint<BufWidth_>, StreamDepth_>& in_dataBuff_stream,
                   hls::stream<ap_uint<32>, StreamDepth_>& in_metaInfo_stream,

                   hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_>& out_keyStream,
                   hls::stream<ap_uint<32>, StreamDepth_>& out_keyCnt,
                   hls::stream<ap_uint<8>, StreamDepth_>& out_streamID,

                   unsigned int in_dataBuff_burst_len,
                   unsigned int in_metaInfo_burst_len) {
        unsigned int dataBuff_burst_cnt = 0;
        unsigned int metaInfo_burst_cnt = 0;
        // we are reading previous stream from memory
        // assume at least on key in it
        if (in_streamID_tag == 0xFF) {
#ifndef __SYNTHESIS__
            printf("key decode #11\n");
#endif

            ap_uint<32> in_keyCnt = in_metaInfo_stream.read();
            metaInfo_burst_cnt++;
#ifndef __SYNTHESIS__
            printf("key decode #12 keyCnt %d\n", (unsigned int)in_keyCnt);
#endif

            unsigned int buf_ptr = 0;
            out_keyCnt.write(in_keyCnt);
            keyDecodeWord<BufWidth_> curr_key;
            curr_key.keyWord = in_dataBuff_stream.read();
            dataBuff_burst_cnt++;
            ap_uint<32> curr_meta = in_metaInfo_stream.read();
            metaInfo_burst_cnt++;
            bool new_key = true;
            unsigned int id = curr_meta.range(7, 0);
            unsigned int tok_cnt = curr_meta.range(23, 16);
            unsigned int cnt = 0;
        memKeyDecode:
            while (cnt < in_keyCnt) {
                if (new_key) {
                    id = curr_meta.range(7, 0);
                    tok_cnt = curr_meta.range(23, 16);
                    new_key = false;
                }
                if (tok_cnt == 1)
                    curr_key.keyEnd = true;
                else
                    curr_key.keyEnd = false;
                curr_key.printDetail();
                out_keyStream.write(curr_key);
                if (cnt < in_keyCnt - 1) {
                    curr_key.keyWord = in_dataBuff_stream.read();
                    dataBuff_burst_cnt++;
                }
                if (tok_cnt == 1) {
                    out_streamID.write(id);
                    if (cnt < in_keyCnt - 1) {
                        curr_meta = in_metaInfo_stream.read();
                        metaInfo_burst_cnt++;
                    }
                    new_key = true;
                    cnt++;
                } else {
                    tok_cnt--;
                }
            }
        } else { // we are reading in a new stream

            ap_uint<32> in_keyCnt = in_metaInfo_stream.read();
            metaInfo_burst_cnt++;
            out_keyCnt.write(in_keyCnt);

#ifndef __SYNTHESIS__
            printf("key decode #1 %d\n", (unsigned int)in_keyCnt);
#endif

            unsigned int write_ptr = 0;
            ap_uint<BufWidth_* 2> curr_word = 0;
            curr_word.range(BufWidth_ - 1, 0) = in_dataBuff_stream.read();
            curr_word.range(2 * BufWidth_ - 1, BufWidth_) = in_dataBuff_stream.read();
            dataBuff_burst_cnt = 2;

            states curr_state = ST_IDLE;
            states next_state = ST_IDLE;
            int cnt = 1;
            int token_cnt = 0;
            unsigned int data_size_orig = 0;

            data_size_orig = in_metaInfo_stream.read();
            metaInfo_burst_cnt++;
            out_streamID.write(in_streamID_tag);

        rawKeyDecode:
            while (cnt <= in_keyCnt) {
                unsigned int data_size;
                bool new_key_single_token = data_size_orig <= bb_cnt_;
                bool curr_key_last_token = token_cnt == 2;
                if (curr_state == ST_IDLE) {
                    data_size = (new_key_single_token) ? (data_size_orig - 1) % bb_cnt_ + 1 : bb_cnt_;
                    token_cnt = (data_size_orig + bb_cnt_ - 1) / bb_cnt_;
                } else {
                    data_size = (curr_key_last_token) ? (data_size_orig - 1) % bb_cnt_ + 1 : bb_cnt_;
                    token_cnt = token_cnt - 1;
                }
                keyDecodeWord<BufWidth_> result;

                // in idle, read in new meta
                if (curr_state == ST_IDLE) {
                    if (!new_key_single_token) {
                        next_state = ST_PROCESS;
                    } else {
                        next_state = ST_IDLE;
                        result.keyEnd = true;
                    }
                } else { // in process, read in data and check for next state
                    if (!curr_key_last_token) {
                        next_state = ST_PROCESS;
                    } else {
                        next_state = ST_IDLE;
                        result.keyEnd = true;
                    }
                }

                if (next_state == ST_IDLE) {
                    cnt++;
                    if (cnt <= in_keyCnt) {
                        data_size_orig = in_metaInfo_stream.read();
                        metaInfo_burst_cnt++;
                        out_streamID.write(in_streamID_tag);
                    }
                }
                curr_state = next_state;

                unsigned int offset = write_ptr * bb_size_;
                unsigned int ingress = data_size * bb_size_;

                result.keyWord.range(ingress - 1, 0) = curr_word.range(offset + ingress - 1, offset);
                if (write_ptr + data_size < bb_cnt_) {
                    write_ptr += data_size;
                } else {
                    curr_word.range(BufWidth_ - 1, 0) = curr_word.range(2 * BufWidth_ - 1, BufWidth_);
                    if (dataBuff_burst_cnt < in_dataBuff_burst_len) {
                        curr_word.range(2 * BufWidth_ - 1, BufWidth_) = in_dataBuff_stream.read();
                        dataBuff_burst_cnt++;
                    } else {
                        curr_word.range(2 * BufWidth_ - 1, BufWidth_) = 0;
                    }
                    write_ptr = (write_ptr + data_size) % bb_cnt_;
                }

            // std::cout << "Key Decode" << std::endl;
            // From little-endian to big-endian for comparison
            EndiannSwap:
                for (unsigned int lower_byte_idx = 0; lower_byte_idx < (BufWidth_ / 8) / 2; lower_byte_idx++) {
                    unsigned int upper_byte_idx = (BufWidth_ / 8) - lower_byte_idx - 1;
                    ap_uint<8> byte_tmp = result.keyWord.range(lower_byte_idx * 8 + 8 - 1, lower_byte_idx * 8);
                    result.keyWord.range(lower_byte_idx * 8 + 8 - 1, lower_byte_idx * 8) =
                        result.keyWord.range(upper_byte_idx * 8 + 8 - 1, upper_byte_idx * 8);
                    result.keyWord.range(upper_byte_idx * 8 + 8 - 1, upper_byte_idx * 8) = byte_tmp;
                }
                // result.printDetail();

                out_keyStream.write(result);
            }
        }
#ifndef __SYNTHESIS__
        printf("key decode #2 | data in %d cnt %d | meta in %d cnt %d\n", in_dataBuff_burst_len, dataBuff_burst_cnt,
               in_metaInfo_burst_len, metaInfo_burst_cnt);
#endif
        if (in_dataBuff_burst_len > dataBuff_burst_cnt) {
        bufRedundantPop:
            for (int i = 0; i < in_dataBuff_burst_len - dataBuff_burst_cnt; i++) in_dataBuff_stream.read();
        }
        if (in_metaInfo_burst_len > metaInfo_burst_cnt) {
        metaRedundantPop:
            for (int i = 0; i < in_metaInfo_burst_len - metaInfo_burst_cnt; i++) in_metaInfo_stream.read();
        }
#ifndef __SYNTHESIS__
        printf("key decode #3\n");
#endif
    }

    void StreamWrite(hls::stream<keyDecodeWord<BufWidth_>, StreamDepth_>& in_keyStream,
                     hls::stream<ap_uint<32>, StreamDepth_>& in_keyCnt,
                     hls::stream<ap_uint<32>, StreamDepth_>& in_streamID,

                     hls::stream<ap_uint<BufWidth_>, 2 * BufDepth_>& out_dataStream,
                     hls::stream<ap_uint<7>, 2>& out_dataStream_status,
                     hls::stream<ap_uint<32>, 2 * MetaDepth_>& out_metaStream,
                     hls::stream<ap_uint<7>, 2>& out_metaStream_status) {
        int data_cnt = 0;
        int meta_cnt = 0;

        // key count is the first value
        unsigned int keyCnt = in_keyCnt.read();
        out_metaStream.write(keyCnt);
        meta_cnt++;
        if (meta_cnt == MetaDepth_) {
            out_metaStream_status.write(MetaDepth_);
            meta_cnt = 0;
        }

        unsigned i = 0;
    keyStreamDump:
        while (i < keyCnt) {
            keyDecodeWord<BufWidth_> curr = in_keyStream.read();
            out_dataStream.write(curr.toBitsSimple());
            data_cnt++;
            if (data_cnt == BufDepth_) {
                out_dataStream_status.write(BufDepth_);
                data_cnt = 0;
            }
            if (curr.keyEnd) {
                i++;
                ap_uint<32> currID = in_streamID.read();
#ifndef __SYNTHESIS__
#if KernelDebug_
                unsigned int cid = currID.range(7, 0);
                unsigned int ccnt = currID.range(23, 16);
                printf("Stream %d Token Cnt %d\n", cid, ccnt);
#endif
#endif
                out_metaStream.write(currID);
                meta_cnt++;
                if (meta_cnt == MetaDepth_) {
                    out_metaStream_status.write(MetaDepth_);
                    meta_cnt = 0;
                }
            }
        }

        if (data_cnt != ap_uint<8>(0)) {
            out_dataStream_status.write(data_cnt);
        }
        if (meta_cnt != ap_uint<8>(0)) {
            out_metaStream_status.write(meta_cnt);
        }
        out_dataStream_status.write(0);
        out_metaStream_status.write(0);
    }

   private:
    const unsigned int bb_size_ = 8;            // basic block size
    const unsigned int bb_cnt_ = BufWidth_ / 8; // basic block count
};

template <unsigned int Width_>
void Writer(hls::burst_maxi<ap_uint<Width_> >& wport,
            hls::stream<ap_uint<Width_>, 2 * BufDepth_>& din,
            hls::stream<ap_uint<7>, 2>& dn) {
    ap_uint<7> cnt = 0;
    int off = 0;
    ap_uint<64> check = 0;
    bool end = false;
writer_loop:
    while (!end) {
#pragma HLS pipeline II = 1
        bool check_l = check[64 - 1];
        if (cnt == 0) {
            ap_uint<7> t;
            if (dn.read_nb(t)) {
                end = (t == 0);
                int off_l = off;
                off += t;
                if (t) {
                    // make request
                    wport.write_request(off_l, t);
                }
                cnt = t;
            }
            check = (check << 1);
        } else {
            // send data
            ap_uint<Width_> d;
            if (din.read_nb(d)) {
                check = (check << 1);
                check[0] = (cnt == 1); // if last of burst, put check to wait queue.
                cnt--;
                wport.write(d);
            } else {
                check = (check << 1);
            }
        }
        // must do at the end, to ensure II
        if (check_l) {
            wport.write_response();
#ifdef __SYNTHESIS__
            // XXX must enforce order between bvalid and ack.
            ap_wait();
#endif
        }
    }
    // check remaining requests
    if (check != 0) {
        for (int i = 0; i < 64; i++) {
            bool check_l = check[64 - 1];
            if (check_l) {
                wport.write_response();
            }
            check = (check << 1);
            if (check == 0) {
                break;
            }
        }
    }
}

} // namespace lsm
} // namespace database
} // namespace xf
