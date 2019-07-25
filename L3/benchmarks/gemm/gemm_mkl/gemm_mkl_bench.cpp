#include "gemm_mkl_helper.hpp"

#define LOOP	4
using namespace std;

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

  TimePointType l_tp[3];

  // Cold Start
  l_tp[0] = chrono::high_resolution_clock::now();
  GEMM_MKL(m, k, n, alpha, beta, a, b, c);
  l_tp[1] = chrono::high_resolution_clock::now();
  
  // Hot benchmarking
  for (int i = 0; i < LOOP; i ++)
	GEMM_MKL(m, k, n, alpha, beta, a, b, c);
  l_tp[2] = chrono::high_resolution_clock::now();
  
  chrono::duration<double> l_durationSec_cold = l_tp[1] - l_tp[0];
  chrono::duration<double> l_durationSec_bench = l_tp[2] - l_tp[1];

  double gflops = 2. * (double)m * (double)k * (double)n;
  cout<<"--- "<<DISPLAY_GEMM_FUNC <<" ("<<m<<", "<<k<<", "<<n<<") ---"<<endl;
  cout<<"Performance\t(Cold Start):\t"<< (gflops / (double)l_durationSec_cold.count() * 1.e-9) << " GFLOPS"<<endl;
  cout<<"CPU Time\t(Cold Start):\t"<< ((double)l_durationSec_cold.count() * 1e3) << " ms"<<endl;
  cout<<"Performance\t(Benchmark):\t"<< (gflops / (double)l_durationSec_bench.count() * (double)LOOP * 1.e-9) << " GFLOPS"<<endl;
  cout<<"CPU Time\t(Benchmark):\t"<< ((double)l_durationSec_bench.count() / (double)LOOP * 1e3) << " ms"<<endl;

  free(a);
  free(b);
  free(c);

  return 0;
}


