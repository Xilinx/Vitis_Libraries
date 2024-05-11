#!/bin/bash
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
CURRENT_DIR=$1
UUT_KERNEL=$2
STATUS_FILE=$3
AIE_VARIANT=$4

PWR_DIR=./pwr_test
VCD_DIR=${CURRENT_DIR}/${UUT_KERNEL}_sim.vcd

if [ $AIE_VARIANT == 1 ]; then 
    PART="XCVC1902-VSVD1760-1LP-E-S"
    elif [ $AIE_VARIANT == 2 ]; then 
    PART="XCVE2802-VSVH1760-1MP-E-S"
fi

if [ -f "$VCD_DIR" ]; then #does the vcd file exist to run the power tests?
    # create power folder
    if [ -d "$PWR_DIR" ]; then
        rm -rf $PWR_DIR
    fi
    mkdir $PWR_DIR

    vcdanalyze --vcd $VCD_DIR --xpe --xpe-dir $PWR_DIR

    #write the python file for pdm calculation
    echo 'from librdi_pypdmtasks import *
prj = PdmProjectMgr.new("new_proj.pdm", part="'${PART}'", process="Typ")
prj.import_xpe("./test.xpe")
prj.save()
prj.export_power_design("./test_pwr.xml")
prj.export_xdc("./test_xdc.xdc")
dyn_pwr=prj.dynamic_power
prj.close()
f = open(".'${STATUS_FILE}'", "a")
f.write("    Dynamic Power:        " +  str(dyn_pwr) + " W\n")
f.close()
exit()
'> $PWR_DIR/pwr_analyze.py

    cd $PWR_DIR
    pdm -mode py -script ./pwr_analyze.py
    cd $CURRENT_DIR    
else
    echo "No VCD file found! Pwr analysis is not performed."
fi





# print("vivado_part          = ", prj.vivado_part         )
# print("family               = ", prj.family              )
# print("device_grade         = ", prj.device_grade        )
# print("device               = ", prj.device              )
# print("package              = ", prj.package             )
# print("speed                = ", prj.speed               )
# print("static_screen        = ", prj.static_screen       )
# print("temperature          = ", prj.temperature         )
# print("vccint_voltage       = ", prj.vccint_voltage      )
# print("process              = ", prj.process             )
# print("char_status          = ", prj.char_status         )
# print("total_power          = ", prj.total_power         )
# print("static_power         = ", prj.static_power        )
# print("dynamic_power        = ", prj.dynamic_power       )
# print("junction_temperature = ", prj.junction_temperature)
# print("ambient_temperature  = ", prj.ambient_temperature )
# print("theta_ja             = ", prj.theta_ja            )
# print("design_power_budget  = ", prj.design_power_budget )
