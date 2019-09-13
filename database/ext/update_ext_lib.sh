#!/bin/bash

rm -rf xf_utils_hw_repo xf_utils_hw
git clone --depth 1 https://gitenterprise.xilinx.com/FaaSApps/xf_utils_hw.git xf_utils_hw_repo
mv xf_utils_hw_repo/L1/include/xf_utils_hw .
rm -rf xf_utils_hw_repo
