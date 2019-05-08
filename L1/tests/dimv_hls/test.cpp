#include <iostream>
#include "util.h"
#include "dimv_top.h"

#define DataType BLAS_dataType
#define N BLAS_size
#define NumDiag BLAS_numDiag
#define EntInPar BLAS_entriesInParallel

int main(int argc, char** argv){
  if (argc < 2) {
    std::cout << "ERROR: passed %d arguments instead of %d, exiting" << argc << 4 << std::endl;
    std::cout << " Usage:" << std::endl;
    std::cout << "    test.exe diagonal_matrix_input_file number_of_entries_along_diagonal_line" << std::endl;
    std::cout << " Example Usage:" << std::endl;
    std::cout << "    test.exe ./data/A1.csv 8192" << std::endl;
    return EXIT_FAILURE;
  }
  
  std::string l_fileName(argv[1]);
  unsigned int l_n = atoi(argv[2]);

  DataType l_in[N][NumDiag];
  DataType l_inV[N];
  DataType l_outVref[N];
  DataType l_outV[N];

  DataType l_init = 1.0/2;

  for(int i=0;i<N;++i){
    l_inV[i] = l_init;
    l_init++;
  };
    
  int l_res = 0;
  l_res = readDiag<DataType, N, NumDiag>(l_fileName, l_n, l_in);
  if (l_res != 0) {
    std::cout <<"ERROR: failed to read input matrix file, exiting" << std::endl;
    return EXIT_FAILURE;
  }
  // compute
  dimv_top(l_in, l_inV, l_n, l_outV);

  // compute golden reference
  diagMult<DataType, N, NumDiag>(l_in, l_inV, l_n, l_outVref);

  // compare
  for (unsigned int i=0; i<N; ++i) {
    if (l_outVref[i] != l_outV[i]) {
      std::cout <<"ERROR: value at index " <<i<< " differs. " 
                <<"refVal=" << l_outVref[i] << " outVal=" << l_outV[i] << std::endl;
    }
  }
  return 0;

};
