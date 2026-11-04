# Copyright (C) 2026, Advanced Micro Devices, Inc.
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
import json
import os
import numpy as np

# simplicity assumptions:
    # 1. Think in PLIO widths, not samples.
    # 2. Assume num output samples iteration to iteration is known and constant.
    # 3. Assume num input samples iteration to iteration is known and constant.

time_factor = {
    's': 1,
    'ms': 1e-3,
    'us': 1e-6,
    'ns': 1e-9,
    'ps': 1e-12
}

datatype_sizes = {  # in bytes
    'int8': 1,
    'uint8': 1,
    'cint8': 2,
    'int16': 2,
    'uint16': 2,
    'cint16': 4,
    'int32': 4,
    'uint32': 4,
    'cint32': 8,
    'bfloat16': 2,
    'cbfloat16': 4,
    'float': 4,
    'cfloat': 8
}

def round_sig_figs(value, sig_figs=4):
    """Rounds value to sig_figs significant figures, preserving the -1 (not-steady-state) sentinel."""
    if value == -1:
        return -1
    if value == 0:
        return 0.0
    return round(value, sig_figs - int(np.floor(np.log10(abs(value)))) - 1)


def parse_timestamps_from_file(filepath) -> list:
    """Parses timestamps from a file and returns them as a list of floats in seconds."""
    timestamps = []
    with open(filepath, 'r') as file:
        for line in file:
            if "T " in line:    # catches timestamps but ignores "TLAST"
                time, unit = line[2:].split()
                timestamps.append(float(time) * time_factor[unit])  # everything in seconds
    return timestamps


def parse_timestamps_from_files(dir, file_pattern) -> list[list]:
    """Factory function which parses timestamps from all files in a directory matching a given pattern."""
    timestamps = []
    for filename in sorted(os.listdir(dir)):    # first file is *_0_0.txt
        if file_pattern in filename:
            timestamps.append(parse_timestamps_from_file(os.path.join(dir, filename)))
    return timestamps


def infer_tstarts(timestamps: list, niter: int) -> list:
    """Infers TSTARTs from timestamps by assuming the same number of samples per iteration."""
    num_timestamps_per_iter = len(timestamps) // niter
    tstarts = []
    for i, timestamp in enumerate(timestamps):
        if i % num_timestamps_per_iter == 0:
            tstarts.append(timestamp)
    return tstarts


def infer_tlasts(timestamps: list, niter: int) -> list:
    """Infers TLASTs from timestamps by assuming the same number of samples per iteration."""
    num_timestamps_per_iter = len(timestamps) // niter
    tlasts = []
    for i, timestamp in enumerate(timestamps):
        if (i + 1) % num_timestamps_per_iter == 0:
            tlasts.append(timestamp)
    return tlasts


def parse_tstarts(filepath: str) -> list:
    """Parses implied TSTARTs from a file by using the TLAST flags to find the start of the next iteration."""
    with open(filepath, 'r') as file:
        lines = [line for line in file if "T" in line]  # only keep timestamps and TLAST flags.

    tstarts = []
    lines = lines[1:] + lines[:1]   # easier parsing logic if first line is moved to the end.
    for i, line in enumerate(lines):
        if "TLAST" in line:
            time, unit = lines[i+1][2:].split()
            tstarts.append(float(time) * time_factor[unit])  # everything in seconds
    tstarts = tstarts[-1:] + tstarts[:-1]  # rotate tstarts back so they are chronological.
    return tstarts


def parse_tlasts(filepath: str) -> list:
    """Parses TLAST timestamps from a file."""
    with open(filepath, 'r') as file:
        lines = [line for line in file if "T" in line]  # only keep timestamps and TLAST flags.

    tlasts = []
    for i, line in enumerate(lines):
        if "TLAST" in line:
            time, unit = lines[i-1][2:].split()
            tlasts.append(float(time) * time_factor[unit])  # everything in seconds
    return tlasts


def get_tstarts_from_files(dir: str, file_pattern: str, niter: int) -> list[list]:
    """Factory function which parses or infers TSTARTs from all files in a directory matching a given pattern."""
    tstarts_per_port = []
    for filename in sorted(os.listdir(dir)):    # first file is *_0_0.txt
        if file_pattern in filename:
            file_tstarts = parse_tstarts(os.path.join(dir, filename))
            if not file_tstarts:  # If no TSTARTs found, infer them
                timestamps = parse_timestamps_from_file(os.path.join(dir, filename))
                file_tstarts = infer_tstarts(timestamps, niter)
            tstarts_per_port.append(file_tstarts)
    return tstarts_per_port


def get_tlasts_from_files(dir: str, file_pattern: str, niter: int) -> list[list]:
    """Factory function which parses or infers TLASTs from all files in a directory matching a given pattern."""
    tlasts_per_port = []
    for filename in sorted(os.listdir(dir)):    # first file is *_0_0.txt
        if file_pattern in filename:
            file_tlasts = parse_tlasts(os.path.join(dir, filename))
            if not file_tlasts:  # If no TLASTs found, infer them
                timestamps = parse_timestamps_from_file(os.path.join(dir, filename))
                file_tlasts = infer_tlasts(timestamps, niter)
            tlasts_per_port.append(file_tlasts)
    return tlasts_per_port


def calc_fifo_latency(in_start_time_per_port: list[float], out_start_time_per_port: list[float]) -> float:
    """Calculates the first-port-in-to-first-port-out latency of an iteration, given input and output TSTARTs."""
    fifo_latency = np.inf
    for in_start_time in in_start_time_per_port:
        for out_start_time in out_start_time_per_port:
            latency = out_start_time - in_start_time
            if latency < fifo_latency:
                fifo_latency = latency
    return fifo_latency


def calc_filo_latency(in_start_time_per_port: list[float], out_start_time_per_port: list[float]) -> float:
    """Calculates the first-port-in-to-last-port-out latency of an iteration, given input and output TSTARTs."""
    filo_latency = 0
    for in_start_time in in_start_time_per_port:
        for out_start_time in out_start_time_per_port:
            latency = out_start_time - in_start_time
            if latency > filo_latency:
                filo_latency = latency
    return filo_latency


def calc_throughput(start_time, end_time, timestamps: list, plio_width_samples: int) -> float:
    """Calculates the throughput (in bytes) of an iteration."""
    timestamps_in_iter = [t for t in timestamps if start_time <= t < end_time]
    samples_per_iter = len(timestamps_in_iter) * plio_width_samples
    throughput = samples_per_iter / (end_time - start_time)  # samples per second
    return throughput


def calc_cum_throughput(start_time_per_port, end_time_per_port, timestamps_per_port: list[list], plio_width_samples: int) -> float:
    """Calculates the cumulative throughput (in bytes) of an iteration across all ports."""
    cum_throughput = 0
    for i in range(len(timestamps_per_port)):
        throughput = calc_throughput(start_time_per_port[i], end_time_per_port[i], timestamps_per_port[i], plio_width_samples)
        cum_throughput += throughput
    return cum_throughput


def calc_frames_per_second(iteration_time, frames_per_iteration) -> float:
    """Calculates the frames per second of an iteration."""
    return frames_per_iteration / iteration_time


def window_steady_state(window: list, window_size=4, cv_threshold=0.02, slope_threshold=0.008):
    """Returns -1 if not steady state, otherwise returns the steady-state value as the window average.
    Steady state is determined according to 2 critieria:
    1. The coefficient of variation (CV) of the window must be below cv_threshold.
    2. The slope of the window must be below slope_threshold (if supplied)."""
    if len(window) != window_size:
        return -1  # not enough data to determine steady state.
    
    if cv_threshold:
        mean = np.mean(window)
        std_dev = np.std(window)
        cv = std_dev / mean
        if cv >= cv_threshold:
            return -1  # not steady state.
    if slope_threshold:
        window_log = np.log(window) # standardizes scale.
        slope = np.polyfit(range(window_size), window_log, 1)[0]
        if abs(slope) >= slope_threshold:
            return -1  # not steady state.
    return np.mean(window)  # steady state found.


def harvest_qor(aiesimulator_output_dir,
                niter,
                frames_per_iteration: int,
                input_pattern:str,
                output_pattern:str,
                plio_width_bits: int,
                out_datatype: str,
                steady_state_window_size: int,
                cv_threshold: float,
                slope_threshold: float,
                verbose_path: str) -> dict:

    plio_width_samples = plio_width_bits // 8 // datatype_sizes[out_datatype]
    ss_window_size = steady_state_window_size

    in_timestamps_per_port = parse_timestamps_from_files(aiesimulator_output_dir, input_pattern) if input_pattern else []
    out_timestamps_per_port = parse_timestamps_from_files(aiesimulator_output_dir + "/data", output_pattern)

    in_tstarts = np.array(get_tstarts_from_files(aiesimulator_output_dir, input_pattern, niter) if input_pattern else [])
    out_tstarts = np.array(get_tstarts_from_files(aiesimulator_output_dir + "/data", output_pattern, niter))
    out_tlasts = np.array(get_tlasts_from_files(aiesimulator_output_dir + "/data", output_pattern, niter))

    in_tstarts = in_tstarts.T # first index is iteration, second is port
    out_tstarts = out_tstarts.T # first index is iteration, second is port
    out_tlasts = out_tlasts.T # first index is iteration, second is port
    fenceposts = np.concatenate((np.zeros_like(out_tlasts)[:1], out_tlasts), axis=0) # first fencepost initialized as 0.
    
    _0_0_latencies = [] # the latency from the first sample of in_0_0 to the first sample of out_0_0.
    crit_latencies = [] # the slowest latency from the first sample of any port to the last sample of any port.
    initiation_intervals = []   # the time between the first sample of one iteration and the first sample of the next iteration.
    cum_throughputs = []    # the cumulative throughput across all ports, in bytes per second.
    frames_per_second = []
    
    _0_0_latency_steady_state_progression = []
    crit_latency_steady_state_progression = []
    initiation_interval_steady_state_progression = []
    cum_throughput_steady_state_progression = []
    frames_per_second_steady_state_progression = []

    for iter in range(niter):
        _0_0_latency = -1 if not input_pattern else out_tstarts[iter,0] - in_tstarts[iter,0]
        crit_latency = -1 if not input_pattern else calc_filo_latency(in_tstarts[iter,:], out_tstarts[iter,:])
        initiation_interval = max(fenceposts[iter+1] - fenceposts[iter])
        cum_throughput = calc_cum_throughput(fenceposts[iter,:], fenceposts[iter+1,:], out_timestamps_per_port, plio_width_samples)
        fps = calc_frames_per_second(initiation_interval, frames_per_iteration)

        _0_0_latencies.append( -1 if not input_pattern else _0_0_latency * 1e6 )        # seconds -> microseconds
        crit_latencies.append( -1 if not input_pattern else crit_latency * 1e6 )        # seconds -> microseconds
        initiation_intervals.append( initiation_interval * 1e6 )  # seconds -> microseconds
        cum_throughputs.append( cum_throughput / 1e6 )     # Sa/s -> MSa/s
        frames_per_second.append( fps / 1e3 )              # Hz -> kHz

        _0_0_latency_steady_state = -1 if not input_pattern else window_steady_state(_0_0_latencies[-ss_window_size:], ss_window_size, cv_threshold, slope_threshold)
        crit_latency_steady_state = -1 if not input_pattern else window_steady_state(crit_latencies[-ss_window_size:], ss_window_size, cv_threshold, slope_threshold)
        initiation_interval_steady_state = window_steady_state(initiation_intervals[-ss_window_size:], ss_window_size, cv_threshold, slope_threshold)
        cum_throughput_steady_state = window_steady_state(cum_throughputs[-ss_window_size:], ss_window_size, cv_threshold, slope_threshold)
        frames_per_second_steady_state = window_steady_state(frames_per_second[-ss_window_size:], ss_window_size, cv_threshold, slope_threshold)

        _0_0_latency_steady_state_progression.append( _0_0_latency_steady_state )
        crit_latency_steady_state_progression.append( crit_latency_steady_state )
        initiation_interval_steady_state_progression.append( initiation_interval_steady_state )
        cum_throughput_steady_state_progression.append( cum_throughput_steady_state )
        frames_per_second_steady_state_progression.append( frames_per_second_steady_state )

    verbose_metrics = { # Reporting the steady state metrics.
        "raw _0_0_latency (us)" : _0_0_latencies,
        "raw critical Latency (us)" : crit_latencies,
        "raw initiation interval (us)" : initiation_intervals,
        "raw throughput (MSa/s)" : cum_throughputs,
        "raw frame rate (kHz)" : frames_per_second,

        "steady _0_0_latency (us)" : _0_0_latency_steady_state_progression,
        "steady critical Latency (us)" : crit_latency_steady_state_progression,
        "steady initiation interval (us)" : initiation_interval_steady_state_progression,
        "steady throughput (MSa/s)" : cum_throughput_steady_state_progression,
        "steady frame rate (kHz)" : frames_per_second_steady_state_progression,
    }

    if verbose_path:
        verbose_metrics_formatted = {
            k: [round_sig_figs(x) if x != -1 else -1 for x in v]
            for k, v in verbose_metrics.items()
        }
        with open(verbose_path, 'w') as f:
            json.dump(verbose_metrics_formatted, f, indent=4)

    status_metrics = {
        "first shortest latency (us)": round_sig_figs(_0_0_latencies[0]),
        "first critical latency (us)": round_sig_figs(crit_latencies[0]),
        "steady shortest latency (us)": round_sig_figs(_0_0_latency_steady_state_progression[-1]),
        "steady critical latency (us)": round_sig_figs(crit_latency_steady_state_progression[-1]),
        "initiation interval (us)": round_sig_figs(initiation_interval_steady_state_progression[-1]),
        "throughput (MSa/s)": round_sig_figs(cum_throughput_steady_state_progression[-1]),
        "frame rate (kHz)": round_sig_figs(frames_per_second_steady_state_progression[-1])
    }
    return status_metrics

    