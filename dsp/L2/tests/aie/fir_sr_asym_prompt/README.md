# Prompt-driven `fir_sr_asym` verification POC

This directory verifies that Codex can convert a natural-language request into
a valid, compilable, and functionally correct instance of
`xf::dsp::aie::fir::sr_asym::fir_sr_asym_graph`, **by driving the same
dsp-ip-config wizard an AI coding agent uses in a real chat session**
(`L2/meta/scripts/ip_config_wizard/ip_config_wizard.py`), not by having Codex
read raw metadata files and guess a whole configuration in one shot.

The committed test case contains a link to a prompt instead of FIR graph
parameters. The FIR parameters are produced at test time -- one parameter per
Codex turn, validated immediately via the wizard's `ParamWizard` -- and then
consumed by the normal DSPLib compile and simulation flow.

## Flow

```text
test_suites/smoke.txt
        |
        v
create_prompt_params.py
        |
        v
multi_params_smoke.json       prompt references only
        |
        v
common/scripts/run_batch.sh
        |
        v
common/scripts/run_pack.mk    creates an isolated result directory
        |
        v
Makefile pre_build
        |
        +--> run_codex_config.py
        |       |
        |       +--> reads prompts/basic_aie1.txt
        |       +--> imports ParamWizard from
        |       |    L2/meta/scripts/ip_config_wizard/ip_config_wizard.py
        |       +--> for each parameter in the wizard's dependency-safe order:
        |       |       - asks it for the parameter's real explanation/legal
        |       |         range (ParamWizard.explain(), or the required
        |       |         length for the "coeff" vector parameter)
        |       |       - runs one `codex exec` turn with codex_output_schema.json,
        |       |         asking Codex to answer that one parameter only
        |       |       - feeds the answer through ParamWizard.set_value() --
        |       |         the same metadata_api.IP validators config_helper.py
        |       |         and metadata_checker.py use -- failing fast, and
        |       |         attributed to that parameter, on rejection
        |       +--> writes config.json from the wizard's fully-validated args
        |       +--> materializes test-harness parameters into the result copy
        |            of multi_params_smoke.json
        |
        +--> L2/meta/scripts/metadata_checker.py --ip fir_sr_asym
                |
                +--> validates metadata schema and dependencies
                +--> runs all fir_sr_asym validators
                +--> runs config_helper checks
                +--> generates fir_sr_asym_native_generated_graph/
        |
        v
reference compile and x86 simulation
        |
        v
generated-graph UUT compile and x86 simulation
        |
        v
existing DSPLib output comparison and status reporting
```

`metadata_checker.py` remains the final acceptance oracle exactly as before --
the wizard's own `ParamWizard.finalize()` (which would also shell out to it)
is deliberately *not* called here, so it only ever runs once, from the
existing pre_build step. The wizard is used purely for the parameter-by-
parameter construction and validation that used to be Codex's own,
unvalidated, one-shot guess.

## Prompt-only suite input

`test_suites/smoke.txt` uses the familiar DSPLib suite syntax:

```text
make all PROMPT_FILE=prompts/basic_aie1.txt DEVICE=aie1 TARGET=x86sim
```

`DEVICE` and `TARGET` schedule the appropriate platform and simulation target.
They do not define the FIR graph. `create_prompt_params.py` converts the suite
entry to a prompt-only parameter file such as:

```json
{
  "test_1_basic_aie1_aie1_x86sim_smoke": {
    "PROMPT_FILE": "prompts/basic_aie1.txt",
    "CODEX_MODEL": ""
  }
}
```

Set `CODEX_MODEL` in a suite entry when CI must pin a particular model. An empty
value uses the configured Codex default.

`test_suites/checkin.txt` adds two more cases, at opposite ends of how much
detail a user might give:

```text
make all PROMPT_FILE=prompts/vague_fir.txt DEVICE=aie1 TARGET=x86sim
make all PROMPT_FILE=prompts/detailed_fir_aie2.txt DEVICE=aie2 TARGET=x86sim
```

- `prompts/vague_fir.txt` -- a loosely described request (device family and
  data width only; no API, SSR/cascade, window size, shift/round/sat, or
  coefficient content given) -- tests how well the per-turn wizard
  explanations alone let Codex converge on a complete, legal configuration
  with no explicit guidance for most parameters.
- `prompts/detailed_fir_aie2.txt` -- every graph parameter and the exact
  coefficient values are spelled out (an AIE-ML/AIE2, real-data, stream,
  reloadable-coefficient configuration, deliberately different from
  `basic_aie1.txt`'s AIE1/complex-data/window/static-coefficient one) --
  tests that a fully-specified request still round-trips through the wizard
  unchanged rather than second-guessed.

Add or edit cases by adding a prompt file under `prompts/` and a line in the
relevant `test_suites/<name>.txt`; nothing else needs to change.

## Full prompt text: where to look

Every word Codex sees on a given turn is one of exactly two files -- there is
no wrapper text hidden in Python for a user to reverse-engineer:

- `prompts/<case>.txt` -- the user's own natural-language request, unchanged
  for the whole session.
- `prompts/turn_instruction_template.txt` -- the one fixed wrapper sent every
  turn (the session framing, the current parameter's name, and either its
  real wizard explanation or the coefficient-array length note, then the
  response-format instructions). `run_codex_config.py` only substitutes the
  per-turn variables into this file; it does not define any instruction text
  of its own.

Reading these two files together is exactly what Codex is asked at any given
turn -- use them to reproduce a run manually, or to compare how a differently
-prompted or differently-driven (e.g. not through `ip_config_wizard.py`)
session would have answered the same question.

## Codex output contract

`run_codex_config.py` invokes Codex non-interactively, once per wizard
parameter, constraining every response with the same `codex_output_schema.json`
(a scalar `value`, a vector `values`, and a one-sentence `reason` -- exactly
one of `value`/`values` is populated depending on whether that turn's
parameter is scalar or a coefficient vector). Each answer is validated
immediately against the real parameter before the wizard advances to the
next one; a rejected answer fails the test at that parameter rather than
surfacing only much later inside `metadata_checker.py`.

The runner writes:

- `codex_responses/<NN>_<PARAM>.json`: raw structured Codex response for each
  individual parameter turn, in wizard order.
- `config.json`: graph parameters consumed by `metadata_checker.py`.
- `codex_configuration_report.json`: prompt, final parameters, and each
  turn's answer/reason/validation result.
- A materialized copy of the selected `multi_params` case containing the test
  harness names required by `uut_config.h` and the existing Makefiles.

These files are generated inside the isolated batch result directory. The
source `multi_params_smoke.json` remains prompt-only.

## Metadata validation and generated graph

The existing `L2/meta/scripts/metadata_checker.py` remains the acceptance
oracle. It imports `L2/meta/fir_sr_asym.py`, validates the generated parameters,
and calls its `generate_graph()` implementation. Successful validation creates:

```text
fir_sr_asym_native_generated_graph/
├── fir_sr_asym_generated_graph.h
└── out_native.json
```

For the UUT build, `test.hpp` instantiates
`fir_sr_asym_native_generated_graph`. The reference build continues to
instantiate `fir_sr_asym_ref_graph`. Both are connected to the established
128-bit PLIO test harness and use the existing input generation and output
comparison scripts.

## Running the smoke suite

Set up the DSPLib and Vitis environment, then run:

```bash
source ~/dsplib_env.csh
cd L2/tests/aie/fir_sr_asym_prompt
./runmake.sh -run_type smoke -nolsf
```

Options:

- Omit `-nolsf` to submit through the existing LSF batch path.
- Add `-no_test_run` to generate `multi_params_smoke.json` without compiling or
  simulating.
- Use `-run_type <name>` to consume `test_suites/<name>.txt` and generate
  `multi_params_<name>.json`.

## Pass and fail criteria

A case passes only when all of these stages succeed:

1. Codex returns JSON matching the output schema.
2. `metadata_checker.py` accepts every parameter and generates the native graph.
3. The reference graph compiles and simulates.
4. The metadata-generated UUT graph compiles and simulates.
5. The existing DSPLib comparison reports matching outputs.
6. The status file reports `COMPILE: 1`, `SIM: 1`, and `FUNC: 1`.

Failures and generated artifacts are stored under:

```text
results/batch_multi_params_<suite>_<timestamp>/<test-name>/
```

Inspect `logs/status_*.txt` first, followed by the per-test build log,
`AIECompiler.log`, `config.json`, and `codex_configuration_report.json`.

## Shared infrastructure extensions

This POC makes two backward-compatible additions to the common test scripts:

- `run_pack.mk` copies optional `run_codex_config.py`,
  `codex_output_schema.json`, and `prompts/` assets into an isolated result
  directory.
- `run_batch.sh` accepts `-uut_kernel <name>`. A parallel directory can therefore
  use the canonical `fir_sr_asym` metadata and status name instead of its
  directory basename. Existing tests retain the basename default.

## Verified result

The result below is from the original one-shot design, where Codex read raw
metadata files and produced a full configuration itself, validated only by
the later `metadata_checker.py` step. `run_codex_config.py` was subsequently
changed to drive `ip_config_wizard.py`'s `ParamWizard` one parameter per
Codex turn instead (see Flow above) -- re-run the smoke suite to confirm this
result still holds under the new flow.

The smoke case was run locally with the 2026.2 Vitis tools. Codex generated an
AIE1 `cint16`/`int16`, 32-tap, window-API configuration. Metadata validation and
native graph generation passed, both graphs compiled and simulated, and the
reference comparison completed with:

```text
COMPILE: 1
SIM:     1
FUNC:    1
```
