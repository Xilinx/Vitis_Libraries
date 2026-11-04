#
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

import argparse
import ast
import json

import pandas as pd

# harvest_qor.py/harvest_memory.py use these casings for the two metrics whose
# names don't match the qor_helper CSV's metric-name strings verbatim.
NAME_MAP = {"num aie": "NUM_AIE", "num banks": "NUM_BANKS"}


def main():
    ap = argparse.ArgumentParser(
        description="Validate that simulated QoR results fall within the range "
        "config_qor_helper.py predicted for the recommended config."
    )
    ap.add_argument("--constraints_file", required=True)
    ap.add_argument("--qor_csv", required=True, help="config_qor_helper.py --out_csv_file output")
    ap.add_argument("--status_json", required=True, help="post-sim status JSON with a 'Results' dict")
    args = ap.parse_args()

    with open(args.constraints_file) as f:
        qor_constraints = json.load(f)["qor_constraints"]

    row0 = pd.read_csv(args.qor_csv).iloc[0]

    with open(args.status_json) as f:
        results = json.load(f)["Results"]

    failures = []
    for metric in qor_constraints:
        lo, hi = ast.literal_eval(row0[metric])
        key = NAME_MAP.get(metric, metric)
        try:
            measured = float(results[key])
        except (KeyError, ValueError, TypeError):
            failures.append(f"{metric}: no measured value found (key={key})")
            continue
        if not (float(lo) <= measured <= float(hi)):
            failures.append(f"{metric}: measured {measured} outside predicted [{lo}, {hi}]")

    if failures:
        print("QOR_HELPER_PREDICTION: FAIL")
        for failure in failures:
            print(f"  - {failure}")
        raise SystemExit(1)

    print("QOR_HELPER_PREDICTION: PASS")


if __name__ == "__main__":
    main()
