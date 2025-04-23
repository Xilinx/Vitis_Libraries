#!/bin/bash
EXE_FILE=$1
LIB_PROJ_ROOT=$2
XCLBIN_FILE=$3
echo "XCL_MODE=${XCL_EMULATION_MODE}"
export XILINX_LIBZ_XCLBIN=$XCLBIN_FILE
if [ "${XCL_EMULATION_MODE}" != "hw_emu" ] ;  
then
    cp $LIB_PROJ_ROOT/common/data/sample.txt sample_run.txt
    cp $LIB_PROJ_ROOT/common/data/test.list test.list
    echo "sample_run.txt.gz" > gzip_test_decomp.list
    echo "sample_run.txt.xz" > zlib_test_decomp.list
    find ./reports/ -type f | xargs cat >> sample_run.txt
    
    for ((i = 0 ; i < 10 ; i++)) 
    do
	cat sample_run.txt >> sample_run.txt${i}
        echo "sample_run.txt${i}"  >> test.list
        echo "sample_run.txt${i}.gz"  >> gzip_test_decomp.list
        echo "sample_run.txt${i}.xz"  >> zlib_test_decomp.list
    done

echo -e "\n\n-----------ZLIB Flow-----------\n"
    cmd1="$EXE_FILE -xbin $XCLBIN_FILE -t sample.txt -zlib 1"
    echo $cmd1
    $cmd1

echo -e "\n\n-----------GZIP Flow (-xbin option)-----------\n"
    cmd2="$EXE_FILE -xbin $XCLBIN_FILE -t sample.txt"
    echo $cmd2
    $cmd2

echo -e "\n\n-----------GZIP Compress list of files -----------\n"
    cmd2="$EXE_FILE -xbin $XCLBIN_FILE -cfl ./test.list"
    echo $cmd2
    $cmd2

echo -e "\n\n-----------ZLIB Compress list of files -----------\n"
    cmd2="$EXE_FILE -xbin $XCLBIN_FILE -cfl ./test.list -zlib 1"
    echo $cmd2
    $cmd2

echo -e "\n\n-----------GZIP Decompress list of files -----------\n"
    cmd2="$EXE_FILE -xbin $XCLBIN_FILE -dfl ./gzip_test_decomp.list"
    echo $cmd2
    $cmd2

echo -e "\n\n-----------ZLIB Decompress list of files -----------\n"
    cmd2="$EXE_FILE -xbin $XCLBIN_FILE -dfl ./zlib_test_decomp.list"
    echo $cmd2
    $cmd2
fi
