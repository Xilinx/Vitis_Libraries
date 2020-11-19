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

RTM_x=36
RTM_y=60
RTM_z=40

MaxY = 36
MaxZ = 128 
MaxB = 10
NTime=4
PE=2
NUM_INST=2
ORDER=8

PYTHON_SCRIPT=../../../sw/python/rtm3d/operation.py

params:
	@echo "#pragma once"            > params.hpp
	@echo "#define ORDER 8"         >> params.hpp
	@echo "#define RTM_x ${RTM_x}"		>> params.hpp 
	@echo "#define RTM_y ${RTM_y}"		>> params.hpp 
	@echo "#define RTM_z ${RTM_z}"		>> params.hpp 
	@echo "#define NTime ${NTime}"	>> params.hpp 
	@echo "#define MaxB ${MaxB}"		>> params.hpp 
	@echo "#define MaxY ${MaxY}"		>> params.hpp 
	@echo "#define MaxZ ${MaxZ}"		>> params.hpp 
	@echo "#define NX (RTM_x - 2 * MaxB)"    >> params.hpp 
	@echo "#define NY (RTM_y - 2 * MaxB)"    >> params.hpp 
	@echo "#define NZ (RTM_z - 2 * MaxB)"   >> params.hpp 
	@echo "#define NUM_INST ${NUM_INST}"    >> params.hpp 
	@echo "#define nPE ${PE}"				>> params.hpp 
	@echo "typedef float DATATYPE;"         >> params.hpp 


OP_SCRIPT=../../../sw/python/rtm3d/operation.py
DES_SCRIPT=../../../sw/python/rtm3d/description.py
TOOL_SCRIPT=/home/liangm/wrk/nobkup/GithubEnterprice/faas_tools_kit/case_gen/gen_hls_case/gen_hls_case.py
TEST_DIR=tests/RTM_x${RTM_x}_y${RTM_y}_z${RTM_z}_t${NTime}_p${PE}

case_gen: data_gen params
	mkdir -p ${TEST_DIR}
	mv params.hpp ${TEST_DIR}
	python ${DES_SCRIPT} --func forward --testDir ${TEST_DIR} 
	${TOOL_SCRIPT} ${TEST_DIR}/description.json

hls: case_gen
	cd ${TEST_DIR} && \
	make run XPART='xcu200-fsgd2104-2-e' COSIM=1 CSIM=1
	cd ${TEST_DIR} && \
	vitis_hls -f run_hls.tcl

data_gen:
	python ${PYTHON_SCRIPT} --func testForward --path ${TEST_DIR}/data --x ${RTM_x} --y ${RTM_y} --z ${RTM_z} --nxb ${MaxB} --nyb ${MaxB} --nzb ${MaxB} --time ${NTime} --order ${ORDER} --rbc

clean:
	@rm -rf hls_prj/
	@rm -rf params.hpp
	@rm -rf *.txt
	@rm -rf *.log
	@rm -rf xsim*
	@rm -rf data/
