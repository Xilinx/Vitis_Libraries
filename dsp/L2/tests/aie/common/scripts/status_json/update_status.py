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
import argparse
import json
import os
from pathlib import Path

from harvest_memory import harvest_memory
from harvest_qor import harvest_qor


def parse_json(filepath):
    with open(filepath, 'r') as f:
        return json.load(f)


def init_status(status_path, config_params):
    """
    Create a new status JSON file with a Configuration block populated from
    config_params and a Results block with all fields set to "default".

    Args:
        status_path (str): Path to the status JSON file to create.
        config_params (dict): Key-value pairs for the Configuration block.
    """
    if os.path.exists(status_path):
        print(f"Warning: {status_path} already exists. Exiting.")
        return
    os.makedirs(os.path.dirname(os.path.abspath(status_path)), exist_ok=True)
    status = {
        "Configuration": config_params,
        "Results": {
            "COMPILE": "default",
            "SIM": "default",
            "FUNC": "default",
        },
    }
    with open(status_path, "w") as f:
        json.dump(status, f, indent=4)


def _parse_kvpairs(pairs):
    """Parse a flat list [KEY1, VAL1, KEY2, VAL2, ...] into a dict."""
    if len(pairs) % 2 != 0:
        raise ValueError(f"Expected an even number of key-value arguments, got {len(pairs)}")
    return {pairs[i]: pairs[i + 1] for i in range(0, len(pairs), 2)}


def _cmd_init(args):
    config_params = _parse_kvpairs(args.config_kvpairs)
    init_status(args.status_path, config_params)


def set_result(status_path, field, value):
    """
    Update a single field in the Results block of an existing status JSON file.

    Args:
        status_path (str): Path to the status JSON file.
        field (str): Field name within Results (e.g. 'COMPILE', 'SIM', 'FUNC').
        value: Value to set.
    """
    status = parse_json(status_path)
    status["Results"][field] = value
    with open(status_path, "w") as f:
        json.dump(status, f, indent=4)


def _cmd_set(args):
    set_result(args.status_path, args.field, args.value)


def _cmd_harvest(args):
    fields = harvest_memory(args.scripts_dir, args.work_dir)
    for field, value in fields.items():
        set_result(args.status_path, field, value)

def _cmd_qor(args):
    aiesimulator_output_dir = args.work_dir + "/../aiesimulator_output/"

    verbose_path = None
    if args.verbose:
        status_path = Path(args.status_path)
        verbose_path = status_path.with_name("verbose_qor_metrics.json")

    fields = harvest_qor(aiesimulator_output_dir,
                        args.niter,
                        args.frames_per_iteration,
                        args.input_pattern,
                        args.output_pattern,
                        args.plio_width_bits,
                        args.out_datatype,
                        args.steady_state_window_size,
                        args.cv_threshold,
                        args.slope_threshold,
                        verbose_path)

    for field, value in fields.items():
        set_result(args.status_path, field, value)

def functional_check(status_path, diff_file_path):
    """Temporary bandaid: read diff.txt and set FUNC based on presence of 'identical'."""
    try:
        with open(diff_file_path) as f:
            content = f.read()
        func = "1" if "identical" in content.lower() else "0"
    except FileNotFoundError:
        func = "0"
    set_result(status_path, "FUNC", func)


def _cmd_functional_check(args):
    functional_check(args.status_path, args.diff_file)
    

def build_parser():
    parser = argparse.ArgumentParser(
        prog="update_status",
        description="Create and update the status JSON file for a test.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    init_parser = subparsers.add_parser("init", help="Initialise a new status JSON file.")
    init_parser.add_argument("status_path", help="Path to the status JSON file to create.")
    init_parser.add_argument("config_kvpairs", nargs="*", help="Configuration parameters as KEY VAL pairs.")

    set_parser = subparsers.add_parser("set", help="Set a field in the Results block.")
    set_parser.add_argument("status_path", help="Path to the status JSON file.")
    set_parser.add_argument("field", help="Results field to update (e.g. COMPILE, SIM, FUNC).")
    set_parser.add_argument("value", help="Value to set.")

    harvest_parser = subparsers.add_parser("harvest", help="Harvest AIE memory metrics into the Results block.")
    harvest_parser.add_argument("status_path", help="Path to the status JSON file.")
    harvest_parser.add_argument("scripts_dir", help="Directory containing the get_num_*.sh subscripts.")
    harvest_parser.add_argument("work_dir", default="Work", help="AIE Work directory (default: Work).")

    qor_parser = subparsers.add_parser("qor", help="Harvest AIE QOR metrics into the Results block.")
    qor_parser.add_argument("status_path", help="Path to the status JSON file.")
    qor_parser.add_argument("work_dir", help="AIE Work directory (default: Work).")
    qor_parser.add_argument("niter", type=int, help="Number of iterations in the AIEsim run.")
    qor_parser.add_argument("frames_per_iteration", type=int, help="Number of frames processed per iteration.")
    qor_parser.add_argument("input_pattern", help="Input file pattern to match for timestamp extraction.")
    qor_parser.add_argument("output_pattern", help="Output file pattern to match for timestamp extraction.")
    qor_parser.add_argument("plio_width_bits", type=int, help="Width of the PLIO in bits.")
    qor_parser.add_argument("out_datatype", type=str, help="Output datatype used for throughput calculations.")
    qor_parser.add_argument("steady_state_window_size", type=int, nargs="?", default=4, help="Size of the steady state window for latency calculation.")
    qor_parser.add_argument("cv_threshold", type=float, nargs="?", default=0.02, help="Coefficient of variation threshold for steady state detection.")
    qor_parser.add_argument("slope_threshold", type=float, nargs="?", default=0.08, help="Slope threshold for steady state detection. Guards against slow monotone drift that CV alone cannot detect.")
    qor_parser.add_argument("--verbose", action="store_true", default=False, help="Enable verbose output.")

    fc_parser = subparsers.add_parser("functional_check", help="Set FUNC based on diff.txt content (temporary).")
    fc_parser.add_argument("status_path", help="Path to the status JSON file.")
    fc_parser.add_argument("diff_file", help="Path to the diff output file (e.g. ./logs/diff.txt).")

    return parser


if __name__ == "__main__":
    parser = build_parser()
    args = parser.parse_args()
    if args.command == "init":
        _cmd_init(args)
    elif args.command == "set":
        _cmd_set(args)
    elif args.command == "harvest":
        _cmd_harvest(args)
    elif args.command == "qor":
        _cmd_qor(args)
    elif args.command == "functional_check":
        _cmd_functional_check(args)