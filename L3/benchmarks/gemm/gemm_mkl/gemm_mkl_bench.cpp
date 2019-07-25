#include <cstdlib>
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

  double flops = 2. * (double)m * (double)k * (double)n;

  cout << std::string("DATA_CSV:,Type,Thread,Func,M,K,N,")
    + "TimeApiMs,"
    + "EffApiPct,PerfApiTops\n";
  cout << "DATA_CSV:,"<<"Cold Start,"<<getenv("OMP_NUM_THREADS")<<","<<DISPLAY_GEMM_FUNC<<","<<m<<","<<k<<","<<n<<","
	<< ((double)l_durationSec_cold.count() * 1e3) << ","
	<< "N/A," << (flops / (double)l_durationSec_cold.count() * 1.e-9) <<endl;
  cout << "DATA_CSV:,"<<"Benchmark,"<<getenv("OMP_NUM_THREADS")<<","<<DISPLAY_GEMM_FUNC<<","<<m<<","<<k<<","<<n<<","
	<< ((double)l_durationSec_bench.count() / (double)LOOP * 1e3) << ","
	<< "N/A," << (flops / (double)l_durationSec_bench.count() * (double)LOOP * 1.e-9) <<endl;


  free(a);
  free(b);
  free(c);

  return 0;
}


