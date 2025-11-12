#
# Copyright (C) 2025, Advanced Micro Devices, Inc.
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
# Makefile helper used for qrd compilation, simulation and QoR harvest.
###############################################################################

HELPER_CUR_DIR ?= .
HELPER_ROOT_DIR ?= ./../../../../
SEED ?= 0


WINDOW_VSIZE_Q := $(shell echo $$(( $(DIM_ROWS) * $(DIM_COLS) * $(NUM_FRAMES))))
WINDOW_VSIZE_R := $(shell echo $$(( $(DIM_COLS) * $(DIM_COLS) * $(NUM_FRAMES))))
WINDOW_VSIZE := $(shell awk 'BEGIN{print ($(WINDOW_VSIZE_Q) > $(WINDOW_VSIZE_R)) ? $(WINDOW_VSIZE_Q) : $(WINDOW_VSIZE_R)}')
DIM_SIZE_PADDED := $(WINDOW_VSIZE)
WINDOW_VSIZE_TOTAL := $(WINDOW_VSIZE_Q) + $(WINDOW_VSIZE_R)
DIM_A_LEADING ?= 0
DIM_Q_LEADING ?= 0
DIM_R_LEADING ?= 0

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DIM_ROWS $(DIM_ROWS) DIM_COLS $(DIM_COLS) NUM_FRAMES $(NUM_FRAMES) CASC_LEN $(CASC_LEN)


HELPER:= $(HELPER_CUR_DIR)/.HELPER

$(HELPER): sim_ref prep_x86_out
	# make cleanall

create_config:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP);

create_input:
	@echo starting create_input
	@echo LOC_INPUT_FILE_A $(LOC_INPUT_FILE_A)
	@echo LOC_INPUT_FILE_Q $(LOC_INPUT_FILE_Q)
	@echo LOC_INPUT_FILE_R $(LOC_INPUT_FILE_R)

	@echo DIM_COLS         $(DIM_COLS)
	@echo DIM_ROWS         $(DIM_ROWS)
	@echo NUM_FRAMES       $(NUM_FRAMES)
	@echo WINDOW_VSIZE_Q   $(WINDOW_VSIZE_Q)
	@echo WINDOW_VSIZE_R   $(WINDOW_VSIZE_R)
	@echo WINDOW_VSIZE     $(WINDOW_VSIZE)
	@echo DIM_SIZE_PADDED  $(DIM_SIZE_PADDED)

	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/qrd/gen_input_qrd.py --loc_input_file $(LOC_INPUT_FILE_A) --aie_variant $(AIE_VARIANT) --data_type $(DATA_TYPE) --row_dim_size $(DIM_ROWS) --col_dim_size $(DIM_COLS) --casc_len $(CASC_LEN) --num_frames $(NUM_FRAMES) --niter $(NITER) --seed $(SEED) --pliowidth 64 --dim_a_leading $(DIM_A_LEADING)

	echo Input ready

sim_ref:
	make UUT_KERNEL=qrd_ref run TARGET=x86sim TAG=REF

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
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/qrd/stitch_output_qrd.py --loc_output_file_Q $(UUT_SIM_FILE_Q) --loc_output_file_R $(UUT_SIM_FILE_R) --aie_variant $(AIE_VARIANT) --data_type $(DATA_TYPE) --row_dim_size $(DIM_ROWS) --col_dim_size $(DIM_COLS) --casc_len $(CASC_LEN) --num_frames $(NUM_FRAMES) --niter $(NITER) --plioWidth 64 --dim_q_leading $(DIM_Q_LEADING) --dim_r_leading $(DIM_R_LEADING) 

	cat $(UUT_SIM_FILE_Q)  $(UUT_SIM_FILE_R)  > ./data/uut_output.txt
	cat $(REF_SIM_FILE_Q)  $(REF_SIM_FILE_R)  > ./data/ref_output.txt

	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE)

	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_qrd.py --A ./data/input_A.txt --Q ./data/ref_output_Q.txt --R ./data/ref_output_R.txt --dim_rows $(DIM_ROWS) --dim_cols $(DIM_COLS) --data_type $(DATA_TYPE) --dim_a_leading $(DIM_A_LEADING) --dim_q_leading $(DIM_Q_LEADING) --dim_r_leading $(DIM_R_LEADING) --num_frames $(NUM_FRAMES) --niter $(NITER) --model_ut "Ref"
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/validation_tests/test_qrd.py --A ./data/input_A.txt --Q ./data/uut_output_Q.txt --R ./data/ref_output_R.txt --dim_rows $(DIM_ROWS) --dim_cols $(DIM_COLS) --data_type $(DATA_TYPE) --dim_a_leading $(DIM_A_LEADING) --dim_q_leading $(DIM_Q_LEADING) --dim_r_leading $(DIM_R_LEADING)  --num_frames $(NUM_FRAMES) --niter $(NITER) --model_ut "UUT"

get_status:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	sh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_A_0.txt ./data/uut_output_Q_0.txt $(STATUS_FILE) $(WINDOW_VSIZE_Q) $(NITER)


get_stats:
	tclsh $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) 1 $(STATUS_FILE) ./aiesimulator_output "qrd" $(NITER)
	$(DSP_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(DSP_ROOT_DIR)/L2/tests/aie/common/scripts


	