#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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

DUMMY_DYN_PT_SIZE=0
UUT_SSR=1
DUMMY_MAX_PT_SIZE=0
USE_PLIO=1
DUMMY_FFT_PARAM=0
SSR_INPUT_WINDOW_VSIZE=$(shell echo $$(($(UUT_SSR) * $(WINDOW_VSIZE))))
SSR_OUTPUT_WINDOW_VSIZE=$(SSR_INPUT_WINDOW_VSIZE)
USE_COEFF_RELOAD=0
DUMMY_COEFF_TYPE=int16
DUMMY_COEFF_STIM_TYPE=0
DUMMY_FIR_LEN=81
DUMMY_CASC_LEN=1
AIE_VARIANT?=1
DATA_SEED=1
MAX_DELAY=256
DUAL_INPUT_SAMPLES=0
DUAL_OUTPUT_SAMPLES=0
DUMMY_COEFF_RELOAD_HEADER_MODE=0
PARAM_MAP = DATA_TYPE $(DATA_TYPE) WINDOW_VSIZE $(WINDOW_VSIZE) PORT_API $(PORT_API) MAX_DELAY $(MAX_DELAY) AIE_VARIANT $(AIE_VARIANT)
STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt
HELPER_CUR_DIR ?= .

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(SSR_INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) $(DUMMY_DYN_PT_SIZE) $(DUMMY_MAX_PT_SIZE) $(DATA_TYPE) $(PORT_API) $(USE_PLIO)  $(DUMMY_FFT_PARAM) $(USE_COEFF_RELOAD) $(DUMMY_COEFF_TYPE) $(DUMMY_COEFF_STIM_TYPE) $(DUMMY_FIR_LEN) 64
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(INPUT_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --split --dual $(DUAL_INPUT_SAMPLES) -k $(DUMMY_COEFF_RELOAD_HEADER_MODE) -w ${SSR_INPUT_WINDOW_VSIZE} -c $(DUMMY_COEFF_TYPE) -fl $(DUMMY_FIR_LEN) --plioWidth 64;\

create_config:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config_json.tcl ./config.json ./ $(UUT_KERNEL) $(PARAM_MAP)

ssr_zip:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(UUT_SSR) --zip --dual $(DUAL_OUTPUT_SAMPLES) -k 0 -w $(SSR_OUTPUT_WINDOW_VSIZE) --plioWidth 64

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(WINDOW_VSIZE) $(DUMMY_CASC_LEN) $(STATUS_FILE) ./aiesimulator_output sample $(NITER)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts