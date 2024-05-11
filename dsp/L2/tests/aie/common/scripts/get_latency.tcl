#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2024, Advanced Micro Devices, Inc.
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
set AIESIM_OUT_DIR          [lindex $argv 0]
set T_IN_FILE               [lindex $argv 1]
set T_OUT_FILE              [lindex $argv 2]
set STATUS_FILE             [lindex $argv 3]
set NUM_OF_SAMPLES          [lindex $argv 4]
set NITER                   [lindex $argv 5]

# percentage threshold for detecting stability in latency (set optional argv 6 to overwrite)
set stabilityThreshold 5  
# Add "USE_OUTPUTS_IF_NO_INPUTS" as final argument to script to ignore warnings if no input file exists, 
# and calculate throughput from only the output file timestamps. 
# If input exists, latency and throughput will be calculated as normal
set USE_OUTPUTS_IF_NO_INPUTS 0  
if {[llength $argv] >= 7} {  
    set arg7 [lindex $argv 6]  
    if {$arg7 == "USE_OUTPUTS_IF_NO_INPUTS"} {  
        set USE_OUTPUTS_IF_NO_INPUTS 1  
    } else {  
        set stabilityThreshold $arg7  
    }  
}   
# Check if a "USE_OUTPUTS_IF_NO_INPUTS" argument was passed as the optional eighth argument  
if {[llength $argv] == 8} {  
    set arg8 [lindex $argv 7]  
    if {$arg8 == "USE_OUTPUTS_IF_NO_INPUTS"} {  
        set USE_OUTPUTS_IF_NO_INPUTS 1  
    }  
} 


set statusFile              [open $STATUS_FILE a]
set logFile                 [open "./logs/performance_log.txt" w]

set fileIn  [glob -nocomplain -directory $AIESIM_OUT_DIR -- $T_IN_FILE]
set fileOut [glob -nocomplain -directory $AIESIM_OUT_DIR -- $T_OUT_FILE]
set fileOutTemp "$AIESIM_OUT_DIR/data/T_uut_output.txt"

# Latency during first few iterations typically not stable
set minNumberOfIterations 4
# if no stable latency or throughput determined:
set stableLatency -1
set stableThroughout -1
# When USE_OUTPUTS_IF_NO_INPUTS is 1, and no output exists, throughput will be calculated from output timestamps
set OUTPUTS_ONLY 0

# Open files if exist
set fexist1 [file exist $fileIn]
set fexist2 [file exist $fileOut]

# Input file exists -> calculate latency and throughput as normal
# Input file does not exist but USE_OUTPUTS_IF_NO_INPUTS is set -> calculate throughput from output file timestamps
# Input file does not exist and USE_OUTPUTS_IF_NO_INPUTS is NOT set -> give error message, exit script
if {$fexist1} {
    set inputsFile [open $fileIn r]   
    puts $fileIn     
} elseif {$USE_OUTPUTS_IF_NO_INPUTS} {
    set OUTPUTS_ONLY 1
    puts "Calculating throught from output file timestamps. No inputs exist"
} else {
    puts "Cannot find timestamped input file $T_IN_FILE. This should be located at $AIESIM_OUT_DIR/$T_IN_FILE"
    puts "Please check arguments to the get_latency.tcl file"
    puts "get_latency.tcl ./aiesimulator_output T_input_0_0.txt data/uut_output_0_0.txt ./logs/NAME_OF_STATUS_FILE.txt  NUMBER_OF_SAMPLES_PER_ITERATION NUMBER_OF_ITERATIONS"
    exit 1
}
if {$fexist2} {
    # remove lines containing "TLAST" from output file, create new output file
    exec sed {/TLAST/d} $fileOut > $fileOutTemp    
    set outputsFileNew [open $fileOutTemp r]    
} else {
    puts "Cannot find timestamped input file $T_OUT_FILE. This should be located at $AIESIM_OUT_DIR/$T_OUT_FILE"
    puts "Please check arguments to the get_latency.tcl file"
    puts "get_latency.tcl ./aiesimulator_output T_input_0_0.txt data/uut_output_0_0.txt ./logs/NAME_OF_STATUS_FILE.txt  NUMBER_OF_SAMPLES_PER_ITERATION NUMBER_OF_ITERATIONS"
    exit 1
}

# Both input and output file are present
if {$OUTPUTS_ONLY == 0} {
    set tsIn {}
    set tsOut {}
    set latencyValues {}
    set throughputValues {}
    set stableItNum {}

    # get number of lines for in/out files
    set numLinesInput [lindex [exec wc -l $fileIn] 0]
    set numLinesOutput [lindex [exec wc -l $fileOutTemp] 0]
    
    # get the number of lines in the file per each iteration
    set numLinesInputIteration [expr $numLinesInput/$NITER]
    set numLinesOutputIteration [expr $numLinesOutput/$NITER]
    set inLineCount 0
    set outLineCount 0

    # loop through input file, get timestamp value from first line of each iteration 
    while {[gets $inputsFile inLine] != -1} { 
        incr inLineCount 
        # if first time stamp of an iteration
        if {[expr ($inLineCount) % ($numLinesInputIteration)] == 1} {
            # lines containing the timestamp in format: "T tsVal tsUnit"
            set tsVal [lindex [split $inLine] 1]
            set tsUnit [lindex [split $inLine " "] 2] 

            # default tsUnit of tsVal to ns
            if {$tsUnit == "us"} {
                lappend tsIn [expr 1000 * $tsVal]
            } elseif {$tsUnit == "ps"} {
                lappend tsIn [expr $tsVal / 1000]
            } elseif {$tsUnit == "ms"} {
                lappend tsIn [expr $tsVal * 1000000]
            } else {
                lappend tsIn [lindex [split $inLine] 1]
            }
        }   
    }
    # repeat for output file
    while {[gets $outputsFileNew outLine] != -1} { 
        incr outLineCount 
        # if first time stamp of an iteration
        if {[expr ($outLineCount) % ($numLinesOutputIteration)] == 1} {
            # lines containing the timestamp in format: "T tsVal tsUnit"
            set tsVal [lindex [split $outLine] 1]
            set tsUnit [lindex [split $outLine " "] 2] 

            # default tsUnit of tsVal to ns
            if {$tsUnit == "us"} {
                lappend tsOut [expr 1000 * $tsVal]
            } elseif {$tsUnit == "ps"} {
                lappend tsOut [expr $tsVal / 1000]
            } elseif {$tsUnit == "ms"} {
                lappend tsOut [expr $tsVal * 1000000]
            } else {
                lappend tsOut [lindex [split $outLine] 1]
            }
        }
    }

    # check number of timestamps for in and out file are equal
    if {[llength $tsIn] == [llength $tsOut]} {
        foreach outTime $tsOut inTime $tsIn {
            lappend latencyValues [expr $outTime - $inTime]
        }
        # Get throughput values of each iteration (MSa/s)
        set tempTS 0
        lappend throughputValues 0
        foreach inTime $tsIn {
            # ignore first iteration as calculation requires timestamp of previous iteration
            if {$tempTS != 0} {
                lappend throughputValues [expr 1000* $NUM_OF_SAMPLES /($inTime - $tempTS)]
            }       
            set tempTS $inTime   
        }

        # Set tcl floating point precision
        set tcl_precision 2 
        set itNum 0
        set tempLatency 0    
        foreach latency $latencyValues throughput $throughputValues {
            incr itNum
            if {$itNum >= $minNumberOfIterations} {
                # get percentage difference between current latency value and previous
                set latencyDiff [expr abs($latency - $tempLatency)]
                # if difference within 1%, set stableLatencyValue
                if {[expr 100*$latencyDiff./$latency.] < $stabilityThreshold} {
                    set stableLatency $latency
                    set stableThroughout $throughput
                    lappend stableItNum $itNum
                }  
            } 
            set tempLatency $latency  
        }
        # # Print results for all iterations 
        set itNum 0         
        foreach latency $latencyValues inTime $tsIn outTime $tsOut throughput $throughputValues {
            incr itNum
            puts $logFile [format "Iteration %2d    ts_in: %5d      ts_out: %5d ns      latency: %4d ns       throughput: %4d MSa/s" $itNum $inTime $outTime $latency $throughput]
        }
        puts $logFile "\nStable Iterations (iterations with a latency within $stabilityThreshold % of the previous iteration) = $stableItNum"
        puts $logFile "If no iterations are stable, a latency and throughput value of -1 will be reported in the status file. Please run for more iterations or increase stabilityThreshold (optional 7th argument to get_latency.tcl)"
        puts $logFile "Reported Latency:        $stableLatency ns\nReported Throughput:     $stableThroughout MSa/s"
        close $inputsFile
        close $outputsFileNew 


    } else {
        puts   "Number of first samples from each iteration in input and output files not equal"
    }    
# Only output file (no input) - such as DDS Mixer Mode 0
# In this case, throughput is calculated using output timestamps. No latency can be measured
} elseif {$OUTPUTS_ONLY == 1} {
    set tsOut {}
    set throughputValues {}

    # get number of lines in new output file
    set numLinesOutput [lindex [exec wc -l $fileOutTemp] 0]
    set numLinesOutputIteration [expr $numLinesOutput/$NITER]

    # repeat for output file
    while {[gets $outputsFileNew outLine] != -1} { 
        incr outLineCount 
        # if first time stamp of an iteration
        if {[expr ($outLineCount) % ($numLinesOutputIteration)] == 1} {
            # lines containing the timestamp in format: "T tsVal tsUnit"
            set tsVal [lindex [split $outLine] 1]
            set tsUnit [lindex [split $outLine " "] 2] 

            # default tsUnit of tsVal to ns
            if {$tsUnit == "us"} {
                lappend tsOut [expr 1000 * $tsVal]
            } elseif {$tsUnit == "ps"} {
                lappend tsOut [expr $tsVal / 1000]
            } elseif {$tsUnit == "ms"} {
                lappend tsOut [expr $tsVal * 1000000]
            } else {
                lappend tsOut [lindex [split $outLine] 1]
            }
        }
    }
    # Get throughput values of each iteration (MSa/s)
    set tempTS 0
    lappend throughputValues 0
    foreach outTime $tsOut {
        if {$outTime != 0} {
            lappend throughputValues [expr 1000* $NUM_OF_SAMPLES /($outTime - $tempTS)]
        }       
        set tempTS $outTime   
    }

    # Set tcl floating point precision
    set tcl_precision 2 
    set itNum 0
    set tempThroughput 0    
    foreach throughput $throughputValues {
        incr itNum

        if {$itNum >= $minNumberOfIterations} {
            # get percentage difference between current latency value and previous
            set throughputDiff [expr abs($throughput - $tempThroughput)]
            # if difference within 1%, set stableLatencyValue
            if {[expr 100*$throughputDiff./$throughput.] < $stabilityThreshold} {
                set stableThroughout $throughput
                lappend stableItNum $itNum
            }  
        } 
        set tempThroughput $throughput  
    }
        # # Print results for all iterations 
    set itNum 0         
    foreach throughput $throughputValues {
        incr itNum
        puts $logFile [format "Iteration %2d throughput: %4d MSa/s" $itNum $throughput]
    }
    puts $logFile "\nLatency:        $stableLatency ns\nThroughput:     $stableThroughout MSa/s"
    puts $logFile "Stable Iterations = $stableItNum"

    close $outputsFileNew 

} else {
    puts  "Files not found for latency calculation"
}

puts $statusFile "    Latency:              $stableLatency ns\n    Throughput:           $stableThroughout MSa/s"
file delete -force "$fileOutTemp"  
close $statusFile
close $logFile
