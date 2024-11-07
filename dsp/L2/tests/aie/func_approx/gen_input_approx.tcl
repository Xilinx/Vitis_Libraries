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
set usage "
For generating random stimulus for data files.
tclsh gen_input.tcl <filename> <numSamples> <iterations> \[<seed>\] \[<dataStimType>\]
Supported dataStimType
    10 = 0 to 1, scaled up to max range
    11 = random positive - floats between 0 and 1
";
if { [lsearch $argv "-h"] != -1 } {
    puts $usage
    exit 0
}
# defaults
set fileDirpath "./data"
set filename "$fileDirpath/input.txt"
set window_vsize 1024
set iterations 8
set seed 1
set dataStimType 0
set tt_data "cint16" ;# sensible default of complex data
set coarse_bits 8
set fine_bits 7
set domain_mode 0
set plioWidth 32

if { $::argc > 2} {
    set filename [lindex $argv 0]
    set fileDirpath [file dirname $filename]
    set window_vsize  [lindex $argv 1]
    set iterations  [lindex $argv 2]
    if {[llength $argv] > 3 } {
        set seed [lindex $argv 3]
    }
    if {[llength $argv] > 4 } {
        set dataStimType [lindex $argv 4]
    }
    if {[llength $argv] > 5 } {
        set tt_data [lindex $argv 5]
    }
    if {[llength $argv] > 6 } {
        set coarse_bits [lindex $argv 6]
    }
    if {[llength $argv] > 7 } {
        set fine_bits [lindex $argv 7]
    }
    if {[llength $argv] > 8 } {
        set domain_mode [lindex $argv 8]
    }
    if {[llength $argv] > 9 } {
        set plioWidth [lindex $argv 9]
    }

    puts "filename          = $filename"
    puts "window_vsize      = $window_vsize"
    puts "iterations        = $iterations"
    puts "seed              = $seed"
    puts "dataStimType      = $dataStimType"
    puts "tt_data           = $tt_data"
    puts "coarse_bits       = $coarse_bits"
    puts "fine_bits         = $fine_bits"
    puts "domain_mode       = $domain_mode"
    puts "plioWidth         = $plioWidth"
    puts "------------------------------"

}

set nextSample $seed

proc srand {seed} {
    set nextSample $seed
}


proc randInt {seed sampleType} {
   set nextSample [expr {($seed * 1103515245 + 12345)}]

   if {($sampleType eq "int8") || ($sampleType eq "cint8")} {
      return [expr (($nextSample % 256) - 128)]
    } elseif {$sampleType eq "uint8"} {
        return [expr ($nextSample % 256)]
    } elseif {($sampleType eq "int16") || ($sampleType eq "cint16")} {
        return [expr (($nextSample % 65536) - 32768)]
    } elseif {$sampleType eq "uint16"} {
        return [expr ($nextSample % 65536)]
    } elseif {($sampleType eq "int32") || ($sampleType eq "cint32")} {
        #return [expr (int($nextSample % 4294967296) -2147483648 )]
        return [expr (($nextSample % 65536) - 32768)]; # int32 is kept within int16 range bounds.
    } elseif {$sampleType eq "uint32"} {
        #return [expr abs(int($nextSample % 4294967296))]
        return [expr ($nextSample % 65536)]; # uint32 is kept within uint16 range bounds.
    } else {
        return [expr (($nextSample % 65536) - 32768)]
    }
}

proc incInt {seed sampleType} {
    set nextSample [expr {$seed +1}]
    if {($sampleType eq "int8") || ($sampleType eq "cint8")} {
        if {$nextSample > 127} {set nextSample -128}
    } elseif {($sampleType eq "uint8")} {
        if {$nextSample > 255} {set nextSample 0}
    } elseif {($sampleType eq "int16") || ($sampleType eq "cint16")} {
        if {$nextSample > 32767} {set nextSample -32768 }
    } elseif {$sampleType eq "uint16"} {
        if {$nextSample > 65535} {set nextSample 0}
    }
    return $nextSample
}

proc generateSample {stimType sampleSeed sample_idx samples sampleType comp minSample maxSample dataPartsPerLine} {

    # 0 = RANDOM
    # 5 = INCR_ONES
    if {$stimType == 0 } {
            if {($sampleType eq "float")  || ($sampleType eq "bfloat16")} {
                # create random int between 0 and 32768
                set randSample [randInt [expr {int($sampleSeed * 32768)}] "int16"]
                # covert to double between 0 and 1
                set tempSample [expr {double(abs($randSample)  / 32768.0)}]
                # scale to min and max domain
                set nextSample [expr { ($maxSample - $minSample) * $tempSample + $minSample }]
            } else {
                # create random int between 0 and 32768
                set randSample [randInt $sampleSeed "int16"]
                # covert to double between 0 and 1
                set tempSample [expr {double(abs($randSample)  / 32768.0)}]
                # scale to required domain and convert back to integer
                set nextSample [expr { int(($maxSample - $minSample) * $tempSample + $minSample) }]
            }
    } elseif {$stimType == 1 } {
        if {$dataPartsPerLine > 1} {
            set tempSample [expr {double(2 * $sample_idx + $comp)  / $samples}]
        } else {
            set tempSample [expr {double($sample_idx)  / $samples}]
        }
        if {($sampleType eq "float")  || ($sampleType eq "bfloat16")} {
            set nextSample [expr { ($maxSample - $minSample) * $tempSample + $minSample }]
        } else {
            set nextSample [expr { int(($maxSample - $minSample) * $tempSample + $minSample) }]
        }

    
    } else {
        # Unsupported default to random
        set nextSample [randInt $sampleSeed $sampleType]
    }
return $nextSample
}

# If directory already exists, nothing happens
file mkdir $fileDirpath
set output_file [open $filename w]
set headRand [srand $seed]

set samplesPerFrame  [expr ($window_vsize)]

set samplesPerLine 1
if {($tt_data eq "int16" || $tt_data eq "bfloat16")} {
    # int16s are organized in 2 samplesPerLine
    set samplesPerLine 2
}
# assume 32 bit lines, plioWidthRation applied after
set dataPartsPerLine 1
if {$tt_data eq "int16" || $tt_data eq "bfloat16" || $tt_data eq "cint16"} {
    set dataPartsPerLine 2
}

set ignoreTopBit 0
if {($domain_mode == 2)} {
    # int16s are organized in 2 samplesPerLine
    set maxDomain 4
    set minDomain 1
} elseif {($domain_mode == 1 )} {
    # int16s are organized in 2 samplesPerLine
    set maxDomain 1
    set minDomain 0
    set ignoreTopBit 1
} else {
    # int16s are organized in 2 samplesPerLine
    set maxDomain 1
    set minDomain 0
} 
if {$tt_data eq "float" || $tt_data eq "bfloat16"} {
    set isFloat 1
    set maxSampleValue $maxDomain
    set minSampleValue $minDomain
} else {
    set isFloat 0
    set maxSampleValue [expr {1 << ($coarse_bits + $fine_bits - $ignoreTopBit)}]
    set minSampleValue [expr {($maxSampleValue * $minDomain) / $maxDomain}]
}

set plioWidthRatio [expr ($plioWidth) / 32]
set dataPartsPerLine [expr $dataPartsPerLine * $plioWidthRatio]
set samplesPerLine [expr $samplesPerLine * $plioWidthRatio]

# Process iterations
for {set iter_nr 0} {$iter_nr < [expr ($iterations)]} {incr iter_nr} {
    # Process Window (single frame or multiple frames in window)
        for {set sample_idx 0} {$sample_idx < $samplesPerFrame / $samplesPerLine} {incr sample_idx} {
            for {set comp 0} {$comp < $dataPartsPerLine} {incr comp} {
                set nextSample [generateSample $dataStimType $nextSample $sample_idx $samplesPerFrame $tt_data $comp $minSampleValue $maxSampleValue $dataPartsPerLine]
                if {$comp eq ($dataPartsPerLine - 1)} {
                    puts $output_file "$nextSample "
                } else {
                    puts -nonewline $output_file "$nextSample "
                }
            }
        }
}
