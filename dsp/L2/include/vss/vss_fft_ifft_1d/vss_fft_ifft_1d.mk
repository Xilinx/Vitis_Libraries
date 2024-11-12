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

VSS        ?= vss_fft_ifft_1d
AIETARGET  ?= hw
PART       = xcvc1902-vsva2197-2MP-e-S
VSSCFG     = ${VSS}_connectivity.cfg
FRONT_XO   = ifft_front_transpose_wrapper/ifft_front_transpose_wrapper.xo
MID_XO     = ifft_transpose_wrapper/ifft_transpose_wrapper.xo
BACK_XO    = ifft_back_transpose_wrapper/ifft_back_transpose_wrapper.xo
XO         = ${FRONT_XO} ${MID_XO} ${BACK_XO}
PARAMS_CFG = ${VSS}_params.cfg
HPARAMS    = hls_params.cfg
CFGFILE    = hls_config.cfg
AIE_PARAMS = aie_params.cfg
AIE_HDR    = config.h
LIBADF     = libadf.a
VSSFILE    = ${VSS}/${VSS}.vss

FRONT_XO_CFG     = ifft_front_transpose_config.cfg
BACK_XO_CFG      = ifft_back_transpose_config.cfg
MID_XO_CFG       = ifft_transpose_config.cfg
CONFIG_TMPL      = ${HELPER_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/hls_config.tmpl

DSPLIB_OPTS 	  := -I ${XILINX_VITIS}/aietools/include  \
                	 -I ${HELPER_ROOT_DIR}/L1/include/aie \
                	 -I ${HELPER_ROOT_DIR}/L1/src/aie \
                	 -I ${HELPER_ROOT_DIR}/L1/tests/aie/inc \
                	 -I ${HELPER_ROOT_DIR}/L1/tests/aie/src \
                	 -I ${HELPER_ROOT_DIR}/L2/include/aie \
                	 -I ${HELPER_ROOT_DIR}/L2/tests/aie/common/inc \
                	 -I ${HELPER_ROOT_DIR}/L2/tests/vss/vss_fft_ifft_1d/ \
                     --mode aie --target=$(AIETARGET) \
                     --aie.verbose  \
                     --aie.Xchess=llvm.xargs=\"-std=c++2a\" \
                     --aie.Xchess=main:backend.mist2.xargs=\"+NOdra\" \
                     --aie.Xchess=main:backend.mist2.pnll=\"off\" \
                     --aie.Xpreproc=-DOUTPUT_FILE=./data/uut_output.txt \
                     --aie.Xpreproc=-DFRONT_INPUT_FILE=./data/input_front.txt \
                     --aie.Xpreproc=-DBACK_INPUT_FILE=./data/input_back.txt \
                     --aie.Xpreproc=-DFRONT_OUTPUT_FILE=./data/output_front.txt \
                     --aie.Xpreproc=-DBACK_OUTPUT_FILE=./data/output_back.txt

VITIS_PYTHON3 = LD_LIBRARY_PATH=$(XILINX_VITIS)/tps/lnx64/python-3.8.3/lib $(XILINX_VITIS)/tps/lnx64/python-3.8.3/bin/python3

libadf: ${LIBADF}
tb_config: ${AIE_HDR}
aie_params: ${AIE_PARAMS}
front_xo: ${FRONT_XO}
back_xo: ${BACK_XO}
mid_xo: ${MID_XO}
vss: ${VSSFILE}
vsscfg: ${VSSCFG}

hls_freq = $(shell $(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_param_cfg.py $(HELPER_CUR_DIR) "freqhz" "top" $(PARAMS_CFG))
${VSSFILE}: ${LIBADF} ${FRONT_XO} ${BACK_XO} ${MID_XO} ${VSSCFG}
	v++ --link --mode vss --part $(shell $(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_param_cfg.py $(HELPER_CUR_DIR) "PART" "top" $(PARAMS_CFG)) --save-temps  --out_dir ${VSS} --config ${VSSCFG} ${LIBADF} ${FRONT_XO} ${BACK_XO} ${MID_XO}

${VSSCFG}: ${HELPER_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py
	$(VITIS_PYTHON3) ${HELPER_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/vss_fft_ifft_1d_con_gen.py  --ssr $(shell $(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_param_cfg.py $(HELPER_CUR_DIR) "SSR" "APP_PARAMS" $(PARAMS_CFG)) --cfg_file_name ${VSSCFG} --vss_unit ${VSS} --version 1 --freqhz ${hls_freq}

${AIE_HDR}: $(PARAMS_CFG) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/gen_tb_from_cfg.py 
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/gen_tb_from_cfg.py $(HELPER_ROOT_DIR) $(HELPER_CUR_DIR) ${PARAMS_CFG}

${AIE_PARAMS}: $(PARAMS_CFG) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_aie_cfg.py
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_aie_cfg.py $(HELPER_CUR_DIR)  ${PARAMS_CFG}

${HPARAMS}: $(PARAMS_CFG) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_hls_cfg.py 
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/scripts/extract_hls_cfg.py $(HELPER_CUR_DIR) ${HPARAMS} ${PARAMS_CFG}

${LIBADF}: ${AIE_HDR} ${AIE_PARAMS}
	v++ -c \
	--config ${AIE_PARAMS} \
	${DSPLIB_OPTS} \
    -o ${LIBADF} \
    ${HELPER_ROOT_DIR}/L2/include/vss/vss_fft_ifft_1d/aie.cpp ${HELPER_ROOT_DIR}/L1/src/aie/twiddle_rotator.cpp ${HELPER_ROOT_DIR}/L1/src/aie/fft_ifft_dit_1ch.cpp

${FRONT_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} $(HELPER_ROOT_DIR)/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(HELPER_CUR_DIR) ${HELPER_ROOT_DIR} ifft_front_transpose  ${PARAMS_CFG}

${FRONT_XO}: ${FRONT_XO_CFG} ${HPARAMS} ${HELPER_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.cpp  ${HELPER_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.h  ${HELPER_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose_class.h
	v++ --compile --mode hls --config ${FRONT_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${HELPER_ROOT_DIR}/L1/src/hw/ifft_front_transpose/ifft_front_transpose.cpp

${BACK_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} $(HELPER_ROOT_DIR)/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(HELPER_CUR_DIR) ${HELPER_ROOT_DIR} ifft_back_transpose  ${PARAMS_CFG}

${BACK_XO}: ${BACK_XO_CFG} ${HPARAMS} ${HELPER_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.cpp  ${HELPER_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.h  ${HELPER_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose_class.h
	v++ --compile --mode hls --config ${BACK_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${HELPER_ROOT_DIR}/L1/src/hw/ifft_back_transpose/ifft_back_transpose.cpp

${MID_XO_CFG}: ${PARAMS_CFG} ${CONFIG_TMPL} $(HELPER_ROOT_DIR)/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py
	$(VITIS_PYTHON3) $(HELPER_ROOT_DIR)/L2/include/vss/vss_fft_ifft_1d/create_hls_cfg.py $(HELPER_CUR_DIR) ${HELPER_ROOT_DIR} ifft_transpose  ${PARAMS_CFG}

${MID_XO}: ${MID_XO_CFG} ${HPARAMS} ${HELPER_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.cpp  ${HELPER_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.h  ${HELPER_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose_class.h
	v++ --compile --mode hls --config ${MID_XO_CFG} --config ${HPARAMS} --freqhz ${hls_freq} ${HELPER_ROOT_DIR}/L1/src/hw/ifft_transpose/ifft_transpose.cpp

help:
	echo "Makefile Usage:"
	echo "  make -f vss_fft_ifft_1d.mk vss HELPER_CUR_DIR=<path/to/build/dir> HELPER_ROOT_DIR=<path/to/dsp/library/root>"
	echo "      Command to generate the full vss according to the parameters defined in vss_fft_ifft_1d_params.cfg. Please edit the vss_fft_ifft_1d_params.cfg"
	echo ""
	echo "  make clean"
	echo "      Command to remove the generated files."
	echo ""

clean:
	rm -rf .crashReporter* .Xil _x ${VSS} *.log *summary ${HPARAMS} ${VSSCFG} ${CFGFILE} ${LIBADF} ${FRONT_XO} ${BACK_XO} ${MID_XO} ${AIE_PARAMS} ${CUR_DIR}/Work ${AIE_HDR} ${FRONT_XO_CFG} ${BACK_XO_CFG} ${MID_XO_CFG} ifft_back_transpose_wrapper ifft_front_transpose_wrapper ifft_transpose_wrapper sol.db map_report.csv
