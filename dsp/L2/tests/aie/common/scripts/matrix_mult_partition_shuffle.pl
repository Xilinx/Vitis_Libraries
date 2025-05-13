#!/usr/bin/perl -w
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

# TODO: accept STDIN as well as inFile.

my $usage = "
This script will tile/untile an input text file with each sample on a newline.
Usage:
matrix_mult_datafile_partition.pl -f data/inputA.txt -m 2 -n 4 -r 16 -c 32
    The above will output a data/inputAtiled.txt file assuming a 16x32 row-major matrix input. It will tile with a 2x4 pattern.

Options:
    -f|--file|--inFile=s       => Input filepath containing input matrix of size r_c. Mandatory.
    -o|--outFile=s             => Optional. Output filepath (default is inFilePath/inFileName_<casc_index>.<inFileExt>).
    -r|--inRow=i               => Actual number of rows for InMatrix. Mandatory.
    -c|--inCol=i               => Actual number of columns for InMatrix. Mandatory.
    -p|--partition|--cascLen=i => Number of partitions / Cascade length for InMatrix.
    -s|--ssr=i                 => Optional. Specify the SSR (Super Sample Rate) factor.
    --splitRows                => Optional. Specify if input data is to be partitioned over rows. Default behavior assumes split over columns.
    --untile                   => Optional. Specify if the input matrix is un-tiled. Works with other options, e.g., if colMajor is present, output will be stored ColumnMajor.
    --isTiled=i                => Optional. Specify if input data is already tiled. Default behavior assumes it is not tiled.
    --tileInPlace              => Optional. Specify if tiling should happen in-place or be given a suffix of _tiled or _untiled.
    --colMajor=i               => Optional. Specifies that the InMatrix is stored colMajor. Output will be tiled & rowMajor.
    --T_DATA_A=s               => Specifies data type of matrix A. Mandatory.
    --T_DATA_B=s               => Specifies data type of matrix B. Mandatory.
    --T_DATA_OUT=s             => Optional. Specifies the data type of the output matrix. Defaults to a sensible value if not chosen.
    --plio|plioWidth=i         => Optional. Specifies the PLIO width in bits. Default is 32.
    -h|--help                  => Optional. Prints this usage.
    -v|--verbose               => Optional. Additional logging.

";

my $aieVariant = 1;
my $inFile = "";
my $outFile = "";
my $inRow = "";
my $inCol = "";
my $cascLen = "";
my $ssr = "";
my $splitRows  = 0;
my $verbose = 0;
my $untile  = 0;
my $inplace = 0;
my $isTiled  = 0;
my $colMajor = 0;
my $help = 0;
my $T_DATA_A = "";
my $T_DATA_B = "";
my $T_DATA_OUT = "";
my $plioWidth = 32;

GetOptions (
            "aieVariant=i"          => \$aieVariant,
            "f|file|inFile=s"       => \$inFile,  # string
            "o|outFile=s"           => \$outFile,  # string
            "r|inRow=i"             => \$inRow,
            "c|inCol=i"             => \$inCol,
            "p|partition|cascLen=i" => \$cascLen,
            "s|ssr=i"               => \$ssr,
            "splitRows"             => \$splitRows,
            "untile"                => \$untile,
            "isTiled=i"             => \$isTiled,
            "--tileInPlace"         => \$inplace,
            "colMajor=i"            => \$colMajor,
            "T_DATA_A=s"            => \$T_DATA_A,
            "T_DATA_B=s"            => \$T_DATA_B,
            "T_DATA_OUT=s"          => \$T_DATA_OUT,
            "plio|plioWidth=i"      => \$plioWidth,
            "h|help"                => \$help,
            "v|verbose"             => \$verbose) # flag
or die("Error in command line arguments\n");

if ( $help ) {
    die "$usage";
}

# TODO: command line arguments for tile / untile and inplace / filename_tiled.txt

# Handle mandatory arguments
if ( $inFile eq "" ) {
    die "ERROR: Provide command line argument for inFile. Use -h for usage. ";
}

if ( $T_DATA_A eq "" ) {
    die "ERROR: Provide command line argument for T_DATA_A. Use -h for usage. ";
}
if ( $T_DATA_B eq "" ) {
    die "ERROR: Provide command line argument for T_DATA_B. Use -h for usage. ";
}

if ( $inRow eq "" ) {
    die "ERROR: Provide command line argument for inRow. Use -h for usage. ";
}

if ( $inCol eq "" ) {
    die "ERROR: Provide command line argument for inCol. Use -h for usage. ";
}

# Handle verbose
if ( $verbose ) {
    if ( $inFile ne "" ) {
        print "inFile is $inFile. \n";
    }
    if ( $inRow ne "" ) {
        print "inRow is $inRow. \n";
    }
    if ( $inCol ne "" ) {
        print "inCol is $inCol. \n";
    }

    if  ($colMajor) {
        print "colMajor is enabled\n";
    }
    if  ($untile) {
        print "untile is enabled\n";
    }
    if  ($help) {
        print "help is enabled\n";
    }
    if  ($verbose) {
        print "verbose is enabled\n";
    }
    if ($outFile ne "" ) {
        print "outFile is $outFile. \n";
    }
}

# Default to sensible value
my ${DIM_A_TILE}  = 4;
my ${DIM_AB_TILE} = 4;
my ${DIM_B_TILE}  = 2;


if ( $aieVariant eq 2) {
    if ( (${T_DATA_A} eq "cint16") && (${T_DATA_B} eq "cint16")) {
        ${DIM_A_TILE}  = 1;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;

    } elsif ( (${T_DATA_A} eq "cint32") && (${T_DATA_B} eq "cint16")) {
        ${DIM_A_TILE}  = 2;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;

    } elsif ( (${T_DATA_A} eq "cint32") && (${T_DATA_B} eq "cint32")) {
        ${DIM_A_TILE}  = 1;
        ${DIM_AB_TILE} = 2;
        ${DIM_B_TILE}  = 8;

    } else {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 4;
    }

} elsif  ( $aieVariant eq 22) {
    if ( (${T_DATA_A} eq "int8") && (${T_DATA_B} eq "int8")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 8;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "int16") && (${T_DATA_B} eq "int8")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "int16") && (${T_DATA_B} eq "int16")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "int32") && (${T_DATA_B} eq "int16")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 2;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "int16") && (${T_DATA_B} eq "int32")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "int32") && (${T_DATA_B} eq "int32")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 2;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "bfloat16") && (${T_DATA_B} eq "bfloat16")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 8;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "float") && (${T_DATA_B} eq "float")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 8;
        ${DIM_B_TILE}  = 4;
    } elsif ( (${T_DATA_A} eq "cint16") && (${T_DATA_B} eq "int16")) {
        ${DIM_A_TILE}  = 4;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "cint16") && (${T_DATA_B} eq "cint16")) {
        ${DIM_A_TILE}  = 1;
        ${DIM_AB_TILE} = 4;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "cint32") && (${T_DATA_B} eq "cint16")) {
        ${DIM_A_TILE}  = 1;
        ${DIM_AB_TILE} = 2;
        ${DIM_B_TILE}  = 8;
    } elsif ( (${T_DATA_A} eq "cint32") && (${T_DATA_B} eq "cint32")) {
        ${DIM_A_TILE}  = 1;
        ${DIM_AB_TILE} = 2;
        ${DIM_B_TILE}  = 8;
    } else {
        # Unsupported
        ${DIM_A_TILE}  = 1;
        ${DIM_AB_TILE} = 1;
        ${DIM_B_TILE}  = 1;
    }
} else {

    # CAUTION: Maintenance hazard - these definitions are duplicated in metadata and matrix_mult.hpp
    if ( ${T_DATA_A} eq "int16" ) {
        if ( ${T_DATA_B} eq "int16" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 4;
        }
        if ( ${T_DATA_B} eq "cint16" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }
        if ( ${T_DATA_B} eq "int32" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }
        if ( ${T_DATA_B} eq "cint32" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }
    }
    if ( ${T_DATA_A} eq "cint16" ) {
        if ( ${T_DATA_B} eq "int16" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cint16" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "int32" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cint32" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }

    }
    if ( ${T_DATA_A} eq "int32" ) {
        if ( ${T_DATA_B} eq "int16" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cint16" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "int32" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cint32" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }

    }
    if ( ${T_DATA_A} eq "cint32" ) {
        if ( ${T_DATA_B} eq "int16" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cint16" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "int32" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cint32" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }

    }
    if ( ${T_DATA_A} eq "float" ) {
        if ( ${T_DATA_B} eq "float" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cfloat" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

    }
    if ( ${T_DATA_A} eq "cfloat" ) {
        if ( ${T_DATA_B} eq "float" ) {
            ${DIM_A_TILE}  = 2;
            ${DIM_AB_TILE} = 4;
            ${DIM_B_TILE}  = 2;
        }

        if ( ${T_DATA_B} eq "cfloat" ) {
            ${DIM_A_TILE}  = 4;
            ${DIM_AB_TILE} = 2;
            ${DIM_B_TILE}  = 2;
        }
    }
}

print "Tiling Dimensions are $DIM_A_TILE x $DIM_AB_TILE x $DIM_B_TILE\n";


my $tileRow = "";
my $tileCol = "";
my $dataType = "";
my $ssrSplit = 1;
my $ssrClone = 1;
my $ssrJoin = 1;
# default tiler gets provided dimensions, if cascade, we divide AB by casc len.
my $tileInRow = $inRow;
my $tileInCol = $inCol;
if ( $cascLen eq "" ) {
    # using output
    $tileRow = $DIM_A_TILE;
    $tileCol = $DIM_B_TILE;
    $ssrJoin = $ssr;
    if ($T_DATA_OUT eq "") {
        # Need to find if output dataType is cin32, or cfloat
        if ($T_DATA_A eq "cfloat" or $T_DATA_B eq "cfloat") {
            $dataType = "cfloat"
        } elsif ($T_DATA_A eq "cint32" or $T_DATA_B eq "cint32") {
            $dataType = "cint32";
        } elsif ($T_DATA_A eq "int32" and $T_DATA_B eq "cint16") {
            $dataType = "cint32";
        } elsif ($T_DATA_A eq "cint16" and $T_DATA_B eq "int32") {
            $dataType = "cint32";
        } elsif ($T_DATA_A eq "int32" and $T_DATA_B eq "int16") {
            $dataType = "int32";
        } elsif ($T_DATA_A eq "int16" and $T_DATA_B eq "int32") {
            $dataType = "int32";
        } elsif ($T_DATA_A eq "int16" and $T_DATA_B eq "cint16") {
            $dataType = "cint16";
        } else {
            $dataType = $T_DATA_A;
        }
    } else {
        $dataType = $T_DATA_OUT;
    }
} elsif ( $splitRows ) {
    # using B
    $tileRow = $DIM_AB_TILE;
    $tileCol = $DIM_B_TILE;
    $dataType = $T_DATA_B;
    $tileInRow = ( $inRow / $cascLen );
    $ssrClone = $ssr;
} else {
    # using A
    $tileRow = $DIM_A_TILE;
    $tileCol = $DIM_AB_TILE;
    $dataType = $T_DATA_A;
    $tileInCol = ( $inCol / $cascLen );
    $tileInRow = ( $inRow / $ssr );
    $ssrSplit = $ssr;
}


print "Data Type is $dataType\n";
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
my $partsPerSample = $type_properties{$dataType}{numParts};
my $sampleSizeBits = $type_properties{$dataType}{numParts} * $type_properties{$dataType}{partSize};

my $samplesPerLine = $plioWidth / $sampleSizeBits;
# prevent samplesPerLine = 0.5 (round up)
if (int($samplesPerLine) != $samplesPerLine) {
  $samplesPerLine = int($samplesPerLine) + 1;
}
my $partsPerLine = $plioWidth / $type_properties{$dataType}{partSize};

# For case when sample is split over two lines (cint32, cfloats with 32-bit PLIO)
my $linesPerSample = 1;
if ($plioWidth < $sampleSizeBits) {
  $linesPerSample = 2;
}

# get component parts of input/output filenames
(my $inFileName, my $inFileDir, my $inFileExt) = fileparse($inFile, '\..*');

my $outFileName;my $outFileDir;my $outFileExt;my $outFileTempName;
if ($outFile ne "" ) {
    ($outFileTempName, $outFileDir, $outFileExt) = fileparse($outFile, '\..*');
    $outFileName = "${outFileTempName}_";
} else {
    $outFileName = "${inFileName}_";
    $outFileDir = $inFileDir;
    $outFileExt = $inFileExt;
    #print "$outFileName  : $outFileDir  :  $outFileExt \n" ;
}


print "Reading $inFile. \n";
print "isTiled is $isTiled\n\n";

my @subFiles;
my @dataParts;

if ( $cascLen eq "" ) {
    # in this case, output is stil tiled and needs detiling.
    zip_files($inFile);

    if ( ! $isTiled ) {
        tile_matrix($inFile);
    }
} else {

    open(my $inFileh, "<" , $inFile)
        or die "Can't open < $inFile";
    # Create array of data parts
    while(my $line = <$inFileh>) {
        chomp $line;
        push @dataParts, split ' ', $line;
    }
    close($inFileh)
        or die "couldn't close inFileh $inFileh";

    partition_matrix();

    print "\n Writing to :\n";
    print join(", ", @subFiles);
    print "\n";
    if ( ! $isTiled ) {
        for my $fileForTile (@subFiles) {
            tile_matrix($fileForTile);
        }
    }

}

if ($splitRows==1 and $ssrClone > 1) {

    foreach my $subFile (@subFiles) {
        print ("Copying $subFile to ");
        for (my $i = 1; $i < $ssrClone; $i++) {
            # print "ssr is $i\n";
            my $cloneFile = $subFile;
            $cloneFile =~ s/_0_/_${i}_/; # replace "_0." with "_$i."
            system("cp $subFile $cloneFile"); # clone the file
            print "$cloneFile "
        }
        print("\n");
    }
}

sub partition_matrix {
    use integer;
    my @duplicateText = @dataParts;
    my @numFiles = (0...(($cascLen * $ssrSplit) - 1));

    # create resultant outputfile names and handlers.
    my @outFileh;
    my @ssrRange = (0...($ssrSplit - 1));
    my @cascRange = (0...($cascLen - 1));
    my @subArrays;
    for my $cascIdx (@cascRange){
        for my $ssrIdx (@ssrRange){
            my $fileIdx = $cascLen * ($ssrIdx) + $cascIdx;
            $subArrays[$fileIdx] = [];
      }
    }
    # Partition for matrix A - split for ssr along rows, split for casc along cols
    if (!$splitRows) {
        my $rowNum = 0;
        my $colNum = 0;
        my $partNum = 0;
        my $rowsPerSSR = $inRow/$ssrSplit;
        my $colsPerCasc = $inCol/$cascLen;
        for my $part (@dataParts) {
            my $ssrIndex = $rowNum/$rowsPerSSR;
            my $cascIndex = $colNum/$colsPerCasc;
            my $fileIdx = $ssrIndex * ($cascLen) + $cascIndex;
            push @{$subArrays[$fileIdx]}, $part;
            if ($partNum % $partsPerSample == ($partsPerSample - 1)) {
                # print "SSR=$ssrIndex casc=$cascIndex\n";
                # print "lineNum = $lineNum\n";
                # print "rowNum = $rowNum\n";
                # print "colNum = $colNum\n";
                # print "fileIdx = $fileIdx\n\n";

                if ($colMajor) {
                    $rowNum = $rowNum + 1;
                    if ($rowNum == $inRow) {
                        $colNum = $colNum + 1;
                        $rowNum = 0;
                    }
                    if ($colNum == $inCol) {
                        $colNum = 0;
                    }
                } else {
                    $colNum = $colNum + 1;
                    if ($colNum == ($inCol)) {
                        $rowNum = $rowNum + 1;
                        $colNum = 0;
                    }
                    if ($rowNum == ($inRow)) {
                        $rowNum = 0;
                    }
                }
            }
            $partNum = $partNum + 1;
        }
    } else {

        # Partition matrix B - split along for cascade along columns - no split for ssr
        my $rowNum = 0;
        my $colNum = 0;
        my $partNum = 0;
        my $rowsPerCasc = $inRow/$cascLen;
        for my $part (@dataParts) {
            my $cascIndex = $rowNum/$rowsPerCasc;
            my $fileIdx = $cascIndex;
            # print "lineNum = $lineNum\n";
            # print "rowNum = $rowNum\n";
            # print "colNum = $colNum\n";
            # print "fileIdx = $fileIdx\n\n";
            push @{$subArrays[$fileIdx]}, $part;
            if ($partNum % $partsPerSample == ($partsPerSample - 1)) {
                if ($colMajor) {
                    $rowNum = $rowNum + 1;
                    if ($rowNum == $inRow) {
                        $colNum = $colNum + 1;
                        $rowNum = 0;
                    }
                    if ($colNum == $inCol) {
                        $colNum = 0;
                    }
                } else {
                    $colNum = $colNum + 1;
                    if ($colNum == ($inCol)) {
                        $rowNum = $rowNum + 1;
                        $colNum = 0;
                    }
                    if ($rowNum == ($inRow)) {
                        $rowNum = 0;
                    }
                }
            }
            $partNum = $partNum + 1;
        }
    }
    # Finally write out resultant data to each file.
    for my $cascIdx (@cascRange){
        for my $ssrIdx (@ssrRange){
            my $fileIdx = $cascLen * ($ssrIdx) + $cascIdx;
            # $subArrays[$fileIdx] = [];
            $subFiles[$fileIdx] = "${outFileDir}${outFileName}${ssrIdx}_${cascIdx}${outFileExt}";
            open(my $subFileH, ">", $subFiles[$fileIdx]) or die "Cannot open $subFiles[$fileIdx]: $!";

            my @fileParts = @{$subArrays[$fileIdx]};

            # Convert parts back into lines and print directly to the file
            for (my $i = 0; $i < @fileParts; $i += $partsPerLine) {
                my $line = join(' ', @fileParts[$i .. $i + $partsPerLine - 1]) . " \n";
                print $subFileH $line;
            }

            close($subFileH) or die "Cannot close $subFiles[$fileIdx]: $!";
      }
    }

}

sub tile_matrix {
    my ($fileForTile) = @_ ;
    (my $fileForTileName, my $fileForTileDir, my $fileForTileExt) = fileparse($fileForTile, '\..*');

    my $outTileFileName;my $outTileFileDir;my $outTileFileExt;
    if ($inplace) {

        $outTileFileName = $fileForTileName;
        $outTileFileDir = $fileForTileDir;
        $outTileFileExt = $fileForTileExt;

    } else {

        if ($outFile ne "" ) {
            ($outTileFileName, $outTileFileDir, $outTileFileExt) = fileparse($outFile, '\..*');
        } else {
            my $un = "";
            if ($untile) { $un = "un"; }
            $outTileFileName = "${un}tiled_${fileForTileName}";
            $outTileFileDir = $fileForTileDir;
            $outTileFileExt = $fileForTileExt;
        }
    }
    #print "$outTileFileName  : $outTileFileDir  :  $outTileFileExt \n" ;
    my $resOutTileFile = "${outTileFileDir}${outTileFileName}${outTileFileExt}";

    print "outTileFile is $resOutTileFile\n";
    print "Reading $fileForTile. \n";

    open(my $fileForTileh, "<" , $fileForTile)
        or die "Can't open < $fileForTile";

    # convert file for tile into array of parts
    my @inFileParts;
    while(<$fileForTileh>) {
        chomp;
        push @inFileParts, split ' ', $_;
    }
    # convert array of parts into array of samples
    my @inTileText;
    for (my $i = 0; $i < @inFileParts; $i += $partsPerSample) {
        my $samp = join(' ', @inFileParts[$i .. $i + $partsPerSample - 1]);
        push @inTileText, $samp;
    }

    close($fileForTileh)
        or die "couldn't close fileForTileh $fileForTileh";

    if ($inplace) {
        rename($fileForTile, $fileForTile . '.beforeTile'); # create a backup file
    }
    print "Finished reading file\n";


    my @duplicateText = @inTileText;
    my @transText = @inTileText;
    # fill with dummy data basically
    my @indices = @inTileText;
    my @transIndices = @inTileText;
    #$((((($i-1)/($AB*$K))*$K + (($i-1)%$K))*$AB + ((($i-1)/$K) % $AB) +1 ))
    #tileInRow
    #tileInCol
    #
    #tileRow
    #tileCol

    #res=$(( (( (($i-1)/($AB*$M))*($AB*$M) + ((($i-1)/$N) % $M)) * $AB) +  (($i-1) % $N) + ((((($i-1)/($N*$M)) * $N) %  $AB)  + 1 ) ))
    #open(my $outFileh, ">" , $resOutFile)
    #    or die "Can't open > $resOutFile";

    print "Shuffling indicies\n";
    my @iIter = (0..$#inTileText);
    for my $i (@iIter){
        my $newIndex;
        my $transposeIndex;
        {
            use integer;
            my $colI = ( $untile ) ? ($i % $tileInRow) : ($i % $tileInCol);
            my $colIncr = ( $untile ) ? $tileInCol: $tileInRow;
            my $rowI = ( $untile ) ? (($i / $tileInRow) % $tileInCol) : (($i / $tileInCol) % $tileInRow);
            my $rowIncr = 1;
            my $batchI = ($i/($tileInCol*$tileInRow)); #unchanged
            my $batchIncr = $tileInCol*$tileInRow; #unchanged

            $transposeIndex = ($colI * $colIncr) + ( $rowI * $rowIncr) + ($batchI * $batchIncr);
            #print "transposeIndex: ($colI * $colIncr) + ($rowI * $rowIncr) + ($batchI * $batchIncr) = $transposeIndex\n ";

            # fine-grained within a chunk index
            my $colInTileI =  ($i % $tileCol);
            my $colInTileIncr = 1;

            # which chunk of N samples within tile row
            my $rowInTileI = (( $i/$tileCol ) % $tileRow);
            my $rowInTileIncr = $tileInCol; # grab next row for each tileRow.

            my $tileIndex = ($i/($tileRow*$tileCol));
            my $tileIncr = $tileCol; # advance further down the row ;

            # Which tile in row of tiles
            my $tileWithinRowOffset = ( $tileIndex * $tileIncr ) % $tileInCol;

             # Coarse grain - increments of row of tile
            my $rowOfTileIndex = ($i/( $tileInCol * $tileRow ));
            my $rowOfTileIncr = ( $tileInCol * $tileRow );


            # force everything to be integer arithmetic

            $newIndex = ($rowOfTileIndex*$rowOfTileIncr) +  $tileWithinRowOffset + ($rowInTileI*$rowInTileIncr) + ($colInTileI * $colInTileIncr);

            #print "newIndex: ($rowOfTileIndex*$rowOfTileIncr) + $tileWithinRowOffset + ($rowInTileI*$rowInTileIncr) + ($colInTileI * $colInTileIncr) = $newIndex\n";
        }
        #print "$i ($newIndex) => $transposeIndex \n";
        #print "$transposeIndex $transposeIndex\n";

        if ($untile) {
            $indices[$newIndex] = $i;
        } else {
            $indices[$i] = $newIndex;
        }
        #if ($colMajor) {
            $transIndices[$i] = $transposeIndex;
        #}
        #print (int $i/( $tileInCol * $tileCol ));
        #print "$newIndex \n";
    }


    my @outFileSamples;

    #my @iIter = (0..$#inText/8);
    for my $i (@iIter){
        #if ($colMajor) {
        #    $duplicateText[$i] = $inText[$transIndices[$indices[$i]]];
        #} else {
            $duplicateText[$i] = "$inTileText[$indices[$i]]";
        #}
    }
    for my $i (@iIter){
        if ($untile) {
            $transText[$i] = "$duplicateText[$transIndices[$i]]";
        } else {
            $transText[$i] = "$inTileText[$transIndices[$indices[$i]]]";
        }
        if ($colMajor) {
            push @outFileSamples,  split ' ',  $transText[$i];
        } else {
            push @outFileSamples,  split ' ',  $duplicateText[$i];
        }
        if ($verbose) {
            print "$i = $indices[$i] = $transIndices[$i]      \t \t ";
            print "$inTileText[$i] => $duplicateText[$i] => $transText[$i] \n";
        }
        #print "$i $i\n";
    }

    # Convert tiled array of samples into array of parts
    my @outFileParts;
    foreach my $samp (@outFileSamples) {
        push @outFileParts, split(' ', $samp);
    }
    my @outFile;
    # Convert parts into lines
    for (my $i = 0; $i < @outFileParts; $i += $partsPerLine) {
        my $line = join(' ', @outFileParts[$i .. $i + $partsPerLine - 1]) . " \n";
        push @outFile, $line;
    }

    print "Writing $resOutTileFile. \n";
    open(my $outFileh, ">" , $resOutTileFile)
        or die "Can't open > $resOutTileFile";
    print $outFileh @outFile;
    close($outFileh)
        or die "couldn't close outFileh $outFileh";

    print "Finished writing $resOutTileFile .\nEnd of tiling.\n";

}

sub zip_files {
    my ($outFile) = @_ ;
    print("OUT file is $outFile\n");
    my @ssrOutFiles = (0...(( $ssrJoin - 1)));

    my $num_lines = $linesPerSample*($inRow*$inCol/$ssrJoin)/$samplesPerLine;

    my @files = map { "./data/uut_output_$_\_0.txt"} (0...$ssrJoin-1);
    print join(", ", @files);
    # print("\nnum_lines =$num_lines\n");
    my @filehandles;
    # open all files
    for my $file (@files) {
        open my $fh, '<', $file or die "Can't open file $file: $!";
        push @filehandles, $fh;
    }

    # # open output file
    open my $outfh, '>', $outFile or die "Can't open output file $outFile: $!";

    # # read and write lines
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
}

sub int16_twoSamplesPerLine {
    my ($fileToParse) = @_ ;
    rename($fileToParse, $fileToParse . '.beforeInt16EditResult');
    open(IN, '<' . $fileToParse . '.beforeInt16EditResult') or die $!;
    open(OUT, '>' . $fileToParse) or die $!;
    my $counter = 1;
    while (<IN>) {
        #print "in is ${_}ok and is num $counter\n";
        s/\s?\n/ /g if $counter % 2;
        print OUT $_;
        #print "out is ${_}ok\n";
        $counter = ( $counter + 1 );
    }
    close(OUT)
        or die "couldn't close OUT";


}

sub doSamplePerLine {
    my ($fileToParse) = @_ ;

    open(IN, '<' . $fileToParse) or die $!;
    # rename($fileToParse, $fileToParse . '.plio');
    my $samplePerLineFile = $fileToParse . ".samplePerLine";
    open(OUT, '>' . $samplePerLineFile) or die $!;
    my $count = 0;
    my $line = "";
    while (my $row = <IN>) {
        chomp $row;
        if ($count % 2 == 0){
            $line = "$row";
        } else {
            print OUT "$line$row\n";
        }
        $count++;
    }
    close(OUT)
        or die "couldn't close OUT";
    close(IN)
        or die "couldn't close IN";
    rename($samplePerLineFile, $fileToParse);
}

sub undoSamplePerLine {
    my ($fileToParse) = @_ ;
    open(IN, '<' . $fileToParse) or die $!;
    my $plio32File = $fileToParse . ".plio";
    open(OUT, '>' . $plio32File) or die $!;
    my $line = "";
    while (my $row = <IN>) {
        chomp $row;
        $line = "$row";
        my @sample = split(' ', $row);
        foreach my $i (@sample) {
            print OUT "$i\n";
        }

    }
    close(OUT)
        or die "couldn't close OUT";
    rename($plio32File, $fileToParse);
}