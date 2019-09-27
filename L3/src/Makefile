
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


EXE_NAME = libxilinxfintech
EXE_EXT ?= so
EXE_FILE ?= $(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)

SRC_DIR = .
HOST_ARGS =
RUN_ENV =
INCLUDE_DIR = ../include
L2_INCLUDE_DIR = ../../L2/include/
OUTPUT_DIR = ./output

SRCS := $(shell find $(SRC_DIR) -maxdepth 4 -name '*.cpp')
SRCS += $(shell find $(XILINX_XCL2_DIR) -maxdepth 1 -name '*.cpp')
OBJ_FILES := $(addsuffix .o, $(basename $(SRCS)))
EXTRA_OBJS :=


CPPFLAGS = -DVITIS_PLATFORM=$(VITIS_PLATFORM) \
			-std=c++11 -O3 -g -Wall -Wno-unknown-pragmas -c -fPIC \
			-I$(L2_INCLUDE_DIR) \
			-I$(INCLUDE_DIR) \
			-Imodels/cf_black_scholes/include \
			-Imodels/cf_black_scholes_merton/include \
			-Imodels/cf_garman_kohlhagen/include \
			-Imodels/hcf/include \
			-Imodels/m76/include \
			-Imodels/heston_fd/include \
			-Imodels/mc_american/include \
			-Imodels/mc_european/include \
			-Imodels/binomial_tree/include \
			-Imodels/mc_european_dje/include \
			-Imodels/binomial_tree/include \
			-Imodels/pop_mcmc/include \
			-Iutils/linear_interpolation/include \
			-I$(XILINX_XCL2_DIR) \
			-I$(XILINX_XRT)/include

LDFLAGS = -shared -lxilinxopencl -lpthread -lrt -lstdc++ -L$(XILINX_XRT)/lib



.PHONY: output all clean cleanall rnn

all: output $(EXE_FILE)

output:
	@mkdir -p ${OUTPUT_DIR}

clean:
	@$(RM) -rf $(OUTPUT_DIR)

cleanall: clean

run:
	@echo "library not executable"


%.o:%.cpp
	@echo $(notdir $(@))
	$(CXX) $(CPPFLAGS) -o ${OUTPUT_DIR}/$(notdir $(@)) -c $<

$(EXE_FILE): $(OBJ_FILES)
	$(CXX) $(addprefix ${OUTPUT_DIR}/,$(notdir $(OBJ_FILES))) -o ${OUTPUT_DIR}/$@ $(LDFLAGS)
