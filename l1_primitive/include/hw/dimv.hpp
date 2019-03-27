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
  @param t_N maximum number of entries alogn diagonal line
	@param t_NumDiag number of diagonal lines indexed low to up, 3: tridiagonal; 5: pentadiagonal; 7:heptadiagonal
  @param t_EntriesInParallel number of entries in each vector processed in parallel
  @param p_in input diagonal matrix
  @param p_inV input vector
	@param p_n number of entries along diagonal line, must be multiple of t_EntriesInParallel
  @param p_outV output vector
*/
template<typename t_DataType, unsigned int t_N, unsigned int t_NumDiag, unsigned int t_EntriesInParallel>
void dimv(
	t_DataType p_in[t_N][t_NumDiag],
	t_DataType p_inV[t_N],
	unsigned int p_n,
	t_DataType p_outV[t_N]
){

#pragma HLS RESOURCE variable=p_in core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_inV core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_outV core=RAM_2P_BRAM

#pragma HLS ARRAY_PARTITION variable=p_in dim=2 complete
#pragma HLS ARRAY_PARTITION variable=p_in dim=1 cyclic factor=t_EntriesInParallel partition

#pragma HLS ARRAY_PARTITION variable=p_inV dim=1 cyclic factor=t_EntriesInParallel partition
#pragma HLS ARRAY_PARTITION variable=p_outV dim=1 cyclic  factor=t_EntriesInParallel partition
  
  t_DataType l_inV[t_EntriesInParallel+t_NumDiag-1];
#pragma HLS ARRAY_PARTITION variable=l_inV complete

  // init l_inV
	l_inV[0] = 0;
	for (unsigned int i=1; i<t_NumDiag-1; ++i) {
		l_inV[i] = p_inV[i-1];
	}

	unsigned int l_nBlocks = p_n / t_EntriesInParallel;
LoopLines:
  for(unsigned int i=0;i<l_nBlocks;++i){
	#pragma HLS PIPELINE
    for(unsigned int r=0;r<t_EntriesInParallel;r++){
		#pragma HLS UNROLL
      unsigned int l_addr = i * t_EntriesInParallel + r + t_NumDiag - 2;
      // update reg
      l_inV[t_NumDiag-1+r] = (l_addr < p_n)?  p_inV[l_addr]: 0;
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
			l_inV[i] = l_inV[t_EntriesInParallel+i];
		}
  } 
}

}
}

#endif
