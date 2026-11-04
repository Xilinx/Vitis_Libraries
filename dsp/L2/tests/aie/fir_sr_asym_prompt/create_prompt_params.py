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
#!/usr/bin/env python3

"""Create prompt-only multi_params files from a DSPLib-style suite file."""

import argparse
import json
from pathlib import Path
import shlex


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("suite")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    suite_path = Path(args.suite).resolve()
    project_dir = suite_path.parent.parent
    tests = {}
    for line_number, raw_line in enumerate(suite_path.read_text().splitlines(), 1):
        line = raw_line.partition("#")[0].strip()
        if not line:
            continue
        tokens = shlex.split(line)
        if tokens[:2] != ["make", "all"]:
            raise ValueError("{}:{} must start with 'make all'".format(suite_path, line_number))
        fields = dict(token.split("=", 1) for token in tokens[2:])
        prompt_file = fields["PROMPT_FILE"]
        prompt_path = project_dir / prompt_file
        if not prompt_path.is_file():
            raise FileNotFoundError(prompt_path)
        device = fields.get("DEVICE", "aie1")
        target = fields.get("TARGET", "x86sim")
        model = fields.get("CODEX_MODEL", "")
        name = "test_{}_{}_{}_{}_{}".format(
            line_number, Path(prompt_file).stem, device, target, suite_path.stem
        )
        tests[name] = {"PROMPT_FILE": prompt_file, "CODEX_MODEL": model}

    if not tests:
        raise ValueError("No prompt tests found in {}".format(suite_path))
    Path(args.output).write_text(json.dumps(tests, indent=4) + "\n")
    print(args.output)


if __name__ == "__main__":
    main()
