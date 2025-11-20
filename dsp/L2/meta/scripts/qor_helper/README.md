### Overview
The QoR Helper returns a user a set of configs according to their constraints, ranked by resource use (NUM_AIE). QoR for a config is predicted using a machine learned model.

There are two types of constraints to specify:
- parameter_constraints = constraints on an IP's configuration parameters.
- qor_constraints = constraints on QoR metrics (Throughput, Latency and NUM_AIE)

### Running config_qor_helper.py
Run the below instructions from this directory (./qor_helper/), and make sure your aie environment is active:
```
python config_qor_helper.py --ip fft_ifft_dit_1ch --constraints_file constraints/fft_ifft_dit_1ch_constraints.json
```
Further options can be viewed with the -h keyword.

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
    "qor_constraints": "Throughput>300 and Latency<10000"
}
```

Note: Parameters which are entered at the graph must be specified in the respective format (metadata level: ```DIM_SIZE```, graph level: ```TP_DIM```). Numeric types must be entered as such. AIE_VARIANT must be one of [1, 2, 22].

Parameter constraints can be one of the following:
 - fixed to a scalar value 
 - one of many values, bounded by [] 
 - one of any legal value, specified as empty brackets OR by ommitting the parameter from the constraints

It should be noted that certain downstream parameters have no impact on QoR and thus will be fixed to a default value
regardless of specification. These are TP_SHIFT, TP_RND, and TP_SAT.

A word of caution - the constraints can be as fixed or as loose as you like. This means that you may experience very
long wait times if you do not constrain adequately.

QoR constraints are in the form of a pandas query. 
The configs returned are according to your query. That is to say, if no configs meet your QoR constraints you will receive back an empty set.
Example of some queries are below:

- "Throughput>300 and Latency<10000" - Return all configs with predicted throughputs greater than 300 (MSa/s) and less than 10000 (ns).
- "NUM_AIE<5" - Return all configs that use less than 5 AIE tiles.
- "True" - Return all configs.