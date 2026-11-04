### Overview
The QoR Helper returns a user a set of configs according to their constraints, ranked by resource use (`num aie` by default). QoR for a config is predicted using a machine learned model.

There are two types of constraints to specify:
- parameter_constraints = constraints on an IP's configuration parameters.
- qor_constraints = constraints on QoR metrics (e.g. throughput, latency, frame rate, num aie — the exact set of supported QoR parameters is IP-specific, see the `dependent` list for the IP in `data/model_params.json`)

Supported models can be viewed from the "models" folder in this directory.

### Running config_qor_helper.py

#### Environment Setup
Before running the script, set up Vitis and Python. Choose one approach:

**Module-based setup (recommended):**
```bash
module load xilinx/ta/2026.1_daily_latest
source $XILINX_VITIS/settings64.sh  # or settings64.csh for tcsh
export PYTHON_DIR=$(find "$XILINX_VITIS/tps/lnx64" -maxdepth 1 -type d -name "python-3*" | head -n 1)
export PATH="$PYTHON_DIR/bin:$PATH"
```

**Direct path setup:**
```bash
source <path_to_vitis_install>/Vitis/settings64.sh
export PYTHON_DIR=$(find "$XILINX_VITIS/tps/lnx64" -maxdepth 1 -type d -name "python-3*" | head -n 1)
export PATH="$PYTHON_DIR/bin:$PATH"
```

Verify: `python -c "import pandas; print('Ready')"`

#### Running the Helper
From this directory (./qor_helper/), run:
```bash
python config_qor_helper.py --ip *ip_name* --constraints_file *constraints_filepath*
```
Further options can be viewed with: `python config_qor_helper.py -h`

An example can be ran as follows:
```bash
python config_qor_helper.py --ip fft_ifft_dit_1ch --constraints_file constraints/fft_ifft_dit_1ch_example.json
```

#### Generating Graphs
Pass `--out_graphs_dir *directory*` to write out the top-ranked configs as generated AIE graph code. `--num_graphs` controls how many of the top-ranked configs are generated (default 10).

### Constraints Breakdown
Constraints files must be in the following json format:

```json
{
    "parameter_constraints": {
        "AIE_VARIANT": 1,
        "TT_DATA": ["float", "int32"],
        "TP_DIM": [32, 64, 256],
        "TP_SSR": []
    },
    "qor_constraints": {
        "throughput (MSa/s)": ">300",
        "steady shortest latency (us)": "<10"
    },
    "sort_by": {
        "num aie": "ascending",
        "throughput (MSa/s)": "descending"
    },
    "confidence": 2,
    "verbose": true
}
```

Note: Parameters which are entered at the graph must be specified in the respective format (metadata level: ```DIM_SIZE```, graph level: ```TP_DIM```). Numeric types must be entered as such. AIE_VARIANT must be one of [1, 2, 22].

Parameter constraints can be one of the following:
 - fixed to a scalar value
 - one of many values, bounded by []
 - one of any legal value, specified as empty brackets OR by ommitting the parameter from the constraints

It should be noted that certain downstream parameters have no impact on QoR and thus will be fixed to a default value
regardless of specification. These are TP_SHIFT, TP_RND, and TP_SAT.

A word of caution - the constraints can be as fixed or as loose as you like. A maximum config limit is set by default to 100000, however this can be modified with the optional argument "--config_limit". You may experience very long wait times if you increase the config_limit and do not constrain adequately.

QoR constraints are specified as a JSON object, mapping each QoR parameter to a single condition string.
The configs returned must satisfy every all of the conditions. If no configs meet your QoR constraints, the script raises an error rather than returning an empty set.
Example of some constraints are below:

- `{"throughput (MSa/s)": ">300", "steady shortest latency (us)": "<10"}` - Return all configs with predicted throughput greater than 300 MSa/s and steady shortest latency less than 10 us.
- `{"num aie": "<5"}` - Return all configs that use less than 5 AIE tiles.

Note: parameter and qor constraint keys are case-sensitive (these can be referred to in data/model_params.json) Each condition must be a single conditional operator followed by a value, e.g. ```>500```. The only conditional operators supported are ```<``` and ```>```.

"sort_by" is a JSON object mapping each sort key to its order, either as the string `"ascending"`/`"descending"` or as a boolean (`true` = ascending, `false` = descending). Multi-stage sorts are supported: the filtered configs are ranked according to the order the keys are listed in, each with its own direction. If "sort_by" is omitted, it defaults to `{"num aie": "ascending"}`. Examples of this use are in the breakdown above.

The model returns predictions in the form of a range. Confidence intervals can be adjusted using the "confidence" tuner, which sets the limits of the range in the form of standard deviations. The model's uncertainty is assumed to obey a normal distribution, and therefore a confidence of 1 (corresponding to 1 standard deviation) means there is a ~68% probability that the true value is within the predicted range. A confidence of 2 (2 standard deviations) means there is a ~95% probability the true value is within the specified range. If "confidence" is omitted, it defaults to 1.

All models expose the following metrics, any of which can be referenced in `qor_constraints` or `sort_by`:
- `first shortest latency (us)` - latency from the first input sample to the first output sample, measured on the first iteration.
- `first critical latency (us)` - worst-case latency from the first input sample to the first output sample of the slowest output port, measured on the first iteration.
- `steady shortest latency (us)` - same as `first shortest latency (us)`, but measured once the design has reached a steady, repeating rate.
- `steady critical latency (us)` - same as `first critical latency (us)`, but measured once the design has reached a steady, repeating rate.
- `throughput (MSa/s)` - samples processed per second, in mega-samples/second.
- `frame rate (kHz)` - algorithm frames processed per second, in kHz.
- `num aie` - number of AIE tiles used by the config.

By default (or if "verbose" is set to `false`), only a reduced set of metrics is displayed for each returned config: `steady shortest latency (us)`, `throughput (MSa/s)`, `num aie`, plus any metrics referenced in `qor_constraints` or `sort_by`. Setting `"verbose": true` displays all model metrics for each config instead.

### Using an AI Assistant Workflow
You can use an AI coding assistant as a front-end tool for this helper.
Provide design requirements in plain language, ask the tool to update a constraints file,
run `config_qor_helper.py`, and return top-ranked configs.

Recommended workflow:
1. Define design requirements in terms of throughput, latency, resource budget, and fixed parameters.
2. Ask the tool to map those requirements into `parameter_constraints` and `qor_constraints`.
3. Ask the tool to set `sort_by` so ranking matches your priority (for example, fastest first).
4. Ask the tool to set up the Vitis environment (see "Running config_qor_helper.py" above) and run the helper from this directory.
5. Ask the tool to summarize top candidates from the output CSV and explain tradeoffs.

### Example Prompt: FFT Design
Use this prompt with your coding assistant:

```
I am in L2/meta/scripts/qor_helper.
Generate an FFT config recommendation using config_qor_helper.py.

Requirements:
- IP: fft_ifft_dit_1ch
- TT_OUT_DATA: cint16
- Minimum throughput: 1200 MSa/s
- Maximum latency: 30000 ns
- Prefer fewer AIEs after throughput target is met

Actions:
1) Update constraints/fft_ifft_dit_1ch_constraints.json with parameter_constraints and qor_constraints.
2) Set sort_by to {"throughput (MSa/s)": "descending", "num aie": "ascending"} to prioritize throughput first, then num aie.
3) Setup Vitis environment with: module load xilinx/ta/2026.1_daily_latest; source $XILINX_VITIS/settings64.sh; export PYTHON_DIR=$(find "$XILINX_VITIS/tps/lnx64" -maxdepth 1 -type d -name "python-3*" | head -n 1); export PATH="$PYTHON_DIR/bin:$PATH"
4) Run config_qor_helper.py and write output to fft_qor.csv
5) Print the top 10 rows and summarize best 3 options.
```

### Example Prompt: FIR Design
Use this prompt with your coding assistant:

```
I am in L2/meta/scripts/qor_helper.
Generate a FIR config recommendation using config_qor_helper.py.

Requirements:
- IP: fir_sr_asym
- TP_FIR_LEN: 64
- TT_DATA: cint16
- TT_COEFF: int16
- Minimum throughput: 1000 MSa/s
- Prefer fastest config; tie-break by lower num aie

Actions:
1) Update constraints/fir_sr_asym_constraints.json accordingly.
2) Use qor_constraints on throughput/latency as needed.
3) Set sort_by to {"throughput (MSa/s)": "descending", "num aie": "ascending"}.
4) Setup Vitis environment with: module load xilinx/ta/2026.1_daily_latest; source $XILINX_VITIS/settings64.sh; export PYTHON_DIR=$(find "$XILINX_VITIS/tps/lnx64" -maxdepth 1 -type d -name "python-3*" | head -n 1); export PATH="$PYTHON_DIR/bin:$PATH"
5) Run config_qor_helper.py and write output to /tmp/fir_qor.csv
6) Show the top-ranked result and confidence ranges.
```