# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
import os
import xml.etree.ElementTree as ET


def harvest_cycles(aiesimulator_output_dir, cores):
    """
    Per-core kernel cycle and instruction counts from the AIE simulator function profiles.

    Cycles and instructions are per call of the kernel function, including its callees. Cycles
    include stalls (e.g. waiting on a lock, stream or cascade), so stall cycles are roughly
    KERNEL_CYCLES_AVG - KERNEL_INSTRUCTIONS_AVG.

    Args:
        aiesimulator_output_dir (str): Directory containing profile_funct_<col>_<row>.xml files.
        cores (dict): CORES dictionary from harvest_memory; each entry needs a single FUNCTIONS name.

    Returns:
        dict: {core: {field: value}} to be merged into the matching CORES entries.
    """
    per_core = {}
    for core, entry in cores.items():
        (function_name,) = entry["FUNCTIONS"]
        root = ET.parse(os.path.join(aiesimulator_output_dir, f"profile_funct_{core}.xml")).getroot()
        function = next(f for f in root.findall("function") if f.find("function_name").text == function_name)
        cycles = function.find("function_and_descendants_time")
        instructions = function.find("function_and_descendants_instruction_count")
        per_core[core] = {
            "KERNEL_CYCLES_AVG":        int(cycles.find("average_cycle_count").text),
            # "KERNEL_CYCLES_MIN":        int(cycles.find("minimum_cycle_count").text),
            # "KERNEL_CYCLES_MAX":        int(cycles.find("maximum_cycle_count").text),
            "KERNEL_INSTRUCTIONS_AVG":  int(instructions.find("average_instruction_count").text),
        }
    return per_core
