#!/usr/bin/perl -w
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

use strict;
use warnings;
use Getopt::Long;
use File::Basename;

my $usage = "
Host-side data marshalling for the transpose IP's multi-kernel (TP_SSR > 1 and/or
TP_CASC_LEN > 1) test bench. TP_SSR splits the input along its rows (--rows), TP_CASC_LEN
splits it along its columns (--cols), forming an --ssr x --cascLen grid of independent kernels:
kernel (s, c) owns a full (rowsThisLane x colsThisKernel) tile of the input and produces the
corresponding (colsThisKernel x rowsThisLane) tile of the output. Each kernel's buffer is also
zero-padded up to the device's memory access width (--memWidth), matching the padding done
inside the transpose kernel itself.

Because transposing swaps rows and columns, kernel (s, c)'s output tile lands at output block
position (c, s) - output rows [colStart(c), colStart(c)+colsThisKernel(c)) x output columns
[rowStart(s), rowStart(s)+rowsThisLane(s)) - not just concatenated in kernel order.

--split:  reads a flat, row-major (--rows x --cols) stimulus file and writes --ssr*--cascLen
          per-kernel input files, each zero-padded to (padRowsThisLane x padColsThisKernel),
          named <name>_<ssrIdx>_<cascIdx><ext> for ssrIdx = 0..ssr-1, cascIdx = 0..cascLen-1.
--strip:  reads the --ssr*--cascLen padded per-kernel UUT output files
          (<name>_<ssrIdx>_<cascIdx><ext>, each padColsThisKernel x padRowsThisLane), strips the
          padding back to the logical (colsThisKernel x rowsThisLane) region of each, places each
          at its (c, s) block position, and writes the combined flat, row-major (--cols x --rows)
          file at --outFile for comparison against the reference model output.

The reference model is always a single, unsplit tile (regardless of the UUT's --ssr/--cascLen),
but its iobuffers are padded the same way the UUT's are (every AIE iobuffer's byte size must be
a multiple of 4 bytes, which an arbitrary un-padded matrix can't generally guarantee). These two
modes pad/strip that single tile directly, with no --ssr/--cascLen block placement involved:
--pad-single:   reads a flat, row-major (--rows x --cols) file and writes a single buffer
                zero-padded to (ceil(rows,memWidth) x ceil(cols,memWidth)) to --outFile.
--strip-single: reads a single (ceil(rows,memWidth) x ceil(cols,memWidth)) padded buffer and
                writes the stripped, logical (--rows x --cols) region to --outFile.

NOTE on --rawFrameSize: gen_input.tcl (the shared script used to generate the raw stimulus file
read by --split/--pad-single) computes samplesLines=floor(samplesPerFrame/samplesPerLine) and
silently DISCARDS any remainder - if asked for a window_vsize that is not already a multiple of
its own PLIO line-packing granularity (e.g. 4 samples/line for int16 at a 64-bit PLIO width), it
truncates rather than erroring. Since TP_DIM_ROWS*TP_DIM_COLS is arbitrary here (17*23=391, not a
multiple of 4), the caller (helper.mk) must request an already-aligned window_vsize from
gen_input.tcl - e.g. the padded (ceil(rows,memWidth) * ceil(cols,memWidth)) size, since that is
always a multiple of any such small line-packing granularity - and tell these two modes the
resulting true per-iteration stride via --rawFrameSize, so they read the right slice of each
frame (the logical rows*cols region always lives at the start of each --rawFrameSize-sized frame;
anything beyond it is unused, harmless filler).

usage:
perl transpose_partition_pad.pl --file input.txt --rows 16 --cols 14 --ssr 2 --cascLen 2 \\
     --splitMode 1 --type cint16 --memWidth 8 --plioWidth 64 --niter 1 --split
perl transpose_partition_pad.pl --file uut_output.txt --outFile uut_output_combined.txt \\
     --rows 16 --cols 14 --ssr 2 --cascLen 2 --splitMode 1 --type cint16 --memWidth 8 \\
     --plioWidth 64 --niter 1 --strip
perl transpose_partition_pad.pl --file input.txt --outFile input_ref_padded.txt \\
     --rows 16 --cols 14 --type cint16 --memWidth 8 --plioWidth 64 --niter 1 --pad-single
perl transpose_partition_pad.pl --file ref_output.txt --outFile ref_output_stripped.txt \\
     --rows 14 --cols 16 --type cint16 --memWidth 8 --plioWidth 64 --niter 1 --strip-single

options:
    -f|--file=s          => split/pad-single/strip-single: input file to read. strip: filename
                            prefix (name/ext) shared by the per-kernel UUT output files
                            <name>_<ssrIdx>_<cascIdx><ext>.
    --outFile=s           => strip/pad-single/strip-single: filepath to write the result to.
    --rows=i              => TP_DIM_ROWS (split/strip); logical row count of --file (pad-single);
                            logical row count to extract (strip-single)
    --cols=i              => TP_DIM_COLS (split/strip); logical col count of --file (pad-single);
                            logical col count to extract (strip-single)
    --ssr=i               => TP_SSR (split/strip only)
    --cascLen=i           => TP_CASC_LEN (split/strip only)
    --splitMode=i         => TP_SPLIT_MODE (0 = equal, 1 = memory-access-width aligned + runt)
    -t|--type=s           => TT_DATA (e.g. cint16, int32, cfloat)
    --memWidth=i          => device memory access width, in samples, for TT_DATA (fnVecSampleNumMax)
    --rawFrameSize=i      => split/pad-single only: the actual per-iteration sample stride in the
                            raw --file being read, if different from --rows*--cols (needed when
                            that file was generated with a padded/aligned window size - see NOTE
                            below). Defaults to --rows*--cols.
    --niter=i             => number of NITER iterations concatenated in the file
    --plioWidth=i         => PLIO width in bits, for output line packing
    --split | --strip | --pad-single | --strip-single   => select operating mode (exactly one)
    -h|--help             => prints this usage
";

my $file = "";
my $outFile = "";
my $rows = 1;
my $cols = 1;
my $ssr = 1;
my $cascLen = 1;
my $splitMode = 0;
my $type = "cint16";
my $memWidth = 8;
my $rawFrameSize = 0;
my $niter = 1;
my $plioWidth = 64;
my $split = 0;
my $strip = 0;
my $padSingle = 0;
my $stripSingle = 0;
my $help = 0;

GetOptions(
    "f|file=s"     => \$file,
    "outFile=s"    => \$outFile,
    "rows=i"       => \$rows,
    "cols=i"       => \$cols,
    "ssr=i"        => \$ssr,
    "cascLen=i"    => \$cascLen,
    "splitMode=i"  => \$splitMode,
    "t|type=s"     => \$type,
    "memWidth=i"   => \$memWidth,
    "rawFrameSize=i" => \$rawFrameSize,
    "niter=i"      => \$niter,
    "plioWidth=i"  => \$plioWidth,
    "split"        => \$split,
    "strip"        => \$strip,
    "pad-single"   => \$padSingle,
    "strip-single" => \$stripSingle,
    "h|help"       => \$help,
) or die("Error in command line arguments\n");
die "$usage" if $help;
die "ERROR: need a filename via --file. -h for usage\n" if ($file eq "");
die "ERROR: need exactly one of --split, --strip, --pad-single or --strip-single. -h for usage\n"
    if (($split + $strip + $padSingle + $stripSingle) != 1);

# Data type properties, matching ssr_card_cut.pl / input_pad_and_split.pl.
my %type_properties = (
    'int16'     => { numParts => 1, partSize => 16 },
    'int32'     => { numParts => 1, partSize => 32 },
    'float'     => { numParts => 1, partSize => 32 },
    'cint16'    => { numParts => 2, partSize => 16 },
    'cint32'    => { numParts => 2, partSize => 32 },
    'cfloat'    => { numParts => 2, partSize => 32 },
);
die "ERROR: unsupported --type $type\n" unless exists $type_properties{$type};
my $partsPerSample = $type_properties{$type}{numParts};
my $partsPerLine = $plioWidth / $type_properties{$type}{partSize};

use integer;
sub ceil_mult {
    my ($x, $y) = @_;
    return (($x + $y - 1) / $y) * $y;
}

# Dimension-agnostic split math - called with (rows, ssr, ssrPos) for the row axis and
# (cols, cascLen, cascPos) for the column axis. Mirrors fnTposeColsPerKernel() in
# transpose_utils.hpp and fn_tpose_cols_per_kernel() in transpose.py.
sub splitSize {
    my ($dimSize, $splitLen, $pos) = @_;
    if ($splitMode == 0) {
        my $base = $dimSize / $splitLen;
        my $rem = $dimSize - $base * $splitLen;
        return $base + (($pos < $rem) ? 1 : 0);
    } else {
        my $avg = ($dimSize + $splitLen - 1) / $splitLen;
        my $perLane = ceil_mult($avg, $memWidth);
        return ($pos < ($splitLen - 1)) ? $perLane : ($dimSize - $perLane * ($splitLen - 1));
    }
}

my @rowsThisLane = map { splitSize($rows, $ssr, $_) } (0 .. $ssr - 1);
my @rowStart = (0);
for my $s (1 .. $ssr - 1) {
    push @rowStart, $rowStart[$s - 1] + $rowsThisLane[$s - 1];
}
my @colsThisKernel = map { splitSize($cols, $cascLen, $_) } (0 .. $cascLen - 1);
my @colStart = (0);
for my $c (1 .. $cascLen - 1) {
    push @colStart, $colStart[$c - 1] + $colsThisKernel[$c - 1];
}

(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');

sub readParts {
    my ($path) = @_;
    open(my $fh, "<", $path) or die "cannot open $path : $!";
    my @parts;
    while (my $line = <$fh>) {
        chomp $line;
        push @parts, split ' ', $line;
    }
    close($fh);
    return @parts;
}

sub writeParts {
    my ($path, @parts) = @_;
    open(my $fh, ">", $path) or die "Cannot open $path: $!";
    for (my $i = 0; $i < @parts; $i += $partsPerLine) {
        my $last = ($i + $partsPerLine - 1 < $#parts) ? $i + $partsPerLine - 1 : $#parts;
        print $fh join(' ', @parts[$i .. $last]) . " \n";
    }
    close($fh);
}

my $effRawFrameSize = $rawFrameSize > 0 ? $rawFrameSize : $rows * $cols;

if ($split) {
    my @parts = readParts($file);
    my $samplesPerIter = $effRawFrameSize;

    for my $s (0 .. $ssr - 1) {
        my $padRows = ceil_mult($rowsThisLane[$s], $memWidth);
        for my $c (0 .. $cascLen - 1) {
            my $padCols = ceil_mult($colsThisKernel[$c], $memWidth);
            my @outParts;
            for my $iter (0 .. $niter - 1) {
                my $iterBase = $iter * $samplesPerIter * $partsPerSample;
                for my $r (0 .. $padRows - 1) {
                    for my $col (0 .. $padCols - 1) {
                        if ($r < $rowsThisLane[$s] && $col < $colsThisKernel[$c]) {
                            my $sampleIdx = ($rowStart[$s] + $r) * $cols + ($colStart[$c] + $col);
                            my $partBase = $iterBase + $sampleIdx * $partsPerSample;
                            push @outParts, @parts[$partBase .. $partBase + $partsPerSample - 1];
                        } else {
                            push @outParts, (0) x $partsPerSample;
                        }
                    }
                }
            }
            my $outPath = "${fileDir}${fileName}_${s}_${c}${fileExt}";
            print "Writing kernel ($s,$c) input (padded " . ($padRows) . "x" . ($padCols) . ") to $outPath\n";
            writeParts($outPath, @outParts);
        }
    }
} elsif ($strip) {
    die "ERROR: --strip requires --outFile\n" if ($outFile eq "");
    my $samplesPerIter = $cols * $rows;
    my @combined = (0) x ($niter * $samplesPerIter * $partsPerSample);

    for my $s (0 .. $ssr - 1) {
        my $padRows = ceil_mult($rowsThisLane[$s], $memWidth);
        for my $c (0 .. $cascLen - 1) {
            my $padCols = ceil_mult($colsThisKernel[$c], $memWidth);
            my $inPath = "${fileDir}${fileName}_${s}_${c}${fileExt}";
            my @parts = readParts($inPath);
            for my $iter (0 .. $niter - 1) {
                my $iterBaseIn = $iter * $padCols * $padRows * $partsPerSample;
                my $iterBaseOut = $iter * $samplesPerIter * $partsPerSample;
                # Kernel (s,c)'s output tile is (colsThisKernel x rowsThisLane), row-major, and
                # lands at output block position (c, s): output rows
                # [colStart(c), colStart(c)+colsThisKernel(c)) x output cols
                # [rowStart(s), rowStart(s)+rowsThisLane(s)).
                for my $localCol (0 .. $colsThisKernel[$c] - 1) {
                    for my $localRow (0 .. $rowsThisLane[$s] - 1) {
                        my $inSampleIdx = $localCol * $padRows + $localRow;
                        my $inPartBase = $iterBaseIn + $inSampleIdx * $partsPerSample;
                        my $outSampleIdx = ($colStart[$c] + $localCol) * $rows + ($rowStart[$s] + $localRow);
                        my $outPartBase = $iterBaseOut + $outSampleIdx * $partsPerSample;
                        for my $p (0 .. $partsPerSample - 1) {
                            $combined[$outPartBase + $p] = $parts[$inPartBase + $p];
                        }
                    }
                }
            }
        }
    }
    print "Writing combined, unpadded output (" . ($cols) . "x" . ($rows) . ") to $outFile\n";
    writeParts($outFile, @combined);
} elsif ($padSingle) {
    # Used for the reference model's own input: it is always a single, unsplit tile, padded the
    # same way each UUT kernel's buffer is, regardless of the UUT's --ssr/--cascLen.
    die "ERROR: --pad-single requires --outFile\n" if ($outFile eq "");
    my @parts = readParts($file);
    my $padRows = ceil_mult($rows, $memWidth);
    my $padCols = ceil_mult($cols, $memWidth);
    my @outParts;
    for my $iter (0 .. $niter - 1) {
        my $iterBase = $iter * $effRawFrameSize * $partsPerSample;
        for my $r (0 .. $padRows - 1) {
            for my $c (0 .. $padCols - 1) {
                if ($r < $rows && $c < $cols) {
                    my $sampleIdx = $r * $cols + $c;
                    my $partBase = $iterBase + $sampleIdx * $partsPerSample;
                    push @outParts, @parts[$partBase .. $partBase + $partsPerSample - 1];
                } else {
                    push @outParts, (0) x $partsPerSample;
                }
            }
        }
    }
    print "Writing padded single buffer (" . $padRows . "x" . $padCols . ") to $outFile\n";
    writeParts($outFile, @outParts);
} elsif ($stripSingle) {
    # Used for the reference model's own output: strips its single padded tile back down to the
    # logical (--rows x --cols) region requested (for the ref model's output buffer, that's
    # (TP_DIM_COLS x TP_DIM_ROWS), since it has already been transposed).
    die "ERROR: --strip-single requires --outFile\n" if ($outFile eq "");
    my @parts = readParts($file);
    my $padRows = ceil_mult($rows, $memWidth);
    my $padCols = ceil_mult($cols, $memWidth);
    my @outParts;
    for my $iter (0 .. $niter - 1) {
        my $iterBase = $iter * $padRows * $padCols * $partsPerSample;
        for my $r (0 .. $rows - 1) {
            for my $c (0 .. $cols - 1) {
                my $sampleIdx = $r * $padCols + $c;
                my $partBase = $iterBase + $sampleIdx * $partsPerSample;
                push @outParts, @parts[$partBase .. $partBase + $partsPerSample - 1];
            }
        }
    }
    print "Writing stripped single buffer (" . $rows . "x" . $cols . ") to $outFile\n";
    writeParts($outFile, @outParts);
}
