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
set tp_num_frames 1; # Num_of_frames
set seed_reset 0;

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
        set tp_num_frames [lindex $argv 15]
    }
    if {[llength $argv] > 16} {
         set seed_reset [lindex $argv 16]
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
    puts "tp_num_frames     = $tp_num_frames"
    puts "seed_reset        = $seed_reset"

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

proc isnan {x} {
    if { ![string is double $x] || $x != $x } {
        return 1
    } else {
        return 0
    }
}

proc randFloat {seed sampleType} {

   set maxfp32 3.4028235e+1
   set minfp32 1.1754944e-1
   set range [expr {($maxfp32+1) - ($minfp32)}]
   set nextSample [expr {($minfp32) + (rand() * $range)}]; # 2147483647.0 --> (2^31)-1
   return $nextSample
}

proc randBFloat16 {seed sampleType} {
    set maxbfp16 3.3895314e+4
    set minbfp16 1.1754944e-4
    set bias 127
    set mask_sign "0x80000000"
    set mask_exp "0x7F800000"
    set mask_mantissa "0x007F8000"
    set shiftbits31 31
    set shiftbits23 23
    set shiftbits7 7
    set shiftbits16 16

    set range [expr {($maxbfp16+1) - ($minbfp16)}]; #range
    set fpSample [expr {($minbfp16) + (rand() * $range)}]; # gen. of random float32 value
    set fpinhex [format %f $fpSample]; # formatted to float i.e. 6 decimal values

    #Generate the equivalent Hexadecimal number of fp32
    binary scan [binary format R $fpinhex] H* hex
    set float_int [expr 0x$hex]; # get integer value which is equivalent to above Hexadecimal value.

    # convert back bfloat16 integer to a float32 approxiation
    set sign [expr {($float_int & $mask_sign ) >> $shiftbits31}]
    set exponent [expr {(($float_int & $mask_exp) >> $shiftbits23)-$bias}]
    set mantissa [expr {(1 + [expr { double([expr {(($float_int & $mask_mantissa) >> $shiftbits16)}])/ [expr {(1 << $shiftbits7)}] }])}]
    set bfloat16_int [expr {((1-[expr {(2*$sign)}]) * ([expr {$mantissa}]) * ([expr {(1<<$exponent)}]))}]
    set nextSample [expr { ($bfloat16_int)}]

    return $nextSample
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
        set integerType 1

        if {($sampleType eq "float") || ($sampleType eq "cfloat") || ($sampleType eq "bfloat16")} {
            set integerType 0
        }
        if {($sampleType eq "float") || ($sampleType eq "cfloat")} {
            #puts "float"
            set nextSample [randFloat $sampleSeed $sampleType]
        } elseif {$sampleType eq "bfloat16"} {
            #puts "bfloat16"
            set nextSample [randBFloat16 $sampleSeed $sampleType]
        } else {
            # Random
            #puts "int16"
            set nextSample [randInt $sampleSeed $sampleType]
        }
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
        if {$comp eq 0} {
            if {$sample_idx % 8 == 0 } {
                set nextSample 8192
            }  elseif {$sample_idx % 8 == 1 || $sample_idx % 8 == 7 } {
                set nextSample 5793
            }  elseif {$sample_idx % 8 == 2 || $sample_idx % 8 == 6 } {
                set nextSample 0
            }  elseif {$sample_idx % 8 == 3 || $sample_idx % 8 == 5 } {
                set nextSample -5793
            }  else {
                set nextSample -8192
            }
        } else {
            if {$sample_idx % 8 == 0 | $sample_idx % 8 == 4 } {
                set nextSample 0
            }  elseif {$sample_idx % 8 == 1 || $sample_idx % 8 == 3 } {
                set nextSample 5793
            }  elseif {$sample_idx % 8 == 2 } {
                set nextSample 8192
            }  elseif {$sample_idx % 8 == 5 || $sample_idx % 8 == 7 } {
                set nextSample -5793
            }  else {
                set nextSample -8192
            }
        }
    } elseif {$stimType == 9 } {
        # Alternating set of zeros and ones.
        set nextSample [expr ($sample_idx % 2) ]
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
set framesInWindow [expr ($tp_num_frames)]
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

#ADF::PLIO expects data in 32-bits per text line, which for cint16 and int16 is 2 samplesPerFrame/dataPartsPerLine per line
set dataPartsPerLine 1
if {$using_plio_class == 0} {
    if {$tt_data eq "cint16" || $tt_data eq "int16" || $tt_data eq "cint32" || $tt_data eq "cfloat" || $tt_data eq "bfloat16"} {
        set dataPartsPerLine 2
    }
     if {$tt_data eq "cint8" || $tt_data eq "int8" || $tt_data eq "uint8"} {
        set dataPartsPerLine 4
    }
} else { #PLIO
    if {$tt_data eq "cint16" || $tt_data eq "int16" || $tt_data eq "uint16" || $tt_data eq "bfloat16" } {
        set dataPartsPerLine 2
    }
    if {$tt_data eq "cint8" || $tt_data eq "int8" || $tt_data eq "uint8"} {
        set dataPartsPerLine 4
    }
}

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
        # Header size = 256-bit, i.e. 4 cint32/cfloat or 8 cint16
        set header_size 4
        if {$tt_data eq "cint16"} {
            set header_size 8
        }
        if {$dataPartsPerLine == 2} {
        set blank_entry "0 0"
        puts $output_file "$fft_nifft 0"
        puts $output_file "$pt_size_pwr 0"
        } else {
        set blank_entry "0 \n0"
        puts $output_file "$fft_nifft"
        puts $output_file "0"
        puts $output_file "$pt_size_pwr"
        puts $output_file "0"
        }

        # pad. This loops starts at 2 because 2 samplesPerFrame have already been written
        for {set i 2} {$i < $header_size} {incr i} {
            puts $output_file $blank_entry
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
            set padding [expr ($padding) * 2]
        }
    }

#    puts "finished header"
#    puts $framesInWindow
#    puts $window_vsize
#    puts $header_size
#    puts $samplesPerFrame
#    puts $padding
#    puts $dataPartsPerLine
#    puts $dataStimType

    # Process FIR's Coefficient Reload Header
    if {$fir_header == 1} {

        if {[expr ($iter_nr % ($iterations / 2))] == 0} {
            # Update twice per simulation
            set newCoeffSet 1
        } else {
            set newCoeffSet 0
        }

        # Header size = 256-bit, i.e. 4 cint32/cfloat or 8 cint16/ int16 (int16 - 2 samples per line)
        # Oddly, parts of the logic is embedded in blank entry...
        if {$tt_data eq "int16" } {
            # 16 samples, but 2 samples per line
            set headerConfigSize 8
        } elseif {$tt_data eq "cint16" || $tt_data eq "int32" ||$tt_data eq "float"} {
            set headerConfigSize 8
        } else {
            set headerConfigSize 4
        }

        # Header size = 256-bit, i.e. 4 cint32/cfloat or 8 cint16/ int16 (int16 - 2 samples per line)
        if {$tt_coeff eq "int16" } {
            set coeffSamplesIn256Bits 16
        } elseif {$tt_coeff eq "cint16" || $tt_coeff eq "int32" ||$tt_coeff eq "float"} {
            set coeffSamplesIn256Bits 8
        } else {
            set coeffSamplesIn256Bits 4
        }

        if {$newCoeffSet == 1} {
            # Set Header length at coefficient array length
            #Ceiled up to 256-bits
            set headerCoeffSize  [expr  ($firLen + ($coeffSamplesIn256Bits - $firLen % $coeffSamplesIn256Bits)% $coeffSamplesIn256Bits)]
        } else {
            set headerCoeffSize 0
        }
        # puts "headerCoeffSize: $headerCoeffSize"

        # if {$dataPartsPerLine == 2} {
        # set blank_entry "0 0 "
        # puts $output_file "$headerCoeffSize 0 "
        # } else {
        # set blank_entry "0 \n0 "
        # puts $output_file "$headerCoeffSize"
        # puts $output_file "0 "
        # }

        set headerConfigSize 8
        if {$dataPartsPerLine == 2} {
        set blank_entry "0 0 "
        puts $output_file "$headerCoeffSize 0 "
        } else {
        set blank_entry "0  "
        puts $output_file "$headerCoeffSize"
        }
        # pad. other header entries are not yet used
        for {set i 1} {$i < $headerConfigSize} {incr i} {
            puts $output_file $blank_entry
        }

        # Generate Coefficient array
        if {$newCoeffSet == 1} {
            set coeffSamples $headerCoeffSize
            #Generate x2 Coeff Array size for complex types, but store in file as per dataType (dataPartsPerLine) requirements
            set coeffComplexSamplesMult 1
            if {$tt_coeff eq "cint16" || $tt_coeff eq "cint32" || $tt_coeff eq "cfloat"} {
                set coeffComplexSamplesMult 2
                # set coeffSamples [expr ($headerCoeffSize) * 2]
            }
            set coeffSamplesPerDataSample 1
            if {($tt_coeff eq "int16" && ($tt_data eq "int32" || $tt_data eq "cint32")) ||
                ($tt_coeff eq "cint16" && ($tt_data eq "cint32"))} {
                set coeffSamplesPerDataSample 2
            }
            # puts "coeffSamples: $coeffSamples"

            for {set i 0} {$i < ($coeffSamples * $coeffComplexSamplesMult / $coeffSamplesPerDataSample / $dataPartsPerLine)} {incr i} {
                for {set comp 0} {$comp < $dataPartsPerLine} {incr comp} {
                    if { $i * $dataPartsPerLine < $firLen * $coeffComplexSamplesMult } {
                        set nextCoeffSample [generateSample  $coeffStimType $nextCoeffSample $i $coeffSamples $tt_coeff $comp]
                        set nextValueToStore $nextCoeffSample
                        if {$coeffSamplesPerDataSample == 2} {
                            set nextCoeffSample [generateSample  $coeffStimType $nextCoeffSample $i $coeffSamples $tt_coeff $comp]
                            set nextValueToStore [expr ($nextValueToStore + ($nextCoeffSample << 16))]
                        }
                    } else {
                        set nextValueToStore 0

                    }
                    # puts "Next to store: $nextValueToStore, hex:  [format "x%x"  $nextValueToStore]"

                    if {$comp eq 0 && $dataPartsPerLine eq 2} {
                        puts -nonewline $output_file "$nextValueToStore "
                    } else {
                        puts $output_file "$nextValueToStore "
                    }
                }
            }

            # pad. make sure header is always 256-bits
            # No need. coeffSamples always aligned to 256-bits already.
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

    # Process Window (single frame or multiple frames in window)
    for {set winSplice 0} {$winSplice < $framesInWindow} {incr winSplice} {
        if {$seed_reset == 1} {
            set nextSample $seed
        }
        for {set sample_idx 0} {$sample_idx < $samplesPerFrame / $samplesPerLine} {incr sample_idx} {
            for {set comp 0} {$comp < $dataPartsPerLine} {incr comp} {
                set nextSample [generateSample  $dataStimType $nextSample $sample_idx $samplesPerFrame $tt_data $comp]

                if {$comp < 3 && $dataPartsPerLine eq 4} {
                    puts -nonewline $output_file "$nextSample "
                } elseif {$comp eq 0 && $dataPartsPerLine eq 2} {
                    puts -nonewline $output_file "$nextSample "
                } else {
                    puts $output_file "$nextSample "
                }
            }
        }
        #padding is only non-zero for dynamic point size, so no need to clause with dyn_pt_size
        for {set sample_idx 0} {$sample_idx < [expr ($padding)]} {incr sample_idx} {
            set padsample -1
            for {set comp 0} {$comp < $dataPartsPerLine} {incr comp} {
                if {$comp eq 0 && $dataPartsPerLine eq 2} {
                    puts -nonewline $output_file "$padsample "
                } else {
                    puts $output_file $padsample
                }
            }
        }
    }
}
