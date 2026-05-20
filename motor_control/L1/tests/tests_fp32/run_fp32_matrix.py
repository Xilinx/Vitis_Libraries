#!/usr/bin/env python3
#
# Copyright (C) 2025-2025, Advanced Micro Devices, Inc.
# SPDX-License-Identifier: X11
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Except as contained in this notice, the name of Advanced Micro Devices
# shall not be used in advertising or otherwise to promote the sale,
# use or other dealings in this Software without prior written authorization
# from Advanced Micro Devices, Inc.
#
"""Run csim + csynth for each *_tb under this directory; write REGRESSION_SYNTH_TABLE.md."""
from __future__ import annotations

import glob
import os
import re
import subprocess
import sys
from datetime import datetime, timezone

ROOT = os.path.dirname(os.path.abspath(__file__))
XPART = os.environ.get("XPART", "xcu200-fsgd2104-2-e")
# Output markdown filename (under this directory), e.g. OUTPUT=REGRESSION_SNAPSHOT_VCK190.md
OUTPUT = os.environ.get("OUTPUT", "REGRESSION_SYNTH_TABLE.md")
KIT = os.environ.get("KIT", "")  # optional human label, e.g. "VCK190" or "Alveo U200"
CSIM_TIMEOUT = int(os.environ.get("CSIM_TIMEOUT", "900"))
CSYNTH_TIMEOUT = int(os.environ.get("CSYNTH_TIMEOUT", "1200"))


def find_tb_dirs() -> list[str]:
    out = []
    for name in sorted(os.listdir(ROOT)):
        if not name.endswith("_tb"):
            continue
        p = os.path.join(ROOT, name)
        if os.path.isdir(p) and os.path.isfile(os.path.join(p, "Makefile")):
            out.append(p)
    return out


def parse_csynth_rpt(path: str) -> dict[str, str]:
    d: dict[str, str] = {
        "bram": "",
        "dsp": "",
        "ff": "",
        "lut": "",
        "est_ns": "",
        "lat_min": "",
        "lat_max": "",
        "ii_min": "",
        "ii_max": "",
    }
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            txt = f.read()
    except OSError:
        return d
    m = re.search(r"\|ap_clk\s+\|\s+[\d.]+\s+ns\|\s+([\d.]+)\s+ns\|", txt)
    if m:
        d["est_ns"] = m.group(1)
    m = re.search(
        r"\|\s*Total\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*(\d+)\s*\|",
        txt,
    )
    if m:
        d["bram"] = m.group(1)
        d["dsp"] = m.group(2)
        d["ff"] = m.group(3)
        d["lut"] = m.group(4)
    m = re.search(
        r"\|\s*(\d+)\|\s*(\d+)\|\s+[\d.]+\s+ns\|\s+[\d.]+\s+ns\|\s+(\d+)\|\s+(\d+)\|\s+",
        txt,
    )
    if m:
        d["lat_min"] = m.group(1)
        d["lat_max"] = m.group(2)
        d["ii_min"] = m.group(3)
        d["ii_max"] = m.group(4)
    return d


def run_one(tb_path: str) -> dict:
    name = os.path.basename(tb_path)
    row: dict = {"tb": name, "csim": "fail", "csynth": "fail", "note": ""}
    env = os.environ.copy()
    env.setdefault("XPART", XPART)

    try:
        p = subprocess.run(
            ["make", "run", "TARGET=csim", f"XPART={XPART}"],
            cwd=tb_path,
            capture_output=True,
            text=True,
            timeout=CSIM_TIMEOUT,
            env=env,
        )
    except subprocess.TimeoutExpired:
        row["note"] = "csim timeout"
        return row
    log = (p.stdout or "") + (p.stderr or "")
    ok_c = p.returncode == 0 and (
        "ALL TESTS PASSED" in log
        or "CSim done with 0 errors" in log
        or "0 errors" in log and "CSIM" in log.upper()
    )
    row["csim"] = "pass" if ok_c else "fail"
    if not ok_c:
        row["note"] = (row["note"] + " csim_rc=%s" % p.returncode).strip()

    try:
        p2 = subprocess.run(
            ["make", "all", "TARGET=csynth", f"XPART={XPART}"],
            cwd=tb_path,
            capture_output=True,
            text=True,
            timeout=CSYNTH_TIMEOUT,
            env=env,
        )
    except subprocess.TimeoutExpired:
        row["note"] = (row["note"] + " csynth timeout").strip()
        return row
    row["csynth"] = "pass" if p2.returncode == 0 else "fail"
    if p2.returncode != 0:
        row["note"] = (row["note"] + " csynth_rc=%s" % p2.returncode).strip()

    rpts = glob.glob(os.path.join(tb_path, "hls", "hls", "syn", "report", "*_csynth.rpt"))
    if not rpts:
        rpts = glob.glob(os.path.join(tb_path, "hls", "**", "*_csynth.rpt"), recursive=True)
    if rpts:
        rpts.sort(key=os.path.getmtime, reverse=True)
        d = parse_csynth_rpt(rpts[0])
        row.update(d)
    else:
        row["note"] = (row["note"] + " no_csynth_rpt").strip()

    return row


def main() -> int:
    tbs = find_tb_dirs()
    if not tbs:
        print("No *_tb with Makefile under", ROOT, file=sys.stderr)
        return 1

    rows = []
    for tb in tbs:
        print("---", os.path.basename(tb), flush=True)
        rows.append(run_one(tb))

    md_path = os.path.join(ROOT, OUTPUT)
    when = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    lines = [
        "# tests_fp32: csim + csynth matrix (no BIND_OP)",
        "",
        f"- Generated: {when}",
    ]
    if KIT:
        lines.append(f"- Kit / board: **{KIT}**")
    lines.extend(
        [
            f"- XPART: `{XPART}`",
            f"- BIND_OP: removed from `xf_motorcontrol` sources (HLS default resource mapping).",
            "",
            "| Testbench | csim | csynth | DSP | FF | LUT | Est clk (ns) | Latency (c) | Interval (c) |",
            "|-----------|------|--------|-----|----|----|--------------|-------------|--------------|",
        ]
    )
    for r in rows:
        lat = ""
        if r.get("lat_min") and r.get("lat_max"):
            lat = f"{r['lat_min']}–{r['lat_max']}"
        ii = ""
        if r.get("ii_min") and r.get("ii_max"):
            ii = f"{r['ii_min']}–{r['ii_max']}"
        lines.append(
            "| {tb} | {csim} | {csynth} | {dsp} | {ff} | {lut} | {est} | {lat} | {ii} |".format(
                tb=r.get("tb", ""),
                csim=r.get("csim", ""),
                csynth=r.get("csynth", ""),
                dsp=r.get("dsp", ""),
                ff=r.get("ff", ""),
                lut=r.get("lut", ""),
                est=r.get("est_ns", ""),
                lat=lat,
                ii=ii,
            )
        )

    lines.append("")
    lines.append("## Notes")
    lines.append("")
    for r in rows:
        if r.get("note"):
            lines.append(f"- **{r['tb']}**: {r['note']}")
    lines.append("")

    with open(md_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print("Wrote", md_path)
    failed = [r["tb"] for r in rows if r.get("csim") != "pass" or r.get("csynth") != "pass"]
    if failed:
        print("Failures:", ", ".join(failed), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
