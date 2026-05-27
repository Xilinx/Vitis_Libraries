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
import argparse
import configparser
import sys
import io

def main():
    parser = argparse.ArgumentParser(description="Update cfg file with missing parameters.")
    parser.add_argument("input_cfg", help="Input configuration file")
    parser.add_argument("output_cfg", help="Output configuration file")
    
    # Optional arguments for specific parameters
    parser.add_argument("--ADD_FRONT_TRANSPOSE", default="1", help="Value for ADD_FRONT_TRANSPOSE if missing")
    parser.add_argument("--ADD_BACK_TRANSPOSE", default="1", help="Value for ADD_BACK_TRANSPOSE if missing")
    parser.add_argument("--POINT_SIZE_D1", default="1", help="Value for POINT_SIZE_D1 if missing")

    args = parser.parse_args()

    required_params = [
        "DATA_TYPE",
        "TWIDDLE_TYPE",
        "POINT_SIZE",
        "FFT_NIFFT",
        "SHIFT",
        "API_IO",
        "ROUND_MODE",
        "SAT_MODE",
        "TWIDDLE_MODE",
        "SSR",
        "AIE_PLIO_WIDTH",
        "VSS_MODE",
        "ADD_FRONT_TRANSPOSE",
        "ADD_BACK_TRANSPOSE",
        "POINT_SIZE_D1",
        "CASC_LEN",
        "USE_WIDGETS"
    ]

    # Read input file
    try:
        with open(args.input_cfg, 'r') as f:
            content = f.read()
    except IOError:
        print("Error: Input file '{}' not found.".format(args.input_cfg))
        sys.exit(1)

    # Prepend dummy section for global parameters
    config_string = "[Global]\n" + content
    
    config = configparser.ConfigParser()
    # Preserve case of keys
    config.optionxform = str
    config.read_string(config_string)

    if not config.has_section("APP_PARAMS"):
        config.add_section("APP_PARAMS")

    for param in required_params:
        if not config.has_option("APP_PARAMS", param):
            if param == "ADD_FRONT_TRANSPOSE":
                config.set("APP_PARAMS", param, args.ADD_FRONT_TRANSPOSE)
            elif param == "ADD_BACK_TRANSPOSE":
                config.set("APP_PARAMS", param, args.ADD_BACK_TRANSPOSE)
            elif param == "POINT_SIZE_D1":
                config.set("APP_PARAMS", param, args.POINT_SIZE_D1)
            elif param == "USE_WIDGETS":
                config.set("APP_PARAMS", param, "0")
            else:
                config.set("APP_PARAMS", param, "1")

    # Write to string buffer
    output_buffer = io.StringIO()
    config.write(output_buffer)
    
    # Get content and remove the dummy section
    output_content = output_buffer.getvalue()
    
    # Split lines and remove the first line ([Global])
    lines = output_content.splitlines()
    if lines and lines[0].strip() == "[Global]":
        lines = lines[1:]
    
    # Reconstruct the content
    final_content = "\n".join(lines) + "\n"

    # Write to output file
    with open(args.output_cfg, 'w') as f:
        f.write(final_content)

    print("Successfully updated configuration and saved to '{}'".format(args.output_cfg))

if __name__ == "__main__":
    main()
