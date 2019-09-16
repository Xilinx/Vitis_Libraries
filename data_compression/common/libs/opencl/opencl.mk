# Definition of include file locations
OPENCL12_INCLUDE:= $(XILINX_XRT)/include/

opencl_CXXFLAGS=-I$(OPENCL12_INCLUDE)

OPENCL_LIB:=$(XILINX_XRT)/lib/
opencl_LDFLAGS=-L$(OPENCL_LIB) -lOpenCL -pthread
