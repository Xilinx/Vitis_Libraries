#include <iostream>
#include "trmv.hpp"

#define DataType BLAS_dataType
#define N BLAS_size
#define EntInPar BLAS_entriesInParallel

void trmv_inst(DataType inlow[N], DataType indiag[N], DataType inup[N], DataType x[N], DataType y[N]){
  trmv<DataType,N,NCU>(inlow,indiag,inup,x,y);
};

int main(){

  DataType inlow[N];
  DataType indiag[N];
  DataType inup[N];
  DataType inrhs[N];
  DataType sol[N];


  for(int i=0;i<N;i++){
    inlow[i] =  100.0 + i;
    indiag[i] = i;
    inup[i] = 50 + i;
    inrhs[i] = 0.0;
    sol[i] = i;
  };
  inlow[0] = 0.0;
  inup[N-1] = 0.0;
    
  inrhs[0] = indiag[0] * sol[0] + inup[0] * sol[1]; 
  inrhs[N-1] = inlow[N-1] * sol[N-2] + indiag[N-1] * sol[N-1];
  for(int i=1;i<N-1;i++){
    inrhs[i] = inlow[i] * sol[i-1] + indiag[i] * sol[i] + inup[i] * sol[i+1]; 
  }; 

  for(int k=0;k<N;k++){
    std::cout << inrhs[k] << " ";
    inrhs[k] = 0.0;
  };
  std::cout << std::endl;

  // compute
  top_trmv(inlow,indiag,inup,sol,inrhs);


  for(int k=0;k<N;k++){
    std::cout << inrhs[k] << " ";
  }
  std::cout << std::endl;
};
