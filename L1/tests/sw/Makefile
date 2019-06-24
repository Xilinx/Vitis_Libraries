# Copyright 2019 Xilinx, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifndef XILINX_VIVADO
	$(error Environment variable XILINX_VIVADO is required and should point to Vivado install area)
endif
SHELL = /bin/bash

BLAS_argInstrWidth	= 8
BLAS_pageSizeBytes	= 4096
BLAS_instrSizeBytes = 8 
BLAS_instrPageIdx 	= 0
BLAS_paramPageIdx		= 1
BLAS_statsPageIdx		= 2
BLAS_dataPageIdx		= 3
BLAS_maxNumInstrs		= 64
BLAS_dataType				= int
BLAS_resDataType		= int
BLAS_memWidthBytes	= 64

DFLAGS = -D BLAS_argInstrWidth=$(BLAS_argInstrWidth) \
				 -D BLAS_pageSizeBytes=$(BLAS_pageSizeBytes) \
				 -D BLAS_instrSizeBytes=$(BLAS_instrSizeBytes) \
				 -D BLAS_instrPageIdx=$(BLAS_instrPageIdx) \
				 -D BLAS_paramPageIdx=$(BLAS_paramPageIdx) \
				 -D BLAS_statsPageIdx=$(BLAS_statsPageIdx) \
				 -D BLAS_dataPageIdx=$(BLAS_dataPageIdx) \
				 -D BLAS_maxNumInstrs=$(BLAS_maxNumInstrs) \
				 -D BLAS_dataType=$(BLAS_dataType) \
				 -D BLAS_resDataType=$(BLAS_resDataType) \
				 -D BLAS_memWidthBytes=$(BLAS_memWidthBytes)

GCC_VERSION=6.2.0
GCC_PATH=${XILINX_VIVADO}/tps/lnx64
#CC = /bin/g++
CC = ${GCC_PATH}/gcc-${GCC_VERSION}/bin/g++

BOOST_INCLUDE=$(XILINX_VIVADO)/tps/boost_1_64_0
BOOST_LIB=$(XILINX_VIVADO)/lib/lnx64.o

INCLUDE_CFLAGS = -I$(XILINX_VIVADO)/include \
								 -I$(BOOST_INCLUDE) \
								 -I./src/c++ \
								 -I../../..

LIB_LFLAGS = -L$(BOOST_LIB) \
						 -lboost_iostreams -lz \
						 -lstdc++ \
						 -lrt \
						 -pthread
CFLAGS = -g -O0 -std=c++11 \
				 $(INCLUDE_CFLAGS)

LFLAGS = $(HOST_LIB_LFLAGS) \
				 -Wl,--rpath=$(BOOST_LIB)

OUT_DIR = out_test
GEN_BIN_EXE = $(OUT_DIR)/blas_gen_bin.exe
GEN_BIN_SO = $(OUT_DIR)/blas_gen_wrapper.so

.PHONY: all

$(GEN_BIN_SO) : ./src/c++/blas_gen_wrapper.cpp |  $(OUT_DIR)
	@echo "***** Link testcase generator executable *****"
	@echo "INFO: LIB IS" ${LIB_LFLAGS}
	@echo "*************************************************"
	$(CC) $(CFLAGS) $(DFLAGS) -shared -fPIC -fdata-sections -ffunction-sections -Wl,--gc-sections $^ -o $@

$(GEN_BIN_EXE) : ./src/c++/* | $(OUT_DIR)
	@echo "***** Compile testcase generator executable *****"
	@echo "INFO: INCLUDE IS " ${INCLUDE_CFLAGS}
	@echo "INFO: LIB IS" ${LIB_LFLAGS}
	@echo "*************************************************"
	$(CC) $(CFLAGS) $(LFLAGS) $(DFLAGS) -fdata-sections -ffunction-sections -Wl,--gc-sections src/c++/blas_gen_bin.cpp -o $@

clean :
	$(RM) -r ${OUT_DIR} 

$(OUT_DIR) :
	@echo "************* Creating DIR $@ *************"
	mkdir $@
