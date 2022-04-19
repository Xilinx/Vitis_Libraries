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

#ifndef XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_LINE_PARSE_HPP
#define XF_DATA_ANALYTICS_L1_DATAFRAME_INTERNAL_LINE_PARSE_HPP

#include <ap_int.h>
#include <hls_stream.h>

#ifndef __SYNTHESIS__
#include <iostream>
#endif

namespace xf {
namespace data_analytics {
namespace dataframe {
namespace internal {

// Tokens
enum Token { QuoteToken = 0, CommaToken, ReturnToken, StringToken, KTokenCount };
// Token Map
#define N StringToken
#define N8 N, N, N, N, N, N, N, N
#define N16 N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
static const unsigned char tokenMap[256] = {N8,          N,   N,
                                            ReturnToken, // \n
                                            N,           N,
                                            N, // \r
                                            N16,         N,   N,   N,   N,
                                            QuoteToken, // "
                                            N8,          N,
                                            CommaToken, // ,
                                            N16,         N16, N16, N16, N16, N16, N16, N16,
                                            N16,         N16, N16, N16, N16, N,   N,   N};
#undef N
#undef N8
#undef N16
// States
enum IterativeParsingState {
    ParsingFieldDispatchState = 0,
    ParsingDQFieldStartState, //
    ParsingDQFieldEndState,   // end of double-quote field
    ParsingFieldStartState,   //
    ParsingFieldEndState,     // end of field
    ParsingErrorState,        // sink state
    ParsingKStateCount
};

// State Transition Graph
static const char G[ParsingKStateCount][KTokenCount] = {
    // Dispatch
    {
        ParsingDQFieldStartState, // Quote
        ParsingErrorState,        // Comma
        ParsingErrorState,        // Return
        ParsingFieldStartState    // String
    },
    // double-quote field start, leading/trailing quote has been reomved
    {
        ParsingDQFieldEndState,   // Quote
        ParsingDQFieldStartState, // Embedded Comma
        ParsingErrorState,        // Return, not support
        ParsingDQFieldStartState  // String
    },
    // double-quote field end
    {
        ParsingErrorState,         // Quote
        ParsingFieldDispatchState, // Comma
        ParsingFieldDispatchState, // Return
        ParsingErrorState          // String
    },
    // field start without quote
    {
        ParsingErrorState,         // Quote
        ParsingFieldEndState,      // Comma
        ParsingFieldDispatchState, // Return
        ParsingFieldStartState     // String
    },
    // field end
    {
        ParsingDQFieldStartState, // Quote
        ParsingErrorState,        // Comma
        ParsingErrorState,        // Return
        ParsingFieldStartState    // String
    },
    // error (sink state)
    {
        ParsingErrorState, // Quote
        ParsingErrorState, // Comma
        ParsingErrorState, // Return
        ParsingErrorState  // String
    }};

inline void transit(ap_uint<8> in_byte,
                    Token token,
                    IterativeParsingState src,

                    // 0~7: data, 8: vld, 9: line end, 10: end
                    hls::stream<ap_uint<11> >& o_data_strm
                    /*hls::stream<bool>& o_e_strm,
                    hls::stream<bool>& o_vld_strm,
                    hls::stream<bool>& o_ln_e_strm*/) {
#pragma HLS inline
#ifndef __SYNTHESIS__
// std::cout << "inbyte:" << std::hex << in_byte << std::dec << ", src:" << (unsigned int)src << std::endl;
#endif
    ap_uint<11> out;
    switch (src) {
        case ParsingFieldDispatchState:
        case ParsingDQFieldStartState: {
            if (token != QuoteToken) {
                out(7, 0) = in_byte;
                out[8] = 0;
                out[9] = 0;
                out[10] = 0;
                o_data_strm.write(out);
                // o_e_strm.write(false);      // not end of file
                // o_vld_strm.write(false);    // not end of field
                // o_ln_e_strm.write(false);   // not end of line
            }
            break;
        }
        case ParsingDQFieldEndState: {
            if (token == ReturnToken || token == CommaToken) {
                out(7, 0) = 0;
                out[8] = 1;
                out[9] = (token == ReturnToken);
                out[10] = 0;
                o_data_strm.write(out);
                // o_data_strm.write(0);
                // o_e_strm.write(false);
                // o_vld_strm.write(true);
                // o_ln_e_strm.write((token == ReturnToken));
            }
            break;
        }
        case ParsingFieldStartState: {
            if (token == StringToken) {
                out(7, 0) = in_byte;
                out[8] = 0;
                out[9] = 0;
                out[10] = 0;
                o_data_strm.write(out);
                // o_data_strm.write(in_byte);
                // o_e_strm.write(false);
                // o_vld_strm.write(false);
                // o_ln_e_strm.write(false);
            } else if (token == CommaToken || token == ReturnToken) {
                out(7, 0) = 0;
                out[8] = 1;
                out[9] = (token == ReturnToken);
                out[10] = 0;
                o_data_strm.write(out);
                // o_data_strm.write(0);
                // o_e_strm.write(false);
                // o_vld_strm.write(true);
                // o_ln_e_strm.write((token == ReturnToken));
            }
            break;
        }
        case ParsingFieldEndState: {
            if (token == StringToken) {
                out(7, 0) = in_byte;
                out[8] = 0;
                out[9] = 0;
                out[10] = 0;
                o_data_strm.write(out);
                // o_data_strm.write(in_byte);
                // o_e_strm.write(false);
                // o_vld_strm.write(false);
                // o_ln_e_strm.write(false);
            }
            break;
        }
    }
}

// Parse the input per-line of CSV file iteratively, 1B/cycle
// Delimiter: comma; Currently it doesn't support embedded double-quote and line breaks;
static void iterativeCSVParse(hls::stream<ap_uint<9> >& in_strm,
                              // output
                              hls::stream<ap_uint<11> >& o_data_strm
                              /*hls::stream<bool>& o_e_strm,
                              hls::stream<bool>& o_vld_strm,
                              hls::stream<bool>& o_ln_e_strm*/) {
    IterativeParsingState state = ParsingFieldDispatchState;
    bool nb = true;
    ap_uint<9> in = in_strm.read();
    bool e = in[8];
PARSER_CORE_LOOP:
    while (!e) {
#pragma HLS pipeline II = 1
        if (nb) {
            ap_uint<8> in_byte = in(7, 0);
            Token token = (Token)tokenMap[(unsigned char)in_byte];
            IterativeParsingState nxt_state = (IterativeParsingState)G[state][token];
            transit(in_byte, token, state, o_data_strm /*, o_e_strm, o_vld_strm, o_ln_e_strm*/);
            state = nxt_state;
        }
        nb = in_strm.read_nb(in);
        if (nb) e = in[8];
    }

    ap_uint<11> out;
    out(7, 0) = 0;
    out[8] = 0;
    out[9] = 0;
    out[10] = 1;
    o_data_strm.write(out);
    // o_e_strm.write(true);
}
} // namespace internal
} // namespace dataframe
} // namespace data_analytics
} // namespace xf
#endif
