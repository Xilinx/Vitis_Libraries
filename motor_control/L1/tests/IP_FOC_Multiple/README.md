# IP_FOC_Multiple validation

## Model waveform visualization

For `NUM_MOTORS=1`, model csim records `motor0_sim_foc_ModelFoc.log`.
Use the shared `L1/tests/Model/plot_model_foc.py` tool to render the closed
loop command voltage, feedback current, speed, and position waveforms.

Install `matplotlib` once in the local tools directory:

```bash
python3 -m pip install --target /wrk/xcohdnobkup1/ryanw/matplotlib matplotlib
export PYTHONPATH=/wrk/xcohdnobkup1/ryanw/matplotlib:${PYTHONPATH}
```

From this directory, render the single-motor ap_fixed waveform:

```bash
python3 ../Model/plot_model_foc.py \
  hls_review_model_m1/hls/csim/build/motor0_sim_foc_ModelFoc.log \
  --primary-label IP_FOC_Multiple-motor0
```

The default output is `motor0_sim_foc_ModelFoc_waveforms.png` in this test
directory. Use `--output <path>` to override it.

To compare a matching FP32 golden run with this ap_fixed run:

```bash
python3 ../Model/plot_model_foc.py \
  ../tests_fp32/foc_fp32_tb/hls_fp32_plot_fixed/hls/csim/build/motor0_fp32_sim_foc_ModelFoc.log \
  hls_review_model_m1/hls/csim/build/motor0_sim_foc_ModelFoc.log \
  --primary-label FP32-golden \
  --reference-label ap_fixed-motor0 \
  --output ../tests_fp32/foc_fp32_tb/fp32_vs_apfixed_waveforms.png
```

In dual-log mode, the primary and reference traces are overlaid and an
additional FOC output-voltage difference plot reports the maximum difference
per phase.
