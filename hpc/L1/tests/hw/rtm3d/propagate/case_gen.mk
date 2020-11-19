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

x=28
y=36
z=20
N_INST=1
PE=2
ORDER=8
maxD=128

PYTHON_SCRIPT=../../../sw/python/rtm3d/operation.py

params:
	@echo "#pragma once"                    > params.hpp
	@echo "#define ORDER ${ORDER}"                 >> params.hpp
	@echo "#define M_y ${y}"			>> params.hpp 
	@echo "#define M_x ${x}"		>> params.hpp 
	@echo "#define M_z ${z}"		>> params.hpp 
	@echo "#define MaxD   ${maxD}"		>> params.hpp 
	@echo "#define N_INST ${N_INST}"				>> params.hpp 
	@echo "#define nPE ${PE}"                   >> params.hpp 
	@echo "typedef float DATATYPE;"         >> params.hpp 

DES_SCRIPT=../../../sw/python/rtm3d/description.py
TOOL_SCRIPT=/home/liangm/wrk/nobkup/GithubEnterprice/faas_tools_kit/case_gen/gen_hls_case/gen_hls_case.py
TEST_DIR=tests/RTM_x${x}_y${y}_z${z}_t${NT}_p${PE}

case_gen: data_gen params
	rm -rf ${TEST_DIR}
	mkdir -p ${TEST_DIR}
	mv params.hpp ${TEST_DIR}
	mv ./data ${TEST_DIR}
	python ${DES_SCRIPT} --func propagate --testDir ${TEST_DIR} 
	${TOOL_SCRIPT} ${TEST_DIR}/description.json

hls: case_gen
	cd ${TEST_DIR} && \
	make run XPART='xcu200-fsgd2104-2-e' COSIM=1 CSIM=1
	cd ${TEST_DIR} && \
	vitis_hls -f run_hls.tcl

data_gen:
	python ${PYTHON_SCRIPT} --func testPropagate --path ./data --x ${x} --y ${y} --z ${z} --time ${N_INST} --order ${ORDER}

clean:
	@rm -rf hls_prj/
	@rm -rf params.hpp
	@rm -rf *.txt
	@rm -rf *.log
	@rm -rf xsim*
	@rm -rf data/
