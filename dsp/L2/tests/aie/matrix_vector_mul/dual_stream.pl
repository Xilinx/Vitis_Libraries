
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

use strict;
use warnings;
use Cwd;
use Cwd 'chdir';
use Getopt::Long;
use File::Basename;
use Term::ReadLine;
use File::Copy;

my $usage = "
This script will convert an array of data txt files into or from dual streams.
Split into dual streams:
    input.txt                           -> input_0.txt and input_0.txt
    input_<0:x_pos-1>.txt               -> input_<0:x_pos-1>_0.txt and input_<0:x_pos-1>_1.txt
    input_<0:x_pos-1>_<0:y_pos-1>.txt   -> input_<0:x_pos-1>_<0:y_pos-1>)_0.txt and input_<0:x_pos-1>_<0:y_pos-1>)_1.txt
Zip from dual streams
    uut_output_0.txt and uut_output_1.txt -> uut_output.txt
    uut_output_<0:x_pos-1>_0.txt and uut_output_<0:x_pos-1>_1.txt -> uut_output_<0:x_pos-1>.txt
options:
    -f|--file=s               => Required. filepath containing data to be split or resultant filepath containing interleaved data.
    [--dual=i]                => Required. Controls whether files should converted to or from dual stream files. If --dual 0, the script does nothing
    [--x_pos=i]               => Optional. If there is an array of data files. This is the fist dimension. For example, input_<0 to x_pos - 1>.txt
    [--y_pos=i]               => Optional. If there is a 2d array of data files. This is the second dimension. For example, input_<0 to x_pos-1>_<0 to y_pos-1>.txt
    [--split | --zip]         => Controls wether to split an input file into x_pos sub-files or to combine (zip) x_pos sub-files into one output file
    [-t|--type=i]             => Data type for the file - note int16 has two samples per line.
    [plioWidth=i]             => Bit width of PLIO (number of bits per line of input/output files)
    [streamWidth=i]           => Bit width of stream read/write
    [-h|--help]               => Optional. prints this usage
    [-v|--verbose]            => Optional. additional logging
";

my $file = "";
my $dualStreams = -1;
my $x_pos = 1;
my $y_pos = 0;
my $split = 0;
my $zip = 0;
my $type = "cint16";
my $findOutType = "";
my $help = 0;
my $verbose = 0;
my $plioWidth = 32;
my $streamWidth = 128;

GetOptions (
    "f|file=s" => \$file,
    "dualStreams=s" => \$dualStreams,
    "s|x_pos=i"  => \$x_pos,
    "c|y_pos=i" => \$y_pos,
    "split" => \$split,
    "zip" => \$zip,
    "t|type=s" => \$type,
    "findOutType=s" => \$findOutType,
    "plioWidth=i" => \$plioWidth,
    "streamWidth=i" => \$streamWidth,
    "h|help" => \$help,
    "v|verbose" => \$verbose)
or die("Error in command line arguments\n");
if ( $help ) {
    die "$usage";
}
# Handle mandatory arguments
if ( $file eq "" ) {
    die "ERROR: need a filename input. -h for usage"
}
# Handle mandatory arguments
if ( $dualStreams ne 0 and $dualStreams ne 1 ) {
    die "ERROR: Switch for dual streams must be used to control usage of this script.
    --dual 1                      -> the script is active and will --zip or --split files
    --dual 0                      -> this script does nothing. \n"
}
if ( ($streamWidth / $plioWidth) < 1 ) {
    die "ERROR: Width of streams ($streamWidth bits) supported by this script must be at least 1 line of plio ($plioWidth bits)"
}
if ( ($split and $zip) or ((not $split) and (not $zip)) ) {
    die "ERROR: need only split or zip. -h for usage"
}
if ( ($zip) and ($findOutType ne "") ) {
    # print "Calculating output data type depending on $type and $findOutType\n";
    # Need to find if output dataType is cin32, or cfloat
    if ($type eq "cfloat" or $findOutType eq "cfloat") {
        $type = "cfloat"
    } elsif ($type eq "cint32" or $findOutType eq "cint32") {
        $type = "cint32";
    } elsif ($type eq "int32" and $findOutType eq "int32") {
        $type = "int32";
    } elsif ($type eq "int32" and $findOutType eq "int16") {
        $type = "int32";
    } elsif ($type eq "int32" and $findOutType eq "cint16") {
        $type = "cint32";
    } elsif ($type eq "cint16" and $findOutType eq "cint16") {
        $type = "cint16";
    } elsif ($type eq "cint16" and $findOutType eq "int16") {
        $type = "cint16";
    } elsif ($type eq "cint16" and $findOutType eq "int32") {
        $type = "cint32";
    } elsif ($type eq "int16" and $findOutType eq "int16") {
        $type = "int16";
    } elsif ($type eq "int16" and $findOutType eq "cint16") {
        $type = "cint16";
    } elsif ($type eq "int16" and $findOutType eq "int32") {
        $type = "int32";

    }
    # print "Output type is $type\n";
}

# Define properties for each data type
my %type_properties = (
    'uint8'     => { numParts => 1, partSize => 8  },
    'uint16'    => { numParts => 1, partSize => 16 },
    'uint32'    => { numParts => 1, partSize => 32 },
    'int8'      => { numParts => 1, partSize => 8  },
    'int16'     => { numParts => 1, partSize => 16 },
    'int32'     => { numParts => 1, partSize => 32 },
    'float'     => { numParts => 1, partSize => 32 },
    'bfloat16'  => { numParts => 1, partSize => 16 },
    'cint8'     => { numParts => 2, partSize => 8  },
    'cint16'    => { numParts => 2, partSize => 16 },
    'cint32'    => { numParts => 2, partSize => 32 },
    'cfloat'    => { numParts => 2, partSize => 32 },
    'cbfloat16' => { numParts => 2, partSize => 16 },
);


# Get properties for the data type
my $partsPerSample = $type_properties{$type}{numParts};
my $sampleSizeBits = $type_properties{$type}{numParts} * $type_properties{$type}{partSize};

my $samplesPerLine = $plioWidth / $sampleSizeBits;
# prevent samplesPerLine = 0.5 (round up)
if (int($samplesPerLine) != $samplesPerLine) {
  $samplesPerLine = int($samplesPerLine) + 1;
}
my $partsPerLine = $plioWidth / $type_properties{$type}{partSize};
# Assumption that at least one line per stream
my $linesPerStream = $streamWidth / $plioWidth;

# For case when sample is split over two lines (cint32, cfloats with 32-bit PLIO)
my $linesPerSample = 1;
if ($plioWidth < $sampleSizeBits) {
  $linesPerSample = 2;
}
my $data_type_size_bytes = $sampleSizeBits / 8;


use integer;
# get component parts of input/output filenames
(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');
my @x_posRange = (0...($x_pos - 1));
my @y_posRange = (0...($y_pos - 1));
my @subFiles;
my @subFilesH;

# Used if int16 two samples per line
my @subFilesFinal;
my @subFilesFinalH;
my @files;

if ($split) {
    my @files;

    if ($x_pos > 0 and $y_pos == 0) {
        # One number suffix (uut_output_0.txt)
        @files = map { "./${fileName}_$_.txt" } (0...$x_pos-1);
    } elsif ($x_pos > 0 and $y_pos > 0) {
        # Two number suffix (uut_output_0_0.txt)
        for my $x (0...$x_pos-1) {
            push @files, map { "./data/${fileName}_${x}_$_.txt" } 0...$y_pos-1;
        }
    }  else {
        @files = "./data/${fileName}.txt" ;
    }

    for my $file (@files) {
        # Open the original file for reading
        open my $fh_in, '<', $file or die "Can't open file $file for reading: $!";

        # Extract the base name without extension
        my ($base_name) = $file =~ /(.*)\.txt/;

        # Construct new filenames with suffixes _0 and _1
        my $file_0 = "${base_name}_0.txt";
        my $file_1 = "${base_name}_1.txt";

        if ($dualStreams) {
            # Open new files for writing
            open my $fh_out_0, '>', $file_0 or die "Can't open file $file_0 for writing: $!";
            open my $fh_out_1, '>', $file_1 or die "Can't open file $file_1 for writing: $!";

            my $line_counter = 0;
            my $current_fh = $fh_out_0; # Start writing to file_0

            # Read from the original file and write to the new files
            while (my $line = <$fh_in>) {
                print $current_fh $line;
                $line_counter++;

                # Check if we've written $linesPerStream lines, then switch filehandles
                if ($line_counter == $linesPerStream) {
                    # if dualStream = 0, then keep writing to file 0
                    $current_fh = (($current_fh == $fh_out_0)) ? $fh_out_1 : $fh_out_0;
                    $line_counter = 0; # Reset the line counter
                }
            }
            close $fh_out_0;
            close $fh_out_1;
            close $fh_in;
        } else {
            open my $fh_out_0, '>', $file_0 or die "Can't open file $file_0 for writing: $!";
            # Copy the file
            copy($file, $file_0) or die "Copy failed: $!";

        }

        # Close the filehandles
    }
}

if ($zip) {
    my @files;
    if ($x_pos > 0 and $y_pos == 0) {
        # One number suffix (uut_output_0.txt)
        @files = map { "./data/${fileName}_$_.txt" } (0...$x_pos-1);
    } elsif ($x_pos > 0 and $y_pos > 0) {
        # Two number suffix (uut_output_0_0.txt)
        for my $x (0...$x_pos-1) {
            push @files, map { "./data/${fileName}_${x}_$_.txt" } 0...$y_pos-1;
        }
    }  else {
        # Single stream in -> copy file <file>.txt to <file>_0.txt
        @files = "./data/${fileName}.txt";
    }

    for my $file (@files) {
        # Extract the base name without extension
        my ($base_name) = $file =~ /(.*)\.txt/;

        # Construct filenames with suffixes _0 and _1
        my $file_0 = "${base_name}_0.txt";
        my $file_1 = "${base_name}_1.txt";

        if ($dualStreams) {
            # Open the split files for reading
            open my $fh_in_0, '<', $file_0 or die "Can't open file $file_0 for reading: $!";
            open my $fh_in_1, '<', $file_1 or die "Can't open file $file_1 for reading: $!";

            # Open the output file for writing
            open my $fh_out, '>', $file or die "Can't open file $file for writing: $!";

            my $line_counter = 0;
            my $current_fh = $fh_in_0; # Start reading from file_0

            # Read from the split files and write to the output file
            while (1) {
                my $line = <$current_fh>;
                last unless defined $line; # Exit loop if no more lines

                print $fh_out $line;
                $line_counter++;

                # Check if we've read $linesPerStream lines, then switch filehandles
                if ($line_counter == $linesPerStream) {
                    $current_fh = ($current_fh == $fh_in_0) ? $fh_in_1 : $fh_in_0;
                    $line_counter = 0; # Reset the line counter
                }
            }

            # Close the filehandles
            close $fh_in_0;
            close $fh_in_1;
            close $fh_out;
        } else {
            # Single stream out -> copy file <file>_0.txt to <file>.txt
            copy($file_0, $file) or die "Copy failed: $!";

        }
    }
}
