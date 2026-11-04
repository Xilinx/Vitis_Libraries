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
import glob
import os
import subprocess


def run_script(script_path, *args):
    """Run a shell subscript and return its stdout as a stripped string."""
    result = subprocess.run(
        [script_path, *args],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        print(f"Warning: {os.path.basename(script_path)} exited with code {result.returncode}: {result.stderr.strip()}")
    return result.stdout.strip()


def get_program_memory(work_dir):
    """
    Replicate the inline perl-regex logic from harvest_memory.sh for PROGRAM_MEMORY.
    Returns a list of integers, one per .map file found.
    """
    map_files = glob.glob(os.path.join(work_dir, "aie", "*_*", "Release", "*_*.map"))
    if not map_files:
        return []
    grep_sections = subprocess.run(
        ["grep", "-A", "10", "Section summary for memory 'PM':"] + map_files,
        capture_output=True, text=True,
    )
    grep_values = subprocess.run(
        ["grep", "-Po", r"\b\d+\b(?=\s+Total)"],
        input=grep_sections.stdout, capture_output=True, text=True,
    )
    return [int(v) for v in grep_values.stdout.splitlines() if v.strip()]


def harvest_memory(scripts_dir, work_dir):
    """
    Run all memory harvest subscripts and return results as a dict.

    Args:
        scripts_dir (str): Directory containing the get_num_*.sh subscripts.
        work_dir (str): AIE Work directory.

    Returns:
        dict: Memory metrics keyed by field name.
    """
    return {
        "NUM_BANKS":      run_script(os.path.join(scripts_dir, "get_num_banks.sh"),   work_dir, "dummy"),
        "NUM_AIE":        run_script(os.path.join(scripts_dir, "get_num_me.sh"),      work_dir, "1"),
        "NUM_CORES":      run_script(os.path.join(scripts_dir, "get_num_cores.sh"),   work_dir),
        "NUM_TILES":      run_script(os.path.join(scripts_dir, "get_num_tiles.sh"),   work_dir),
        "DATA_MEMORY":    run_script(os.path.join(scripts_dir, "get_data_memory.sh"), work_dir),
        "PROGRAM_MEMORY": get_program_memory(work_dir),
    }
