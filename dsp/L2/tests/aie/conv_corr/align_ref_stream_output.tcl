#
# Copyright (C) 2019-2022, Xilinx, Inc.
# Copyright (C) 2022-2026, Advanced Micro Devices, Inc.
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

if { [lsearch $argv "-h"] != -1 || $argc < 7 } {
    puts "Usage: tclsh align_ref_stream_output.tcl <ref_file> <uut_file> <phases> <casc_len> <ref_f_len> <g_len> <data_g>"
    exit 0
}

set inp_ref_file   [lindex $argv 0]
set inp_uut_file   [lindex $argv 1]
set tp_phases      [lindex $argv 2]
set tp_casc_len    [lindex $argv 3]
set tp_window_size [lindex $argv 4]   ;# REF_F_LEN = F_LEN * NITER_UUT  (total REF samples across all iterations)
set tp_g_len       [lindex $argv 5]   ;# G_LEN
set tt_data_g      [lindex $argv 6]   ;# DATA_G type string

puts "Aligning REF stream output: phases=$tp_phases casc_len=$tp_casc_len g_len=$tp_g_len data_g=$tt_data_g ref_f_len=$tp_window_size"

# ---------------------------------------------------------------------------
# getNumOfPoints: returns the number of G-data points consumed per MAC4ROT
# lane for the given data type.  Only cint16 and int16 are supported on the
# AIE-1 stream path.  Any other type is a configuration error.
# ---------------------------------------------------------------------------
proc getNumOfPoints {tt_data} {
    if {$tt_data eq "cint16"} { return 2 } ;# cint16 = 2 scalar int16 components (real + imag) consumed per MAC4ROT lane
    if {$tt_data eq "int16"}  { return 4 } ;# int16  = 4 real points consumed per MAC4ROT lane
    puts stderr "ERROR: Unsupported tt_data_g type '$tt_data'. Only cint16 and int16 are valid for stream API."
    exit 1
}

# ---------------------------------------------------------------------------
# Compute delay_offset (= startIndex from user-guide Stream Output Alignment):
#   delay_offset = MAC4ROTDELAY * PHASES * (PhaseIncrement - 1)
#   PhaseIncrement = PHASES / streams_per_core
#
# streams_per_core is 1 when G_LEN is large enough to fill both streams of a
# single core (i.e. G_LEN > PHASES*Lanes*Points/2), otherwise 2.
#
# Note: MAC4ROTDELAY is used here via $MAC4ROTDELAY (also defined below for the
# kStartSample block) to keep both sites in sync.
# ---------------------------------------------------------------------------
set MAC4ROTDELAY  3   ;# AIE-1 MAC4ROT instruction pipeline rotation delay: 3 output cycles before results are available
set Lanes         4   ;# AIE-1 vector unit SIMD lane count per MAC4ROT cycle

set Points        [getNumOfPoints $tt_data_g]

# offset = total G-data capacity across all PHASES and SIMD lanes in one vector pass
# (= PHASES * Lanes * Points).  Half of this (offset/2) is the per-stream capacity
# threshold: if G_LEN exceeds offset/2 then a single stream per core is sufficient;
# otherwise two streams per core are required to keep all lanes busy.
set offset           [expr {$tp_phases * $Lanes * $Points}]
set streams_per_core [expr {($tp_g_len > $offset / 2) ? 1 : 2}] ;# 1 stream/core (large G) or 2 streams/core (small G)

# Guard: PHASES must be exactly divisible by streams_per_core; otherwise
# phase_incr is silently truncated and delay_offset is wrong.
if {$tp_phases % $streams_per_core != 0} {
    puts stderr "ERROR: TP_PHASES ($tp_phases) is not divisible by streams_per_core ($streams_per_core)."
    exit 1
}
set phase_incr   [expr {$tp_phases / $streams_per_core}] ;# phases assigned to each core (= TP_PHASES / streams_per_core)
set delay_offset [expr {$MAC4ROTDELAY * $tp_phases * ($phase_incr - 1)}]

# ---------------------------------------------------------------------------
# CASC_LEN >= PHASES cascade-broadcast extra alignment offset.
#
# Whenever CASC_LEN >= PHASES the F broadcast introduces additional pipeline
# latency that shifts where the UUT's first valid output appears in the
# reference stream. This extra offset is applied ONLY to the REF comparison
# window (ref_head_trim / ref_tail_trim). It must NOT be added to delay_offset
# because kStartSample is derived from delay_offset and must mirror the C++
# kernel's kStartSampleUUT which does NOT include this correction. Adding it
# to both sides would preserve the misalignment unchanged (UUT and REF shift
# by the same amount).
#
# Verified empirically for CASC_LEN==PHASES (test_101: CASC_LEN=PHASES=4):
# without this term, UUT output matched REF only when REF was shifted by an
# extra 36 samples - exactly MAC4ROTDELAY*CASC_LEN*(phase_incr-1) = 3*4*3 = 36,
# i.e. the same formula already used for CASC_LEN>PHASES. The strict '>' guard
# was excluding the CASC_LEN==PHASES case even though the same broadcast
# latency applies there too.
# ---------------------------------------------------------------------------
set casc_broadcast_offset 0  ;# default: no extra broadcast latency; overridden when CASC_LEN >= PHASES
# ">=": the broadcast latency applies for CASC_LEN == PHASES as well as CASC_LEN > PHASES;
# the strictly-greater guard was found to miss the equal case (verified empirically, test_101).
if {$tp_casc_len >= $tp_phases} {
    set casc_broadcast_offset [expr {$MAC4ROTDELAY * $tp_casc_len * ($phase_incr - 1)}]
}

# ---------------------------------------------------------------------------
# Compute kStartSample = kStartSampleUUT from conv_corr_stream_align.hpp.
#
# The cascade delay uses unsigned 32-bit wrap arithmetic to handle cases where
# (MAC4ROTDELAY - kMacsPerCore*kPoints*kDataBuffLenFactor) is negative; the
# result is reinterpreted as a signed int32 when added to G_LEN/PHASES.
#
# Constants matching conv_corr_traits.hpp (AIE-1 stream path):
#   MAC4ROTDELAY  = 3
#   kShiftFactor2 = 2   (log2 of kDataBuffLenFactor=4)
#   kMuls         = Lanes * Points
#   kMaxMuls      = kMuls * CASC_LEN * PHASES
#   kMacsPerCore  = ceil(G_LEN / kMaxMuls)
#   kCascDelayComp= uint32((MAC4ROTDELAY - kMacsPerCore*Points*(1<<kShiftFactor2))
#                          * ((CASC_LEN-1) >> log2(phase_incr)))
#   kPhaseDelay   = uint32(3*((kLastKern>>log2(pi))-1)*(pi-1) + 3*(kLastKern&(pi-1)))
#   kCascDelayLast= 0 if CASC_LEN==1, else uint32(kCascDelayComp+kPhaseDelay)
#   kStartSample  = (G_LEN + delay_offset) / PHASES + int32(kCascDelayLast)
# ---------------------------------------------------------------------------

# tcl_uint32 / tcl_int32: model C++ uint32/int32 cast semantics so that the
# cascade-delay compensation wraps correctly (matching hardware behaviour).
proc tcl_uint32 {x} { return [expr {int($x) & 0xFFFFFFFF}] }  ;# 0xFFFFFFFF = 2^32-1: mask keeping only the lower 32 bits
proc tcl_int32  {x} {
    set u [tcl_uint32 $x]
    # 0x80000000 = 2^31: the sign bit (MSB) of a 32-bit integer.
    # 0x100000000 = 2^32: subtracting it converts an unsigned value with MSB set
    # into the equivalent negative two's-complement int32.
    return [expr { ($u >= 0x80000000) ? ($u - 0x100000000) : $u }]
}

set kShiftFactor2 2   ;# log2(kDataBuffLenFactor): kDataBuffLenFactor = 1<<kShiftFactor2 = 4 (data-buffer length multiplier from conv_corr_traits.hpp)
set kMuls         [expr {$Lanes * $Points}]                                   ;# multiplications per MAC4ROT cycle (= Lanes * Points)
set kMaxMuls      [expr {$kMuls * $tp_casc_len * $tp_phases}]                 ;# total MACs across all cascade kernels and phases
set kMacsPerCore  [expr {int(ceil(double($tp_g_len) / $kMaxMuls))}]           ;# MAC4ROT operations per core per stream (G-tap groups per core)
set kLastKernPos  [expr {$tp_casc_len - 1}]                                   ;# 0-based index of the last cascade kernel; 0 when CASC_LEN=1
set kPhaseIncrMinusOne [expr {$phase_incr - 1}]
# Compute log2 of phase_incr (phase_incr is always an exact power of 2: 1, 2, 4, 8, …).
#
# Tcl's built-in log() is the natural logarithm (base-e). To obtain log base-2
# we apply the change-of-base identity: log2(x) = ln(x) / ln(2).
#
# The "+ 0.5" added to phase_incr before taking the log is a floating-point
# guard: for an exact power of 2 (phase_incr = 2^k), floating-point rounding can
# make ln(2^k)/ln(2) evaluate to a value just below the integer k (e.g., 2.9999999
# instead of 3 for k=3), causing int() to truncate incorrectly to k-1.
# Adding 0.5 makes the argument strictly greater than 2^k while still less than
# 2^(k+1), so ln(phase_incr+0.5)/ln(2) is always slightly above k and int()
# reliably truncates to the correct value k.
set kLog2PhaseIncr [expr {int(log($phase_incr + 0.5) / log(2))}]
# kRshift: cascade tier index of the last kernel = floor(kLastKernPos / phase_incr).
# Counts how many full phase_incr-sized groups of kernels precede the last kernel
# in the cascade; used to scale the inter-stage pipeline delay components.
set kRshift          [expr {$kLastKernPos >> $kLog2PhaseIncr}]

# kMacDiff mirrors kCascDelayComponent in conv_corr_stream_align.hpp: the per-tier delay
# correction.  Can be negative (wraps as uint32) when kMacsPerCore is large enough that
# the data-buffer pipeline dominates the MAC4ROT rotation delay.
set kMacDiff      [tcl_uint32 [expr {$MAC4ROTDELAY - ($kMacsPerCore * $Points << $kShiftFactor2)}]]
set kCascDelayComp [tcl_uint32 [expr {$kMacDiff * $kRshift}]]
set kPhaseDelay    [tcl_uint32 [expr {
    $MAC4ROTDELAY * ($kRshift - 1) * $kPhaseIncrMinusOne +
    $MAC4ROTDELAY * ($kLastKernPos & $kPhaseIncrMinusOne)
}]]
# kCascDelayLast is 0 for a single cascade stage (no inter-stage pipeline drain).
set kCascDelayLast [expr {($kLastKernPos == 0) ? 0 : [tcl_uint32 [expr {$kCascDelayComp + $kPhaseDelay}]]}]
set kStartSample   [expr {($tp_g_len + $delay_offset) / $tp_phases + [tcl_int32 $kCascDelayLast]}]

# ---------------------------------------------------------------------------
# Alignment windows: REF is trimmed head/tail to the valid comparison window
# (ref_tail_trim derivation below); UUT has already had its head discarded by
# the kernel, so only its tail (cascade drain samples) needs clipping here.
# ---------------------------------------------------------------------------
# ref_head_trim: samples removed from the REF head to align with UUT's first valid output.
# When CASC_LEN >= PHASES, casc_broadcast_offset accounts for the additional broadcast latency that
# shifts the UUT start position forward in the reference stream.
set ref_head_trim [expr {$delay_offset + $casc_broadcast_offset}]

# ref_tail_trim derivation — architectural, not a workaround:
#
#   CASC_LEN=1 (kLastKernPos == 0):
#     kCascDelayLast = 0  (no cascade-compensation term for a single stage)
#     kStartSampleUUT = (G_LEN + delay_offset) / PHASES = G_LEN
#                       (delay_offset = MAC4ROTDELAY*(PHASES-1)*0 = 0 when
#                        phase_incr=1, so delay_offset=0 for single-phase case)
#     The C++ kernel (conv_corr_stream_align) discards exactly G_LEN head samples
#     from the UUT stream.  To match, the reference tail must also be trimmed by
#     G_LEN — not G_LEN-1 — so both sides present the same valid window:
#       ref_valid = tp_window_size - 0 - G_LEN = F_LEN*NITER - G_LEN
#
#   CASC_LEN>1 (kLastKernPos > 0):
#     kCascDelayLast = uint32(kCascDelayComp + kPhaseDelay), which may wrap
#     negative.  When reinterpreted as int32 it reduces kStartSampleUUT by 1
#     relative to the CASC_LEN=1 case (net: G_LEN - 1 + ref_head_trim).
#     Hence ref_tail_trim = G_LEN - 1 + ref_head_trim for the multi-stage case.
#
#   Both branches are consistent with the C++ formula in conv_corr_stream_align.hpp.
set ref_tail_trim [expr {($kLastKernPos == 0) ? ($tp_g_len + $ref_head_trim) : ($tp_g_len - 1 + $ref_head_trim)}]
set ref_valid     [expr {$tp_window_size - $ref_head_trim - $ref_tail_trim}]

if {$ref_valid <= 0} {
    puts stderr "ERROR: ref_valid=$ref_valid is non-positive. Check TP_PHASES, G_LEN, and REF_F_LEN."
    exit 1
}

# uut_raw: total UUT samples after the kernel has discarded its head.
#   tp_phases * kStartSample: each of the TP_PHASES per-phase streams discards
#   kStartSample startup-transient samples independently; the total head removal
#   across all phases is tp_phases * kStartSample.
# uut_tail_clip: cascade drain samples at the end of the last invocation that must
#   be removed here (the kernel cannot know which invocation is last).
set uut_raw       [expr {$tp_window_size - $tp_phases * $kStartSample}]
set uut_tail_clip [expr {$uut_raw - $ref_valid}]

if {$uut_tail_clip < 0} {
    puts stderr "WARNING: uut_tail_clip is negative ($uut_tail_clip); clamping to 0. This may indicate a parameter mismatch."
    set uut_tail_clip 0
}

puts "  delay_offset=$delay_offset kStartSample=$kStartSample"
puts "  ref_valid=$ref_valid  uut_raw=$uut_raw  uut_tail_clip=$uut_tail_clip"

# ---------------------------------------------------------------------------
# align_outdata: trim an output text file to the window [startIndex, endIndex].
#
# Parameters:
#   input_filename  - file to trim in-place (read-then-overwrite; no backup is made
#                     before overwriting — callers must not pass a file they cannot
#                     afford to lose)
#   len_window      - number of samples in the file as passed to this proc.
#                     For REF this is tp_window_size; for UUT this is uut_raw
#                     (already head-trimmed by the C++ kernel, not the original file size).
#   len_tail_trim   - number of samples to remove from the tail
#   head_skip       - number of samples to skip from the head
#
# Samples are stored as whitespace-separated scalar tokens with real/imaginary
# parts interleaved, so all counts are multiplied by 2 (tokens per sample).
# Output is reformatted with 4 tokens per line (matching PLIO 32-bit word groups).
# ---------------------------------------------------------------------------
proc align_outdata {input_filename len_window len_tail_trim head_skip} {
    set fd [open $input_filename r]
    set indata [read $fd]
    close $fd

    set lines [split [string trim $indata] "\n"]
    set all_elements {}
    foreach line $lines {
        set line_words [regexp -all -inline {\S+} $line]
        lappend all_elements {*}$line_words
    }

    set startIndex [expr {$head_skip * 2}]                              ;# *2: 2 tokens (re+im) per sample
    set endIndex   [expr {($len_window * 2 - 1) - $len_tail_trim * 2}] ;# *2 for tokens/sample; -1 because lrange uses 0-based inclusive end index

    if {$endIndex < $startIndex} {
        puts stderr "ERROR: Degenerate trim window (startIndex=$startIndex > endIndex=$endIndex) in $input_filename."
        exit 1
    }

    set extracted [lrange $all_elements $startIndex $endIndex]
    set joined    [join $extracted " "]
    append joined " "
    set newindata [regsub -all {\s+} $joined " "]
    # Reformat as 4 tokens per line (one PLIO 32-bit word per sample pair).
    set newindata [regsub -all {(\S+\s+\S+\s+\S+\s+\S+) } $newindata "\\1 \n"]

    set fd [open $input_filename w]
    puts -nonewline $fd $newindata
    close $fd
}

# Trim REF: remove ref_head_trim samples from head and ref_tail_trim from tail.
# ref_head_trim = delay_offset + casc_broadcast_offset (where casc_broadcast_offset=0 for CASC_LEN<PHASES).
# Both head_skip and len_tail_trim are passed explicitly.
align_outdata $inp_ref_file $tp_window_size $ref_tail_trim $ref_head_trim

# Trim UUT tail (cascade drain samples at end of last invocation).
# The UUT head has already been discarded by the conv_corr_stream_align kernel,
# so head_skip=0 here.
if {$uut_tail_clip > 0} {
    align_outdata $inp_uut_file $uut_raw $uut_tail_clip 0
}

puts "Alignment complete."
