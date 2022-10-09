#
# Copyright 2021 Xilinx, Inc.
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

ifeq ($(DATA_TYPE), cint16)
WINDOW_VSIZE = $$(($(INPUT_WINDOW_VSIZE)-8*$(DYN_PT_SIZE)))
else
WINDOW_VSIZE = $$(($(INPUT_WINDOW_VSIZE)-4*$(DYN_PT_SIZE)))
endif

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

ifeq ($(API_IO),0)
	NUM_INPUTS := 1
else
	ifeq ($(PARALLEL_POWER), 0)
		NUM_INPUTS := 2
	else ifeq ($(PARALLEL_POWER), 1)
		NUM_INPUTS := 4
	else ifeq ($(PARALLEL_POWER), 2)
		NUM_INPUTS := 8
	else ifeq ($(PARALLEL_POWER), 3)
		NUM_INPUTS := 16
	else ifeq ($(PARALLEL_POWER), 4)
		NUM_INPUTS := 32
	else ifeq ($(PARALLEL_POWER), 5)
		NUM_INPUTS := 64
	else ifeq ($(PARALLEL_POWER), 6)
		NUM_INPUTS := 128
	endif
endif

ifeq ($(DYN_PT_SIZE), 1)
	DYN_PT_HEADER_MODE := 1
else
	DYN_PT_HEADER_MODE := 0
endif

NUM_OUTPUTS = $(NUM_INPUTS)

PARAM_MAP = DATA_TYPE $(DATA_TYPE) TWIDDLE_TYPE $(TWIDDLE_TYPE) POINT_SIZE $(POINT_SIZE) FFT_NIFFT $(FFT_NIFFT) SHIFT $(SHIFT) CASC_LEN $(CASC_LEN) DYN_PT_SIZE $(DYN_PT_SIZE) WINDOW_VSIZE $(WINDOW_VSIZE) PARALLEL_POWER $(PARALLEL_POWER) API_IO $(API_IO)

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): validate_config create_input sim_ref prep_x86_out
	make cleanall

validate_config:
	echo validating configuration;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);\
    vitis -exec ipmetadata_config_checker $(HELPER_ROOT_DIR)/L2/meta/fft_ifft_dit_1ch.json ./config.json -newflow

create_input:
	@echo starting create_input
	@echo NUM_INPUTS $(NUM_INPUTS)
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(PT_SIZE_PWR) $(DATA_TYPE) $(API_IO) 1  ${PARALLEL_POWER};\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(NUM_INPUTS) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${WINDOW_VSIZE};\
	echo Input ready

sim_ref:
	make UUT_KERNEL=fft_ifft_dit_1ch_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

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


check_op_ref: prep_x86_out prep_aie_out
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_OUTPUTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${WINDOW_VSIZE} ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_OUTPUTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w ${WINDOW_VSIZE} ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status: check_op_ref
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)
