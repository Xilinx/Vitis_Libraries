#include "dimv_top.h"

void dimv_top(
  BLAS_dataType p_in[BLAS_size][BLAS_numDiag], 
  BLAS_dataType p_inV[BLAS_size],
  unsigned int p_n,
  BLAS_dataType p_outV[BLAS_size]
) {
  
  xf::linear_algebra::blas::dimv<BLAS_dataType, BLAS_size, BLAS_numDiag, BLAS_entriesInParallel>(
    p_in,
    p_inV,
    p_n,
    p_outV
  );
}
