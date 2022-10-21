# Copyright 2019-2021 Xilinx, Inc.
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
# sc makefile-generator v1.0.0

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform> HOST_ARCH=<x86>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make host HOST_ARCH=<hw/hw_emu/sw_emu/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) "      By default, HOST_ARCH=x86. HOST_ARCH is required for SoC shells"
	$(ECHO) ""
	$(ECHO) "  make clean "
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

# setting devault value
TARGET ?= sw_emu
HOST_ARCH ?= x86

#setting PLATFORM
ifeq ($(PLATFORM),)
PLATFORM := $(DEVICE)
endif
ifeq ($(PLATFORM),)
PLATFORM := xilinx_u50_gen3x16_xdma_5_202210_1
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  u50
PLATFORM_BLOCKLIST +=  zc

GCC_INTOOL := 8.3.0
BINUTILS_INTOOL := 2.37
include ./utils.mk
TEMP_DIR := _x_temp.$(TARGET).$(PLATFORM_NAME)
TEMP_REPORT_DIR := $(CUR_DIR)/reports/_x.$(TARGET).$(PLATFORM_NAME)
BUILD_DIR := build_dir.$(TARGET).$(PLATFORM_NAME)
BUILD_REPORT_DIR := $(CUR_DIR)/reports/_build.$(TARGET).$(PLATFORM_NAME)
EMCONFIG := $(BUILD_DIR)/emconfig.json
XCLBIN_DIR := $(CUR_DIR)/$(BUILD_DIR)
export XCL_BINDIR = $(XCLBIN_DIR)

EXE_FILE_DEPS :=
BINARY_CONTAINERS_DEPS :=
RUN_DEPS :=

# set debug switch
ifneq ($(debug),yes)
CXXFLAGS += -O3
endif

# workaround for compiling on CentOS7 (Must match "RedHat*" and "Red Hat*")
ifneq ($(filter Cent% Red%,$(shell lsb_release -is)),)
  ifeq ($(shell lsb_release -rs|cut -d. -f1),7)
    $(info CentOS/RHEL 7)
    CXXFLAGS  += -D_GLIBCXX_USE_CXX11_ABI=0
    VPP_FLAGS  += -D_GLIBCXX_USE_CXX11_ABI=0
    VPP_LDFLAGS += -D_GLIBCXX_USE_CXX11_ABI=0
    SYS_COMP_LIB := $(XILINX_VITIS)/system_compiler/lib/centos7
  else
    SYS_COMP_LIB := $(XILINX_VITIS)/system_compiler/lib/x86
  endif
endif

# get global setting
ifdef XILINX_SC_PFM_CONFIG
CXXFLAGS += -DXILINX_SC_PFM_CONFIG=$(XILINX_SC_PFM_CONFIG)
endif
ifdef XILINX_SC_PFM_EXT
CXXFLAGS += -DXILINX_SC_PFM_EXT=$(XILINX_SC_PFM_EXT)
endif
ifeq ($(HOST_ARCH), x86)
CXXFLAGS += -I $(XILINX_VITIS)/system_compiler/include -I $(XILINX_HLS)/include 
LDFLAGS += -L$(XILINX_XRT)/lib -L$(SYS_COMP_LIB) -lvpp_acc -l$(LIB_XRT) -lxrt_coreutil  -Wl,-rpath=$(SYS_COMP_LIB):$(XILINX_XRT)/lib:$(XILINX_XRT)/lib:$(GCC_HOME)/lib64  -Wl,--enable-new-dtags -lpthread 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --temp_dir $(TEMP_DIR) --save-temps -g -I $(XILINX_VITIS)/system_compiler/include 
VPP_LDFLAGS += 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += 
LDFLAGS += 
VPP_FLAGS += 
VPP_LDFLAGS += 
endif
CXXFLAGS += $(EXTRA_CXXFLAGS)
VPP_FLAGS += $(EXTRA_VPP_FLAGS)

ifeq ($(TARGET),sw)
  $(error Error: The sw target is not supported anymore. Please use sw_emu instead)
else ifeq ($(TARGET),sw_emu)
  LIB_XRT  := xrt_swemu
  HOST_PREAMBLE := XCL_EMULATION_MODE=sw_emu
else ifeq ($(TARGET),hw_emu)
  LIB_XRT  := xrt_hwemu
  HOST_PREAMBLE := XCL_EMULATION_MODE=hw_emu
  ifneq (,$(findstring -g,$(EXTRA_VPPFLAGS) $(CXXFLAGS)))
    # for sourcing pre/post xsim scripts
    ifneq ($(XILINX_SC_HW_EMU),0)
      HOST_PREAMBLE += XILINX_SC_HW_EMU=1 XILINX_SC_BUILD_DIR=$(PWD)/$(BUILD_DIR)
    endif
  endif
else ifeq ($(TARGET),hw)
  LIB_XRT  := xrt_core
endif

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
HOST_SRCS += $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cjxl.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cjxl_main.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cmdline.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/codec_config.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/speed_stats.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cpu/cpu.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cpu/os_specific.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/box/box.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/time.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_png.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_pgx.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_pnm.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_jpg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_psd.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/threads/thread_parallel_runner_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/decode_to_jpeg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_huffman.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/quantizer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/ans_common.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/coeff_order.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_context_map.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/progressive_split.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_detect_dots.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/opsin_params.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/entropy_coder.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/blending.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_comparator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/huffman_table.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/huffman_tree.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/linalg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_file.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/aux_out.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/headers.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/alpha.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image_bundle.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image_metadata.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/frame_header.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/color_encoding_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/quant_weights.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_fast_heuristics.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jxl_encode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/fields.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/luminance.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_color_management.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_bit_writer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/loop_filter.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/color_management.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_quant_weights.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_chroma_from_luma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_adaptive_quantization.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_cache.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_group.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ac_strategy.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_photon_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_splines.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_patch_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/splines.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_xyb.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/gaborish.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ar_control_field.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/gauss_blur.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/memory_manager_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_external_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_file.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_image_bundle.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_external_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ans.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/passes_state.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/chroma_from_luma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_context_map.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_coeff_order.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_ans.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_entropy_coder.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/icc_codec_common.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/compressed_dc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/epf.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_dot_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_xyb.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_frame.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_patch_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_butteraugli_comparator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_reconstruct.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_group.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_group_border.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/filters.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_upsample.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/convolve.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_cache.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_upsample.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_huffman.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dct_scales.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/ac_strategy.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jxl_decode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/icc_codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_icc_codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/butteraugli/butteraugli.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_cluster.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/dec_jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_huffman_decode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/dec_jpeg_data_writer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_data_reader.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/padded_bytes.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/data_parallel.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/cache_aligned.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/status.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/dec_ma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/modular_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/encoding.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_rct.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_squeeze.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_palette.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/squeeze.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_transform.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/jxl_transform.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/enc_ma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/enc_encoding.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/encode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/memory.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/backward_references_hq.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/brotli_bit_stream.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/block_splitter.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/metablock.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/compress_fragment.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/compress_fragment_two_pass.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/backward_references.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/encoder_dict.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/utf8_util.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/decode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/static_dict.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/literal_cost.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/entropy_encode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/bit_cost.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/cluster.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/dictionary_hash.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/histogram.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/bit_reader.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/huffman.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/state.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/common/dictionary.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/common/transform.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmslut.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsnamed.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmspack.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscnvrt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsio1.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsgmt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsopt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsalpha.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmstypes.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsintrp.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsgamma.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscam02.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscgats.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmshalf.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsmtrx.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsps2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmssamp.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmssm.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsxform.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsio0.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsplugin.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmserr.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmspcs.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmswtpnt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsvirt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lodepng/lodepng.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway/hwy/aligned_allocator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway/hwy/targets.cc $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_frame.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_lossy_enc_compute/acc_host.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_lossy_enc_compute/acc_phase1.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_lossy_enc_compute/acc_phase2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_lossy_enc_compute/acc_phase3.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_frame.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/acc_lossy_enc_compute_sc/host/host_lossy_enc_compute.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/ -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/build/lib/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lodepng -I $(XFLIB_DIR)/L2/demos/jxlEnc/others/include/host_acc_lossy_enc_compute -I $(XFLIB_DIR)/L2/demos/jxlEnc/others/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_lossy_enc_compute_sc/kernel -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_lossy_enc_compute_sc/host
CXXFLAGS += -O3 

EXE_NAME := host.exe
EXE_OBJS := $(addprefix $(TEMP_DIR)/, $(addsuffix .o,$(basename $(HOST_SRCS))))
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(EXE_OBJS) 
MAKEDEPEND = $(CXX) $< -MM -MP -MF $(basename $@).d -MT $@  $(CXXFLAGS)

HOST_ARGS :=  --xclbin $(BUILD_DIR)/jxlEnc.xclbin $(XFLIB_DIR)/L2/demos/jxlEnc/images/small32x32.png small32x32.jxl
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_lossy_enc_compute_sc/kernel

######################### binary container global settings ##########################
VPP_FLAGS_hls_lossy_enc_compute +=  -D KERNEL_NAME=hls_lossy_enc_compute
VPP_FLAGS_hls_lossy_enc_compute += --hls.clock 300000000:hls_lossy_enc_compute
ifneq ($(HOST_ARCH), x86)
VPP_LDFLAGS_hls_lossy_enc_compute += --clock.defaultFreqHz 300000000
else
VPP_LDFLAGS_hls_lossy_enc_compute += --kernel_frequency 300
endif
VPP_LDFLAGS_hls_lossy_enc_compute_temp := --advanced.param compiler.userPostSysLinkOverlayTcl=postSysLink.tcl
VPP_LDFLAGS_hls_lossy_enc_compute += $(VPP_LDFLAGS_hls_lossy_enc_compute_temp)

ifeq ($(HOST_ARCH), x86)
BINARY_CONTAINERS_TMP := $(BUILD_DIR)/$(TARGET).o
BINARY_CONTAINERS := $(BUILD_DIR)/$(TARGET).xclbin
ifeq ($(TARGET),sw_emu)
  BINARY_CONTAINERS_TMP :=
endif
else
# placeholder for non_x86
endif

.SECONDEXPANSION:
# ################ Setting Rules for Binary Containers (Building Kernels) ################
ACC_SRCS_hls_lossy_enc_compute += $(XFLIB_DIR)/L2/demos/jxlEnc/acc_lossy_enc_compute_sc/kernel/hls_lossy_enc_compute.cpp
ACC_OBJS_hls_lossy_enc_compute  := $(addprefix $(TEMP_DIR)/, $(addsuffix .o,$(basename $(ACC_SRCS_hls_lossy_enc_compute))))
$(ACC_OBJS_hls_lossy_enc_compute): $(TEMP_DIR)/%.o : %.cpp $$(@D)/.f
	@echo "--> Making $@ from: $?"
	$(MAKEDEPEND)
	$(VPP) $(VPP_FLAGS) $(VPP_FLAGS_hls_lossy_enc_compute) -o $@  -c $<
BINARY_CONTAINERS_DEPS  += $(ACC_OBJS_hls_lossy_enc_compute) 
$(BINARY_CONTAINERS_TMP) : $(BINARY_CONTAINERS_DEPS)
	@echo "--> Making $@ from: $?"
	$(VPP) $(VPP_FLAGS) $(VPP_LDFLAGS) $(VPP_LDFLAGS_hls_lossy_enc_compute) -o $(BINARY_CONTAINERS) -l $^
EXE_FILE_DEPS += $(BINARY_CONTAINERS_TMP)
EXE_FILE_DEPS += $(BINARY_CONTAINERS_DEPS)

############################## Setting Rules for Host (Building Host Executable) ##############################
ifeq ($(HOST_ARCH), x86)
$(TEMP_DIR)/%.o : %.cpp $$(@D)/.f
	@echo "--> Making $@ from: $?"
	mkdir -p $(BUILD_DIR)
	$(MAKEDEPEND)
	$(CXX) -o $@ $(CXXFLAGS)  -I . -c $<
$(TEMP_DIR)/%.o : %.cc $$(@D)/.f
	@echo "--> Making $@ from: $?"
	mkdir -p $(BUILD_DIR)
	$(MAKEDEPEND)
	$(CXX) -o $@ $(CXXFLAGS)  -I . -c $<
$(TEMP_DIR)/%.o : %.c $$(@D)/.f
	@echo "--> Making $@ from: $?"
	mkdir -p $(BUILD_DIR)
	$(MAKEDEPEND)
	$(CXX) -o $@ $(CXXFLAGS)  -I . -c $<
$(EXE_FILE): $(EXE_FILE_DEPS)  
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)
else
# place holder for arch64
endif

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

%/.f:
	mkdir -p $(dir $@)
	touch $@

.PRECIOUS: %/.f

RUN_DEPS += $(EXE_FILE) $(EMCONFIG)

run: check_device  $(RUN_DEPS)
#sw_emu
ifneq (,$(filter sw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(HOST_PREAMBLE) $(EXE_FILE) $(HOST_ARGS)
	./check.sh
else
# place holder for arch64
endif
endif

#hw_emu
ifneq (,$(filter hw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(HOST_PREAMBLE) $(EXE_FILE) $(HOST_ARGS)
	./check.sh
else
# place holder for arch64
endif
endif

#hw
ifeq ($(TARGET), hw)
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(HOST_PREAMBLE) $(EXE_FILE) $(HOST_ARGS)
	./check.sh
else
# place holder for arch64
endif
endif

############################## Setting Targets ##############################

.PHONY: all clean cleanall emconfig
emconfig: $(EMCONFIG)
ifeq ($(HOST_ARCH), x86)
all:  check_vpp check_platform check_xrt $(EXE_FILE) $(BINARY_CONTAINERS) emconfig
else
all:  check_vpp check_platform check_sysroot $(EXE_FILE) $(BINARY_CONTAINERS) emconfig sd_card
endif

.PHONY: host xclbin
ifeq ($(HOST_ARCH), x86)
host:  check_xrt $(EXE_FILE)
else
host:  check_sysroot $(EXE_FILE)
endif
xclbin: $(BINARY_CONTAINERS_TMP)

############################## Cleaning Rules ##############################
cleanh:
	-$(RMDIR) $(EXE_FILE) vitis_* TempConfig system_estimate.xtxt *.rpt .run/
	-$(RMDIR) src/*.ll _xocc_* .Xil dltmp* xmltmp* *.log *.jou *.wcfg *.wdb sample_link.ini sample_compile.ini obj*  bin* *.csv *.jpg *.jpeg *.png

cleank:
	-$(RMDIR) $(BUILD_DIR)/*.xclbin _vimage *xclbin.run_summary qemu-memory-_* emulation/ _vimage/ pl*start_simulation. sh *.xclbin
	-$(RMDIR) _x_temp.*/_x.* _x_temp.*/.Xil _x_temp.*/profile_summary.* xo_* _x*
	-$(RMDIR) _x_temp.*/dltmp* _x_temp.*/kernel_info.dat _x_temp.*/*.log
	-$(RMDIR) _x_temp.* 

cleanall: cleanh cleank
	-$(RMDIR) $(BUILD_DIR)  build_dir.* emconfig.json *.html $(TEMP_DIR) $(CUR_DIR)/reports *.csv *.run_summary  $(CUR_DIR)/*.raw package_*   $(BUILD_DIR)/run_script.sh .ipcache *.str
	-$(RMDIR) $(XFLIB_DIR)/common/data/*.xe2xd* $(XFLIB_DIR)/common/data/*.orig*

clean: cleanh
