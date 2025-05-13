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
set usage "
For generating random stimulus for data files.
tclsh gen_input.tcl <filename> <numSamples> <iterations> \[<seed>\] \[<dataStimType>\]
Supported dataStimType
    0 = RANDOM
    3 = IMPULSE
    4 = ALL_ONES
    5 = INCR_ONES
    6 = ALL 10000s
    7 = cos/sin, non-modal, i.e. not a harmonic of window period, amplitude 10000
    8 = 45 degree spin
    9 = ALT_ZEROES_ONES
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
set dyn_pt_size 0
set max_pt_size_pwr 10
set tt_data "cint16" ;# sensible default of complex data
set tp_api 0 ;#  for high throughput fft (planned)
set using_plio_class 0 ;# default (backwards compatible)
set par_power 0 ;# modifies mimimum frame size when in dynamic mode
set coeff_reload_mode 0;
set tt_coeff "int16" ;# sensible default
set coeffStimType 0 ;# FIR length
set firLen 16 ;# FIR length
set plioWidth 32
set headSize 0
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
        set dyn_pt_size [lindex $argv 5]
    }
    if {[llength $argv] > 6 } {
        set max_pt_size_pwr [lindex $argv 6]
    }
    if {[llength $argv] > 7 } {
        set tt_data [lindex $argv 7]
    }
    if {[llength $argv] > 8 } {
        set tp_api [lindex $argv 8]
    }
    if {[llength $argv] > 9 } {
        set using_plio_class [lindex $argv 9]
    }
    if {[llength $argv] > 10 } {
        set par_power [lindex $argv 10]
    }
    if {[llength $argv] > 11 } {
        set coeff_reload_mode [lindex $argv 11]
    }
    if {[llength $argv] > 12 } {
        set tt_coeff [lindex $argv 12]
    }
    if {[llength $argv] > 13 } {
        set coeffStimType [lindex $argv 13]
    }
    if {[llength $argv] > 14 } {
        set firLen [lindex $argv 14]
    }
    if {[llength $argv] > 15 } {
        set plioWidth [lindex $argv 15]
    }
    if {[llength $argv] > 16 } {
        set headSize [lindex $argv 16]
    }
    puts "filename          = $filename"
    puts "window_vsize      = $window_vsize"
    puts "iterations        = $iterations"
    puts "seed              = $seed"
    puts "dataStimType      = $dataStimType"
    puts "dyn_pt_size       = $dyn_pt_size"
    puts "max_pt_size_pwr   = $max_pt_size_pwr"
    puts "tt_data           = $tt_data"
    puts "tp_api            = $tp_api"
    puts "using_plio_class  = $using_plio_class"
    puts "par_power         = $par_power"
    puts "coeff_reload_mode = $coeff_reload_mode"
    puts "tt_coeff          = $tt_coeff"
    puts "coeffStimType     = $coeffStimType"
    puts "firLen            = $firLen"
    puts "plioWidth         = $plioWidth"
    puts "headSize          = $headSize"
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

proc generateSample {stimType sampleSeed sample_idx samples sampleType comp} {

    # 0 = RANDOM
    # 3 = IMPULSE
    # 4 = ALL_ONES
    # 5 = INCR_ONES
    # 6 = ALL 10000s
    # 7 = cos/sin, non-modal, i.e. not a harmonic of window period, amplitude 10000
    # 8 = 45 degree spin
    # 9 = ALT_ZEROES_ONES

    if { $stimType == 0 } {
        # Random
        set nextSample [randInt $sampleSeed $sampleType]

    } elseif { $stimType == 3 } {
        # Impulse
        if {$sample_idx == 0} {
            set nextSample 1
        } else {
            set nextSample 0
        }
    } elseif { $stimType == 4 } {
        # All Ones
        set nextSample 1
    } elseif { $stimType == 5 } {
        # Incrementing patttern
        set nextSample [incInt $sampleSeed $sampleType]

        # Only increment on real part?
        # if {$comp == 0} {
        #     set nextSample [expr ($sampleSeed+1)]
        # } else {
        #     set nextSample [expr ($sampleSeed+0)]
        # }
        # Modulo 256? Is there any need for large numers?
    } elseif { $stimType == 6 } {
        # all 10000
        set nextSample 10000
    } elseif { $stimType == 7 } {
        # 7 = cos/sin, non-modal, i.e. not a harmonic of window period, amplitude 10000
        set integerType 1
        if {($sampleType eq "float") || ($sampleType eq "cfloat")} {
            set integerType 0
        }
        #if real part ...
        if { (($sampleType eq "cint16") && ($comp == 0)) || (($sample_idx % 2 == 0) && (($sampleType eq "cint32") || ($sampleType eq "cfloat"))) } {
            set theta [expr {10.0*$sample_idx/$samples}]
            set nextSample [expr {10000.0 * cos($theta)}]
            if {$integerType == 1} {
                set nextSample [expr {int($nextSample)}]
            }
            #puts "cos = $nextSample"
        } else {
             #imaginary part
            if {$sampleType eq "cint16"} {
                set theta [expr {10.0*$sample_idx/$samples}]
            } else {
                #this isn't a 'new' sample, just the second part of the sample, hence the modification to sample_idx
                set theta [expr {10.0*($sample_idx-1)/$samples}]
            }
            set nextSample [expr {10000.0 * sin($theta)}]
            if {$integerType == 1} {
                set nextSample [expr {int($nextSample)}]
            }
            #puts "sin = $nextSample\n"
        }
    } elseif { ($stimType == 8) } {
        #This is a horrible kludge for $cint32 and $cfloat because this script confuses $linesPerSample and $samplesPerLine
        if {$sampleType eq "cint32" || $sampleType eq "cfloat"} {
            set mod [expr ($sample_idx*2+$comp) % 16 ]
            if {$mod == 0 || $mod == 5} {
                set nextSample 8192
            }  elseif {$mod == 2 || $mod == 3 || $mod == 7 || $mod == 14 } {
                set nextSample 5793
            }  elseif {$mod == 1 || $mod == 4 || $mod == 9 || $mod == 12 } {
                set nextSample 0
            }  elseif {$mod == 6 || $mod == 10 || $mod == 11 || $mod == 15 } {
                set nextSample -5793
            }  else {
                set nextSample -8192
            }
        } else {
            set mod [expr ($sample_idx*4+$comp) % 16 ]
            if {$mod == 0 || $mod == 5} {
                set nextSample 8192
            }  elseif {$mod == 2 || $mod == 3  || $mod == 7 || $mod == 14}  {
                set nextSample 5793
            }  elseif {$mod == 1 || $mod== 4 || $mod == 9 || $mod== 12 } {
                set nextSample 0
            }  elseif {$mod == 6 || $mod == 10 || $mod == 11 || $mod == 15 } {
                set nextSample -5793
            }  else {
                set nextSample -8192
            }
        }
    } elseif {$stimType == 9 } {
        # Alternating set of zeros and ones.
        set nextSample [expr ($sample_idx % 2) ]
        # Hazard for cint32 type, which has double the amount of samples, so all real get even index and all imag get odd.
    } elseif {$stimType == 10 } {
        # Alternating set of zeros and ones.
        if {($sampleType eq "float") || ($sampleType eq "bfloat16")} {
            set integerType 0
            if {($sampleType eq "bfloat16")} {
                # set tcl_precision 4
                set nextSample [expr {double(2 * $sample_idx + $comp)  / $samples}]
            } else {
                set tcl_precision 7
                set nextSample [expr {double($sample_idx)  / $samples}]
            }

        } else {
            if {($sampleType eq "int16")} {
                set nextSample [expr {((2 * $sample_idx + $comp) * 32768 / $samples) }]
            } else {
                set nextSample [expr {(($sample_idx) * 32768 / $samples) }]
            }
        }
    } elseif {$stimType == 11 } {
        if {($sampleType eq "float")  || ($sampleType eq "bfloat16")} {
            set randSeed $sample_idx
            set randSample [randInt $randSeed "int16"]
            set integerType 0
            set tcl_precision 17
            set nextSample [expr {abs($randSample)  / 32768.0}]
        } else {
            set randSample [randInt $sampleSeed $sampleType]
            set nextSample [expr {abs($randSample)}]
        }
        # Hazard for cint32 type, which has double the amount of samples, so all real get even index and all imag get odd.
        # set nextSample [expr {($sample_idx / $samples) * 32768} ]
        # Hazard for cint32 type, which has double the amount of samples, so all real get even index and all imag get odd.
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
#ensure that a sim stall doesn't occur because of insufficient data (yes that would be a bug)
set overkill 1
set padding 0
set pt_size_pwr $max_pt_size_pwr+1
set framesInWindow 1
set samplesPerFrame  [expr ($window_vsize)]
set fir_header 0
set nextCoeffSample 0
if {$coeff_reload_mode == 2} {
    set fir_header 1
}

#ADF::PLIO class expects data in 32-bits per text line, which for cint32 & cfloat is half a sample per line.
if {$using_plio_class == 1 && ($tt_data eq "cint32" || $tt_data eq "cfloat")} {
    set samplesPerFrame [expr ($samplesPerFrame) * 2]
}
set samplesPerLine 1
if {($tt_data eq "int16" || $tt_data eq "uint16" || $tt_data eq "bfloat16")} {
    # int16s are organized in 2 samplesPerLine
    set samplesPerLine 2
}

if {($tt_data eq "int8" || $tt_data eq "uint8" || $tt_data eq "cint8")} {
    # int8 values are organized in 4 samplesPerLine
    set samplesPerLine 4
}
set plioWidthRatio [expr ($plioWidth) / 32]

#ADF::PLIO expects data in 32-bits per text line, which for cint16 and int16 is 2 samplesPerFrame/dataPartsPerLine per line
set dataPartsPerLine 1
if {$using_plio_class == 0} {
    if {$tt_data eq "cint16" || $tt_data eq "int16" || $tt_data eq "cint32" || $tt_data eq "cfloat"} {
        set dataPartsPerLine 2
    }
} else { #PLIO
    if {$tt_data eq "cint16" || $tt_data eq "int16" || $tt_data eq "uint16" || $tt_data eq "bfloat16" } {
        set dataPartsPerLine 2
    }
    if {$tt_data eq "cint8" || $tt_data eq "int8" || $tt_data eq "uint8"} {
        set dataPartsPerLine 4
    }
}
set dataPartsPerLine [expr $dataPartsPerLine * $plioWidthRatio]
set samplesPerLine [expr $samplesPerLine * $plioWidthRatio]




# Coeff parts may be different than the data type part, e.g. cint32 data and int16/cint16 coeffs.
# However, coeffs are embedded in data stream, hence coeffParts is set to whatever dataPartsPerLine is.
set coeffParts 1
if {$using_plio_class == 0} {
    if {$tt_coeff eq "cint16" || $tt_coeff eq "int16" || $tt_coeff eq "cint32" || $tt_coeff eq "cfloat"} {
        set coeffParts 2
    }
} else { #PLIO
    if {$tt_coeff eq "cint16" || $tt_coeff eq "int16" } {
        set coeffParts 2
    }
}

# flag to be set when minimum dyn_pt_size is reached. Used to keep pt_size_pwr at max_pt_pwer
set endOfDynPt 0
# Process iterations
for {set iter_nr 0} {$iter_nr < [expr ($iterations*$overkill)]} {incr iter_nr} {

    # Process FFT's dynamic Point Size Header
    if {$dyn_pt_size == 1} {
        set headRand [randInt $headRand $tt_data]
        # use fields of the random number to choose FFT_NIFFT and PT_SIZE_PWR. Choose a legal size
        set fft_nifft [expr (($headRand >> 14) % 2)]
        if {!$endOfDynPt} {
            set pt_size_pwr [expr ($pt_size_pwr - 1)]
        }
        if {$pt_size_pwr < (4+$par_power)} {
            set pt_size_pwr $max_pt_size_pwr
            set endOfDynPt 1
        }
        # Header size = 256-bit, i.e. 4 cint32/cfloat or 8 cint16. It is 512 for AIEMLv2, so header size
        #headSize = 32 when AIE or AIE-ML, 64 for AIE-MLv2
        #headSize is in bytes, header size in samples, so convert (/8 for cint32 or cfloat, /4 for cint16)
        set header_size [expr ($headSize/8) ]
        if {$tt_data eq "cint16"} {
            set header_size [expr ($headSize/4) ]
        }

        if {$dataPartsPerLine == 2} {
            puts $output_file "$fft_nifft 0"
            puts $output_file "$pt_size_pwr 0"
        } elseif {$dataPartsPerLine == 4} {
            puts $output_file "$fft_nifft 0 $pt_size_pwr 0"
        } else {
            set blank_entry "0 \n0"
            puts $output_file "$fft_nifft"
            puts $output_file "0"
            puts $output_file "$pt_size_pwr"
            puts $output_file "0"
        }
        # 2 headers samples already written. 2 * (header_size - 2) parts still to write
        for {set i 0} {$i < ( (2 * ($header_size - 2))) } {incr i} {
            if {$i % $dataPartsPerLine eq ($dataPartsPerLine - 1)} {
                puts $output_file  "0"
            } else {
                puts -nonewline $output_file  "0 "
            }
        }


        set samplesPerFrame [expr (1 << $pt_size_pwr)]
        set padding 0
        if { $pt_size_pwr < $max_pt_size_pwr } {
            set padding [expr ((1 << $max_pt_size_pwr) - $samplesPerFrame)]
        }
        set framesInWindow [expr (($window_vsize)/($samplesPerFrame+$padding))]
        #ADF::PLIO class expects data in 32-bits per text line, which for cint32 & cfloat is half a sample per line.
        if {$using_plio_class == 1 && ($tt_data eq "cint32" || $tt_data eq "cfloat")} {
            #TODO. There is a confusing mix of concepts regarding samplesPerFrame per line and complex numbers here.
            #for complex numbers split over two lines the number of samplesPerFrame is doubled, but really this should be using $comp.
            #alas, $comp is used to insert newlines because it is mixed with samplesPerFrame per line. Tangle!
            set samplesPerFrame [expr ($samplesPerFrame) * 2]
        }
    }

#    puts "finished header"
#    puts $framesInWindow
#    puts $window_vsize
#    puts $headerConfigSize
#    puts $samplesPerFrame
#    puts $padding
#    puts $dataPartsPerLine
#    puts $dataStimType

    puts "framesInWindow   = $framesInWindow"
    puts "samplesPerFrame  = $samplesPerFrame"
    puts "samplesPerLine   = $samplesPerLine"
    puts "dataPartsPerLine = $dataPartsPerLine"
    # Process Window (single frame or multiple frames in window)
    for {set winSplice 0} {$winSplice < $framesInWindow} {incr winSplice} {
        for {set sample_idx 0} {$sample_idx < $samplesPerFrame / $samplesPerLine} {incr sample_idx} {
            for {set comp 0} {$comp < $dataPartsPerLine} {incr comp} {
                set nextSample [generateSample  $dataStimType $nextSample $sample_idx $samplesPerFrame $tt_data $comp]
                if {$comp < $dataPartsPerLine-1} {
                    puts -nonewline $output_file "$nextSample "
                } else {
                    puts $output_file "$nextSample "
                }
            }
        }
        #padding is only non-zero for dynamic point size, so no need to clause with dyn_pt_size
        for {set part_idx 0} {$part_idx < [expr ($padding) * 2]} {incr part_idx} { #each pad has two parts
            set padsample -1
            # check if part_idx is at end of line
            if {$part_idx % $dataPartsPerLine == $dataPartsPerLine - 1} {
                puts $output_file $padsample
            } else {
                puts -nonewline $output_file "$padsample "
            }
        }
    }
}
