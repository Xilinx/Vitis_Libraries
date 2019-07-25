#!/usr/bin/env bash

# Copyright 2019 Xilinx, Inc.
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

if [ "$#" -ne 6 ]; then
	echo "Usage: $0 <thread#> <m> <k> <n> <data_type> <mode>" >&2
	echo "where <data_type> = float double" >&2
	echo "<mode> = (g)enerate binary, (b)enchmark, (a)ll" >&2
	exit 1
fi

NUM_M=$2
NUM_K=$3
NUM_N=$4
DATA_TYPE=$5
MODE=$6

NUMA="numactl -i all"
export OMP_NUM_THREADS=$1

if [[ ("$MODE" == "g") || ("$MODE" == "a") ]]; then
	if [ ! -e "../data" ]; then
		mkdir ../data
	fi
	if [[ ("$DATA_TYPE" == "double") ]]; then
		make dgemm_mkl_gen
		./dgemm_mkl_gen $NUM_M $NUM_K $NUM_N
	elif [[ ("$DATA_TYPE" == "float") ]]; then
		make sgemm_mkl_gen
		./sgemm_mkl_gen $NUM_M $NUM_K $NUM_N
	else
		echo "Error in data_type"
		exit 1
	fi
	echo "====================="
	echo "Generating binary complete"
	echo "Binary File is at ../data/"
	echo "====================="
fi

if [[ ("$MODE" == "b") || ("$MODE" == "a") ]]; then
	if [[ ("$DATA_TYPE" == "double") ]]; then
		make dgemm_mkl_bench
		$NUMA ./dgemm_mkl_bench $NUM_M $NUM_K $NUM_N
	elif [[ ("$DATA_TYPE" == "float") ]]; then
		make sgemm_mkl_bench
		$NUMA ./sgemm_mkl_bench $NUM_M $NUM_K $NUM_N
	else
		echo "Error in data_type"
		exit 1
	fi
	echo "====================="
	echo "Benchmarking complete"
	echo "====================="
fi

echo "Done!"

exit 0