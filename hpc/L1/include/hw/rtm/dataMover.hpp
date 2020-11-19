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

#ifndef XF_HPC_RTM_DATAMOVER_HPP
#define XF_HPC_RTM_DATAMOVER_HPP

/**
 * @file dataMover.hpp
 * @brief datamovers are defined here, including memory to stream,
 * stream to stream and stream to memory
 */
namespace xf {
namespace hpc {
namespace rtm {

/**
 * @brief memSelStream reads data alternatively from two memory addresses to a stream
 *
 * @tparam t_InterfaceType is the datatype in memory
 * @tparam t_DataType is the datatype in of the stream
 *
 * @param p_n is the number of data to be read
 * @param p_k is the selection
 * @param p_mem0 is the first memory port
 * @param p_mem1 is the second memory port
 * @param p_str is the output stream
 */
template <typename t_InterfaceType, typename t_DataType>
void memSelStream(unsigned int p_n,
                  unsigned int p_k,
                  t_InterfaceType* p_mem0,
                  t_InterfaceType* p_mem1,
                  hls::stream<t_DataType>& p_str) {
    switch (p_k) {
        case 0:
            for (int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
                t_DataType l_in = p_mem0[i];
                p_str.write(l_in);
            }
            break;
        case 1:
            for (int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
                t_DataType l_in = p_mem1[i];
                p_str.write(l_in);
            }
            break;
    }
}

/**
 * @brief streamSelMem reads write alternatively to two memory addresses from a stream
 *
 * @tparam t_InterfaceType is the datatype in memory
 * @tparam t_DataType is the datatype in of the stream
 *
 * @param p_n is the number of data to be read
 * @param p_k is the selection
 * @param p_mem0 is the first memory port
 * @param p_mem1 is the second memory port
 * @param p_str is the input stream
 */
template <typename t_InterfaceType, typename t_DataType>
void streamSelMem(unsigned int p_n,
                  unsigned int p_k,
                  t_InterfaceType* p_mem0,
                  t_InterfaceType* p_mem1,
                  hls::stream<t_DataType>& p_str) {
    switch (p_k) {
        case 0:
            for (int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
                p_mem0[i] = p_str.read();
            }
            break;
        case 1:
            for (int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
                p_mem1[i] = p_str.read();
            }
            break;
    }
}
/**
 * @brief wide2stream converts an integer of wide datawidth to an integer of base datawidth
 *
 * @tparam t_DataWidth is the base datawidth
 * @tparam t_Multi is the factor between two datawidth
 *
 * @param p_n is the number of data to be read
 * @param p_wide is the input stream of wide datawidth
 * @param p_str is the output stream of base datawidth
 */

template <unsigned int t_DataWidth, unsigned int t_Multi>
void wide2stream(unsigned int p_n,
                 hls::stream<ap_uint<t_DataWidth * t_Multi> >& p_wide,
                 hls::stream<ap_uint<t_DataWidth> >& p_str) {
    blas::WideType<ap_uint<t_DataWidth>, t_Multi, t_DataWidth> l_wide;
    for (int i = 0, j = 0; i < p_n * t_Multi; i++) {
#pragma HLS PIPELINE
        if (j == 0) l_wide = p_wide.read();
        p_str.write(l_wide.unshift());
        if (j == t_Multi - 1) {
            j = 0;
        } else
            j++;
    }
}

/**
 * @brief stream2wide converts an integer of base datawidth to an integer of wide datawidth
 *
 * @tparam t_DataWidth is the base datawidth
 * @tparam t_Multi is the factor between two datawidth
 *
 * @param p_n is the number of data to be read
 * @param p_str is the input stream of base datawidth
 * @param p_wide is the output stream of wide datawidth
 */
template <unsigned int t_DataWidth, unsigned int t_Multi>
void stream2wide(unsigned int p_n,
                 hls::stream<ap_uint<t_DataWidth> >& p_str,
                 hls::stream<ap_uint<t_DataWidth * t_Multi> >& p_wide) {
    blas::WideType<ap_uint<t_DataWidth>, t_Multi, t_DataWidth> l_wide;
    for (int i = 0, j = 0; i < p_n * t_Multi; i++) {
#pragma HLS PIPELINE
        ap_uint<t_DataWidth> l_str = p_str.read();
        l_wide.unshift(l_str);
        if (j == t_Multi - 1) {
            j = 0;
        } else
            j++;
        if (j == 0) p_wide.write(l_wide);
    }
}

template <unsigned int t_ParEntries, typename t_DataType>
void wide2stream(unsigned int p_n,
                 hls::stream<blas::WideType<t_DataType, t_ParEntries> >& p_wide,
                 hls::stream<t_DataType>& p_str) {
    blas::WideType<t_DataType, t_ParEntries> l_wide;
    for (int i = 0, j = 0; i < p_n * t_ParEntries; i++) {
#pragma HLS PIPELINE
        if (j == 0) l_wide = p_wide.read();
        p_str.write(l_wide[j]);
        if (j == t_ParEntries - 1) {
            j = 0;
        } else
            j++;
    }
}

template <unsigned int t_ParEntries, typename t_DataType>
void stream2wide(unsigned int p_n,
                 hls::stream<t_DataType>& p_str,
                 hls::stream<blas::WideType<t_DataType, t_ParEntries> >& p_wide) {
    blas::WideType<t_DataType, t_ParEntries> l_wide;
    for (int i = 0, j = 0; i < p_n * t_ParEntries; i++) {
#pragma HLS PIPELINE
        l_wide[j] = p_str.read();
        if (j == t_ParEntries - 1) p_wide.write(l_wide);
        if (j == t_ParEntries - 1) {
            j = 0;
        } else
            j++;
    }
}

template <typename t_DesDataType, typename t_DataType>
void streamConversion(unsigned int p_n, hls::stream<t_DataType>& p_in, hls::stream<t_DesDataType>& p_out) {
    for (int i = 0; i < p_n; i++) {
        p_out.write(p_in.read());
    }
}

template <unsigned int t_NumEntries, unsigned int t_ParEntries, typename t_DataType>
void conv2stream(unsigned int p_n,
                 hls::stream<blas::WideType<t_DataType, t_ParEntries> >& p_wide,
                 hls::stream<blas::WideType<t_DataType, t_NumEntries> >& p_str) {
    blas::WideType<t_DataType, t_ParEntries> l_wide;
    blas::WideType<t_DataType, t_NumEntries> l_str;
#ifndef __SYNTHESIS__
    assert(t_ParEntries % t_NumEntries == 0);
#endif

    for (int i = 0; i < p_n; i++) {
        for (int j = 0; j < t_ParEntries / t_NumEntries; j++) {
#pragma HLS PIPELINE
            if (j == 0) l_wide = p_wide.read();
            for (int k = 0; k < t_NumEntries; k++) {
#pragma HLS UNROLL
                l_str[k] = l_wide[j * t_NumEntries + k];
            }
            p_str.write(l_str);
        }
    }
}

template <unsigned int t_NumEntries, unsigned int t_ParEntries, typename t_DataType>
void conv2wide(unsigned int p_n,
               hls::stream<blas::WideType<t_DataType, t_NumEntries> >& p_str,
               hls::stream<blas::WideType<t_DataType, t_ParEntries> >& p_wide) {
#ifndef __SYNTHESIS__
    assert(t_ParEntries % t_NumEntries == 0);
#endif

    blas::WideType<t_DataType, t_ParEntries> l_wide;
    blas::WideType<t_DataType, t_NumEntries> l_str;
    for (int i = 0; i < p_n; i++) {
        for (int j = 0; j < t_ParEntries / t_NumEntries; j++) {
#pragma HLS PIPELINE
            l_str = p_str.read();
            for (int k = 0; k < t_NumEntries; k++) {
#pragma HLS UNROLL
                l_wide[j * t_NumEntries + k] = l_str[k];
            }
            if (j == t_ParEntries / t_NumEntries - 1) p_wide.write(l_wide);
        }
    }
}

template <typename T>
void duplicate(const unsigned int p_n, hls::stream<T>& p_in, hls::stream<T>& p_out0, hls::stream<T>& p_out1) {
    for (int i = 0; i < p_n; i++) {
#pragma HLS PIPELINE
        T l_in = p_in.read();
        p_out0.write(l_in);
        p_out1.write(l_in);
    }
}

template <typename T>
void dataConsumer(const unsigned int p_n, hls::stream<T>& p_s) {
    for (int i = 0; i < p_n; i++)
#pragma HLS PIPELINE
        p_s.read();
}
}
}
}
#endif
