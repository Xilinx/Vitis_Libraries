# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
# vitis makefile-generator v2.0.10

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw_emu/hw> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  NOTE: For embedded platform the following setup steps are required:"
	$(ECHO) "      a.If the platform and common-image are downloaded from Xilinx Download Center(Suggested):"
	$(ECHO) "        Run the sdk.sh script from the common-image directory to install sysroot using the command : ./sdk.sh -y -d ./ -p "
	$(ECHO) "        Unzip the rootfs file : gunzip ./rootfs.ext4.gz"
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "      b.User could also define SYSROOT, K_IMAGE and ROOTFS by themselves: "
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "        export K_IMAGE=< path-to-Image-files >"
	$(ECHO) "        export ROOTFS=< path-to-rootfs >"
	$(ECHO) ""
	$(ECHO) "  make clean"
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

############################## Setting up Project Variables ##############################

MK_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
XF_PROJ_ROOT ?= $(shell bash -c 'export MK_PATH=$(MK_PATH); echo $${MK_PATH%/L2/*}')
CUR_DIR := $(patsubst %/,%,$(dir $(MK_PATH)))
XFLIB_DIR = $(XF_PROJ_ROOT)

# setting default value
TARGET ?= hw_emu

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := u200
endif

# add warnning for sw_emu
ifeq ($(TARGET),sw_emu)
$(error Error: The sw_emu target is no longer supported starting from 2025.1.)
endif
# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST += u200
PLATFORM_BLOCKLIST += zc

include ./utils.mk
TEMP_DIR := _x_temp.$(TARGET).$(PLATFORM_NAME)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(PLATFORM_NAME)
BUILD_DIR := build_dir.$(TARGET).$(PLATFORM_NAME)
ifneq ($(RESULT_DIR),)
BUILD_DIR = $(RESULT_DIR)
endif
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(PLATFORM_NAME)
EMCONFIG := $(BUILD_DIR)/emconfig.json
XCLBIN_DIR := $(CUR_DIR)/$(BUILD_DIR)
export XCL_BINDIR = $(XCLBIN_DIR)

EXE_FILE_DEPS :=
BINARY_CONTAINERS_DEPS :=
RUN_DEPS :=

# get global setting
ifeq ($(HOST_ARCH), x86)
CXXFLAGS += -fmessage-length=0 -I$(CUR_DIR)/src/ -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include -std=c++17 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -Wl,--as-needed -lOpenCL -lxrt_coreutil -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += -I$(CUR_DIR)/src/ -fmessage-length=0 --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I$(XILINX_HLS)/include -std=c++17 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(SYSROOT)/usr/lib -Wl,--as-needed -lxilinxopencl -lxrt_coreutil 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
endif
CXXFLAGS += $(EXTRA_CXXFLAGS)
VPP_FLAGS += $(EXTRA_VPP_FLAGS)

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
HOST_SRCS += $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/alpha.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/buffer.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/frame.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/idec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/io.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/quant.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/tree.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/vp8.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/vp8l.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/webp.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/demux/anim_decode.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/demux/demux.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/alpha_processing.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/alpha_processing_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/alpha_processing_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/alpha_processing_sse41.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/cpu.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec_clip_tables.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec_mips32.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec_neon.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/dec_sse41.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/filters.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/filters_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/filters_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_neon.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/rescaler.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/rescaler_mips32.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/rescaler_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/rescaler_neon.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/rescaler_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/upsampling.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/upsampling_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/upsampling_neon.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/upsampling_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/yuv.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/yuv_mips32.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/argb.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/argb_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/argb_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/yuv_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/yuv_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/cost.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/cost_mips32.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/cost_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/cost_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc_avx2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc_mips32.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc_neon.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/enc_sse41.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_enc.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_enc_mips32.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_enc_mips_dsp_r2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_enc_neon.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_enc_sse2.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/lossless_enc_sse41.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/alpha.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/analysis.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/backward_references.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/config.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/cost.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/delta_palettization.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/filter.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/frame.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/histogram.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/iterator.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/near_lossless.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/picture.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/picture_csp.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/picture_psnr.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/picture_rescale.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/picture_tools.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/quant.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/syntax.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/token.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/tree.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/vp8l.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/webpenc.c $(XFLIB_DIR)/L2/demos/webpEnc/host/jpegdec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/metadata.c $(XFLIB_DIR)/L2/demos/webpEnc/host/pngdec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/webpdec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/example_util.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/mux/anim_encode.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/mux/muxedit.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/mux/muxinternal.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/mux/muxread.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/bit_reader.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/color_cache.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/filters.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/huffman.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/quant_levels_dec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/random.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/rescaler.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/thread.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/utils.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/bit_writer.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/huffman_encode.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/quant_levels.c $(XFLIB_DIR)/L2/demos/webpEnc/host/cwebp.c $(XFLIB_DIR)/L2/demos/webpEnc/host/wicdec.c $(XFLIB_DIR)/L2/demos/webpEnc/host/create_kernel.c $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/kernel/oclHelper.cpp $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/kernel/oclErrorCodes.cpp $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/profiling.c 
CXXFLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/webpEnc/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/ -I $(XFLIB_DIR)/L2/demos/webpEnc/kernel/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dec/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/dsp/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/enc/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/mux/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/webp/ -I $(XFLIB_DIR)/L2/demos/webpEnc/host/src/utils/
CXXFLAGS += -O3 -fpermissive -DWEBP_NBINSTANCES=1 -DWEBPDSA=\"$(PLATFORM_NEW)\" -DHAVE_MALLOC_H -DHAVE_PTHREAD -DWEBP_HAVE_PNG -DWEBP_HAVE_TIFF -DWEBP_USE_THREAD -DWEBP_HAVE_GIF -DFPGA_DEVICE 
LDFLAGS +=  -lrt -Wno-narrowing -DVERBOSE -L$(XILINX_VITIS)/lib/lnx64.o -lxilinxopencl -pthread -lpng

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := cwebp
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(INSTANCE_FILES)  $(EXE_FILE_DEPS)

HOST_ARGS :=  -xclbin $(BUILD_DIR)/kernel.xclbin list.rst -use_ocl -q 80 -o images
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u200/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u200.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/webpEnc -I $(XFLIB_DIR)/L2/demos/webpEnc/kernel

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/webpEnc -I $(XFLIB_DIR)/L2/demos/webpEnc/kernel

endif

######################### binary container global settings ##########################
VPP_FLAGS_webp_IntraPredLoop2_NoOut_1 +=  -D KERNEL_NAME=webp_IntraPredLoop2_NoOut_1
VPP_FLAGS_webp_IntraPredLoop2_NoOut_1 += --hls.clock 250000000:webp_IntraPredLoop2_NoOut_1
VPP_FLAGS_webp_2_ArithmeticCoding_1 +=  -D KERNEL_NAME=webp_2_ArithmeticCoding_1
VPP_FLAGS_webp_2_ArithmeticCoding_1 += --hls.clock 250000000:webp_2_ArithmeticCoding_1
ifneq ($(HOST_ARCH_temp), x86)
VPP_LDFLAGS_kernel += --clock.defaultFreqHz 250000000
else
VPP_LDFLAGS_kernel += --kernel_frequency 250
endif

ifneq ($(PACKAGE_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/kernel.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/kernel_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/kernel.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/webp_IntraPredLoop2_NoOut_1.xo: $(XFLIB_DIR)/L2/demos/webpEnc/kernel/vp8_hls_pred.cpp 
	$(ECHO) "Compiling Kernel: webp_IntraPredLoop2_NoOut_1"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_webp_IntraPredLoop2_NoOut_1) $(VPP_FLAGS) -k webp_IntraPredLoop2_NoOut_1 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_kernel_OBJS += $(TEMP_DIR)/webp_IntraPredLoop2_NoOut_1.xo
$(TEMP_DIR)/webp_2_ArithmeticCoding_1.xo: $(XFLIB_DIR)/L2/demos/webpEnc/kernel/vp8_hls_ac.cpp 
	$(ECHO) "Compiling Kernel: webp_2_ArithmeticCoding_1"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_webp_2_ArithmeticCoding_1) $(VPP_FLAGS) -k webp_2_ArithmeticCoding_1 -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_kernel_OBJS += $(TEMP_DIR)/webp_2_ArithmeticCoding_1.xo

BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_kernel_OBJS) 
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/kernel $(VPP_LDFLAGS)  $(VPP_LDFLAGS_kernel) $(AIE_LDFLAGS)   -o $@ $^

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(EXE_FILE_DEPS)
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $(filter %.s %.c %.cc %.cpp %cxx, $^) $(CXXFLAGS) $(LDFLAGS)

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

############################## Package final xclbin or sd_card folder (for embedded) ##############################
# AIE_ON_X86 Flow
ifeq ($(pcie_versal), on)
	@echo "### ***** running AIE ON_X86 ***** ###"
	${VPP} -p $(VPP_PACKAGE) -t ${TARGET} -f ${XPLATFORM} ${AIE_CONTAINER} ${BINARY_CONTAINERS} -o $(BINARY_CONTAINERS_PKG) --package.boot_mode ospi
	@echo "### ***** xclbin generation done! ***** ###"
endif

ifeq ($(SD_CARD_NEEDED), on)
RUN_SCRIPT := $(BUILD_DIR)/run_script.sh
$(RUN_SCRIPT):
	rm -rf $(RUN_SCRIPT)
	@echo 'export LD_LIBRARY_PATH=/mnt:/tmp:$(LIBRARY_PATH)' >> $(RUN_SCRIPT)
ifneq ($(filter hw_emu, $(TARGET)),)
	@echo 'export XCL_EMULATION_MODE=$(TARGET)' >> $(RUN_SCRIPT)
endif
	@echo 'export XILINX_VITIS=/mnt' >> $(RUN_SCRIPT)
	@echo 'export XILINX_XRT=/usr' >> $(RUN_SCRIPT)
	@echo 'if [ -f platform_desc.txt  ]; then' >> $(RUN_SCRIPT)
	@echo '        cp platform_desc.txt /etc/xocl.txt' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo './$(EXE_NAME) $(PKG_HOST_ARGS)' >> $(RUN_SCRIPT)
	@echo 'return_code=$$?' >> $(RUN_SCRIPT)
	@echo 'if [ $$return_code -ne 0 ]; then' >> $(RUN_SCRIPT)
	@echo '        echo "ERROR: TEST FAILED, RC=$$return_code"' >> $(RUN_SCRIPT)
	@echo 'else' >> $(RUN_SCRIPT)
	@echo '        echo "INFO: TEST PASSED, RC=0"' >> $(RUN_SCRIPT)
	@echo 'fi' >> $(RUN_SCRIPT)
	@echo 'echo "INFO: Embedded host run completed."' >> $(RUN_SCRIPT)
	@echo 'exit $$return_code' >> $(RUN_SCRIPT)
DATA_FILE := $(custom_data_file)
DATA_DIR := $(CUR_DIR)/images $(custom_data_dir)
SD_FILES += $(RUN_SCRIPT)
SD_FILES += $(EXE_FILE)
SD_FILES += $(EMCONFIG)
SD_FILES += xrt.ini
SD_FILES += $(DATA_FILE)# where define DATAFILE in json
SD_FILES_WITH_PREFIX = $(foreach sd_file,$(SD_FILES), $(if $(filter $(sd_file),$(wildcard $(sd_file))), --package.sd_file $(sd_file)))
SD_DIRS_WITH_PREFIX = $(foreach sd_dir,$(DATA_DIR),--package.sd_dir $(sd_dir))
PACKAGE_FILES := $(BINARY_CONTAINERS)
PACKAGE_FILES += $(AIE_CONTAINER)
SD_CARD := $(CUR_DIR)/package_$(TARGET)
$(SD_CARD): host xclbin $(RUN_SCRIPT) $(EMCONFIG) check_kimage check_rootfs
	@echo "Generating sd_card folder...."
	mkdir -p $(SD_CARD)
	chmod a+rx $(BUILD_DIR)/run_script.sh
# 1. DFX HW Flow
ifeq ($(dfx_hw), on)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p $(PACKAGE_FILES) $(VPP_PACKAGE) -o $(BINARY_CONTAINERS_PKG)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -p --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX) --package.sd_file $(BINARY_CONTAINERS_PKG)
	@echo "### ***** sd_card generation done! ***** ###"
endif
# 2. General Embeded flow
ifeq ($(dfx_hw), off)
ifeq ($(pcie_versal), off)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif
endif

.PHONY: sd_card
sd_card: $(SD_CARD)
endif

############################## Setting Essential Checks and Building Rules ##############################
RUN_DEPS += host xclbin $(EMCONFIG)
ifeq ($(SD_CARD_NEEDED), on)
RUN_DEPS += $(SD_CARD)
endif

.PHONY: mkflag all run
mkflag:
	mkdir -p $(BUILD_DIR)
	rm -rf $(BUILD_DIR)/makefile_args.txt
	@for var in $(MAKEFLAGS); do echo $$var >> $(BUILD_DIR)/makefile_args.txt; done

all: check_device check_vpp check_platform mkflag $(RUN_DEPS)

run: all
#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
ifeq ($(pcie_versal), on)
	cp $(AIE_WORK_DIR)/reports/dma_lock_report.json ./
	cp $(AIE_WORK_DIR)/ps/c_rts/aie_control_config.json ./
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(BINARY_CONTAINERS_PKG) $(HOST_ARGS) 
	
else
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(BINARY_CONTAINERS_PKG) $(HOST_ARGS)
	
endif
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	
endif
endif
#hw
ifeq ($(TARGET), hw)
ifneq (,$(findstring aws-vu9p-f1, $(PLATFORM_NAME)))
ifeq (,$(wildcard $(BUILD_DIR)/kernel.awsxclbin))
	$(ECHO) "This makefile does not directly support converting .xclbin to .awsxclbin, please refer https://github.com/aws/aws-fpga/blob/master/Vitis/README.md for next operations"
else
	$(ECHO) "Running HW using generated .awsxclbin"
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(subst .xclbin,.awsxclbin,$(HOST_ARGS))
	
endif
else ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(HOST_ARGS)
	
else
	$(ECHO) "Please copy the content of sd_card folder and data to an SD Card and run on the board"
endif
endif

############################## Setting Targets ##############################

.PHONY: clean cleanall emconfig valid_params
emconfig: $(EMCONFIG)

.PHONY: host
ifeq ($(HOST_ARCH), x86)
host: check_xrt   $(EXE_FILE)
else
host: check_sysroot   $(EXE_FILE)
endif

.PHONY: xclbin
ifeq ($(HOST_ARCH), x86)
xclbin: check_vpp check_xrt  $(BINARY_CONTAINERS)
else
xclbin: check_vpp check_sysroot  $(BINARY_CONTAINERS)
endif

#################### Parameterized File Generation ##########################

gen_instances: $(GENERATED_FILES)

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE)  vitis_* TempConfig system_estimate.xtxt *.rpt .run/  $(INST_TB_FILES)
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png *.db

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.* _x* $(INST_FILES)

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR)  $(AIE_WORK_DIR) $(AIE_PKG_DIR) $(CUR_DIR)/*.xpe $(CUR_DIR)/hw.o $(CUR_DIR)/*.xsa $(CUR_DIR)/xnwOut
	-$(RMDIR)  *.html $(GENERATED_FILES)

clean: cleanh
