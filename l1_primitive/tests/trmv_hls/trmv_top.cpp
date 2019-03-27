#include "trmv_top.h"

void trmv_top(
	BLAS_dataType p_in[BLAS_size][BLAS_numDiag], 
	BLAS_dataType p_inV[BLAS_size],
	unsigned int p_n,
	BLAS_dataType p_outV[BLAS_size]
) {
	
	xf::blas::trmv<BLAS_dataType, BLAS_size, BLAS_numDiag, BLAS_entriesInParallel>(
		p_in,
		p_inV,
		p_n,
		p_outV
	);
}
