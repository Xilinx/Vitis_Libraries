# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
# vitis makefile-generator v2.0.9

############################## Help Section ##############################
.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to generate the design for specified Target and Shell."
	$(ECHO) ""
	$(ECHO) "  make run TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to run application in emulation."
	$(ECHO) ""
	$(ECHO) "  make xclbin TARGET=<hw/hw_emu/sw_emu/> PLATFORM=<FPGA platform>"
	$(ECHO) "      Command to build xclbin application."
	$(ECHO) ""
	$(ECHO) "  make host TARGET=<hw/hw_emu/sw_emu/>"
	$(ECHO) "      Command to build host application."
	$(ECHO) ""
	$(ECHO) "  NOTE: For embedded devices, e.g. zcu102/zcu104/vck190, HOST_ARCH is either aarch32 or aarch64."
	$(ECHO) "      a.IF Download the platform, and common-image from Xilinx Download Center(Suggested):"
	$(ECHO) "        Run the sdk.sh script from the common-image directory to install sysroot using the command : ./sdk.sh -y -d ./ -p "
	$(ECHO) "        Unzip the rootfs file : gunzip ./rootfs.ext4.gz"
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "      b.User could also define SYSROOT, K_IMAGE and ROOTFS by themselves: "
	$(ECHO) "        export SYSROOT=< path-to-platform-sysroot >"
	$(ECHO) "        export K_IMAGE=< path-to-Image-files >"
	$(ECHO) "        export ROOTFS=< path-to-rootfs >"
	$(ECHO) ""
	$(ECHO) "  make clean "
	$(ECHO) "      Command to remove the generated non-hardware files."
	$(ECHO) ""
	$(ECHO) "  make cleanall TARGET=<hw/hw_emu/sw_emu/>"
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
PLATFORM := xilinx_u250_gen3x16_xdma_4_1_202210_1
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  u250
PLATFORM_BLOCKLIST +=  zc

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
CXXFLAGS += -fmessage-length=0 -I$(CUR_DIR)/src/ -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(XILINX_XRT)/lib -L$(XILINX_HLS)/lnx64/tools/fpo_v7_1 -Wl,--as-needed -lOpenCL -lxrt_coreutil -lgmp -lmpfr -lIp_floating_point_v7_1_bitacc_cmodel 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
else ifeq ($(HOST_ARCH), aarch64)
CXXFLAGS += -I$(CUR_DIR)/src/ -fmessage-length=0 --sysroot=$(SYSROOT)  -I$(SYSROOT)/usr/include/xrt -I$(XILINX_HLS)/include -std=c++14 -O3 -Wall -Wno-unknown-pragmas -Wno-unused-label 
LDFLAGS += -pthread -L$(SYSROOT)/usr/lib -Wl,--as-needed -lxilinxopencl -lxrt_coreutil 
VPP_FLAGS += -t $(TARGET) --platform $(XPLATFORM) --save-temps 
VPP_LDFLAGS += --optimize 2 -R 2 
endif
CXXFLAGS += $(EXTRA_CXXFLAGS)
VPP_FLAGS += $(EXTRA_VPP_FLAGS)

########################## Setting up Host Variables ##########################

#Inclue Required Host Source Files
ifeq ($(ps_on_x86), on)
HOST_SRCS += $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cpik_main.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ac_predictions.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ac_strategy.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/adaptive_quantization.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/adaptive_reconstruction.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/alpha.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ans_common.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ans_decode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ans_encode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ar_control_field.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/arch_specific.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/bilinear_transform.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/block_dictionary.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/butteraugli_comparator.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/butteraugli_distance.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cache_aligned.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/chroma_from_luma.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cmdline.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/codec_impl.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/codec_png.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/codec_pnm.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/color_correlation.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/color_encoding.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/compressed_dc.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/compressed_image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/context_map_decode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/context_map_encode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cpik.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/data_parallel.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/dc_predictor.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/dct.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/dct_util.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/deconvolve.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/descriptive_statistics.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/detect_dots.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/entropy_coder.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/epf.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/epf_target.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/external_image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/gaborish.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/gauss_blur.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/gradient_map.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/headers.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/huffman_decode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/huffman_encode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lehmer_code.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/linalg.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lossless16.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lossless8.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lossless_entropy.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/metadata.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/noise.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/opsin_image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/opsin_inverse.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/opsin_params.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/os_specific.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/padded_bytes.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/pik.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/pik_frame.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/pik_info.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/quant_weights.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/quantizer.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/saliency_map.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/single_image_handler.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/status.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/upscaler.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/yuv_convert.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/yuv_opsin_convert.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/butteraugli/butteraugli.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/simd/targets.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/brotli.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/color_management.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/encode_order.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik_codec_common.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/host_dev.cc $(XFLIB_DIR)/ext/xcl2/xcl2.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsalpha.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmscam02.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmscgats.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmscnvrt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmserr.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsgamma.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsgmt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmshalf.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsintrp.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsio0.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsio1.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmslut.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsmtrx.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsnamed.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsopt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmspack.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmspcs.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsplugin.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsps2.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmssamp.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmssm.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmstypes.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsvirt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmswtpnt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsxform.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lodepng/lodepng.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lodepng/lodepng_util.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/entropy_common.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/fseU16.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/fse_compress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/fse_decompress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/huf_compress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/huf_decompress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/backward_references.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/backward_references_hq.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/bit_cost.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/block_splitter.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/brotli_bit_stream.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/cluster.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/compress_fragment.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/compress_fragment_two_pass.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/dictionary_hash.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/encode.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/encoder_dict.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/entropy_encode.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/histogram.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/literal_cost.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/memory.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/metablock.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/static_dict.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/utf8_util.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/common/dictionary.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/common/transform.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/bit_reader.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/decode.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/huffman.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/state.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/fuzz/decode_fuzzer.c 
CXXFLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/pikEnc/ -I $(XFLIB_DIR)/L2/include/hw/pikEnc/kernel3/ -I $(XFLIB_DIR)/L2/demos/pikEnc/host -I $(XFLIB_DIR)/L2/demos/pikEnc/kernel -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/include -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/include -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lodepng -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party
CXXFLAGS += -O3 
else
HOST_SRCS += $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cpik_main.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ac_predictions.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ac_strategy.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/adaptive_quantization.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/adaptive_reconstruction.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/alpha.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ans_common.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ans_decode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ans_encode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/ar_control_field.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/arch_specific.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/bilinear_transform.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/block_dictionary.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/butteraugli_comparator.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/butteraugli_distance.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cache_aligned.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/chroma_from_luma.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cmdline.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/codec_impl.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/codec_png.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/codec_pnm.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/color_correlation.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/color_encoding.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/compressed_dc.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/compressed_image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/context_map_decode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/context_map_encode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/cpik.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/data_parallel.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/dc_predictor.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/dct.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/dct_util.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/deconvolve.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/descriptive_statistics.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/detect_dots.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/entropy_coder.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/epf.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/epf_target.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/external_image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/gaborish.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/gauss_blur.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/gradient_map.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/headers.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/huffman_decode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/huffman_encode.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lehmer_code.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/linalg.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lossless16.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lossless8.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/lossless_entropy.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/metadata.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/noise.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/opsin_image.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/opsin_inverse.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/opsin_params.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/os_specific.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/padded_bytes.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/pik.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/pik_frame.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/pik_info.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/quant_weights.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/quantizer.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/saliency_map.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/single_image_handler.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/status.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/upscaler.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/yuv_convert.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/yuv_opsin_convert.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/butteraugli/butteraugli.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/simd/targets.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/brotli.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik/color_management.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/encode_order.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/pik_codec_common.cc $(XFLIB_DIR)/L2/demos/pikEnc/host/host_dev.cc $(XFLIB_DIR)/ext/xcl2/xcl2.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsalpha.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmscam02.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmscgats.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmscnvrt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmserr.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsgamma.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsgmt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmshalf.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsintrp.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsio0.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsio1.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmslut.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsmtrx.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsnamed.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsopt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmspack.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmspcs.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsplugin.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsps2.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmssamp.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmssm.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmstypes.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsvirt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmswtpnt.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/src/cmsxform.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lodepng/lodepng.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lodepng/lodepng_util.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/entropy_common.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/fseU16.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/fse_compress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/fse_decompress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/huf_compress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib/huf_decompress.cpp $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/backward_references.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/backward_references_hq.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/bit_cost.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/block_splitter.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/brotli_bit_stream.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/cluster.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/compress_fragment.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/compress_fragment_two_pass.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/dictionary_hash.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/encode.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/encoder_dict.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/entropy_encode.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/histogram.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/literal_cost.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/memory.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/metablock.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/static_dict.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/enc/utf8_util.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/common/dictionary.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/common/transform.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/bit_reader.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/decode.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/huffman.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/dec/state.c $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/fuzz/decode_fuzzer.c 
CXXFLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/pikEnc/ -I $(XFLIB_DIR)/L2/include/hw/pikEnc/kernel3/ -I $(XFLIB_DIR)/L2/demos/pikEnc/host -I $(XFLIB_DIR)/L2/demos/pikEnc/kernel -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lcms/include -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/brotli/c/include -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/lodepng -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party/FiniteStateEntropy/lib -I $(XFLIB_DIR)/L2/demos/pikEnc/host/third_party
CXXFLAGS += -O3 
endif

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(INSTANCE_FILES)  $(EXE_FILE_DEPS)

HOST_ARGS :=  --xclbin $(BUILD_DIR)/pikEnc.xclbin images/small32x32.png test.pik --fast
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u250/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u200.cfg --config $(CUR_DIR)/opts.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw -I $(XFLIB_DIR)/L2/include/hw/pikEnc -I $(XFLIB_DIR)/L2/demos/pikEnc/kernel

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw -I $(XFLIB_DIR)/L2/include/hw/pikEnc -I $(XFLIB_DIR)/L2/demos/pikEnc/kernel

endif

######################### binary container global settings ##########################
VPP_FLAGS_pikEncKernel1Top +=  -D KERNEL_NAME=pikEncKernel1Top
VPP_FLAGS_pikEncKernel1Top += --hls.clock 300000000:pikEncKernel1Top
VPP_FLAGS_pikEncKernel2Top +=  -D KERNEL_NAME=pikEncKernel2Top --hls.pre_tcl $(CUR_DIR)/hls_pre.tcl
VPP_FLAGS_pikEncKernel2Top += --hls.clock 300000000:pikEncKernel2Top
VPP_FLAGS_pikEncKernel3Top +=  -D KERNEL_NAME=pikEncKernel3Top --hls.pre_tcl $(CUR_DIR)/hls_pre.tcl
VPP_FLAGS_pikEncKernel3Top += --hls.clock 300000000:pikEncKernel3Top
ifneq ($(HOST_ARCH_temp), x86)
VPP_LDFLAGS_pikEnc += --clock.defaultFreqHz 180000000
else
VPP_LDFLAGS_pikEnc += --kernel_frequency 180
endif

ifneq ($(SD_CARD_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/pikEnc.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/pikEnc_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/pikEnc.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/pikEncKernel1Top.xo: $(XFLIB_DIR)/L2/demos/pikEnc/kernel/XAccPIKKernel1.cpp 
	$(ECHO) "Compiling Kernel: pikEncKernel1Top"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_pikEncKernel1Top) $(VPP_FLAGS) -k pikEncKernel1Top -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_pikEnc_OBJS += $(TEMP_DIR)/pikEncKernel1Top.xo
$(TEMP_DIR)/pikEncKernel2Top.xo: $(XFLIB_DIR)/L2/demos/pikEnc/kernel/XAccPIKKernel2.cpp 
	$(ECHO) "Compiling Kernel: pikEncKernel2Top"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_pikEncKernel2Top) $(VPP_FLAGS) -k pikEncKernel2Top -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_pikEnc_OBJS += $(TEMP_DIR)/pikEncKernel2Top.xo
$(TEMP_DIR)/pikEncKernel3Top.xo: $(XFLIB_DIR)/L2/demos/pikEnc/kernel/XAccPIKKernel3.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/build_cluster.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/ctrl_tokenize.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/dc_tokenize.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/dc_shrink.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/build_table_encode_histo.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/ac_tokenize.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/ans.cpp $(XFLIB_DIR)/L2/demos/pikEnc/kernel/kernel3/kernel3_common.cpp 
	$(ECHO) "Compiling Kernel: pikEncKernel3Top"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_pikEncKernel3Top) $(VPP_FLAGS) -k pikEncKernel3Top -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_pikEnc_OBJS += $(TEMP_DIR)/pikEncKernel3Top.xo
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_pikEnc_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/pikEnc $(VPP_LDFLAGS)  $(VPP_LDFLAGS_pikEnc) $(AIE_LDFLAGS)   -o $@ $^

############################## Setting Rules for Host (Building Host Executable) ##############################
$(EXE_FILE): $(EXE_FILE_DEPS)
	mkdir -p $(BUILD_DIR)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS)

$(EMCONFIG):
	emconfigutil --platform $(XPLATFORM) --od $(BUILD_DIR)

############################## Preparing sdcard folder ##############################
ifeq ($(SD_CARD_NEEDED), on)
RUN_SCRIPT := $(BUILD_DIR)/run_script.sh
$(RUN_SCRIPT):
	rm -rf $(RUN_SCRIPT)
	@echo 'export LD_LIBRARY_PATH=/mnt:/tmp:$(LIBRARY_PATH)' >> $(RUN_SCRIPT)
ifneq ($(filter sw_emu hw_emu, $(TARGET)),)
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
DATA_DIR := $(custom_data_dir)
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
# 2. PS_ON_X86 - From 2022.2, Target:sw_emu
ifeq ($(ps_on_x86), on)
	@echo "### ***** running PS X86 SE_EMU ***** ###"
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir $(SD_CARD) --package.emu_ps x86
endif
# 3. AIE_ON_X86 Flow
ifeq ($(pcie_aie), on)
	@echo "### ***** running AIE ON_X86 ***** ###"
	${VPP} -p $(VPP_PACKAGE) -t ${TARGET} -f ${XPLATFORM} ${AIE_CONTAINER} ${BINARY_CONTAINERS} -o $(BINARY_CONTAINERS_PKG) --package.boot_mode ospi
	@echo "### ***** sd_card generation done! ***** ###"
endif
# 4. General Embeded flow
ifeq ($(dfx_hw), off)
ifeq ($(ps_on_x86), off)
ifeq ($(pcie_aie), off)
	$(VPP) -t $(TARGET) --platform $(XPLATFORM) -o $(BINARY_CONTAINERS_PKG) -p $(PACKAGE_FILES) $(VPP_PACKAGE) --package.out_dir  $(SD_CARD) --package.rootfs $(ROOTFS) --package.kernel_image $(K_IMAGE)  $(SD_FILES_WITH_PREFIX) $(SD_DIRS_WITH_PREFIX)
	@echo "### ***** sd_card generation done! ***** ###"
endif
endif
endif

.PHONY: sd_card
sd_card: $(SD_CARD)
endif
############################## Setting Essential Checks and Building Rules ##############################
RUN_DEPS += host xclbin $(EMCONFIG)
RUN_DEPS += $(SD_CARD)

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
ifeq ($(pcie_aie), on)
	cp $(AIE_WORK_DIR)/reports/dma_lock_report.json ./
	cp $(AIE_WORK_DIR)/ps/c_rts/aie_control_config.json ./
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS) $(BINARY_CONTAINERS_PKG)
	
endif
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
	
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	
endif
endif
#sw_emu
ifneq (,$(filter sw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS) 
	
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	
endif
endif
#hw
ifeq ($(TARGET), hw)
ifneq (,$(findstring aws-vu9p-f1, $(PLATFORM_NAME)))
ifeq (,$(wildcard $(BUILD_DIR)/pikEnc.awsxclbin))
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

.PHONY: clean cleanall emconfig gen_instances valid_params
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

gen_instances: $(INSTANCE_FILES)
	@echo "Nothing to generate."

valid_params:
	@echo "Nothing to validate."

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
	-$(RMDIR)  *.html $(INSTANCE_FILES)

clean: cleanh