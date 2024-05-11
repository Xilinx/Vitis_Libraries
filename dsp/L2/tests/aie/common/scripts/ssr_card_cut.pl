
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
my $usage = "
This script will split an input text file of matrice samples into an SSR*CASC_LEN number of files. SSR will be split along column, CASC_LEN will be split across row:
ssr_matrix_split.pl -f input.txt --rows 16 --cols 24 --ssrSplit 2 --casc 3 --columnMajor --split -t int32  
    The above will output data/input_0.txt data/input_1.txt, where input_0.txt has samples 0, 2, 4, 6, 8.. and input_1.txt has samples 1, 3, 5...
options:
    -f|--file=s                                       => filepath containing data to be split or resultant filepath containing interleaved data.
    [--rows=i]
    [-cols=i]
    [-s|--ssrSplit=i]                                      => ssr number - number of files to split over
    [--ssrSplit=i]
    [--casc=i]
    [--split | --zip]                                 => Controls wether to split an input file into SSR sub-files or to combine (zip) SSR sub-files into one output file
    [--niter | -n]
    [-t|--type=i]                                     => Data type for the file - note int16 has two samples per line.
    [-h|--help]                                       => Optional. prints this usage
    [-v|--verbose]                                    => Optional. additional logging
";

my $file = "";
my $rows = 1;
my $cols = 1;
my $colMajor = 1;
my $ssrSplit = 1;
my $ssrClone = 0;
my $casc = 1;
my $split = 0;
my $zip = 0;
my $NITER = 1;
my $type = "cint16";
my $findOutType = "";
my $windowVsize = 0;
my $help = 0;
my $verbose = 0;

GetOptions (
    "f|file=s" => \$file,
    "rows=i" => \$rows,
    "cols=i" => \$cols,
    "colMajor=i" => \$colMajor,
    "s|ssrSplit=i"  => \$ssrSplit,
    "ssrClone=i"  => \$ssrClone,
    "casc=i" => \$casc,
    "split" => \$split,
    "zip" => \$zip,
    "niter=i" => \$NITER,
    "t|type=s" => \$type,
    "findOutType=s" => \$findOutType,
    "w|windowVsize=i" => \$windowVsize,
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
if ( ($zip) and ($findOutType eq "") ) {
    die "ERROR: When zip, insert the data type that -type DATA_A is multiplied with -findOutType DATA_B -h for usage"
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
use integer;
# get component parts of input/output filenames
(my $fileName, my $fileDir, my $fileExt) = fileparse($file, '\..*');
my @ssrRange = (0...($ssrSplit - 1));
my @cascRange = (0...($casc - 1));
my @subFiles;
my @subFilesH;

# Used if int16 two samples per line
my @subFilesFinal;
my @subFilesFinalH;
if ($split) {
    print "reading $file\n";
    open(fileH, "<", $file)
        or die "cannot open $file : $!";
    my $outFileMod = "";
    # Rearrange files with multiple samples per line to 1 sample per line and twice the lines.
    # This is set up for data types of 16 bits and less.
    # This is required since SSR splits input stream on a sample-by-sample basis.
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
    # Open a bunch of files in an array of file handles with a specific filename
    for my $cascIdx (@cascRange){
      for my $ssrIdx (@ssrRange){
          my $fileIdx = $cascIdx + ($ssrIdx)*$casc;
          $subFiles[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${cascIdx}${fileExt}${outFileMod}";
        #   print "Writing file $fileIdx to $subFiles[$fileIdx]\n";
          open($subFilesH[$fileIdx], ">", $subFiles[$fileIdx])
              or die "cannot open $subFiles[$fileIdx] : $!";
      }
    }
    my $rowNum = 0;
    my $colNum = 0;
    my $lineNum = 0;

    my $lineNumHeader = 0;
    my $cascIndex = 0;
    my $ssrIndex = 0;
    my $rowIdx = 0;
    my $rowsPerSSR = $rows/$ssrSplit;
    my $colsPerCasc = $cols/$casc;

    my $colIdx = 0;
    while(my $line = <fileH>){
        $lineNum = $lineNum + 1;
        my $ssrIndex = $rowNum/$rowsPerSSR;
        my $cascIndex = $colNum/$colsPerCasc;
        my $fileIdx = $cascIndex + ($ssrIndex)*$casc;
        # print "lineNum = $lineNum\n";
        # print "rowNum = $rowNum ";
        # print "colNum = $colNum ->";
        # print "fileIdx = $fileIdx\n\n";

        print {$subFilesH[$fileIdx]} $line;
        if($colMajor) { 
            # new row with each sample
            if ($lineNum % ($linesPerSample) == 0 ) {
                $rowNum = $rowNum + 1;
            }
            if ($rowNum == ($rows)) {
                $colNum = $colNum + 1;
                $rowNum = 0;
            }
            if ($colNum == ($cols)) {
                $colNum = 0;
            }
        } else {
            # new row with each sample
            if ($lineNum % ($linesPerSample) == 0 ) {
                $colNum = $colNum + 1;
            }
            if ($colNum == ($cols)) {
                $rowNum = $rowNum + 1;
                $colNum = 0;
            }
            if ($rowNum == ($rows)) {
                $rowNum = 0;
            }
        }
    }

    if  ($samplesPerLine > 1) {
        for my $cascIdx (@cascRange){
            for my $ssrIdx (@ssrRange){
                my $fileIdx = $cascIdx * ($ssrSplit) + $ssrIdx;

                close($subFilesH[$fileIdx])
                    or die "cannot close $subFiles[$fileIdx] : $!";
                # Open out files for read
                open($subFilesH[$fileIdx], "<", $subFiles[$fileIdx])
                    or die "cannot open $subFiles[$fileIdx] : $!";

                print "Reading from $subFiles[$fileIdx]\n";

                # intendedfinal output file without outFileMod
                $subFilesFinal[$fileIdx] = "${fileDir}${fileName}_${ssrIdx}_${cascIdx}${fileExt}";
                print "Writing to $subFilesFinal[$fileIdx]\n";    
                open($subFilesFinalH[$fileIdx], ">", $subFilesFinal[$fileIdx])
                    or die "cannot open $subFilesFinal[$fileIdx] : $!"; 
                my $ssrHandle = $subFilesH[$fileIdx];
                my $ssrHandleFinal = $subFilesFinalH[$fileIdx];
                my $lineCount = 0;
                while(my $line = <$ssrHandle>){
                    # Every even line we remove the newline character.
                    if ($lineCount % $samplesPerLine == 0) {
                    $line =~ s/\n/ /g ;
                    }
                    print $ssrHandleFinal $line;
                    $lineCount++;
                }
            }
        }
    }

    # When input is a vector to be split over cascade, it is cloned for each ssr
    print "ssrClone is $ssrClone\n";
    if ($ssrClone > 1) {
        if  ($samplesPerLine > 1) {
            foreach my $subFile (@subFilesFinal) {  
                for (my $i = 1; $i < $ssrClone; $i++) {  
                    # print "ssr is $i\n";
                    my $cloneFile = $subFile;  
                    $cloneFile =~ s/_0_/_${i}_/; # replace "_0." with "_$i."  
                    system("cp $subFile $cloneFile"); # clone the file  
                }  
            }
        } else {
            foreach my $subFile (@subFiles) {  
                for (my $i = 1; $i < $ssrClone; $i++) {  
                    # print "ssr is $i\n";
                    my $cloneFile = $subFile;  
                    print $subFile;
                    $cloneFile =~ s/_0_/_${i}_/; # replace "_0." with "_$i."  
                    print $cloneFile;
                    system("cp $subFile $cloneFile"); # clone the file  
                }  
            }  
        }
    }

    close(fileH)
}
if ($zip) {
    my $num_lines = ($linesPerSample)*($rows/$ssrSplit)/$samplesPerLine; # number of lines to read at a time. (double for cint32, cfloat or half for int16)
    # print "num lines is $num_lines\n";
    # print "lines per sample is $linesPerSample\nsamplesPerLine is $samplesPerLine\n";
    # Rearrange files with multiple samples per line to 1 sample per line and twice the lines.
    # This is set up for data types of 16 bits and less.
    # This is required since SSR splits input stream on a sample-by-sample basis.

    my @files = map { "./data/uut_output_$_.txt"} (0...$ssrSplit-1);
    my @filehandles;  
    my $outfile = $file; # output file 

    # open all files  
    for my $file (@files) {  
        open my $fh, '<', $file or die "Can't open file $file: $!";  
        push @filehandles, $fh;  
    }  
    
    # open output file  
    open my $outfh, '>', $outfile or die "Can't open output file $outfile: $!";  
    
    # read and write lines  
    while (1) {  
        my $eof_count = 0;  
        for my $fh (@filehandles) {  
            for (1..$num_lines) {  
                my $line = <$fh>;  
                if (defined $line) {  
                    print $outfh $line;  
                } else {  
                    $eof_count++;  
                    last;  
                }  
            }  
        }  
        last if $eof_count == scalar @filehandles; # exit loop if all files are at EOF  
    }  
    
    # close all files  
    for my $fh (@filehandles) {  
        close $fh;  
    }  
    close $outfh; 
;}

