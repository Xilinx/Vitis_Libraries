#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
HELPER_ROOT_DIR     ?= ./../../../../../
OUTPUT_DIR          ?= $(HELPER_CUR_DIR)
PARAMS_CFG          ?= ${VSS}_params.cfg
DSPLIB_ROOT_DIR     ?= $(HELPER_ROOT_DIR)
DSPLIB_IP_ROOT_DIR  ?= $(HELPER_ROOT_DIR)
AGREEMENT_PATH      ?= ${OUTPUT_DIR}/agreement.txt


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
XFFT_XO                 = $(OUTPUT_DIR)/xo/xfft_bd.xo
INT_PARAMS_CFG          = $(OUTPUT_DIR)/internal_params.cfg
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
xfft_xo: ${XFFT_XO}

hls_freq:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "freqhz" "top" ${PARAMS_CFG})
aie_name:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "enable-partition" "aie" ${PARAMS_CFG})
SSR:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "SSR" "APP_PARAMS" ${PARAMS_CFG})
VSS_MODE:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "VSS_MODE" "APP_PARAMS" ${PARAMS_CFG})
DATA_TYPE:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "DATA_TYPE" "APP_PARAMS" ${PARAMS_CFG})
ADD_FRONT_TRANSPOSE:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "ADD_FRONT_TRANSPOSE" "APP_PARAMS" ${PARAMS_CFG})
ADD_BACK_TRANSPOSE:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "ADD_BACK_TRANSPOSE" "APP_PARAMS" ${PARAMS_CFG})
PART:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "PART" "top" ${PARAMS_CFG})
AIE_VARIANT:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/meta/scripts/get_aievar_from_part.py ${PART})
POINT_SIZE:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "POINT_SIZE" "APP_PARAMS" ${PARAMS_CFG})
POINT_SIZE_D1:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "POINT_SIZE_D1" "APP_PARAMS" ${PARAMS_CFG})
USE_WIDGETS:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "USE_WIDGETS" "APP_PARAMS" ${PARAMS_CFG})
CASC_LEN:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "CASC_LEN" "APP_PARAMS" ${PARAMS_CFG})
API_IO:=$(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_param_cfg.py ${OUTPUT_DIR} "API_IO" "APP_PARAMS" ${PARAMS_CFG})

# set default values for parameters non-necessary parameters to maintain backwards compatibility
ifeq ($(filter -1,$(VSS_MODE)), -1)
VSS_MODE := 1
endif
ifeq ($(filter -1,$(ADD_FRONT_TRANSPOSE)), -1)
ADD_FRONT_TRANSPOSE := 1
endif
ifeq ($(filter -1,$(ADD_BACK_TRANSPOSE)), -1)
ADD_BACK_TRANSPOSE := 1
endif
ifeq ($(filter -1,$(CASC_LEN)), -1)
CASC_LEN := 1
endif
ifeq ($(filter -1,$(USE_WIDGETS)), -1)
USE_WIDGETS := 0
endif
ifeq ($(filter -1,$(API_IO)), -1)
API_IO := 0
endif

# infer values for decomposed fft based on inputs and defaults. use this calculation if user does not provide values for POINT_SIZE_D1
ifneq (,$(filter -1 1,$(POINT_SIZE_D1)))
ifeq ($(filter 2,$(VSS_MODE)), 2)
POINT_SIZE_D2 = $(SSR)
else
POINT_SIZE_D2 = $(POINT_SIZE)
endif
POINT_SIZE_D1 := $(shell expr $(POINT_SIZE) / $(POINT_SIZE_D2))
endif
POINT_SIZE_D2 := $(shell expr $(POINT_SIZE) / $(POINT_SIZE_D1))

# set top level aie file
ifeq ($(filter 1,$(VSS_MODE)), 1)
AIE_TL = aie.cpp
else
AIE_TL = aie_front_only.cpp
endif

ifeq ($(filter cint16,$(DATA_TYPE)), cint16)
    DATA_WIDTH = 32
    SAMPLES_PER_READ = 4
else
    DATA_WIDTH = 64
    SAMPLES_PER_READ = 2
endif

DUAL_STREAMS = 0
ifeq ($(filter 1,$(AIE_VARIANT)), 1)
ifeq ($(filter 1,$(API_IO)), 1)
DUAL_STREAMS = 1
endif
endif

HAS_BD_TRANSPOSE := $(shell $(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/check_aie_bd_use.py --vss_mode $(VSS_MODE) --aie_variant $(AIE_VARIANT) --ssr $(SSR) --data_width $(DATA_WIDTH) --point_size_d1 $(POINT_SIZE_D1) --point_size_d2 $(POINT_SIZE_D2))
ifeq ($(filter 1,$(HAS_BD_TRANSPOSE)), 1)
    ADD_FRONT_TRANSPOSE := 0
    ADD_BACK_TRANSPOSE := 0
else
    ADD_FRONT_TRANSPOSE := 1
    ADD_BACK_TRANSPOSE := 1
endif

.PHONY: meta_check

meta_check: $(INT_PARAMS_CFG)
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/create_config_json.py ${OUTPUT_DIR} ${INT_PARAMS_CFG}
	$(VITIS_PYTHON3) ${DSPLIB_ROOT_DIR}/L2/meta/scripts/metadata_checker.py --ip vss_fft_ifft_1d

############################################################################################
# VSS MODE 1
############################################################################################

ifeq ($(filter 1,$(VSS_MODE)), 1)

VSS_DEPS :=
# the order of dependencies is important in this makefile. first item of deps needs to be cfg file.
VSS_DEPS +=  ${VSSCFG} ${LIBADF} 
ifeq ($(filter 1,$(HAS_BD_TRANSPOSE)), 1)
VSS_DEPS += ${MID_XO_SINGLE_BUFF} ${SPLITTER_XO} ${JOINER_XO}
else
VSS_DEPS += ${MID_XO}
endif

ifeq ($(filter 1,$(ADD_BACK_TRANSPOSE)), 1)
VSS_DEPS += ${BACK_XO_GEN}
endif
ifeq ($(filter 1,$(ADD_FRONT_TRANSPOSE)), 1)
VSS_DEPS += ${FRONT_XO}
endif

${VSSCFG}: ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py  --ssr ${SSR} --cfg_file_name ${VSSCFG} --vss_unit ${VSS} --version 1 --freqhz ${hls_freq} --add_front_transpose ${ADD_FRONT_TRANSPOSE} --add_back_transpose ${ADD_BACK_TRANSPOSE} --data_type ${DATA_TYPE} --aie_obj_name ${aie_name} -aie_bd ${HAS_BD_TRANSPOSE} --vss_mode ${VSS_MODE} --dual_streams ${DUAL_STREAMS}

${VSSFILE}: $(VSS_DEPS)
	v++ --link --mode vss --part $(PART) --save-temps  --out_dir ${OUTPUT_DIR}/${VSS} --config $(VSS_DEPS)

endif

############################################################################################
# VSS MODE 2
############################################################################################
ifeq ($(filter 2,$(VSS_MODE)), 2)
ifeq ($(filter $(SSR),$(POINT_SIZE_D2)), $(SSR))
VSS_DEPS :=
# the order of dependencies is important in this makefile. first item needs to be the cfg file.
VSS_DEPS += ${VSSCFG} ${LIBADF} ${HLS_FFT_XO} ${SPLITTER_XO} ${JOINER_XO}
ifeq ($(filter 1,$(ADD_BACK_TRANSPOSE)), 1)
VSS_DEPS += ${BACK_XO_SPEC}
endif
${VSSCFG}: ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_pl_biased_con_gen.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_pl_biased_con_gen.py  --ssr $(SSR) --cfg_file_name ${VSSCFG} --vss_unit ${VSS} --version 1 --freqhz ${hls_freq} --add_back_transpose ${ADD_BACK_TRANSPOSE} --data_type ${DATA_TYPE} --aie_obj_name ${aie_name} --dual_streams ${DUAL_STREAMS}

${VSSFILE}: $(VSS_DEPS)
	v++ --link --mode vss --part $(PART) --save-temps  --out_dir ${OUTPUT_DIR}/${VSS} --config $^
else

license_check:
	sh ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/license_checker.sh ${AGREEMENT_PATH}

VSS_DEPS :=
# the order of dependencies is important in this makefile. first item needs to be the cfg file.
ifeq ($(filter 1,$(HAS_BD_TRANSPOSE)), 1)
VSS_DEPS += ${VSSCFG} ${LIBADF} ${XFFT_XO} ${MID_XO_SINGLE_BUFF} ${SPLITTER_XO} ${JOINER_XO}
else
VSS_DEPS += ${VSSCFG} ${LIBADF} ${XFFT_XO} ${MID_XO} ${BACK_XO_GEN} ${FRONT_XO}
endif
ifeq ($(filter cfloat,$(DATA_TYPE)), cfloat)
	XFFT_DTYPE=native_floating_point
else
	XFFT_DTYPE=fixed_point
endif
XFFT_DW = $(shell expr $(DATA_WIDTH) / 2)
XFFT_FREQ = $(shell awk 'BEGIN {x=$(hls_freq)/1000000; print int(x) + (x>int(x))}')

${VSSCFG}: ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py  --ssr ${SSR} --cfg_file_name ${VSSCFG} --vss_unit ${VSS} --version 1 --freqhz ${hls_freq} --add_front_transpose ${ADD_FRONT_TRANSPOSE} --add_back_transpose ${ADD_BACK_TRANSPOSE} --data_type ${DATA_TYPE} --aie_obj_name ${aie_name} -aie_bd ${HAS_BD_TRANSPOSE} --vss_mode ${VSS_MODE} --dual_streams ${DUAL_STREAMS}

# Define the target
${XFFT_XO}:
	vivado -mode batch -source ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/xo_scripts/pack_xfft_gen.tcl -tclargs $(PART) $(XFFT_DTYPE) $(SAMPLES_PER_READ) $(XFFT_FREQ) $(POINT_SIZE_D2) $(XFFT_DW)
	rm -rf ${OUTPUT_DIR}/project_1/

${VSSFILE}: license_check $(VSS_DEPS)
	v++ --link --mode vss --part $(PART) --save-temps  --out_dir ${OUTPUT_DIR}/${VSS} --config $(VSS_DEPS)
endif
endif

###########################################################################################
# Common Elements
###########################################################################################

################################ AIE GRAPH AND DEPENDENCIES ###############################
${INT_PARAMS_CFG}: ${PARAMS_CFG} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/update_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/update_cfg.py --ADD_FRONT_TRANSPOSE ${ADD_FRONT_TRANSPOSE} --ADD_BACK_TRANSPOSE ${ADD_BACK_TRANSPOSE} --POINT_SIZE_D1 ${POINT_SIZE_D1} ${PARAMS_CFG} ${INT_PARAMS_CFG}

${AIE_HDR}: $(INT_PARAMS_CFG) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/gen_tb_from_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/gen_tb_from_cfg.py ${DSPLIB_ROOT_DIR} $(OUTPUT_DIR) ${INT_PARAMS_CFG}

${AIE_PARAMS}: $(INT_PARAMS_CFG) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_aie_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_aie_cfg.py $(OUTPUT_DIR)  ${INT_PARAMS_CFG}

${LIBADF}: ${AIE_HDR} ${AIE_PARAMS}
	v++ -c \
	--aie.workdir ${OUTPUT_DIR}/Work \
	--config ${AIE_PARAMS} \
	${DSPLIB_OPTS} \
    -o ${LIBADF} \
    ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/${AIE_TL} ${DSPLIB_ROOT_DIR}/L1/src/aie/twiddle_rotator.cpp ${DSPLIB_ROOT_DIR}/L1/src/aie/fft_ifft_dit_1ch.cpp

######################### HLS KERNEL COMPILATIONS AND DEPENDENCIES ########################

${HPARAMS}: $(INT_PARAMS_CFG) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/scripts/extract_hls_cfg.py $(OUTPUT_DIR) ${HPARAMS} ${INT_PARAMS_CFG}

### FRONT TRANSPOSE
${FRONT_XO_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ifft_front_transpose  ${INT_PARAMS_CFG}

${FRONT_XO}: ${FRONT_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose_class.h
	v++ --compile --mode hls --config ${FRONT_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.cpp

### BACK TRANSPOSE IN VSS MODE 2

${BACK_XO_SPEC_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} back_transpose_simple  ${INT_PARAMS_CFG}

${BACK_XO_SPEC}: ${BACK_XO_SPEC_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple_class.h
	v++ --compile --mode hls --config ${BACK_XO_SPEC_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/back_transpose_simple/back_transpose_simple.cpp

### BACK TRANSPOSE IN VSS MODE 1

${BACK_XO_GEN_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ifft_back_transpose  ${INT_PARAMS_CFG}

${BACK_XO_GEN}: ${BACK_XO_GEN_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose_class.h
	v++ --compile --mode hls --config ${BACK_XO_GEN_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.cpp

### MIDDLE TRANSPOSE IN VSS MODE 1 (PING-PONG BUFFER)
### General case

${MID_XO_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ifft_transpose  ${INT_PARAMS_CFG}

${MID_XO}: ${MID_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose_class.h
	v++ --compile --mode hls --config ${MID_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.cpp

### MIDDLE TRANSPOSE IN VSS MODE 1 (SINGLE BUFFER)
### Is used only when aie buffer descriptors work

${MID_SINGLE_BUFF_XO_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} mid_transpose  ${INT_PARAMS_CFG}

${MID_XO_SINGLE_BUFF}: ${MID_SINGLE_BUFF_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose.cpp  ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose.h  ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose_class.h
	v++ --compile --mode hls --config ${MID_SINGLE_BUFF_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/mid_transpose/mid_transpose.cpp

### General AXIS helper functions 
${SPLITTER_XO_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} splitter ${INT_PARAMS_CFG} axis_split_join_config.tmpl

${SPLITTER_XO}: ${SPLITTER_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.h
	v++ --compile --mode hls --config ${SPLITTER_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp

${JOINER_XO_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} joiner ${INT_PARAMS_CFG} axis_split_join_config.tmpl

${JOINER_XO}: ${JOINER_XO_CFG} ${HPARAMS} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.h
	v++ --compile --mode hls --config ${JOINER_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${DSPLIB_ROOT_DIR}/L1/src/hw/common_fns/axis_split_join.cpp

### PL HLS FFT

${HLS_FFT_XO_CFG}: ${INT_PARAMS_CFG} ${CONFIG_TMPL} ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) ${DSPLIB_IP_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(OUTPUT_DIR) ${DSPLIB_ROOT_DIR} ssr_fft ${INT_PARAMS_CFG} ssr_fft_config.tmpl

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
