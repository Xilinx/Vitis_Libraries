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

#$(INPUT_HEADER_FILE) $(WINDOW_VSIZE) $(NITER) $(DATA_SEED) $(STIM_TYPE) $(DYN_PT_SIZE) $(POINT_SIZE) $(DATA_TYPE) ;

set usage "
For generating random stimulus for data files.
tclsh gen_mrfft_header.tcl <filename> <windowVSize> <iterations> <dynPtSize> <maxPtSize> <dataType> \[<seed>\] \[<dataStimType>\]
Supported dataStimType
    0 = RANDOM
    1 = INCREASING
    2 = DECREASING
    3 = UNIFORM
";
if { [lsearch $argv "-h"] != -1 } {
    puts $usage
    exit 0
}
# defaults
set fileDirpath "./data"
set filename "$fileDirpath/mrfft_header_input.txt"
set window_vsize 1024
set iterations 8
set dyn_pt_size 0
set maxPtSize 4000
set tt_data "cint16" ;# sensible default of complex data
set seed 1
set dataStimType 0
set pointSizesFile "header_pointsizes.txt"
if { $::argc > 2} {
    set filename [lindex $argv 0]
    set fileDirpath [file dirname $filename]
    set window_vsize  [lindex $argv 1]
    set iterations  [lindex $argv 2]
    set dyn_pt_size  [lindex $argv 3]
    set maxPtSize  [lindex $argv 4]
    set tt_data  [lindex $argv 5]
    if {[llength $argv] > 6 } {
        set seed [lindex $argv 6]
    }
    if {[llength $argv] > 7 } {
        set dataStimType [lindex $argv 7]
    }
    puts "filename     = $filename"
    puts "window_vsize = $window_vsize"
    puts "iterations   = $iterations"
    puts "dyn_pt_size  = $dyn_pt_size"
    puts "maxPtSize    = $maxPtSize"
    puts "tt_data      = $tt_data"
    puts "seed         = $seed"
    puts "dataStimType = $dataStimType"
    puts "------------------------"

}

puts "pointsizesfile $pointSizesFile"


set nextSample $seed

proc srand {seed} {
    set nextSample $seed
}

proc randInt {seed sampleType range} {
   set nextSample [expr {($seed * 1103515245 + 12345)}]

   if {($sampleType eq "int8") || ($sampleType eq "cint8")} {
      return [expr floor((($nextSample % 256) - 128)*$range/128)]
    } elseif {$sampleType eq "uint8"} {
        return [expr floor($nextSample % 256)]
    } elseif {($sampleType eq "int16") || ($sampleType eq "cint16")} {
        return [expr floor((($nextSample % 65536) - 32768)*$range/32768)]
    } elseif {$sampleType eq "uint16"} {
        return [expr floor($nextSample % 65536)]
    } elseif {($sampleType eq "int32") || ($sampleType eq "cint32")} {
        #return [expr (int($nextSample % 4294967296) -2147483648 )]
        return [expr floor((($nextSample % 65536) - 32768)*$range/32768)]; # int32 is kept within int16 range bounds.
    } elseif {$sampleType eq "uint32"} {
        #return [expr abs(int($nextSample % 4294967296))]
        return [expr floor($nextSample % 65536)]; # uint32 is kept within uint16 range bounds.
    } else {
        return [expr floor((($nextSample % 65536) - 32768)*$range/32768)]
    }
}

# set nR2, nR3, nR5 TO BE DELETED
proc generateNumbersOfRadix {sampleSeed sampleType pointsize} {

    # initialisations
    set mult_incr 1
    set nR2 0
    set nR3 0
    set nR5 0

    while {$mult_incr <= $pointsize} {
        # pick 2, 3 or 5 randomly
        #set r [randInt $sampleSeed $sampleType 3]
        set r [expr {floor(rand() * 4)}]
        #puts "r $r"
        if {$r eq 0.0} { ;# 1.0
            set radix 2
            set nR2 [expr ($nR2 + 1)]
        } elseif {$r eq 1.0} { ;# -2.0
            set radix 3
            set nR3 [expr ($nR3 + 1)]
        } elseif {$r eq 2.0} { ;# -3.0
            set radix 5
            set nR5 [expr ($nR5 + 1)]
        } elseif {$r eq 3.0} { ;# catches case (allows for smaller numbers)
            set radix 1
        } 

        set mult_incr [expr ($mult_incr * $radix)]
        #puts $mult_incr
        #puts [expr ($mult_incr <= $pointsize)]
    }

    #catch the case when the last radix number takes the pointsize above the maxPtSize
    if {$mult_incr > $pointsize} {
        ## Note: could include errorStatus assignment here if not doing in kernel
        if {$radix == 2} {
            set nR2 [expr ($nR2 - 1)]
            puts "GOT POINTSIZE [expr ($mult_incr / 2)]"
        } elseif {$radix == 3} {
            set nR3 [expr ($nR3 - 1)]

            # room to choose one more R2 potentially...
            set r [expr {floor(rand() * 3)}]  ;# choose either radix 2 (increment) or radix 3 (do nothing) or neither
            if {$r eq 0.0} {
                set radix 2
                set nR2 [expr ($nR2 + 1)]
                set mult_incr [expr ($mult_incr * $radix)]
            } elseif {$r eq 1.0} { ;# -2.0
                set radix 3
            } elseif {$r eq 2.0} { ;# catches case (allows for smaller numbers)
                set radix 1
            }
            puts "GOT POINTSIZE [expr ($mult_incr / 3)]"
        } elseif {$radix == 5} {
            set nR5 [expr ($nR5 - 1)]
            # room to choose one more R2 or R3 potentially
            set r [expr {floor(rand() * 4)}]  ;# choose either radix 2 (increment) or radix 3 or radix 5 or neither
            #puts "r $r"
            if {$r eq 0.0} { ;# 1.0
                set radix 2
                set nR2 [expr ($nR2 + 1)]
                set mult_incr [expr ($mult_incr * $radix)]
            } elseif {$r eq 1.0} { ;# -2.0
                set radix 3
                set nR3 [expr ($nR3 + 1)]
                set mult_incr [expr ($mult_incr * $radix)]
            } elseif {$r eq 2.0} { ;# -3.0
                set radix 5
            } elseif {$r eq 3.0} { ;# catches case (allows for smaller numbers)
                set radix 1
            } 
            puts "GOT POINTSIZE [expr ($mult_incr / 5)]"
        }
    }

    return [list $nR2 $nR3 $nR5]
}

# set nR2, nR3, nR5
proc generateNumbersOfRadix {pointsize pointSizesFile} {

    # initialisations
    set generatedPointsize [expr {(int($pointsize) + 1)}]
    set nR2max [expr {floor(log($pointsize)/log(2))}]
    set nR3max [expr {floor(log($pointsize)/log(3))}]
    set nR5max [expr {floor(log($pointsize)/log(5))}]
    #puts "$nR2max , $nR3max , $nR5max"

    while {$generatedPointsize > $pointsize} {
        # pick numbers of 2, 3 or 5 randomly

        set nR2 [expr {int(rand() * ($nR2max -3 + 1)) + 3}] ;# produces only valid number of R2's
        set nR3 [expr {int(rand() * ($nR3max + 1))}]
        set nR5 [expr {int(rand() * ($nR5max + 1))}]
        #puts "$nR2 , $nR3 , $nR5"

        set generatedPointsize [expr {int(pow(2, $nR2) * pow(3, $nR3) * pow(5, $nR5))}]
    }
    puts $pointSizesFile $generatedPointsize
    return [list $nR2 $nR3 $nR5]
}

# get random boolean (for invFFT)
proc getBooleanInt {} {

    #set b [randInt $sampleSeed $sampleType 2]
    set b [expr {int(rand() * 2)}]
    #set b 0
    return $b
}


### MAIN ###
expr {srand($seed)}

# OPEN FILE
# If directory already exists, nothing happens
file mkdir $fileDirpath
set output_file [open $filename w]
set input_pts_file [open $pointSizesFile w]
#ensure that a sim stall doesn't occur because of insufficient data (yes that would be a bug)
set overkill 1
set padding 0

# errorStatus (0 since this is a filler currently and will be assigned meaningfully in first kernel)
set errorStatus 0

for {set i 1} {$i <= $iterations} {incr i} {


    # set invFFT (either 0 or 1)
    set invFFT [getBooleanInt]

    # set nR2, nR3, nR5
    #set numRadix [list 2 1 2]
    set numRadix [generateNumbersOfRadix $maxPtSize $input_pts_file]
    set nR2 [lindex $numRadix 0]
    set nR3 [lindex $numRadix 1]
    set nR5 [lindex $numRadix 2]


    # POPULATE HEADER
    set headerConfigSize 8
    # pad. make sure header is always 256-bits
    if {$tt_data eq "cint16"} {
        #puts "WRITING for cint16"
        puts $output_file "$invFFT 0 "
        puts $output_file "$nR2 $nR3 "
        puts $output_file "$nR5 0 "
        puts $output_file "0 $errorStatus "
        set headerConfigSizeLeft 4
        set dataPartsPerLine 2
    } elseif {$tt_data eq "cint32" || $tt_data eq "cfloat"} {
        #puts "WRITING for cint32 or cfloat"
        puts $output_file "$invFFT "
        puts $output_file "0 "
        puts $output_file "$nR2 "
        puts $output_file "$nR3 "
        puts $output_file "$nR5 "
        puts $output_file "0 "
        puts $output_file "0 "
        puts $output_file "$errorStatus "
        set headerConfigSizeLeft 0
        set dataPartsPerLine 1
    }
    #puts "WRITING for blanks"
    for {set lineNum 1} {$lineNum <= $headerConfigSizeLeft} {incr lineNum} {
        if {$dataPartsPerLine == 2} {
            set blank_entry "0 0 "
            puts $output_file $blank_entry
        } else {
            set blank_entry "0 "
            puts $output_file $blank_entry
    }
    }

    #for clarity in output file
    #puts $output_file ""
} ;# end for (iterations)