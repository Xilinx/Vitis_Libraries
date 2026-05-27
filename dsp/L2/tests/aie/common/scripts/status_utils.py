#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2025, Advanced Micro Devices, Inc.
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
import os
import glob
import sys
import re
from pathlib import Path
from collections import defaultdict



def parse_aiesim_data(window_size, casc_len, out_status, file_dir, func_name="filter", num_iter=1):
    """
    Parses AIESimulator data from XML files and simulation summary to compute 
    cycle counts and throughput metrics.

    Args:
        window_size (int): Window size for computations.
        casc_len (int): Cascaded length.
        out_status (str): Output status file path.
        file_dir (str): Directory containing profile XML and summary files.
        func_name (str, optional): Name of the function to analyze. Defaults to "filter".
        num_iter (int, optional): Number of iterations for averaging. Defaults to 1.

    Returns:
        None: Writes computed values into the output status file.
    """

    def get_min(values):
        return min(values)

    def get_max(values):
        return max(values)

    def parse_xml_file(file_names, func_name):
        cycle_count_total = []
        cycle_count_min = []
        cycle_count_max = []
        cycle_count_avg = []
        initiation_interval_aprx = []
        kernel_stats_done = False

        str_lvl2main = "        <function_name>main"
        str_lvl2 = f"        <function_name>{func_name}"
        str_lvl3 = "        <function_and_descendants_time>"

        for file_name in file_names:
            if not kernel_stats_done:
                with open(file_name, 'r') as in_file:
                    for line in in_file:
                        if str_lvl2main in line:
                            for line in in_file:
                                if str_lvl3 in line:
                                    line = next(in_file)
                                    main_cycle_count_total = int(line.split('>')[1].split('<')[0])
                                    initiation_interval_aprx.append(main_cycle_count_total / num_iter)
                                    break

                        if str_lvl2 in line:
                            for line in in_file:
                                if str_lvl3 in line:
                                    line = next(in_file)
                                    cycle_count_total.append(int(line.split('>')[1].split('<')[0]))
                                    line = next(in_file)
                                    cycle_count_min.append(int(line.split('>')[1].split('<')[0]))
                                    line = next(in_file)
                                    cycle_count_max.append(int(line.split('>')[1].split('<')[0]))
                                    line = next(in_file)
                                    cycle_count_avg.append(int(line.split('>')[1].split('<')[0]))
                                    break
                            break
        return cycle_count_total, cycle_count_min, cycle_count_max, cycle_count_avg, initiation_interval_aprx

    def parse_simulation_summary(file_sim_summary):
        freq = 1000  # Default frequency
        for sim_summary in file_sim_summary:
            with open(sim_summary, 'r') as in_file:
                for line in in_file:
                    if "frequency" in line:
                        freq = int(line.split(":")[1].split(",")[0])
                        break
        return freq

    # Get AIESimulator File Names
    file_names = glob.glob(os.path.join(file_dir, 'profile_funct_*.xml'))
    file_sim_summary = glob.glob(os.path.join(file_dir, '*_summary'))

    # Parse XML Files
    cycle_count_total, cycle_count_min, cycle_count_max, cycle_count_avg, initiation_interval_aprx = parse_xml_file(file_names, func_name)

    # Parse Simulation Summary to get AIE Frequency
    aie_frequency = parse_simulation_summary(file_sim_summary)

    # Set Default Cycle Count if Empty
    if not cycle_count_avg:
        for _ in range(casc_len):
            cycle_count_total.append(-1)
            cycle_count_min.append(-1)
            cycle_count_max.append(-1)
            cycle_count_avg.append(-1)

    # Compute Throughput for each Kernel
    throughput_cycle_count = []
    throughput_ii_avg = []
    for x in range(casc_len):
        throughput_cycle_count.append(aie_frequency * window_size / cycle_count_avg[x])
        throughput_ii_avg.append(aie_frequency * window_size / initiation_interval_aprx[x])

    # Compute Throughput for the design
    max_des_cycle_count_total = get_max(cycle_count_total)
    min_des_cycle_count_min = get_min(cycle_count_min)
    max_des_cycle_count_max = get_max(cycle_count_max)
    max_des_cycle_count_avg = get_max(cycle_count_avg)
    max_des_initiation_interval = get_max(initiation_interval_aprx)

    # Similarly, get the lowest throughput figure.
    min_des_throughput_avg = get_min(throughput_cycle_count)
    min_des_throughput_i_i_avg = get_min(throughput_ii_avg)
    
    return max_des_cycle_count_avg, min_des_throughput_avg
                       

def find_TLAST_in_file(t_out_file_path):
    try:
        with open(t_out_file_path, 'r') as file:
            for line in file:
                if "TLAST" in line:
                    return True
        return False
    except FileNotFoundError:
        print(f"Output File not found: {t_out_file_path}")
        return False
    

def extract_output_tlast_and_tfirst_values_with_units(file_path):
    """
    Extracts TLAST and TFIRST values from the output files, converts them to numeric values 
    (in picoseconds for consistency), and returns them as separate lists.
    """
    with open(file_path, 'r') as file:
        lines = file.readlines()

    tlast_indexes = []
    tlast_values = []
    tfirst_values = []

    # Extract the absolute first T_FIRST from the first timestamp in the file
    first_line = lines[0].strip()
    if first_line.startswith("T "):
        first_value = first_line[2:]
        tfirst_values.append(process_timestamp(first_line)) # Convert to numeric value in picoseconds

    # Find indices of lines containing 'TLAST'
    for index, line in enumerate(lines):
        if 'TLAST' in line:
            tlast_indexes.append(index - 1)  # Get index of the line before TLAST

    # Process TLAST values and find TFIRST values for each
    for i in range(len(tlast_indexes)):
        # Process TLAST
        tlast_index = tlast_indexes[i]
        tlast_value = lines[tlast_index].strip()

        # Strip off the leading "T "
        if tlast_value.startswith("T "):
            tlast_value = tlast_value[2:]
        tlast_values.append(process_timestamp(tlast_value))

        # Find TFIRST after TLAST
        tfirst_index = tlast_index + 1
        while tfirst_index < len(lines):
            tfirst_value = lines[tfirst_index].strip()
            if tfirst_value.startswith("T "):
                tfirst_value = tfirst_value[2:]
                tfirst_values.append(process_timestamp(tfirst_value))
                break
            tfirst_index += 1

    return tfirst_values, tlast_values 

def convert_time(value, from_unit, to_unit):
    # Conversion factors relative to seconds
    conversion_factors = {
        's': 1,
        'ms': 1e-3,
        'us': 1e-6,
        'ns': 1e-9,
        'ps': 1e-12
    }
    
    if from_unit not in conversion_factors or to_unit not in conversion_factors:
        raise ValueError("Invalid unit provided. Please use: 's', 'ms', 'us', 'ns', 'ps'")
    
    # Convert the value to seconds first
    value_in_seconds = value * conversion_factors[from_unit]
    
    # Convert from seconds to the desired unit
    converted_value = value_in_seconds / conversion_factors[to_unit]
    
    return converted_value

def process_timestamp(timestamp_line):
    timestamp = timestamp_line.strip('T').strip()
    if timestamp.endswith(' ms'):
        value = int(timestamp[:-3].strip()) * 1e9  # Convert nanoseconds to picoseconds
    elif timestamp.endswith(' us'):
        value = int(timestamp[:-3].strip()) * 1e6  # Convert nanoseconds to picoseconds
    elif timestamp.endswith(' ns'):
        value = int(timestamp[:-3].strip()) * 1e3  # Convert nanoseconds to picoseconds
    else:
        value = int(timestamp[:-3].strip())  # It's already in picoseconds

    unit = 'ps'
    return value, unit
    
def extract_timestamps_from_file_with_units(file_path, niter):
    """ Extracts TFIRST and TLAST values from the input file, converts them to numeric values (in picoseconds for consistency)."""
    
    t_first_values = []
    t_last_values = []

    with open(file_path, 'r') as file:
        lines = file.readlines()

    # Extract timestamps and data lines
    timestamps = [process_timestamp(line) for line in lines if line.startswith('T')]
    data_lines = [line for line in lines if not line.startswith('T')]

    # Total number of data samples and chunk size calculation
    total_data_samples = len(data_lines)
    chunk_size = total_data_samples // niter
    remainder = total_data_samples % niter

    for i in range(niter):
        start_idx = i * chunk_size
        end_idx = start_idx + chunk_size + (1 if i < remainder else 0)  # distribute the remainder

        # Access timestamps for the actual data range
        if start_idx < len(timestamps):
            t_first_values.append(timestamps[start_idx])
        
        # Adjust end_idx to fetch the correct last timestamp
        if end_idx < len(timestamps):
            t_last_values.append(timestamps[end_idx - 1])
        else:
            t_last_values.append(timestamps[-1])  # Ensure the last value is always within bounds

    return t_first_values, t_last_values


def calc_latency(tfirst_values_in, tfirst_values_out):
    """
    Calculate the latency from the input file and write it to the output file.

    """

    if len(tfirst_values_in) != len(tfirst_values_out):
        print("Calc Latency Error: Mismatch between number of TFIRST values of Input and Output files.")
        return [], ""
    latencies = []
    
    for i in range(len(tfirst_values_in)):
        tfirst_numeric_in, unit_in = tfirst_values_in[i]
        tfirst_numeric_out, unit_out = tfirst_values_out[i]
        if unit_in != unit_out:
            print(f"Calc Latency Error: Unit mismatch between input and output TFIRST values: {unit_in} vs {unit_out}")
            return [], ""
        latency = tfirst_numeric_out - tfirst_numeric_in
        latencies.append(latency)
    return latencies, unit_in

def calc_initiation_intervals(t_values):
    '''Calculate the initiation intervals (update rates) in Hz from the TFIRST values.'''
    # Calculate the differences and update rate in Hz
    intervals = [0] # First interval is set to 0 as there is no previous timestamp to compare with
    
    # Calculate difference in picoseconds and convert to update rate in Hz
    for i in range(1, len(t_values)):
        tlast_prev, unit_prev = t_values[i-1]
        tfirst_current, unit_current = t_values[i]
        if unit_prev != unit_current:
            print(f"Calc Initation Interval Error: Unit mismatch between consecutive TFIRST values: {unit_prev} vs {unit_current}")
            return [], ""
        time_diff_ps = tfirst_current - tlast_prev

        # Calculate update rate in Hz
        if time_diff_ps > 0:
            intervals.append(time_diff_ps*1e-3)  # Convert picoseconds to nanoseconds for update rate calculation
    unit_intervals = "ns"
    return intervals, unit_intervals

def calc_throughput(t_values, num_samples):

    intervals, unit_intervals = calc_initiation_intervals(t_values) #these are not initation intervals, they are being measured at the output time samples, because throughput is defined at the output!
    if unit_intervals == "ps": #intervals are in ns, but this code is here in case if we want to report things in different units.
        intervals = [interval * 1e-6 for interval in intervals]
    if unit_intervals == "ns":
        intervals = [interval * 1e-3 for interval in intervals]
    elif unit_intervals == "us":
        intervals = intervals
    elif unit_intervals == "ms":
        intervals = [interval * 1e3 for interval in intervals]
    elif unit_intervals == "s":
        intervals = [interval * 1e6 for interval in intervals]

    unit = "MSa/s" # intervals are now in us, so throughput will be in MSa/s

    if not intervals:
        return 0.0
    throughputs=[]
    for interval in intervals:
        if interval <= 0:
            throughputs.append(0)      
        else:
            throughput = (1/interval) * num_samples
            throughputs.append(throughput)
    return throughputs, unit

def calc_average_all(values, qor="Latency", unit="ps"):
    # Calculate the average of the matching values
    average_value = sum(values) / len(values)
    return average_value, unit, len(values)


def calc_average(values, qor="Latency", unit="ps", required_it_num=4, required_matches=2, tolerance_percentage=5):
    """
    Calculates the average of the last consecutive matching values within a tolerance
    percentage such that they occur at the end of the list, and returns the actual number
    of these matching values found.
    """

    if not values or (required_matches <= 0) or (len(values) < required_it_num):
        print(f"Calc Average {qor} Error: Not enough samples to calculate average or invalid required matches.")   
        return -1, unit, 0
    
    matching_values = []  # To store the matching values
    matched_samples = 0
    
    # Check for matching consecutive values starting from the end
    values_reversed = list(reversed(values))
    final_value = values_reversed[0]
    for value in values_reversed:
        if abs(final_value - value) / final_value < tolerance_percentage / 100:
            matching_values.append(value)
            matched_samples += 1
        else:
            break
    
    # If the required number of matches is not met, return (None, 0)
    if len(matching_values) < required_matches:
        print(f"Calc Average {qor} Error: Not enough matching samples found. Required: {required_matches}, Found: {len(matching_values)}")
        return -1, unit, matched_samples
    
    # Calculate the average of the matching values
    average_value = sum(matching_values) / len(matching_values)
    return average_value, unit, matched_samples


def set_unit(value, unit):
    
    if value >= 1_000_000.0:
        converted_value = round(value / 1_000_000.0, 1)
        if unit == 'Hz':
            unit = 'MHz'
        elif unit == 'Sa/s':
            unit = 'MSa/s'
        elif unit == 'ps':
            unit = 'us'

        return int(converted_value*100)/100, unit
    
    elif value >= 1000.0:
        converted_value = round(value / 1000.0, 1)
        if unit == 'Hz':
            unit = 'kHz'
        elif unit == 'Sa/s':
            unit = 'kSa/s'
        elif unit == 'ps':
            unit = 'ns'
        
        return int(converted_value*100)/100, unit 

    return int(value*100)/100, unit


def fix_unit(value, unit):
    
    if unit == 'Hz':
        converted_value = value / 1000
        unit = 'kHz'
    elif unit == 'kHz':
        converted_value = value
        unit = 'kHz'
    elif unit == 'MHz':
        converted_value = value * 1000
        unit = 'kHz'

    if unit == 'Sa/s':
        converted_value = value / 1000000
        unit = 'MSa/s'
    elif unit == 'kSa/s':
        converted_value = value / 1000
        unit = 'MSa/s'
    elif unit == 'MSa/s':
        converted_value = value
        unit = 'MSa/s'    


    if unit == 'ps':
        converted_value = value / 1000
        unit = 'ns'
    elif unit == 'ns':
        converted_value = value
        unit = 'ns' 
    elif unit == 'us':
        converted_value = value * 1000          
        unit = 'ns'

    if value == -1:
        converted_value = -1
    else: 
        converted_value = int(converted_value*100)/100
    return converted_value, unit


def get_num_banks(loc, dummy=None):
    """
    Calculate the number of memory banks from the mapping analysis report.
    
    Args:
        loc: Location of the Work directory
        dummy: Unused parameter (for compatibility with shell script)
        
    Returns:
        int: Number of banks
    """
    num_banks = 0
    target_fname = "*mapping_analysis_report.txt"
    
    # Find the mapping analysis report file
    report_path = os.path.join(loc, "reports", target_fname)
    report_files = glob.glob(report_path)
    
    if not report_files:
        return num_banks
    
    report_file = report_files[0]
    
    try:
        with open(report_file, 'r') as f:
            content = f.read()
        
        # Extract Memory Bank Report section
        if "Memory Bank Report" not in content:
            return num_banks
            
        # Find the section after "Memory Bank Report"
        mem_bank_section = content.split("Memory Bank Report")[1]
        
        # Extract lines starting with MG(
        mg_lines = []
        for line in mem_bank_section.split('\n'):
            if line.strip().startswith('MG('):
                mg_lines.append(line)
        
        # Skip first 2 lines (header lines)
        mg_lines = mg_lines[2:] if len(mg_lines) > 2 else []
        
        # Process the lines
        entries = []
        for line in mg_lines:
            # Squeeze multiple spaces
            parts = re.sub(r'\s+', ' ', line.strip()).split(' ')
            if len(parts) >= 4:
                # Extract columns 1, 2, and 4 (0-indexed: 0, 1, 3)
                entry = (parts[0], parts[1], parts[3])
                entries.append(entry)
        
        # Sort and get unique entries
        unique_entries = list(set(entries))
        
        # Group by first column (MG location)
        mg_groups = defaultdict(list)
        for entry in unique_entries:
            if entry[0]:  # Skip empty entries
                mg_groups[entry[0]].append(int(entry[2]) if entry[2].isdigit() else 0)
        
        # Calculate number of banks for each MG
        for mg_name, sizes in mg_groups.items():
            total_size = sum(sizes)
            # Calculate banks: (8191 + total_size) / 8192
            nb = (8191 + total_size) // 8192
            num_banks += nb
            
    except Exception as e:
        print(f"Error processing {report_file}: {e}", file=sys.stderr)
    
    return num_banks


def get_num_me(loc, short=1):
    """
    Count the number of memory elements (ME cores) used.
    
    Args:
        loc: Location of the Work directory
        short: If 0, use 8x8 grid; if 1, use larger grid (0-49)
        
    Returns:
        int: Number of ME cores
    """
    num_me_cores = 0
    target_fname = "*mapping_analysis_report.txt"
    
    # Find the mapping analysis report file
    report_path = os.path.join(loc, "reports", target_fname)
    report_files = glob.glob(report_path)
    
    if not report_files:
        return num_me_cores
    
    report_file = report_files[0]
    
    # Define row/col list based on short parameter
    if short == 0:
        rowcol_list = list(range(8))
    else:
        rowcol_list = list(range(50))
    
    try:
        with open(report_file, 'r') as f:
            content = f.read()
        
        # Extract section before "Port Mapping Report"
        if "Port Mapping Report" in content:
            section = content.split("Port Mapping Report")[0]
        else:
            section = content
        
        # Count cores used
        for row in rowcol_list:
            for col in rowcol_list:
                # Search for CR(row,col) pattern
                pattern = rf'CR\({row},{col}\)'
                matches = re.findall(pattern, section)
                
                if len(matches) > 0:
                    num_me_cores += 1
                    
    except Exception as e:
        print(f"Error processing {report_file}: {e}", file=sys.stderr)
    
    return num_me_cores


def get_num_cores(loc):
    """
    Read the total number of cores from complexity.csv.
    
    Args:
        loc: Location of the Work directory
        
    Returns:
        int: Total number of cores
    """
    complexity_file = os.path.join(loc, "reports", "complexity.csv")
    total_num_cores = 0
    
    try:
        with open(complexity_file, 'r') as f:
            for line in f:
                if 'total_num_cores' in line:
                    # Extract value after the comma
                    parts = line.strip().split(',')
                    if len(parts) >= 2:
                        total_num_cores = int(parts[1])
                    break
    except Exception as e:
        print(f"Error reading {complexity_file}: {e}", file=sys.stderr)
    
    return total_num_cores


def get_num_tiles(loc):
    """
    Read the total number of tiles from complexity.csv.
    
    Args:
        loc: Location of the Work directory
        
    Returns:
        int: Total number of tiles
    """
    complexity_file = os.path.join(loc, "reports", "complexity.csv")
    total_num_tiles = 0
    
    try:
        with open(complexity_file, 'r') as f:
            for line in f:
                if 'total_num_tiles' in line:
                    # Extract value after the comma
                    parts = line.strip().split(',')
                    if len(parts) >= 2:
                        total_num_tiles = int(parts[1])
                    break
    except Exception as e:
        print(f"Error reading {complexity_file}: {e}", file=sys.stderr)
    
    return total_num_tiles


def get_data_memory(loc):
    """
    Calculate the total data memory from the mapping analysis report.
    Replicates the bash script logic from get_data_memory.sh
    
    Args:
        loc: Location of the Work directory
        
    Returns:
        int: Total data memory
    """
    data_memory = 0
    target_fname = "*mapping_analysis_report.txt"
    
    # Find the mapping analysis report file
    report_path = os.path.join(loc, "reports", target_fname)
    report_files = glob.glob(report_path)
    
    if not report_files:
        return data_memory
    
    report_file = report_files[0]
    
    try:
        with open(report_file, 'r') as f:
            content = f.read()
        
        # Extract Memory Bank Report section
        if "Memory Bank Report" not in content:
            return data_memory
            
        # Find the section after "Memory Bank Report"
        mem_bank_section = content.split("Memory Bank Report")[1]
        
        # Extract lines starting with MG(
        mg_lines = []
        for line in mem_bank_section.split('\n'):
            if line.strip().startswith('MG('):
                mg_lines.append(line)
        
        # Skip first 2 lines (header lines) - matches bash: tail -n +3
        mg_lines = mg_lines[2:] if len(mg_lines) > 2 else []
        
        # Process the lines - extract (MG location, buffer name, size) tuples
        # This matches bash: cut -d" " -f1,2,4 | sort -u
        entries = []
        for line in mg_lines:
            # Squeeze multiple spaces - matches bash: tr -s \
            parts = re.sub(r'\s+', ' ', line.strip()).split(' ')
            if len(parts) >= 4:
                # Extract columns 1, 2, and 4 (0-indexed: 0, 1, 3)
                entry = (parts[0], parts[1], parts[3])
                entries.append(entry)
        
        # Get unique entries - matches bash: sort -u
        unique_entries = sorted(set(entries))
        
        # Extract sizes from unique entries - matches bash: cut -d" " -f3
        # Note: bash sort may produce empty first line, which tail -n +2 skips
        sizes = []
        for entry in unique_entries:
            size_str = entry[2]
            if size_str.isdigit():
                sizes.append(int(size_str))
        
        # Sort sizes - matches bash: sort (string sort, but with only digits it's same as numeric)
        sizes = sorted(sizes)
        
        # Sum all sizes - the bash tail -n +2 was skipping an empty line, not a size
        data_memory = sum(sizes)
                
    except Exception as e:
        print(f"Error processing {report_file}: {e}", file=sys.stderr)
    
    return data_memory


def get_program_memory(work_dir):
    """
    Extract the maximum program memory from .map files.
    
    Args:
        work_dir: Location of the Work directory
        
    Returns:
        int: Maximum program memory
    """
    prgmem = []
    
    # Find all .map files
    map_pattern = os.path.join(work_dir, "aie", "*_*", "Release", "*_*.map")
    map_files = glob.glob(map_pattern)
    
    try:
        for map_file in map_files:
            with open(map_file, 'r') as f:
                content = f.read()
            
            # Look for "Section summary for memory 'PM':" section
            if "Section summary for memory 'PM':" in content:
                # Extract the section
                pm_section = content.split("Section summary for memory 'PM':")[1]
                # Take next 10 lines
                lines = pm_section.split('\n')[:10]
                
                # Search for numbers before "Total" or "total"
                for line in lines:
                    if 'Total' in line or 'total' in line:
                        # Extract isolated numbers before Total/total
                        match = re.search(r'\b(\d+)\b\s+[Tt]otal', line)
                        if match:
                            value = int(match.group(1))
                            prgmem.append(value)
                            
    except Exception as e:
        print(f"Error processing map files: {e}", file=sys.stderr)
    
    return prgmem


def harvest_memory(work_dir="Work"):
    """
    Main function that harvests memory information.
    """

    # NUM_BANKS
    num_banks = get_num_banks(work_dir, "dummy")
    
    # NUM_AIE
    num_aie = get_num_me(work_dir, 1)
    
    # NUM_CORES
    num_cores = get_num_cores(work_dir)
    
    # NUM_TILES
    num_tiles = get_num_tiles(work_dir)
    
    # DATA_MEMORY
    data_memory = calculate_data_memory(work_dir)

    # PROGRAM_MEMORY
    program_memory = get_program_memory(work_dir)

    return num_banks, num_aie, num_cores, num_tiles, data_memory, program_memory

def calculate_data_memory(loc):
    """
    Replicates the shell script logic:
    - Finds *mapping_analysis_report.txt in loc/reports/
    - Extracts 'Memory Bank Report' section
    - Keeps lines starting with 'MG('
    - Removes first two MG lines (like tail -n +3)
    - Squeezes repeated whitespace
    - Keeps fields 1,2,4
    - Removes duplicates
    - Extracts buffer sizes (field 3 after cut)
    - Sorts sizes
    - Skips smallest (tail -n +2)
    - Sums the rest
    """

    reports_path = Path(loc) / "reports"
    files = glob.glob(str(reports_path / "*mapping_analysis_report.txt"))
    if not files:
        raise FileNotFoundError("No matching mapping_analysis_report.txt file found")

    target_file = files[0]

    with open(target_file, "r") as f:
        lines = f.readlines()

    # Step 1: Extract from "Memory Bank Report"
    try:
        start_idx = next(i for i, l in enumerate(lines) if "Memory Bank Report" in l)
    except StopIteration:
        return 0

    lines = lines[start_idx + 1:]

    # Step 2: Keep lines starting with "MG("
    mg_lines = [l for l in lines if l.strip().startswith("MG(")]

    # Step 3: Remove first two lines (tail -n +3)
    mg_lines = mg_lines[2:]

    # Step 4: Squeeze repeated whitespaces
    mg_lines = [re.sub(r"\s+", " ", l.strip()) for l in mg_lines]

    # Step 5: Keep fields 1,2,4
    processed = []
    for line in mg_lines:
        parts = line.split(" ")
        if len(parts) >= 4:
            processed.append(f"{parts[0]} {parts[1]} {parts[3]}")

    # Step 6: Unique + sort
    processed = sorted(set(processed))

    # Step 7: Extract sizes (3rd field)
    sizes = []
    for line in processed:
        parts = line.split(" ")
        if len(parts) >= 3:
            try:
                sizes.append(int(parts[2]))
            except ValueError:
                pass

    # Step 8: Sort and skip smallest (tail -n +2)
    sizes = sorted(sizes)

    # Step 9: Sum
    return sum(sizes)
