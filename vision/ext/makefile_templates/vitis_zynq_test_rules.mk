#
# Copyright 2019 Xilinx, Inc.
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

# This makefile specifies how the host code will be compiled.
# If EXE_FILE is set to empty, nothing will be built.

# MK_BEGIN

# -----------------------------------------------------------------------------
#                                clean up

clean:
ifneq (,$(OBJ_DIR_BASE))
	rm -rf $(CUR_DIR)/$(OBJ_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*
endif

cleanx:
ifneq (,$(VPP_DIR_BASE))
	rm -rf $(CUR_DIR)/$(VPP_DIR_BASE)*
endif
ifneq (,$(XO_DIR_BASE))
	rm -rf $(CUR_DIR)/$(XO_DIR_BASE)*
endif
ifneq (,$(XCLBIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(XCLBIN_DIR_BASE)*
endif
ifneq (,$(BIN_DIR_BASE))
	rm -rf $(CUR_DIR)/$(BIN_DIR_BASE)*/emconfig.json
endif

cleanall: clean cleanx
	rm -rf *.log plist $(DATA_STAMP)
	rm -rf *.ini sd_card* emulation _vimage .Xil
# -----------------------------------------------------------------------------
#                                simulation run

$(BIN_DIR)/emconfig.json :
	emconfigutil --platform $(XPLATFORM) --od $(BIN_DIR)

    ifeq ($(TARGET), $(filter $(TARGET),sw_emu hw_emu))
        EMU_CONFIG = $(BIN_DIR)/emconfig.json
    endif

emulate : host xclbin $(EMU_CONFIG) sd_card
ifeq ($(TARGET), $(filter $(TARGET),sw_emu hw_emu))
       RUN_ENV += mkdir -p sd_card/data/emulation;
       RUN_ENV += cp -rf $(XILINX_VITIS)/data/emulation/unified sd_card/data/emulation;
       RUN_ENV += mkfatimg sd_card sd_card.img 500000;
       EMU_CMD = launch_emulator -no-reboot -runtime ocl -t $(TARGET) -sd-card-image sd_card.img -device-family $(DEV_FAM);
else ifeq ($(TARGET), hw)
       RUN_ENV += 
       EMU_CMD += @echo "Please copy the contents of sd_card and input images to an SD Card and run on the board"
endif

sd_card : host xclbin
	@echo "Generating sd_card folder...."
	mkdir -p $(SDCARD)/$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)
	cp -rf $(XCLBIN_DIR)/*.xclbin $(SDCARD)/$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)
	cp -rf $(B_NAME)/sw/$(XDEVICE)/boot/generic.readme $(B_NAME)/sw/$(XDEVICE)/xrt/image/* $(EXE_FILE) $(HOST_ARGS) $(SDCARD)
ifeq ($(TARGET), $(filter $(TARGET),sw_emu hw_emu))
	@echo 'cd /mnt/' >> $(SDCARD)/init.sh
	@echo 'export XILINX_VITIS=$$PWD' >> $(SDCARD)/init.sh
	@echo 'export XCL_BINDIR=$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)' >> $(SDCARD)/init.sh
	@echo 'export XCL_EMULATION_MODE=$(TARGET)' >> $(SDCARD)/init.sh
	@echo './$(EXE_NAME).$(EXE_EXT) $(notdir $(HOST_ARGS))' >> $(SDCARD)/init.sh
	@echo 'reboot' >> $(SDCARD)/init.sh
	@echo "sd_card folder generation done!"
else ifeq ($(TARGET), hw)
	@echo 'cp -rf $(XCLBIN_DIR)/BOOT.BIN $(SDCARD)/'
	@echo 'export XCL_BINDIR=$(XCLBIN_DIR_BASE)$(XCLBIN_DIR_SUFFIX)' >> $(SDCARD)/init.sh
	@echo './$(EXE_NAME).$(EXE_EXT) $(notdir $(HOST_ARGS))' >> $(SDCARD)/init.sh
	@echo "sd_card folder generation done!"
endif



.PHONY: run run_sw_emu run_hw_emu run_hw check

run_sw_emu:
	make TARGET=sw_emu run

run_hw_emu:
	make TARGET=hw_emu run

run_hw:
	make TARGET=hw run

run: host xclbin $(EMU_CONFIG) emulate
	$(RUN_ENV)
	$(EMU_CMD)

check: run
