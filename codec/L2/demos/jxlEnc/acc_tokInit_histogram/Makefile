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
PLATFORM := xilinx_u50_gen3x16_xdma_5_202210_1
endif

# #################### Checking if PLATFORM in whitelist ############################
PLATFORM_ALLOWLIST +=  u50
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
HOST_SRCS += $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cjxl.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cjxl_main.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cmdline.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/codec_config.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/speed_stats.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cpu/cpu.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cpu/os_specific.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/box/box.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/time.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_png.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_pgx.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_pnm.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_jpg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_psd.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/threads/thread_parallel_runner_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/decode_to_jpeg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_huffman.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/quantizer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/ans_common.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/coeff_order.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_context_map.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/progressive_split.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_detect_dots.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/opsin_params.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/entropy_coder.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/blending.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_comparator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/huffman_table.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/huffman_tree.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/linalg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_file.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/aux_out.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/headers.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/alpha.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image_bundle.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image_metadata.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/frame_header.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/color_encoding_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/quant_weights.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_fast_heuristics.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jxl_encode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/fields.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/luminance.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_color_management.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_bit_writer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/loop_filter.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/color_management.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_quant_weights.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_photon_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_splines.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_patch_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/splines.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_xyb.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/gaborish.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ar_control_field.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/gauss_blur.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/memory_manager_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_external_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_file.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_image_bundle.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_external_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ans.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/passes_state.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/chroma_from_luma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_context_map.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_coeff_order.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_ans.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_entropy_coder.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/icc_codec_common.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/compressed_dc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/epf.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_dot_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_xyb.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_frame.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_patch_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_butteraugli_comparator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_reconstruct.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_group.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_group_border.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/filters.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_upsample.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/convolve.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_cache.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_upsample.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_huffman.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dct_scales.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/ac_strategy.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jxl_decode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/icc_codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_icc_codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/butteraugli/butteraugli.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/dec_jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_huffman_decode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/dec_jpeg_data_writer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_data_reader.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/padded_bytes.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/data_parallel.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/cache_aligned.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/status.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/dec_ma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/modular_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/encoding.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_rct.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_squeeze.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_palette.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/squeeze.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_transform.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/jxl_transform.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/enc_ma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/enc_encoding.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/encode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/memory.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/backward_references_hq.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/brotli_bit_stream.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/block_splitter.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/metablock.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/compress_fragment.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/compress_fragment_two_pass.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/backward_references.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/encoder_dict.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/utf8_util.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/decode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/static_dict.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/literal_cost.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/entropy_encode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/bit_cost.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/cluster.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/dictionary_hash.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/histogram.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/bit_reader.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/huffman.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/state.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/common/dictionary.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/common/transform.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmslut.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsnamed.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmspack.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscnvrt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsio1.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsgmt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsopt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsalpha.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmstypes.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsintrp.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsgamma.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscam02.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscgats.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmshalf.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsmtrx.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsps2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmssamp.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmssm.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsxform.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsio0.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsplugin.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmserr.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmspcs.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmswtpnt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsvirt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lodepng/lodepng.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway/hwy/aligned_allocator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway/hwy/targets.cc $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_cluster_histogram.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_ac_strategy.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_adaptive_quantization.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_cache.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_chroma_from_luma.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_cluster.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_frame.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_group.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_host.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_phase1.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_phase2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_phase3.cpp $(XFLIB_DIR)/ext/xcl2/xcl2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/host/host_tokinit_histogram.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/ -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/build/lib/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lodepng -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/kernel -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/host -I $(XFLIB_DIR)/L2/demos/jxlEnc/others/include/host_acc_tokInit_histogram -I $(XFLIB_DIR)/L2/demos/jxlEnc/others/include
CXXFLAGS += -O3 
else
HOST_SRCS += $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cjxl.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cjxl_main.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cmdline.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/codec_config.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/speed_stats.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cpu/cpu.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/cpu/os_specific.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/tools/box/box.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/time.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_png.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_pgx.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_pnm.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_jpg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/extras/codec_psd.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/threads/thread_parallel_runner_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/decode_to_jpeg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_huffman.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/quantizer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/ans_common.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/coeff_order.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_context_map.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/progressive_split.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_detect_dots.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/opsin_params.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/entropy_coder.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/blending.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_comparator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/huffman_table.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/huffman_tree.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/linalg.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_file.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/aux_out.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/headers.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/alpha.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image_bundle.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image_metadata.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/frame_header.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/color_encoding_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/quant_weights.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_fast_heuristics.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jxl_encode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/fields.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/luminance.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_color_management.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_bit_writer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/loop_filter.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/color_management.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_quant_weights.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_photon_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_splines.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_patch_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/splines.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_xyb.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/gaborish.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ar_control_field.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/gauss_blur.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/memory_manager_internal.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_external_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_file.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_image_bundle.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_external_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_toc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_ans.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_modular.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/passes_state.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/chroma_from_luma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_context_map.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_coeff_order.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_ans.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_entropy_coder.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/icc_codec_common.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/compressed_dc.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/epf.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_dot_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_xyb.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_frame.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_patch_dictionary.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_butteraugli_comparator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_reconstruct.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_group.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_group_border.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/filters.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_upsample.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/convolve.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_cache.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_noise.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_upsample.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dec_huffman.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/dct_scales.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/ac_strategy.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jxl_decode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/icc_codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/enc_icc_codec.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/butteraugli/butteraugli.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/dec_jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_huffman_decode.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/dec_jpeg_data_writer.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/jpeg_data.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/jpeg/enc_jpeg_data_reader.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/padded_bytes.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/data_parallel.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/cache_aligned.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/base/status.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/dec_ma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/modular_image.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/encoding.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_rct.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_squeeze.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_palette.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/squeeze.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/enc_transform.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/transform/jxl_transform.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/enc_ma.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/jxl/modular/encoding/enc_encoding.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/encode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/memory.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/backward_references_hq.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/brotli_bit_stream.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/block_splitter.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/metablock.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/compress_fragment.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/compress_fragment_two_pass.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/backward_references.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/encoder_dict.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/utf8_util.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/decode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/static_dict.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/literal_cost.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/entropy_encode.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/bit_cost.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/cluster.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/dictionary_hash.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/enc/histogram.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/bit_reader.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/huffman.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/dec/state.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/common/dictionary.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/common/transform.c $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmslut.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsnamed.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmspack.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscnvrt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsio1.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsgmt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsopt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsalpha.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmstypes.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsintrp.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsgamma.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscam02.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmscgats.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmshalf.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsmtrx.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsps2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmssamp.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmssm.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsxform.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsio0.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsplugin.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmserr.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmspcs.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmswtpnt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/src/cmsvirt.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lodepng/lodepng.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway/hwy/aligned_allocator.cc $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway/hwy/targets.cc $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_cluster_histogram.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_ac_strategy.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_adaptive_quantization.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_cache.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_chroma_from_luma.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_cluster.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_frame.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/acc_enc_group.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_host.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_phase1.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_phase2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/others/src/host_acc_tokInit_histogram/acc_phase3.cpp $(XFLIB_DIR)/ext/xcl2/xcl2.cpp $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/host/host_tokinit_histogram.cpp 
CXXFLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/ext/xcl2 -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/ -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/lib/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/build/lib/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lcms/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/highway -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/brotli/c/include -I $(XFLIB_DIR)/L2/demos/jxlEnc/third_partys/third_party/lodepng -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/kernel -I $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/host -I $(XFLIB_DIR)/L2/demos/jxlEnc/others/include/host_acc_tokInit_histogram -I $(XFLIB_DIR)/L2/demos/jxlEnc/others/include
CXXFLAGS += -O3 
endif

# workaround for opencv
ifeq (,$(findstring opencv,$(CXXFLAGS)))
CXXFLAGS += $(XRT_CXXFLAGS)
endif

EXE_NAME := host.exe
EXE_FILE := $(BUILD_DIR)/$(EXE_NAME)
EXE_FILE_DEPS := $(HOST_SRCS) $(INSTANCE_FILES)  $(EXE_FILE_DEPS)

HOST_ARGS :=  --xclbin $(BUILD_DIR)/jxlEnc.xclbin $(XFLIB_DIR)/L2/demos/jxlEnc/images/t0.png t0.jxl
ifneq ($(HOST_ARCH), x86)
PKG_HOST_ARGS = $(foreach args,$(HOST_ARGS),$(subst $(dir $(patsubst %/,%,$(args))),,$(args)))
endif

########################## Kernel compiler global settings ##########################
ifneq (,$(shell echo $(XPLATFORM) | awk '/u50/'))
VPP_FLAGS +=   --config $(CUR_DIR)/conn_u50.cfg
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/jxlEnc

else 
VPP_FLAGS +=  -I $(XFLIB_DIR)/../utils/L1/include/ -I $(XFLIB_DIR)/L2/include/hw/jxlEnc

endif

######################### binary container global settings ##########################
VPP_FLAGS_JxlEnc_ans_initHistogram +=  -D KERNEL_NAME=JxlEnc_ans_initHistogram
VPP_FLAGS_JxlEnc_ans_initHistogram += --hls.clock 300000000:JxlEnc_ans_initHistogram
ifneq ($(HOST_ARCH_temp), x86)
VPP_LDFLAGS_jxlEnc += --clock.defaultFreqHz 300000000
else
VPP_LDFLAGS_jxlEnc += --kernel_frequency 300
endif

ifneq ($(SD_CARD_NEEDED), on)
BINARY_CONTAINERS += $(BUILD_DIR)/jxlEnc.xclbin
else
BINARY_CONTAINERS += $(BUILD_DIR)/jxlEnc_pkg.$(LINK_TARGET_FMT)
BINARY_CONTAINERS_PKG += $(BUILD_DIR)/jxlEnc.xclbin
endif

# ################ Setting Rules for Binary Containers (Building Kernels) ################
$(TEMP_DIR)/JxlEnc_ans_initHistogram.xo: $(XFLIB_DIR)/L2/demos/jxlEnc/acc_tokInit_histogram/kernel/hls_init_histogram.cpp 
	$(ECHO) "Compiling Kernel: JxlEnc_ans_initHistogram"
	mkdir -p $(TEMP_DIR)
	$(VPP) -c $(VPP_FLAGS_JxlEnc_ans_initHistogram) $(VPP_FLAGS) -k JxlEnc_ans_initHistogram -I'$(<D)' --temp_dir $(TEMP_DIR) --report_dir $(TEMP_REPORT_DIR) -o $@ $^
BINARY_CONTAINER_jxlEnc_OBJS += $(TEMP_DIR)/JxlEnc_ans_initHistogram.xo
BINARY_CONTAINERS_DEPS += $(BINARY_CONTAINER_jxlEnc_OBJS)
$(BINARY_CONTAINERS): $(BINARY_CONTAINERS_DEPS)
	mkdir -p $(BUILD_DIR)
	$(VPP) -l $(VPP_FLAGS) --temp_dir $(TEMP_DIR) --report_dir $(BUILD_REPORT_DIR)/jxlEnc $(VPP_LDFLAGS)  $(VPP_LDFLAGS_jxlEnc) $(AIE_LDFLAGS)   -o $@ $^

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
	./check.sh
endif
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS)
	./check.sh
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	./check.sh
endif
endif
#sw_emu
ifneq (,$(filter sw_emu, $(TARGET)))
ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	XCL_EMULATION_MODE=$(TARGET) $(EXE_FILE) $(HOST_ARGS) 
	./check.sh
else
	@echo $(RUN_DEPS)
	$(SD_CARD)/launch_$(TARGET).sh -no-reboot -run-app $(notdir $(RUN_SCRIPT)) 
	grep "TEST PASSED, RC=0" $(SD_CARD)/qemu_output.log || exit 1
	./check.sh
endif
endif
#hw
ifeq ($(TARGET), hw)
ifneq (,$(findstring aws-vu9p-f1, $(PLATFORM_NAME)))
ifeq (,$(wildcard $(BUILD_DIR)/jxlEnc.awsxclbin))
	$(ECHO) "This makefile does not directly support converting .xclbin to .awsxclbin, please refer https://github.com/aws/aws-fpga/blob/master/Vitis/README.md for next operations"
else
	$(ECHO) "Running HW using generated .awsxclbin"
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(subst .xclbin,.awsxclbin,$(HOST_ARGS))
	./check.sh
endif
else ifeq ($(HOST_ARCH), x86)
	LD_LIBRARY_PATH=$(LIBRARY_PATH):$$LD_LIBRARY_PATH \
	$(EXE_FILE) $(HOST_ARGS)
	./check.sh
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