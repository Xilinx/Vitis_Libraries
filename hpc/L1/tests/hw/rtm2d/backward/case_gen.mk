
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


WIDTH=128
HEIGHT=128
NXB=20
NZB=20
NT=15
PE=2
NUM_INST=3


PYTHON_SCRIPT=../../../sw/python/rtm2d/operation.py
TEST_DIR=tests/data_h${HEIGHT}_w${WIDTH}_t${NT}_pe${PE}

params:
	@echo "#pragma once"                    > params.hpp
	@echo "#define ORDER 8"                 >> params.hpp
	@echo "#define WIDTH ${WIDTH}"			>> params.hpp 
	@echo "#define HEIGHT ${HEIGHT}"		>> params.hpp 
	@echo "#define NTime ${NT}"				>> params.hpp 
	@echo "#define NXB ${NXB}"				>> params.hpp 
	@echo "#define NZB ${NZB}"					>> params.hpp 
	@echo "#define NX (WIDTH - 2 * NXB)"    >> params.hpp 
	@echo "#define NZ (HEIGHT - 2 * NZB)"   >> params.hpp 
	@echo "#define NUM_INST ${NUM_INST}"    >> params.hpp 
	@echo "#define nPE ${PE}"               >> params.hpp 
	@echo "typedef float DATATYPE;"         >> params.hpp 

DES_SCRIPT=../../../sw/python/rtm2d/description.py
TOOL_SCRIPT=/home/liangm/wrk/nobkup/GithubEnterprice/faas_tools_kit/case_gen/gen_hls_case/gen_hls_case.py

case_gen: data_gen params
	mkdir -p ${TEST_DIR}
	mv params.hpp ${TEST_DIR}
	python ${DES_SCRIPT} --func backward --testDir ${TEST_DIR} 
	${TOOL_SCRIPT} ${TEST_DIR}/description.json

hls: case_gen
	cd ${TEST_DIR} && \
	make run XPART='xcu200-fsgd2104-2-e' COSIM=1 CSIM=1
	cd ${TEST_DIR} && \
	vitis_hls -f run_hls.tcl

data_gen:
	python ${PYTHON_SCRIPT} --func testBackward --path ./${TEST_DIR}/data/ --width ${WIDTH} --depth ${HEIGHT} --nxb ${NXB} --nzb ${NZB} --time ${NT}

clean:
	@rm -rf hls_prj/
	@rm -rf params.hpp
	@rm -rf *.txt
	@rm -rf *.log
	@rm -rf xsim*
	@rm -rf data/
