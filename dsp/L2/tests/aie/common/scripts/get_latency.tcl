#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2023, Advanced Micro Devices, Inc.
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
set OUT_DIR                 [lindex $argv 0]
set STATUS_FILE             [lindex $argv 1]
set NUM_OF_SAMPLES          [lindex $argv 2]
set NITER                   [lindex $argv 3]
if { [llength $argv] > 4 } {
    set stabilityThreshold           [lindex $argv 4]
} else {
    set stabilityThreshold 3
}

set statusFile              [open $STATUS_FILE a]
set logFile                 [open "./logs/iterationStats.txt" w]


set fileIn  [glob -nocomplain -directory $OUT_DIR -- "T_input_0_0.txt"]
set fileOut [glob -nocomplain -directory $OUT_DIR -- "data/uut_output_0_0.txt"]
# If above files not found, check for files in following format (occurs for matrix_mult)
if {![llength $fileIn] && ![llength $fileOut]} {
    set fileIn  [glob -nocomplain -directory $OUT_DIR -- "T_inputA_0.txt"]
    set fileOut [glob -nocomplain -directory $OUT_DIR -- "data/uut_output.txt"]
} 
set fileOutNew "$OUT_DIR/data/new_uut_output_0_0.txt"

# Latency during first few iterations typically not stable
set minNumberOfIterations 4
# if no stable latency or throughput determined:
set stableLatency -1
set stableThroughout -1

# Open files if exist
set fexist1 [file exist $fileIn]
set fexist2 [file exist $fileOut]
if {$fexist1} {
    set inputsFile [open $fileIn r]        
}
if {$fexist2} {
    # remove lines containing "TLAST" from output file, create new output file
    exec sed {/TLAST/d} $fileOut > $fileOutNew    
    set outputsFileNew [open $fileOutNew r]    
}

# Both input and output file are present
if {$fexist1 && $fexist2} {
    set tsIn {}
    set tsOut {}
    set latencyValues {}
    set throughputValues {}
    set stableItNum {}

    # get number of lines for in/out files
    set numLinesInput [lindex [exec wc -l $fileIn] 0]
    set numLinesOutput [lindex [exec wc -l $fileOutNew] 0]
    
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
        
        # Get latency values for each iteration (first ouput TS - first input TS)
        foreach outTime $tsOut inTime $tsIn {
            lappend latencyValues [expr $outTime - $inTime]
        }
        # Get throughput values of each iteration (MSa/s)
        set tempTS 0
        lappend throughputValues 0
        foreach inTime $tsIn {
            if {$inTime != 0} {
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
        puts $logFile "\nLatency:        $stableLatency ns\nThroughput:     $stableThroughout MSa/s"
        puts $logFile "Stable Iterations = $stableItNum"
        close $inputsFile
        close $outputsFileNew 


    } else {
        puts   "Number of input and output timestamps not equal"
    }    
# Only output file (no input) - such as DDS Mixer Mode 0
# In this case, throughput is calculated using output timestamps. No latency can be measured
} elseif {!$fexist1 && $fexist2} {
    set tsOut {}
    set throughputValues {}

    # get number of lines in new output file
    set numLinesOutput [lindex [exec wc -l $fileOutNew] 0]
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

close $statusFile
close $logFile
