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

/**
 * @file amin.h
 * @brief BLAS Level 1 amin template function implementation.
 *
 * This file is part of XF BLAS Library.
 */

#ifndef XF_BLAS_AMAX_H
#define XF_BLAS_AMAX_H

#ifndef __cplusplus
#error "BLAS Library only works with C++."
#endif

#include "ap_int.h"
#include "hls_math.h"
#include "hls_stream.h"
#include "xf_blas/utility.h"

#define abs(x) (x > 0? x : -x)

namespace xf {
namespace linear_algebra {
namespace blas {

/**
 * @brief amin function that returns the position of the vector element that has the minimum magnitude.
 *
 * @tparam t_DataType the data type of the vector entries
 * @tparam t_N maximum number of entries in the input vector, p_N >= 1+(p_n-1)*|p_incx|
 * @tparam l_ParEntries number of parallelly processed entries in the input vector 
 *
 * @param p_n the number of stided entries entries in the input vector p_x, p_n % l_ParEntries == 0
 * @param p_x the input vector
 * @param p_result the resulting index, which is 0 if p_n <= 0
*/


template<typename t_DataType, 
	unsigned int t_DataWidth, 
	unsigned int t_Entries, 
	typename t_IndexType,
	bool t_Max> 
		class BinaryCmp{
			public:
				static const void cmp(t_DataType p_x[t_Entries], t_DataType &p_value, t_IndexType &p_index){
					const unsigned int l_halfEntries = t_Entries >> 1;
					t_DataType l_msbValue, l_lsbValue;
					t_IndexType l_msbIndex, l_lsbIndex;
					BinaryCmp<t_DataType, t_DataWidth,  l_halfEntries, t_IndexType, t_Max>::cmp(p_x, l_lsbValue, l_lsbIndex);
					BinaryCmp<t_DataType, t_DataWidth, l_halfEntries, t_IndexType, t_Max>::cmp(p_x + l_halfEntries, l_msbValue, l_msbIndex);
					if(l_msbValue == l_lsbValue){
						p_value = l_lsbValue;
						p_index = l_lsbIndex;
					} else if((l_msbValue > l_lsbValue) == t_Max){
						p_value = l_msbValue;
						p_index = l_halfEntries + l_msbIndex;
					} else {
						p_value = l_lsbValue;
						p_index = l_lsbIndex;
					}
				}
		};
template<typename t_DataType, 
	unsigned int t_DataWidth, 
	typename t_IndexType,
	bool t_Max> 
		class BinaryCmp<t_DataType, t_DataWidth, 1, t_IndexType, t_Max>{
			public:
				static const void cmp(t_DataType p_x[1], t_DataType &p_value, t_IndexType &p_index){
					p_index = 0;
					p_value = p_x[p_index];
				}
		};

template<typename t_DataType, typename t_IndexType>
	struct EntryPair{
		t_DataType s_Value;
		t_IndexType s_Index;
	};

template<typename t_DataType, 
	unsigned int t_DataWidth,
	unsigned int t_ParEntries, 
	typename t_IndexType>
		void preProcess(unsigned int p_numElement,
				hls::stream<t_DataType>& p_valueStream,
				hls::stream<t_IndexType>& p_indexStream,
				hls::stream<ap_uint<(t_DataWidth * t_ParEntries)> >& p_x) {
			for (t_IndexType i = 0; i < p_numElement; i++) {
#pragma HLS PIPELINE
				ap_uint<t_DataWidth * t_ParEntries> l_elem = p_x.read();
				t_DataType l_x[t_ParEntries];
				for (t_IndexType k = 0; k < t_ParEntries; k++) {
					ap_uint<t_DataWidth> l_tmp = l_elem.range((k + 1) * t_DataWidth - 1,
							k * t_DataWidth);
					l_x[k] = abs(*(t_DataType* )&l_tmp);
				}
				t_IndexType l_pos;
				t_DataType l_value;
				BinaryCmp<t_DataType, t_DataWidth, t_ParEntries, t_IndexType, false>::cmp(
						l_x, l_value, l_pos);
				p_valueStream.write(l_value);
				p_indexStream.write((i * t_ParEntries) + l_pos);
			}
		}

template<typename t_DataType, 
	unsigned int t_DataWidth,
	unsigned int t_II, 
	typename t_IndexType>
		void postProcess(unsigned int p_numElement,
				hls::stream<t_DataType>& p_valueStream,
				hls::stream<t_IndexType>& p_indexStream,
				t_IndexType &p_result) {
#ifndef __SYNTHESIS__
			assert(p_numElement % t_II == 0);
#endif
			t_DataType l_min;
			t_IndexType l_minIndex = 0;
			for (t_IndexType i = 0; i < p_numElement / t_II; i++) {
#pragma HLS PIPELINE
				t_DataType l_v[t_II];
				t_IndexType l_i[t_II];
				for (t_IndexType j = 0; j < t_II; j++) {
					l_v[j] = p_valueStream.read();
					l_i[j] = p_indexStream.read();
				}
				if (i == 0) {
					l_min = l_v[0];
					l_minIndex = l_i[0];
				}
				t_IndexType l_pos;
				t_DataType l_value;
				BinaryCmp<t_DataType, t_DataWidth, t_II, t_IndexType, false>::cmp(
						l_v, l_value, l_pos);
				if (l_value < l_min) {
					l_min = l_value;
					l_minIndex = l_i[l_pos];
				}
			}
			p_result = l_minIndex;
		}

template<typename t_DataType, 
	unsigned int t_DataWidth, 
	unsigned int t_ParEntries, 
	typename t_IndexType>
		void amin (
				unsigned int p_n,
				hls::stream<ap_uint<(t_DataWidth * t_ParEntries)> > & p_x,
				t_IndexType &p_result
				) {

			unsigned int l_numElem = p_n/t_ParEntries;
#ifndef __SYNTHESIS__
			assert(p_n % t_ParEntries == 0);
#endif
#pragma HLS DATAFLOW
			hls::stream<t_DataType> l_valueStream;
#pragma HLS stream variable=l_valueStream depth=t_ParEntries*2
			hls::stream<t_IndexType> l_indexStream;
#pragma HLS stream variable=l_indexStream depth=t_ParEntries*2

			preProcess<t_DataType, t_DataWidth, t_ParEntries, t_IndexType>(l_numElem, l_valueStream,l_indexStream, p_x);
			postProcess<t_DataType, t_DataWidth, 2, t_IndexType>(l_numElem, l_valueStream, l_indexStream, p_result);

		}

} //end namespace blas
} //end namspace linear_algebra
} //end namespace xf

#endif

