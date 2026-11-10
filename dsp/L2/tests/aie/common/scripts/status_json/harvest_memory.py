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
import csv
import glob
import json
import os
import re

# Size in bytes of one memory bank as labelled in the mapping report (MG(x,y):b),
# keyed by aie_driver_config.hw_gen in compiler_report.json.
BANK_SIZE_BY_HW_GEN = {1: 8192, 2: 16384, 5: 16384}

# Per-core heap fields, read from reports/report_heap.txt: field name -> regex for the value.
HEAP_FIELDS = {
    "HEAP_USED":     r"Heap Size Used[^=]*=\s*(\d+)",
    # "HEAP_ALLOTTED": r"Heap Size Allotted\s*=\s*(\d+)",
}


def _memory_bank_rows(reports_dir):
    """
    Parse the Memory Bank Report of the mapping analysis report.
    Returns a set of unique (bank_label, buffer_name, addr, size) tuples, where bank_label is e.g.
    'MG(23,2):0'. Buffers are listed once per accessing block, so identical rows are collapsed. The
    address is part of the key because buffer names are not unique (e.g. one leftQColBuffer per kernel).
    """
    path = glob.glob(os.path.join(reports_dir, "*mapping_analysis_report.txt"))[0]
    with open(path) as f:
        text = f.read()
    rows = set()
    for line in text.split("Memory Bank Report:", 1)[1].splitlines():
        fields = line.split()
        # Data rows: MG(x,y):b  name  0xADDR  size  ...  (header row has no numeric size)
        if len(fields) >= 4 and fields[0].startswith("MG(") and fields[3].isdigit():
            rows.add((fields[0], fields[1], int(fields[2], 16), int(fields[3])))
    return rows


def get_num_banks(reports_dir, compiler_report):
    """
    Number of memory banks used: for each bank label in the Memory Bank Report, the buffer sizes
    are summed and rounded up to whole banks (a buffer larger than one bank is only listed under
    its starting bank, so it also occupies the following ones).
    """
    bank_size = BANK_SIZE_BY_HW_GEN[compiler_report["aie_driver_config"]["hw_gen"]]
    bytes_per_label = {}
    for label, _, _, size in _memory_bank_rows(reports_dir):
        bytes_per_label[label] = bytes_per_label.get(label, 0) + size
    return sum(-(-total // bank_size) for total in bytes_per_label.values())


def _parse_core_report(path, patterns):
    """
    Parse a per-core report (report_heap.txt) made of blocks starting with 'Core <col>_<row>'.
    patterns maps field name -> regex with one capture group. Returns {core: {field: int}}.
    """
    with open(path) as f:
        text = f.read()
    parts = re.split(r"^Core (\d+_\d+)[ \t]*$", text, flags=re.MULTILINE)
    # re.split with one group gives [preamble, core1, body1, core2, body2, ...]
    return {
        core: {field: int(re.search(regex, body).group(1)) for field, regex in patterns.items()}
        for core, body in zip(parts[1::2], parts[2::2])
    }


def _core_functions(compiler_report):
    """Map 'col_row' -> list of top-level kernel function names placed on that core."""
    functions = {}
    for inst, mapping in compiler_report["mapping"]["blockInstanceMapping"].items():
        if "coreInfo" not in mapping:  # PLIO blocks are placed on the shim, not a core
            continue
        core = mapping["coreInfo"]
        block_type = compiler_report["blockInstances"][inst]["blockType"]
        name = compiler_report["blockTypes"][block_type]["function"]
        functions.setdefault(f"{core['column']}_{core['row']}", []).append(name)
    return functions


def _parse_build_outputs(work_dir, core):
    """
    Program memory and stack of a core from its linker map and call tree. These are rewritten whenever
    the core's ELF is rebuilt, unlike the reports/ files, which only aiecompiler regenerates.
    """
    release_dir = os.path.join(work_dir, "aie", core, "Release")
    with open(os.path.join(release_dir, f"{core}.map")) as f:
        symbols = {name: int(value, 16) for name, value in
                   re.findall(r"^\s+(\w+) = 0x([0-9a-f]+)\s*$", f.read(), flags=re.MULTILINE)}
    with open(os.path.join(release_dir, f"{core}.calltree")) as f:
        stack_used = int(re.search(r"Maximum stack size\s*:\s*(\d+)", f.read()).group(1))
    return {
        "PROGRAM_MEMORY_USED": symbols["_pc_end"],  # end of program memory, including alignment gaps
        "STACK_USED":          stack_used,          # deepest call chain
        # "STACK_ALLOTTED":      symbols["_sp_end_DM_stack"] - symbols["_sp_start_DM_stack"],
    }


def get_cores(work_dir, compiler_report):
    """Per-core program memory, stack, heap and top-level function(s), keyed by 'col_row'."""
    heap = _parse_core_report(os.path.join(work_dir, "reports", "report_heap.txt"), HEAP_FIELDS)
    functions = _core_functions(compiler_report)
    cores = {}
    for core in sorted(functions, key=lambda c: tuple(int(i) for i in c.split("_"))):
        cores[core] = {**_parse_build_outputs(work_dir, core), **heap[core], "FUNCTIONS": functions[core]}
    return cores


def harvest_memory(scripts_dir, work_dir):
    """
    Collect memory metrics for an AIE test directly from the Work directory reports and the per-core
    linker maps and call trees.

    Args:
        scripts_dir (str): Unused; kept so existing callers (update_status.py harvest) keep working.
        work_dir (str): AIE Work directory.

    Returns:
        dict: Memory metrics keyed by field name.
    """
    reports_dir = os.path.join(work_dir, "reports")

    with open(os.path.join(reports_dir, "complexity.csv")) as f:
        complexity = dict(csv.reader(f))

    with open(os.path.join(reports_dir, "compiler_report.json")) as f:
        compiler_report = json.load(f)

    return {
        "NUM_BANKS":   get_num_banks(reports_dir, compiler_report),
        "NUM_AIE":     complexity["total_num_cores"],
        "NUM_TILES":   complexity["total_num_tiles"],
        "NUM_KERNELS": complexity["total_num_kernels"],
        "DATA_MEMORY": complexity["total_memory_size"],
        "CORES":       get_cores(work_dir, compiler_report),
    }
