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
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
SEED ?= 1
ifeq ($(POINT_SIZE), 16)
	PT_SIZE_PWR       := 4
else ifeq ($(POINT_SIZE), 32)
	PT_SIZE_PWR       := 5
else ifeq ($(POINT_SIZE), 64)
	PT_SIZE_PWR       := 6
else ifeq ($(POINT_SIZE), 128)
	PT_SIZE_PWR       := 7
else ifeq ($(POINT_SIZE), 256)
	PT_SIZE_PWR       := 8
else ifeq ($(POINT_SIZE), 512)
	PT_SIZE_PWR       := 9
else ifeq ($(POINT_SIZE), 1024)
	PT_SIZE_PWR       := 10
else ifeq ($(POINT_SIZE), 2048)
	PT_SIZE_PWR       := 11
else ifeq ($(POINT_SIZE), 4096)
	PT_SIZE_PWR       := 12
endif
LOG_WINDOW_VSIZE := $(PT_SIZE_PWR)

POINT_SIZE := $$(( $POINT_SIZE/$UUT_SSR  ))

ifeq ($(POINT_SIZE), 16)
    LOG_POINT_SIZE := 4
else ifeq ($(POINT_SIZE), 32)
    LOG_POINT_SIZE := 5
else ifeq ($(POINT_SIZE), 64)
    LOG_POINT_SIZE := 6
else ifeq ($(POINT_SIZE), 128)
    LOG_POINT_SIZE := 7
else ifeq ($(POINT_SIZE), 256)
    LOG_POINT_SIZE := 8
else ifeq ($(POINT_SIZE), 512)
    LOG_POINT_SIZE := 9
else ifeq ($(POINT_SIZE), 1024)
    LOG_POINT_SIZE := 10
else ifeq ($(POINT_SIZE), 2048)
    LOG_POINT_SIZE := 11
else ifeq ($(POINT_SIZE), 4096)
    LOG_POINT_SIZE := 12
else ifeq ($(POINT_SIZE), 8192)
    LOG_POINT_SIZE := 13
else ifeq ($(POINT_SIZE), 16384)
    LOG_POINT_SIZE := 14
else ifeq ($(POINT_SIZE), 32768)
    LOG_POINT_SIZE := 15
else ifeq ($(POINT_SIZE), 65536)
    LOG_POINT_SIZE := 16
endif


ifeq ($(AIE_VARIANT),1)
	ifeq ($(API_IO),0)
		NUM_PORTS := $(UUT_SSR)
	else
		NUM_PORTS := $$(( $(UUT_SSR) * 2 ))
	endif
else
	NUM_PORTS := $(UUT_SSR)
endif


HEADER_SIZE ?= 0
ifeq ($(DYN_PT_SIZE), 1)
	ifeq ($(AIE_VARIANT), 1)
		HEADER_SIZE := 32
	else ifeq ($(AIE_VARIANT), 2)
		HEADER_SIZE := 32
	else ifeq ($(AIE_VARIANT), 22)
		HEADER_SIZE := 64
	else
		HEADER_SIZE := 0
	endif
endif

PLIO_WIDTH  = 64

PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) COEFF_TYPE $(COEFF_TYPE) POINT_SIZE $(POINT_SIZE) WINDOW_VSIZE $(WINDOW_VSIZE) SHIFT $(SHIFT) DYN_PT_SIZE $(DYN_PT_SIZE)  UUT_SSR $(UUT_SSR) API_IO $(API_IO) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_input:
	@echo helper.mk stage:	create_input
	@echo NUM_PORTS $(NUM_PORTS)
	@echo INPUT_FILE $(INPUT_FILE)
	echo tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_POINT_SIZE) $(DATA_TYPE) $(API_IO) 1 0 0 0 0 0 ${PLIO_WIDTH} ${HEADER_SIZE};\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_POINT_SIZE) $(DATA_TYPE) $(API_IO) 1 0 0 0 0 0 ${PLIO_WIDTH} ${HEADER_SIZE};\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --split --dual 0 -k $(HEADER_SIZE) -w $(WINDOW_VSIZE) --plioWidth ${PLIO_WIDTH} ;\
	echo Input ready

sim_ref:
	@echo helper.mk stage:	sim_ref
	make UUT_KERNEL=fft_window_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

prep_x86_out:
	@echo helper.mk stage:	prep_x86_out
	@x86_out_files=`ls $(HELPER_CUR_DIR)/x86simulator_output/data`;\
	echo "X86 files= " $$x86_out_files;\
	for n in $$x86_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/x86simulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

prep_aie_out:
	@echo helper.mk stage:	prep_aie_out
	@aie_out_files=`ls $(HELPER_CUR_DIR)/aiesimulator_output/data`;\
	echo "AIE files= " $$aie_out_files;\
	for n in $$aie_out_files; do \
		grep -ve '[XT]' $(HELPER_CUR_DIR)/aiesimulator_output/data/$$n > $(HELPER_CUR_DIR)/data/$$n;\
	done

get_diff:
	@echo helper.mk stage:	get_diff
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --zip --dual 0 -k $(HEADER_SIZE) -w $(WINDOW_VSIZE) --plioWidth ${PLIO_WIDTH} ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --zip --dual 0 -k $(HEADER_SIZE) -w $(WINDOW_VSIZE) --plioWidth ${PLIO_WIDTH} ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status:
	@echo helper.mk stage:	get_status
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	@echo helper.mk stage:	get_stats
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output "fft_window_main" $(NITER)

harvest_mem:
	@echo helper.mk stage:	harvest_mem
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
