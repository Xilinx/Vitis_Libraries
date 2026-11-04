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
Generic regression smoke test for ip_config_wizard.py, run across every IP
`list_available_ips()` returns (minus KNOWN_GAP_IPS below).

For each IP it drives ParamWizard with a naive "pick the first legal
enum/minimum" walk -- the same mechanism used to survey the full catalog
during the initial rollout, which is how the fft_dit_2ch_real/fft_ifft_2d
missing-metadata gap and the vss_fft_ifft_1d import crash were found in the
first place. This is deliberately not a claim that every legal configuration
works, only that the metadata-driven walk mechanics (param_update/
param_validate/auto_fill_vector) do not crash and reach a config
`metadata_checker.py` accepts, for at least one path through each IP's
parameter space.

Usage:
    python3 wizard_ip_survey.py                  # full run, finalize included
    python3 wizard_ip_survey.py --skip-finalize   # walk only, no Vitis env needed
    python3 wizard_ip_survey.py --ip fir_sr_asym  # single IP

Exit code is 0 if every non-excluded IP passes, 1 otherwise -- suitable as a
CI/regression gate.
"""
import argparse
import json
import os
import sys
import tempfile

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))
if SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, SCRIPTS_DIR)

import ip_config_wizard as w  # noqa: E402

# Known, separately-tracked gaps -- see analysis/dsp_ip_config_wizard/ in the
# internal dsplib workspace for the follow-up plan on each. Excluded by
# default so this survey stays a meaningful green/red signal; pass
# --include-known-gaps to see them fail again.
KNOWN_GAP_IPS = {
    "fft_dit_2ch_real": "no L2/meta/fft_dit_2ch_real.json exists (listed in api.json, not configurable by any tool today)",
    "fft_ifft_2d": "no L2/meta/fft_ifft_2d.json exists (listed in api.json, not configurable by any tool today)",
    "vss_fft_ifft_1d": "importing it crashes via an unrelated top-level sys.argv[1] read in get_aievar_from_part.py",
}


def _pick_candidate(param_obj):
    result = param_obj.update_result or {}
    if "enum" in result and result["enum"]:
        return result["enum"][0]
    if "minimum" in result:
        return result["minimum"]
    return 0


def survey_one(ip_name, skip_finalize):
    try:
        wizard = w.ParamWizard(ip_name)
    except SystemExit as e:
        return "SYS_EXIT", "metadata_api called sys.exit(%r) -- missing/malformed <ip>.json" % (e.code,)
    except Exception as e:
        return "EXCEPTION", "%s: %s" % (type(e).__name__, e)

    try:
        while not wizard.is_complete():
            param = wizard.current_param()
            if wizard.is_vector_param(param):
                ok, err, _length = wizard.auto_fill_vector(param)
                if not ok:
                    return "WALK_FAIL", "%s (vector): %s" % (param, err)
                continue

            param_obj = wizard.ip.param_obj_dict[param]
            param_obj.param_update(wizard.ip.args, wizard.ip.module)
            candidate = _pick_candidate(param_obj)
            ok, err = wizard.set_value(param, candidate)
            if not ok:
                # Mirror config_helper.py's own recovery: re-update to see
                # if the validator proposed a corrected "actual" value.
                param_obj.param_update(wizard.ip.args, wizard.ip.module)
                actual = (param_obj.update_result or {}).get("actual")
                if actual is not None:
                    ok, err = wizard.set_value(param, actual)
                if not ok:
                    return "WALK_FAIL", "%s: %s" % (param, err)
    except SystemExit as e:
        return "SYS_EXIT", "metadata_api called sys.exit(%r) mid-walk" % (e.code,)
    except Exception as e:
        return "EXCEPTION", "%s: %s" % (type(e).__name__, e)

    if skip_finalize:
        return "PASS (walk only)", ""

    ok, msg = w.check_vitis_env()
    if not ok:
        return "SKIPPED", msg

    with tempfile.TemporaryDirectory(prefix="wizard_survey_%s_" % ip_name) as out_dir:
        result = wizard.finalize("wizard_survey_instance", out_dir)
    if result["ok"]:
        return "PASS", ""
    return "FINALIZE_FAIL", result.get("error", "")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ip", help="survey a single IP instead of the full catalog")
    parser.add_argument(
        "--skip-finalize",
        action="store_true",
        help="only exercise the parameter walk; skip metadata_checker.py (no Vitis env needed)",
    )
    parser.add_argument(
        "--include-known-gaps",
        action="store_true",
        help="also survey the IPs in KNOWN_GAP_IPS (expected to fail)",
    )
    parser.add_argument("--json", action="store_true", help="print machine-readable JSON instead of a table")
    args = parser.parse_args()

    ip_names = [args.ip] if args.ip else w.list_available_ips()
    if not args.include_known_gaps:
        ip_names = [name for name in ip_names if name not in KNOWN_GAP_IPS]

    results = {}
    for ip_name in ip_names:
        status, detail = survey_one(ip_name, args.skip_finalize)
        results[ip_name] = {"status": status, "detail": detail}

    passed = [n for n, r in results.items() if r["status"].startswith("PASS")]
    failed = [n for n, r in results.items() if not r["status"].startswith(("PASS", "SKIPPED"))]

    if args.json:
        print(json.dumps({"results": results, "pass": len(passed), "total": len(results)}, indent=2))
    else:
        print("=== wizard_ip_survey: %d/%d passed ===" % (len(passed), len(results)))
        for name in sorted(results):
            r = results[name]
            marker = "OK  " if r["status"].startswith("PASS") else ("SKIP" if r["status"] == "SKIPPED" else "FAIL")
            print("%s %-28s %-16s %s" % (marker, name, r["status"], r["detail"][:120]))

    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
