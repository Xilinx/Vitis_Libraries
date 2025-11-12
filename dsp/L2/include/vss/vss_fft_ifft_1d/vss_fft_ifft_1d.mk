#
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
#

### User Params
VSS                 ?= vss_fft_ifft_1d
AIETARGET           ?= hw
HELPER_CUR_DIR      ?= ./
OUTPUT_DIR          ?= $(HELPER_CUR_DIR)
PARAMS_CFG          ?= ${VSS}_params.cfg
DSPLIB_ROOT_DIR     ?= $(HELPER_ROOT_DIR)
DSPLIB_IP_ROOT_DIR  ?= $(HELPER_ROOT_DIR)


VSSCFG             = ${VSS}_connectivity.cfg
HPARAMS            = $(OUTPUT_DIR)/hls_params.cfg
CFGFILE            = $(OUTPUT_DIR)/hls_config.cfg
AIE_PARAMS         = $(OUTPUT_DIR)/aie_params.cfg
AIE_HDR            = $(OUTPUT_DIR)/config.h
LIBADF             = $(OUTPUT_DIR)/libadf.a
VSSFILE            = ${VSS}/${VSS}.vss
AIE_TL             = aie.cpp


### HLS kernels and corresponding cfg files
FRONT_XO_CFG            = $(OUTPUT_DIR)/ifft_front_transpose_config.cfg
FRONT_XO                = $(OUTPUT_DIR)/ifft_front_transpose_wrapper/ifft_front_transpose_wrapper.xo
BACK_XO_GEN_CFG         = $(OUTPUT_DIR)/ifft_back_transpose_config.cfg
BACK_XO_GEN             = $(OUTPUT_DIR)/ifft_back_transpose_wrapper/ifft_back_transpose_wrapper.xo
BACK_XO_SPEC_CFG        = $(OUTPUT_DIR)/back_transpose_simple_config.cfg
BACK_XO_SPEC            = $(OUTPUT_DIR)/back_transpose_simple_wrapper/back_transpose_simple_wrapper.xo
MID_XO_CFG              = $(OUTPUT_DIR)/ifft_transpose_config.cfg
MID_XO                  = $(OUTPUT_DIR)/ifft_transpose_wrapper/ifft_transpose_wrapper.xo
MID_SINGLE_BUFF_XO_CFG  = $(OUTPUT_DIR)/mid_transpose_config.cfg
MID_XO_SINGLE_BUFF      = $(OUTPUT_DIR)/mid_transpose_wrapper/mid_transpose_wrapper.xo
JOINER_XO_CFG           = $(OUTPUT_DIR)/joiner_config.cfg
JOINER_XO               = $(OUTPUT_DIR)/joiner_wrapper/joiner_wrapper.xo
SPLITTER_XO_CFG         = $(OUTPUT_DIR)/splitter_config.cfg
SPLITTER_XO             = $(OUTPUT_DIR)/splitter_wrapper/splitter_wrapper.xo
HLS_FFT_XO_CFG          = $(OUTPUT_DIR)/ssr_fft_config.cfg
HLS_FFT_XO              = $(OUTPUT_DIR)/ssr_fft_wrapper/ssr_fft_wrapper.xo

CONFIG_TMPL             = ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/hls_config.tmpl

DSPLIB_OPTS 	  := -I ${XILINX_VITIS}/aietools/include  \
                	 -I ${DSPLIB_ROOT_DIR}/L1/include/aie \
                	 -I ${DSPLIB_ROOT_DIR}/L1/src/aie \
                	 -I ${DSPLIB_ROOT_DIR}/L1/include/vss/common \
                	 -I ${DSPLIB_ROOT_DIR}/L1/include/vss/vss_fft_ifft_1d \
                	 -I ${DSPLIB_ROOT_DIR}/L1/tests/aie/inc \
                	 -I ${DSPLIB_ROOT_DIR}/L1/tests/aie/src \
                	 -I ${DSPLIB_ROOT_DIR}/L2/include/aie \
                	 -I ${DSPLIB_ROOT_DIR}/L2/tests/aie/common/inc \
                	 -I ${DSPLIB_ROOT_DIR}/L2/tests/vss/vss_fft_ifft_1d/ \
					 -I $(OUTPUT_DIR)/ \
                     --mode aie --target=$(AIETARGET) \
                     --aie.verbose  \
                     --aie.Xchess=llvm.xargs=\"-std=c++2a\" \
                     --aie.Xchess=main:backend.mist2.xargs=\"+NOdra\" \
                     --aie.Xchess=main:backend.mist2.pnll=\"off\" \
                     --aie.Xpreproc=-DOUTPUT_FILE=./data/uut_output.txt \
                     --aie.Xpreproc=-DFRONT_INPUT_FILE=./data/input_front.txt \
                     --aie.Xpreproc=-DBACK_INPUT_FILE=./data/input_back.txt \
                     --aie.Xpreproc=-DFRONT_OUTPUT_FILE=./data/output_front.txt \
                     --aie.Xpreproc=-DBACK_OUTPUT_FILE=./data/output_back.txt 

VITIS_PYTHON3 = LD_LIBRARY_PATH=$(XILINX_VITIS)/tps/lnx64/python-3.13.0/lib $(XILINX_VITIS)/tps/lnx64/python-3.13.0/bin/python3

libadf: ${LIBADF}
tb_config: ${AIE_HDR}
aie_params: ${AIE_PARAMS}
front_xo: ${FRONT_XO}
back_xo_gen: ${BACK_XO_GEN}
back_xo_spec: ${BACK_XO_SPEC}
mid_xo: ${MID_XO}
vss: ${VSSFILE}
vsscfg: ${VSSCFG}
hls_fft_xo: ${HLS_FFT_XO}

hls_freq := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "freqhz" "top" ${PARAMS_CFG})
aie_name := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "enable-partition" "aie" ${PARAMS_CFG})
SSR := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "SSR" "APP_PARAMS" ${PARAMS_CFG})
VSS_MODE := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "VSS_MODE" "APP_PARAMS" ${PARAMS_CFG})
DATA_TYPE := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "DATA_TYPE" "APP_PARAMS" ${PARAMS_CFG})
ADD_FRONT_TRANSPOSE := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "ADD_FRONT_TRANSPOSE" "APP_PARAMS" ${PARAMS_CFG})
ADD_BACK_TRANSPOSE := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "ADD_BACK_TRANSPOSE" "APP_PARAMS" ${PARAMS_CFG})
PART  := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "PART" "top" ${PARAMS_CFG})
AIE_VARIANT  := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "AIE_VARIANT" "top" ${PARAMS_CFG})

# set default values for parameters non-necessary parameters
ifeq ($(VSS_MODE), -1)
VSS_MODE := 1
endif
ifeq ($(ADD_FRONT_TRANSPOSE), -1)
ADD_FRONT_TRANSPOSE := 1
endif
ifeq ($(ADD_BACK_TRANSPOSE), -1)
ADD_BACK_TRANSPOSE := 1
endif

# set top level aie file
ifeq ($(VSS_MODE), 1)
AIE_TL = aie.cpp
else
AIE_TL = aie_front_only.cpp
endif



.PHONY: meta_check set_transpose_area


############################################################################################
# VSS MODE 1
############################################################################################
ifeq ($(VSS_MODE), 1)

HAS_BD_TRANSPOSE = 0
# Temporary ugly code to see if the part has an AIE1 array.
# in future release (2026.1), this will be updated for so the customer passes the aie-variant to avoid maintanence on vss 
# this limitation was discovered too close to 2025.2 release to add a new parameter.
AIE1_VARIANT_NAMES := vc vp vr
ifeq ($(AIE_VARIANT), 1)
	ifeq ($(DATA_TYPE),cint16)
		HAS_BD_TRANSPOSE = 1
		ADD_BACK_TRANSPOSE = 0
		ADD_FRONT_TRANSPOSE = 0
	else
		HAS_BD_TRANSPOSE = 0
		ADD_BACK_TRANSPOSE = 1
		ADD_FRONT_TRANSPOSE = 1
	endif
else
	HAS_BD_TRANSPOSE = 1
	ADD_BACK_TRANSPOSE = 0
	ADD_FRONT_TRANSPOSE = 0
endif


VSS_DEPS :=
# the order of dependencies is important in this makefile. first item of deps needs to be cfg file.
VSS_DEPS +=  ${VSSCFG} ${LIBADF} 
ifeq ($(HAS_BD_TRANSPOSE), 1)
VSS_DEPS += ${MID_XO_SINGLE_BUFF} ${SPLITTER_XO} ${JOINER_XO}
else
VSS_DEPS += ${MID_XO}
endif

ifeq ($(ADD_BACK_TRANSPOSE), 1)
VSS_DEPS += ${BACK_XO_GEN}
endif
ifeq ($(ADD_FRONT_TRANSPOSE), 1)
VSS_DEPS += ${FRONT_XO}
endif

meta_check:
# create config.json file from cfg
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/create_config_json.py ${OUTPUT_DIR} ${PARAMS_CFG}
# call metadata checks
	$(VITIS_PYTHON3) ${DSPLIB_ROOT_DIR}/L2/meta/scripts/metadata_checker.py --ip vss_fft_ifft_1d

${VSSCFG}: ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py  --ssr ${SSR} --cfg_file_name ${VSSCFG} --vss_unit ${VSS} --version 1 --freqhz ${hls_freq} --add_front_transpose ${ADD_FRONT_TRANSPOSE} --add_back_transpose ${ADD_BACK_TRANSPOSE} --data_type ${DATA_TYPE} --aie_obj_name ${aie_name} -aie_bd ${HAS_BD_TRANSPOSE}

${VSSFILE}: set_transpose_area $(VSS_DEPS)
	v++ --link --mode vss --part $(PART) --save-temps  --out_dir ${OUTPUT_DIR}/${VSS} --config $(VSS_DEPS)

endif

############################################################################################
# VSS MODE 2
############################################################################################
ifeq ($(VSS_MODE), 2)
VSS_DEPS :=
# the order of dependencies is important in this makefile. first item needs to be the cfg file.
VSS_DEPS += ${VSSCFG} ${LIBADF} ${HLS_FFT_XO} ${SPLITTER_XO} ${JOINER_XO}
ifeq ($(ADD_BACK_TRANSPOSE), 1)
VSS_DEPS += ${BACK_XO_SPEC}
endif
${VSSCFG}: ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_pl_biased_con_gen.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_pl_biased_con_gen.py  --ssr $(SSR) --cfg_file_name ${VSSCFG} --vss_unit ${VSS} --version 1 --freqhz ${hls_freq} --add_back_transpose ${ADD_BACK_TRANSPOSE} --data_type ${DATA_TYPE} --aie_obj_name ${aie_name}

${VSSFILE}: $(VSS_DEPS)
	v++ --link --mode vss --part $(PART) --save-temps  --out_dir ${OUTPUT_DIR}/${VSS} --config $^
endif

###########################################################################################
# Common Elements
###########################################################################################

################################ AIE GRAPH AND DEPENDENCIES ###############################
${AIE_HDR}: $(PARAMS_CFG) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/gen_tb_from_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/gen_tb_from_cfg.py ${DSPLIB_ROOT_DIR} $(OUTPUT_DIR) ${PARAMS_CFG}

${AIE_PARAMS}: $(PARAMS_CFG) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_aie_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_aie_cfg.py $(OUTPUT_DIR)  ${PARAMS_CFG}

${LIBADF}: ${AIE_HDR} ${AIE_PARAMS}
	v++ -c \
	--aie.workdir ${OUTPUT_DIR}/Work \
	--config ${AIE_PARAMS} \
	${DSPLIB_OPTS} \
    -o ${LIBADF} \
    ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/${AIE_TL} ${DSPLIB_ROOT_DIR}/L1/src/aie/twiddle_rotator.cpp ${DSPLIB_ROOT_DIR}/L1/src/aie/fft_ifft_dit_1ch.cpp

######################### HLS KERNEL COMPILATIONS AND DEPENDENCIES ########################

${HPARAMS}: $(PARAMS_CFG) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_hls_cfg.py $(OUTPUT_DIR) ${HPARAMS} ${PARAMS_CFG}

### FRONT TRANSPOSE
${FRONT_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ifft_front_transpose  ${PARAMS_CFG}

${FRONT_XO}: ${FRONT_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose_class.h
	v++ --compile --mode hls --config ${FRONT_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.cpp

### BACK TRANSPOSE IN VSS MODE 2

${BACK_XO_SPEC_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} back_transpose_simple  ${PARAMS_CFG}

${BACK_XO_SPEC}: ${BACK_XO_SPEC_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple_class.h
	v++ --compile --mode hls --config ${BACK_XO_SPEC_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple.cpp

### BACK TRANSPOSE IN VSS MODE 1

${BACK_XO_GEN_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ifft_back_transpose  ${PARAMS_CFG}

${BACK_XO_GEN}: ${BACK_XO_GEN_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose_class.h
	v++ --compile --mode hls --config ${BACK_XO_GEN_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.cpp

### MIDDLE TRANSPOSE IN VSS MODE 1 (PING-PONG BUFFER)
### General case

${MID_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ifft_transpose  ${PARAMS_CFG}

${MID_XO}: ${MID_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose_class.h
	v++ --compile --mode hls --config ${MID_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.cpp

### MIDDLE TRANSPOSE IN VSS MODE 1 (SINGLE BUFFER)
### Is used only when aie buffer descriptors work

${MID_SINGLE_BUFF_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} mid_transpose  ${PARAMS_CFG}

${MID_XO_SINGLE_BUFF}: ${MID_SINGLE_BUFF_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose_class.h
	v++ --compile --mode hls --config ${MID_SINGLE_BUFF_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose.cpp

### General AXIS helper functions 
${SPLITTER_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} splitter ${PARAMS_CFG} axis_split_join_config.tmpl

${SPLITTER_XO}: ${SPLITTER_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.h
	v++ --compile --mode hls --config ${SPLITTER_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp

${JOINER_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} joiner ${PARAMS_CFG} axis_split_join_config.tmpl

${JOINER_XO}: ${JOINER_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.h
	v++ --compile --mode hls --config ${JOINER_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp

### PL HLS FFT

${HLS_FFT_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ssr_fft ${PARAMS_CFG} ssr_fft_config.tmpl

${HLS_FFT_XO}: ${HLS_FFT_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ssr_fft/ssr_fft.cpp ${DSPLIB_ROOT_DIR}/L1/src/hw/ssr_fft/ssr_fft.h
	v++ --compile --mode hls --config ${HLS_FFT_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ssr_fft/ssr_fft.cpp

### help

help:
	echo "Makefile Usage:"
	echo "  make -f vss_fft_ifft_1d.mk vss OUTPUT_DIR=<path/to/build/dir> DSPLIB_ROOT_DIR=<path/to/dsp/library/root>"
	echo "      Command to generate the full vss according to the parameters defined in vss_fft_ifft_1d_params.cfg. Please edit the vss_fft_ifft_1d_params.cfg"
	echo ""
	echo "  make clean"
	echo "      Command to remove the generated files."
	echo ""

clean:
	rm -rf .crashReporter* .Xil _x ${VSS} *.log *summary ${HPARAMS} ${VSSCFG} ${CFGFILE} ${LIBADF} ${FRONT_XO} ${BACK_XO_GEN} ${MID_XO} ${AIE_PARAMS} ${CUR_DIR}/Work ${AIE_HDR} ${FRONT_XO_CFG} ${BACK_XO_GEN_CFG} ${MID_XO_CFG} back_transpose_simple_wrapper ifft_front_transpose_wrapper ifft_transpose_wrapper sol.db map_report.csv
