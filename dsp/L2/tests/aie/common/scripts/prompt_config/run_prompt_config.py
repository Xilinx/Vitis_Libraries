#!/usr/bin/env python3
#
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
#
"""Configure one prompt-driven test case of any AIE IP from a natural-language prompt.

Called from an IP's Makefile pre_build step when the test case defines PROMPT_FILE. Drives
L2/meta/scripts/ip_config_wizard/ip_config_wizard.py's ParamWizard one graph parameter at a
time and asks an LLM CLI (Codex) to answer exactly one parameter per turn. Every answer is
validated immediately by the IP's metadata validators, so a bad answer fails fast and is
attributed to the parameter that caused it.

The resulting graph parameters are translated back to testbench names and written over the
test case's entry in the (result-directory copy of the) multi_params file -- the same mechanism
apply_qor_helper_recommendation.py uses -- so every later step of the IP's normal flow
(create_config, metadata_checker.py, input generation, reference model, uut_config.h, diff)
runs on the prompt-derived configuration unchanged.

Prompt-driven tests need a user-specific LLM API token, so they must never be part of the
Jenkins regressions: create_params.py only accepts PROMPT_FILE in test_suites/prompt*.txt.

Parameter handling:
  - AIE_VARIANT is pinned from the test case (the test name has already selected the platform).
  - Vector parameters (e.g. FIR "coeff") are auto-filled; the IP testbench generates its own
    stimulus/coefficients, so an LLM answer would never be consumed.
  - Every other graph parameter is answered by the LLM, one turn each.

Outputs (in --out_dir):
  prompt_config_report.json      prompt, final graph/testbench parameters, per-turn answers
  codex_responses/NN_<PARAM>.json raw structured LLM response per turn
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

SCRIPT_DIR = Path(__file__).resolve().parent
SCHEMA_PATH = SCRIPT_DIR / "codex_output_schema.json"
TEMPLATE_NAME = "turn_instruction_template.txt"
PINNED_PARAMS = ("AIE_VARIANT",)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--ip", required=True, help="IP name as in L2/meta/<ip>.json")
    parser.add_argument("--params_file", required=True, help="multi_params file holding the test case")
    parser.add_argument("--instance", required=True, help="test case name in the multi_params file")
    parser.add_argument("--prompt_file", required=True, help="prompt file name, relative to --prompt_dir")
    parser.add_argument("--prompt_dir", required=True, help="the IP's prompts/ directory")
    parser.add_argument("--lib_dir", required=True, help="xf_dsp repository root")
    parser.add_argument("--out_dir", required=True, help="directory for the report and raw responses")
    parser.add_argument(
        "--codex", default=None, help="LLM CLI binary (default: $CODEX_BIN, else 'codex' on PATH)"
    )
    return parser.parse_args()


def resolve_codex(explicit):
    """Shell aliases are not visible to make recipes, so the binary must come from the
    command line, the CODEX_BIN environment variable, or PATH."""
    for candidate in (explicit, os.environ.get("CODEX_BIN"), "codex"):
        if candidate and shutil.which(candidate):
            return shutil.which(candidate)
    raise RuntimeError(
        "no Codex CLI found. Prompt-driven tests need a user-specific LLM token and run "
        "locally only: set CODEX_BIN to the codex binary (a shell alias is not visible to make)."
    )


def load_wizard_module(lib_dir):
    wizard_dir = str(lib_dir / "L2" / "meta" / "scripts" / "ip_config_wizard")
    if wizard_dir not in sys.path:
        sys.path.insert(0, wizard_dir)
    import ip_config_wizard

    return ip_config_wizard


def lookup_api_name(lib_dir, ip):
    api_list = json.loads((lib_dir / "L2" / "meta" / "api.json").read_text(encoding="utf-8"))["api_list"]
    for api in api_list:
        if "%s_graph.hpp" % ip in api["spec"].get("header_file_name", []):
            return api["api_name"]
    return ip


def graph_to_test_keys(lib_dir, test_case):
    """Invert config_translation.json (testbench name -> graph name), keeping only testbench
    names this test case actually uses. This resolves the table's many-to-one entries, e.g.
    TP_API <- PORT_API / API_IO / P_API, in the same way apply_qor_helper_recommendation.py does."""
    translation_path = lib_dir / "L2" / "meta" / "scripts" / "qor_helper" / "data" / "config_translation.json"
    forward = json.loads(translation_path.read_text(encoding="utf-8"))
    reverse = {}
    for test_key, graph_key in forward.items():
        if test_key in test_case:
            reverse.setdefault(graph_key, []).append(test_key)
    return reverse


def ask_codex(codex_bin, model, lib_dir, response_path, instruction):
    command = [
        codex_bin, "--ask-for-approval", "never", "exec", "--ephemeral", "--ignore-rules",
        "--sandbox", "read-only", "--cd", str(lib_dir),
        "--output-schema", str(SCHEMA_PATH), "--output-last-message", str(response_path), "-",
    ]
    if model:
        command[1:1] = ["--model", model]
    subprocess.run(command, input=instruction, universal_newlines=True, check=True, env=os.environ.copy())
    return json.loads(response_path.read_text(encoding="utf-8"))


def render_instruction(template, api_name, user_prompt, decided, param, guidance):
    decided_text = "\n".join("  %s = %s" % (k, v) for k, v in decided.items()) or "  (none yet)"
    return (
        template.replace("{api_name}", api_name)
        .replace("{user_request}", user_prompt)
        .replace("{decided_params}", decided_text)
        .replace("{param_guidance}", guidance)
        .replace("{param}", param)
    )


def main():
    args = parse_args()
    lib_dir = Path(args.lib_dir).resolve()
    prompt_dir = Path(args.prompt_dir).resolve()
    out_dir = Path(args.out_dir).resolve()
    params_path = Path(args.params_file).resolve()

    prompt_path = prompt_dir / args.prompt_file
    if not prompt_path.is_file():
        raise FileNotFoundError("PROMPT_FILE %s not found in %s" % (args.prompt_file, prompt_dir))
    # An IP may provide its own turn template; otherwise use the shared, IP-agnostic one.
    template_path = prompt_dir / TEMPLATE_NAME
    if not template_path.is_file():
        template_path = SCRIPT_DIR / TEMPLATE_NAME

    params = json.loads(params_path.read_text(encoding="utf-8"))
    test_case = params[args.instance]
    model = test_case.get("CODEX_MODEL", "") or os.environ.get("CODEX_MODEL", "")
    user_prompt = prompt_path.read_text(encoding="utf-8")
    template = template_path.read_text(encoding="utf-8")
    api_name = lookup_api_name(lib_dir, args.ip)
    reverse = graph_to_test_keys(lib_dir, test_case)
    codex_bin = resolve_codex(args.codex)

    icw = load_wizard_module(lib_dir)
    wizard = icw.ParamWizard(args.ip)

    responses_dir = out_dir / "codex_responses"
    responses_dir.mkdir(parents=True, exist_ok=True)

    turns = []
    decided = {}
    step = 0
    while not wizard.is_complete():
        step += 1
        param = wizard.current_param()
        if wizard.is_vector_param(param):
            ok, err, length = wizard.auto_fill_vector(param)
            turns.append({"param": param, "source": "auto_fill_vector", "length": length, "ok": ok, "err": err})
        elif param in PINNED_PARAMS and reverse.get(param):
            value = test_case[reverse[param][0]]
            ok, err = wizard.set_value(param, int(value) if str(value).isdigit() else value)
            turns.append({"param": param, "source": "pinned", "answer": value, "ok": ok, "err": err})
        else:
            guidance = wizard.explain(param)
            instruction = render_instruction(template, api_name, user_prompt, decided, param, guidance)
            answer = ask_codex(codex_bin, model, lib_dir, responses_dir / ("%02d_%s.json" % (step, param)), instruction)
            value = answer["value"]
            ok, err = wizard.set_value(param, value)
            turns.append({"param": param, "source": "llm", "answer": value, "reason": answer.get("reason", ""),
                          "ok": ok, "err": err})
        if not ok:
            write_report(out_dir, prompt_path, args.instance, dict(wizard.ip.args), {}, turns)
            raise RuntimeError("value for %s was rejected: %s" % (param, err))
        if not wizard.is_vector_param(param):
            decided[param] = wizard.ip.args[param]

    graph_params = dict(wizard.ip.args)
    unmapped = [p for p in graph_params if not wizard.is_vector_param(p) and not reverse.get(p)]
    if unmapped:
        write_report(out_dir, prompt_path, args.instance, graph_params, {}, turns)
        raise RuntimeError(
            "graph parameters %s have no testbench key in test case %s (check "
            "qor_helper/data/config_translation.json and test_suites/default_params.txt)"
            % (unmapped, args.instance)
        )

    testbench_params = {}
    for graph_key, test_keys in reverse.items():
        if graph_key in graph_params and not wizard.is_vector_param(graph_key):
            for test_key in test_keys:
                testbench_params[test_key] = str(graph_params[graph_key])
    test_case.update(testbench_params)
    params_path.write_text(json.dumps(params, indent=4) + "\n", encoding="utf-8")
    write_report(out_dir, prompt_path, args.instance, graph_params, testbench_params, turns)
    print("Prompt %s configured %s: %s" % (args.prompt_file, args.instance, json.dumps(testbench_params)))


def write_report(out_dir, prompt_path, instance, graph_params, testbench_params, turns):
    report = {
        "prompt": str(prompt_path),
        "instance": instance,
        "graph_parameters": graph_params,
        "testbench_parameters": testbench_params,
        "turns": turns,
    }
    (out_dir / "prompt_config_report.json").write_text(json.dumps(report, indent=4) + "\n", encoding="utf-8")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, ValueError, OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print("ERROR: prompt-driven configuration failed: %s" % error, file=sys.stderr)
        raise SystemExit(1)
