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
Parses the two prose sources of parameter guidance for xf_dsp IPs, so an
interactive configuration tool (see ip_config_wizard.py) can explain *why* a
parameter has the constraints it does, not just enforce them:

- doxygen @tparam blocks in L2/include/aie/<ip>_graph.hpp -- the authoritative
  per-parameter description (ranges, enums, cross-parameter notes). This is
  the only place the prose explanation of a parameter lives; the JSON/py
  metadata is purely numeric/enum.
- docs/src/user_guide/L2/*.rst "Configuration Notes" (or "Design Notes")
  sections -- cross-parameter efficiency guidance, used as the fallback when
  no QoR model exists for an IP (see ip_config_wizard.py's --qor-result path).

Both extractors are best-effort and read only source files that ship in the
repo -- docs/xml and docs/html are gitignored local Doxygen/Sphinx build
artifacts external users will not have, so nothing here depends on them.
Callers must degrade gracefully (skip the extra prose) when a source doesn't
match the expected convention; there is no metadata field guaranteeing either
one exists for a given IP.
"""
import os
import re

IP_FAMILY_RST_MAP = {
    # Hand-maintained. Content-grepping an RST for a graph class name is
    # unreliable -- family-shared docs (e.g. every fir_* variant) often never
    # mention the specific class name at all. Update this table whenever a
    # new IP is added or a user_guide/L2 file is renamed/split. Verified
    # against docs/src/user_guide/L2/*.rst -- each entry's target file has
    # either a "Configuration Notes" or "Design Notes" heading, confirmed
    # during the initial pilot and the full-catalog rollout.
    #
    # Deliberately absent: fft_dit_2ch_real, fft_ifft_2d (no L2/meta/<ip>.json
    # exists for either -- not yet configurable by any tool, tracked
    # separately) and vss_fft_ifft_1d (graph_hpp/rst resolution works, but the
    # IP crashes on import via an unrelated get_aievar_from_part.py bug, also
    # tracked separately). See analysis/dsp_ip_config_wizard/ for both.
    "fir_sr_asym": "func-fir-filtersAIE.rst",
    "fir_sr_sym": "func-fir-filtersAIE.rst",
    "fir_interpolate_asym": "func-fir-filtersAIE.rst",
    "fir_interpolate_hb": "func-fir-filtersAIE.rst",
    "fir_decimate_asym": "func-fir-filtersAIE.rst",
    "fir_decimate_sym": "func-fir-filtersAIE.rst",
    "fir_decimate_hb": "func-fir-filtersAIE.rst",
    "fir_resampler": "func-fir-filtersAIE.rst",
    "fir_tdm": "func-fir-TDM.rst",
    "fft_ifft_dit_1ch": "func-fft-ifft-aie-only.rst",
    "bitonic_sort": "func-bitonic-sort.rst",
    "conv_corr": "func-conv-corr.rst",
    "cumsum": "func-cumsum.rst",
    "dds_mixer": "func-dds_mixer_intrinsic.rst",
    "dds_mixer_lut": "func-dds_mixer_lut.rst",
    "dft": "func-dft.rst",
    "euclidean_distance": "func-ed.rst",
    "fft_window": "func-fft_window.rst",
    "func_approx": "func-func-approx.rst",
    "hadamard": "func-hadamard.rst",
    "kronecker": "func-kronecker.rst",
    "matrix_mult": "func-matmul.rst",
    "matrix_vector_mul": "func-matrix_vector_mul.rst",
    "mixed_radix_fft": "func-mixed_radix_fft.rst",
    "outer_tensor": "func-outer-tensor.rst",
    "sample_delay": "func-sample_delay.rst",
    "widget_api_cast": "func-widget-apicast.rst",
    "widget_real2complex": "func-widget-real2comp.rst",
}


def graph_hpp_path_for(ip_name, l2_dir):
    """Resolve the graph header by convention: L2/include/aie/<ip>_graph.hpp.

    Returns None (not an exception) if the convention doesn't hold for this
    IP -- callers must degrade gracefully (skip prose explanations, keep
    validating/generating) rather than fail the whole session over a missing
    doc source. There is no metadata field naming the graph header; this is
    convention plus an existence check, not a guarantee.
    """
    candidate = os.path.join(l2_dir, "include", "aie", "%s_graph.hpp" % ip_name)
    return candidate if os.path.isfile(candidate) else None


_DOXYGEN_BLOCK_RE = re.compile(r"/\*\*(.*?)\*\*/", re.DOTALL)
_TPARAM_ENTRY_RE = re.compile(r"@tparam\s+(\w+)\b")


def extract_tparam_docs(graph_hpp_path):
    """Parse the class-level doxygen block above the <ip>_graph class and
    return {param_name: description_text} for every @tparam entry.

    Regex-only over the raw comment text -- no doxygen toolchain dependency,
    since docs/xml and docs/html are gitignored build artifacts external
    users will not have. Picks the *first* /** ... **/ block that contains at
    least one @tparam entry (the class-level template-parameter block);
    per-method doxygen blocks further down the file are not class-level and
    are ignored. Splits on '@tparam <NAME>' boundaries -- a name is a single
    token so this does not accidentally split on the mid-comment references
    to unrelated identifiers.
    """
    if not graph_hpp_path or not os.path.isfile(graph_hpp_path):
        return {}

    with open(graph_hpp_path) as f:
        content = f.read()

    doc_block = None
    for match in _DOXYGEN_BLOCK_RE.finditer(content):
        if "@tparam" in match.group(1):
            doc_block = match.group(1)
            break
    if doc_block is None:
        return {}

    # Strip the doxygen comment leader ("* " or "*") from each line so the
    # regex below sees plain prose rather than comment decoration.
    cleaned_lines = []
    for raw_line in doc_block.splitlines():
        line = raw_line.strip()
        if line.startswith("*"):
            line = line[1:].strip()
        cleaned_lines.append(line)
    cleaned = "\n".join(cleaned_lines)

    docs = {}
    entries = list(_TPARAM_ENTRY_RE.finditer(cleaned))
    for i, entry in enumerate(entries):
        name = entry.group(1)
        start = entry.end()
        end = entries[i + 1].start() if i + 1 < len(entries) else len(cleaned)
        text = cleaned[start:end].strip()
        text = re.sub(r"\\n", " ", text)  # doxygen literal line-break markers
        text = re.sub(r"\s+", " ", text).strip()
        docs[name] = text
    return docs


def rst_path_for(ip_name, ug_dir):
    """Look up the RST file documenting this IP via IP_FAMILY_RST_MAP.
    Returns None if the IP has no table entry or the file doesn't exist --
    both are valid "no doc source available" outcomes, not errors."""
    filename = IP_FAMILY_RST_MAP.get(ip_name)
    if filename is None:
        return None
    candidate = os.path.join(ug_dir, filename)
    return candidate if os.path.isfile(candidate) else None


_SCENARIO_HEADING_RE = re.compile(r"^\*\*(SCENARIO[^*]*)\*\*:?\s*$", re.IGNORECASE)


def extract_configuration_notes(rst_path):
    """Pull the prose under the 'Configuration Notes' heading (falling back
    to 'Design Notes'), split into named blocks wherever the RST uses a
    '**SCENARIO N**' sub-heading convention.

    Returns a list of {"title": str, "text": str, "preset": dict}. "preset"
    is always {} today: these sections are prose throughput/resource
    tradeoff walkthroughs (e.g. "set TP_SSR = 2 and TP_PARA_INTERP_POLY = 1"
    embedded in a paragraph), not a machine-parseable param=value list, so
    extracting concrete presets from them reliably is not attempted. The key
    is still present so a caller can offer a pre-fill when a future, more
    literal RST convention populates it, without changing this function's
    contract. Falls back to an empty list (never an exception) when the
    heading can't be found -- callers should treat that the same as "no
    guidance available for this IP", identical to a missing QoR model.
    """
    if not rst_path or not os.path.isfile(rst_path):
        return []

    with open(rst_path) as f:
        text = f.read()

    section = _extract_rst_section(text, "Configuration Notes")
    if section is None:
        section = _extract_rst_section(text, "Design Notes")
    if section is None:
        return []

    blocks = []
    current_title = "General"
    current_lines = []

    def flush():
        body = "\n".join(current_lines).strip()
        if body:
            blocks.append({"title": current_title, "text": body, "preset": {}})

    for line in section.splitlines():
        heading_match = _SCENARIO_HEADING_RE.match(line.strip())
        if heading_match:
            flush()
            current_title = heading_match.group(1).strip()
            current_lines = []
        else:
            current_lines.append(line)
    flush()
    return blocks


def _extract_rst_section(text, heading):
    """RST section headings are a title line immediately followed by a
    same-or-greater-length underline of a single repeated symbol (here,
    '=' or '-', which is what every user_guide/L2 file uses). Return the
    body between `heading`'s underline and the next heading at the SAME
    level, or None if `heading` isn't present.

    "Same level" matters: docutils assigns heading depth by the order
    distinct underline characters are first encountered in a document, not
    by a fixed '=' vs '-' mapping, so this only tracks the underline
    character `heading` itself used and treats a later heading as the
    section's end only if it reuses that same character. A shallower
    boundary check (stopping at *any* underlined heading) would end a
    section like "Design Notes" at its own first subsection -- several
    user_guide/L2 files head straight into a '-'-underlined subsection
    (e.g. "Cascade Feature") with no top-level prose of its own, and a
    same-or-lower-character check returned an empty section for every one
    of them until this was caught during the full-catalog rollout."""
    lines = text.splitlines()

    def underline_char(line):
        stripped = line.strip()
        if stripped and len(set(stripped)) == 1 and stripped[0] in "=-":
            return stripped[0]
        return None

    start = None
    heading_char = None
    for i in range(len(lines) - 1):
        if lines[i].strip() == heading:
            heading_char = underline_char(lines[i + 1])
            if heading_char is not None:
                start = i + 2
                break
    if start is None:
        return None

    def is_underline(line):
        return underline_char(line) == heading_char

    end = len(lines)
    for j in range(start + 1, len(lines) - 1):
        if lines[j].strip() and is_underline(lines[j + 1]):
            end = j
            break
    return "\n".join(lines[start:end])
