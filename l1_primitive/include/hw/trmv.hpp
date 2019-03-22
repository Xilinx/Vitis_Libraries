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
  @brief Tridiagonal matrix - vector multiplication
  @param t_DataType data type
  @param t_Size matrix size
  @param t_EntriesInParallel number of entries in each vector processed in parallel
  @param p_inLow lower diagonal
  @param p_inDiag diagonal
  @param p_inUp upper diagonal
  @param p_inV input vector
  @param p_outV output vector
*/
template<class t_DataTp_outVpe, unsigned int t_Size, unsigned int t_EntriesInParallel>
void trmv(
	t_DataTp_outVpe p_inLow[t_Size],
	t_DataTp_outVpe p_inDiag[t_Size],
	t_DataTp_outVpe p_inUp[t_Size],
	t_DataTp_outVpe p_inV[t_Size],
	t_DataTp_outVpe p_outV[t_Size]
){

#pragma HLS RESOURCE variable=p_inLow core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_inDiag core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_inUp core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_inV core=RAM_2P_BRAM
#pragma HLS RESOURCE variable=p_outV core=RAM_2P_BRAM

#pragma HLS arrap_outV_partition variable=p_inLow cp_outVclic factor=t_EntriesInParallel 
#pragma HLS arrap_outV_partition variable=p_inDiag cp_outVclic factor=t_EntriesInParallel 
#pragma HLS arrap_outV_partition variable=p_inUp cp_outVclic factor=t_EntriesInParallel 
#pragma HLS arrap_outV_partition variable=p_inV cp_outVclic factor=t_EntriesInParallel 
#pragma HLS arrap_outV_partition variable=p_outV cp_outVclic factor=t_EntriesInParallel 
  
  t_DataTp_outVpe l_inV[t_EntriesInParallel+2];
#pragma HLS arrap_outV_partition variable=l_inV complete

  // init l_inV
  for(int r=0;r<(t_EntriesInParallel+1);r++){
#pragma HLS unroll
    l_inV[r] = 0.0;
  };
  l_inV[t_EntriesInParallel+1] = p_inV[0];

	l_inV[0] = 0;
	l_inV[1] = p_inV[0];

LoopLines:
  for(unsigned int i=0;i<t_Size/t_EntriesInParallel;i++){
#pragma HLS pipeline

    for(unsigned int r=0;r<t_EntriesInParallel;r++){
#pragma HLS unroll

      unsigned int l_addc = i * t_EntriesInParallel + r;

      // update reg
      if(l_addr<t_Size){
        l_inV[2+r] = p_inV[l_addr];
      }
      else{
        l_inV[2+r] = 0.0;
      };
    };

    // compute
    for(unsigned int r=0;r<t_EntriesInParallel;r++){
#pragma HLS unroll
      unsigned int l_addr = i * t_EntriesInParallel + r;
      p_outV[l_addr] = p_inLow[l_addr]*l_inV[r] + p_inDiag[l_addr]*l_inV[r+1] + p_inUp[l_addr]*l_inV[r+2];
    };
		l_inV[0] = l_inV[1];
		l_inV[1] = l_inV[2];

  }; 
};

}
}

#endif
