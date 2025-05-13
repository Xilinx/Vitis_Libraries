
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
DIFF_TOLERANCE = 0.0025
PARAM_MAP = AIE_VARIANT $(AIE_VARIANT) DATA_TYPE $(DATA_TYPE) DATA_OUT_TYPE $(DATA_OUT_TYPE) COEFF_TYPE $(COEFF_TYPE) FIR_LEN $(FIR_LEN) SHIFT $(SHIFT) ROUND_MODE $(ROUND_MODE) INPUT_WINDOW_VSIZE $(INPUT_WINDOW_VSIZE) CASC_LEN $(CASC_LEN) DUAL_IP $(DUAL_IP) USE_COEFF_RELOAD $(USE_COEFF_RELOAD) NUM_OUTPUTS $(NUM_OUTPUTS) UUT_SSR $(UUT_SSR) PORT_API $(PORT_API) INTERPOLATE_FACTOR $(INTERPOLATE_FACTOR) DECIMATE_FACTOR $(DECIMATE_FACTOR) UUT_PARA_DECI_POLY $(UUT_PARA_DECI_POLY) UUT_PARA_INTERP_POLY $(UUT_PARA_INTERP_POLY) UPSHIFT_CT $(UPSHIFT_CT) TDM_CHANNELS $(TDM_CHANNELS)
DUAL_INPUT_SAMPLES =  $(shell echo $$(($(PORT_API) * $(DUAL_IP))))
DUAL_OUTPUT_SAMPLES = $(shell echo $$(( $(PORT_API) * ($(NUM_OUTPUTS)-1) )))
UUT_COEFF_RELOAD_HEADER_MODE = $(shell echo $$(( $(USE_COEFF_RELOAD) & 2)))
REF_COEFF_RELOAD_HEADER_MODE = $(shell echo $$(( ($(USE_COEFF_RELOAD) & 2)  * 100 + 99 )))
OUTPUT_WINDOW_VSIZE = $(shell echo $$(( $(INPUT_WINDOW_VSIZE) * ($(INTERPOLATE_FACTOR))  / ($(DECIMATE_FACTOR))  )))

STATUS_FILE = ./logs/status_$(UUT_KERNEL)_$(PARAMS).txt

HELPER_CUR_DIR ?= .
COEFF_STIM_TYPE ?= 0

ifeq ($(TAG), REF)
TAG_SSR_IN = 1
TAG_SSR_OUT = 1
TAG_DUAL_INP = 0
TAG_DUAL_OP = 0
TAG_COEFF_RELOAD_HEADER_MODE = $(REF_COEFF_RELOAD_HEADER_MODE)
else
TAG_SSR_IN = $(shell echo $$(( $(UUT_SSR) * $(UUT_PARA_DECI_POLY) )))
TAG_SSR_OUT = $(shell echo $$(( $(UUT_SSR) * $(UUT_PARA_INTERP_POLY) )))
TAG_DUAL_INP = $(DUAL_INPUT_SAMPLES)
TAG_COEFF_RELOAD_HEADER_MODE = $(UUT_COEFF_RELOAD_HEADER_MODE)
TAG_DUAL_OP = $(DUAL_OUTPUT_SAMPLES)
endif

ifeq ($(DATA_TYPE), float)
CC_TOLERANCE = 0.0025
else ifeq ($(DATA_TYPE), cfloat)
CC_TOLERANCE = 0.0025
else
CC_TOLERANCE = 0
endif

diff:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/diff.tcl ./data/uut_output.txt ./data/ref_output.txt ./logs/diff.txt $(DIFF_TOLERANCE) $(CC_TOLERANCE)

gen_input:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/gen_input.tcl $(INPUT_FILE) $(INPUT_WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(DATA_STIM_TYPE) 0 0 $(DATA_TYPE) $(PORT_API) 1 0 0 $(COEFF_TYPE) $(COEFF_STIM_TYPE) $(FIR_LEN) 64

ssr_split:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_TYPE) --ssr $(TAG_SSR_IN) --split --dual $(TAG_DUAL_INP) -k 0 -w ${INPUT_WINDOW_VSIZE} -c $(COEFF_TYPE) -fl $(FIR_LEN) -plioWidth 64

ssr_zip:
	perl $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/ssr_split_zip.pl --file $(SPLIT_ZIP_FILE) --type $(DATA_OUT_TYPE) --ssr $(TAG_SSR_OUT) --zip --dual $(TAG_DUAL_OP) -k 0 -w $(OUTPUT_WINDOW_VSIZE) -plioWidth 64

get_status:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_common_config.tcl $(STATUS_FILE) ./ UUT_KERNEL $(UUT_KERNEL) $(PARAM_MAP) SINGLE_BUF $(SINGLE_BUF)

get_latency:
	sh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_pwr.sh $(HELPER_CUR_DIR) $(UUT_KERNEL) $(STATUS_FILE) $(AIE_VARIANT)
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_latency.tcl ./aiesimulator_output T_input_0_0.txt ./data/uut_output_0_0.txt $(STATUS_FILE) $(INPUT_WINDOW_VSIZE) $(NITER)

get_stats:
	tclsh $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/get_stats.tcl $(INPUT_WINDOW_VSIZE) $(CASC_LEN) $(STATUS_FILE) ./aiesimulator_output filter $(NITER)

harvest_mem:
	$(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts/harvest_memory.sh $(STATUS_FILE) $(HELPER_ROOT_DIR)/L2/tests/aie/common/scripts
