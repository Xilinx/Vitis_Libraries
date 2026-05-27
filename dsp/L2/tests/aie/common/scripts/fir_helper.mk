
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

TAPYTHON = $(shell find $(XILINX_VITIS)/tps/lnx64/ -maxdepth 1 -type d -name "python-3*" | head -n 1)
VITIS_PYTHON3 = LD_LIBRARY_PATH=$(TAPYTHON)/lib $(TAPYTHON)/bin/python3

DATA_OUT_TYPE ?= $(DATA_TYPE)
UPSHIFT_CT ?= 0
CASC_LEN ?= 1
USE_COEFF_RELOAD ?= 0
INTERPOLATE_FACTOR ?= 1
DECIMATE_FACTOR ?= 1
UUT_SSR ?= 1
TDM_CHANNELS ?= 1
UUT_PARA_DECI_POLY ?= 1
UUT_PARA_INTERP_POLY ?= 1
USE_PKT_SWITCHING ?= 0
NPORT_I ?= 1
NPORT_O ?= 1
POLYPHASE_ORDER ?= 0
DIFF_TOLERANCE = 0.0025
# TODO: PARAM_MAP could be trimmed of unused params, e.g. single rate FIRs don't use interpolate/decimate factor or decomposition of these params. UUT_KERNEL has all the info that should be required...
ifeq ($(UUT_KERNEL), fir_tdm)
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DATA_OUT_TYPE $(DATA_OUT_TYPE) COEFF_TYPE $(COEFF_TYPE) FIR_LEN $(FIR_LEN) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) CASC_LEN $(CASC_LEN) DUAL_IP $(DUAL_IP) USE_COEFF_RELOAD $(USE_COEFF_RELOAD) NUM_OUTPUTS $(NUM_OUTPUTS) UUT_SSR $(UUT_SSR) PORT_API $(PORT_API) TDM_CHANNELS $(TDM_CHANNELS) USE_PKT_SWITCHING $(USE_PKT_SWITCHING) NPORT_I $(NPORT_I) NPORT_O $(NPORT_O) SINGLE_BUF $(SINGLE_BUF)
else
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DATA_OUT_TYPE $(DATA_OUT_TYPE) COEFF_TYPE $(COEFF_TYPE) FIR_LEN $(FIR_LEN) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) CASC_LEN $(CASC_LEN) DUAL_IP $(DUAL_IP) USE_COEFF_RELOAD $(USE_COEFF_RELOAD) NUM_OUTPUTS $(NUM_OUTPUTS) UUT_SSR $(UUT_SSR) PORT_API $(PORT_API) INTERPOLATE_FACTOR $(INTERPOLATE_FACTOR) DECIMATE_FACTOR $(DECIMATE_FACTOR) UUT_PARA_DECI_POLY $(UUT_PARA_DECI_POLY) UUT_PARA_INTERP_POLY $(UUT_PARA_INTERP_POLY) UPSHIFT_CT $(UPSHIFT_CT) SINGLE_BUF $(SINGLE_BUF)
endif
DUAL_INPUT_SAMPLES =  $(shell echo $$(($(PORT_API) * $(DUAL_IP))))
DUAL_OUTPUT_SAMPLES = $(shell echo $$(( $(PORT_API) * ($(NUM_OUTPUTS)-1) )))
OUTPUT_WINDOW_VSIZE = $(shell echo $$(( $(INPUT_WINDOW_VSIZE) * ($(INTERPOLATE_FACTOR))  / ($(DECIMATE_FACTOR))  )))

PKT_SIZE = $(shell echo $$(( $(INPUT_WINDOW_VSIZE) / $(UUT_SSR) )))
PKT_SPLITTERS = $(shell echo $$(( $(UUT_SSR) / $(NPORT_I) )))
PKT_MERGERS = $(shell echo $$(( $(UUT_SSR) / $(NPORT_O) )))
INPUT_FILE_PATTERN = "data/input_[0-9]*_0.txt"
OUTPUT_FILE_PATTERN = "*simulator_output/data/output_pkts_[0-9]*_0.txt"
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

HELPER_CUR_DIR ?= .
COEFF_STIM_TYPE ?= 0

ifeq ($(TAG), REF)
TAG_SSR_IN = 1
TAG_SSR_OUT = 1
TAG_DUAL_INP = 0
TAG_DUAL_OP = 0
else
TAG_SSR_IN = $(shell echo $$(( $(UUT_SSR) * $(UUT_PARA_DECI_POLY) )))
TAG_SSR_OUT = $(shell echo $$(( $(UUT_SSR) * $(UUT_PARA_INTERP_POLY) )))
TAG_DUAL_INP = $(DUAL_INPUT_SAMPLES)
TAG_DUAL_OP = $(DUAL_OUTPUT_SAMPLES)
endif

ifeq ($(DATA_TYPE), float)
CC_TOLERANCE = 0.0025
else ifeq ($(DATA_TYPE), cfloat)
CC_TOLERANCE = 0.0025
else
CC_TOLERANCE = 0
endif

# PLIO_WIDTH  = 64
PLIO_WIDTH  = 128

diff:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) $(PORT_API) 1 0 0 $(COEFF_TYPE) $(COEFF_STIM_TYPE) $(FIR_LEN) ${PLIO_WIDTH} 0 0

ssr_split:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(TAG_SSR_IN) --split --dual $(TAG_DUAL_INP) -k 0 -w ${INPUT_WINDOW_VSIZE} -c $(COEFF_TYPE) -fl $(FIR_LEN) -plioWidth ${PLIO_WIDTH}
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_recombine.py --combine 1 --input_pattern $(INPUT_FILE_PATTERN) --group_size $(PKT_SPLITTERS) --windowVsize ${PKT_SIZE}  --data_type $(DATA_TYPE) --polyphase_order $(POLYPHASE_ORDER) --output_file data/input_pkts

ssr_zip:
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_recombine.py --output_pattern $(OUTPUT_FILE_PATTERN) --windowVsize ${OUTPUT_WINDOW_VSIZE} --data_type $(DATA_OUT_TYPE) --output_file $(SPLIT_ZIP_FILE) --combine 0 --group_size $(PKT_MERGERS) --polyphase_order $(POLYPHASE_ORDER)
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_OUT_TYPE) --ssr $(TAG_SSR_OUT) --zip --dual $(TAG_DUAL_OP) -k 0 -w $(OUTPUT_WINDOW_VSIZE) -plioWidth ${PLIO_WIDTH}

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)

	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_qor.py -t_in_file T_input_0_0.txt -t_out_file uut_output_0_0.txt -status_file_dir $(STATUS_FILE) -num_of_samples $(INPUT_WINDOW_VSIZE) -niter $(NITER) -casc_len $(CASC_LEN) -aiesim_out_dir ./aiesimulator_output -ip filter
