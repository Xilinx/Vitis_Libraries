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
import status_utils
import sys
import argparse
import os
import subprocess #to call shell

def main():

    first_latency = -1
    avg_latency = -1
    avg_throughput = -1
    avg_initiation_int = -1
    max_des_cycle_count_avg = -1
    min_des_throughput_avg = -1
    first_latency_unit = "ns"
    avg_latency_unit = "ns"
    avg_throughput_unit = "MSa/s"
    avg_initiation_int_unit = "ns"
    
    parser = argparse.ArgumentParser(
        prog="get_qor",
        description="Script to extract latency, throughput, initiation interval, max DES cycle count and min DES throughput from AIEsim output and input files and print them in the status file",
    )
    parser.add_argument("-ip", help="ip name")
    parser.add_argument("-aiesim_out_dir", help="path to AIEsim output directory")
    parser.add_argument("-t_in_file", help="path to AIEsim input file to extract tfirst and tlast values")
    parser.add_argument("-t_out_file", help="path to AIEsim output file to extract tfirst and tlast values")
    parser.add_argument("-status_file_dir", help="path to status file to report qor results")
    parser.add_argument("-num_of_samples", type=int, help="number of graphs to generate")
    parser.add_argument("-casc_len", type=int, help="number of kernels in cascade")
    parser.add_argument("-niter", type=int, help="number of iterations in the AIEsim run")
    parser.add_argument("-use_outputs_if_no_inputs", type=bool, default=False, help="if set, only use input file timestamps for throughput and initiation interval calculation, and skip latency calculation")
    parser.add_argument("-vss_ip", type=bool, default=False, help="if set, use VSS file format for timestamp extraction instead of AIEsim output and input files")
    parser.add_argument("-incr_stability", type=int, default=5, help="sets a threshold for percentage of stability between iterations.")
    parser.add_argument("-min_num_iteration", type=int, default=4, help="sets the min required iteration number.")
    parser.add_argument("-min_num_match", type=int, default=2, help="sets the  min required matches that will be averaged.")
    args = parser.parse_args()

    ip = args.ip
    aiesim_out_dir = args.aiesim_out_dir
    t_in_file = args.t_in_file
    t_out_file = args.t_out_file
    status_file_dir = args.status_file_dir
    num_of_samples = args.num_of_samples
    casc_len = args.casc_len
    niter = args.niter
    percentage_stability_threshold= args.incr_stability
    min_num_iteration = args.min_num_iteration
    min_num_match = args.min_num_match
    USE_OUTPUTS_IF_NO_INPUTS = args.use_outputs_if_no_inputs
    VSS_IP = args.vss_ip

    if not VSS_IP:
        t_in_file_dir = aiesim_out_dir + "/" + t_in_file
        t_out_file_dir = aiesim_out_dir + "/data/" + t_out_file

    OUTPUT_ONLY = False
    if USE_OUTPUTS_IF_NO_INPUTS and not os.path.isfile(t_in_file_dir): #if the flag to use outputs if no inputs is set, and the input file doesn't exist, then only use outputs for calculation and skip latency calculation
        OUTPUT_ONLY = True  

    #extract input file times
    if not VSS_IP:
        if not OUTPUT_ONLY:
            t_in_firsts, t_in_lasts = status_utils.extract_timestamps_from_file_with_units(t_in_file_dir, niter)

    #extract output file times
    if not VSS_IP:
        TLAST_EXISTS = status_utils.find_TLAST_in_file(t_out_file_dir)
        if TLAST_EXISTS:
            t_out_firsts, t_out_lasts = status_utils.extract_output_tlast_and_tfirst_values_with_units(t_out_file_dir)
        else: 
            t_out_firsts, t_out_lasts = status_utils.extract_timestamps_from_file_with_units(t_out_file_dir, niter)
        throughput_t_values = t_out_firsts

    #Latency calculation
    if not OUTPUT_ONLY:
        latency_vals, latency_unit = status_utils.calc_latency(t_in_firsts, t_out_firsts)
        avg_latency, avg_latency_unit, latency_matched_samples_num = status_utils.calc_average(latency_vals, "Latency", latency_unit, min_num_iteration, min_num_match, percentage_stability_threshold)
        avg_latency, avg_latency_unit = status_utils.fix_unit(avg_latency, avg_latency_unit)
        first_latency, first_latency_unit = status_utils.fix_unit(latency_vals[0], latency_unit)

        initation_int_vals, initiation_int_unit = status_utils.calc_initiation_intervals(t_in_firsts)
        avg_initiation_int, avg_initiation_int_unit, initiation_int_matched_samples_num=status_utils.calc_average(initation_int_vals, "Initiation Interval", initiation_int_unit, min_num_iteration, min_num_match, percentage_stability_threshold)
        avg_initiation_int, avg_initiation_int_unit = status_utils.fix_unit(avg_initiation_int, avg_initiation_int_unit)   
    
    #throughput calculation
    throughput_vals, throughput_unit = status_utils.calc_throughput(throughput_t_values, num_of_samples)
    avg_throughput, avg_throughput_unit, throughput_matched_samples_num=status_utils.calc_average(throughput_vals, "Throughput", throughput_unit, min_num_iteration, min_num_match, percentage_stability_threshold)
    avg_throughput, avg_throughput_unit = status_utils.fix_unit(avg_throughput, avg_throughput_unit)

    #extract max DES cycle count and min DES throughput from status files
    max_des_cycle_count_avg, min_des_throughput_avg = status_utils.parse_aiesim_data(num_of_samples, casc_len, status_file_dir, aiesim_out_dir, func_name=ip, num_iter=niter)
    # num_banks, num_aie, num_cores, num_tiles, data_memory, program_memory = status_utils.harvest_memory() #Function to be revised

    with open(status_file_dir, 'a') as out_file:
        if not OUTPUT_ONLY: #do not report if OUTPUT_ONLY
            out_file.write(f"    Latency_first ({first_latency_unit}):             {first_latency} \n") 
            out_file.write(f"    Latency_avg ({avg_latency_unit}):               {avg_latency} \n")
        out_file.write(f"    Throughput_avg ({avg_throughput_unit}):         {avg_throughput} \n")

        if not OUTPUT_ONLY: #do not report if OUTPUT_ONLY
            out_file.write(f"    Initation Interval_avg ({avg_initiation_int_unit}):    {avg_initiation_int} \n")

        out_file.write(f"    cycleCountAvg:                  {max_des_cycle_count_avg}\n")
        out_file.write(f"    throughputCycleCount (MSa/s):   {int(min_des_throughput_avg*100)/100} \n")
        # out_file.write(f"    NUM_BANKS:            {num_banks}\n")
        # out_file.write(f"    NUM_AIE:              {num_aie}\n")
        # out_file.write(f"    NUM_CORES:            {num_cores}\n")
        # out_file.write(f"    NUM_TILES:            {num_tiles}\n")
        # out_file.write(f"    DATA_MEMORY:          {data_memory}\n")
        # out_file.write(f"    PROGRAM_MEMORY:       ")
        # for kernel in range(len(program_memory)):
        #     out_file.write(f"{program_memory[kernel]} ")
        # out_file.write("\n")

    # Call the shell script using subprocess.run
    script_dir = os.path.dirname(os.path.abspath(__file__))
    shell_script_path = os.path.join(script_dir, 'harvest_memory.sh')

    result = subprocess.run(
    ['bash', shell_script_path, status_file_dir, script_dir],
    capture_output=True,
    text=True
    )

    # Check the return code
    if result.returncode != 0:
        print("Unable to run harvest_memory.sh:", result.returncode)

    #generate performance.log file for performance regression tracking
    logs_path_directory = os.path.dirname(status_file_dir)
    performance_log_path = os.path.join(logs_path_directory, "performance.log")
    
    # Write to performance.log
    with open(performance_log_path, "w") as log_file:
               
        for iteration in range(niter):
            if not OUTPUT_ONLY:
                t_in_val, t_in_unit = t_in_firsts[iteration]
                t_in_val_perflog = status_utils.convert_time(t_in_val, t_in_unit, "ns")
                t_out_val, t_out_unit = t_out_firsts[iteration]
                t_out_val_perflog = status_utils.convert_time(t_out_val, t_out_unit, "ns")
                latency_val_perflog = status_utils.convert_time(latency_vals[iteration], latency_unit, "ns")
                throughput_val = throughput_vals[iteration]

                log_file.write(
                    f"Iteration {iteration} "
                    f"ts_in: {int(t_in_val_perflog*100)/100} ns "
                    f"ts_out: {int(t_out_val_perflog*100)/100} ns "
                    f"latency: {int(latency_val_perflog*100)/100} ns "
                    f"throughput: {int(throughput_val*100)/100} MSa/s\n"
                )
 
            else:
                throughput_val = throughput_vals[iteration]
                log_file.write(
                    f"Iteration {iteration} "
                    f"throughput: {int(throughput_val*100)/100} MSa/s\n"
                )

        if not OUTPUT_ONLY:
            log_file.write(
                "If no iterations are stable, a latency and throughput value of -1 will be reported in the status file. "
                "Please run for more iterations or increase stabilityThreshold (optional -inc_stability argument to get_qor.py)\n"
            )
            log_file.write(f"Reported Latency_avg:        {avg_latency} {avg_latency_unit}\n")
            log_file.write(f"Reported Throughput_avg:     {avg_throughput} {avg_throughput_unit}\n")
        else:
            log_file.write(
                f"Latency_avg: -1 ns\n"
                f"Reported Throughput_avg: {avg_throughput} {avg_throughput_unit}\n"
                f"Stable Iterations: {' '.join(str(i) for i in range(niter-throughput_matched_samples_num, niter))} \n" )

if __name__ == "__main__":
    main()
