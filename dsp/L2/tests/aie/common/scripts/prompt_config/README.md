# Prompt-driven AIE IP tests

A prompt-driven test case configures an IP from a natural-language request instead of from
explicit graph parameters. The configuration is chosen at test time by an LLM (Codex), driving
`L2/meta/scripts/ip_config_wizard/ip_config_wizard.py` one parameter per turn. The IP's own test
harness (reference model, input generation, `uut_config.h`, diff, status) then runs on the
result, unchanged.

## Regression isolation

An LLM API token is user specific and is not available in Jenkins. **Prompt-driven tests are
never part of the regressions** defined by `description.json`:

- They may only be written in `test_suites/prompt*.txt`. `create_params.py` rejects
  `PROMPT_FILE` in any other suite, and always in the Jenkins `multi_params.json`, which is built
  from `checkin.txt` + `qor.txt` only.
- Test names end in `_prompt`, which matches none of the `testinfo.param_set` patterns.
- If no Codex binary is found, `run_prompt_config.py` fails the test with an explicit message
  rather than passing silently.

## Running

```bash
source ~/dsplib_env.csh
setenv CODEX_BIN <path to codex>   # a shell alias is not visible to make recipes
cd L2/tests/aie/<ip>
../common/scripts/runmake.sh -run_type prompt -nolsf
```

`-nolsf` keeps the run on the local machine, where the user's Codex credentials are.
`CODEX_MODEL` (environment variable or a test-case key) pins a model; if it is empty, Codex uses
its configured default.

## Writing test cases

`test_suites/prompt.txt` uses the normal suite syntax plus `PROMPT_FILE`, a file name under the
IP's `prompts/` directory:

```text
make all AIE_VARIANT=1 PROMPT_FILE=basic_aie1.txt TARGET=x86sim
```

`AIE_VARIANT` selects the platform, as it does for any other test, and is pinned. The LLM is not
asked for it. Every other graph parameter written on the line is only a placeholder that the
prompt overwrites. It has to be legal just for `create_params.py` validation, which pads missing
values with the IP's updaters. Harness-only keys (NITER, STIM_TYPE, DIFF_TOLERANCE, ...) come from
`default_params.txt` unless they are set on the line.

## Flow (pre_build, UUT build only)

1. `params.mk` reads `PROMPT_FILE` from the test case. If it is set, it takes precedence over
   `QOR_HELPER_CONSTRAINTS_PATH_OVERRIDE`, which is cleared with an INFO message. This disables
   the QoR helper recommendation and prediction check for that test.
2. `run_prompt_config.py --ip <ip>`:
   - Walks `ParamWizard(<ip>)`. Scalar parameters are answered by one `codex exec` turn each
     (`turn_instruction_template.txt` + the prompt + the parameters decided so far + the
     wizard's explanation/legal range). Each answer is validated immediately. The first rejected
     answer fails the test.
   - Vector parameters (FIR `coeff`, `fft_window` `weights`, `func_approx` `lookup_values`) are
     auto-filled. The harness generates its own stimulus, so an LLM answer would not be used.
   - Translates the graph parameters to testbench keys through
     `L2/meta/scripts/qor_helper/data/config_translation.json`, restricted to keys the test case
     has. It then rewrites the test case in the result directory's multi_params file, as
     `apply_qor_helper_recommendation.py` does.
   - Writes `logs/prompt_config_report.json` and `logs/codex_responses/`.
3. The rest of pre_build (`create_config`, `metadata_checker.py`, input and reference generation)
   and the build/sim/diff run as for any other test case.

An IP can override the turn template by providing `prompts/turn_instruction_template.txt`.

## Adding the flow to an IP

Add this step to `pre_build` in both `description.json` and `Makefile`, directly after
`mkdir -p logs` (before any QoR-helper steps):

```text
$(if $(PROMPT_FILE), $(if $(filter-out REF,$(TAG)), $(VITIS_PYTHON3) LIB_DIR/L2/tests/aie/common/scripts/prompt_config/run_prompt_config.py --ip <ip> --params_file $(PARAMS_FILE) --instance $(PARAMS) --prompt_file $(PROMPT_FILE) --prompt_dir PROJECT/prompts --lib_dir LIB_DIR --out_dir PROJECT/logs))
```

In the Makefile, `LIB_DIR` is `$(XFLIB_DIR)` and `PROJECT` is `$(CUR_DIR)`. Then add
`prompts/*.txt` and `test_suites/prompt.txt`.
