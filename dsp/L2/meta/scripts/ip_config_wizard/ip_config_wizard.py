#
# Copyright (C) 2024-2026, Advanced Micro Devices, Inc.
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
"""
Interactive configuration wizard for xf_dsp IPs.

Relationship to config_helper.py: this is the interactive/agent-facing
successor for humans and AI agents. config_helper.py is left in place
unchanged -- metadata_checker.py's own smoke test subprocess depends on it
(it exercises config_helper.py's wizard in non-interactive
--test_config_helper mode as part of the official validation pipeline).
This tool reuses config_helper.py's extract_aie_ip_names() (IP listing) and
valid_name() (instance-name validation) rather than re-deriving them.

This module is built entirely on metadata_api.IP -- it never re-implements
parameter validation/update logic. IP already builds a param_obj_dict wired
to each parameter's real validator/updater function (in JSON-declared,
dependency-safe order); this wizard drives that object one parameter at a
time instead of calling IP.update_all()/validate_all() on a fully-resolved
dict.

Two operating surfaces:
  1. A plain interactive CLI (input()-driven) for anyone without an AI agent.
  2. A stateless --step/--set/--finalize JSON API (a state file on disk) for
     an AI agent to drive turn by turn -- deliberately not a live piped
     subprocess to config_helper.py, since parsing free-text TTY prompts
     from a subprocess has no clean hook for chat-based Q&A.

Finalization (validate + generate) always shells out to the existing
metadata_checker.py unmodified, so the result has 100% parity with what a
human running the existing tools would get.
"""
import argparse
import csv
import json
import os
import sys

WIZARD_DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPTS_DIR = os.path.dirname(WIZARD_DIR)
if WIZARD_DIR not in sys.path:
    sys.path.insert(0, WIZARD_DIR)
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

from metadata_api import IP, extract_param_json  # noqa: E402
import config_helper  # noqa: E402
import param_doc_extractor as pde  # noqa: E402

L2_DIR = os.path.abspath(os.path.join(SCRIPTS_DIR, "..", ".."))
REPO_ROOT = os.path.abspath(os.path.join(L2_DIR, ".."))
META_DIR = os.path.join(L2_DIR, "meta")
UG_DIR = os.path.join(REPO_ROOT, "docs", "src", "user_guide", "L2")
QOR_MODELS_DIR = os.path.join(SCRIPTS_DIR, "qor_helper", "models")


def check_vitis_env():
    """finalize() shells to metadata_checker.py, which validates against
    $XILINX_VITIS/data/ipmetadata/vitis_library_api_spec_schema.json -- i.e.
    it hard-requires a sourced Vitis environment, exactly like
    config_qor_helper.py does. The interactive param walk itself does not
    need this (metadata_api.IP has no Vitis dependency). Callers use this as
    an informational warning at startup and a hard gate right before
    finalize() -- never let this surface for the first time as a subprocess
    traceback after a long interactive session."""
    xilinx_vitis = os.environ.get("XILINX_VITIS")
    if not xilinx_vitis:
        return False, (
            "$XILINX_VITIS is not set. You can keep building a configuration now, "
            "but finishing this session (validate + generate the graph) will need "
            "a Vitis environment sourced first -- see .github/skills/dsp/SKILL.md."
        )
    schema_path = os.path.join(
        xilinx_vitis, "data", "ipmetadata", "vitis_library_api_spec_schema.json"
    )
    if not os.path.isfile(schema_path):
        return False, (
            "$XILINX_VITIS is set to %s but the expected schema file was not "
            "found at %s. finalize() will fail until this is fixed."
            % (xilinx_vitis, schema_path)
        )
    return True, ""


def list_available_ips():
    """Reuse config_helper.py's own IP-listing helper instead of re-deriving
    it -- see the __main__ guard added to config_helper.py for why this is
    importable without running config_helper.py's script body."""
    api_json = os.path.join(META_DIR, "api.json")
    if not os.path.isfile(api_json):
        return []
    return config_helper.extract_aie_ip_names(api_json)


def qor_model_exists(ip_name):
    return os.path.isfile(os.path.join(QOR_MODELS_DIR, "%s_polynomial.pkl" % ip_name))


def format_explanation(doc_text, param_obj):
    """Combine the doxygen-derived semantic explanation with the concrete
    legal range/enum computed by this parameter's real updater function."""
    parts = []
    if doc_text:
        parts.append(doc_text)
    update_result = param_obj.update_result or {}
    if "enum" in update_result:
        parts.append("Legal values: %s" % update_result["enum"])
    elif "minimum" in update_result:
        maximum = update_result.get("maximum")
        if maximum is not None:
            parts.append(
                "Legal range: [%s, %s]" % (update_result["minimum"], maximum)
            )
        else:
            parts.append("Minimum: %s" % update_result["minimum"])
    if update_result.get("actual") is not None:
        parts.append(
            "Closest legal value to your last input: %s" % update_result["actual"]
        )
    return "\n".join(parts) if parts else "(no additional guidance available)"


class ParamWizard:
    """Drives metadata_api.IP one parameter at a time, in the JSON's
    declared (dependency-safe) order. Every not-yet-visited parameter is
    held as the empty string "" until the wizard reaches it.

    This is NOT the literal "auto" that metadata_api.IP itself uses for its
    auto-resolution feature (IP.update_all()/validate_all()/generate_graph()
    special-case that string) -- this wizard never calls those methods, only
    the per-parameter ip_parameter.param_update()/.param_validate() that IP
    happens to have already wired up. "" is required instead: several
    updater functions (e.g. update_TP_FIR_LEN, update_TP_SSR,
    update_TP_INPUT_WINDOW_VSIZE in fir_sr_asym.py) read their *own*
    not-yet-decided value back from args with a truthiness check --
    `if "TP_FIR_LEN" in args and args["TP_FIR_LEN"]:` -- to decide whether a
    value has already been proposed for them. "auto" is truthy and gets
    fed into arithmetic meant for a real int, crashing with a TypeError;
    "" is falsy and takes the intended "nothing decided yet" branch. This
    matches config_helper.py's own placeholder convention exactly (its
    param_vals dict is seeded with "" for every parameter) -- discovered by
    running this wizard against fir_sr_asym and hitting the same crash class
    config_helper.py's convention exists to avoid.
    """

    def __init__(self, ip_name, meta_dir=None):
        self.ip_name = ip_name
        meta_dir = meta_dir or META_DIR
        if meta_dir not in sys.path:
            sys.path.insert(0, meta_dir)

        meta_json = extract_param_json(ip_name, meta_dir)
        self.type_dict = {p["name"]: p.get("type") for p in meta_json}
        args = {p["name"]: "" for p in meta_json}

        self.ip = IP(ip_name, args, meta_dir)
        hpp_path = pde.graph_hpp_path_for(ip_name, L2_DIR)
        self.docs = pde.extract_tparam_docs(hpp_path) if hpp_path else {}
        self.order = self.ip.params
        self.cursor = 0

    def current_param(self):
        if self.is_complete():
            return None
        return self.order[self.cursor]

    def is_complete(self):
        return self.cursor >= len(self.order)

    def is_vector_param(self, param):
        return self.type_dict.get(param) == "vector"

    def _require_current(self, param):
        current = self.current_param()
        if param != current:
            raise ValueError(
                "%r is not the current parameter (expected %r) -- explain()/"
                "set_value() only ever target the cursor, never a param "
                "ahead of it (those are still the 'auto' placeholder)."
                % (param, current)
            )

    def explain(self, param):
        self._require_current(param)
        param_obj = self.ip.param_obj_dict[param]
        param_obj.param_update(self.ip.args, self.ip.module)
        return format_explanation(self.docs.get(param, ""), param_obj)

    def set_value(self, param, value):
        self._require_current(param)
        if value == "" or value == "auto":
            raise ValueError(
                '%r is a reserved internal placeholder for not-yet-decided '
                "parameters -- it is never a valid answer." % (value,)
            )
        self.ip.args[param] = value
        param_obj = self.ip.param_obj_dict[param]
        param_obj.param_validate(self.ip.args, self.ip.module, print_err=0)
        if param_obj.valid == "False":
            return False, param_obj.err_msg
        self.cursor += 1
        return True, ""

    def auto_fill_vector(self, param):
        """Vector-typed params (e.g. a FIR's "coeff" tap array) are actual
        data content, not a configuration choice with a legal range --
        config_helper.py's own wizard auto-fills these with a zero vector of
        the computed length and tells the user to hand-edit the generated
        graph file afterward. metadata_api.IP has no special "vector"
        handling (its param_update() only reacts to enum/minimum/actual
        keys), so replicate config_helper.py's behavior here rather than
        surfacing a param_update() result with no legal-range fields to ask
        the user about."""
        self._require_current(param)
        param_obj = self.ip.param_obj_dict[param]
        param_obj.param_update(self.ip.args, self.ip.module)
        length = (param_obj.update_result or {}).get("len", 0)
        ok, err = self.set_value(param, [0] * length)
        return ok, err, length

    def back(self):
        self.cursor = max(0, self.cursor - 1)

    def state_dict(self):
        return {"ip": self.ip_name, "cursor": self.cursor, "args": dict(self.ip.args)}

    @classmethod
    def from_state(cls, state, meta_dir=None):
        wizard = cls(state["ip"], meta_dir=meta_dir)
        wizard.ip.args.update(state["args"])
        wizard.cursor = state["cursor"]
        return wizard

    def finalize(self, instance_name, out_dir):
        ok, msg = check_vitis_env()
        if not ok:
            return {"ok": False, "error": msg}
        if not config_helper.valid_name(instance_name):
            return {
                "ok": False,
                "error": (
                    "%r is not a valid instance name (letters/digits/"
                    "underscore only, cannot start with a digit)." % instance_name
                ),
            }
        if not self.is_complete():
            return {
                "ok": False,
                "error": "configuration is not complete: %r is still unset"
                % self.order[self.cursor],
            }

        os.makedirs(out_dir, exist_ok=True)
        config_path = os.path.join(out_dir, "config.json")
        with open(config_path, "w") as f:
            json.dump({"parameters": self.ip.args}, f, indent=2)

        import subprocess

        result = subprocess.run(
            [
                sys.executable,
                os.path.join(SCRIPTS_DIR, "metadata_checker.py"),
                "--ip",
                self.ip_name,
                "--lib_dir",
                REPO_ROOT,
            ],
            cwd=out_dir,
        )
        if result.returncode != 0:
            return {
                "ok": False,
                "error": "metadata_checker.py failed -- see its output above.",
            }

        if "vss_" in self.ip_name:
            # metadata_checker.py explicitly skips graph generation for vss_*
            # IPs (`if not "vss" in ip_name: gen_out_files(...)`) because
            # their generate_graph is a non-functional stub -- the real
            # output comes from generate_cfg, which only config_helper.py's
            # own script previously knew to call. Replicate that call
            # directly rather than silently returning nonexistent .h/.json
            # paths for this family.
            cfg_out = self.ip.module.generate_cfg(instance_name, self.ip.args)
            cfg_path = os.path.join(
                out_dir, "cfg_%s_%s.cfg" % (self.ip_name, instance_name)
            )
            with open(cfg_path, "w") as f:
                f.write(cfg_out["cfg"])
            return {
                "ok": True,
                "config_json": config_path,
                "cfg_path": cfg_path,
            }

        generated_dir = os.path.join(
            out_dir, "%s_native_generated_graph" % self.ip_name
        )
        return {
            "ok": True,
            "config_json": config_path,
            "generated_dir": generated_dir,
            "graph_header": os.path.join(
                generated_dir, "%s_generated_graph.h" % self.ip_name
            ),
            "port_info_json": os.path.join(generated_dir, "out_native.json"),
        }


def _coerce_value(raw):
    if isinstance(raw, (int, float)):
        return raw
    raw = raw.strip()
    try:
        return int(raw)
    except ValueError:
        return raw


def _seed_from_qor_result(wizard, csv_path, row_index):
    """--qor-result never shells to config_qor_helper.py itself (that tool
    needs the Vitis-bundled Python with pandas/scikit-learn -- a heavier,
    different interpreter than this engine assumes, and its env setup is
    already documented once in .github/skills/dsp/SKILL.md). Instead this
    reads a CSV *already produced* by a prior, separate run of that tool."""
    with open(csv_path, newline="") as f:
        rows = list(csv.DictReader(f))
    if not rows:
        print("WARNING: %s has no rows; continuing without a QoR seed." % csv_path)
        return
    if row_index >= len(rows):
        print(
            "WARNING: --qor-row %d out of range (%d rows); using row 0."
            % (row_index, len(rows))
        )
        row_index = 0
    row = rows[row_index]

    seeded = 0
    while not wizard.is_complete():
        param = wizard.current_param()
        if wizard.is_vector_param(param) or param not in row:
            break
        ok, err = wizard.set_value(param, _coerce_value(row[param]))
        if not ok:
            print(
                "QoR-seeded value for %s (%r) was rejected: %s -- continuing "
                "manually from here." % (param, row[param], err)
            )
            break
        seeded += 1
    print("Seeded %d parameter(s) from %s (row %d)." % (seeded, csv_path, row_index))


def _offer_qor_or_doc_guidance(ip_name):
    if qor_model_exists(ip_name):
        answer = (
            input(
                "\nA trained QoR model exists for %s. Want a throughput/"
                "latency-optimized starting point? [y/N] " % ip_name
            )
            .strip()
            .lower()
        )
        if answer == "y":
            print(
                "\nRun config_qor_helper.py yourself first (env setup + "
                "constraints authoring: .github/skills/dsp/SKILL.md), then "
                "re-run this wizard with --qor-result <path-to-its-csv>."
            )
        return

    rst_path = pde.rst_path_for(ip_name, UG_DIR)
    notes = pde.extract_configuration_notes(rst_path) if rst_path else []
    if not notes:
        return
    answer = (
        input(
            "\nNo trained QoR model exists yet for %s. Show efficiency/"
            "configuration guidance from the user guide before starting? "
            "[y/N] " % ip_name
        )
        .strip()
        .lower()
    )
    if answer == "y":
        for block in notes:
            print("\n--- %s ---" % block["title"])
            print(block["text"])


def _load_state(path):
    with open(path) as f:
        return json.load(f)


def _save_state(path, wizard):
    with open(path, "w") as f:
        json.dump(wizard.state_dict(), f, indent=2)


def _ensure_state(args):
    if not os.path.isfile(args.state):
        _save_state(args.state, ParamWizard(args.ip, meta_dir=args.mdir))


def cmd_step(args):
    _ensure_state(args)
    wizard = ParamWizard.from_state(_load_state(args.state), meta_dir=args.mdir)
    if wizard.is_complete():
        print(json.dumps({"complete": True}))
        return
    param = wizard.current_param()
    if wizard.is_vector_param(param):
        print(
            json.dumps(
                {
                    "param": param,
                    "is_vector": True,
                    "note": (
                        "auto-filled with zeros on --set; edit the generated "
                        "graph's array by hand afterward"
                    ),
                }
            )
        )
        return
    explanation = wizard.explain(param)
    print(json.dumps({"param": param, "is_vector": False, "explanation": explanation}))


def cmd_set(args):
    _ensure_state(args)
    wizard = ParamWizard.from_state(_load_state(args.state), meta_dir=args.mdir)
    param = wizard.current_param()
    if param is None:
        print(json.dumps({"ok": False, "err": "configuration already complete"}))
        return

    if wizard.is_vector_param(param):
        ok, err, length = wizard.auto_fill_vector(param)
        _save_state(args.state, wizard)
        print(json.dumps({"ok": ok, "err": err, "auto_filled_length": length}))
        return

    name, sep, raw_value = args.set.partition("=")
    if not sep or name != param:
        print(
            json.dumps(
                {
                    "ok": False,
                    "err": "expected --set %s=<value>, got %r" % (param, args.set),
                }
            )
        )
        return
    ok, err = wizard.set_value(param, _coerce_value(raw_value))
    _save_state(args.state, wizard)
    print(json.dumps({"ok": ok, "err": err}))


def cmd_finalize(args):
    _ensure_state(args)
    wizard = ParamWizard.from_state(_load_state(args.state), meta_dir=args.mdir)
    out_dir = args.out_dir or os.getcwd()
    result = wizard.finalize(args.finalize, out_dir)
    print(json.dumps(result))


def _finalize_interactive(wizard, args, out_dir):
    instance_name = args.instance_name
    while not instance_name or not config_helper.valid_name(instance_name):
        instance_name = input("\nPlease enter a name for your instance: ").strip()
    result = wizard.finalize(instance_name, out_dir)
    if not result["ok"]:
        print("\nERROR: %s" % result["error"])
        sys.exit(1)
    print("\nGenerated:")
    print("  config:       %s" % result["config_json"])
    print("  graph header: %s" % result["graph_header"])
    print("  port info:    %s" % result["port_info_json"])


def run_interactive(args):
    if not args.ip or args.list_ips:
        print("Available IPs:")
        for name in list_available_ips():
            print("  %s" % name)
        if not args.ip:
            args.ip = input("\nWhich IP would you like to configure? ").strip()

    ok, msg = check_vitis_env()
    if not ok:
        print("\nNOTE: %s" % msg)

    out_dir = args.out_dir or os.getcwd()

    if args.config:
        with open(args.config) as f:
            config_args = json.load(f)["parameters"]
        wizard = ParamWizard(args.ip, meta_dir=args.mdir)
        wizard.ip.args.update(config_args)
        wizard.cursor = len(wizard.order)
        _finalize_interactive(wizard, args, out_dir)
        return

    wizard = ParamWizard(args.ip, meta_dir=args.mdir)

    if args.qor_result:
        _seed_from_qor_result(wizard, args.qor_result, args.qor_row)
    else:
        _offer_qor_or_doc_guidance(args.ip)

    while not wizard.is_complete():
        param = wizard.current_param()

        if wizard.is_vector_param(param):
            ok, err, length = wizard.auto_fill_vector(param)
            print(
                "\n%s is auto-filled with an all-zero vector of length %d."
                % (param, length)
            )
            print("Edit the array in the generated graph file by hand afterward.")
            continue

        explanation = wizard.explain(param)
        print("\n%s" % explanation)
        raw = input(
            "Please input a value for %s (or 'z' to go back): " % param
        ).strip()
        if raw.lower() == "z":
            wizard.back()
            continue
        ok, err = wizard.set_value(param, _coerce_value(raw))
        if not ok:
            print("ERROR: %s" % err)

    _finalize_interactive(wizard, args, out_dir)


def build_arg_parser():
    parser = argparse.ArgumentParser(
        description="Interactive configuration wizard for xf_dsp IPs.",
        add_help=False,
    )
    parser.add_argument(
        "--h", "--help", "-h", action="help", help="show this help message and exit"
    )
    parser.add_argument("--ip", help="IP name, e.g. fir_sr_asym")
    parser.add_argument("--mdir", help="override the L2/meta directory (advanced)")
    parser.add_argument(
        "--list-ips", action="store_true", help="list available IPs and exit"
    )
    parser.add_argument(
        "--state", help="state file for --step/--set/--finalize (AI-agent mode)"
    )
    parser.add_argument(
        "--step",
        action="store_true",
        help="print the current parameter + explanation as JSON and exit",
    )
    parser.add_argument(
        "--set", metavar="NAME=VALUE", help="set the current parameter and exit"
    )
    parser.add_argument(
        "--finalize",
        metavar="INSTANCE_NAME",
        help="validate + generate using --state's config and exit",
    )
    parser.add_argument(
        "--config",
        help="skip straight to validate+generate using this existing config.json",
    )
    parser.add_argument(
        "--qor-result",
        metavar="CSV",
        help=(
            "seed parameters from a config_qor_helper.py --out_csv_file result "
            "(run that tool separately -- see .github/skills/dsp/SKILL.md)"
        ),
    )
    parser.add_argument(
        "--qor-row",
        type=int,
        default=0,
        help="row of --qor-result to use (default: 0, the top-ranked config)",
    )
    parser.add_argument("--instance-name")
    parser.add_argument("--out-dir")
    return parser


def main():
    args = build_arg_parser().parse_args()

    if args.list_ips and not (args.step or args.set or args.finalize):
        for name in list_available_ips():
            print(name)
        return

    if args.step or args.set or args.finalize:
        if not args.state or not args.ip:
            print(json.dumps({"ok": False, "err": "--step/--set/--finalize require --ip and --state"}))
            sys.exit(1)
        if args.step:
            cmd_step(args)
        elif args.set:
            cmd_set(args)
        else:
            cmd_finalize(args)
        return

    run_interactive(args)


if __name__ == "__main__":
    main()
