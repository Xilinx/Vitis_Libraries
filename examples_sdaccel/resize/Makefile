#***************************************************************************
#	Copyright (c) 2019, Xilinx, Inc.
#	All rights reserved.

#	Redistribution and use in source and binary forms, with or without modification, 
#	are permitted provided that the following conditions are met:

#	1. Redistributions of source code must retain the above copyright notice, 
#	this list of conditions and the following disclaimer.

#	2. Redistributions in binary form must reproduce the above copyright notice, 
#	this list of conditions and the following disclaimer in the documentation 
#	and/or other materials provided with the distribution.

#	3. Neither the name of the copyright holder nor the names of its contributors 
#	may be used to endorse or promote products derived from this software 
#	without specific prior written permission.

#	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
#	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
#	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
#	IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
#	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
#	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
#	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
#	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
#	EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#***************************************************************************/



# ----------------------------------------------------------
#  Global Info
# ----------------------------------------------------------
   PLATFORM := $(PLATFORM_PATH)/xilinx_u200_xdma_201830_2/xilinx_u200_xdma_201830_2.xpfm

# Source Code
COMMON_REPO := $(XLNX_SRC_PATH)/examples_sdaccel/resize

# ----------------------------------------------------------
# Tools/Plaform
# ----------------------------------------------------------
CLC ?= xocc
#CLC:=$(XILINX_SDX)/bin/xocc
LDCLC   := $(CLC)
DEVICES := $(PLATFORM)


# ----------------------------------------------------------
#  Tool Flags
# ----------------------------------------------------------
XP_VIVADO_PROPS += --xp vivado_prop:run.impl_1.strategy=Performance_Explore
LDCLFLAGS       += $(XP_VIVADO_PROPS)

   # TODO: Need to sort this include
   HLS_DIR = $(shell dirname $(shell dirname $(shell which vivado_hls)))
   CXXFLAGS += -I$(HLS_DIR)/include

#XILINX_VIVADO_HLS:= /proj/xbuilds/2018.3_released/installs/lin64/Vivado/2018.3

# ----------------------------------------------------------
# OpenCL, OpenCV
# ----------------------------------------------------------
#opencv_LDFLAGS := -lopencv_core -lopencv_imgproc -lopencv_highgui #-lopencv_imgcodecs

  # Library directories
  SDA_LIB:=$(XILINX_SDX)/lib/lnx64.o
  OPENCL_LIB:=$(XILINX_CL_PATH)/lib64

  opencl_LDFLAGS := -L$(OPENCL_LIB) -L$(SDA_LIB) -lOpenCL -lpthread
  opencv_LDFLAGS += -L$(XILINX_SDX)/lnx64/tools/opencv
  opencv_LDFLAGS += -lopencv_core -lopencv_imgproc -lopencv_highgui


# ----------------------------------------------------------
# Host Application(s)
# ----------------------------------------------------------
# xcl
xcl2_SRCS := $(COMMON_REPO)/xcl2.cpp
xcl2_HDRS := $(COMMON_REPO)/xcl2.hpp

# Application1:
APP_NAME := resize
$(APP_NAME)_SRCS := $(COMMON_REPO)/xf_$(APP_NAME)_tb.cpp $(xcl2_SRCS)
$(APP_NAME)_HDRS := $(xcl2_HDRS)
$(APP_NAME)_CXXFLAGS := -I. -I$(XLNX_SRC_PATH)/include -I${COMMON_REPO}
$(APP_NAME)_LDFLAGS  := $(opencv_LDFLAGS) $(opencl_LDFLAGS) -Wall -lrt

  $(APP_NAME)_CXXFLAGS += -I$(XILINX_CL_PATH)/include -I$(XILINX_SDX)/include

EXES += $(APP_NAME)


# ----------------------------------------------------------
# Kernel(s)
# ----------------------------------------------------------
#Kernel1:
KERNEL=resize
krnl_$(KERNEL)_SRCS := $(COMMON_REPO)/xf_resize_accel.cpp
krnl_$(KERNEL)_CLFLAGS := --kernel resize_accel -I. -I$(COMMON_REPO) -I$(XLNX_SRC_PATH)/include --xp prop:kernel.resize_accel.kernel_flags="-std=c++0x"
XOS += krnl_$(KERNEL)
$(APP_NAME)_KRNLS += krnl_$(KERNEL)

#Kernel2:
#KERNEL2=pyr_down
#krnl_$(KERNEL2)_SRCS :=
#krnl_$(KERNEL2)_CFLAGS :=
#XOS += krnl_$(KERNEL2)
#$(APP_NAME)_KRNLS += krnl_$(KERNEL2)



# ----------------------------------------------------------
#  Actual Body
# ----------------------------------------------------------
# Defines the prefix for each kernel.
XCLBIN_DIR=xclbin

#  $(1) - base name for this kernel
#  $(2) - compilation target (i.e. hw, hw_emu, sw_emu)
#  $(3) - device name (i.e. xilinx:adm-pcie-ku3:1ddr:3.0)

# xclbin
krnl_$(APP_NAME)_XOS := $($(APP_NAME)_KRNLS:%=$(XCLBIN_DIR)/%.xo)
XCLBINS=krnl_$(APP_NAME)

# mk_exe - build an exe from host code
define mk_exe
$(1): $($(1)_SRCS) $($(1)_HDRS)
	@echo "Creating \"$$@\""
	$(CXX) $(CXXFLAGS) $($(1)_CXXFLAGS) $($(1)_SRCS) -o $$@ $($(1)_LDFLAGS) $(LDFLAGS) -DHLS_NO_XIL_FPO_LIB
EXE_GOALS+= $(1)
endef

# mk_xo - create an xo from a set of kernel sources
define mk_xo
$(XCLBIN_DIR)/$(1).xo: $($(1)_SRCS) $($(1)_HDRS) $(call device2dep,$(3))
	@echo "Creating \"$$@\""
	mkdir -p ${XCLBIN_DIR}
	$(CLC) -c $(CLFLAGS) $($(1)_CLFLAGS) $($(1)_$(call device2sandsa,$(3))_CLFLAGS) -o $$@ -t $(2) --platform $(3) $($(1)_SRCS) -DHLS_NO_XIL_FPO_LIB -D__SDSVHLS__
XO_GOALS+= $(XCLBIN_DIR)/$(1).xo
endef

# mk_xclbin - create an xclbin from a set of krnl sources
define mk_xclbin
$(XCLBIN_DIR)/$(1).xclbin: $($(1)_XOS)
	@echo "Creating \"$$@\""
	mkdir -p ${XCLBIN_DIR}
	$(LDCLC) -l $(LDCLFLAGS) $($(1)_LDCLFLAGS) $($(1)_$(call device2sandsa,$(3))_LDCLFLAGS) -o $$@ -t $(2) --platform $(3) $($(1)_XOS) -DHLS_NO_XIL_FPO_LIB -D__SDSVHLS__
XCLBIN_GOALS+= $(XCLBIN_DIR)/$(1).xclbin
endef

$(foreach exe,$(EXES),$(eval $(call mk_exe,$(exe))))
$(foreach xo,$(XOS),$(foreach target,$(TARGETS),$(foreach device,$(DEVICES),$(eval $(call mk_xo,$(xo),$(target),$(device))))))
$(foreach xclbin,$(XCLBINS),$(foreach target,$(TARGETS),$(foreach device,$(DEVICES),$(eval $(call mk_xclbin,$(xclbin),$(target),$(device))))))

.PHONY: all
all: $(EXE_GOALS) $(XCLBIN_GOALS)

.PHONY: clean
clean:
	rm -rf $(EXE_GOALS) $(XCLBIN_DIR)/* _x xclbin .run sdaccel_* TempConfig system_estimate.xtxt *.rpt *.png
	rm -rf src/*.ll _xocc_* .Xil emconfig.json $(EXTRA_CLEAN) dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

#.PHONY: copy
#copy:
#	/bin/cp -prf ${COMMON_REPO}/*.h ${COMMON_REPO}/*.hpp ${COMMON_REPO}/*.cpp .
