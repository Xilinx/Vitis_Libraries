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
PARAMS ?=test_0_tool_canary_aie
PARAMS_FILE ?=multi_params.json
RESULTS_DIR ?=./results/$(PARAMS)

ifndef PLATFORM
	ifeq ($(findstring aie1,$(PARAMS)),aie1)
		PLATFORM=vck190
	else ifeq ($(findstring aie2,$(PARAMS)),aie2)
		PLATFORM=vek280
	endif
endif
ifndef TARGET
	ifeq ($(findstring hw,$(PARAMS)),hw)
		TARGET=aiesim
	else ifeq ($(findstring x86sim,$(PARAMS)),x86sim)
		TARGET=x86sim
	else
		TARGET=aiesim
	endif
endif

all_pack:
	@echo PARAMS=$(PARAMS)
	@echo PARAMS_FILE=$(PARAMS_FILE)
	@echo PLATFORM=$(PLATFORM)
	@echo TARGET=$(TARGET)
	@echo RESULTS_DIR=$(RESULTS_DIR)
	@rm -rf $(RESULTS_DIR)
	@mkdir -p $(RESULTS_DIR)/logs $(RESULTS_DIR)/data
	@if [ -f test.cpp ]; then \
		cp -f test.cpp $(RESULTS_DIR) ;\
	fi
	@if [ -f test.hpp ]; then \
		cp -f test.hpp $(RESULTS_DIR) ;\
	fi
	@if [ -f uut_static_config.h ]; then \
		cp -f uut_static_config.h $(RESULTS_DIR) ;\
	fi
	@cp -f utils.mk $(RESULTS_DIR)
	@cp -f Makefile $(RESULTS_DIR)
	@cp -f $(PARAMS_FILE) $(RESULTS_DIR)
	@cp -f description.json $(RESULTS_DIR)
	@if [ -f helper.mk ]; then \
		cp -f helper.mk $(RESULTS_DIR) ;\
	fi
	@if [ -f host.cpp ]; then \
		cp -f host.cpp $(RESULTS_DIR) ;\
	fi
	@if [ -f vss_generator.py ]; then \
		cp -f vss_generator.py $(RESULTS_DIR) ;\
	fi
	@if [ -f paramset.py ]; then \
		cp -f paramset.py $(RESULTS_DIR) ;\
	fi
	@if [ -f system.cfg ]; then \
		cp -f system.cfg $(RESULTS_DIR) ;\
	fi
	@if [ -f sim_options.txt ]; then \
		cp -f sim_options.txt $(RESULTS_DIR) ;\
	fi
	@if [ -f aie_libadf.mk ]; then \
		cp -f aie_libadf.mk $(RESULTS_DIR) ;\
	fi
	@make -C $(RESULTS_DIR) cleanall PLATFORM=$(PLATFORM) >  $(RESULTS_DIR)/logs/log_$(PARAMS).txt 
	@make -C $(RESULTS_DIR) run TARGET=$(TARGET) PARAMS=$(PARAMS) PARAMS_FILE=$(PARAMS_FILE) PLATFORM=$(PLATFORM) 2>&1 | tee $(RESULTS_DIR)/logs/log_$(PARAMS).txt
