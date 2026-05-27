### Overview
The QoR Helper returns a user a set of configs according to their constraints, ranked by resource use (NUM_AIE). QoR for a config is predicted using a machine learned model.

There are two types of constraints to specify:
- parameter_constraints = constraints on an IP's configuration parameters.
- qor_constraints = constraints on QoR metrics (throughput, latency, num_aie, num_banks)

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

### Constraints Breakdown
Constraints files must be in the following json format:

```
{
    "parameter_constraints": {
        AIE_VARIANT: 1
        TT_DATA: ["float", "int32"]
        TP_DIM: [32, 64, 256]
        TP_SSR: []
    }
    "qor_constraints": "Throughput>300 and Latency<10000",
    "sort_by": ["num_aie", "throughput"],
    "ascending": [true, false],
    "confidence": 2
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

QoR constraints are in the form of a pandas query.
The configs returned are according to your query. That is to say, if no configs meet your QoR constraints you will receive back an empty set.
Example of some queries are below:

- "Throughput>300 and Latency<10000" - Return all configs with predicted throughputs greater than 300 (MSa/s) and less than 10000 (ns).
- "NUM_AIE<5" - Return all configs that use less than 5 AIE tiles.

Note: qor_constraints are case insensitive, however they must follow the schema of "```parameter``` ```conditional operator``` ```value```" e.g. ```throughput>500 and latency<10000```. The only conditional operators supported are ```<``` and ```>```, and the only logical operators supported are ```and``` and ```or```.

Multi-stage sorts are supported, which ranks the filtered configs according to the specified hierarchy, and in ascending or descending order. Examples of this use are in the breakdown above.

The model returns predictions in the form of a range. Confidence intervals can be adjusted using the "confidence" tuner, which sets the limits of the range in the form of standard deviations. The model's uncertainty is assumed to obey a normal distribution, and therefore a confidence of 1 (corresponding to 1 standard deviation) means there is a ~68% probability that the true value is within the predicted range. A confidence of 2 (2 standard deviations) means there is a ~95% probability the true value is within the specified range.

### Using an AI Assistant Workflow
You can use an AI coding assistant as a front-end tool for this helper.
Provide design requirements in plain language, ask the tool to update a constraints file,
run `config_qor_helper.py`, and return top-ranked configs.

Recommended workflow:
1. Define design requirements in terms of throughput, latency, resource budget, and fixed parameters.
2. Ask the tool to map those requirements into `parameter_constraints` and `qor_constraints`.
3. Ask the tool to set `sort_by` and `ascending` so ranking matches your priority (for example, fastest first).
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
2) Set sort_by and ascending to prioritize throughput first, then num_aie.
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
- Prefer fastest config; tie-break by lower num_aie

Actions:
1) Update constraints/fir_sr_asym_constraints.json accordingly.
2) Use qor_constraints on throughput/latency as needed.
3) Set sort_by to ["throughput", "num_aie"] and ascending to [false, true].
4) Setup Vitis environment with: module load xilinx/ta/2026.1_daily_latest; source $XILINX_VITIS/settings64.sh; export PYTHON_DIR=$(find "$XILINX_VITIS/tps/lnx64" -maxdepth 1 -type d -name "python-3*" | head -n 1); export PATH="$PYTHON_DIR/bin:$PATH"
5) Run config_qor_helper.py and write output to /tmp/fir_qor.csv
6) Show the top-ranked result and confidence ranges.
```