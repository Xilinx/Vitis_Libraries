#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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
import sys
import re
import os

def extract_timestamp(line):
    # Assuming format like "[ number unit ]"
    # Extract the number and unit
    match = re.search(r'\[\s*(\d+)\s+(\w+)\s*\]', line)
    if match:
        ts_val = int(match.group(1))
        unit = match.group(2)
        # Convert to ps
        if unit == 'ps':
            ts_ps = ts_val
        elif unit == 'ns':
            ts_ps = ts_val * 1000
        elif unit == 'us':
            ts_ps = ts_val * 1000000
        elif unit == 'ms':
            ts_ps = ts_val * 1000000000  # 1 ms = 1e9 ps
        else:
            print(f"Unknown unit: {unit}, assuming ps")
            ts_ps = ts_val
        return ts_ps, unit
    return None, None

def main():
    if len(sys.argv) != 7:
        print("Usage: python script.py <folder_path> <SSR> <point_size> <datawidth> <niter> <results_file>")
        sys.exit(1)
    
    folder_path = sys.argv[1]
    ssr = int(sys.argv[2])
    point_size = int(sys.argv[3])
    datawidth = int(sys.argv[4])
    niter = int(sys.argv[5])
    results_file = sys.argv[6]
    
    input_file = os.path.join(folder_path, "sim", "behav_waveform", "xsim", "aie_log", "S00_AXIS.log")
    output_log_file = os.path.join(folder_path, "sim", "behav_waveform", "xsim", "aie_log", "M00_AXIS.log")
    
    try:
        with open(input_file, 'r') as f:
            input_lines = f.readlines()
    except FileNotFoundError:
        print(f"Input file not found: {input_file}")
        sys.exit(1)
    
    try:
        with open(output_log_file, 'r') as f:
            output_lines = f.readlines()
    except FileNotFoundError:
        print(f"Output log file not found: {output_log_file}")
        sys.exit(1)
    
    # Find the first line that contains "[ number unit ]" in input file
    first_line_num = None
    for i, line in enumerate(input_lines, start=1):
        if re.search(r'\[\s*\d+\s+\w+\s*\]', line):
            first_line_num = i
            break
    
    if first_line_num is None:
        print("No line with [number unit] format found in input file.")
        sys.exit(1)
    
    # Calculate the offset per iteration
    offset_per_iter = (point_size / ssr) / (128 / datawidth)
    
    start_times = []
    output_times = []
    latencies = []
    for iter_num in range(1, niter + 1):
        line_num = int(first_line_num + (iter_num - 1) * offset_per_iter)
        if line_num < 1 or line_num > len(input_lines):
            print(f"Line number {line_num} for iteration {iter_num} is out of range in input file.")
            sys.exit(1)
        if line_num > len(output_lines):
            print(f"Line number {line_num} for iteration {iter_num} is out of range in output file.")
            sys.exit(1)
        
        input_line = input_lines[line_num - 1].strip()
        output_line = output_lines[line_num - 1].strip()
        
        input_ts_ps, _ = extract_timestamp(input_line)
        output_ts_ps, _ = extract_timestamp(output_line)
        
        if input_ts_ps is None:
            print(f"Could not extract timestamp from input line {line_num} for iteration {iter_num}.")
            sys.exit(1)
        if output_ts_ps is None:
            print(f"Could not extract timestamp from output line {line_num} for iteration {iter_num}.")
            sys.exit(1)
        
        start_times.append(input_ts_ps)
        output_times.append(output_ts_ps)
        latency_ps = output_ts_ps - input_ts_ps
        latencies.append(latency_ps)
    
    # Calculate throughput for each iteration (starting from iteration 3, after first two)
    throughputs = []
    for i in range(2, len(start_times)):
        time_diff_ps = start_times[i] - start_times[i-1]
        if time_diff_ps == 0:
            print(f"Warning: Time difference is zero for iteration {i+1}")
            continue
        throughput_msps = point_size * 1e6 / time_diff_ps
        throughputs.append(throughput_msps)
    
    # Find the stable throughput value (within 5% of previous value)
    final_throughput = None
    stability_threshold = 0.05  # 5%
    
    if len(throughputs) < 3:
        print("Not enough iterations to calculate throughput.")
        sys.exit(1)
    
    # Start with the first calculated throughput
    final_throughput = throughputs[2]
    
    # Check each subsequent throughput for stability
    for i in range(3, len(throughputs)):
        current_throughput = throughputs[i]
        percent_diff = abs(current_throughput - final_throughput) / final_throughput
        
        if percent_diff <= stability_threshold:
            # Update final throughput if within 5%
            final_throughput = current_throughput
            final_latency = latencies[i]  
        else:
            # Stop at the first value that exceeds 5% difference
            break
    
    # Calculate average latency (from last 3 iterations if available)
    if len(latencies) < 3:
        avg_latency_ps = sum(latencies) / len(latencies)
    else:
        avg_latency_ps = sum(latencies[-3:]) / 3
    
    avg_latency_ns = avg_latency_ps / 1000  # Convert to ns
    
    # Write to results file
    try:
        with open(results_file, 'a') as f:
            f.write(f"Throughput: {final_throughput} MSa/s\n")
            f.write(f"Latency: {final_latency / 1000} ns\n")
    except Exception as e:
        print(f"Error writing to results file: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()