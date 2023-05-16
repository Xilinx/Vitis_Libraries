/*
 * Copyright (C) 2019-2022, Xilinx, Inc.
 * Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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

#include "xf_data_analytics/text/regex_engine_sc.hpp"

extern "C" {
#include "oniguruma.h"
}

namespace xf {
namespace data_analytics {
namespace text {
namespace re {

VPP_BP RegexEngine::msg_buf_pool;
VPP_BP RegexEngine::len_buf_pool;
VPP_BP RegexEngine::out_buf_pool;

// constructor
// load binary and program
RegexEngine::RegexEngine(const int instr_depth,
                         const int char_class_num,
                         const int capture_grp_num,
                         const int msg_size,
                         const int max_slice_size,
                         const int max_slice_num)
    : reCfg(instr_depth, char_class_num, capture_grp_num),
      kInstrDepth(instr_depth),
      kCharClassNum(char_class_num),
      kCaptureGrpNum(capture_grp_num),
      kMsgSize(msg_size),
      kMaxSliceSize(max_slice_size),
      kMaxSliceNum(max_slice_num) {
    msg_buf_pool = re_engine_acc::create_bufpool(vpp::input);
    len_buf_pool = re_engine_acc::create_bufpool(vpp::input);
    out_buf_pool = re_engine_acc::create_bufpool(vpp::output);
}

ErrCode RegexEngine::compile(std::string pattern) {
    return reCfg.compile(pattern);
}

uint32_t RegexEngine::getCpgpNm() const {
    return reCfg.getCpgpNm();
}

ErrCode RegexEngine::match(
    uint32_t total_lnm, const uint64_t* msg_buff, uint32_t* offt_buff, uint16_t* len_buff, uint32_t* out_buff) {
    timeval tv_start, tv_end;
    gettimeofday(&tv_start, 0);
    details::MM mm;
    const uint64_t* cfg_buff = reCfg.getConfigBits();
    uint32_t cpgp_nm = reCfg.getCpgpNm();
    std::string pattern = reCfg.pattern;
    uint32_t max_slice_lnm = 0;

    uint16_t* lnm_per_sec = mm.aligned_alloc<uint16_t>(kMaxSliceNum);
    uint32_t* pos_per_sec = mm.aligned_alloc<uint32_t>(kMaxSliceNum);

    uint32_t sec_num = findSecNum(len_buff, total_lnm, &max_slice_lnm, lnm_per_sec, pos_per_sec);

    const size_t cfg_size_per_cu = (kInstrDepth + kCharClassNum * 4 + 2);
    const size_t msg_size_per_cu = cfg_size_per_cu + ((kMaxSliceSize / 8) + 1);
    const size_t len_size_per_cu = (max_slice_lnm + 2);
    const size_t out_size_per_cu = (max_slice_lnm * (cpgp_nm + 1) + 1);
    ap_uint<16>* len_ptr_src = mm.aligned_alloc<ap_uint<16> >(sec_num * len_size_per_cu);
    ap_uint<64>* msg_ptr_src = mm.aligned_alloc<ap_uint<64> >(sec_num * msg_size_per_cu);
    for (int sec = 0; sec < sec_num; sec++) {
        size_t slice_sz = 0;
        uint16_t lnm = lnm_per_sec[sec];
        uint32_t pos = pos_per_sec[sec];
        for (uint16_t i = 0; i < lnm; i++) {
            size_t tmp = slice_sz;
            if (len_buff[pos + i] < kMsgSize) tmp += (len_buff[pos + i] + 7) / 8;
            if (len_buff[pos + i] > kMsgSize) {
                len_ptr_src[sec * len_size_per_cu + i + 2] = 0;
            } else {
                len_ptr_src[sec * len_size_per_cu + i + 2] = len_buff[pos + i];
                memcpy(msg_ptr_src + sec * msg_size_per_cu + cfg_buff[0] + slice_sz + 1, msg_buff + offt_buff[pos + i],
                       len_buff[pos + i]);
            }
            slice_sz = tmp;
        }
        memcpy(msg_ptr_src + sec * msg_size_per_cu, cfg_buff, cfg_buff[0] * sizeof(ap_uint<64>));
        msg_ptr_src[sec * msg_size_per_cu + cfg_buff[0]] = slice_sz + 1;
        len_ptr_src[sec * len_size_per_cu] = (lnm + 2) / 65536;
        len_ptr_src[sec * len_size_per_cu + 1] = (lnm + 2) % 65536;
    }

    gettimeofday(&tv_end, 0);
    double tvtime = details::tvdiff(tv_start, tv_end);
    fprintf(stdout, "The log file is partition into %d section with max_slice_lnm %d and  takes %f ms.\n", sec_num,
            max_slice_lnm, tvtime / 1000);

    // initialize oniguruma regex
    regex_t* reg;
    OnigRegion* region;
    region = onig_region_new();
    OnigEncoding use_encs[1];
    use_encs[0] = ONIG_ENCODING_ASCII;
    onig_initialize(use_encs, sizeof(use_encs) / sizeof(use_encs[0]));
    UChar* pattern_c = (UChar*)pattern.c_str();
    OnigErrorInfo einfo;
    int r = onig_new(&reg, pattern_c, pattern_c + strlen((char*)pattern_c), ONIG_OPTION_DEFAULT, ONIG_ENCODING_ASCII,
                     ONIG_SYNTAX_DEFAULT, &einfo);

    int sec = 0;
    gettimeofday(&tv_start, 0);
    re_engine_acc::send_while([=, &sec]() -> bool {
        re_engine_acc::set_handle(sec);
        ap_uint<64>* msg_ptr =
            (ap_uint<64>*)re_engine_acc::alloc_buf(msg_buf_pool, msg_size_per_cu * sizeof(ap_uint<64>));
        ap_uint<16>* len_ptr =
            (ap_uint<16>*)re_engine_acc::alloc_buf(len_buf_pool, len_size_per_cu * sizeof(ap_uint<16>));
        ap_uint<32>* out_ptr =
            (ap_uint<32>*)re_engine_acc::alloc_buf(out_buf_pool, out_size_per_cu * sizeof(ap_uint<32>));

        memcpy(len_ptr, len_ptr_src + len_size_per_cu * sec, len_size_per_cu * sizeof(ap_uint<16>));
        memcpy(msg_ptr, msg_ptr_src + msg_size_per_cu * sec, msg_size_per_cu * sizeof(ap_uint<64>));

        re_engine_acc::compute(msg_ptr, len_ptr, out_ptr);

        return (++sec < sec_num);
    });
    // receive all result in order
    re_engine_acc::receive_all_in_order([=]() {
        int secID = re_engine_acc::get_handle();
        ap_uint<32>* ptr_src = (ap_uint<32>*)re_engine_acc::get_buf(out_buf_pool);

        uint16_t lnm = lnm_per_sec[secID];
        uint32_t pos = pos_per_sec[secID];
        unsigned char* max_str = (unsigned char*)malloc(65536);
        for (uint16_t i = 0; i < lnm; i++) {
            uint8_t result = ptr_src[i * (cpgp_nm + 1) + 1];
            // step 1: stack overflow or large message
            if (result == 2 || result == 3) {
                // step 2: find the position and length message
                size_t msg_pos = offt_buff[pos + i];
                const uint64_t* msg = &msg_buff[msg_pos];
                uint16_t msg_len = len_buff[pos + i];
                memcpy(max_str, msg, msg_len);
                max_str[msg_len] = '\0';
                UChar* str = (UChar*)max_str;
                unsigned char* end = str + strlen((char*)str);
                unsigned char* start = str;
                unsigned char* range = end;
                int r = onig_search(reg, str, end, start, range, region, ONIG_OPTION_NONE);
                // step 4: insert the result back to out_ptr
                if (r == 0) {
                    ptr_src[i * (cpgp_nm + 1) + 1] = 1;
                    for (int k = 0; k < cpgp_nm; ++k) {
                        uint32_t out = region->end[k] * 65536 + region->beg[k];
                        ptr_src[i * (cpgp_nm + 1) + 2 + k] = out;
                    }
                } else if (r == ONIG_MISMATCH) {
                    ptr_src[i * (cpgp_nm + 1) + 1] = 0;
                }
            }
        }

        size_t sz = lnm * (cpgp_nm + 1) * sizeof(ptr_src[0]);
        memcpy(out_buff + (size_t)(pos * (cpgp_nm + 1)), ptr_src + 1, sz);
    });

    re_engine_acc::join();
    gettimeofday(&tv_end, 0);
    double re_tvtime = details::tvdiff(tv_start, tv_end);
    double total_log_size = (double)offt_buff[total_lnm - 1] * 8 / 1024 / 1024;
    std::cout << "regex pipelined, time: " << (double)re_tvtime / 1000 << " ms, size: " << total_log_size
              << " MB, throughput: " << total_log_size / 1024 / ((double)re_tvtime / 1000000) << " GB/s" << std::endl;
    std::cout
        << "-----------------------------Finished regex pipelined test----------------------------------------------"
        << std::endl
        << std::endl;
    return SUCCESS;
}

uint32_t RegexEngine::findSecNum(
    uint16_t* len_buff, uint32_t lnm, uint32_t* slice_lnm, uint16_t* lnm_per_sec, uint32_t* pos_per_sec) {
    uint32_t sec_sz = 0;
    uint32_t sec_nm = 0;
    uint32_t start_lnm = 0;
    uint32_t end_lnm = 0;
    uint32_t tmp_slice_nm = 0;
    for (unsigned int i = 0; i < lnm; ++i) {
        if (len_buff[i] < kMsgSize) {
            sec_sz += (len_buff[i] + 7) / 8;
            if (sec_sz > kMaxSliceSize / 8) {
                start_lnm = end_lnm;
                end_lnm = i;
                if (end_lnm - start_lnm > tmp_slice_nm) tmp_slice_nm = end_lnm - start_lnm;
                lnm_per_sec[sec_nm] = end_lnm - start_lnm;
                pos_per_sec[sec_nm] = start_lnm;
                sec_nm++;
                sec_sz = (len_buff[i] + 7) / 8;
            } else if (i == lnm - 1) {
                start_lnm = end_lnm;
                end_lnm = lnm;
                lnm_per_sec[sec_nm] = end_lnm - start_lnm;
                pos_per_sec[sec_nm] = start_lnm;
                sec_nm++;
                if (end_lnm - start_lnm > tmp_slice_nm) tmp_slice_nm = end_lnm - start_lnm;
            }
        }
    }
    *slice_lnm = tmp_slice_nm + 2;
    return sec_nm;
}

} // namesapce re
} // namespace text
} // namespace data_analytics
} // namespace xf
