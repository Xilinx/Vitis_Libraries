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

"""Materialize one prompt-driven FIR test case for the normal DSPLib flow.

Drives L2/meta/scripts/ip_config_wizard/ip_config_wizard.py's ParamWizard one
parameter at a time -- the same engine and per-parameter explanation/legality
data the dsp-ip-config skill uses -- and asks Codex to answer exactly one
question per turn. This is deliberately not "hand Codex the raw metadata
files and let it guess a whole config in one shot": that path never
validates anything until metadata_checker.py runs much later, and duplicates
the exact validator/updater logic the wizard exists to wrap. Here every
answer is fed through ParamWizard.set_value()/auto_fill_vector() immediately,
using the real metadata_api.IP validators, so a bad answer fails fast and is
attributed to the parameter that caused it.

The full wrapper text sent to Codex every turn lives in a single file,
prompts/turn_instruction_template.txt -- not split between there and this
script -- so a user can read the prompts/ directory and see exactly what
Codex is asked, for guidance and manual repro, without reading this file.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

IP_NAME = "fir_sr_asym"

GRAPH_TO_TEST = {
    "AIE_VARIANT": "AIE_VARIANT", "TT_DATA": "DATA_TYPE", "TT_COEFF": "COEFF_TYPE",
    "TP_API": "PORT_API", "TP_USE_COEFF_RELOAD": "USE_COEFF_RELOAD",
    "TP_FIR_LEN": "FIR_LEN", "TP_SSR": "UUT_SSR", "TP_CASC_LEN": "CASC_LEN",
    "TP_INPUT_WINDOW_VSIZE": "INPUT_WINDOW_VSIZE", "TP_DUAL_IP": "DUAL_IP",
    "TP_NUM_OUTPUTS": "NUM_OUTPUTS", "TP_SHIFT": "SHIFT", "TP_RND": "ROUND_MODE",
    "TP_SAT": "SAT_MODE",
}

# These values control the harness and are not FIR graph configuration inputs.
HARNESS_DEFAULTS = {
    "NITER": "16", "STIM_TYPE": "0", "INTERPOLATE_FACTOR": "1",
    "DECIMATE_FACTOR": "1", "USE_CUSTOM_CONSTRAINT": "0",
    "UUT_PARA_INTERP_POLY": "1", "UUT_PARA_DECI_POLY": "1", "DATA_SEED": "0",
    "DATA_STIM_TYPE": "0", "COEFF_STIM_TYPE": "4", "SINGLE_BUF": "0",
}


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--params-file", required=True)
    parser.add_argument("--instance", required=True)
    parser.add_argument("--lib-dir", required=True)
    parser.add_argument("--codex", default="codex")
    return parser.parse_args()


def load_wizard_module(lib_dir):
    """Import ip_config_wizard.py from its new subdirectory without copying
    it into this test's isolated result directory -- same convention as the
    existing metadata_checker.py pre_build step, which also always refers to
    the shared LIB_DIR copy rather than a local one."""
    wizard_dir = str(Path(lib_dir) / "L2" / "meta" / "scripts" / "ip_config_wizard")
    if wizard_dir not in sys.path:
        sys.path.insert(0, wizard_dir)
    import ip_config_wizard  # noqa: E402

    return ip_config_wizard


def ask_codex(codex_bin, model, lib_dir, schema_path, response_path, instruction):
    command = [
        codex_bin, "--ask-for-approval", "never", "exec", "--ephemeral",
        "--ignore-rules", "--sandbox", "read-only", "--cd", str(lib_dir),
        "--output-schema", str(schema_path), "--output-last-message", str(response_path), "-",
    ]
    if model:
        command[1:1] = ["--model", model]
    subprocess.run(
        command,
        input=instruction,
        universal_newlines=True,
        check=True,
        env=os.environ.copy(),
    )
    return json.loads(response_path.read_text(encoding="utf-8"))


COMPLEX_COEFF_TYPES = {"cint16", "cint32", "cfloat"}


def _format_taps_literal(coeff, is_complex):
    """Mirror L2/meta/fir_sr_asym.py's generate_graph()/fn_get_taps_vector()
    literal-array formatting exactly: {real, imag} pairs for a complex
    COEFF_TYPE, a flat list otherwise. `coeff` is the wizard's flat,
    possibly-interleaved array (see _VECTOR_GUIDANCE)."""
    if is_complex:
        pairs = ["{%s, %s}" % (coeff[2 * i], coeff[2 * i + 1]) for i in range(len(coeff) // 2)]
        return "{%s}" % ", ".join(pairs)
    return "{%s}" % ", ".join(str(v) for v in coeff)


def _write_ref_coeff_header(path, coeff_type, coeff, is_complex):
    """Reference-model taps as a literal C++ initializer, not a runtime-read
    file. STIM_GEN_FROM_FILE's fopen/fscanf/fclose, called from test_graph's
    *global static initializer* (test.cpp declares `test_graph filter;` at
    file scope), segfaults deep inside a closed-source Xilinx-internal I/O
    library (libxv_isl_iostreams.so, transitively linked via
    libaiecompiler_client.so) the first time real stdio touches an actual
    file that early in process life -- confirmed with gdb against the crash,
    not a dsplib bug, and not fixable from this codebase. This mirrors
    exactly how L2/meta/fir_sr_asym.py's generate_graph() already embeds the
    UUT's own coefficients (fn_get_taps_vector()) -- pure aggregate
    initialization, zero file I/O, proven safe (it's what the UUT side
    already does successfully)."""
    literal = _format_taps_literal(coeff, is_complex)
    path.write_text(
        "// Generated by run_codex_config.py -- see its module docstring for why\n"
        "// this is a literal array rather than a file STIM_GEN_FROM_FILE reads.\n"
        "static const std::vector<%s> kRefTaps = %s;\n" % (coeff_type, literal),
        encoding="utf-8",
    )


TURN_INSTRUCTION_TEMPLATE_PATH = Path(__file__).resolve().parent / "prompts" / "turn_instruction_template.txt"

_VECTOR_GUIDANCE = """This is a coefficient array of exactly {length} scalar values (complex
coefficients use interleaved real/imaginary scalars, so a complex coeff
array of N taps needs 2*N values here). Supply exactly {length} values that
satisfy the user's request."""


def _render_instruction(user_prompt, param, param_guidance):
    """The full wrapper prompt sent to Codex every turn lives in a single
    file, prompts/turn_instruction_template.txt, not split across this
    script -- a user reading the prompts/ directory should see exactly what
    Codex is asked, not have to read Python to reconstruct it. Only the
    per-turn parameter name/guidance and the user's own request text (itself
    a separate, complete file under prompts/) are substituted in."""
    template = TURN_INSTRUCTION_TEMPLATE_PATH.read_text(encoding="utf-8")
    return (
        template.replace("{user_request}", user_prompt)
        .replace("{param_guidance}", param_guidance)
        .replace("{param}", param)
    )


def main():
    args = parse_args()
    project_dir = Path.cwd()
    params_path = (project_dir / args.params_file).resolve()
    params = json.loads(params_path.read_text(encoding="utf-8"))
    test_case = params[args.instance]
    prompt_path = (project_dir / test_case["PROMPT_FILE"]).resolve()
    model = test_case.get("CODEX_MODEL", "")
    lib_dir = Path(args.lib_dir).resolve()
    user_prompt = prompt_path.read_text(encoding="utf-8")

    icw = load_wizard_module(lib_dir)
    wizard = icw.ParamWizard(IP_NAME)

    schema_path = project_dir / "codex_output_schema.json"
    responses_dir = project_dir / "codex_responses"
    responses_dir.mkdir(exist_ok=True)

    turns = []
    step = 0
    while not wizard.is_complete():
        step += 1
        param = wizard.current_param()
        response_path = responses_dir / f"{step:02d}_{param}.json"

        if wizard.is_vector_param(param):
            param_obj = wizard.ip.param_obj_dict[param]
            param_obj.param_update(wizard.ip.args, wizard.ip.module)
            length = (param_obj.update_result or {}).get("len", 0)
            param_guidance = _VECTOR_GUIDANCE.format(length=length)
            instruction = _render_instruction(user_prompt, param, param_guidance)
            answer = ask_codex(args.codex, model, lib_dir, schema_path, response_path, instruction)
            value = answer["values"]
        else:
            param_guidance = wizard.explain(param)
            instruction = _render_instruction(user_prompt, param, param_guidance)
            answer = ask_codex(args.codex, model, lib_dir, schema_path, response_path, instruction)
            value = answer["value"]

        ok, err = wizard.set_value(param, value)
        turns.append({"param": param, "answer": value, "reason": answer.get("reason", ""), "ok": ok, "err": err})
        if not ok:
            print(f"ERROR: Codex's answer for {param} ({value!r}) was rejected: {err}", file=sys.stderr)
            raise SystemExit(1)

    graph_params = dict(wizard.ip.args)
    (project_dir / "config.json").write_text(
        json.dumps({"parameters": graph_params}, indent=4) + "\n", encoding="utf-8")

    _write_ref_coeff_header(
        project_dir / "ref_coeff.h",
        graph_params["TT_COEFF"],
        graph_params["coeff"],
        graph_params["TT_COEFF"] in COMPLEX_COEFF_TYPES,
    )

    for graph_name, test_name in GRAPH_TO_TEST.items():
        test_case[test_name] = str(graph_params[graph_name])
    for name, value in HARNESS_DEFAULTS.items():
        test_case.setdefault(name, value)
    params_path.write_text(json.dumps(params, indent=4) + "\n", encoding="utf-8")

    report = {
        "prompt": str(prompt_path),
        "instance": args.instance,
        "parameters": graph_params,
        "turns": turns,
    }
    (project_dir / "codex_configuration_report.json").write_text(
        json.dumps(report, indent=4) + "\n", encoding="utf-8")


if __name__ == "__main__":
    try:
        main()
    except (KeyError, ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"ERROR: prompt-driven configuration failed: {error}", file=sys.stderr)
        raise SystemExit(1)
