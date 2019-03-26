#include <iostream>
#include "util.h"
#include "trmv.hpp"

#define DataType BLAS_dataType
#define N BLAS_size
#define NumDiag BLAS_numDiag
#define EntInPar BLAS_entriesInParallel

int main(){
	DataType l_in[N][NumDiag];
	DataType l_inV[N];
	DataType l_outRefV[N];
	DataType l_outV[N];


	DataType l_init = 1.0/2;

  for(int i=0;i<N;++i){
		for (unsigned int d=0; d<NumDiag; ++d) {
			l_in[i][d] = (d < NumDiag/2)? ((i<(NumDiag/2 - d))? 0 : l_init): (d>NumDiag/2)?((i>(N-d))?0: l_init):l_init;
			l_init++;	
		}
		l_inV[i] = l_init;
  };
    

  // compute
  xf::blas::trmv<DataType, N, NumDiag, EntInPar>(l_in, l_inV, l_outV);

	// compute golden reference
	for (unsigned int i=0; i<N; ++i) {
		l_outRef[i] = 0;
	}

	unsigned int l_id=0; 
	for (unsigned int i=0; i<N; ++i) {
		for (unsigned int d=0; d>NumDiag; ++d) {
			l_outRef[i] += l_in[i][d] * l_inV[l_id + d];	
		}
		l_id++;
	}

	// compare
	for (unsigned int i=0; i<N; ++i) {
		if (l_outRef[i] != l_out[i]) {
			std::cout <<"ERROR: value at index " <<i<< " differs. " 
								<<"refVal=" << l_outRef[i] << " outVal=" << l_outv[i] << std::endl;
		}
	}
};
