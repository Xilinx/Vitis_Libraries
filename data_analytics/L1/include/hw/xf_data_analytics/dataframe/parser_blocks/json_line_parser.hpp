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

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_JSON_LINE_PARSE_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_JSON_LINE_PARSE_HPP
#include "hls_stream.h"
#include "ap_int.h"
#include <stdint.h>

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

// States (15)
enum IterativeParsingState {
    IterativeParsingFinishState = 0, // sink states at top
    IterativeParsingErrorState,      // sink states at top
    IterativeParsingStartState,

    // Object states
    IterativeParsingObjectInitialState,
    IterativeParsingMemberKeyStartState,
    IterativeParsingMemberKeyEndState,
    IterativeParsingMemberStringStartState,
    IterativeParsingMemberNumberStartState,
    IterativeParsingMemberValueEndState,
    IterativeParsingObjectFinishState,

    // Delimiter states (at bottom)
    IterativeParsingKeyValueDelimiterState,
    IterativeParsingMemberDelimiterState,

    cIterativeParsingStateCount
};
// Tokens (11)
enum Token {
    LeftCurlyBracketToken = 0,
    RightCurlyBracketToken,

    CommaToken,
    ColonToken,

    StringToken,
    NumberToken,

    SpaceToken,

    kTokenCount
};
#define N NumberToken
#define N16 N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
#define N8 N, N, N, N, N, N, N, N
// Maps from ASCII to Token
static const unsigned char tokenMap[256] = {
    SpaceToken, //\0
    N8,         // 1-8
    SpaceToken, //\t
    N,           N,
    N,
    SpaceToken, //\r
    N,           N,
    N16, // 10~1F
    SpaceToken,  N,
    StringToken, N,
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    CommaToken,  N,
    N,
    N, // 20~2F
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    ColonToken,  N,
    N,           N,
    N,
    N,   // 30~3F
    N16, // 40~4F
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    N,
    N, // LeftBracketToken,
    N,
    N, // RightBracketToken,
    N,
    N, // 50~5F
    N,           N,
    N,           N,
    N,           NumberToken,
    NumberToken, N,
    N,           N,
    N,           N,
    NumberToken, N,
    NumberToken,
    N, // 60~6F
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    N,           N,
    N,           LeftCurlyBracketToken,
    N,           RightCurlyBracketToken,
    N,
    N, // 70~7F
    N16,         N16,
    N16,         N16,
    N16,         N16,
    N16,
    N16 // 80~FF
};

#undef N
#undef N16

// array
static const char G[cIterativeParsingStateCount][kTokenCount] = {
    // Finish(sink state)
    {
        IterativeParsingErrorState, // Left curly bracket
        IterativeParsingErrorState, // Right curly bracket
        IterativeParsingErrorState, // Comma
        IterativeParsingErrorState, // Colon
        IterativeParsingErrorState, // String
        IterativeParsingErrorState, // Number
        IterativeParsingFinishState // Space
    },
    // Error(sink state)
    {
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
        IterativeParsingErrorState,          // Left curly bracket
        IterativeParsingObjectFinishState,   // Right curly bracket
        IterativeParsingErrorState,          // Comma
        IterativeParsingErrorState,          // Colon
        IterativeParsingMemberKeyStartState, // String
        IterativeParsingErrorState,          // Number
        IterativeParsingObjectInitialState   // Space
    },
    // MemberKeyStartState
    {
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
        IterativeParsingMemberStringStartState, // Left curly bracket
        IterativeParsingMemberStringStartState, // Right curly bracket
        IterativeParsingMemberStringStartState, // Comma
        IterativeParsingMemberStringStartState, // Colon
        IterativeParsingMemberValueEndState,    // String
        IterativeParsingMemberStringStartState, // Number
        IterativeParsingMemberStringStartState  // Space
    },
    // MemberNumberStart
    {
        IterativeParsingErrorState,             // Left curly bracket
        IterativeParsingObjectFinishState,      // Right curly bracket
        IterativeParsingMemberDelimiterState,   // Comma
        IterativeParsingErrorState,             // Colon
        IterativeParsingErrorState,             // String
        IterativeParsingMemberNumberStartState, // Number
        IterativeParsingMemberNumberStartState  // Space
    },
    // ValueEnd
    {
        IterativeParsingErrorState,           // Left curly bracket
        IterativeParsingObjectFinishState,    // Right curly bracket
        IterativeParsingMemberDelimiterState, // Comma
        IterativeParsingErrorState,           // Colon
        IterativeParsingErrorState,           // String
        IterativeParsingErrorState,           // Number
        IterativeParsingMemberValueEndState   // Space
    },
    // ObjectFinish(sink state)
    {
        IterativeParsingObjectInitialState, // Left curly bracket
        IterativeParsingErrorState,         // Right curly bracket
        IterativeParsingErrorState,         // Comma
        IterativeParsingErrorState,         // Colon
        IterativeParsingErrorState,         // String
        IterativeParsingErrorState,         // Number
        IterativeParsingObjectFinishState   // Space
    },                                      // Number

    // KeyValueDelimiter
    {
        IterativeParsingErrorState,             // Left curly bracket(push MemberValue state)
        IterativeParsingErrorState,             // Right curly bracket
        IterativeParsingErrorState,             // Comma
        IterativeParsingErrorState,             // Colon
        IterativeParsingMemberStringStartState, // String
        IterativeParsingMemberNumberStartState, // Number
        IterativeParsingKeyValueDelimiterState  // Space
    },
    // memberDelimiter
    {
        IterativeParsingErrorState,          // Left curly bracket(push MemberValue state)
        IterativeParsingErrorState,          // Right curly bracket
        IterativeParsingErrorState,          // Comma
        IterativeParsingErrorState,          // Colon
        IterativeParsingMemberKeyStartState, // String
        IterativeParsingErrorState,          // Number
        IterativeParsingMemberDelimiterState // Space
    }};                                      // End of G

inline IterativeParsingState Transit(ap_uint<8> in_byte,
                                     hls::stream<ap_uint<8> >& o_k_strm,
                                     hls::stream<bool>& k_vld_strm,
                                     hls::stream<bool>& e_k_strm,

                                     hls::stream<ap_uint<8> >& o_val_strm,
                                     hls::stream<bool>& val_vld_strm,
                                     hls::stream<bool>& e_val_strm,

                                     hls::stream<bool>& ln_e_strm,

                                     IterativeParsingState src,
                                     Token token,
                                     IterativeParsingState dst) {
#pragma HLS inline
    switch (dst) {
        case IterativeParsingMemberKeyStartState: {
            // keep write out the char of key
            o_k_strm.write(in_byte);
            k_vld_strm.write(true);
            e_k_strm.write(false);
            ln_e_strm.write(false);
            break;
        }
        case IterativeParsingMemberKeyEndState: {
            // stop write-out the key
            if (src == IterativeParsingMemberKeyStartState) {
                o_k_strm.write(in_byte);
                k_vld_strm.write(false);
                e_k_strm.write(false);
                ln_e_strm.write(false);
            }
            break;
        }
        case IterativeParsingMemberDelimiterState:
        case IterativeParsingObjectFinishState: {
            // end of value
            if (src == IterativeParsingMemberNumberStartState) {
                o_val_strm.write(in_byte);
                val_vld_strm.write(false);
                e_val_strm.write(false);
            }
            // write the flag to indicate the end of the line
            if (dst == IterativeParsingObjectFinishState) {
                o_k_strm.write(0);
                k_vld_strm.write(false);
                e_k_strm.write(false);
                ln_e_strm.write(true);
            }
            break;
        }
        case IterativeParsingMemberStringStartState:
        case IterativeParsingMemberNumberStartState: {
            // keep write-out the value
            o_val_strm.write(in_byte);
            val_vld_strm.write(true);
            e_val_strm.write(false);
            break;
        }
        case IterativeParsingMemberValueEndState: {
            // end of value
            if (src == IterativeParsingMemberStringStartState) {
                o_val_strm.write(in_byte);
                val_vld_strm.write(false);
                e_val_strm.write(false);
            }
            break;
        }
        default:
            break;
    }

    return dst;
}

/**
 *
 * @brief Parse the input line of JSON file iteratively.
 *
 * There is no complete error check mechanism. Currently it doesn't support nested-object and array.
 * This module seperates the key and value for each fields based on state machine.
 *
 *
 * @param in_strm input the char of each line every cycle.
 * @param i_e_strm end flag for in_strm.
 * @param o_k_strm ouput key for each field char by char.
 * @param e_k_strm end flag for o_k_strm.
 * @param k_vld_strm valid flag for o_k_strm, it indicates the start and end of each key.
 * @param o_val_strm output the value for each field char by char.
 * @param e_val_strm end flag for o_val_strm.
 * @param val_vld_strm valid flag for o_val_strm, it indicates the start and end of each value.
 * @param ln_e_strm end flag for each line.
 *
 */

static void iterativeParse(hls::stream<ap_uint<8> >& in_strm,
                           hls::stream<bool>& i_e_strm,
                           // output key to the parsekey module
                           hls::stream<ap_uint<8> >& o_k_strm,
                           hls::stream<bool>& e_k_strm,
                           // valid signal for each key
                           hls::stream<bool>& k_vld_strm,
                           // output value to the parseValue module
                           hls::stream<ap_uint<8> >& o_val_strm,
                           hls::stream<bool>& e_val_strm,
                           // valid signal for each value
                           hls::stream<bool>& val_vld_strm,
                           hls::stream<bool>& ln_e_strm) {
    IterativeParsingState state = IterativeParsingStartState;
    bool e = i_e_strm.read();
    ap_uint<8> in_byte;
    bool skip_space = true;
    Token t;
    static int cnt1 = 0;
    // write on dummy
    e_k_strm.write(false);
    while (!e) {
#pragma HLS pipeline II = 1
        in_byte = in_strm.read();
        e = i_e_strm.read();
        t = (Token)tokenMap[(unsigned char)in_byte];
        IterativeParsingState n = (IterativeParsingState)G[state][t];
        IterativeParsingState d = Transit(in_byte, o_k_strm, k_vld_strm, e_k_strm, o_val_strm, val_vld_strm, e_val_strm,
                                          ln_e_strm, state, t, n);
        // Transition to the new state.
        state = d;
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
