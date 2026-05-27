#!/usr/bin/env python3
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

def is_power_of_2(n):
    """Check if n is a power of 2"""
    return n > 0 and (n & (n - 1)) == 0

def check_aie_bd_use(vss_mode, aie_variant, ssr, data_width, point_size_d1=1, point_size_d2=1):
    """
    Determine transpose flags based on FFT configuration parameters.
    
    Args:
        vss_mode: VSS mode (1 or other)
        aie_variant: AIE variant (1 or 2)
        ssr: Sampling rate ratio
        data_width: Data width in bits
        point_size_d1: Point size dimension 1 (optional)
        point_size_d2: Point size dimension 2 (optional)
    
    Returns:
        Tuple of (hasBdTranspose)
    """
    hasBdTranspose = 1

    if vss_mode == 1:
        ssrPow2 = is_power_of_2(ssr)
        # front and back transpose get simplified only if SSR is a power of 2. Otherwise, the transpose will be done in PL. Irrespective of AIE variant
        if not ssrPow2:
            hasBdTranspose = 0
        elif aie_variant == 1:
        # aie variant 1 BDs are limited in their support for 64 bit data width.
            if data_width == 64:
                hasBdTranspose = 0   
            if point_size_d1 == 1 and point_size_d2 >= 65536:
                sizeSupportsBD = 0
            else:
                sizeSupportsBD = 1
            if sizeSupportsBD == 0:
                hasBdTranspose = 0
    else:
            # potential for future optimisation
            hasBdTranspose = 0
    return hasBdTranspose

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Check AIE BD transpose usage")
    parser.add_argument("--vss_mode", type=int, required=True, help="VSS mode")
    parser.add_argument("--aie_variant", type=int, required=True, help="AIE variant")
    parser.add_argument("--ssr", type=int, required=True, help="Sampling rate ratio")
    parser.add_argument("--data_width", type=int, required=True, help="Data width in bits")
    parser.add_argument("--point_size_d1", type=int, default=1, help="Point size dimension 1 (default: 1)")
    parser.add_argument("--point_size_d2", type=int, default=1, help="Point size dimension 2 (default: 1)")
    
    args = parser.parse_args()
    
    hasBdTranspose = check_aie_bd_use(
        args.vss_mode, args.aie_variant, args.ssr, args.data_width,
        args.point_size_d1, args.point_size_d2
    )
    
    print(hasBdTranspose)
