#!/bin/bash
EXE_FILE=$2
LIB_PROJ_ROOT=$3
XCLBIN_FILE_C=$4
XCLBIN_FILE_D=$5
echo "XCL_MODE=${XCL_EMULATION_MODE}"
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] 
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt ./sample_run.txt
    cp $LIB_PROJ_ROOT/common/data/test.list ./test.list
    for ((i = 0 ; i < 10 ; i++))
    do
        find ./reports/ -type f | xargs cat >> ./sample_run.txt
    done

    echo -e "\n\n-----------Running only Compression-----------\n"
    cmd1="$EXE_FILE -c ./sample_run.txt -cx $XCLBIN_FILE_C"
    echo $cmd1
    $cmd1
    echo -e "\n\n-----------Running only Decompression-----------\n"
    cmd2="$EXE_FILE -d ./sample_run.txt.lz4 -dx $XCLBIN_FILE_D"
    echo $cmd2
    $cmd2
    echo -e "\n\n-----------Running both Compression and Decompression-----------\n"
    cmd2="$EXE_FILE -l ./test.list -cx $XCLBIN_FILE_C -dx $XCLBIN_FILE_D"
    echo $cmd2
    $cmd2
    echo -e "\n\n-----------Block Size: 256Kb-----------\n"
    cmd1="$EXE_FILE -c ./sample_run.txt -cx $XCLBIN_FILE_C -B 1"
    echo $cmd1
    $cmd1
    echo -e "\n\n-----------Block Size: 1024Kb-----------\n"
    cmd1="$EXE_FILE -c ./sample_run.txt -cx $XCLBIN_FILE_C -B 2"
    echo $cmd1
    $cmd1
    echo -e "\n\n-----------Block Size: 4096Kb-----------\n"
    cmd1="$EXE_FILE -c ./sample_run.txt -cx $XCLBIN_FILE_C -B 3"
    echo $cmd1
    $cmd1
fi
