#!/usr/bin/env python3
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

"""
Script to extract resource utilization data from Vivado reports and config.h parameters.
Outputs all extracted resources in a readable format.
"""

import argparse
import os
import re
import sys
from pathlib import Path


def parse_config_h(filepath):
    """
    Parse config.h file and extract all #define key-value pairs.
    
    Args:
        filepath: Path to the config.h file
        
    Returns:
        Dictionary of {key: value} pairs from #define statements
    """
    defines = {}
    
    if not os.path.exists(filepath):
        print(f"Warning: config.h not found at: {filepath}", file=sys.stderr)
        return defines
    
    with open(filepath, 'r') as f:
        for line in f:
            # Match #define KEY VALUE pattern
            match = re.match(r'^\s*#define\s+(\w+)\s+(.+?)\s*$', line)
            if match:
                key = match.group(1)
                value = match.group(2).strip()
                defines[key] = value
    
    return defines


def parse_params_cfg(filepath):
    """
    Parse vss_fft_ifft_1d_params.cfg file to extract part, freqhz, and APP_PARAMS.
    
    Args:
        filepath: Path to the params.cfg file
        
    Returns:
        Dictionary of extracted parameters
    """
    params = {}
    
    if not os.path.exists(filepath):
        print(f"Warning: params.cfg not found at: {filepath}", file=sys.stderr)
        return params
    
    in_app_params = False
    
    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            
            # Skip empty lines and comments
            if not line or line.startswith('#'):
                continue
            
            # Check for section headers
            if line.startswith('['):
                in_app_params = line == '[APP_PARAMS]'
                continue
            
            # Parse key=value pairs
            if '=' in line:
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip()
                params[key] = value
    
    return params


def parse_utilization_report(filepath):
    """
    Parse Vivado place_report_utilization_0.rpt to extract resource utilization.
    
    Args:
        filepath: Path to the utilization report file
        
    Returns:
        Dictionary of resource utilization values
    """
    resources = {
        'Slices': 'N/A',
        'CLB_LUTs': 'N/A',
        'CLB_Registers': 'N/A',
        'Block_RAM_Tiles': 'N/A',
        'DSP_Slices': 'N/A',
        'URAM': 'N/A'
    }
    
    if not os.path.exists(filepath):
        print(f"Warning: Utilization report not found at: {filepath}", file=sys.stderr)
        return resources
    
    with open(filepath, 'r') as f:
        content = f.read()
    
    # Extract SLICE (from CLB Distribution section)
    match = re.search(r'\|\s*SLICE\s*\|\s*(\d+)\s*\|', content)
    if match:
        resources['Slices'] = match.group(1)
    
    # Extract CLB LUTs
    match = re.search(r'\|\s*CLB LUTs\s*\|\s*(\d+)\s*\|', content)
    if match:
        resources['CLB_LUTs'] = match.group(1)
    
    # Extract CLB Registers
    match = re.search(r'\|\s*CLB Registers\s*\|\s*(\d+)\s*\|', content)
    if match:
        resources['CLB_Registers'] = match.group(1)
    
    # Extract Block RAM Tile
    match = re.search(r'\|\s*Block RAM Tile\s*\|\s*(\d+)\s*\|', content)
    if match:
        resources['Block_RAM_Tiles'] = match.group(1)
    else:
        # Try alternative patterns
        match = re.search(r'\|\s*RAMB36/FIFO36\s*\|\s*(\d+)\s*\|', content)
        if match:
            resources['Block_RAM_Tiles'] = match.group(1)
        else:
            match = re.search(r'\|\s*BRAM\s*\|\s*(\d+)\s*\|', content)
            if match:
                resources['Block_RAM_Tiles'] = match.group(1)
    
    # Extract DSP Slices
    match = re.search(r'\|\s*DSP Slices\s*\|\s*(\d+)\s*\|', content)
    if match:
        resources['DSP_Slices'] = match.group(1)
    else:
        # Try alternative patterns
        match = re.search(r'\|\s*DSP48E2\s*\|\s*(\d+)\s*\|', content)
        if match:
            resources['DSP_Slices'] = match.group(1)
        else:
            match = re.search(r'\|\s*DSP58\s*\|\s*(\d+)\s*\|', content)
            if match:
                resources['DSP_Slices'] = match.group(1)
    
    # Extract URAM
    match = re.search(r'\|\s*URAM\s*\|\s*(\d+)\s*\|', content)
    if match:
        resources['URAM'] = match.group(1)
    
    return resources


def calculate_datawidth(data_type):
    """
    Calculate DATAWIDTH based on DATA_TYPE.
    
    Args:
        data_type: The DATA_TYPE value (e.g., 'cint16', 'cint32', 'cfloat')
        
    Returns:
        DATAWIDTH value (32 or 64)
    """
    if data_type and data_type.lower() == 'cint16':
        return 32
    else:
        return 64


def calculate_aie_variant(part):
    """
    Calculate AIE_VARIANT based on the PART name.
    
    Args:
        part: The PART string (e.g., 'xcvc1902-vsva2197-2MP-e-S')
        
    Returns:
        AIE_VARIANT string ('AIE', 'AIE-ML', 'AIE-MLv2', or 'Unknown')
    """
    if not part:
        return 'Unknown'
    
    part_lower = part.lower()
    
    if 'vck190' in part_lower or 'vc1902' in part_lower:
        return 'AIE'
    elif 'vek280' in part_lower or 've2802' in part_lower:
        return 'AIE-ML'
    elif 'vek385' in part_lower or 've2302' in part_lower:
        return 'AIE-MLv2'
    else:
        return 'Unknown'


def extract_work_resources(work_dir):
    """
    Extract resources from the Work directory.
    
    This function attempts to extract NUM_BANKS, NUM_AIE, NUM_TILES, and DATA_MEMORY
    from the AIE Work directory by parsing the compilation output files.
    
    Args:
        work_dir: Path to the Work directory
        
    Returns:
        Dictionary of resource values
    """
    import json
    
    resources = {
        'NUM_BANKS': 'N/A',
        'NUM_AIE': 'N/A',
        'NUM_TILES': 'N/A',
        'DATA_MEMORY': 'N/A'
    }
    
    if not os.path.isdir(work_dir):
        print(f"Warning: Work directory not found at: {work_dir}", file=sys.stderr)
        return resources
    
    # Try to parse compiler_report.json which contains detailed AIE information
    compiler_report_path = os.path.join(work_dir, 'reports', 'compiler_report.json')
    
    if os.path.exists(compiler_report_path):
        try:
            with open(compiler_report_path, 'r') as f:
                data = json.load(f)
                
                # Count AIE tiles (blocks with fabric="me")
                if 'blockInstances' in data:
                    aie_tiles = set()
                    for instance_id, instance in data['blockInstances'].items():
                        if instance.get('fabric') == 'me':
                            # Get tile location from mapping
                            if 'mapping' in data and 'blockInstanceMapping' in data['mapping']:
                                mapping = data['mapping']['blockInstanceMapping'].get(instance_id, {})
                                if 'coreInfo' in mapping:
                                    core_info = mapping['coreInfo']
                                    tile_key = f"{core_info.get('column', 0)}_{core_info.get('row', 0)}"
                                    aie_tiles.add(tile_key)
                    
                    if aie_tiles:
                        resources['NUM_TILES'] = str(len(aie_tiles))
                        resources['NUM_AIE'] = str(len(aie_tiles))
                
                # Count memory banks from mapping
                if 'mapping' in data and 'portInstanceMapping' in data['mapping']:
                    memory_banks = set()
                    total_memory = 0
                    for port_id, port_data in data['mapping']['portInstanceMapping'].items():
                        if 'bufferInfo' in port_data:
                            for buf in port_data['bufferInfo']:
                                if 'column' in buf and 'row' in buf:
                                    bank_key = f"{buf['column']}_{buf['row']}"
                                    memory_banks.add(bank_key)
                                if 'size' in buf:
                                    total_memory += buf['size']
                    
                    if memory_banks:
                        resources['NUM_BANKS'] = str(len(memory_banks))
                    if total_memory > 0:
                        resources['DATA_MEMORY'] = str(total_memory)
                        
        except Exception as e:
            print(f"Warning: Error parsing {compiler_report_path}: {e}", file=sys.stderr)
    
    # Fallback: Try to count AIE tiles from ELF files
    if resources['NUM_TILES'] == 'N/A':
        try:
            core_files = list(Path(work_dir).rglob('*.elf'))
            if core_files:
                resources['NUM_TILES'] = str(len(core_files))
        except Exception:
            pass
    
    # Fallback: Try to extract from log files
    log_paths = [
        os.path.join(work_dir, 'aiecompiler.log'),
        os.path.join(work_dir, '..', 'AIECompiler.log'),
    ]
    
    for log_path in log_paths:
        if os.path.exists(log_path):
            try:
                with open(log_path, 'r') as f:
                    content = f.read()
                    
                    # Look for tile count
                    match = re.search(r'Total tiles used:\s*(\d+)', content)
                    if match and resources['NUM_TILES'] == 'N/A':
                        resources['NUM_TILES'] = match.group(1)
                    
                    # Look for memory banks
                    match = re.search(r'Memory banks used:\s*(\d+)', content)
                    if match and resources['NUM_BANKS'] == 'N/A':
                        resources['NUM_BANKS'] = match.group(1)
                        
            except Exception as e:
                print(f"Warning: Error parsing {log_path}: {e}", file=sys.stderr)
    
    return resources


def print_resources(config_values, derived_values, utilization, work_resources):
    """
    Print all extracted resources in simple KEY: VALUE format.
    """
    # Print config.h values
    for key in sorted(config_values.keys()):
        print(f"{key}: {config_values[key]}")
    
    # Print derived values
    for key, value in derived_values.items():
        print(f"{key}: {value}")
    
    # Print utilization values
    for key, value in utilization.items():
        print(f"{key}: {value}")
    
    # Print Work directory resources
    for key, value in work_resources.items():
        print(f"{key}: {value}")


def main():
    parser = argparse.ArgumentParser(
        description='Extract resource utilization data from Vivado reports and config.h parameters.'
    )
    parser.add_argument(
        'input_folder',
        help='Path to the input folder containing config.h'
    )
    
    args = parser.parse_args()
    
    # Convert to absolute path
    input_folder = os.path.abspath(args.input_folder)
    
    if not os.path.isdir(input_folder):
        print(f"Error: Directory '{input_folder}' does not exist", file=sys.stderr)
        sys.exit(1)
    
    # Parse config.h
    config_h_path = os.path.join(input_folder, 'config.h')
    config_values = parse_config_h(config_h_path)
    
    # Parse vss_fft_ifft_1d_params.cfg (needed for PART)
    params_cfg_path = os.path.join(input_folder, 'vss_fft_ifft_1d_params.cfg')
    params_values = parse_params_cfg(params_cfg_path)
    
    # Parse utilization report (relative path from input_folder)
    util_report_path = os.path.join(
        input_folder, 
        '_x', 'link', 'vivado', 'vpl', 'prj', 'prj.runs', 'impl_1', 
        'place_report_utilization_0.rpt'
    )
    utilization = parse_utilization_report(util_report_path)
    
    # Extract Work directory resources
    work_dir = os.path.join(input_folder, 'Work')
    work_resources = extract_work_resources(work_dir)
    
    # Calculate derived values
    derived_values = {}
    
    # DATAWIDTH
    data_type = config_values.get('DATA_TYPE', params_values.get('DATA_TYPE', ''))
    derived_values['DATAWIDTH'] = calculate_datawidth(data_type)
    
    # AIE_VARIANT
    part = params_values.get('part', config_values.get('PART', ''))
    derived_values['AIE_VARIANT'] = calculate_aie_variant(part)
    derived_values['PART'] = part if part else 'N/A'
    
    # Print all resources
    print_resources(config_values, derived_values, utilization, work_resources)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
