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
my $plioWidth = 64;



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
    "plio|plioWidth=i" => \$plioWidth,
    "h|help" => \$help)
or die("Error in command line arguments\n");

if ( $help ) {
    die "$usage";
}

# Handle mandatory arguments
if ( $file eq "" ) {
    die "ERROR: need a filename input. -h for usage"
}


# Define properties for each data type  
my %type_properties = (  
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

# For case when sample is split over two lines (cint32, cfloats with 32-bit PLIO)
my $linesPerSample = 1;
if ($plioWidth < $sampleSizeBits) {
  $linesPerSample = 2;
} 
my $data_type_size_bytes = $sampleSizeBits / 8;
my $dual_gran;

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

# Rearrange files with multiple samples per line to 1 sample per line and twice the lines.
# This is set up for data types of 16 bits and less.
# This is required since casc splits input stream on a sample-by-sample basis.

# Read the file into an array  
# Open the input file  
open(fileH, "<", $file)
    or die "cannot open $file : $!";
# Create array of data parts
my @dataParts;
while (my $line = <fileH>) {  
    chomp $line;  
    push @dataParts, split ' ', $line;  
} 


###################### PAD WITH ZEROS ###################
# Loop through input, insert pad at end of each window
my @paddedDataParts;
my $partNum = 1;
my $currentFrame = 1;
my $part;
my $partsToPad = $partsPerSample*(($paddedWindowSize/$numFrames)-$pointSize);

for (my $i = 0; $i < @dataParts; $i++) {  
    $part = $dataParts[$i];
    push @paddedDataParts, $dataParts[$i];
    # End of a frame once paddedPointSize lines has been reached
    if ($partNum % ($partsPerSample*$pointSize) == 0) {
        for(my $j = 0; $j < $partsToPad; $j++){
            push @paddedDataParts, "0";
        }
        $currentFrame++;   
    }    
    $partNum++;
}


##########################################################

# Open a bunch of files in an array of file handles with a specific filename
my @subArrays;
for my $cascIdx (@cascRange){
    my $fileIdx = $cascIdx;
    $subArrays[$fileIdx] = [];      
}
my $cascIdx = 0;
$partNum = 0;
for (my $i = 0; $i < @paddedDataParts; $i++) {  
    my $part = $paddedDataParts[$i];        
    my $fileIdx = $cascIdx;  
    
    push @{$subArrays[$fileIdx]}, $part;  

    if ($partNum % $partsPerSample == ($partsPerSample - 1)) { 
        $cascIdx = ($cascIdx + 1) % $cascLen;  
    }  
    $partNum++;   
}  
# Assuming @subArrays is an array of array references  
for my $cascIdx (@cascRange) {  
    my $fileIdx = $cascIdx;  
    $subFiles[$fileIdx] = "${newFileDir}${newFileName}_0_$cascIdx${newFileExt}";
    print "Writing to $subFiles[$fileIdx]\n";  
    
    open(my $subFileH, ">", $subFiles[$fileIdx]) or die "Cannot open $subFiles[$fileIdx]: $!";  
    
    my @cascParts = @{$subArrays[$fileIdx]};  
    
    # Convert parts back into lines and print directly to the file  
    for (my $i = 0; $i < @cascParts; $i += $partsPerLine) {  
        my $line = join(' ', @cascParts[$i .. $i + $partsPerLine - 1]) . " \n";  
        print $subFileH $line;  
    }  
    
    close($subFileH) or die "Cannot close $subFiles[$fileIdx]: $!";  
} 

# When input is a vector to be split over cascade, it is cloned for each ssr
# print "ssr is $ssr\n";
if ($ssr > 1) {
    foreach my $subFile (@subFiles) {  
        for (my $i = 1; $i < $ssr; $i++) {  
            # print "ssr is $i\n";
            my $cloneFile = $subFile;  
            $cloneFile =~ s/_0_/_${i}_/; # replace "_0." with "_$i."  
            system("cp $subFile $cloneFile"); # clone the file  
        }  
    }  
}

close(fileH)



