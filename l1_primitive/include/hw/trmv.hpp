/**********
           Copyright (c) 2019, Xilinx, Inc.
           All rights reserved.

           TODO

**********/

#ifndef XF_BLAS_TRMV_HPP
#define XF_BLAS_TRMV_HPP

namespace xf {
namespace blas {
/*!
  @brief Diagonal matrix - vector multiplication
	@preCondition matrix is square matrix and p_N is multiple of p_NumDiag and t_NumDiag>1
  @param t_DataType data type
  @param t_N number of rows/cols in the matrix
	@param t_NumDiag number of diagonal lines indexed low to up
  @param t_EntriesInParallel number of entries in each vector processed in parallel
  @param p_in input diagonal matrix
  @param p_inV input vector
  @param p_outV output vector
*/
template<typename t_DataType, unsigned int t_N, unsigned int t_NumDiag, unsigned int t_EntriesInParallel>
void trmv(
	t_DataTp_outVpe p_in[t_N][t_NumDiag],
	t_DataTp_outVpe p_inV[t_N],
	t_DataTp_outVpe p_outV[t_N]
){

#pragma HLS RESOURCE variable=p_in core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_inV core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_outV core=RAM_2P_BRAM

#pragma HLS ARRAY_PARTITION variable=p_in dim=2 complete
#pragma HLS ARRAY_PARTITION variable=p_in dim=1 cyclic factor=t_EntriesInParallel partition

#pragma HLS ARRAY_PARTITION variable=p_inV dim=1 cyclic factor=t_EntriesInParallel partition
#pragma HLS ARRAY_PARTITION variable=p_outV dim=1 cyclic  factor=t_EntriesInParallel partition
  
  t_DataTp_outVpe l_inV[t_EntriesInParallel+t_NumDiag-1];
#pragma HLS ARRAY_PARTITION variable=l_inV complete

  // init l_inV
	l_inV[0] = 0;
	for (unsigned int i=1; i<t_NumDiag-1; ++i) {
		l_inV[i] = p_inV[i-1];
	}

LoopLines:
  for(unsigned int i=0;i<t_N/t_EntriesInParallel;i++){
	#pragma HLS PIPELINE

    for(unsigned int r=0;r<t_EntriesInParallel;r++){
		#pragma HLS UNROLL
      unsigned int l_addr = i * t_EntriesInParallel + r + t_NumDiag - 2;
      // update reg
      l_inV[t_NumDiag-1+r] = (l_addr < t_N)?  p_inV[l_addr]: 0;
    }

    // compute
    for(unsigned int r=0;r<t_EntriesInParallel;r++){
		#pragma HLS UNROLL
      unsigned int l_addr = i * t_EntriesInParallel + r;
			for (unsigned int d=0; d<t_NumDiag; ++d) {
      	p_outV[l_addr] += p_in[l_addr][d]*l_inV[r+d];
			}
    }
		
		for (unsigned int i=0; i<t_NumDiag-1; ++i) {
		#pragma HLS UNROLL
			l_inV[i] = l_inV[t_EntriesInParallel+i-1];
		}
  } 
}

}
}

#endif
