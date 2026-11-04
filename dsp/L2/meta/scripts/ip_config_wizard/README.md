### Overview
The IP Configuration Wizard is an interactive alternative to `config_helper.py` for
building a `config.json` and generating a graph instance (or, for `vss_*` IPs, a
`.cfg` file) for an xf_dsp AIE IP. Unlike `config_helper.py`, it also explains *why*
a parameter has the constraints it does as it asks for a value -- using the
`@tparam` documentation from the IP's graph header, and (when no throughput/latency
model exists for the IP) the "Configuration Notes" / "Design Notes" section of the
IP's user guide page.

It is built entirely on `metadata_api.IP` -- the same validator/updater functions
`config_helper.py` and `metadata_checker.py` use -- and always finalizes through
`metadata_checker.py` unmodified, so its output has full parity with the existing
tools. See `analysis/dsp_ip_config_wizard/dsp_ip_config_wizard_proposal.md` in the
internal dsplib workspace for the full design rationale, including why this is a
separate tool rather than an extension of `config_helper.py`.

Files in this directory:
- `ip_config_wizard.py` -- the wizard itself (interactive CLI + stateless step API)
- `param_doc_extractor.py` -- parses `@tparam` doxygen blocks and user-guide RST
  sections; used by the wizard, not run directly
- `wizard_ip_survey.py` -- full-catalog regression smoke test (see below)

### Environment Setup
The per-parameter walk itself needs only plain `python3` -- no Vitis environment
required. Finalizing a configuration (`--finalize`, or the interactive CLI's
end-of-wizard step) shells out to `metadata_checker.py`, which validates against
`$XILINX_VITIS/data/ipmetadata/vitis_library_api_spec_schema.json` and therefore
does need a sourced Vitis environment, the same as `config_qor_helper.py`:

**Module-based setup (recommended):**
```bash
module load xilinx/ta/2026.1_daily_latest
source $XILINX_VITIS/settings64.sh  # or settings64.csh for tcsh
```

**Direct path setup:**
```bash
source <path_to_vitis_install>/Vitis/settings64.sh
```

If `$XILINX_VITIS` isn't set, the wizard still lets you build a complete
configuration -- it warns at startup and only hard-fails at the finalize step.

### Running the Wizard (standalone, no AI agent)
From this directory (`./ip_config_wizard/`), run:
```bash
python3 ip_config_wizard.py --ip <ip_name>
```
It will list every parameter in order, explain it, and prompt for a value; a
`config.json` and generated graph instance are written on completion. See the full
option list with:
```bash
python3 ip_config_wizard.py --h
```

Already have a complete `config.json` and just want it validated and turned into a
graph instance? Skip the walk entirely:
```bash
python3 ip_config_wizard.py --ip <ip_name> --config <path_to_config.json> --out-dir <dir>
```

To see the full IP catalog the wizard supports:
```bash
python3 ip_config_wizard.py --list-ips
```

### Driving the Wizard from an AI Coding Assistant
The wizard also exposes a stateless, JSON-in/JSON-out `--step` / `--set` /
`--finalize` API backed by a `--state` file, intended for an AI agent to drive turn
by turn (present the explanation conversationally, relay validation errors
alongside the parameter's explanation, and so on). This is documented in full,
with the exact flow an agent should follow, in
`.github/skills/dsp-ip-config/SKILL.md` -- point a coding assistant there rather
than duplicating that walkthrough here. A QoR-optimized starting point
(`config_qor_helper.py`, see `L2/meta/scripts/qor_helper/README.md`) can be fed
into the wizard via `--qor-result`, also documented in that skill file.

### Full-Catalog Regression Survey
`wizard_ip_survey.py` drives every IP `ip_config_wizard.py --list-ips` returns
through a naive "pick the first legal enum/minimum" walk, confirming the
metadata-driven walk mechanics don't crash and reach a config
`metadata_checker.py` accepts, for at least one path through each IP's parameter
space. This is the same mechanism used to find the known gaps below during the
initial full-catalog rollout, kept as a standing regression gate.

```bash
python3 wizard_ip_survey.py                  # full run, finalize included (needs Vitis env)
python3 wizard_ip_survey.py --skip-finalize  # walk only, no Vitis env needed
python3 wizard_ip_survey.py --ip fir_sr_asym # single IP
```

Exit code is `0` if every non-excluded IP passes, `1` otherwise.

### Known Gaps
`fft_dit_2ch_real` and `fft_ifft_2d` have no `L2/meta/<ip>.json` today, so no
metadata-driven tool (this wizard, `config_helper.py`, or `config_qor_helper.py`)
can configure them yet. `vss_fft_ifft_1d` can crash or print stray non-JSON output
due to an unrelated bug in a shared helper module it imports
(`get_aievar_from_part.py`). All three are excluded from `wizard_ip_survey.py` by
default (`KNOWN_GAP_IPS`; pass `--include-known-gaps` to re-check them). See
`analysis/dsp_ip_config_wizard/dsp_ip_config_wizard_known_gaps.md` in the internal
dsplib workspace for full root-cause detail and the fix recommended for each when
picked up. Every other IP is fully supported.
