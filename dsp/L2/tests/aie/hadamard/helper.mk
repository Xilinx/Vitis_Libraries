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

###############################################################################
# Makefile helper used for hadamard compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
SEED ?= 1

ifeq ($(T_DATA_A), int16)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := int16
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 16
			else
			VEC_IN_FRAME := 8
			endif
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := int32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint16
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	endif

else ifeq ($(T_DATA_A), int32)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := int32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := int32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	endif

else ifeq ($(T_DATA_A), cint16)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := cint16
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint16
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	endif

else ifeq ($(T_DATA_A), cint32)
	ifeq ($(T_DATA_B), int16)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 8
			endif
	else ifeq ($(T_DATA_B), int32)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	else ifeq ($(T_DATA_B), cint16)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	else ifeq ($(T_DATA_B), cint32)
		T_DATA_OUT := cint32
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	endif

else ifeq ($(T_DATA_A), float)
	ifeq ($(T_DATA_B), float)
		T_DATA_OUT := float
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 8
			else
			VEC_IN_FRAME := 4
			endif
	else ifeq ($(T_DATA_B), cfloat)
		T_DATA_OUT := cfloat
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
	endif
else ifeq ($(T_DATA_A), cfloat)
	T_DATA_OUT := cfloat
		ifeq ($(API_IO), 0)
			VEC_IN_FRAME := 4
			else
			VEC_IN_FRAME := 2
			endif
endif
DIM_SIZE_PADDED := $(shell echo $$(( $(DIM_SIZE) / $(UUT_SSR))) )
DIM_SIZE_PADDED := $(shell echo $$(( $$(($$(( $(DIM_SIZE_PADDED) + $(VEC_IN_FRAME) - 1)) / $(VEC_IN_FRAME))) * $(VEC_IN_FRAME))) )
WINDOW_VSIZE := $(shell echo $$(( $(DIM_SIZE_PADDED) * $(NUM_FRAMES) * $(UUT_SSR))))


ifeq ($(DIM_SIZE_PADDED), 16)
	PT_SIZE_PWR       := 4
else ifeq ($(DIM_SIZE_PADDED), 32)
	PT_SIZE_PWR       := 5
else ifeq ($(DIM_SIZE_PADDED), 64)
	PT_SIZE_PWR       := 6
else ifeq ($(DIM_SIZE_PADDED), 128)
	PT_SIZE_PWR       := 7
else ifeq ($(DIM_SIZE_PADDED), 256)
	PT_SIZE_PWR       := 8
else ifeq ($(DIM_SIZE_PADDED), 512)
	PT_SIZE_PWR       := 9
else ifeq ($(DIM_SIZE_PADDED), 1024)
	PT_SIZE_PWR       := 10
else ifeq ($(DIM_SIZE_PADDED), 2048)
	PT_SIZE_PWR       := 11
else ifeq ($(DIM_SIZE_PADDED), 4096)
	PT_SIZE_PWR       := 12
endif
LOG_WINDOW_VSIZE := $(PT_SIZE_PWR)


LOG_DIM_SIZE := 1
ifeq ($(DIM_SIZE_PADDED), 16)
    LOG_DIM_SIZE := 4
else ifeq ($(DIM_SIZE_PADDED), 32)
    LOG_DIM_SIZE := 5
else ifeq ($(DIM_SIZE_PADDED), 64)
    LOG_DIM_SIZE := 6
else ifeq ($(DIM_SIZE_PADDED), 128)
    LOG_DIM_SIZE := 7
else ifeq ($(DIM_SIZE_PADDED), 256)
    LOG_DIM_SIZE := 8
else ifeq ($(DIM_SIZE_PADDED), 512)
    LOG_DIM_SIZE := 9
else ifeq ($(DIM_SIZE_PADDED), 1024)
    LOG_DIM_SIZE := 10
else ifeq ($(DIM_SIZE_PADDED), 2048)
    LOG_DIM_SIZE := 11
else ifeq ($(DIM_SIZE_PADDED), 4096)
    LOG_DIM_SIZE := 12
else ifeq ($(DIM_SIZE_PADDED), 8192)
    LOG_DIM_SIZE := 13
else ifeq ($(DIM_SIZE_PADDED), 16384)
    LOG_DIM_SIZE := 14
else ifeq ($(DIM_SIZE_PADDED), 32768)
    LOG_DIM_SIZE := 15
else ifeq ($(DIM_SIZE_PADDED), 65536)
    LOG_DIM_SIZE := 16
endif

ifeq ($(AIE_VARIANT), 1)
	ifeq ($(API_IO),0)
		PORTS_PER_TILE := 1
	else
		PORTS_PER_TILE := 1
	endif
else
	PORTS_PER_TILE := 1
endif

ifeq ($(API_IO), 1)
	NUM_PORTS := $(shell echo $$(( $(UUT_SSR)*$(PORTS_PER_TILE) )) )
else
	NUM_PORTS := $(UUT_SSR)
endif

DYN_PT_HEADER_MODE := 0
DYN_PT_SIZE := 0
ifeq ($(DYN_PT_SIZE), 1)
	DYN_PT_HEADER_MODE = 1
else
	DYN_PT_HEADER_MODE = 0
endif

ifeq ($(T_DATA_B), float)
	T_DATA_OUT := float
else ifeq ($(T_DATA_B), cfloat)
	T_DATA_OUT := cfloat
endif

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) T_DATA_A $(T_DATA_A) T_DATA_B $(T_DATA_B) DIM_SIZE $(DIM_SIZE) NUM_FRAMES $(NUM_FRAMES) SHIFT $(SHIFT) UUT_SSR $(UUT_SSR) API_IO $(API_IO) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_config:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	@echo starting create_input
	@echo NUM_PORTS        $(NUM_PORTS)
	@echo LOC_INPUT_FILE_A $(LOC_INPUT_FILE_A)
	@echo LOC_INPUT_FILE_B $(LOC_INPUT_FILE_B)
	@echo DIM_SIZE         $(DIM_SIZE)
	@echo DIM_SIZE_PADDED  $(DIM_SIZE_PADDED)
	@echo NUM_FRAMES       $(NUM_FRAMES)
	@echo WINDOW_VSIZE     $(WINDOW_VSIZE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_DIM_SIZE) $(T_DATA_A) $(API_IO) 1 0 0 0 0 0 64 ;\
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_DIM_SIZE) $(T_DATA_B) $(API_IO) 1 0 0 0 0 0 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_A) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_DIM_SIZE) $(T_DATA_A) $(API_IO) 1 0 0 0 0 0 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(LOC_INPUT_FILE_B) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_DIM_SIZE) $(T_DATA_B) $(API_IO) 1 0 0 0 0 0 64 ;\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(LOC_INPUT_FILE_A) --type $(T_DATA_A) --ssr $(NUM_PORTS) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) --plioWidth 64 ;\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(LOC_INPUT_FILE_B) --type $(T_DATA_B) --ssr $(NUM_PORTS) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) --plioWidth 64 ;\
	echo Input ready

sim_ref:
	make UUT_KERNEL=hadamard_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

get_diff:
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(T_DATA_OUT) --ssr $(NUM_PORTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) --plioWidth 64 ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(T_DATA_OUT) --ssr $(NUM_PORTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) --plioWidth 64 ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)  SINGLE_BUF $(SINGLE_BUF)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_A_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output "hadamard_main" $(NITER)
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
