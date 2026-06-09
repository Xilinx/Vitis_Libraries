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

###############################################################################
# Makefile helper used for FFT compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../

HELPER:= $(HELPER_CUR_DIR)/.HELPER

PYTHON3 ?= python3
TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

ifeq ($(AIE_VARIANT), 2)
	SAMPLES_IN_VECT		  := 8
else ifeq ($(DATA_TYPE), cint32)
	SAMPLES_IN_VECT 	  := 4
else ifeq ($(DATA_TYPE), cfloat)
	SAMPLES_IN_VECT 	  := 4
else ifeq ($(DATA_TYPE), cint16)
	SAMPLES_IN_VECT 	  := 8
else
	SAMPLES_IN_VECT 	  := 8
endif
SSR_SAMPLES 		:= $(shell echo $$(( 	$(SAMPLES_IN_VECT) * $(UUT_SSR)		)))
IN_FRAME_SIZE 		:= $(shell echo $$((   (($(POINT_SIZE) + $(SAMPLES_IN_VECT) - 1) / $(SAMPLES_IN_VECT)) * $(SAMPLES_IN_VECT) )))
OUT_FRAME_SIZE 		:= $(shell echo $$((   (($(POINT_SIZE) + $(SSR_SAMPLES) - 1) / $(SSR_SAMPLES)) * $(SSR_SAMPLES) )))

IN_WINDOW_VSIZE 	:= $(shell echo $$(( 	$(IN_FRAME_SIZE) * $(NUM_FRAMES)		)))
OUT_WINDOW_VSIZE 	:= $(shell echo $$(( 	$(OUT_FRAME_SIZE) * $(NUM_FRAMES)		)))
NO_PAD_WINDOW_VSIZE := $(shell echo $$(( 	$(POINT_SIZE) 	 * $(NUM_FRAMES)		)))

USE_PKT_SWITCHING ?= 0
NPORT_I ?= 1
NPORT_O ?= 1
POLYPHASE_ORDER ?= 0

PKT_SIZE = $(shell echo $$(( $(IN_WINDOW_VSIZE) / $(CASC_LEN) )))
PKT_SPLITTERS = $(shell echo $$(( $(CASC_LEN) / $(NPORT_I) )))
PKT_MERGERS = $(shell echo $$(( $(CASC_LEN) / $(NPORT_O) )))
INPUT_FILE_PATTERN = "data/input_[0-9]*_0.txt"
OUTPUT_FILE_PATTERN = "*simulator_output/data/output_pkts_[0-9]*_0.txt"

META_PARAM_MAP = DATA_TYPE $(DATA_TYPE) TWIDDLE_TYPE $(TWIDDLE_TYPE) POINT_SIZE $(POINT_SIZE) CASC_LEN $(CASC_LEN) NUM_FRAMES $(NUM_FRAMES) FFT_NIFFT $(FFT_NIFFT) SHIFT $(SHIFT) API_IO $(API_IO) AIE_VARIANT $(AIE_VARIANT) ROUND_MODE $(ROUND_MODE) SAT_MODE $(SAT_MODE) UUT_SSR $(UUT_SSR)
PARAM_MAP = $(META_PARAM_MAP) USE_PKT_SWITCHING $(USE_PKT_SWITCHING) POLYPHASE_ORDER $(POLYPHASE_ORDER) NPORT_I $(NPORT_I) NPORT_O $(NPORT_O) SINGLE_BUF $(SINGLE_BUF)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
LOC_REF_INPUT_FILE = ./data/input_ref_0_0.txt
REF_LOC_OUTPUT_FILE_X86 = ./data/ref_output.txt

# PLIO_WIDTH  = 64
PLIO_WIDTH  = 128

$(HELPER):create_input sim_ref prep_x86_out
	make cleanall

create_config:
	echo validating configuration;
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(META_PARAM_MAP);

create_input:
	echo starting create_input
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(REF_INPUT_FILE) $(NO_PAD_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) 0 0 $(DATA_TYPE) $(API_IO) 1 0 0 0 0 0
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/input_pad_and_split.pl --variant $(AIE_VARIANT) --file $(REF_INPUT_FILE) --newFile $(LOC_INPUT_FILE) --type $(DATA_TYPE) --pointSize $(POINT_SIZE) --cascLen $(CASC_LEN) --numFrames $(NUM_FRAMES) --ssr $(UUT_SSR) --plioWidth ${PLIO_WIDTH} --pkt_switch 1
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_recombine.py --combine 1 --input_pattern $(INPUT_FILE_PATTERN) --group_size $(PKT_SPLITTERS) --windowVsize ${PKT_SIZE}  --data_type $(DATA_TYPE) --polyphase_order $(POLYPHASE_ORDER) --output_file data/input_pkts
	cp $(REF_INPUT_FILE) $(LOC_REF_INPUT_FILE)
	echo Input ready

sim_ref:
	make UUT_KERNEL=dft_ref UUT_SIM_FILE=./data/ref_output.txt run TARGET=x86sim TAG=REF

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
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_recombine.py --output_pattern $(OUTPUT_FILE_PATTERN) --windowVsize ${OUT_WINDOW_VSIZE} --data_type $(DATA_TYPE) --output_file $(UUT_SIM_FILE) --combine 0 --group_size $(PKT_MERGERS) --polyphase_order $(POLYPHASE_ORDER)
	echo perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr ${UUT_SSR} --zip --dual 0 -k 0 -w ${OUT_WINDOW_VSIZE} --pldioWidth ${PLIO_WIDTH};
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(UUT_SIM_FILE) --type $(DATA_TYPE) --ssr ${UUT_SSR} --zip --dual 0 -k 0 -w ${OUT_WINDOW_VSIZE} --plioWidth ${PLIO_WIDTH};
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output_0_0.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE) PERCENT

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_qor:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)

	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(NO_PAD_WINDOW_VSIZE) $(NITER)

	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_input_0_0.txt -t_out_file uut_output_0_0.txt -status_file_dir $(STATUS_FILE) -num_of_samples $(NO_PAD_WINDOW_VSIZE) -niter $(NITER) -casc_len $(CASC_LEN) -aiesim_out_dir ./aiesimulator_output -ip dftMain 
