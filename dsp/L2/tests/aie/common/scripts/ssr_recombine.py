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

import argparse
import glob
import csv
import os

def parse_args():
    parser = argparse.ArgumentParser(description="Combine/Recombine SSR files into a packetized files with 64-bit formatting.")
    parser.add_argument('--combine', type=int, default=1, help='1 - Combine (default), 0 - Recombine')
    parser.add_argument('--input_pattern', type=str,
                        help='Glob pattern to match input files, e.g. "input_*.txt"')
    parser.add_argument('--output_pattern', type=str,
                        help='Glob pattern to match output files, e.g. "uut_output_*.txt"')
    parser.add_argument('--group_size', type=int, default=1,
                        help='Number of input files per conversion')
    parser.add_argument('--windowVsize', type=int, required=True, help='Number of samples per SSR file per packet')
    parser.add_argument('--data_type', required=True, help='Sample precision in bits (16 or 32)')
    parser.add_argument('--output_file', required=True, help='Output file name')
    parser.add_argument('--verbose', action='store_true', help='Verbose output')
    return parser.parse_args()

def read_samples(files):
    data = []
    for fname in files:
        with open(fname, 'r') as f:
            samples = []
            for line in f:
                samples.extend(line.strip().split())
            data.append(samples)
    return data

# 32-bit header containts an 8-bit packet index
# if sample_bits are less than 32-bit, header must be formatted to have packet index in the LSBs
# and contain extra samples to fill in a 32-bit word.
def add_header(sample, sample_bits):
    parts = 4 if sample_bits == 8 else 2 if sample_bits == 16 else 1
    int_group = []
    for i in range(parts - 1):
        int_group.append(0)
    int_group.append(sample)
    return int_group


def format_64bit(samples, sample_bits):
    words = []
    # step = 4 if sample_bits == 16 else 2
    step = 8 if sample_bits == 8 else 4 if sample_bits == 16 else 2 if sample_bits == 32 else 1
    for i in range(0, len(samples), step):
        group = samples[i:i+step]
        group += ['0'] * (step - len(group))
        # Flatten any nested lists and convert to int
        flat_group = []
        for s in group:
            if isinstance(s, list):
                flat_group.extend(s)
            else:
                flat_group.append(s)
        int_group = [str(int(s)) for s in flat_group]
        word = ' '.join(int_group)
        if i + step >= len(samples):
            words.append('TLAST')
        words.append(word)
    return words

def format_comment_csv(comment):
    rows = []
    rows.append(comment)
    return rows

def format_64bit_csv(samples, sample_bits):
    rows = []
    preamble = "DATA"
    step = 8 if sample_bits == 8 else 4 if sample_bits == 16 else 2 if sample_bits == 32 else 1
    total_groups = (len(samples) + step - 1) // step

    for i in range(0, len(samples), step):
        group = samples[i:i+step]
        group += ['0'] * (step - len(group))  # pad if needed

        flat_group = []
        for s in group:
            if isinstance(s, list):
                flat_group.extend(s)
            else:
                flat_group.append(s)

        int_group = [int(s) for s in flat_group]
        # Add TLAST, TKEEP

        tlast = 1 if i + step >= len(samples) else 0
        # samples always aligned to 64-bit boundary. Adding 32-bit header, always leaves 32-bits hanging
        tkeep = '0x0F' if tlast == 1 else '0x0'

        row = [preamble] + int_group + [tkeep, tlast]
        rows.append(row)
    return rows

def combine_files(input_ssr_files, windowVsize, data_type, output_file, verbose):
    ssr_data = read_samples(input_ssr_files)
    # Reconstruct 32-bit samples for int16/cint16 types
    if data_type in ['cint16', 'int16']:
        ssr_32bit_data = []
        for samples in ssr_data:
            combined = []
            for i in range(0, len(samples), 2):
                low = int(samples[i]) if i < len(samples) else 0
                high = int(samples[i+1]) if i+1 < len(samples) else 0
                # Convert to unsigned 16-bit
                low &= 0xFFFF
                high &= 0xFFFF
                combined_sample = (high << 16) | low
                combined.append(combined_sample)
            ssr_32bit_data.append([str(s) for s in combined])
        ssr_data = ssr_32bit_data

    # Reconstruct 32-bit samples for int8 type
    elif data_type == 'int8':
        ssr_32bit_data = []
        for samples in ssr_data:
            combined = []
            for i in range(0, len(samples), 4):
                b0 = int(samples[i]) if i < len(samples) else 0
                b1 = int(samples[i+1]) if i+1 < len(samples) else 0
                b2 = int(samples[i+2]) if i+2 < len(samples) else 0
                b3 = int(samples[i+3]) if i+3 < len(samples) else 0
                # Convert to unsigned 8-bit
                b0 &= 0xFF
                b1 &= 0xFF
                b2 &= 0xFF
                b3 &= 0xFF
                combined_sample = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0
                combined.append(combined_sample)
            ssr_32bit_data.append(combined)
        ssr_data = ssr_32bit_data

    else:
        ssr_32bit_data = []
        for samples in ssr_data:
            combined = []
            for i in range(0, len(samples), 1):
                low = int(samples[i]) if i < len(samples) else 0
                low &= 0xFFFFFFFF
                combined_sample = low
                combined.append(combined_sample)
            ssr_32bit_data.append(combined)
        ssr_data = ssr_32bit_data

    num_ssr = len(ssr_data)
    max_len = max(len(samples) for samples in ssr_data)
    num_packets = (max_len + windowVsize - 1) // windowVsize

    complex_types = {'cfloat', 'cint32', 'cint16'}
    windowVsize = 2 * windowVsize if data_type in complex_types else windowVsize
    sample_bits = {
        'cfloat': 32,
        'cint32': 32,
        'cint16': 32,
        'float': 32,
        'int32': 32,
        'int16': 32,
        'int8': 32
    }[data_type]
    samples_per_line = {
        'cfloat': 2,
        'cint32': 2,
        'cint16': 2,
        'float': 2,
        'int32': 2,
        'int16': 2,
        'int8': 2
    }[data_type]
    csv_header = get_samples_csv_header(samples_per_line)

    if verbose:
        print(f"num_ssr: {num_ssr}")
        print(f"max_len: {max_len}")
        print(f"num_packets: {num_packets}")

    output_txt = f"{output_file}.txt"

    with open(output_txt, 'w') as out:
        for pkt_idx in range(num_packets):
            for ssr_idx in range(num_ssr):
                words = []
                # Write 32-bit packet index
                packet_samples = add_header(ssr_idx, sample_bits)

                if verbose:
                    print(f"packet_samples: {packet_samples}")
                start = pkt_idx * windowVsize
                end = min(start + windowVsize, len(ssr_data[ssr_idx]))
                # Write packet data
                packet_samples.extend(ssr_data[ssr_idx][start:end])
                if verbose:
                    print(f"packet_samples with ssr_data: {packet_samples}")
                words = format_64bit(packet_samples, sample_bits)
                if verbose:
                    print(f"words: {words}")
                for word in words:
                    out.write(word + '\n')
            if verbose:
                print(f"Packet {pkt_idx}: {ssr_idx} start {start} end {end} samples, {len(words)} lines written to {output_file}")
            packet_samples = []

    output_csv = f"{output_file}.csv"
    with open(output_csv, 'w', newline='') as out:
        writer = csv.writer(out)
        rows = format_comment_csv(csv_header)
        writer.writerows(rows)
        for pkt_idx in range(num_packets):
            for ssr_idx in range(num_ssr):
                packet_samples = add_header(ssr_idx, sample_bits)
                start = pkt_idx * windowVsize
                end = min(start + windowVsize, len(ssr_data[ssr_idx]))
                packet_samples.extend(ssr_data[ssr_idx][start:end])
                rows = format_64bit_csv(packet_samples, sample_bits)
                writer.writerows(rows)

def get_samples_csv_header(samples_per_line):
    if samples_per_line == 8:
        csv_header = ['CMD', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'D', 'TKEEP', 'TLAST']
    elif samples_per_line == 4:
        csv_header = ['CMD', 'D', 'D', 'D', 'D', 'TKEEP', 'TLAST']
    elif samples_per_line == 2:
        csv_header = ['CMD', 'D', 'D', 'TKEEP', 'TLAST']
    else:
        csv_header = ['CMD', 'D', 'TKEEP', 'TLAST']
    return csv_header

def unpack_64bit_txt(input_txt, data_type, output_prefix, output_idx, pkt_streams_per_file, verbose=False):
    sample_bits = {
        'cfloat': 32,
        'cint32': 32,
        'cint16': 32,
        'float': 32,
        'int32': 32,
        'int16': 32,
        'int8': 32
    }[data_type]
    samples_per_line = {
        'cfloat': 2,
        'cint32': 2,
        'cint16': 2,
        'float': 2,
        'int32': 2,
        'int16': 2,
        'int8': 2
    }[data_type]
    step_32b = 4 if sample_bits == 8 else 2 if sample_bits == 16 else 1 if sample_bits == 32 else 1
    header_idx = 3 if sample_bits == 8 else 1 if sample_bits == 16 else 0 if sample_bits == 32 else 0
    step = 2 * step_32b
    with open(input_txt, 'r') as f:
        lines = f.readlines()

    idx = 0
    while idx < len(lines):
        line = lines[idx].strip()
        if line.startswith('T ') and len(line.split()) == 3 and line.split()[2].endswith('s'):
            # Discard timestamp lines like "T [number] [p]s"
            if verbose:
                print(f"Discarding timestamp line: {line}")
            idx += 1
            # line = lines[idx].strip()
            continue
            # if line == 'TLAST':
        #     idx += 1
        #     continue
        samples = line.split()
        # First sample(s) are header (packet index)
        packet_hdr_idx = int(samples[header_idx]) & 0xFF
        packet_index = packet_hdr_idx + (output_idx * pkt_streams_per_file)
        data_samples = samples[header_idx+1:]  # skip header
        if verbose:
            print(f"header_idx {header_idx}")
            print(f"step_32b {step_32b}")
            print(f"samples {samples}")
            print(f"data_samples {data_samples}")
            print(f"packet_hdr_idx {packet_hdr_idx}")
            print(f"packet_index {packet_index}")
        collected = []
        # Add the first data sample sent alongside packet header
        collected.extend(data_samples)
        idx += 1
        # Collect samples until TLAST
        while idx < len(lines):
            line = lines[idx].strip()
            if line == 'TLAST':
                if verbose:
                    print(f"line {line} idx {idx}")
                idx += 1
                # Collect samples after TLAST
                if idx < len(lines):
                    next_line = lines[idx].strip()
                    samples = next_line.split()
                    if verbose:
                        print(f"next_line {next_line}")
                        print(f"samples {samples}")
                    # TODO: Check for TKEEP and mask samples accordingly?
                    collected.extend(samples)
                    idx += 1
                    break
            elif line.startswith('T ') and len(line.split()) == 3 and line.split()[2].endswith('s'):
                # Discard timestamp lines like "T [number] [p]s"
                if verbose:
                    print(f"Discarding timestamp line: {line}")
                idx += 1
            else:
                samples = line.split()
                collected.extend(samples)
                idx += 1

        # Reconstruct 16-bit samples for int16/cint16 types, based on 32-bit format
        if data_type in ['int16', 'cint16']:
            reconstructed = []
            for val in collected:
                v = int(val)
                low = v & 0xFFFF
                high = (v >> 16) & 0xFFFF
                # Convert to signed 16-bit
                low = low if low < 0x8000 else low - 0x10000
                high = high if high < 0x8000 else high - 0x10000
                reconstructed.extend([low, high])
            collected = reconstructed
        elif data_type == 'int8':
            reconstructed = []
            for val in collected:
                v = int(val)
                b0 = v & 0xFF
                b1 = (v >> 8) & 0xFF
                b2 = (v >> 16) & 0xFF
                b3 = (v >> 24) & 0xFF
                # Convert to signed 8-bit
                b0 = b0 if b0 < 0x80 else b0 - 0x100
                b1 = b1 if b1 < 0x80 else b1 - 0x100
                b2 = b2 if b2 < 0x80 else b2 - 0x100
                b3 = b3 if b3 < 0x80 else b3 - 0x100
                reconstructed.extend([b0, b1, b2, b3])
            collected = reconstructed
        else:
            reconstructed = []
            for val in collected:
                v = int(val)
                low = v & 0xFFFFFFFF
                # Convert to signed 32-bit
                low = low if low < 0x80000000 else low - 0x100000000
                reconstructed.extend([low])
            collected = reconstructed
        # Write to output file
        out_file = f"{output_prefix}_{packet_index}_0.txt"
        os.makedirs(os.path.dirname(out_file), exist_ok=True)
        with open(out_file, 'a') as out:
            words = []
            # Split collected samples into groups of 'step' elements
            for i in range(0, len(collected), step):
                group = collected[i:i+step]
                group += ['0'] * (step - len(group))  # pad if needed
                word = ' '.join(str(s) for s in group)
                words.append(word)
            for word in words:
                out.write(word + '\n')
        if verbose:
            print(f"Wrote {len(collected)} samples to {out_file}")

def main():
    args = parse_args()

    verbose = args.verbose

    if args.combine == 1:
        # Expand pattern to actual file list
        input_ssr_files = sorted(glob.glob(args.input_pattern))
        total_files = len(input_ssr_files)

        if total_files == 0:
            print(f"No files matched pattern: {args.input_pattern}")
            return

        # Group input files
        file_groups = [input_ssr_files[i:i+args.group_size] for i in range(0, total_files, args.group_size)]

        for group_idx, file_group in enumerate(file_groups):
            output_pkt_file = f"{args.output_file}_{group_idx}_0"
            if verbose:
                print(f"Group idx: {group_idx}")
                print(f"file_group: {file_group}")
                print(f"output_pkt_file: {output_pkt_file}")
            combine_files(file_group, args.windowVsize, args.data_type, output_pkt_file, args.verbose)

    if args.combine == 0:
        # Expand pattern to actual file list
        input_pkt_files = sorted(glob.glob(args.output_pattern))
        total_files = len(input_pkt_files)
        pkt_streams_per_file = args.group_size

        if total_files == 0:
            print(f"No files matched pattern: {args.output_pattern}")
            return
        # Process each output file individually
        for file_idx, input_pkt_file in enumerate(input_pkt_files):
            if verbose:
                print(f"Processing file idx: {file_idx}")
                print(f"input_pkt_file: {input_pkt_file}")
            # Unpack TXT and CSV formats
            output_prefix = os.path.splitext(args.output_file)[0]
            unpack_64bit_txt(input_pkt_file, args.data_type, output_prefix, file_idx, pkt_streams_per_file, args.verbose)

if __name__ == "__main__":
    main()