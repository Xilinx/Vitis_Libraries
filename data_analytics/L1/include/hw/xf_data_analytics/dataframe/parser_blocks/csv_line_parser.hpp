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

                    hls::stream<ap_uint<8> >& o_data_strm,
                    hls::stream<bool>& o_e_strm,
                    hls::stream<bool>& o_vld_strm,
                    hls::stream<bool>& o_ln_e_strm) {
#pragma HLS inline
#ifndef __SYNTHESIS__
// std::cout << "inbyte:" << std::hex << in_byte << std::dec << ", src:" << (unsigned int)src << std::endl;
#endif
    switch (src) {
        case ParsingFieldDispatchState:
        case ParsingDQFieldStartState: {
            if (token != QuoteToken) {
                o_data_strm.write(in_byte); // payload
                o_e_strm.write(false);      // not end of file
                o_vld_strm.write(false);    // not end of field
                o_ln_e_strm.write(false);   // not end of line
            }
            break;
        }
        case ParsingDQFieldEndState: {
            if (token == ReturnToken || token == CommaToken) {
                o_data_strm.write(0);
                o_e_strm.write(false);
                o_vld_strm.write(true);
                o_ln_e_strm.write((token == ReturnToken));
            }
            break;
        }
        case ParsingFieldStartState: {
            if (token == StringToken) {
                o_data_strm.write(in_byte);
                o_e_strm.write(false);
                o_vld_strm.write(false);
                o_ln_e_strm.write(false);
            } else if (token == CommaToken || token == ReturnToken) {
                o_data_strm.write(0);
                o_e_strm.write(false);
                o_vld_strm.write(true);
                o_ln_e_strm.write((token == ReturnToken));
            }
            break;
        }
        case ParsingFieldEndState: {
            if (token == StringToken) {
                o_data_strm.write(in_byte);
                o_e_strm.write(false);
                o_vld_strm.write(false);
                o_ln_e_strm.write(false);
            }
            break;
        }
    }
}

/**
 * @brief Parse the input per-line of CSV file iteratively with 1B per cycle
 *
 * Delimiter: comma; Currently it doesn't support embedded double-quote and line breaks;
 *
 * @param in_strm input CSV raw data char by char
 * @param i_e_strm end flag for in_strm
 * @param o_data_strm output value for each column char by char
 * @param o_e_strm end flag for o_data_strm
 * @param o_vld_strm valid flag for o_data_strm, it indicates the start and end of each column
 * @param o_ln_e_strm end flag for each line
 *
 **/
static void iterativeCSVParse(hls::stream<ap_uint<8> >& in_strm,
                              hls::stream<bool>& i_e_strm,
                              // output
                              hls::stream<ap_uint<8> >& o_data_strm,
                              hls::stream<bool>& o_e_strm,
                              hls::stream<bool>& o_vld_strm,
                              hls::stream<bool>& o_ln_e_strm) {
    IterativeParsingState state = ParsingFieldDispatchState;
    bool e = i_e_strm.read();
PARSER_CORE_LOOP:
    while (!e) {
#pragma HLS pipeline II = 1
        ap_uint<8> in_byte = in_strm.read();
        e = i_e_strm.read();
        Token token = (Token)tokenMap[(unsigned char)in_byte];
        IterativeParsingState nxt_state = (IterativeParsingState)G[state][token];
        transit(in_byte, token, state, o_data_strm, o_e_strm, o_vld_strm, o_ln_e_strm);
        state = nxt_state;
    }

    o_e_strm.write(true);
}
}
}
}
}
#endif
