#include "gemm_mkl_helper.hpp"

int main(int argc, char **argv) {

  if (argc < 4) {
    printf("Usage: gemm_mkl m k n\n");
    return EXIT_FAILURE;
  }

  int m=atoi(argv[1]), k=atoi(argv[2]), n=atoi(argv[3]);
  XFBLAS_dataType *a, *b, *c, alpha = 1., beta = 1.;

  a = createMat(m, k);
  b = createMat(k, n);
  c = createMat(m, n);

  // Generating Random Input and Golden Output
  GEMM_MKL(m, k, n, alpha, beta, a, b, c);

  // TODO, generate bin via C++ ofstream 
  FILE* pFile;
  pFile = fopen ("../data/matA.bin", "wb");
  fwrite( a, sizeof(XFBLAS_dataType), m*k, pFile);
  fclose(pFile);
  pFile = fopen ("../data/matB.bin", "wb");
  fwrite( b, sizeof(XFBLAS_dataType), k*n, pFile);
  fclose(pFile);
  pFile = fopen ("../data/matC.bin", "wb");
  fwrite( c, sizeof(XFBLAS_dataType), m*n, pFile);
  fclose(pFile);

  free(a);
  free(b);
  free(c);

  return 0;
}


