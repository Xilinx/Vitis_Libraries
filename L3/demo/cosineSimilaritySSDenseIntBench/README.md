# Xilinx Single Source Cosine Similarity Demo

This is a demo of running Graph L3 library. It has two main steps, firstly `Build dynamic library and xclbin`, it may take several hours. Once the xclbin is ready, users should follow steps in the `Run demo`.

## Build dynamic library and xclbin
    ./build.sh

## Run demo
    change PROJECTPATH in ../../tests/cosineSimilaritySSDenseInt/config.json to graph library's absolute path 
    ./run.sh

