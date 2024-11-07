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
This script will split an input text file into an SSR number of input text files or interleave (zip) multiple files into one file.
use it thus:
ssr_split_zip.pl -f data/input.txt --ssr 2  --split --dual 1 -t cint16
    The above will output data/input_0.txt data/input_1.txt, where input_0.txt has samples 0, 2, 4, 6, 8.. and input_1.txt has samples 1, 3, 5...
options:
    -f|--file=s                                       => filepath containing data to be split or resultant filepath containing interleaved data.
    [-s|--ssr=i]                                      => ssr number - number of files to split over
    [--split | --zip]                                 => Controls wether to split an input file into SSR sub-files or to combine (zip) SSR sub-files into one output file
    [-d | --dual]                                     => If using dual input/output streams, SSR phases are split into io1 and io2 with a granularity determined by the data type.
    [-t|--type=i]                                     => Data type for the file - note int16 has two samples per line.
    [-k|--headerMode=i]                               => Header Embedded in Data stream. Supported modes: 0 - no header, 1 - fixed length.
    [-w|--windowVsize=i]                              => Number of samples per window
    [-h|--help]                                      => Optional. prints this usage
    [-v|--verbose]                                    => Optional. additional logging
";

my $file = "";
my $ssr = 1;
my $split = 0;
my $zip = 0;
my $type = "cint16";
my $dual = 0;
my $headerMode = 0;
my $windowVsize = 0;
my $coeffType = 0;
my $firLen = 0;
my $help = 0;
my $verbose = 0;
my $plioWidth = 32;


GetOptions (
    "f|file=s" => \$file,
    "s|ssr=i"  => \$ssr,
    "split" => \$split,
    "zip" => \$zip,
    "t|type=s" => \$type,
    "d|dual=i" => \$dual,
    "k|headerMode=i" => \$headerMode,
    "w|windowVsize=i" => \$windowVsize,
    "c|coeffType=s" => \$coeffType,
    "fl|firLen=i" => \$firLen,
    "plio|plioWidth=i" => \$plioWidth,
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


if ( ($split and $zip) or ((not $split) and (not $zip)) ) {
    die "ERROR: need only split or zip. -h for usage"
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

# For case when sample is split over two lines (cint32, cfloats with 32-bit PLIO)
my $linesPerSample = 1;
if ($plioWidth < $sampleSizeBits) {
  $linesPerSample = 2;
} 

my $dual_gran;
my $data_type_size_bytes = $sampleSizeBits / 8;

my $headerVsize = 0;
if ($headerMode != 0 ) {
  if ($type eq "int16" ) {
    $headerVsize = 16;
  } elsif ($type eq "cint16" || $type eq "int32" || $type eq "float") {
    $headerVsize = 8;
  } else {
    $headerVsize = 4;
  }
} else {
  $headerVsize = 0;
}
my $dualFactor = $dual+1; #i.e. when dual=1, dualFactor=2 otherwise =1

use integer;
#128b granularity for each data type
$dual_gran = ((128/8) / $data_type_size_bytes );

print("Splitting/Zipping to SSR files then each file into dual streams of granuarity ${dual_gran}.\n") if ($dual == 1);

# get component parts of input/output filenames
(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');

my @ssrRange = (0...($ssr - 1));
my @dualRange = (0...($dual));
my @subFiles;
my @subFilesH;
# Used if int16 two samples per line
my @subFilesFinal;
my @subFilesFinalH;

if ($split) {
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

  # Prepare output arrays  
  my @subArrays;  
  for my $ssrIdx (@ssrRange) {  
      for my $dualIdx (@dualRange) {  
          my $fileIdx = $ssrIdx * $dualFactor + $dualIdx;  
          $subArrays[$fileIdx] = [];  
      }  
  }  

    my $partNum = 0;
    my $lineNumHeader = 0;
    my $ssrIndex = 0;
    my $dualIdx = 0;
    my $partsPerSSRDualStream = $partsPerSample * $dual_gran * $ssr;
    # Convert parts back into lines
  if ($headerMode == 1) {  
      my $thisHeaderParts = $headerVsize * $partsPerSample;  
      my $thisFrameParts = $windowVsize * $partsPerSample;  
    
      foreach my $part (@dataParts) {  
          if ($thisHeaderParts > 0) {  
              for my $sI (@ssrRange) {  
                  for my $dI (@dualRange) {  
                      my $fI = $sI * $dualFactor + $dI;  
                      push @{$subArrays[$fI]}, $part;  
                  }  
              }  
              $thisHeaderParts--;  
              $thisFrameParts = $windowVsize * $partsPerSample;  
          } else {  
              if ($thisFrameParts > 0) {  
                  my $fileIdx = $ssrIndex * $dualFactor + $dualIdx;  
                  push @{$subArrays[$fileIdx]}, $part;  

                  if ($partNum % $partsPerSSRDualStream == ($partsPerSSRDualStream - 1)) { 
                      $dualIdx = ($dualIdx + 1) % $dualFactor;  
                  }  
                  if ($partNum % $partsPerSample == ($partsPerSample - 1)) { 
                      $ssrIndex = ($ssrIndex + 1) % $ssr;  
                  }  
                  $partNum++;   
                  $thisFrameParts--;  
                  if ($thisFrameParts == 0) {  
                      $thisHeaderParts = $headerVsize * $partsPerSample;  
                  }  
              }  
          }  
      }  
  } else {
      
    for (my $i = 0; $i < @dataParts; $i++) {  
        my $part = $dataParts[$i];        
        my $fileIdx = $ssrIndex * $dualFactor + $dualIdx;  
        
        push @{$subArrays[$fileIdx]}, $part;  

        if ($partNum % $partsPerSSRDualStream == ($partsPerSSRDualStream - 1)) { 
            $dualIdx = ($dualIdx + 1) % $dualFactor;  
        }  
        if ($partNum % $partsPerSample == ($partsPerSample - 1)) { 
            $ssrIndex = ($ssrIndex + 1) % $ssr;  
        }  
        $partNum++;   
    }  
  }

  # Assuming @subArrays is an array of array references  
  for my $ssrIdx (@ssrRange) {  
      for my $dualIdx (@dualRange) {  
          my $fileIdx = $ssrIdx * $dualFactor + $dualIdx;  
          $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}";  
          print "Writing to $subFiles[$fileIdx]\n";  
            
          open(my $subFileH, ">", $subFiles[$fileIdx]) or die "Cannot open $subFiles[$fileIdx]: $!";  
            
          my @ssrParts = @{$subArrays[$fileIdx]};  
            
          # Convert parts back into lines and print directly to the file  
          for (my $i = 0; $i < @ssrParts; $i += $partsPerLine) {  
              my $line = join(' ', @ssrParts[$i .. $i + $partsPerLine - 1]) . " \n";  
              print $subFileH $line;  
          }  
            
          close($subFileH) or die "Cannot close $subFiles[$fileIdx]: $!";  
      }  
  } 

    




} elsif ($zip) {
  my $numSamples;
  my @samples; #whole file, converted to samples
  my @headers; #concatenated headers, held as samples. (not as lines)
  my @dataSamples; #concatenated payload, held as samples (not as lines)
  my $headersIdx = 0;
  my $dataSamplesIdx = 0;
  my $sampleIdx = 0;
  my $numWindows = 0;
  my @ssrSamples; #the zipped array
  my @ssrLines; #The zipped output file/array.
  my $numDataSamples;

  print "Will write to $file\n";
  open(my $fileH, ">", $file)
    or die "cannot open $file : $!";

  for my $ssrIdx (@ssrRange){
    for my $dualIdx (@dualRange){
      # create file names
      my $fileIdx = $ssrIdx* $dualFactor + $dualIdx;
      $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${dualIdx}${fileExt}";
      #print "Reading from $subFiles[$fileIdx]\n";
      # open for read
      open($subFilesH[$fileIdx], "<", $subFiles[$fileIdx])
        or die "cannot open $subFiles[$fileIdx] : $!";

      # read all the lines from file
      my $ssrHandle = $subFilesH[$fileIdx];
      my @lines = <$ssrHandle>;
      # Make sure we get rid of timestamp with the same grep in makefile.
      @lines = grep !/[XT]/, @lines;
      #print @lines;

      #re-index the lines according to SSR
      my $numLines = @lines;

      my $line;
      my @lineRange = (0...($numLines-1));
      my @all_parts;
      my $sampleIdx = 0;
      # Convert lines into an array of parts
      # print("partsPerLine = $partsPerLine, samplesPerLine = $samplesPerLine, linesPerSample = $linesPerSample\n");
      for my $lineIdx (@lineRange) {
        push @all_parts, $lines[$lineIdx] =~ /-?\d+(?:\.\d+)?(?:[eE][+-]?\d+)?/g;  
      }
      # Convert parts into samples
      for (my $i = 0; $i < @all_parts; $i += $partsPerSample) {  
          my $sample = join(' ', @all_parts[$i .. $i + $partsPerSample - 1]);  
          $samples[$sampleIdx] = "$sample ";    
          $sampleIdx++;      
      }

      $headersIdx = 0;
      $dataSamplesIdx = 0;
      $sampleIdx = 0;
      $numWindows=0;
      $numSamples = @samples;

      while ($sampleIdx < $numSamples) {
        $numWindows = $numWindows+1;
        for my $Idx (0...($headerVsize-1)) {
          $headers[$headersIdx] = $samples[$sampleIdx];
          $sampleIdx = $sampleIdx+1;
          $headersIdx = $headersIdx+1;
        }
        for my $idx (0...($windowVsize/($ssr*$dualFactor)-1)) {
          $dataSamples[$dataSamplesIdx] = $samples[$sampleIdx];
          $sampleIdx = $sampleIdx+1;
          $dataSamplesIdx = $dataSamplesIdx+1;
        }
      }
      $numDataSamples = $dataSamplesIdx;

      #perform zip to combined array.
      #print "\n...Zipping headers and data samples from contributing file to single output array...\n";
      #print "\n Number of windows detected = $numWindows\n";
      #print "\n Number of samples in each window = $numSamples\n";
      #print "\n dataSamplesIdx = $dataSamplesIdx\n";
      #print "Headers...\n";
      #print @headers;
      #print "\nData samples... \n";
      #print "\n\nSplit files\n\n";
      #print @dataSamples;

      #The actual zipping takes place in this loop. From one of the files, the output file is sparsely written
      my $inSampleStart = 0;
      my $inHeaderStart = 0;
      my $outStart = 0;
      for my $window (0...($numWindows-1)) {
        $outStart = $window * ($headerVsize+$windowVsize);
        for my $headerIdx (0...($headerVsize-1)) {
          $ssrSamples[$outStart+$headerIdx] = $headers[$inHeaderStart+$headerIdx];
        }
        $outStart = $outStart + $headerVsize;
        $inHeaderStart = $inHeaderStart + $headerVsize;
        #print "outStart = $outStart\n";
        #print "windowVsize/(ssr*dualFactor) = $windowVsize/($ssr*$dualFactor)\n";
        #print "numDataSamples = $numDataSamples\n";
        for my $sampleIdx (0...($windowVsize/($ssr*$dualFactor)-1)) { #$windowVsize is dangerous in FIRs.
        #for my $sampleIdx (0...($numDataSamples-1)) {
          my $dualchunk  = $sampleIdx / $dual_gran;
          my $dualoffset = $sampleIdx % $dual_gran;
          my $outIdx = $outStart +
                       $ssrIdx +
                       $ssr * $dualoffset  +
                       $ssr * $dualIdx * $dual_gran +
                       $dualchunk*$dualFactor*$dual_gran*$ssr;
          #print "Writing $dataSamples[$inSampleStart+$sampleIdx] from $inSampleStart+$sampleIdx to $outIdx for outStart=$outStart, ssrIdx=$ssrIdx, dualoffset=$dualoffset, dualIdx=$dualIdx, Dualchunk=$dualchunk, dual_gran=$dual_gran\n";
          $ssrSamples[$outIdx] =  $dataSamples[$inSampleStart+$sampleIdx];
        }
        $inSampleStart = $inSampleStart + $windowVsize/($ssr*$dualFactor);
      }
    } #dual
  } #ssr
  #print "\nZipped samples...\n";
  #print @ssrSamples;

  #convert output array to output file. What's the difference? SamplesPerLine or LinesPerSample.
  #print "\n...Converting output array to output file. This takes samples per line and lines per sample into account...\n";
  my $ssrSample;
  my $numSSRSamples = @ssrSamples;
  #print "numSSRSamples = $numSSRSamples\n";
  my @outFile;
  my @ssrParts;  

  # Convert zipped samples back into parts
  foreach my $ssrSample (@ssrSamples) {  
      push @ssrParts, split(' ', $ssrSample);  
  }  
  # Convert parts back into lines
  for (my $i = 0; $i < @ssrParts; $i += $partsPerLine) {  
      my $line = join(' ', @ssrParts[$i .. $i + $partsPerLine - 1]) . " \n";  
      push @outFile, $line;  
  }  

  # newly re-arranged lines to result file
  print $fileH @outFile;
  close($fileH);
}