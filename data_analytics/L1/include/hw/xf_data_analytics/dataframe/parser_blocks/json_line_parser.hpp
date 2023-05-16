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

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_JSON_LINE_PARSE_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_JSON_LINE_PARSE_HPP
#include "hls_stream.h"
#include "ap_int.h"
#include <stdint.h>

//#define __DEBUG_JSON_KEY__
//#define __DEBUG_JSON_VALUE__

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

// States
enum IterativeParsingState {
    IterativeParsingErrorState = 0, // sink states at top
    IterativeParsingStartState,

    // Object states
    IterativeParsingObjectInitialState,
    IterativeParsingMemberKeyStartState,
    IterativeParsingMemberKeyEndState,
    IterativeParsingMemberStringStartState,
    IterativeParsingMemberNumberStartState,
    IterativeParsingMemberStringEndState,
    IterativeParsingObjectFinishState,

    // Array states
    IterativeParsingArrayInitialState,
    IterativeParsingElementStringStartState,
    IterativeParsingElementNumberStartState,
    IterativeParsingElementStringEndState,
    IterativeParsingArrayFinishState,

    // Delimiter states
    IterativeParsingElementDelimiterState,
    IterativeParsingMemberDelimiterState,
    IterativeParsingKeyValueDelimiterState,

    cIterativeParsingStateCount
};

// Tokens
enum Token {
    LeftBracketToken = 0,
    RightBracketToken,
    LeftCurlyBracketToken,
    RightCurlyBracketToken,

    CommaToken,
    ColonToken,

    StringToken,
    NumberToken,

    SpaceToken,

    kTokenCount
};

#define N NumberToken
#define N8 N, N, N, N, N, N, N, N
#define N16 N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N

// Maps from ASCII to Token
static const unsigned char tokenMap[256] = {
    SpaceToken, //\0
    N8,         // 1-8
    SpaceToken, //\t
    N,
    N,
    N,
    SpaceToken, //\r
    N,
    N,
    N16,        // 10~1F
    SpaceToken, // space
    N,
    StringToken, // "
    N,
    N8,
    CommaToken, // ,
    N,
    N,
    N, // 20~2F
    N8,
    N,
    N,
    ColonToken, // :
    N,
    N,
    N,
    N,
    N,   // 30~3F
    N16, // 40~4F
    N8,
    N,
    N,
    N,
    LeftBracketToken, // [
    N,
    RightBracketToken, // ]
    N,
    N,   // 50~5F
    N16, // 60~6F
    N8,
    N,
    N,
    N,
    LeftCurlyBracketToken, // {
    N,
    RightCurlyBracketToken, // }
    N,
    N, // 70~7F
    N16,
    N16,
    N16,
    N16,
    N16,
    N16,
    N16,
    N16 // 80~FF
};

#undef N
#undef N8
#undef N16

// array
static const char TransitionMap[cIterativeParsingStateCount][kTokenCount] = {
    // Error(sink state)
    {
        IterativeParsingErrorState, // Left bracket
        IterativeParsingErrorState, // Right bracket
        IterativeParsingErrorState, // Left curly bracket
        IterativeParsingErrorState, // Right curly bracket
        IterativeParsingErrorState, // Comma
        IterativeParsingErrorState, // Colon
        IterativeParsingErrorState, // String
        IterativeParsingErrorState, // Number
        IterativeParsingErrorState  // Space
    },
    // Start
    {
        IterativeParsingErrorState,         // Left bracket
        IterativeParsingErrorState,         // Right bracket
        IterativeParsingObjectInitialState, // Left curly bracket
        IterativeParsingErrorState,         // Right curly bracket
        IterativeParsingErrorState,         // Comma
        IterativeParsingErrorState,         // Colon
        IterativeParsingErrorState,         // String
        IterativeParsingErrorState,         // Number
        IterativeParsingStartState          // Space
    },
    // ObjectInitial
    {
        IterativeParsingErrorState,          // Left bracket
        IterativeParsingErrorState,          // Right bracket
        IterativeParsingErrorState,          // Left curly bracket
        IterativeParsingObjectFinishState,   // Right curly bracket
        IterativeParsingErrorState,          // Comma
        IterativeParsingErrorState,          // Colon
        IterativeParsingMemberKeyStartState, // String
        IterativeParsingErrorState,          // Number
        IterativeParsingObjectInitialState   // Space
    },
    // MemberKeyStart
    {
        IterativeParsingMemberKeyStartState, // Left bracket
        IterativeParsingMemberKeyStartState, // Right bracket
        IterativeParsingMemberKeyStartState, // Left curly bracket
        IterativeParsingMemberKeyStartState, // Right curly bracket
        IterativeParsingMemberKeyStartState, // Comma
        IterativeParsingMemberKeyStartState, // Colon
        IterativeParsingMemberKeyEndState,   // String
        IterativeParsingMemberKeyStartState, // Number
        IterativeParsingMemberKeyStartState  // Space
    },
    // MemberKeyEnd
    {
        IterativeParsingErrorState,             // Left bracket
        IterativeParsingErrorState,             // Right bracket
        IterativeParsingErrorState,             // Left curly bracket
        IterativeParsingErrorState,             // Right curly bracket
        IterativeParsingErrorState,             // Comma
        IterativeParsingKeyValueDelimiterState, // Colon
        IterativeParsingErrorState,             // String
        IterativeParsingErrorState,             // Number
        IterativeParsingMemberKeyEndState       // Space
    },
    // MemberStringStart
    {
        IterativeParsingMemberStringStartState, // Left bracket
        IterativeParsingMemberStringStartState, // Right bracket
        IterativeParsingMemberStringStartState, // Left curly bracket
        IterativeParsingMemberStringStartState, // Right curly bracket
        IterativeParsingMemberStringStartState, // Comma
        IterativeParsingMemberStringStartState, // Colon
        IterativeParsingMemberStringEndState,   // String
        IterativeParsingMemberStringStartState, // Number
        IterativeParsingMemberStringStartState  // Space
    },
    // MemberNumberStart
    {
        IterativeParsingErrorState,             // Left bracket
        IterativeParsingErrorState,             // Right bracket
        IterativeParsingErrorState,             // Left curly bracket
        IterativeParsingObjectFinishState,      // Right curly bracket
        IterativeParsingMemberDelimiterState,   // Comma
        IterativeParsingErrorState,             // Colon
        IterativeParsingErrorState,             // String
        IterativeParsingMemberNumberStartState, // Number
        IterativeParsingMemberNumberStartState  // Space
    },
    // MemberStringEnd
    {
        IterativeParsingErrorState,           // Left bracket
        IterativeParsingErrorState,           // Right bracket
        IterativeParsingErrorState,           // Left curly bracket
        IterativeParsingObjectFinishState,    // Right curly bracket
        IterativeParsingMemberDelimiterState, // Comma
        IterativeParsingErrorState,           // Colon
        IterativeParsingErrorState,           // String
        IterativeParsingErrorState,           // Number
        IterativeParsingMemberStringEndState  // Space
    },
    // ObjectFinish
    {
        IterativeParsingErrorState,         // Left bracket
        IterativeParsingErrorState,         // Right bracket
        IterativeParsingObjectInitialState, // Left curly bracket
        IterativeParsingObjectFinishState,  // Right curly bracket
        IterativeParsingObjectInitialState, // Comma
        IterativeParsingErrorState,         // Colon
        IterativeParsingErrorState,         // String
        IterativeParsingErrorState,         // Number
        IterativeParsingObjectFinishState   // Space
    },
    // ArrayInitial
    {
        IterativeParsingErrorState,              // Left bracket
        IterativeParsingArrayFinishState,        // Right bracket
        IterativeParsingErrorState,              // Left curly bracket
        IterativeParsingErrorState,              // Right curly bracket
        IterativeParsingErrorState,              // Comma
        IterativeParsingErrorState,              // Colon
        IterativeParsingElementStringStartState, // String
        IterativeParsingElementNumberStartState, // Number
        IterativeParsingArrayInitialState        // Space
    },
    // ElementStringStart
    {
        IterativeParsingElementStringStartState, // Left bracket
        IterativeParsingElementStringStartState, // Right bracket
        IterativeParsingElementStringStartState, // Left curly bracket
        IterativeParsingElementStringStartState, // Right curly bracket
        IterativeParsingElementStringStartState, // Comma
        IterativeParsingElementStringStartState, // Colon
        IterativeParsingElementStringEndState,   // String
        IterativeParsingElementStringStartState, // Number
        IterativeParsingElementStringStartState  // Space
    },
    // ElementNumberStart
    {
        IterativeParsingErrorState,              // Left bracket
        IterativeParsingArrayFinishState,        // Right bracket
        IterativeParsingErrorState,              // Left curly bracket
        IterativeParsingErrorState,              // Right curly bracket
        IterativeParsingElementDelimiterState,   // Comma
        IterativeParsingErrorState,              // Colon
        IterativeParsingErrorState,              // String
        IterativeParsingElementNumberStartState, // Number
        IterativeParsingElementNumberStartState  // Space
    },
    // ElementStringEnd
    {
        IterativeParsingErrorState,            // Left bracket
        IterativeParsingArrayFinishState,      // Right bracket
        IterativeParsingErrorState,            // Left curly bracket
        IterativeParsingErrorState,            // Right curly bracket
        IterativeParsingElementDelimiterState, // Comma
        IterativeParsingErrorState,            // Colon
        IterativeParsingErrorState,            // String
        IterativeParsingErrorState,            // Number
        IterativeParsingElementStringEndState  // Space
    },
    // ArrayFinish
    {
        IterativeParsingErrorState,           // Left bracket
        IterativeParsingErrorState,           // Right bracket
        IterativeParsingErrorState,           // Left curly bracket
        IterativeParsingObjectFinishState,    // Right curly bracket
        IterativeParsingMemberDelimiterState, // Comma
        IterativeParsingErrorState,           // Colon
        IterativeParsingErrorState,           // String
        IterativeParsingErrorState,           // Number
        IterativeParsingArrayFinishState      // Space
    },
    // ElementDelimiter
    {
        IterativeParsingErrorState,              // Left bracket
        IterativeParsingErrorState,              // Right bracket
        IterativeParsingErrorState,              // Left curly bracket
        IterativeParsingErrorState,              // Right curly bracket
        IterativeParsingErrorState,              // Comma
        IterativeParsingErrorState,              // Colon
        IterativeParsingElementStringStartState, // String
        IterativeParsingElementNumberStartState, // Number
        IterativeParsingElementDelimiterState    // Space
    },
    // MemberDelimiter
    {
        IterativeParsingErrorState,          // Left bracket
        IterativeParsingErrorState,          // Right bracket
        IterativeParsingErrorState,          // Left curly bracket
        IterativeParsingErrorState,          // Right curly bracket
        IterativeParsingErrorState,          // Comma
        IterativeParsingErrorState,          // Colon
        IterativeParsingMemberKeyStartState, // String
        IterativeParsingErrorState,          // Number
        IterativeParsingMemberDelimiterState // Space
    },
    // KeyValueDelimiter
    {
        IterativeParsingArrayInitialState,      // Left bracket
        IterativeParsingErrorState,             // Right bracket
        IterativeParsingObjectInitialState,     // Left curly bracket
        IterativeParsingErrorState,             // Right curly bracket
        IterativeParsingErrorState,             // Comma
        IterativeParsingErrorState,             // Colon
        IterativeParsingMemberStringStartState, // String
        IterativeParsingMemberNumberStartState, // Number
        IterativeParsingKeyValueDelimiterState  // Space
    }};                                         // End of TransitionMap

// Stack for push/pop operations for nested field
template <typename T, int DEPTH>
class Stack {
   public:
    Stack() {
#pragma HLS array_partition variable = stack_ dim = 0
        stk_ptr_ = 1;
        stack_[0] = 0;
    }
    void push(T t) { stack_[stk_ptr_++] = t; }
    T pop() { return stack_[--stk_ptr_]; }
    bool empty() { return (stk_ptr_ == 1); }
    T top() { return stack_[stk_ptr_ - 1]; }

   private:
    T stack_[DEPTH];
    int stk_ptr_;
};

/**
 *
 * @brief Parse the input line of JSON file iteratively.
 *
 * There is no complete error check mechanism. Currently it supports nested-object and array on leaf node.
 * The nested array and array of object are not supported.
 * This module seperates the key and value for each fields based on the input byte stream driving the internal
 * transition of the state machine.
 *
 *
 * @param in_strm input character of each JSON line, 1 byte per cycle.
 * @param i_e_strm end flag for in_strm.
 * @param o_k_strm ouput key for each field char by char.
 * @param e_k_strm end flag for o_k_strm.
 * @param k_idx_strm index of key.
 * @param k_vld_strm valid flag for o_k_strm, it indicates the start and end of each key.
 * @param o_val_strm output value for each field char by char.
 * @param e_val_strm end flag for o_val_strm.
 * @param val_idx_strm index of value.
 * @param val_vld_strm valid flag for o_val_strm, it indicates the start and end of each value.
 * @param ln_e_strm end flag for each line.
 *
 */
template <int ARRAY_BW>
void iterativeParse(hls::stream<ap_uint<8> >& in_strm,
                    hls::stream<bool>& i_e_strm,
                    // output key to the parseKey module
                    hls::stream<ap_uint<8> >& o_k_strm,
                    hls::stream<bool>& e_k_strm,
                    hls::stream<ap_uint<8> >& k_idx_strm,
                    // valid signal for each key
                    hls::stream<bool>& k_vld_strm,
                    // output value to the parseValue module
                    hls::stream<ap_uint<8> >& o_val_strm,
                    hls::stream<bool>& e_val_strm,
                    hls::stream<ap_uint<ARRAY_BW> >& val_idx_strm,
                    // valid signal for each value
                    hls::stream<bool>& val_vld_strm,
                    hls::stream<bool>& ln_e_strm) {
#ifndef __SYNTHESIS__
#if defined(__DEBUG_JSON_KEY__) || defined(__DEBUG_JSON_VALUE__)
    const char* IterativeParsingStateString[] = {"IterativeParsingErrorState",
                                                 "IterativeParsingStartState",
                                                 "IterativeParsingObjectInitialState",
                                                 "IterativeParsingMemberKeyStartState",
                                                 "IterativeParsingMemberKeyEndState",
                                                 "IterativeParsingMemberStringStartState",
                                                 "IterativeParsingMemberNumberStartState",
                                                 "IterativeParsingMemberStringEndState",
                                                 "IterativeParsingObjectFinishState",
                                                 "IterativeParsingArrayInitialState",
                                                 "IterativeParsingElementStringStartState",
                                                 "IterativeParsingElementNumberStartState",
                                                 "IterativeParsingElementStringEndState",
                                                 "IterativeParsingArrayFinishState",
                                                 "IterativeParsingElementDelimiterState",
                                                 "IterativeParsingMemberDelimiterState",
                                                 "IterativeParsingKeyValueDelimiterState"};
#endif
#endif

    // 2^8 byte key length
    ap_uint<8> key_idx = 0;
    ap_uint<8> key_offt = 0;
    // 128 levels of nests should be sufficient
    Stack<ap_uint<8>, 128> stk;

    ap_uint<ARRAY_BW> val_idx = 0;

    IterativeParsingState src = IterativeParsingStartState;
    ap_uint<8> in_byte;
    Token token;

    bool e = i_e_strm.read();
    // write on dummy
    e_k_strm.write(false);
    while (!e) {
#pragma HLS pipeline II = 1
        in_byte = in_strm.read();
        e = i_e_strm.read();
        token = (Token)tokenMap[(unsigned char)in_byte];
        IterativeParsingState dst = (IterativeParsingState)TransitionMap[src][token];
#ifndef __SYNTHESIS__
#if defined(__DEBUG_JSON_KEY__) || defined(__DEBUG_JSON_VALUE__)
        std::cout << "in_byte = " << (char)in_byte << std::endl;
        std::cout << "dst_state = " << IterativeParsingStateString[dst] << std::endl;
#endif
#endif
        // write out on key/value stream based on internal state transition
        switch (dst) {
            case IterativeParsingObjectInitialState: {
                if (src == IterativeParsingKeyValueDelimiterState) {
                    {
#pragma HLS latency min = 0 max = 0
                        // write out the nested separator
                        o_k_strm.write(0);
                        k_idx_strm.write(key_offt + key_idx);
                        k_vld_strm.write(true);
                        e_k_strm.write(false);
                        ln_e_strm.write(false);
                    }
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_KEY__
                    std::cout << "key_byte = " << (char)0 << std::endl;
                    std::cout << "key_idx = " << key_offt + key_idx << std::endl;
                    std::cout << "key_vld = 1" << std::endl;
                    std::cout << "ln_e = 0" << std::endl;
#endif
#endif
                    // push current nested key information
                    stk.push(key_offt + ++key_idx);
                    // update base key offset
                    key_offt += key_idx;
                    // reset current key index
                    key_idx = 0;
                }
                break;
            }
            case IterativeParsingMemberKeyStartState: {
                // keep write out the key, and filter out the \" at the start/end
                if (in_byte != '"') {
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_KEY__
                    std::cout << "key_byte = " << (char)in_byte << std::endl;
                    std::cout << "key_idx = " << key_offt + key_idx << std::endl;
                    std::cout << "key_vld = 1" << std::endl;
                    std::cout << "ln_e = 0" << std::endl;
#endif
#endif
                    {
#pragma HLS latency min = 0 max = 0
                        o_k_strm.write(in_byte);
                        k_idx_strm.write(key_offt + key_idx++);
                        k_vld_strm.write(true);
                        e_k_strm.write(false);
                        ln_e_strm.write(false);
                    }
                }
                break;
            }
            case IterativeParsingMemberDelimiterState: {
                // end of value
                if (src == IterativeParsingMemberNumberStartState || src == IterativeParsingMemberStringEndState) {
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(-1);
                    val_vld_strm.write(false);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 0" << std::endl;
#endif
#endif
                }
                break;
            }
            case IterativeParsingObjectFinishState: {
                // end of value
                if (src == IterativeParsingMemberNumberStartState || src == IterativeParsingMemberStringEndState) {
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(-1);
                    val_vld_strm.write(false);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 0" << std::endl;
#endif
#endif
                }
                // reset key index for new key
                key_idx = 0;
                // restore last nest key offset, this is not the end of the JSON line
                if (!stk.empty() && token == RightCurlyBracketToken) {
                    // pop last nested key information
                    stk.pop();
                    key_offt = stk.top();
                    // end of JSON line, write out line end flag
                } else if (stk.empty()) {
                    if (src == IterativeParsingMemberNumberStartState || src == IterativeParsingMemberStringEndState ||
                        src == IterativeParsingArrayFinishState) {
                        {
#pragma HLS latency min = 0 max = 0
                            // end of json line
                            o_k_strm.write(0);
                            k_idx_strm.write(0);
                            k_vld_strm.write(false);
                            e_k_strm.write(false);
                            ln_e_strm.write(true);
                        }
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_KEY__
                        std::cout << "key_byte = " << (char)0 << std::endl;
                        std::cout << "key_idx = 0" << std::endl;
                        std::cout << "key_vld = 0" << std::endl;
                        std::cout << "ln_e = 1" << std::endl;
#endif
#endif
                    }
                }
                break;
            }
            case IterativeParsingMemberStringStartState:
            case IterativeParsingMemberNumberStartState: {
                // end of key
                if (src == IterativeParsingKeyValueDelimiterState) {
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_KEY__
                    std::cout << "key_byte = " << (char)in_byte << std::endl;
                    std::cout << "key_idx = " << key_offt + key_idx << std::endl;
                    std::cout << "key_vld = 0" << std::endl;
                    std::cout << "ln_e = 0" << std::endl;
#endif
#endif
                    {
#pragma HLS latency min = 0 max = 0
                        o_k_strm.write(in_byte);
                        k_idx_strm.write(key_offt + key_idx);
                        k_vld_strm.write(false);
                        e_k_strm.write(false);
                        ln_e_strm.write(false);
                    }
                    key_idx = 0;
                }
                // do not write out when space character exists between the number and comma
                if (!(dst == IterativeParsingMemberNumberStartState && token == SpaceToken)) {
                    // keep write out the value
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(-1);
                    val_vld_strm.write(true);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 1" << std::endl;
#endif
#endif
                }
                break;
            }
            case IterativeParsingMemberStringEndState: {
                // end of value
                if (src == IterativeParsingMemberStringStartState) {
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(-1);
                    val_vld_strm.write(true);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 1" << std::endl;
#endif
#endif
                }
                break;
            }
            case IterativeParsingArrayInitialState: {
                // end of key
                if (src == IterativeParsingKeyValueDelimiterState) {
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_KEY__
                    std::cout << "key_byte = " << (char)in_byte << std::endl;
                    std::cout << "key_idx = " << key_offt + key_idx << std::endl;
                    std::cout << "key_vld = 0" << std::endl;
                    std::cout << "ln_e = 0" << std::endl;
#endif
#endif
                    {
#pragma HLS latency min = 0 max = 0
                        o_k_strm.write(in_byte);
                        k_idx_strm.write(key_offt + key_idx);
                        k_vld_strm.write(false);
                        e_k_strm.write(false);
                        ln_e_strm.write(false);
                    }
                    key_idx = 0;
                }
                // reset the value index
                val_idx = 0;
                break;
            }
            case IterativeParsingArrayFinishState: {
                // write out null (corner case)
                if (src == IterativeParsingArrayInitialState) {
                    o_val_strm.write('n');
                    val_idx_strm.write(-1);
                    val_vld_strm.write(false);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = n" << std::endl;
                    std::cout << "val_vld = 0" << std::endl;
#endif
#endif
                    // write out normal value end
                } else if (src == IterativeParsingElementStringEndState ||
                           src == IterativeParsingElementNumberStartState) {
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(-1);
                    val_vld_strm.write(false);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 0" << std::endl;
#endif
#endif
                }
                break;
            }
            case IterativeParsingElementStringStartState:
            case IterativeParsingElementNumberStartState: {
                // do not write out when space character exists between the number and comma
                if (!(dst == IterativeParsingElementNumberStartState && token == SpaceToken)) {
                    // keep write out the value
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(val_idx);
                    val_vld_strm.write(true);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 1" << std::endl;
#endif
#endif
                }
                break;
            }
            case IterativeParsingElementStringEndState: {
                // end of value
                if (src == IterativeParsingElementStringStartState) {
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(val_idx);
                    val_vld_strm.write(true);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 1" << std::endl;
#endif
#endif
                }
                break;
            }
            case IterativeParsingElementDelimiterState: {
                if (src == IterativeParsingElementNumberStartState || src == IterativeParsingElementStringEndState) {
                    // end of value
                    o_val_strm.write(in_byte);
                    val_idx_strm.write(val_idx);
                    val_vld_strm.write(false);
                    e_val_strm.write(false);
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_VALUE__
                    std::cout << "val_byte = " << (char)in_byte << std::endl;
                    std::cout << "val_vld = 0" << std::endl;
#endif
#endif
                    {
#pragma HLS latency min = 0 max = 0
                        // insert same key for pairing each element
                        o_k_strm.write(0);
                        k_idx_strm.write(0);
                        k_vld_strm.write(true);
                        e_k_strm.write(false);
                        ln_e_strm.write(false);
                    }
#ifndef __SYNTHESIS__
#ifdef __DEBUG_JSON_KEY__
                    std::cout << "key_byte = " << (char)0 << std::endl;
                    std::cout << "key_idx = 0" << std::endl;
                    std::cout << "key_vld = 1" << std::endl;
                    std::cout << "ln_e = 0" << std::endl;
#endif
#endif
                    // to next value
                    val_idx++;
                }
                break;
            }
            default:
                break;
        }
        // transit to the new state
        src = dst;
    }
    e_val_strm.write(true);
    // write one more dummy data
    e_k_strm.write(true);
    k_vld_strm.write(false);
    o_k_strm.write(0);
    ln_e_strm.write(false);
}
}
}
}
}
#endif
