#
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
#


ifndef XILINX_XRT
$(error "XILINX_XRT should be set on or after 2019.2 release.")
endif

ifndef XILINX_XCL2_DIR
$(error "XILINX_XCL2_DIR should be set to the directory containing xcl2")
endif

ifndef XILINX_FINTECH_L3_INC
$(error "XILINX_FINTECH_L3_INC should be set to path of the fintech header files.")
endif

ifndef XILINX_FINTECH_L2_INC
$(error "XILINX_FINTECH_L2_INC should be set to path of the fintech header files.")
endif

ifndef XILINX_FINTECH_LIB_DIR
$(error "XILINX_FINTECH_LIB_DIR should be set to the path of the directory containing the fintech library")
endif


EXE_NAME = binomial_example
EXE_EXT ?= exe
EXE_FILE ?= $(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)


SRC_DIR = .
HOST_ARGS =
RUN_ENV =
OUTPUT_DIR = ./output


SRCS := $(shell find $(SRC_DIR) -maxdepth 1 -name '*.cpp')
OBJ_FILES := $(addsuffix .o, $(basename $(SRCS)))
EXTRA_OBJS :=


CPPFLAGS = -std=c++11 -g -O3 -Wall -Wno-unknown-pragmas -c -I$(XILINX_FINTECH_L3_INC) -I$(XILINX_FINTECH_L2_INC) -I$(XILINX_XCL2_DIR) -I$(XILINX_XRT)/include 
LDFLAGS = -lpthread -lstdc++ -lxilinxfintech -lxilinxopencl -L$(XILINX_FINTECH_LIB_DIR) -L$(XILINX_XRT)/lib


.PHONY: output all clean cleanall run

all: output $(EXE_FILE)

output:
	@mkdir -p ${OUTPUT_DIR}

clean:
	@$(RM) -rf $(OUTPUT_DIR)

cleanall: clean

run:
	${OUTPUT_DIR}/$(EXE_FILE) $(HOST_ARGS)


%.o:%.cpp
	@echo $(notdir $(@))
	$(CXX) $(CPPFLAGS) -o ${OUTPUT_DIR}/$(notdir $(@)) -c $<


$(EXE_FILE): $(OBJ_FILES)
	$(CXX) -o ${OUTPUT_DIR}/$@ $(addprefix ${OUTPUT_DIR}/,$(notdir $(OBJ_FILES))) $(LDFLAGS)
