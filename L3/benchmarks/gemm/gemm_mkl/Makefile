ifndef MKLROOT
$(error MKLROOT not defined)
endif

CXX	= g++
CFLAGS	= -O2 -DMKL_ILP64 -m64 -fopenmp -static -std=c++11 -I${MKLROOT}/include
MKL_LIBS = -L$(MKLROOT)/lib/intel64 -Wl,--start-group -lmkl_intel_lp64 -lmkl_gnu_thread -lmkl_core -Wl,--end-group

all : sgemm

sgemm  : sgemm_mkl_bench sgemm_mkl_gen

sgemm_mkl_bench  : gemm_mkl_bench.cpp
	$(CXX) $(CFLAGS) -DUSE_FLOAT_PRECISION -o $@ $^ $(MKL_LIBS) -lpthread -lm

dgemm_mkl_bench  : gemm_mkl_bench.cpp
	$(CXX) $(CFLAGS) -DUSE_DOUBLE_PRECISION -o $@ $^ $(MKL_LIBS) -lpthread -lm

sgemm_mkl_gen: gemm_mkl_gen_bin.cpp
	$(CXX) $(CFLAGS) -DUSE_FLOAT_PRECISION -o $@ $^ $(MKL_LIBS) -lpthread -lm

dgemm_mkl_gen: gemm_mkl_gen_bin.cpp
	$(CXX) $(CFLAGS) -DUSE_DOUBLE_PRECISION -o $@ $^ $(MKL_LIBS) -lpthread -lm

clean:
	rm -f *.o *~ sgemm_mkl_bench dgemm_mkl_bench sgemm_mkl_gen dgemm_mkl_gen
