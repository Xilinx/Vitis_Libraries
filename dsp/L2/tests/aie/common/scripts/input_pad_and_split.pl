#!/usr/bin/perl -w
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


use strict;
use warnings;
use Cwd;
use Cwd 'chdir';
use Getopt::Long;
use File::Basename;

use Term::ReadLine;



# TODO: accept STDIN as well as inFile.
# TODO: Granularity
# TODO: int16
# TODO: Lots of error checking


my $usage = "
This script will split an input text file into an casc number of input text files or interleave (zip) multiple files into one file.
use it thus:
perl input_pad_and_split.pl -file input.txt -type cint16 -pointsize 14 -cascLen 3 -numFrames 2 -windowSize 32
    The above will output data/input_0.txt data/input_1.txt, where input_0.txt has samples 0, 2, 4, 6, 8.. and input_1.txt has samples 1, 3, 5...
options:
    -f|--file=s                                       => filepath containing data to be split or resultant filepath containing interleaved data.
    --newFile=s
    [-t|--type=i]                                     => Data type for the file - note int16 has two samples per line.
    [-c|--cascLen=i]                                      => casc number - number of files to split over
    [--ssr=i]
    [--numFrames=i]                                    => frames of data to be operated on within a window
    [-d | --dual]                                     => If using dual input/output streams, casc phases are split into io1 and io2 with a granularity determined by the data type.
    [-h|--help]                                      => Optional. prints this usage
";

my $file = "";
my $newFile = "";
my $type = "cint16";
my $pointSize = 0;
my $cascLen = 1;
my $ssr = 1;
my $numFrames = 1;
my $variant = 1;
my $dual = 0;
my $help = 0;



GetOptions (
    "f|file=s" => \$file,
    "newFile=s" => \$newFile,
    "t|type=s" => \$type,
    "p|pointSize=i" =>\$pointSize,
    "c|cascLen=i"  => \$cascLen,
    "numFrames=i"  => \$numFrames,
    "ssr=i" => \$ssr,
    "variant=i"    => \$variant,
    "d|dual=i" => \$dual,
    "h|help" => \$help)
or die("Error in command line arguments\n");

if ( $help ) {
    die "$usage";
}

# Handle mandatory arguments
if ( $file eq "" ) {
    die "ERROR: need a filename input. -h for usage"
}


#"i|g|interleaveGranularity|granularity=i" => \$gran,
# int16 has two samples per line, but we hanlde this when we initially read file; so that it IS 1 line per sample.
my $linesPerSample = 1;
if ($type eq "cint32" or $type eq "cfloat") {
  # A single complex sample is split over two lines
  $linesPerSample = 2;
}
my $samplesPerLine = 1;
if ($type eq "int16" ) {
  # A single line has 32 bits of information, so two real int16 samples.
  $samplesPerLine = 2;
}
my $partsPerLine = 1;
if ($type eq "cint16" or $type eq "int16") {
  # 2 parts per single line
  $partsPerLine = 2;
}

my $dual_gran;
my $data_type_size_bytes;
if ($type eq "cint32")  {
    $data_type_size_bytes = 8;
} elsif ($type eq "cfloat") {
    $data_type_size_bytes = 8;
} elsif ($type eq "cint16") {
    $data_type_size_bytes = 4;
} elsif ($type eq "int32") {
    $data_type_size_bytes = 4;
} elsif ($type eq "float") {
    $data_type_size_bytes = 4;
} else {
    $data_type_size_bytes = 2;
}

my $complex = 0;
if ($type eq "cint32" || $type eq "cint16" || $type eq "cfloat") {
  $complex = 1;
}
############################################################################################
sub ceil {
    my ($x, $y) = @_;
    return int(($x + $y - 1) / $y) * $y;
}
my $kSamplesInVectData = 0;
# Get padded size for inCol
if ( $type eq "cint16") {
    $kSamplesInVectData = 256 / 8 / 4;

} elsif ( $type eq "cint32") {
    if ($variant eq 2) {
        # QoR improvement when using 512 bit vectors for cint32 on AIE-ML
        $kSamplesInVectData = 8;
    } else {
        $kSamplesInVectData = 256 / 8 / 8;
    }
} elsif ( $type eq "cfloat") {
    $kSamplesInVectData = 256 / 8 / 8;

} else {
    $kSamplesInVectData = 256 / 8 / 8;
}
# pad pointSize to a multiple of the vector size
my $paddedPointSize = ceil($pointSize, $kSamplesInVectData);
# Padding needed to split nicely across kernels
my $paddedWindowSize = $numFrames*ceil($paddedPointSize, $kSamplesInVectData*$cascLen);
print("paddedPointSize - $paddedPointSize\n");
print("paddedWindowSize = $paddedWindowSize\n");
#############################################################################################
my $headerVsize = 0;

my $dualFactor = $dual+1; #i.e. when dual=1, dualFactor=2 otherwise =1

use integer;
#128b granularity for each data type
$dual_gran = ((128/8) / $data_type_size_bytes );

print("Splitting/Zipping to casc files then each file into dual streams of granuarity ${dual_gran}.\n") if ($dual == 1);

# get component parts of input/output filenames
(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');
(my $newFileName, my $newFileDir, my $newFileExt) = fileparse($newFile, '\..*');

my @cascRange = (0...($cascLen - 1));
my @dualRange = (0...($dual));
my @subFiles;
my @subFilesH;
# Used if int16 two samples per line
my @subFilesFinal;
my @subFilesFinalH;

print "reading $file\n";
open(fileH, "<", $file)
    or die "cannot open $file : $!";

my $outFileMod = "";
# Rearrange files with multiple samples per line to 1 sample per line and twice the lines.
# This is set up for data types of 16 bits and less.
# This is required since casc splits input stream on a sample-by-sample basis.
if  ($samplesPerLine > 1) {

    # Parse input files
    open(fileH_mod, ">", "${file}_int16mod")
        or die "cannot open ${file}_int16Mod : $!";

    # split two samples per line into two lines
    while (<fileH>) {
        s/\s+(-?[0-9]+)\s?/\n$1/g ;
        print fileH_mod $_;
    }

    close(fileH)
        or die "couldn't close $file";
    close(fileH_mod)
        or die "couldn't close ${file}_int16mod";

    # reset fileH to point to the file we just created
    open(fileH, "<", "${file}_int16mod")
        or die "cannot open ${file}_int16Mod : $!";

    # Prep output file name suffix
    $outFileMod = "_beforeInt16Mod";
}


###################### PAD WITH ZEROS ###################
# Create new file file to contain padded input data
open(fileH_padded, ">", "$newFile")
    or die "cannot open newFile : $!";

# Loop through input, insert pad at end of each window
my $lineNum = 1;
my $currentFrame = 1;
while(my $line = <fileH>){
    print fileH_padded $line;
    # End of a frame once paddedPointSize lines has been reached
    if ($lineNum % ($linesPerSample*$pointSize) == 0) {
        for(my $i = 0; $i < ($paddedWindowSize/$numFrames)-$pointSize; $i++){
            if ($linesPerSample==1) {
                print fileH_padded "0 0\n";
            } else {
                print fileH_padded "0\n0\n";
            }   
        }
        $currentFrame++;   
    }    
    $lineNum++;
}
# Set fileH to new padded file
close(fileH)
    or die "couldn't close $file";
close(fileH_padded)
    or die "couldn't close $newFile";
open(fileH, "<", "$newFile")
    or die "cannot open $newFile : $!";

##########################################################

# Open a bunch of files in an array of file handles with a specific filename
for my $cascIdx (@cascRange){
    for my $dualIdx (@dualRange){
        my $fileIdx = $cascIdx*$dualFactor + $dualIdx;
        $subFiles[$fileIdx] = "${newFileDir}${newFileName}_0_${cascIdx}${newFileExt}${outFileMod}";
        print "Writing to $subFiles[$fileIdx]\n";
        open($subFilesH[$fileIdx], ">", $subFiles[$fileIdx])
            or die "cannot open $subFiles[$fileIdx] : $!";
    }
}

$lineNum = 0;
my $cascIndex = 0;
my $dualIdx = 0;
my $linesPerDualStream = $linesPerSample * $dual_gran;
my $linesPerCascDualStream = $linesPerDualStream * $cascLen;

while(my $line = <fileH>){

    my $fileIdx = $cascIndex*$dualFactor + $dualIdx;
    print {$subFilesH[$fileIdx]} $line;
    if ($lineNum % $linesPerSample == ($linesPerSample-1)) {
        $cascIndex = ($cascIndex+1) % $cascLen;
    }
    if ($lineNum % ($linesPerCascDualStream) == (($linesPerCascDualStream)-1)) {
        $dualIdx = ($dualIdx+1) % $dualFactor;
    }
    $lineNum = $lineNum+1;
}

# Rearrange tmp output files back to multiple samples per line.
if  ($samplesPerLine > 1) {

    # Simply open all the output files and force two samples per line
    for my $cascIdx (@cascRange){
        for my $dualIdx (@dualRange){
            my $fileIdx = $cascIdx* $dualFactor + $dualIdx;
            #close for Write
            close($subFilesH[$fileIdx])
                or die "cannot close $subFiles[$fileIdx] : $!";
            # Open out files for read
            open($subFilesH[$fileIdx], "<", $subFiles[$fileIdx])
                or die "cannot open $subFiles[$fileIdx] : $!";

            print "Reading from $subFiles[$fileIdx]\n";
            # intendedfinal output file without outFileMod
            $subFilesFinal[$fileIdx] = "${newFileDir}${newFileName}_0_$cascIdx${newFileExt}";
            print "Writing to $subFilesFinal[$fileIdx]\n";

            open($subFilesFinalH[$fileIdx], ">", $subFilesFinal[$fileIdx])
                or die "cannot open $subFilesFinal[$fileIdx] : $!";

            my $cascHandle = $subFilesH[$fileIdx];
            my $cascHandleFinal = $subFilesFinalH[$fileIdx];
            my $lineCount = 0;
            while(my $line = <$cascHandle>){
                # Every even line we remove the newline character.
                if ($lineCount % $samplesPerLine == 0) {
                    $line =~ s/\n/ /g ;
                }
                print $cascHandleFinal $line;
                $lineCount++;
            }
        }
    }
}
# When input is a vector to be split over cascade, it is cloned for each ssr
# print "ssr is $ssr\n";
# if ($ssr > 1) {
#     if  ($samplesPerLine > 1) {
#         foreach my $subFile (@subFilesFinal) {  
#             for (my $i = 1; $i < $ssr; $i++) {  
#                 print "ssr is $i\n";
#                 my $cloneFile = $subFile;  
#                 $cloneFile =~ s/_0\_/_$i\_/; # replace "_0." with "_$i."  
#                 system("cp $subFile $cloneFile"); # clone the file  
#             }  
#         }
#     } else {
#         foreach my $subFile (@subFiles) {  
#             for (my $i = 1; $i < $ssr; $i++) {  
#                 print "ssr is $i\n";
#                 my $cloneFile = $subFile;  
#                 $cloneFile =~ s/_0\_/_$i\_/; # replace "_0." with "_$i."  
#                 system("cp $subFile $cloneFile"); # clone the file  
#             }  
#         }  
#     }
# }
close(fileH)



