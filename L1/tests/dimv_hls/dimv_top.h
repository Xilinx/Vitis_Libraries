#ifndef TRMV_TOP_H
#define TRMV_TOP_H

#include "dimv.hpp"

void dimv_top(
  BLAS_dataType p_in[BLAS_size][BLAS_numDiag], 
  BLAS_dataType p_inV[BLAS_size],
  unsigned int p_n,
  BLAS_dataType p_outV[BLAS_size]
);
#endif
