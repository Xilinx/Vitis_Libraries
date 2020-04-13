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

# path the the matching engine
KRNL_PATH = ../../../L2/tests/CFBlackScholes
KRNL_NAME = bs_kernel.xclbin

# Target check
TARGET ?= sw_emu
ifeq ($(filter $(TARGET),sw_emu hw_emu hw),)
$(error TARGET is not sw_emu, hw_emu or hw)
endif

ifeq ($(TARGET),sw_emu)
RUN_ENV += export XCL_EMULATION_MODE=sw_emu;
EMU_CONFIG = $(OUTPUT_DIR)/emconfig.json
else ifeq ($(TARGET),hw_emu)
RUN_ENV += export XCL_EMULATION_MODE=hw_emu;
EMU_CONFIG = $(OUTPUT_DIR)/emconfig.json
else ifeq ($(TARGET),hw)
RUN_ENV += echo "TARGET=hw";
EMU_CONFIG =
endif

# default to u200
DEVICE ?= u200

ifneq (,$(findstring u50,$(DEVICE)))
        DEVICE_PART := u50
else ifneq (,$(findstring u200,$(DEVICE)))
        DEVICE_PART := u200
else ifneq (,$(findstring u250,$(DEVICE)))
        DEVICE_PART := u250
else ifneq (,$(findstring u280,$(DEVICE)))
        DEVICE_PART := u280
else
        DEVICE_PART := unknown
endif

# executable
EXE_NAME = cfBSMEngine_example
EXE_EXT ?= exe
EXE_FILE ?= $(EXE_NAME)$(if $(EXE_EXT),.,)$(EXE_EXT)

SRC_DIR = .
HOST_ARGS =
OUTPUT_DIR = ./output

SRCS := $(shell find $(SRC_DIR) -maxdepth 1 -name '*.cpp')
OBJ_FILES := $(addsuffix .o, $(basename $(SRCS)))
EXTRA_OBJS :=


CPPFLAGS = -std=c++11 -g -O3 -Wall -Wno-unknown-pragmas -c -DDEVICE_PART=$(DEVICE_PART) -I$(XILINX_FINTECH_L3_INC) -I$(XILINX_FINTECH_L2_INC) -I$(XILINX_XCL2_DIR) -I$(XILINX_XRT)/include
LDFLAGS = -lpthread -lstdc++ -lxilinxfintech -lxilinxopencl -L$(XILINX_FINTECH_LIB_DIR) -L$(XILINX_XRT)/lib

# simulation
$(OUTPUT_DIR)/emconfig.json :
	emconfigutil --platform $(DEVICE) --od $(OUTPUT_DIR)


.PHONY: output host clean cleanall run

host: output $(EXE_FILE)

output:
	@mkdir -p ${OUTPUT_DIR}

run: host kernel $(EMU_CONFIG)
	@$(RUN_ENV) \
	${OUTPUT_DIR}/$(EXE_FILE) $(HOST_ARGS)

# create symbolic link to L2 kernel
kernel:
	@ln -sf '$(KRNL_PATH)/xclbin_$(DEVICE)_$(TARGET)/$(KRNL_NAME)'
	@if [ ! -f $(KRNL_NAME) ]; then echo -e '\n\nThe $(TARGET) kernel for $(KRNL_NAME) does not exist, refer to README for instructions to build...\n\n'; exit -1 ; fi

clean:
	@$(RM) $(KRNL_NAME)

cleanall: clean
	@$(RM) -rf $(OUTPUT_DIR)


%.o:%.cpp
	@echo $(notdir $(@))
	$(CXX) $(CPPFLAGS) -o ${OUTPUT_DIR}/$(notdir $(@)) -c $<


$(EXE_FILE): $(OBJ_FILES)
	$(CXX) -o ${OUTPUT_DIR}/$@ $(addprefix ${OUTPUT_DIR}/,$(notdir $(OBJ_FILES))) $(LDFLAGS)
