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

ifeq ($(API_IO),0)
	NUM_PORTS := $(UUT_SSR)
else
	NUM_PORTS := $$(( $(UUT_SSR) * 2 ))
endif


DYN_PT_HEADER_MODE = 0
ifeq ($(DYN_PT_SIZE), 1)
	DYN_PT_HEADER_MODE = 1
else
	DYN_PT_HEADER_MODE = 0
endif

HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): create_input sim_ref prep_x86_out
	make cleanall

create_input:
	@echo starting create_input
	@echo NUM_PORTS $(NUM_PORTS)
	@echo INPUT_FILE $(INPUT_FILE)
	@tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(WINDOW_VSIZE) $(NITER) $(SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(LOG_POINT_SIZE) $(DATA_TYPE) $(API_IO) 1 0 ;\
    perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --split --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) ;\
	echo Input ready

sim_ref:
	make UUT_KERNEL=fft_window_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

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
	@perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) ;\
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(REF_SIM_FILE) --type $(DATA_TYPE) --ssr $(NUM_PORTS) --zip --dual 0 -k $(DYN_PT_HEADER_MODE) -w $(WINDOW_VSIZE) ;\
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

get_status: check_op_ref
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) DATA_TYPE $(DATA_TYPE) COEFF_TYPE $(COEFF_TYPE) POINT_SIZE $(POINT_SIZE) WINDOW_VSIZE $(WINDOW_VSIZE) SHIFT $(SHIFT) API_IO $(API_IO) UUT_SSR $(UUT_SSR) DYN_PT_SIZE $(DYN_PT_SIZE)

get_qor:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/theoretical_minimum_scripts/get_wgt_theoretical_min.tcl $(DATA_TYPE) $(WINDOW_VSIZE) $(STATUS_FILE) $(UUT_KERNEL) $(API_IO) $(API_IO) $(NUM_PORTS) $(NUM_PORTS)
